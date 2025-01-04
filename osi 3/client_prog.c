#include <stdint.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

int main(int argc, char **argv) {
	char buf[4096];
	ssize_t bytes;

	int32_t file = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0600);
	if (file == -1) {
		const char msg[] = "error: failed to open requested file\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		exit(EXIT_FAILURE);
	}
		
	pid_t pid = getpid();
	char messg[300];
	int32_t len = snprintf(messg, sizeof(messg), "%d: Solution of expression (", pid);
	write(STDERR_FILENO, messg, len);
	write(file, messg, len);

	char* SHARED_MEMORY_OBJECT_NAME = argv[2];
	
	int i, shm, N = 0;
	float *addr;
	float d, res;

	if ( (shm = shm_open(SHARED_MEMORY_OBJECT_NAME, 0|O_RDWR, 0777)) == -1 ) {
		perror("shm_open");
		return 1;
	}

	for (i=0; i<2; i++)
	{
		addr = (float*)mmap(NULL, (N+1)*sizeof(float), PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);

		if (addr == MAP_FAILED) {
			printf("Mapping Failed\n");
			return 1;
		}

		if (i==0) 
		{
			N = (int)addr[0];
		}
	}	
	
	res = 0;
	for (i = 1; i <= N; i++) {
		d = addr[i];
		//printf("\nd=%f\n", d);
		
		if (res == 0) 
		   res = d;
		else
		   res = res / d;
   			   
		// Выводим результат преобразования на экран
		char msg[32];
		int32_t len;
		if (i==1) 
			len = snprintf(msg, sizeof(msg) - 1, "%0.3f", d);
		else
			len = snprintf(msg, sizeof(msg) - 1, " / %0.3f", d);
		write(STDERR_FILENO, msg, len);
		write(file, msg, len);
	}
	len = snprintf(messg, sizeof(messg) - 1, ") is %0.3f\n",res);
	write(STDERR_FILENO, messg, len);

	int32_t written = write(file, messg, len);
	if (written != len) {
		const char msg[] = "error: failed to write to file\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		exit(EXIT_FAILURE);
	}
	
	int err = munmap(addr, (N+1) * sizeof(float));	
	if (err != 0) {
		printf("Unmapping Failed\n");
		return 1;
	}
	shm_unlink(SHARED_MEMORY_OBJECT_NAME);	
}	
