#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include "library.h"

// NOTE: MSVC compiler does not export symbols unless annotated
#ifdef _MSC_VER
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

// массив для хранения очередей указателей на свободные блоки
int* block_list[POWR_N+1];

// выделение памяти аллокатором памяти размера size методом близнецов (non-aligned allocation from linear buffer)
EXPORT void* twins_allocator_alloc(Allocator* const allocator, const size_t size) 
{
	if(!allocator || !size)
		return NULL;

	int i;

	static int start = 0;
	if (start==0)
	{
		for (i = 0; i<POWR_N-1; i++) {
			block_list[i] = NULL;
		}
		block_list[POWR_N] = (void*)allocator->mem;
		start = 1;
	}

	// определим размер выделяемого блока с учетом заголовка
	int power = NearestPow2(size + sizeof(memory_block));
	int blockLength = Pow2(power);
	int tmp_power = power;
	
	if (power >= POWR_N) {
		fprintf(stdout, "Max power: %d\n", POWR_N);
		perror("Out of memory");
		return NULL;
	}
	
	void* ptr = block_list[power];
	fprintf(stdout, "Power for block: %d\n", power);
	fprintf(stdout, "Power block size: %d\n", blockLength);
	fprintf(stdout, "Power block address: %p\n", block_list[power]);

	if (block_list[power] == NULL) {
		do {
			tmp_power++;
			ptr = block_list[tmp_power];
			//fprintf(stdout, "Tmp_power block address: %p\n", ptr);
		} while (tmp_power < POWR_N && block_list[tmp_power] == NULL);

		
		memory_block* current_location_mcb = (memory_block*)ptr;
		void* cur_ptr = ptr;
		while (current_location_mcb != NULL)
		{
			if (current_location_mcb->is_available > 0) break;
			current_location_mcb = current_location_mcb->next;
		}

		cur_ptr = (void*)current_location_mcb;
		
		if (tmp_power==POWR_N && cur_ptr == NULL) return NULL;

		/*fprintf(stdout, "Start block address: %p\n", cur_ptr);
		fprintf(stdout, "Start block size: %d\n", current_location_mcb->size);*/
		
		if (current_location_mcb != NULL) {
			// отметим найденный блок как занятый и разделим его пополам
			//current_location_mcb = (memory_block*)cur_ptr;
			//current_location_mcb->is_available = 1;
			
			int tmp_size = current_location_mcb->size;
			
			// надо убрать текущий блок из списка свободных блоков этого размера
			void* ppp = block_list[tmp_power];
			memory_block* ccc = (memory_block*)ppp;
			if (ppp = cur_ptr)
				block_list[tmp_power] = (void*)ccc->next;
			else
				ccc->next = NULL;
			
			do {
				tmp_power--;
				tmp_size /= 2;
				
				current_location_mcb = (memory_block*)cur_ptr;
				current_location_mcb->size = tmp_size;
				current_location_mcb->is_available = 0;
				current_location_mcb->next = NULL;

				current_location_mcb = (memory_block*)(cur_ptr+tmp_size);
				current_location_mcb->size = tmp_size;
				current_location_mcb->is_available = 1;
				current_location_mcb->next = (void*)block_list[tmp_power];
				block_list[tmp_power] = cur_ptr+tmp_size;
			} while (tmp_power > power);
			
			return cur_ptr;
		}
	}
	else
	{
		memory_block* current_location_mcb = (memory_block*)ptr;

		if (current_location_mcb != NULL) {
			current_location_mcb->is_available = 0;
			current_location_mcb->size = blockLength;
			current_location_mcb->block_num = blocknum;
			
			// надо убрать текущий блок из списка свободных блоков этого размера
			void* ppp = block_list[power];
			memory_block* ccc = (memory_block*)ppp;
			if (ppp = ptr)
				block_list[power] = (void*)ccc->next;
			else
				ccc->next = NULL;
		}
		
		return ptr;
	}

	return NULL; /* out of memory */
}


