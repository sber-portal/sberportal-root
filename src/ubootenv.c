// ubootenv — правка u-boot environment (env-партиция) с пересчётом CRC32.
// usage: ubootenv <device> dump
//        ubootenv <device> set <name> <value>
//        ubootenv <device> del <name>
// env layout: [crc32 4B LE][data 65532B], data = ("name=value\0")* , добито нулями.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>

#define ENV_SIZE 0x10000
#define CRC_LEN  (ENV_SIZE - 4)

static uint32_t tbl[256];
static void crc_init(void){ for (uint32_t i=0;i<256;i++){uint32_t c=i;for(int k=0;k<8;k++)c=(c&1)?(0xEDB88320u^(c>>1)):(c>>1);tbl[i]=c;} }
static uint32_t crc32_(const uint8_t*b,size_t n){uint32_t c=0xFFFFFFFFu;for(size_t i=0;i<n;i++)c=tbl[(c^b[i])&0xff]^(c>>8);return c^0xFFFFFFFFu;}

int main(int argc,char**argv){
    if(argc<3){fprintf(stderr,"usage: %s <dev> dump|set|del ...\n",argv[0]);return 2;}
    crc_init();
    int fd=open(argv[1],O_RDWR);
    if(fd<0){perror("open");return 1;}
    uint8_t buf[ENV_SIZE];
    if(pread(fd,buf,ENV_SIZE,0)!=(ssize_t)ENV_SIZE){perror("pread");return 1;}
    uint32_t stored=*(uint32_t*)buf;
    uint32_t calc=crc32_(buf+4,CRC_LEN);
    fprintf(stderr,"crc: stored=%08x calc=%08x %s\n",stored,calc,stored==calc?"OK":"MISMATCH");

    if(!strcmp(argv[2],"dump")){ fwrite(buf+4,1,CRC_LEN,stdout); return 0; }

    char* data=(char*)(buf+4);
    size_t end=0;
    while(end+1<CRC_LEN && !(data[end]==0 && data[end+1]==0)) end+=strlen(data+end)+1;

    static char* vars[4096];
    static char store[3*ENV_SIZE];
    int nv=0; size_t sp=0;
    for(size_t i=0;i<end && nv<4096;){ char* s=data+i; size_t l=strlen(s)+1; memcpy(store+sp,s,l); vars[nv++]=store+sp; sp+=l; i+=l; }

    size_t nlen=strlen(argv[3]);
    int found=-1;
    for(int i=0;i<nv;i++){ if(!strncmp(vars[i],argv[3],nlen) && vars[i][nlen]=='='){found=i;break;} }

    if(!strcmp(argv[2],"del")){
        if(found>=0){ for(int i=found;i<nv-1;i++) vars[i]=vars[i+1]; nv--; }
    } else if(!strcmp(argv[2],"set")){
        if(argc<5){fprintf(stderr,"need value\n");return 2;}
        static char newvar[8192]; snprintf(newvar,sizeof newvar,"%s=%s",argv[3],argv[4]);
        if(found>=0) vars[found]=newvar;
        else if(nv<4096) vars[nv++]=newvar;
    } else { fprintf(stderr,"bad cmd\n"); return 2; }

    static uint8_t out[CRC_LEN];
    memset(out,0,CRC_LEN);
    size_t pos=0;
    for(int i=0;i<nv;i++){ size_t l=strlen(vars[i])+1; if(pos+l>CRC_LEN){fprintf(stderr,"ENV overflow\n");return 1;} memcpy(out+pos,vars[i],l); pos+=l; }
    uint32_t c=crc32_(out,CRC_LEN);
    memcpy(buf,&c,4); memcpy(buf+4,out,CRC_LEN);
    if(pwrite(fd,buf,ENV_SIZE,0)!=(ssize_t)ENV_SIZE){perror("pwrite");return 1;}
    fsync(fd);
    fprintf(stderr,"OK: env updated (%s=%s) nv=%d datalen=%zu newcrc=%08x\n",argv[3],argc>4?argv[4]:"",nv,pos,c);
    return 0;
}
