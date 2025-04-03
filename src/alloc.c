#include "alloc.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define ALIGNMENT 16 /**< The alignment of the memory blocks */

static free_block *HEAD = NULL; /**< Pointer to the first element of the free list */

/**
 * Split a free block into two blocks
 *
 * @param block The block to split
 * @param size The size of the first new split block
 * @return A pointer to the first block or NULL if the block cannot be split
 */
void *split(free_block *block, int size) {
    if((block->size < size + sizeof(free_block))) {
        return NULL;
    }

    void *split_pnt = (char *)block + size + sizeof(free_block);
    free_block *new_block = (free_block *) split_pnt;

    new_block->size = block->size - size - sizeof(free_block);
    new_block->next = block->next;

    block->size = size;

    return block;
}

/**
 * Find the previous neighbor of a block
 *
 * @param block The block to find the previous neighbor of
 * @return A pointer to the previous neighbor or NULL if there is none
 */
free_block *find_prev(free_block *block) {
    free_block *curr = HEAD;
    while(curr != NULL) {
        char *next = (char *)curr + curr->size + sizeof(free_block);
        if(next == (char *)block)
            return curr;
        curr = curr->next;
    }
    return NULL;
}

/**
 * Find the next neighbor of a block
 *
 * @param block The block to find the next neighbor of
 * @return A pointer to the next neighbor or NULL if there is none
 */
free_block *find_next(free_block *block) {
    char *block_end = (char*)block + block->size + sizeof(free_block);
    free_block *curr = HEAD;

    while(curr != NULL) {
        if((char *)curr == block_end)
            return curr;
        curr = curr->next;
    }
    return NULL;
}

/**
 * Remove a block from the free list
 *
 * @param block The block to remove
 */
void remove_free_block(free_block *block) {
    free_block *curr = HEAD;
    if(curr == block) {
        HEAD = block->next;
        return;
    }
    while(curr != NULL) {
        if(curr->next == block) {
            curr->next = block->next;
            return;
        }
        curr = curr->next;
    }
}

/**
 * Coalesce neighboring free blocks
 *
 * @param block The block to coalesce
 * @return A pointer to the first block of the coalesced blocks
 */
void *coalesce(free_block *block) {
    if (block == NULL) {
        return NULL;
    }

    free_block *prev = find_prev(block);
    free_block *next = find_next(block);

    // Coalesce with previous block if it is contiguous.
    if (prev != NULL) {
        char *end_of_prev = (char *)prev + prev->size + sizeof(free_block);
        if (end_of_prev == (char *)block) {
            prev->size += block->size + sizeof(free_block);

            // Ensure prev->next is updated to skip over 'block', only if 'block' is directly next to 'prev'.
            if (prev->next == block) {
                prev->next = block->next;
            }
            block = prev; // Update block to point to the new coalesced block.
        }
    }

    // Coalesce with next block if it is contiguous.
    if (next != NULL) {
        char *end_of_block = (char *)block + block->size + sizeof(free_block);
        if (end_of_block == (char *)next) {
            block->size += next->size + sizeof(free_block);

            // Ensure block->next is updated to skip over 'next'.
            block->next = next->next;
        }
    }

    return block;
}

/**
 * Call sbrk to get memory from the OS
 *
 * @param size The amount of memory to allocate
 * @return A pointer to the allocated memory
 */
void *do_alloc(size_t size) {

    // Give me location in memory
    void *block = sbrk(0);
    /*
    * From this address we need to see if we need to adjust memory
    * Get last digit with ampersign
    */
    intptr_t align = (intptr_t)block &(ALIGNMENT-1);

    intptr_t adjust = (align==0)?0:ALIGNMENT-align;

    int *pAllocatedMemory = sbrk(size + sizeof(header) + adjust);
    printf("doalloc: Gathered (%ld) more memory usign sbrk. New allocated memory: %p\n", size, pAllocatedMemory);

    // If returned pointer is an invalid memory address then it failed
    if (pAllocatedMemory == NULL) {
        printf("doalloc: System Break increase failed, lol\n");
        return NULL;
    }

    void *aligned = (void *)((intptr_t)pAllocatedMemory + adjust);
    header *hdr = (header *)aligned;
    hdr->size = size;
    hdr->magic = 0x01234567;


    return (char *)aligned+sizeof(header);
}

/**
 * Allocates memory for the end user
 *
 * @param size The amount of memory to allocate
 * @return A pointer to the requested block of memory
 */
void *tumalloc(size_t size) {
    free_block *curr = HEAD;
    // Check if head is Null (No free memory)
    if (HEAD == NULL) {
        // Increase memory
        int *ptr = do_alloc(size);
        // Return pointer to new memory address
        return ptr;
    }
    // If there is memory in free list (Memory available)
    else {
        // Iterate through free linked list
        while (curr != NULL) {
            // If current block is big enough for requested size
            if (size+sizeof(header) <= curr->size) {
                free_block *free = split(curr, size+sizeof(header));
                header *hdr = (header *)free;
                printf("tumalloc: Found block in free list: %p\n", hdr+1);

                if (hdr != NULL) {
                    printf("tumalloc: Successfully split block\n");
                } else {
                    printf("tumalloc: Block cannot be split\n");
                    hdr = (header *)curr;
                }
                // Remove block from free list
                remove_free_block(curr);
                hdr->size = size;
                hdr->magic = 0x01234567;
                return hdr + sizeof(header);
            }
            // If current node was not big enough go to next block
            printf("tumalloc: Current node not big enough in free list going to next block\n");
            curr = curr->next;
        }
        // If no block is big enough then increase memory
        int *ptr = do_alloc(size);
        printf("tumalloc: No block in free list big enough, increased using sbrk. New pointer: %p\n", ptr);
        return ptr;
    }
}

/**
 * Allocates and initializes a list of elements for the end user
 *
 * @param num How many elements to allocate
 * @param size The size of each element
 * @return A pointer to the requested block of initialized memory
 */
void *tucalloc(size_t num, size_t size) {
    int totalsize = num * size;

    int *memory = tumalloc(totalsize);

    return memory;
}

/**
 * Reallocates a chunk of memory with a bigger size
 *
 * @param ptr A pointer to an already allocated piece of memory
 * @param new_size The new requested size to allocate
 * @return A new pointer containing the contents of ptr, but with the new_size
 */
void *turealloc(void *ptr, size_t new_size) {
    if (ptr == NULL) {
        printf("turealloc: Pointer is null\n");
        return NULL;
    }
    
    header *hdr = (header *)(ptr-sizeof(header));

    printf("turealloc: Increasing by: %ld\n", new_size);
    int *new_chunk = tumalloc(new_size);
    memcpy(new_chunk, ptr, hdr->size);
    tufree(ptr);

    return new_chunk;
}

/**
 * Removes used chunk of memory and returns it to the free list
 *
 * @param ptr Pointer to the allocated piece of memory
 */
void tufree(void *ptr) {
    header *hdr = (header *)(ptr-sizeof(header));

    printf("tufree: Pointer for free %p\n", ptr);
    printf("tufree: Header for free %p\n", hdr);
    printf("tufree: Size of header %zu\n", sizeof(header));

    if (HEAD == NULL) {
        printf("tufree: Head is null\n");
    }
    if (ptr == NULL) {
        printf("tufree: Pointer is null\n");
    }
    printf("tufree: Header Magic %d\n", hdr->magic);
    if (hdr->magic == 0x01234567) {
        printf("tufree: Attempting to free block at %p\n", ptr);
        free_block *free = (free_block *)hdr;
        free->size = hdr->size;
        // Add to front of free list
        free->next=HEAD;
        // Set as new head
        HEAD = free;
        coalesce(free);
        printf("tufree: Memory was freed and coalesce. New free list HEAD %p\n", HEAD);
    }
    else {
        printf("tufree: Memory Corruption Detected\n");
        abort();
    }
}