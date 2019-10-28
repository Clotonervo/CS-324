#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


int main(int argc, char** argv)
{
    int p[2];
    char* first_pipe[20] = {0};
    char* second_pipe[20] = {0};
    int second_half = 0; 
    int index = 0;
    int first_size = 0;
    int second_size = 0;

    for (int i = 1; i < argc; i++){
        if (strcmp(argv[i], "PIPE") == 0){
            second_half = 1;
            first_size = i - 1;
            i++;
        }

        if (second_half){
            second_pipe[index] = argv[i];
            second_pipe[i+1] = 0;
            index++;
            second_size++;
        }
        else {
            first_pipe[i-1] = argv[i];
            first_pipe[i] = 0;
        }
    }

    // for(int i = 0; i < first_size; i++){
    //     printf("first_pipe = %s\n", first_pipe[i]);
    // }

    // for(int i = 0; i < second_size; i++){
    //     printf("second_pipe = %s\n", second_pipe[i]);
    // }

    pipe(p);
    char *newenviron[] = { NULL };

    if(fork() == 0)           
    {
        dup2(p[1], 1);         //replacing stdout with pipe write 
        close(p[0]);       //closing pipe read
        execve(first_pipe[0], first_pipe, newenviron);
        exit(1);
    }

    if(fork() == 0)            
    {     
        dup2(p[0], 0);       //replacing stdin with pipe read
        close(p[1]);        //closing pipe write

        execve(second_pipe[0], second_pipe, newenviron);
        exit(1);
    }

    close(p[0]);
    close(p[1]);
    wait(0);
    wait(0);
    return 0;
}