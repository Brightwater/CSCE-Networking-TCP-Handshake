// Jeremiah Spears | 11/19/2019 | Intro To Networking Lab 4
/*
	This program is the client side of the TCP handshake program
	It will create a tcp segment using the tcpHeader struct which
	demonstrates the client side of the handshake.
	It will first send a SYN message to the server and then
	wait for the server to reply back. Then the client will reply back
	with an ACK containing the first file segment.
	(The file to be sent is specified by the user in the command line)
	The server will reply back with an ACK and the client will continue
	sending the file segments until the full file has been sent. 
	The client will then send a FIN message and wait for two
	replys from the server and will finally send a final ACK message.
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// struct to hold tcpHeader information 
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
int inet_pton(int af, const char *src, void *dst);
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
void printSegment(struct tcpHeader segment, int received, FILE *file);

// FLAG macros
#define FLAG_FIN 0b000001
#define FLAG_SYN 0b000010
#define FLAG_ACK 0b010000

int main(int argc, char*argv[])
{
    int sockfd;	// socket integer and 
    struct sockaddr_in servaddr;	// sockaddr
	struct tcpHeader segment;	// struct object
	unsigned short int cksum_arr[12];	// chksum array
	int received = 0; // 0 = false (or transmitted) | 1 = true (or received)

	// check for correct command line args
	if (argc < 2)
	{
		printf("Usage: ./a.out <filename.txt>\n");
		return 0;
	}

	// get file to send from command line args
	int x, v = 0, s = argc -1;
	char *str = (char *)malloc(v);
	for (x = 1; x <= s; x++)
	{
		str = (char *)realloc(str, (v + strlen(argv[x])));
		strcat(str, argv[x]);
	}

	FILE *file;	// file pointer
	file = fopen("client.out.txt", "w"); // open file in write mode

	FILE *dataFile; // data file pointer
	dataFile = fopen("fileToBeSent.txt", "r"); // open file in read mode

	// make sure file is open
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

	// setup segment values
	segment.source = 5087;
	segment.destination = 5087;
	segment.seq = 0;
	segment.ack = 0;
	segment.offset = 5;
	segment.reserved = 0;
	segment.flags = FLAG_SYN;	
	segment.window = 32767;
	segment.checksum = 0;
	segment.urgptr = 0;
	segment.opt = 0;
	

	// allocate chksum with segment
	memcpy(cksum_arr, &segment, 24);

	unsigned int i, sum = 0, chksum;	// declarations
	
	// calculate chksum
	for (i = 0; i < 12; i++)	// compute sum
		sum = sum + cksum_arr[i];

	chksum = sum >> 16; // fold once
	sum = sum & 0x0000FFFF; 
	sum = chksum + sum;
	chksum = sum >> 16; // fold again
	sum = sum & 0x0000FFFF;
	chksum = chksum + sum;

	segment.checksum = chksum;	// set chksum
	
	// create socket to server
    /* AF_INET - IPv4 IP , Type of socket, protocol*/
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    bzero(&servaddr, sizeof(servaddr));	// reset servaddr
 
    servaddr.sin_family = AF_INET;	// set AF_INET
    servaddr.sin_port = htons(5087); // Server port number 22000
 
    /* Convert IPv4 and IPv6 addresses from text to binary form */
	// 129.120.151.94 (cse01.cse.unt.edu)
	inet_pton(AF_INET, "129.120.151.94", &(servaddr.sin_addr));
 
    /* Connect to the server */
    if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) == -1)
	{	
		perror("connection error\n");
		exit(1);	// exit on error
	}
	
	printf("Connected to server\n\n");	// print connected message
	
	// start 3 way handshake
	printSegment(segment, received, file);	// run printSegment function
	write(sockfd, &segment, sizeof(struct tcpHeader)); // write to the server

	int size;	// size
	// receive message from server
	if ((size = recv(sockfd, (void*)&segment, sizeof(struct tcpHeader), 0)) < 0)
	{
		printf("recv error\n");
		exit(1);
	}
	received = 1;	// true (received)
	printSegment(segment, received, file);	// run printSegment

	// init seq numbers
	int initClientSeq = 0;
	int initServSeq = segment.seq;
	unsigned int tempSeq = segment.ack;	// temp int to hold seq
	// reset values 
	segment.ack = initServSeq + 1;	
	segment.seq = tempSeq;
	segment.flags = FLAG_ACK;

	// allocate chksum with segment 
	memcpy(cksum_arr, &segment, 24);

	// reset values
	sum = 0;
	chksum = 0;

	// calculate chksum
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

	// start sending file 
	fseek(dataFile, 0L, SEEK_END);	// seek to end of data file
	int fileSize = ftell(dataFile);	// get size of dataFile
	rewind(dataFile); // return to beginning of dataFile

	//segment.data = malloc(128 *sizeof(char));	// malloc segment data

	int fileSteps = fileSize / 128;	// calculate number of steps to send entire file

	// pseudocode for how file is sent between client
	// and server
	/*
		client:
			continue sending until entire file is sent
			start closing file with FLAG_FIN
			
		server:
			continue listening for ACKs to get entire file
			if FLAG_FIN is sent, stop writing to file and start closing process
	*/

	int y;
	// continue sending to server until full file is sent
	for (y = 0; y < fileSteps; y++)
	{
		fread(segment.data, 128, 1, dataFile);	// read 128 bytes from file

		// allocate chksum with segment
		memcpy(cksum_arr, &segment, 24);
		
		// reset values
		sum = 0;
		chksum = 0;

		// calculate chksum
		for (i = 0; i < 12; i++)	// compute sum
			sum = sum + cksum_arr[i];

		chksum = sum >> 16; // fold once
		sum = sum & 0x0000FFFF; 
		sum = chksum + sum;

		chksum = sum >> 16; // fold again
		sum = sum & 0x0000FFFF;
		chksum = chksum + sum;
		segment.checksum = chksum;
		received = 0; // false (transmitted)
		printSegment(segment, received, file);	// printSegment function
		write(sockfd, &segment, sizeof(struct tcpHeader)); // write to the server

		// receive message from server
		if ((size = recv(sockfd, (void*)&segment, sizeof(struct tcpHeader), 0)) < 0)
		{
			printf("recv error\n");
			exit(1);
		}
		received = 1;	// true (received)
		printSegment(segment, received, file);	// printSegment function
	}

	// start closing the TCP connection
	// reset values
	segment.seq = 0;
	segment.ack = 0;
	segment.flags = FLAG_FIN;

	// allocate chksum with segment
	memcpy(cksum_arr, &segment, 24);
	
	// reset values
	sum = 0;
	chksum = 0;

	// calculate chksum
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
	write(sockfd, &segment, sizeof(struct tcpHeader)); // write to the server

	// receive message from server
	if ((size = recv(sockfd, (void*)&segment, sizeof(struct tcpHeader), 0)) < 0)
	{
		printf("recv error\n");
		exit(1);
	}
	received = 1;	// true (received)
	printSegment(segment, received, file);	// printSegment function

	// receive message from server
	if ((size = recv(sockfd, (void*)&segment, sizeof(struct tcpHeader), 0)) < 0)
	{
		printf("recv error\n");
		exit(1);
	}
	printSegment(segment, received, file);	// printSegment function

	// setup new segment values
	segment.seq = initClientSeq + 1;
	segment.ack = initServSeq + 1;
	segment.flags = FLAG_ACK;

	// allocate chksum with segment
	memcpy(cksum_arr, &segment, 24);	
	
	// reset values
	sum = 0;
	chksum = 0;

	// calculate chksum
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
	write(sockfd, &segment, sizeof(struct tcpHeader)); // write to the server
	printSegment(segment, received, file);	// print segment function
	
	printf("\n");	// newline

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