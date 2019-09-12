#include<stdio.h>
#include<stdlib.h>
#include<string.h>

int main(int argc, char *argv[]) {
	char s1[] = "CS 324";
	char s2[] = "CS 324";

	char *e1 = s1;
//    printf("e1 = %p s1 = %p \n", s2, s1);

//    if (s1 == s2) {
//        printf("Pointers are the same!\n");
//    } else {
//        printf("Pointers are NOT the same!\n");
//    }
    
	char a1[] = { 'C', 'S', ' ', '3', '2', '4', '\0' };
	char a2[] = { 67, 83, 32, 51, 50, 52, 0 };
	char a3[] = { 0x43, 0x53, 0x20, 0x33, 0x32, 0x34, 0x00 };
	char a4[] = { '\x43', '\x53', '\x20', '\x33', '\x32', '\x34', '\x00' };

	char b1[] = { 'C', 'S', ' ', '3', '2', '4' };
	char b2[] = { 67, 83, 32, 51, 50, 52 };
	char b3[] = { 0x43, 0x53, 0x20, 0x33, 0x32, 0x34 };
	char b4[] = { '\x43', '\x53', '\x20', '\x33', '\x32', '\x34' };

	char c1[] = { 'C', 'S', '\0', '3', '2', '4', '\0' };
	char c2[] = { 'C', 'S', '\0', '4', '6', '0', '\0' };

	char d1[100];
	char d2[100];
    char z1[100];
    
	/* Just to prove a point. */
	memset(d1, 0xff, sizeof(d1));
	memset(d2, 0xff, sizeof(d2));

	d1[0] = 'C';
       	d1[1] = 'S';
       	d1[2] = ' ';
       	d1[3] =	'3';
       	d1[4] = '2';
       	d1[5] = '4';
       	d1[6] = '\0';

	d2[0] = 'C';
       	d2[1] = 'S';
       	d2[2] = ' ';
       	d2[3] =	'3';
       	d2[4] = '2';
       	d2[5] = '4';

    strcpy(c2, z1);
    
	/* Test some comparisons here */
	if (strcmp(c1, c2) == 0) {
		printf("Strings are the same!\n");
	} else {
		printf("Strings are NOT the same!\n");
	}
    
	/* Test size of arrays */
	if (sizeof(c1) == sizeof(c2)) {
		printf("Array sizes are the same!\n");
	} else {
		printf("Array sizes are NOT the same!\n");
	}
    
    printf("s1 size = %d b1 size = %d", sizeof(s1)/sizeof(char), sizeof(b1)/sizeof(char));

}
