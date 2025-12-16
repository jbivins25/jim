#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

int main()
{
    pid_t pid = fork();
    if (pid) {
        char* args[] = {"pwd", NULL};
        int status;
        waitpid(pid, &status, WUNTRACED | WCONTINUED);       
	execvp(args[0], args);
	printf("Test\n");
    }
    //else {char* args[] = {"mkdir", "test_folder", NULL}; execvp(args[0], args);}

    return 0;
}
