/*
	Jeremiah Spears | 11/19/2019 | Intro To Networking Lab 4
	This program is the server side of the TCP handshake program
	It will receive the tcp segment from the client using the tcpHeader struct
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
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

struct tcpHeader
{
	unsigned short int source;		// source port
	unsigned short int destination;	// dest port
	unsigned int seq;				// sequence num
	unsigned int ack;				// ack num
	unsigned int offset:4;			// offset 
	unsigned int reserved:6;		// reserved
	unsigned short int flags;		// flags
	unsigned short int window;		// window
	unsigned short int checksum;	// checksum
	unsigned short int urgptr;		// urgptr
	unsigned int opt;				// socket options
	unsigned char data[128];		// data
};

// function prototypes
ssize_t write(int fd, const void *buf, size_t count);
void printSegment(struct tcpHeader segment, int received, FILE *file);
 
// FLAG macros
#define FLAG_FIN 0b000001
#define FLAG_SYN 0b000010
#define FLAG_ACK 0b010000

int main()
{
	int listen_fd, conn_fd;	// fd ints
	struct sockaddr_in servaddr;	// sockaddr for server
	int received = 1; // 0 = false (or transmitted) | 1 = true (or received)
	unsigned short int cksum_arr[12];	// chksum array

	FILE *file;	// file pointer
	file = fopen("server.out.txt", "w");	// open file in write mode

	FILE *dataFile; // data file pointer
	dataFile = fopen("dataFile.txt", "w");	// open file in write mode
	
	// check if file is open
	if (file == NULL)
	{
		perror("Could not open output file\n");
		exit(1);
	}
	if (dataFile == NULL)
	{
		perror("Could not open data file\n");
		exit(1);
	}

	/* AF_INET - IPv4 IP , Type of socket, protocol*/
	listen_fd = socket(AF_INET, SOCK_STREAM, 0); // create socket

	bzero(&servaddr, sizeof(servaddr));	// reset servaddr
	
	servaddr.sin_family = AF_INET;	// set AF_INET
	servaddr.sin_addr.s_addr = htons(INADDR_ANY);	// setup addr
	servaddr.sin_port = htons(5087); 	// set port

	/* Binds the above details to the socket */
	if ((bind(listen_fd,  (struct sockaddr *) &servaddr, sizeof(servaddr)) == -1))
	{
		perror("bind error\n");
		exit(1);
	}

	/* Start listening to incoming connections */
	if ((listen(listen_fd, 10) == -1))
	{
		perror("listen error\n");
		exit(1);
	}
	
	while(1)
	{
		printf("Waiting for client..\n");	// waiting for client message

		/* Accepts an incoming connection */
		conn_fd = accept(listen_fd, (struct sockaddr*)NULL, NULL);

		printf("Client connected\n\n");	// client connected message

		struct tcpHeader segment;	// segment object
		int size;	// size
		unsigned int i, sum = 0, chksum;	// declarations
	  
	  	// receive message from client
	  	if ((size = recv(conn_fd, (void*)&segment, sizeof(struct tcpHeader), 0)) < 0)
		{
			printf("recv error\n");
			exit(1);
		}

		printSegment(segment, received, file); // printSegment function

		// set segment values
		segment.ack = segment.seq + 1;
		segment.seq = 100;
		segment.flags = FLAG_SYN | FLAG_ACK;	// bitwise or to set ACK and SYN

		// allocate chksum with segment
		memcpy(cksum_arr, &segment, 24);

		for (i = 0; i < 12; i++)	// compute sum
			sum = sum + cksum_arr[i];
		
		// calculate chksum
		chksum = sum >> 16; // fold once
		sum = sum & 0x0000FFFF; 
		sum = chksum + sum;
		chksum = sum >> 16; // fold again
		sum = sum & 0x0000FFFF;
		chksum = chksum + sum;

		segment.checksum = chksum;	// set checksum

		received = 0;	// false (transmitted)
		printSegment(segment, received, file);	// printsegment function
		write(conn_fd, &segment, sizeof(struct tcpHeader)); // write to the client

		// file receive start
		while (1)
		{
			received = 1; // received
			// receive message from client
			if ((size = recv(conn_fd, (void*)&segment, sizeof(struct tcpHeader), 0)) < 0)
			{
				printf("recv error\n");
				exit(1);
			}
			printSegment(segment, received, file); // printSegment function

			// check for FIN flag
			if (segment.flags == 0b000001)
			{
				// if FIN flag start closing process and stop reading to file
				break;
			}

			// else read segment.data to file
			int dataLen = strlen(segment.data);		// get data len
			segment.data[dataLen - 1] = '\0';	// remove Null char
			fprintf(dataFile, "%s", segment.data);	// print data to file
			
			segment.ack = segment.seq + 1;	// set ack and seq
			segment.seq = segment.seq + 1;
			strcpy(segment.data, "");	// remove data

			// allocate chksum with segment
			memcpy(cksum_arr, &segment, 24);

			for (i = 0; i < 12; i++)	// compute sum
				sum = sum + cksum_arr[i];
			
			// calculate chksum
			chksum = sum >> 16; // fold once
			sum = sum & 0x0000FFFF; 
			sum = chksum + sum;
			chksum = sum >> 16; // fold again
			sum = sum & 0x0000FFFF;
			chksum = chksum + sum;

			segment.checksum = chksum;	// set checksum

			received = 0;	// false (transmitted)
			printSegment(segment, received, file);	// printSegment
			write(conn_fd, &segment, sizeof(struct tcpHeader)); // write to the client
		}
		
		fclose(dataFile);	// close file

		segment.ack = segment.seq + 1;
		segment.seq = 100;
		segment.flags = FLAG_ACK;
		
		// allocate chksum with segment
		memcpy(cksum_arr, &segment, 24);
	
		// reset values
		sum = 0;
		chksum = 0;

		// calculate checksum
		for (i = 0; i < 12; i++)	// compute sum
			sum = sum + cksum_arr[i];
		
		chksum = sum >> 16; // fold once
		sum = sum & 0x0000FFFF; 
		sum = chksum + sum;

		chksum = sum >> 16; // fold again
		sum = sum & 0x0000FFFF;
		chksum = chksum + sum;

		segment.checksum = chksum;

		received = 0;	// false (transmitted)
		printSegment(segment, received, file);	// printSegment
		write(conn_fd, &segment, sizeof(struct tcpHeader)); // write to the client

		segment.flags = FLAG_FIN;
		
		// allocate chksum with segment
		memcpy(cksum_arr, &segment, 24);

		// reset values
		sum = 0;
		chksum = 0;

		// calculate checksum
		for (i = 0; i < 12; i++)	// compute sum
			sum = sum + cksum_arr[i];
		
		chksum = sum >> 16; // fold once
		sum = sum & 0x0000FFFF; 
		sum = chksum + sum;

		chksum = sum >> 16; // fold again
		sum = sum & 0x0000FFFF;
		chksum = chksum + sum;

		segment.checksum = chksum;
		
		printSegment(segment, received, file);	// printSegment function
		write(conn_fd, &segment, sizeof(struct tcpHeader)); // write to the client

		// receive message
		if ((size = recv(conn_fd, (void*)&segment, sizeof(struct tcpHeader), 0)) < 0)
		{
			printf("recv error\n");
			exit(1);
		}
		received = 1;	// true (transmitted)
		printSegment(segment, received, file); // printSegment function
		
		close (conn_fd); //close the connection with client
		
		printf("Client disconnected\n\n");	// disconnection message
	}
	
	fclose(file);	// close file

	return 0;
}

// function to print the segment values and also print them in the outfile
void printSegment(struct tcpHeader segment, int received, FILE *file)
{
	// check if received or transmitted message
	if (received)
	{
		printf("Received:\n");
		fprintf(file, "Received:\n");
	}
	else
	{
		printf("Transmitted:\n");
		fprintf(file, "Transmitted:\n");
	}

	// print to stdout
	printf("source: %d\n", segment.source);
	printf("dest: %d\n", segment.destination);
	printf("sequence #: %d\n", segment.seq);
	printf("ack #: %d\n", segment.ack);
	printf("offset: %d\n", segment.offset);
	printf("reserved: %d\n", segment.reserved);
	printf("flags: 0x%04X\n", (0xFFFF^segment.flags));
	printf("window: %d\n", segment.window);
	printf("checksum: 0x%04X\n", (0xFFFF^segment.checksum));
	printf("urgptr: %d\n", segment.urgptr);
	printf("options: %d\n\n", segment.opt);

	// print to file
	fprintf(file, "source: %d\n", segment.source);
	fprintf(file, "dest: %d\n", segment.destination);
	fprintf(file, "sequence #: %d\n", segment.seq);
	fprintf(file, "ack #: %d\n", segment.ack);
	fprintf(file, "offset: %d\n", segment.offset);
	fprintf(file, "reserved: %d\n", segment.reserved);
	fprintf(file, "flags: 0x%04X\n", (0xFFFF^segment.flags));
	fprintf(file, "window: %d\n", segment.window);
	fprintf(file, "checksum: 0x%04X\n", (0xFFFF^segment.checksum));
	fprintf(file, "urgptr: %d\n", segment.urgptr);
	fprintf(file, "options: %d\n\n", segment.opt);
	
	fflush(file);	// flush file
}