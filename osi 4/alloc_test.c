#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <dlfcn.h> // dlopen, dlsym, dlclose, RTLD_*
#include "lib.c"
#include "library.h"

static alloc_func *falloc;

// инициализация аллокатора на памяти memory размера size
Allocator *allocator_create(void *const memory, const size_t size)
{
	//Allocator a = {memory, size, 0};
	static Allocator alloc;
	alloc.mem = memory;
	alloc.totalSize = size;
	alloc.offset = 0;
	
	memory_block* current_location_mcb = (memory_block*)memory;
	current_location_mcb->is_available = 1;
	current_location_mcb->size = size;
	current_location_mcb->block_num = 0;
	current_location_mcb->next = NULL;	
	
	int i;
	for (i = 0; i<POWR_N-1; i++) {
		block_list[i] = NULL;
	}
	block_list[POWR_N] = memory;
	
	return &alloc;	
}

// Инициализации памяти для Allocator-а
void* alloc_init(const size_t mem_size)
{
	int shm;
	if ( (shm = shm_open(SHARED_MEMORY_OBJECT_NAME, O_CREAT|O_RDWR, 0777)) == -1 ) {
		perror("shm_open");
		return NULL;
	} // memory = mmap(NULL, ..., MAP_ANONYMOUS...) в 
	
	// trunc file to Size bytes
	if ( ftruncate(shm, mem_size) == -1 ) {
		perror("ftruncate");
		return NULL;
	}    

	void *shm_address = mmap(NULL, mem_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);

	if (shm_address == MAP_FAILED) {
		perror("Mapping Failed");
		return NULL;
	}
	
	close(shm);	
	
	return shm_address;
}

// деинициализация структуры аллокатора
void allocator_destroy(Allocator *const allocator)
{
	int err = munmap(allocator->mem, allocator->totalSize);	
	shm_unlink(SHARED_MEMORY_OBJECT_NAME);
	if (err) {
		fprintf(stdout, "Unmapping Failed, %d\n", err);
		return;
	}
	allocator->mem = NULL;
	allocator->totalSize = 0;
	allocator->offset = 0;
}

// выделение памяти аллокатором памяти размера size (non-aligned allocation from linear buffer)
void* allocator_alloc(Allocator *const allocator, const size_t size) 
{
	if(!allocator || !size || allocator->totalSize-allocator->offset < size)
		return NULL;

	//if (allocator->offset == 0) blocknum = 0;
	
	uint32_t newOffset = allocator->offset + size + sizeof(memory_block);
	if(newOffset <= allocator->totalSize) {
		void* ptr = allocator->mem + allocator->offset;
		allocator->offset = newOffset;
		
		memory_block* current_location_mcb = (memory_block*)ptr;
		current_location_mcb->is_available = 0;
		current_location_mcb->size = size;
		current_location_mcb->block_num = blocknum;
		blocknum++; 
		
		return ptr;
	}
	return NULL; /* out of memory */
}

// возвращает выделенную память аллокатору
void allocator_free(Allocator *const allocator, void *const memory)
{
	void *current_location = allocator->mem;
	
	memory_block* current_location_mcb = (memory_block*)current_location;
	
	while (current_location < (void*)(allocator->mem + allocator->offset)) {
		current_location_mcb->is_available = 1;
		//fprintf(stdout, "Block number %d released\n", current_location_mcb->block_num);
		
		current_location += current_location_mcb->size + sizeof(memory_block);
		current_location_mcb = (memory_block*)current_location;
	}	
}

// NOTE: Functions stubs will be used, if library failed to load
// NOTE: Stubs are better than NULL function pointers,
//       you don't need to check for NULL before calling a function
static float func_impl_stub(float x) {
	(void)x; // NOTE: Compiler will warn about unused parameter otherwise
	return 0.0f;
}

int main(int argc, char **argv) {
	char *error;
	void *library = dlopen(argv[1], RTLD_LOCAL | RTLD_NOW);
	
	if (library != NULL)
	{
		falloc = dlsym(library, "twins_allocator_alloc");
		if (falloc == NULL) {
			const char msg[] = "warning: failed to find alloc function implementation\n";
			write(STDERR_FILENO, msg, sizeof(msg));
			falloc = allocator_alloc;
		}
	}
	else
	{
		fputs(dlerror(), stderr);
		const char msg[] = "\nwarning: library failed to load, trying standard implemntations\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		falloc = allocator_alloc;
	}
	

	// allocate a 1 MB region (1*1024*1024)
	size_t size_of_mem = Pow2(POWR_N);
	
	//fprintf(stdout, "Nearest %d\n", NearestPow2(mem_part + sizeof(memory_block)));
	
	void *memory = alloc_init(size_of_mem);
	
	Allocator *alloc = allocator_create(memory, size_of_mem);

	fprintf(stdout, "Size of memory %ld\n", size_of_mem);
	fprintf(stdout, "Address of memory %p\n", alloc->mem);
	//fprintf(stdout, "Size of block %ld\n", sizeof(memory_block));
	
	int mem_part;

	size_t sum_mem = 0;
	size_t free_mem = 0;
	int block_count = 0;
	void *next = (void*)1;
	srand(time(NULL));
	while (next != NULL) {
		mem_part = (rand() % 100 + 1)*32;
		//fprintf(stdout, "New block size: %d\n", mem_part);
		next = falloc(alloc, mem_part);
		if (next != NULL) {
			memory_block* current_location_mcb = (memory_block*)next;
			//fprintf(stdout, "(found block size [%d], is available [%d])\n", current_location_mcb->size, current_location_mcb->is_available);
			sum_mem += current_location_mcb->size;
			block_count++;
		}
		//else fprintf(stdout, "Out of memory\n");
		//fprintf(stdout, "Used memory: %ld\n", sum_mem);
	}

	// calc free memory
	int i;
	for (i = 0; i<=POWR_N; i++) {
		//fprintf(stdout, "block_list[%d] address: %p\n", i, block_list[i]);
		if (block_list[i] != NULL) {
			void* cur_ptr = block_list[i];
			while (cur_ptr != NULL) {
				memory_block* current_location_mcb = (memory_block*)cur_ptr;
				free_mem += current_location_mcb->size;
				//fprintf(stdout, "(block size [%d], is available [%d])\n", current_location_mcb->size, current_location_mcb->is_available);
				current_location_mcb = current_location_mcb->next;
				if (cur_ptr == (void*)current_location_mcb) break;
				cur_ptr = (void*)current_location_mcb;
			}
		}
	}
	fprintf(stdout, "Used memory: %ld\n", sum_mem);
	fprintf(stdout, "Free memory: %ld\n", free_mem);
	fprintf(stdout, "Total memory: %ld\n", sum_mem+free_mem);
	fprintf(stdout, "Count of blocks: %d\n", block_count);

	allocator_free(alloc, memory);
	allocator_destroy(alloc);

	return 0;
}

