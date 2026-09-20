#ifndef MEM_H
#define MEM_H

#include "types.h"

#define HEAP_START 0x100000
#define HEAP_SIZE  (896 * 1024) // 896 KB Heap

typedef struct block {
    uint32_t      size;
    uint8_t       used;
    struct block* next;
} block_t;

void  kmem_init(void);
void* kmalloc(uint32_t size);
void  kfree(void* ptr);
void  kmem_get_stats(uint32_t* used_kb, uint32_t* free_kb, uint32_t* total_kb);

#endif

