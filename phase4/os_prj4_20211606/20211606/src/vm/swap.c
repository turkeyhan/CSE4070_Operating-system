#include "vm/page.h"
#include "vm/swap.h"
#include "vm/frame.h"
#include "threads/synch.h"
#include "devices/block.h"
#include "lib/kernel/bitmap.h"
#define SECTORS_PER_PAGE 8
// #define SECTORS_PER_PAGE (PGSIZE / BLOCK_SECTOR_SIZE)
#define POS 1
#define NEG 0

struct lock sw_lock;
struct block *sw_block;
struct bitmap *sw_table;

void init_swap(void)
{
    sw_block = block_get_role(BLOCK_SWAP);
    if(sw_block == NULL) return;
    sw_table = bitmap_create(block_size(sw_block) / SECTORS_PER_PAGE);
    if(sw_table == NULL) return;
    bitmap_set_all(sw_table, NEG);
    lock_init(&sw_lock);
}
void in_swap(size_t idx, void *fr)
{
    lock_acquire(&sw_lock);
    
    if(bitmap_test(sw_table, idx) == NEG) return;
    for(int i = 0; i < SECTORS_PER_PAGE; i++) block_read(sw_block, SECTORS_PER_PAGE * idx + i, (uint8_t *)fr + BLOCK_SECTOR_SIZE * i);
    bitmap_flip(sw_table, idx);
    
    lock_release(&sw_lock);
}
size_t out_swap(void *fr)
{
    lock_acquire(&sw_lock);
    
    size_t idx = bitmap_scan_and_flip(sw_table, NEG, POS, NEG);
    if(idx == BITMAP_ERROR) return BITMAP_ERROR;
    for(int i = 0; i < SECTORS_PER_PAGE; i++) block_write(sw_block, SECTORS_PER_PAGE * idx + i, (uint8_t *)fr + BLOCK_SECTOR_SIZE * i);
    
    lock_release(&sw_lock);
    return idx;
}