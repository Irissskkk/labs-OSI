#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>

#if defined (WIN32) || (_WIN64)

#include <windows.h>
#define pthread_t DWORD
#define pthread_create(THREAD_ID_PTR, ATTR, ROUTINE, PARAMS) CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)ROUTINE,(void*)PARAMS,0,THREAD_ID_PTR)
#define sleep(ms) Sleep(ms)

#else // Linux

#include <pthread.h>
#include <unistd.h>

#endif


typedef struct TASK
{
	int low;
	int high;
	int busy;
	int* a;
}TASK;


void merge(int* a, int low, int mid, int high)
{

	int n1 = mid - low + 1;
	int n2 = high - mid;

	int* left = (int*)malloc(n1 * sizeof(int));
	int* right = (int*)malloc(n2 * sizeof(int));

	int i;
	int j;

	
	for (i = 0; i < n1; i++)
		left[i] = a[i + low];

	
	for (i = 0; i < n2; i++)
		right[i] = a[i + mid + 1];

	int k = low;

	i = j = 0;

	
	while (i < n1 && j < n2)
	{
		if (left[i] <= right[j])
			a[k++] = left[i++];
		else
			a[k++] = right[j++];
	}

	
	while (i < n1)
		a[k++] = left[i++];

	
	while (j < n2)
		a[k++] = right[j++];

	free(left);
	free(right);
}

void merge_sort(int* a, int low, int high)
{
	int mid = low + (high - low) / 2;

	if (low < high)
	{
		merge_sort(a, low, mid);

		merge_sort(a, mid + 1, high);

		merge(a, low, mid, high);
	}
}


void* merge_sort_thread(void* arg)
{
	TASK* task = (TASK*)arg;
	int low;
	int high;

	
	low = task->low;
	high = task->high;

	int mid = low + (high - low) / 2;

	if (low < high)
	{
		merge_sort(task->a, low, mid);
		merge_sort(task->a, mid + 1, high);
		merge(task->a, low, mid, high);
	}
	task->busy = 0;
	return 0;
}


int main(int argc, char** argv)
{
	char* sz;

	int MAX_ARRAY_ELEMENTS = 100000000;
	int MAX_THREADS = 1;
	
	char msg[1024];
	uint32_t msg_len;

	if (argc < 3) {
		msg_len = snprintf(msg, sizeof(msg) - 1, "usage: %s array_count thread_count\n", argv[0]);
		write(STDERR_FILENO, msg, msg_len);
		exit(EXIT_SUCCESS);
	}
	
	if (argc == 3)
	{
		MAX_ARRAY_ELEMENTS = atoi(argv[1]);
		MAX_THREADS = atoi(argv[2]);
	}
	
	float time_sec = (float)clock() / CLOCKS_PER_SEC;
	long int start_time;
	start_time = time(NULL);
	msg_len = snprintf(msg, sizeof(msg) - 1, "Now time is: %s", ctime(&start_time));
	write(STDERR_FILENO, msg, msg_len);
	
	msg_len = snprintf(msg, sizeof(msg) - 1, "Array[%d]\nThreads[%d]\n", MAX_ARRAY_ELEMENTS, MAX_THREADS);
	write(STDERR_FILENO, msg, msg_len);

	int* array = (int*)malloc(sizeof(int) * MAX_ARRAY_ELEMENTS);

	clock_t time_start = clock();

	srand(time_start);
	int i;
	for (i = 0; i < MAX_ARRAY_ELEMENTS; i++)
		array[i] = rand();

	msg_len = snprintf(msg, sizeof(msg) - 1, "Array Randomized\n");
	write(STDERR_FILENO, msg, msg_len);

	pthread_t* threads = (pthread_t*)malloc(sizeof(pthread_t) * MAX_THREADS);
	TASK* tasklist = (TASK*)malloc(sizeof(TASK) * MAX_THREADS);

	
	int len = MAX_ARRAY_ELEMENTS / MAX_THREADS;

	TASK* task;
	int low = 0;

	
	for (i = 0; i < MAX_THREADS; i++, low += len)
	{
		task = &tasklist[i];
		task->a = array;
		task->busy = 1;
		
		task->low = low;
		task->high = low + len - 1;
		if (i == (MAX_THREADS - 1))
			task->high = MAX_ARRAY_ELEMENTS - 1;
		
		pthread_create(&threads[i], 0, merge_sort_thread, task);
	}

#if defined (WIN32) || (_WIN64)
	// ожидаем выполнение всех потоков для windows
	for (i = 0; i < MAX_THREADS; i++)
		while (tasklist[i].busy)
			sleep(10);
#else // Linux
	// ожидаем выполнение всех потоков
	// wait for all threads
	for(i = 0; i < MAX_THREADS; i++)
		pthread_join(threads[i], NULL);
#endif		
 
 	
	TASK* taskm = &tasklist[0];
	for (i = 1; i < MAX_THREADS; i++)
	{
		TASK* task = &tasklist[i];
		merge(taskm->a, taskm->low, task->low - 1, task->high);
	}

	
	int last = 0;
	for (i = 0; i < MAX_ARRAY_ELEMENTS; i++)
	{
		if (array[i] < last)
		{
			msg_len = snprintf(msg, sizeof(msg) - 1, "Array is not sorted!\n");
			write(STDERR_FILENO, msg, msg_len);
			return 0;
		}
		last = array[i];
	}

	long int end_time = time(NULL);
	msg_len = snprintf(msg, sizeof(msg) - 1, "Now time is: %s", ctime(&end_time));
	write(STDERR_FILENO, msg, msg_len);
	msg_len = snprintf(msg, sizeof(msg) - 1, "Array sorted in %ld Seconds\n", time(NULL) - start_time);
	write(STDERR_FILENO, msg, msg_len);

	
	free(tasklist);
	free(threads);

	return 0;
}
