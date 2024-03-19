#include <stdio.h>
#include "vm/frame.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "userprog/pagedir.h"
#include "userprog/syscall.h"
#include "filesys/file.h"
#include "vm/swap.h"

void init_frame(void)
{
    list_init(&fr_table);
    lock_init(&fr_table_lock);
    cur_fte = NULL;
}
struct fte* allocate_frame(enum palloc_flags flag)
{
    if(!(flag & PAL_USER)) return NULL;
    void *fr = palloc_get_page(flag);
    while(fr == NULL)
    {
        lock_acquire(&fr_table_lock);
        find_victim();
        lock_release(&fr_table_lock);
        fr = palloc_get_page(flag);
    }
    struct fte *new_fr = malloc(sizeof(struct fte));
    if(new_fr == NULL)
    {
        palloc_free_page(fr);
        return NULL;
    }
    new_fr->fr = (uint32_t*)fr;
    new_fr->fr_thread = thread_current();
    insert_frame(new_fr);
    return new_fr;
}
void insert_frame(struct fte *frame_table)
{
    if(frame_table == NULL) return;
    lock_acquire(&fr_table_lock);
    list_push_back(&fr_table, &frame_table->le);
    lock_release(&fr_table_lock);
}
void free_frame(uint32_t *fr)
{
    lock_acquire(&fr_table_lock);
    
    for(struct list_elem *e = list_begin(&fr_table); e != list_end(&fr_table); e = list_next(e))
    {
        struct fte *ft = list_entry(e, struct fte, le);
        if(ft->fr == fr)
        {
            palloc_free_page(ft->fr);
            if(ft != NULL)
            {
                if(cur_fte == ft) cur_fte = list_entry(list_remove(&ft->le), struct fte, le);
                else list_remove(&ft->le);
            }
            free(ft);
            break;
        }
    }
    lock_release(&fr_table_lock);
}
void find_victim(void)
{
    if(list_empty(&fr_table)) return;
    for(struct list_elem *e = get_elem(); e != NULL; e = get_elem())
    {
        if(e == NULL) return;
        struct fte *frame_table = list_entry(e, struct fte, le);
        if(pagedir_is_accessed(frame_table->fr_thread->pagedir, frame_table->sup_page_table->u_vaddr))
        {
            pagedir_set_accessed(frame_table->fr_thread->pagedir, frame_table->sup_page_table->u_vaddr, false);
            continue;
        }
        if(pagedir_is_dirty(frame_table->fr_thread->pagedir, frame_table->sup_page_table->u_vaddr) || frame_table->sup_page_table->t == PAGE_SWAP)
        {
            frame_table->sup_page_table->t = PAGE_SWAP;
            frame_table->sup_page_table->swap = out_swap(frame_table->fr);
        }
        frame_table->sup_page_table->mem_load = false;
        pagedir_clear_page(frame_table->fr_thread->pagedir, frame_table->sup_page_table->u_vaddr);
        palloc_free_page(frame_table->fr);
        if(cur_fte == frame_table) cur_fte = list_entry(list_remove(&frame_table->le), struct fte, le);
        else list_remove(&frame_table->le);
        free(frame_table);
        break;
    }
    return;
}
struct list_elem* get_elem(void)
{
    if(cur_fte == NULL)
    {
        struct list_elem *elem = list_begin(&fr_table);
        if(elem != list_end(&fr_table))
        {
            cur_fte = list_entry(elem, struct fte, le);
            return elem;
        }
        else return NULL;
    }
    struct list_elem *elem = list_next(&cur_fte->le);
    if(elem == list_end(&fr_table))
    {
        if(&cur_fte->le == list_begin(&fr_table)) return NULL;
        else elem = list_begin(&fr_table);
    }
    cur_fte = list_entry(elem, struct fte, le);
    return elem;
}