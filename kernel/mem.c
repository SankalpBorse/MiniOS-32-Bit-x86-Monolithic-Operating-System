#include "mem.h"

static block_t* free_list = (block_t*)HEAP_START;

void kmem_init(void) {
    free_list = (block_t*)HEAP_START;
    free_list->size = HEAP_SIZE - sizeof(block_t);
    free_list->used = 0;
    free_list->next = 0;
}

void* kmalloc(uint32_t size) {
    if (size == 0) {
        return 0;
    }

    // 4-byte align the requested size
    size = (size + 3) & ~3;

    block_t* curr = free_list;
    while (curr) {
        if (!curr->used && curr->size >= size) {
            // Can we split this block?
            if (curr->size >= size + sizeof(block_t) + 4) {
                block_t* new_block = (block_t*)((uint8_t*)curr + sizeof(block_t) + size);
                new_block->size = curr->size - size - sizeof(block_t);
                new_block->used = 0;
                new_block->next = curr->next;

                curr->size = size;
                curr->next = new_block;
            }

            curr->used = 1;
            return (void*)((uint8_t*)curr + sizeof(block_t));
        }
        curr = curr->next;
    }

    // Out of memory
    return 0;
}

void kfree(void* ptr) {
    if (!ptr) {
        return;
    }

    block_t* b = (block_t*)((uint8_t*)ptr - sizeof(block_t));
    b->used = 0;

    // Coalesce adjacent free blocks
    block_t* curr = free_list;
    while (curr && curr->next) {
        if (!curr->used && !curr->next->used) {
            curr->size += sizeof(block_t) + curr->next->size;
            curr->next = curr->next->next;
        } else {
            curr = curr->next;
        }
    }
}

void kmem_get_stats(uint32_t* used_kb, uint32_t* free_kb, uint32_t* total_kb) {
    uint32_t used_bytes = 0;
    block_t* curr = free_list;

    while (curr) {
        if (curr->used) {
            used_bytes += curr->size + sizeof(block_t);
        }
        curr = curr->next;
    }

    *total_kb = HEAP_SIZE / 1024;
    *used_kb  = (used_bytes + 1023) / 1024;
    if (*used_kb > *total_kb) {
        *used_kb = *total_kb;
    }
    *free_kb  = *total_kb - *used_kb;
}

