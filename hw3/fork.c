#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<signal.h>

int main(int argc, char *argv[]) {
	int pid;
    FILE *fp;
    
    fp = fopen("fork-output.txt", "w");
	printf("Starting program; process has pid %d\n", getpid());

	fprintf(fp, "BEFORE FORK\n");
	fflush(fp);

	if ((pid = fork()) < 0) {
		fprintf(stderr, "Could not fork()");
		exit(1);
	}

	/* BEGIN SECTION A */
    fprintf(fp, "Section A\n");
    fflush(fp);
//	printf("Section A;  pid %d\n", getpid());
	//sleep(30);

	/* END SECTION A */
	if (pid == 0) {
		/* BEGIN SECTION B */

//		printf("Section B\n");
        fprintf(fp, "Section B\n");
        fflush(fp);
		//sleep(30);
		printf("Section B done sleeping\n");
        
		exit(0);

		/* END SECTION B */
	} else {
		/* BEGIN SECTION C */
        
//		printf("Section C\n");
        fprintf(fp, "Section C\n");
	fflush(fp);
		//sleep(30);
        wait(0);
		//sleep(30);
		printf("Section C done sleeping\n");

		exit(0);

		/* END SECTION C */
	}
	/* BEGIN SECTION D */

	printf("Section D\n");
    fprintf(fp, "Section D\n");
    fflush(fp);
	//sleep(30);

	/* END SECTION D */
}
