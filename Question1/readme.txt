Execution instructions:
First server must be executed and then client.
client takes an optional command line argument that specifies the input file name. In its absence client assumes the input file is input.txt.
server also takes an optional argument; the output file name. In its absence, the output file is assumed to be output.txt.

Compilation instructions:
These programs can be compiled without any special flags, etc.

Methodology:
This program is implemented entirely at a very low level.
Firstly the client forks(), one subprocess to handle one connection. These processes use low level system calls and file descriptors to read and write from output.
The properties of file descriptors and system calls take care of process syncronization directly. This is advantageous as there is not even a need to buffer at all.
The server does the same mostly, but as a concurrent server. It takes two connections one after the other and forks two children for each client.
This way one server instance handles multiple file transfers. However all the files so created have the same name. The server finally can be shut down gracefully by a SIGINT i.e. <Ctrl>+<C>. A handler closes sockets and files.
A little excessive signal handling has been used but only to deal with possible errors.