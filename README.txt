Jeremiah Spears | 11/19/2019 | Intro To Networking Lab 4

(port is preset to 5087)

The client determines the file to send using the given command line parameters

client.c
To compile: gcc client.c
Run parameters: ./a.out <filename.txt>

In the client side the program will create a tcp segment using 
the tcpHeader struct which demonstrates the client side of the handshake.
It will first send a SYN message to the server and then
wait for the server to reply back. Then the client will reply back
with an ACK containing the first file segment.
(The file to be sent is specified by the user in the command line)
The server will reply back with an ACK and the client will continue
sending the file segments until the full file has been sent. 
The client will then send a FIN message and wait for two
replys from the server and will finally send a final ACK message.

server.c
To compile: gcc server.c
Run parameters: ./a.out

In the server side It will receive the tcp segment 
from the client using the tcpHeader struct
which demonstrate the server side of the handshake.
It will first wait for a SYN message from the client and then will reply back
with an ACK message. Then the server will wait for another reply back from
the client. This reply will hold the first segment of the file.  The server will
then write the file segment to dataFile.txt and reply back with an ACK message. 
Next the server will continue this process with the client until the entire
file ahs been transferred.
Then the server will wait for another message which is a FIN.
Next the server will send an ACK message followed by a FIN message and
will wait for a final ACK message from the client.
