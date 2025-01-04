#include <stdint.h>
#include <stdbool.h>

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
	
	
	int i;
	for (i = 2; i<argc; i++)
	{
		
		char* str = argv[i];
   		char* nstr = str;
   		float d = 0, res = 0;
		int i = 0;
   		while (++i)
   		{  
       			d = strtof(nstr, &nstr);
       			if (d==0) break;

       			if (i == 1) 
       			   res = d;
       			else
       			   res = res / d;
       			   
       			
       			char msg[32];
       			int32_t len;
       			if (i==1) 
       				len = snprintf(msg, sizeof(msg) - 1, "%0.3f", d);
       			else
       				len = snprintf(msg, sizeof(msg) - 1, " / %0.3f", d);
       			write(STDERR_FILENO, msg, len);
       			write(file, msg, len);
       			
    		}
    		int32_t len = snprintf(messg, sizeof(messg) - 1, ") is %0.3f\n",res);
    		write(STDERR_FILENO, messg, len);
		int32_t written = write(file, messg, len);
		if (written != len) {
			const char msg[] = "error: failed to write to file\n";
			write(STDERR_FILENO, msg, sizeof(msg));
			exit(EXIT_FAILURE);
		}
	}
}	
