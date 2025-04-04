# OS-MemoryAllocator
Memory Allocator written in C for a Project for an Operating Systems class.

## How to run
To run simply clone this repo with:
```bash
git clone git@github.com:mjh-TU/OS-MemoryAllocator.git
```
Then just use the `compile.sh` bash script in the root of the git directory:
```bash
./compile.sh
```

## Functions
The functions I had to implement were:
- do_alloc()
    - Gathers memory from system using built-in sbrk() and then returns the pointer to the added memory.
- tumalloc()
    - Manages memory for the user by passing in the size of memory to allocate and returns a pointer to the requested block of memory. Utilizes a first fit logic while searching the free list for a memory block. If there is no more memory in the free list then tumalloc() will then call do_alloc() to request more.
- tucalloc()
    - Allocates and initializes a list of elements given the size of each element and the number of those elements. Returns a pointer to the requested block of initialized memory. Utilizes the implemented tumalloc() function.
- turealloc()
    - Reallocates a block of memory for a block with a bigger size. Utilizes tumalloc() to allocate memory with the new requested size and then uses memcpy() to copy over the data from the previous block to the new block and then frees the old memory block. Returns a pointer to the new block.
- tufree()
    - Will free a block of memory by passing in a pointer to the block of memory that is to be free'd. Also utilizes a free list of the previous blocks that tufree() has free'd to prevent any double free's.



