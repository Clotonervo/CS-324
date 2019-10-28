#include <unistd.h>
#include <stdlib.h>
#include <string.h>


int main(int argc, char** argv)
{
    // printf("argv = %s\n", argv[0]);
    int fd[2];
    char* first_pipe[20] = {0};
    char* second_pipe[20] = {0};
    int second_half, index = 0;

    for (int i = 0; i < argc; i++){
        // printf("%s ", argv[i]);

        if (strcmp(argv[i], "PIPE") == 0){
            second_half = 1;
            i++;
        }

        if (second_half){
            second_pipe[index] = argv[i];
            second_pipe[i+1] = 0;
            index++;
        }
        else {
            first_pipe[i] = argv[i];
            first_pipe[i+1] = 0;
        }
    }
    // printf("\n argv = %s\n", argv[0]);
    // printf("first pipe = %s\n", first_pipe[1]);
    // printf("second pipe = %s\n", second_pipe[0]);

    if(pipe(fd) == -1) {
        // perror("Pipe failed");
        exit(1);
    }

    if(fork() == 0)            //first fork
    {
        close(STDOUT_FILENO);  //closing stdout
        dup(fd[1]);         //replacing stdout with pipe write 
        close(fd[0]);       //closing pipe read
        close(fd[1]);

        // const char* prog1[] = { "ls", "-l", 0};
        // char* prog1 = first_pipe;
        execvp(first_pipe[0], first_pipe);
        // perror("execvp of ls failed");
        exit(1);
    }

    if(fork() == 0)            //creating 2nd child
    {
        close(STDIN_FILENO);   //closing stdin
        dup(fd[0]);         //replacing stdin with pipe read
        close(fd[1]);       //closing pipe write
        close(fd[0]);

        // const char* prog2[] = { "wc", "-l", 0};
        // const char* prog2 = second_pipe;
        execvp(second_pipe[0], second_pipe);
        // perror("execvp of second pipe failed");
        exit(1);
    }

    close(fd[0]);
    close(fd[1]);
    wait(0);
    wait(0);
    return 0;
}