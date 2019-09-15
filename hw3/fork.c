#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<signal.h>

int main(int argc, char *argv[]) {
	int pid;
    FILE *fp;
    FILE *fp2;
    int p[2];
    FILE *stream;
    
    fp = fopen("fork-output.txt", "w");
	printf("Starting program; process has pid %d\n", getpid());

	fprintf(fp, "BEFORE FORK\n");
	fflush(fp);
    
    pipe(p);

	if ((pid = fork()) < 0) {
		fprintf(stderr, "Could not fork()");
		exit(1);
	}

	/* BEGIN SECTION A */
    fprintf(fp, "Section A\n");
    fflush(fp);
    printf("Section A;  pid %d\n", getpid());

	/* END SECTION A */
	if (pid == 0) {
		/* BEGIN SECTION B */

        close(p[0]);
        stream = fdopen(p[1], "w");
        fputs("hello from Section B\n", stream);
        
		printf("Section B\n");
        fprintf(fp, "Section B\n");
        fflush(fp);
        
		printf("Section B done sleeping\n");
        
        char *newenviron[] = { NULL };
        
        printf("Program \"%s\" has pid %d. Sleeping.\n", argv[0], getpid());
        
        if (argc <= 1) {
            printf("No program to exec.  Exiting...\n");
            exit(0);
        }
        
        printf("Running exec of \"%s\"\n", argv[1]);
        dup2(fileno(fp), STDOUT_FILENO);
        fclose(fp);
        execve(argv[1], &argv[1], newenviron);
        printf("End of program \"%s\".\n", argv[0]);
        
		exit(0);

		/* END SECTION B */
	} else {
		/* BEGIN SECTION C */
        
		printf("Section C\n");
        fprintf(fp, "Section C\n");
        fflush(fp);
        
        close(p[1]);
        stream = fdopen(p[0], "r");
        char readString [100];
        fgets(readString, 100, stream);
        printf("%s", readString);

        wait(0);
		printf("Section C done sleeping\n");

		exit(0);

		/* END SECTION C */
	}
	/* BEGIN SECTION D */

	printf("Section D\n");
    fprintf(fp, "Section D\n");
    fflush(fp);

	/* END SECTION D */
}
