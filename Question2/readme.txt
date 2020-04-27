Execution instructions:
First execute server.
Then execute relay twice.
Finally execute client.
There is already a file input.txt that can be used with these programs. To use this file execute client without any command line arguments.
To use any other file pass its name as a command line argument to client.
The output from server is by default written to a file output.txt. Pass any other alternative name as a command line argument to server.
Call relay with arguments 1 and 2 to create the two nodes.

Compilation notes:
There are many posix specific functions used here, but they should pose no problems.
Programs server.c and relay.c can be compiled directly. client.c uses math.h so it needs the -lm flag.

Methodology:
For the second question, there are three programs.
The simplest of these, the relay program, takes one command line argument, which decides whether it is relay1 or relay2.
The relay program has two sockets. One socket connects with the client. The other connects with the server.
The socket connecting to the client is bound to ports. The server-side socket binds to ports, so the other socket here is not bound to anything.
This program is eternally in a loop. Once it reads a message from the client socket, it prints out relevant logs and immediately forwards it to the server. Just before it forwards, it generates a random number and then drops according to PDR.
The drop is implemented by an immediate loop continue. Relay just ignores the packet and waits for the next one.
If the packet is not dropped, another random number is generated and a delay is produced. Relay waits and then forwards the packet.
If the program reads a message from the server socket, it just directly forwards it to the client after printing necessary logs.

The server program takes one command line argument (this argument is optional), the name of the output file.
If no argument is passed this output file is named output.txt.
The server works as two loops. The outer loop has a relatively smaller job. As soon as the buffer is half full, this loop writes to the file.
This is one point of ambiguity resolved. The file is written to as and when the buffer is half full.
However, it is only given access to the buffer once the inner loop allows it. The inner loop is implemented in a function readIntoBuffer.
readIntoBuffer has some state information. Firstly this is the function that keeps track of the sliding window. It does this through the use of one variable beg and the macro WINDOWSIZE.
This function also keeps track of packets received out of order but with gaps. That is, if the first half of the sliding window is full, but the second has only two elements, these two elements are stateful. This is maintained through an array arr.
Lastly, to exit the loop and finish the process, this loop keeps track of whether or not it has encountered the the last packet.
This function uses static variables to keep track of these pieces of information.
end is the next packet that is not yet received. That is, all packets from beg to end have been received. The check end-beg<WINDOWSIZE/2 evidently decides when to write into the array and exit the inner loop.
As soon as a packet is received, the function checks whether it is a part of the sliding window.
If it is not a part of the current window, but already was received earlier, it is acknowledged once again and then ignored.
If it is not a part of the current window, but yet to be encountered, it is immediately ignored after it is received.
If it is a part of the sliding window and was already received, it is ignored again (essentially).
Finally, if it is a part of the sliding window and was just encountered for the first time, its presence is noted. end is subsequently updated and the check occurs.
This function handles input from two sockets by making use of select. As it returns, it keeps all state variables up to date.
It prints logs as it does what it needs to do.

The main part of the client program is one do while loop, whose invariant is that all packets in the window have been sent out, and the first packet is unacknowledged.
It maintains this invariant by checking for acknowledgements from either relay node using select. If it finds an acknowledgement for some node that is not the first in the sliding window, it makes a note and proceeds.
If it receives an acknowledgement for the first node in the window, it removes all the nodes from the beginning that have been acknowledged and sends out as many new nodes. This main loop is exited once the sliding window is empty.
It is perhaps important to note that the sliding window in the server was implemented by an array.
The sliding window in the server is implemented by a linked list.
If select times out without receiving an acknowledgement, the client takes it as a sign that some packets were dropped and need to be resent.
It greedily resends the packet that was sent last. It does this by maintaining a queue of timeouts for all nodes that were sent out. It takes the tiemout from the front of the queue, which will be the shortest.
If the packet corresponding to this timeout was acknowledged already, this timeout is simply ignored. Otherwise, it is used in the select call and the packet corresponding to this timeout is re-sent.
The packet is given a new timeout and added back to the queue. Retransmission times of the order of seconds are necessary to see this in action.
Before it enters the main loop, the client sends out the first few packets to ensure that the window is full.
