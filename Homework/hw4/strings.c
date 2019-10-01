#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//For this assignment, you will need to understand the following C functions:
//strcmp, strlen, strcpy, strtok, memcpy, malloc

//You may test each of these functions by running ./strings <num>, where num is a number 1-6
//corresponding to the function you want to test.


//#1: Check if str equals the string "Hello World!"
//You may assume that str is null-terminated
int isHelloWorld(char* str) {
    return (str == "Hello World!");
}

//#2: Replace str with the string "Hello World!"
//You may assume that str is long enough to hold 'Hello World!'
void fillHelloWorld(char* str) {
    str = "Hello World!";
}

//#3: Return a new string that is a copy of str
//You may assume that str is null-terminated
char* copyString(char* str) {
    char newStr[strlen(str)+1];
    strcpy(newStr, str);
    return newStr;
}

//#4: Return a new character array that is a copy of the passed-in array
//You may NOT assume that array is null-terminated
//NOTE: You will need to replicate the solution you did for Part 3 to fix an identical bug in this part.
//However, that is not the only issue with this code.
//HINT: You may need to add another argument to the function to make this work.
char* copyArray(char* array) {
    char newArray[sizeof(array)];
    memcpy(newArray, array, sizeof(array));
    return newArray;
}

//#5: Get the substring in str of the given begin index and length
//You may assume that begin+length is not greater than the length of str
//EXAMPLE: On input str='Hello World!', begin=3, length=7, the function should return the string 'lo Worl'
char* getSubstring(char* str, unsigned int begin, unsigned int length) {
    char* substr = malloc(length+1);
    memcpy(substr, str[begin], length);
    substr[length] = 0; //Null terminator
    return substr;
}

//#6: Return the first word of str (space delimited) without modifying str
//You may assume that str is null-terminated and is not empty
char* getFirstWord(char* str) {
    char* word = strtok(str, " ");
    return word;
}


void printBytes(char* bytes, int size);

//You may not change main except to add another argument to copyArray() in Test 4
int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: ./strings <num>(1-6)\n");
        return 1;
    }
    int num = atoi(argv[1]);
    char hello[strlen("Hello World!")+1];
    strcpy(hello, "Hello World!");
    char hello2[] = "Hello Worl!";
    char str[] = "The quick brown fox jumped over the lazy dog";
    const char SMSG[] = "\nTEST PASSED!\n";
    const char FMSG[] = "\nTEST FAILED!\n";
    char expected[strlen(str)+1];
    strcpy(expected, str);
    char expectedFirst[] = "The";
    char expectedSubstr[] = "lo Worl";
    char* cpy;
    char* word;
    char bytes[] = {4, 8, 15, 16, 23, 42, 0, 1, 2, 3};
    int pass, h1, h2;
    switch(num) {
        case 0:
            printf("Invalid test number: %s\n", argv[1]);
            return 1;
        case 1:
	        printf("RUNNING TEST 1 - isHelloWorld(char* str)\nCheck if str equals the string 'Hello World!'\n\n");
            h1 = isHelloWorld(hello);
            printf("Input: str='%s'\nValue returned from function: %d (%s)\n", hello, h1, h1 ? "True" : "False");
	        printf("Expected: True\n\n");
            h2 = isHelloWorld(hello2);
            printf("Input: '%s'\nValue returned from function: %d (%s)\n", hello2, h2, h2 ? "True" : "False");
	        printf("Expected: False\n");
            printf(h1 && !h2 ? SMSG : FMSG);
            return 0;
        case 2:
            printf("RUNNING TEST 2 - fillHelloWorld(char* str)\nReplace str with the string 'Hello World!'\n\n");
            printf("Input: str='%s'\n", str);
            fillHelloWorld(str);
            printf("Value of str after function call: '%s'\n", str);
	        printf("Expected: 'Hello World!'\n");
            printf(strcmp(str, "Hello World!") == 0 ? SMSG : FMSG);
            return 0;
        case 3:
            printf("RUNNING TEST 3 - copyString(char* str)\nReturn a new string that is a copy of str\n\n");
            printf("Input: str='%s'\n", str);
            cpy = copyString(str);
            printf("String returned from function: '%s'\n", cpy);
	        printf("Expected: '%s'\n", str);
	        if (str == cpy) {
		        printf("\nTEST FAILED: Input and output pointers are identical - this is not a copy\n");
		        return 0;
	        }
            printf(strcmp(cpy, str) == 0 ? SMSG : FMSG);
            return 0;
        case 4:
	        printf("RUNNING TEST 4 - copyArray(char* array)\nReturn a new character array that is a copy of the passed-in array\n\n");
            printf("Input: bytes=");
	        printBytes(bytes, sizeof(bytes));
            cpy = copyArray(bytes); //You may change this line by adding another argument to copyArray
	        printf("Array returned from function (first %ld bytes): ", sizeof(bytes));
	        printBytes(cpy, sizeof(bytes));
	        printf("Expected: ");
            printBytes(bytes, sizeof(bytes));
	        if (bytes == cpy) {
                printf("\nTEST FAILED: Input and output pointers are identical - this is not a copy\n");
                return 0;
            }
            printf(memcmp(bytes, cpy, sizeof(bytes)) == 0 ? SMSG : FMSG);
            return 0;
        case 5:
	        printf("RUNNING TEST 5 - getSubstring(char* str, unsigned int begin, unsigned int length)\nGet the substring in str of the given begin index and length\n\n");
	        printf("Input: str='%s', begin=3, length=7\n", hello);
	        word = getSubstring(hello, 3, 7);
	        printf("String returned from function: '%s'\n", word);
	        printf("Expected: '%s'\n", expectedSubstr);
	        printf(strcmp(word, expectedSubstr) == 0 ? SMSG : FMSG);
	        return 0;
	    case 6:
	        printf("RUNNING TEST 6 - getFirstWord(char* str)\nReturn the first word of str (space delimited) without modifying str\n\n");
	        printf("Input: str='%s'\n", str);
	        word = getFirstWord(str);
	        printf("String returned from function: '%s'\n", word);
            printf("Expected: '%s'\n", expectedFirst);
            printf("Value of str after function call: '%s'\n", str);
	        printf(strcmp(word, expectedFirst) != 0 ? FMSG :
                    strcmp(str, expected) != 0 ? "TEST FAILED: The input string was modified by the function\n" : SMSG);
	        return 0;
	    default:
	        printf("Invalid test number: %s\n", argv[1]);
            return 1;
    }
}

void printBytes(char* bytes, int size) {
    for (int i = 0; i < size; i++) {
	printf("%d ", bytes[i]);
    }
    printf("\n");
}
