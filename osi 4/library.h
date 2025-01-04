#ifndef __LIBRARY_H
#define __LIBRARY_H

#define SHARED_MEMORY_OBJECT_NAME "my_shared_memory"

#define POWR_N 20 // размерность выделяемой памяти 2^POWR_N

// Структура Memory Block
typedef struct mem_block {
	int is_available;
	int size;
	int block_num;	
	struct mem_block* next;
} memory_block;

// Header structure for linear buffer
typedef struct _Allocator {
	uint8_t *mem;       /*!< Pointer to buffer memory. */
	uint32_t totalSize; /*!< Total size in bytes. */
	uint32_t offset;    /*!< Offset. */
} Allocator;

int Pow2(int n) {
	return 1<<n;
}

int NearestPow2(int x) {
	int y = 1;
	int pwr = 0;
	while (y < x) {
		y *= 2;
		pwr++;
	}
	return pwr;
}

int blocknum = 0;

typedef void* alloc_func(Allocator* const allocator, const size_t size);

#endif // __LIBRARY_H
