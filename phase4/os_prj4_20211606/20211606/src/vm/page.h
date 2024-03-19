#ifndef PAGE_H
#define PAGE_H

#include <hash.h>


/* Process identifier. */
// typedef int pid_t;
// #define PID_ERROR ((pid_t) -1)

// /* Map region identifier. */
// typedef int mapid_t;
// #define MAP_FAILED ((mapid_t) -1)
/*Type of page*/
enum top 
{
    PAGE_FILE,
    PAGE_SWAP
};

/*Supplementary page table struct*/
struct spte
{
    /*Page type*/
    enum top t;
    
    struct hash_elem he;

    struct file *f;
    size_t offset;
    size_t read_bytes;
    size_t zero_bytes;
    size_t swap;

    void *u_vaddr;
    
    bool read_only;
    bool mem_load;
};

void init_page (struct hash *sup_page_table);
bool alloc_page(struct hash *sup_page_table, struct spte *sup_page_table_entry);
struct spte *get_supple_page(void *u_vaddr);
void free_spt(struct hash *sup_page_table);
bool load_page(void *address, struct spte *sup_page_table);
#endif
