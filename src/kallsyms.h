/*
 * kallsyms.h — рантайм-резолвер символов ядра из in-kernel kallsyms-таблиц.
 *
 * ЗАЧЕМ: получить точные адреса символов (commit_creds, init_cred, selinux_enforcing,
 * sel_read_handle_unknown, ...) прямо на устройстве, не завися от совпадения сборки.
 *
 * СТАТУС: заготовка. Всё, что нужно — реализовать kread64() (произвольное чтение
 * kernel-памяти). Сейчас read-примитива нет: CPU не ходит через подменённую GPU-таблицу
 * (см. VERIFY=2/4), GPU-чтения у Mali нет. Как только появится любой read — резолвер готов.
 *
 * Конфиг целевого ядра: CONFIG_KALLSYMS_ALL=y, CONFIG_KALLSYMS_BASE_RELATIVE=y.
 */
#ifndef RAVEN_KALLSYMS_H
#define RAVEN_KALLSYMS_H
#include <stdint.h>

typedef struct {
    /* читает 8 байт по kernel virtual address; реализует владелец read-примитива */
    uint64_t (*rd64)(uint64_t va);
} kread_t;

typedef struct {
    uint64_t names;          /* kallsyms_names[]        */
    uint64_t offsets;        /* kallsyms_offsets[] (s32, отн. relative_base) */
    uint64_t num_syms;       /* kallsyms_num_syms      */
    uint64_t markers;        /* kallsyms_markers[]      */
    uint64_t token_table;    /* kallsyms_token_table[]  */
    uint64_t token_index;    /* kallsyms_token_index[]  */
    uint64_t relative_base;  /* kallsyms_relative_base  */
    int      base_relative;  /* CONFIG_KALLSYMS_BASE_RELATIVE */
    unsigned nsyms;          /* значение kallsyms_num_syms */
} kallsyms_t;

/* Разложить имя символа по индексу в out (NUL-terminated). Возврат: длина. */
int kallsyms_expand(const kallsyms_t *ks, const kread_t *k, unsigned idx, char *out, int outlen);

/* Адрес символа по индексу (виртуальный). */
uint64_t kallsyms_addr(const kallsyms_t *ks, const kread_t *k, unsigned idx);

/* Линейный поиск по имени (для отладки; быстрее — по markers/сортировке). */
uint64_t kallsyms_lookup(const kallsyms_t *ks, const kread_t *k, const char *name);

/* Найти token_table сканом [start,end) (256 NUL-terminated printable-строк),
 * затем восстановить остальные таблицы. Возврат 0 при успехе. */
int kallsyms_locate_scan(kallsyms_t *ks, const kread_t *k, uint64_t start, uint64_t end);

/* Явно задать адреса таблиц, если известен hint (напр. из приблизительного System.map). */
int kallsyms_set_tables(kallsyms_t *ks, const kread_t *k,
                        uint64_t token_table, uint64_t token_index,
                        uint64_t names, uint64_t markers,
                        uint64_t num_syms, uint64_t offsets,
                        uint64_t relative_base, int base_relative);

#endif /* RAVEN_KALLSYMS_H */
