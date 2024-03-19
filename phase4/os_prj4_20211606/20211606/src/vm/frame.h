#ifndef FILE_H
#define FILE_H
#include "lib/kernel/list.h"
#include "vm/page.h"
#include "threads/palloc.h"

struct fte
{
    void *fr;
    struct thread *fr_thread;
    struct spte* sup_page_table;
    struct list_elem le;
};
void init_frame(void);
struct fte* allocate_frame(enum palloc_flags flag);
void insert_frame(struct fte *frame_table);
void free_frame(uint32_t *fr);
void find_victim(void);
struct list_elem* get_elem(void);
#endif