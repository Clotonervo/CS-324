HTTP Proxy Lab 3 (I/O Multiplexing)

In this lab, you will again be implementing an HTTP proxy that handles concurrent requests.  However, the server you produce this time will take advantage of I/O multiplexing.  Your server will not spawn any additional threads or processes (i.e., it will be single-threaded), and all sockets will be set to non-blocking.  While your server will not take advantage of multiprocessing, it will be more efficient by holding the processor longer because it is not blocking (and thus sleeping) on I/O.  Popular servers, such as NGINX, use a similar model (see https://www.nginx.com/blog/thread-pools-boost-performance-9x/).  This model is also referred to as an example of event-based programming, wherein execution of code is dependent on "events"--in this case the availability of I/O.

Please read the epoll man page in preparation for this lab.  You will also want to have ready reference to several other man pages related to epoll.  Here is a list:
 
epoll - general overview of epoll, including detailed examples
epoll_create1 - shows the usage of the simple function to create an epoll instance
epoll_ctl - shows the definition of the epoll_data and epoll_event structures, which are used by both epoll_ctl() and epoll_wait().  Also describes the event types with which events are registered to an epoll instance, e.g., for reading or writing, and which type of triggering is used (for this lab you will use edge-level triggering).
epoll_wait - shows the usage of the simple epoll_wait() function, including how events are returned and how errors are indicated,
Handout
An updated version of the handout is here:
 
proxylab-handout.tar  Download 
 
 
WARNING: Please save/rename any directories named "proxylab-handout" before you untar this file!
 
Procedure
The following is a general outline that might help you organize your server code:
 
When you start up your HTTP server:
Create an epoll instance with epoll_create1()
Set up your listen socket (as you've done in previous labs), and configure it to use non-blocking I/O (see the man page for fcntl() for how to do this).
Register your listen socket with the epoll instance that you created, for reading.
Open your log file.  Since this server is single-threaded, you don't need to use producer-consumer threads, and you can log directly, rather than sending it to a queue (see Client Request Handling below).
Initialize your cache.  This time, there is no need to guard read or write access to the cache with semaphores because only a single thread will be accessing the cache.
Start an epoll_wait() loop, with a timeout of 1 second.

In your epoll_wait() loop, you will do the following:
If the result was a timeout (i.e., return value from epoll_wait() is 0), check if a global flag has been set by a handler and, if so, break out of the loop; otherwise, continue.
If the result was an error (i.e., return value from epoll_wait() is less than 0), handle the error appropriately (see the man page for epoll_wait for more).

If there was no error, you should loop through all the events and handle each appropriately.  See next bullet items.
If an event corresponds to the listen socket, you should loop to accept() any and all client connections, configure each, in turn, to use non-blocking I/O (see the man page for fcntl() for how to do this), and register each returned client socket with the epoll instance that you created, for reading, using edge-triggered monitoring.  You will stop calling accept() when it returns a value less than 0.  If errno is set to EAGAIN or EWOULDBLOCK, then that is an indicator that there are no more clients currently waiting.

If an event corresponds to the socket associated with a client request (client socket) or the socket associated with a the proxied request to an upstream server (server socket) , you should determine where you are in terms of handling the corresponding client request and begin (or resume) handling it.  You should only read() or write() on said socket if your event indicates that you can, and only until the read() or write() call returns a value less than 0.  In such cases (where a value less than 0 is returned), if errno is EAGAIN or EWOULDBLOCK, then that is an indicator that there is no more data to be read, or (for write) that the file descriptor is no longer available for writing.  See the Client Request Handling section for more information.
After your epoll_wait() loop, you should clean up any resources (e.g., freeing malloc'd memory), and exit.
 
Client Request Handling
Just as with the previous parts of the proxy lab, when you receive an HTTP request from a client, you will first want to check the cache for the resource corresponding to that URL requested.  If it exists, then you can send the resource back to the client.  As mentioned earlier, there is no reason to guard access to the cache with semaphores, as there is only a single thread accessing the cache.  However, this time you cannot simply begin sending back over the socket; you must wait until the call to epoll_wait() indicates that the socket is available for writing.
 
If the URL you have requested is not in the cache, then (just as in the previous lab) your proxy will need to retrieve it from the upstream server.  The difference is that the socket you set up for communication with the server must now be set to non-blocking, just as the listening socket and the client socket are.  And you must register this socket with the epoll instance, for writing, using edge-triggered monitoring.
 
For simplicity, you may wait to set the proxy-to-server socket as non-blocking after you call connect(), rather than before.  While that will mean that your server not fully non-blocking, it will allow you to focus on the more important parts of I/O multiplexing.  This is permissible.
 
If you instead choose to set the socket as non-blocking before calling connect() (this is not required), you can execute connect() immediately, but you cannot initiate the write() call until epoll_wait() indicates that this socket is ready for writing; because the socket is non-blocking, connect() will return before the connection is actually set up.
 
You will need to keep track of the "state" of reach request.  The reason is that, just like when using blocking sockets, you won't always be able to receive or send all your data with a single call to read() or write().  With blocking sockets in a multi-threaded server, the solution was to use a loop that received or sent until you had everything, before you moved on to anything else.  Because it was blocking, the kernel would context switch out the thread and put it into a suspended state until there was I/O.  However, with I/O multiplexing and non-blocking I/O, you can't loop until you receive (or send) everything; you have to stop when you get an value less than 0 and finish handling the other ready events, after which you will return to the epoll_wait() loop to see if it is ready for more I/O.  When a return value to read() or write() is less than 0 and errno is EAGAIN or EWOULDBLOCK, it is a an indicator that you are done for the moment--but you need to know where you should start next time it's your turn (see man pages for accept and read, and search for blocking).  For example, you'll need to associate with the request:
the socket corresponding to the requesting client
the socket corresponding to the connection to the Web server
the current state of the request (see Client Request States)
the buffer to read into and write from
the total number of bytes read from the client
the total number of bytes to write to the server
the total number of bytes written to the server
the total number of bytes read from the server
the total number of bytes written to the client
You can define a struct request_info (for example) that contains each of these members.

 
Client Request States
One way of thinking about the problem is in terms of "states".  The following is an example of a set of client request states, each associated with different I/O operations related to proxy operation:
READ_REQUEST:
This is the start state for every new client request.  You should initialize every new client request to be in this state.
In this state, loop to read from the client socket until one of the following happens:
you have read the entire HTTP request from the client.  If this is the case:
parse the client request and create the request that you will send to the server.
log the request.
If the URL is cached, then pull it from the cache, and change state to SEND_RESPONSE.  Stop here.
set up a new socket, configure it as non-blocking, and use it to connect() to the Web server.
register the socket with the epoll instance for writing.
change state to SEND_REQUEST.
read() (or recv()) returns a value less than 0 and errno is EAGAIN or EWOULDBLOCK.  In this case, don't change state; you will continue reading when you are notified by epoll that there is more data to be read.
read() (or recv()) returns a value less than 0 and errno is something other than EAGAIN or EWOULDBLOCK.  This is an error, and you can cancel your client request and deregister your socket at this point.
SEND_REQUEST:
You reach this state only after the entire request has been received from the client and the connection to the server has been initiated (i.e., in the READ_REQUEST state).
In this state, loop to write the request to the server socket until one of the following happens:
you have written the entire HTTP request to the server socket.  If this is the case:
register the socket with the epoll instance for reading.
change state to READ_RESPONSE
write() (or send()) returns a value less than 0 and errno is EAGAIN or EWOULDBLOCK.  In this case, don't change state; you will continue writing when you are notified by epoll that you can write again.
write() (or send()) returns a value less than 0 and errno is something other than EAGAIN or EWOULDBLOCK.  This is an error, and you can cancel your client request and deregister your sockets at this point.
READ_RESPONSE:
You reach this state only after you have sent the entire HTTP request (i.e., in the SEND_REQUEST state) to the Web server.
In this state, loop to read from the server socket until one of the following happens:
you have read the entire HTTP response from the server.  Since this is HTTP/1.0, this is when the call to read() (recv()) returns 0, indicating that the server has closed the connection.  If this is the case:
register the client socket with the epoll instance for writing.
change state to SEND_RESPONSE.
read() (or recv()) returns a value less than 0 and errno is EAGAIN or EWOULDBLOCK.  In this case, don't change state; you will continue reading when you are notified by epoll that there is more data to be read.
read() (or recv()) returns a value less than 0 and errno is something other than EAGAIN or EWOULDBLOCK.  This is an error, and you can cancel your client request and deregister your socket at this point.
SEND_RESPONSE:
You reach this state only after you have received the entire response from the Web server (i.e., in the READ_RESPONSE state) or you have found the requested URL in the cache..
In this state, loop to write to the client socket until one of the following happens:
you have written the entire HTTP response to the client socket.  If this is the case:
close your client socket.
write() (or send()) returns a value less than 0 and errno is EAGAIN or EWOULDBLOCK.  In this case, don't change state; you will continue writing when you are notified by epoll that you can write again.
write() (or send()) returns a value less than 0 and errno is something other than EAGAIN or EWOULDBLOCK.  This is an error, and you can cancel your client request and deregister your sockets at this point.
 
Hints
When planning your assignment, it will greatly help you to create a state machine that include all of the above client states and the conditions that trigger transitions from one state to another.  While there is some detail in the model above, this will help with your understanding of what is going on.
As mentioned in previous labs, do not use the RIO code distributed with this and the previous proxy labs.  It was written for blocking sockets, and it will not work properly.
Your code MUST compile and run properly (i.e., as tested by the driver) on the CS lab machines.  Note that you are still welcome to develop in another (Linux) environment (e.g., a virtual machine), but please compile and test on a CS lab machine before submission!
The log entries must include the URL that was requested by the client.  Other than that, the format is loose.
Two notes on getaddrinfo():
While the typical use of getaddrinfo() is to call it and iterate through the linked list of addresses returned to find one that works, that is unnecessary complexity for this lab because of the non-blocking I/O.  You can simply pick the first address in the list.
getaddrinfo() involves performing a DNS lookup, which is, effectively, I/O.  However, there is no asynchronous/non-blocking version of getaddrinfo(), so you may use it in synchronous/blocking fashion (For those interested in a more flexible alternative, see https://getdnsapi.net/).
Because you are not sharing data across  threads, you shouldn't need to allocate any memory on the heap (e.g., using malloc or calloc); you can simply declare a large array of struct client_requests (for example) and then just keep track of which ones are being used, etc.
Read the man pages - really.
Reminders
You may not use blocking I/O.  All sockets must be configured as non-blocking.  You need to set this up--it is not the default.  See instructions above.  There is one exception to this: you can wait to configure your proxy-to-server socket until after the call to connect().
You may not use threads (i.e., with pthread_create()) or multiple processes (i.e., with fork()).
You should not need or want to use semaphores, locks, or other components of synchronization.  You're welcome :)
 
Testing
Use the following command-line to test your proxy with the driver:

./driver.sh epoll 15 60 10 1 1 10
Grading Breakdown
15 pts basic proxy operation
60 pts for handling concurrent requests (with epoll and non-blocking sockets)
10 pts for implementing a working cache
10 pts for logging URLs to a file
3 pts for compilation without any warnings
2 pts for no memory leaks
Note that the driver doesn't include the three points for checking for compiler warnings.

Submission
To submit, run "make handin" and upload the resulting tar file to LearningSuite.
Please make sure your working directory is named "proxylab-handout".