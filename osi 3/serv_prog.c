#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <mach-o/dyld.h>
#include <libproc.h>

#define SHARED_MEMORY_OBJECT_NAME "my_shared_memory"

static char CLIENT_PROGRAM_NAME[] = "client_prog";

float* make_array(char* buf)
{
	float* arr = NULL;

	int i = 0;
	char* nstr = buf;
	while (++i)
	{  
		float d = 0;
		d = strtof(nstr, &nstr);
		if (d==0) break;

		float* new_arr = (float*)realloc(arr, (i+1)*sizeof(float));
		if (new_arr != NULL)
		{
			arr = new_arr;
			arr[0] = i;                    // 1-й элемент массива - его длина
			arr[i] = d;                    // добавить к массиву только что введённое число
		}
	}

	return arr;
}

int main(int argc, char **argv) {
	if (argc == 1) {
		char msg[1024];
		uint32_t len = snprintf(msg, sizeof(msg) - 1, "usage: %s filename\n", argv[0]);
		
		write(STDERR_FILENO, msg, len);
		exit(EXIT_SUCCESS);
	}

	pid_t ppid = getpid(); 

	{
		char msg[128];
		int32_t len = snprintf(msg, sizeof(msg) - 1, "%d: Start typing row of number. Press 'Ctrl-D' or 'Enter' with no input to exit\n", ppid);
		write(STDOUT_FILENO, msg, len);
	}
	
	char buf[4096];
	ssize_t bytes;
	while (bytes = read(STDIN_FILENO, buf, sizeof(buf))) {
		if (bytes < 0) {
			const char msg[] = "error: failed to read from stdin\n";
			write(STDERR_FILENO, msg, sizeof(msg));
			exit(EXIT_FAILURE);
		} else if (buf[0] == '\n' || buf[bytes-1] == '\n') {
			break;
		}
	}

	
	uint32_t ArrSize, i;
	
	float* numbers = make_array(buf);
	if (numbers != NULL)
	{
		ArrSize = (int)numbers[0];
	}
	
	// create a shared memory object to store data
	uint32_t shm;
	
	if ( (shm = shm_open(SHARED_MEMORY_OBJECT_NAME, O_CREAT|O_RDWR, 0777)) == -1 ) {
		perror("shm_open");
		return 1;
	}
 
	// trunc file to size N+1 bytes
	if ( ftruncate(shm, ArrSize+1) == -1 ) {
		perror("ftruncate");
		return 1;
	}    

	// data mapping to memory
	float *addr = (float*)mmap(NULL, (ArrSize+1) * sizeof(float), PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);
	
	if (addr == MAP_FAILED) {
		perror("Mapping Failed\n");
		return 1;
	}
	
	for (i = 0; i <= ArrSize; i++) addr[i] = (float)numbers[i];
	
	int err = munmap(addr, (ArrSize+1) * sizeof(float));
	close(shm);

	if (err != 0) {
		perror("Unmapping Failed\n");
		return 1;
	}	

	char progpath[1024];
	{
#ifdef __APPLE__
		uint32_t len = proc_pidpath(ppid, progpath, sizeof(progpath));
#else  // not __APPLE__
		// Read the symbolic link '/proc/self/exe'.
		// определить каталог с программой
		const char *linkName = "/proc/self/exe";
		uint32_t len = readlink(linkName, progpath, sizeof(progpath) - 1);
#endif // else not __APPLE__

		if (len <= 0)
		{
			const char msg[] = "error: failed to read full program path\n";
			write(STDERR_FILENO, msg, sizeof(msg));
			exit(EXIT_FAILURE);
		}

		while (progpath[len] != '/')
			--len;

		progpath[len] = 0;			
	}

	int channel[2];
	if (pipe(channel) == -1) {
		const char msg[] = "error: failed to create pipe\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		exit(EXIT_FAILURE);
	}

	const pid_t child = fork();

	switch (child) {
		case -1: { 
			const char msg[] = "error: failed to spawn new process\n";
			write(STDERR_FILENO, msg, sizeof(msg));
			exit(EXIT_FAILURE);
		} break;

		case 0: { 
			pid_t pid = getpid(); 
			
			dup2(STDIN_FILENO, channel[STDIN_FILENO]);
			close(channel[STDOUT_FILENO]);

			{
				char msg[64];
				const int32_t length = snprintf(msg, sizeof(msg), "%d: I'm a child\n", pid);
				write(STDOUT_FILENO, msg, length);
			}

			{
				char path[1024];
				snprintf(path, sizeof(path) - 1, "%s/%s", progpath, CLIENT_PROGRAM_NAME) < 0 ? abort() : (void)0;

				char *const args[] = {CLIENT_PROGRAM_NAME, argv[1], SHARED_MEMORY_OBJECT_NAME, NULL};
				int32_t status = execv(path, args);

				if (status == -1) {
					const char msg[] = "error: failed to exec into new exectuable image\n";
					write(STDERR_FILENO, msg, sizeof(msg));
					exit(EXIT_FAILURE);
				}
			}
		} break;

		default: { 
			pid_t pid = getpid(); 

			{
				char msg[64];
				const int32_t length = snprintf(msg, sizeof(msg),
					"%d: I'm a parent, my child has PID %d\n", pid, child);
				write(STDOUT_FILENO, msg, length);
			}

			
			int child_status;
			wait(&child_status);

			if (child_status != EXIT_SUCCESS) {
				const char msg[] = "error: child exited with error\n";
				write(STDERR_FILENO, msg, sizeof(msg));
				exit(child_status);
			}
		} break;
	}
}
