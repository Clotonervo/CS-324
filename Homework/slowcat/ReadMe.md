Slowcat

In the first part of this assignment, you will learn about how strings are stored and compared as arrays in C.  Reference the attached code, strings.c, and answer the following questions about it in a file called hw2.txt.  Note that you will see deliberate patterns in the code to help you learn concepts related to strings, pointers, and arrays (i.e., I'm not trying to trick you, but to teach you).

You can manipulate the code, compile it, and run it however you would like.  You will not be turning in the code; it is simply a tool to help you learn the concepts of C strings.

You might also find the following resource helpful: Strings in C

1. Of the following, which of the arrays have the same total bytes allocated AND have the same contents as s1 (indicate all that apply)?

s2
a1, a2, a3, a4
b1, b2, b3, b4
d1, d2

Hint: You can pass an array to sizeof() to return the total number of bytes allocated for that array.  You can pass a type to sizeof() to return the total number of bytes associated with that type.  If you divide the total number of bytes allocated for a given array by the size of the type contained in the array, then you will get the total number of elements in the array.  For example:

int foo[100];
int num_elements = sizeof(foo)/sizeof(int);

In this case, sizeof(foo) is 400 and sizeof(int) is 4, so num_elements is 100.


2. Which of the following pairs of pointer values are the same (indicate all that apply)?

s1 and s2
s1 and e1

What expression do you use to compare these pointer values?  Will this same expression always work for comparing the string values (i.e., array content) of two strings?


3. Which of the following arrays can be considered "strings" for the purposes of using them as arguments to strcpy(), strncpy(), strcmp(), strncmp(), and similar string functions (indicate all the apply)?


s1, s2,
a1, a2, a3, a4,
b1, b2, b3, b4,
c1, c2,
d1, d2,
e1

What makes an array a "string" for use with these string functions?


4. Which of the following arrays can be used as arguments to memcpy() and memcmp()?


s1, s2,
a1, a2, a3, a4,
b1, b2, b3, b4,
c1, c2,
d1, d2,
e1

5. Which of the following pairs will always result in strcmp() returning a 0 value (i.e., that the strings are equivalent)?

s1, s2
s1, a1
s1, b1
c1, c2
s1, d1
s1, e1
For those pairs for which strcmp() returns a non-zero value (i.e., strings are not equivalent), explain why, in just a few words.

Part 2 (20 points):

In the second part of this assignment, you will learn and practice:

- Coding and compilation in C, including basic Input/Output
- Accessing environment variables
- Process ID

For this lab you will implement a basic cat-like program.

The usage is as follows:

./slowcat filename

which means that it will take a single filename as input on the command line.

It will determine the value sleep_time by getting the SLOWCAT_SLEEP_TIME environment variable from the environment (see the getenv() function).  If not specified, the sleep time is 1.
Open the file (see the fopen() function); if the filename is "-", then use standard input (stdin).
Print the process ID of the current process to stderr (see the getpid() function).
For each line in the file:
a) read the line (see the fgets() function);
b) print the line (see the fprintf() function); and
c) sleep for sleep_time seconds (see the sleep() function).
Save your file as slowcat.c, and use the command 'gcc -o slowcat slowcat.c' to compile your program.  Here is the file to use for your testing: test.txt  Download 
 
Helps:

Read and use the man page for each of the functions referenced above in the specification.  It will make your life easier.
For more on setting and reading environment variables, read and reference the ENVIRONMENT section in the bash man page.
To turn in:

Please include 1) the answers from questions 1 through 5 from part 1, in a file called hw2.txt; and 2) your source code, slowcat.c, from part 2.  Use the following command to tar/gzip your files:

tar -zcvf hw2.tar.gz slowcat.c hw2.txt

Submit hw2.tar.gz to LearningSuite.