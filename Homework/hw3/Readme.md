HW 3 - fork/exec

a. Download hw3.tar.gz and un-tar/gzip it to a directory:

hw3.tar.gz  Download 

$ tar -zxvf hw3.tar.gz
(Note: LearningSuite does something weird to my tar.gz files, so you might need to do this:

$ gunzip hw3.tar.gz; tar -xvf hw3.tar
)

b. Run "make" to build three executables: fork and exec.  These are programs that illustrate the system calls fork() and exec().

You will be doing a writeup on this assignment. Call your writeup hw3.txt. Make sure you answer any questions/provide outputs as stated in the bolded questions/statements (there should be 18 in all).


Part 1: Exec Overview

1.  Open exec.c and look at what it does. Give a description of its behavior.

2.  Under what condition will the final print statement get executed?

 

Part 2: Exec Experimentation

In the next steps, you'll be using the ps terminal command to examine how the exec process changes over time. Because of this, you'll want to read all of Parts 3 6 together before you start.

3.  Run the exec program with "/bin/cat" as an argument. Show your terminal commands and the output.

4.  In another terminal window (remember tmux?), run the ps command first during the first 30 second sleep() call, then again after the first 30 seconds is over, but before the end of the program.

Use flags for the two ps commands so that:

only the process with the PID output by exec is shown
the ps output is formatted to have the following fields:
user: the username of the user running the process
pid: the process ID of the process
ppid: the process ID of the parent process
state: the state of the process, e.g., Running, Sleep, Zombie
ucmd: the command executed
Use the man page for ps to learn how to select by PID and format the output.

Show your terminal commands and the output.

5.  What similarities/differences do you notice between the output of the two ps commands? What are the reasons for those similarities/differences?
    
(You can use ctrl-d to specify end of file (EOF), so the program will run to completion)

6.  Run exec program again, but this time using a nonexistent program as an argument. Show the output and explain what happened.

 

Part 3: Fork Overview

7.  Open fork.c and look at what it does. Give a description of its behavior.

8.  Which sections ("A", "B", "C", and "D") are run by the parent process and which are run by the child process?

 

Part 4: Fork Experimentation

In the next few steps, you'll be using ps to examine how the fork process changes over time. Because of this, you'll want to read all of 9 - 13 together before you start.

9.  Run the fork program, then in another terminal window (tmux anyone?) run the ps command with the options mentioned below, first during the initial 30 second sleep call, then again after "Section B done sleeping" is printed.

For the two ps commands, use arguments such that

only the processes with the PIDs output by fork are shown
the ps output is formatted to have the following fields:
user: the username of the user running the process
pid: the process ID of the process
ppid: the process ID of the parent process
state: the state of the process, e.g., Running, Sleep, Zombie
ucmd: the command executed
process ancestry is illustrated (use the '--forest' option)
Use the man page for ps to learn how to select by PID and format the output.

Show all 3 terminal commands and their output.

10.  What similarities/differences do you notice between the output of the two ps commands? What are the reasons for those similarities/differences?


11.  What special line of code could you add to fork.c to eliminate the process with state "Z" from the output of the ps command? Where would you put it?

Add that line to fork.c, and re-make (note: you may also need to add a few #include statements at the top of the file for it to compile properly--see the man page for which).

Check to see that your fork still runs, then comment out the line of code temporarily until a later part of the assignment.


12.  Modify fork.c, so that:

Section B has two 30 second sleep calls.
Section C has only one 30 second sleep call.
Run the fork program, then in another terminal window (you guessed it!) run the ps command with the same options mentioned earlier, first during the initial 30 second sleep call, then again after "Section C done sleeping" is printed.

Show the terminal commands you used and their outputs.

13.  What similarities/differences do you notice between the output of the two ps commands? What are the reasons for those similarities/differences?

 

Part 5: Streams and Pipes

14.  Modify fork.c:

un-comment the special line you added in #11.
comment out all calls to sleep()
before the call to fork(), open the file "fork-output.txt" for writing (see man fopen)
write "BEFORE FORK\n" to the file before the call to fork()
write "SECTION A\n" to the file in section A. Do the same for sections B, C, and D (but replacing the "A" with "B", "C", and "D"...you get the idea). If using fprintf(), make sure to flush the file stream after each fprintf() (see man fflush).
Re-make and execute the newly recompiled fork. Using cat, show the contents of the file you created.

15.  Describe what you observe about the parent and child processes writing to the file.

16.  Modify fork.c:

Prior to the call to fork(), open a pipe (see man pipe).
In section B:
close the file descriptor corresponding to the read end of the pipe
create a file stream using the file descriptor for the write end of the pipe (see man fdopen)
write "hello from Section B\n" to the newly opened stream (see man fputs)
In section C:
close the file descriptor corresponding to the write end of the pipe
create a stream using the file descriptor corresponding to the read end of the pipe
Read a line from the newly opened file stream (see man fgets)
print the line retrieved from fgets() to stdout.
Show the output of your program.

 

Part 6: Putting it all together (literally)

17.  Modify fork.c:

Copy the body of exec.c and paste it into fork.c, such that the child process created with fork() runs the execve() call with the same arguments. Make sure the call to execve() is the last thing the child process does before exiting. (note: you shouldn't have to change any of the exec.c code, only where it's placed in the file).
Remove all sleep() calls in the program.
Re-make and execute the following to show that it works:

echo foobar | ./fork /bin/cat
Show the output from running this.

18.  Modify fork.c:

Immediately before calling execve(), duplicate the file descriptor associated with the file stream you made to write to fork-output.txt such that the standard output of the child process goes to that stream instead (see man fileno and man dup2).
Re-make and execute the following to show that it works:

echo foobar | ./fork /bin/cat
Show the output from running this. Also show the contents of fork-output.txt.

Submission:

Please include the following in your submission:

the final modified source code for fork.c and exec.c
the output requested from each of the numbered items above, in a single file called hw3.txt.  
Use the following command to tar/gzip your files:

tar -zcvf hw3.tar.gz exec.c fork.c hw3.txt
Grading:

18 points for the writeup hw3.txt (1 point for each correct answer).
2 points for a fork.c and exec.c that compile properly