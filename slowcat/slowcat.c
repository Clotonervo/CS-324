#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<errno.h>


int main(int argc, char *argv[]) {
    
    // ------------------ Get/Set file input --------------
    
    char* filename = argv[1];
    char* stdin_string = "-";
    int stdin_flag = 0;
    
    if (!strcmp(stdin_string, filename)){
        stdin_flag = 1;
    }
    
    //-------------------- Get/Set slowcat_timer from environment variable or default --------
    int slowcat_timer;
    
    if (getenv("SLOWCAT_SLEEP_TIME") == NULL){
        slowcat_timer = 1;
    }
    else {
        slowcat_timer = atoi(getenv("SLOWCAT_SLEEP_TIME"));
    }
    
    
    //----------------- Get/Set Process Id -------------------
    pid_t process_id = getpid();
    
    printf("process_id = %d \n", process_id);
    
    if (stdin_flag){
        printf("STDIN input \n");
        char *line[256];
        size_t size;
        while (fgets(line, sizeof(line), stdin) != EOF){
            printf("%s", line);
            sleep(slowcat_timer);
        }
        return 0;
    }
    else {
        FILE *file = fopen(filename, "r");
        if (file == NULL){
            fprintf(stderr, "There was an issue opening %s \n", filename);
            exit(-1);
        }
        else {
            char *line[256];
            while (fgets(line, sizeof(line), file)){
                printf("%s", line);
                sleep(slowcat_timer);
            }
            fclose(file);
            return 0;
        }
    }
}
