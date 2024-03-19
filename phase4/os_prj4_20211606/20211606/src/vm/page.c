#include <stdbool.h>
#include <stdio.h>
#include "vm/page.h"
#include "threads/malloc.h"
#include "userprog/pagedir.h"
#include "vm/frame.h"
#include "filesys/file.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/palloc.h"
#include "threads/synch.h"
#include "userprog/process.h"
#include "userprog/syscall.h"
#include "lib/kernel/list.h"

static unsigned hash_func(const struct hash_elem *e, void *aux UNUSED);
static bool less_func(const struct hash_elem *e, const struct hash_elem *ee, void *aux UNUSED);
static void destructor(struct hash_elem *he, void *aux UNUSED);
void init_page (struct hash *sup_page_table)
{
    hash_init(sup_page_table, hash_func, less_func, NULL);
}
bool alloc_page(struct hash *sup_page_table, struct spte *sup_page_table_entry)
{
    if(hash_insert(sup_page_table, &sup_page_table_entry->he) == NULL) return true;
    else return false;
}
struct spte *get_supple_page(void *u_vaddr)
{
    struct spte sup_page_table;
    sup_page_table.u_vaddr = pg_round_down(u_vaddr);
    struct hash_elem* hash_e = hash_find(&thread_current()->h_e, &sup_page_table.he); 
    if(hash_e == NULL) return NULL;
    else return hash_entry(hash_e, struct spte, he);
}
void free_spt(struct hash *sup_page_table)
{
    hash_destroy(sup_page_table, destructor);
}
static void destructor(struct hash_elem *he, void *aux UNUSED)
{
    struct spte *sup_page_table = hash_entry(he, struct spte, he);
    if(sup_page_table->mem_load)
    {
        free_frame(pagedir_get_page(thread_current()->pagedir, sup_page_table->u_vaddr));
        pagedir_clear_page(thread_current()->pagedir, sup_page_table->u_vaddr);
    }
    free(sup_page_table);
}
bool load_page(void *address, struct spte *sup_page_table)
{
    if(file_read_at(sup_page_table->f, address, sup_page_table->read_bytes, sup_page_table->offset) == (off_t)sup_page_table->read_bytes)
    {
        memset(address + sup_page_table->read_bytes, 0, sup_page_table->zero_bytes);
        return true;
    }
    else return false;
}
static unsigned hash_func(const struct hash_elem *e, void *aux UNUSED)
{
    return hash_int((int)(hash_entry(e, struct spte, he)->u_vaddr));
}
static bool less_func(const struct hash_elem *e, const struct hash_elem *ee, void *aux UNUSED)
{
    if(hash_entry(e, struct spte, he)->u_vaddr < hash_entry(ee, struct spte, he)->u_vaddr) return true;
    else return false;
}
