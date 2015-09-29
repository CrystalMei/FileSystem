/*
Name:		FC.c
Author:		Hu Jingmei
Function:	The source code for File System Clients in File System, which is a command-line driven client. Work in a loop in order to get the user type commands and send the commands to the file system server. Also display a single character representing the results.
*/
#include "cse356header.h"
using namespace std;

//error check
void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
	int sockfd, BASIC_SERVER_PORT, n;
	struct sockaddr_in serv_addr;
	struct hostent *server;

	char buffer[1024];
	if (argc < 3) {
		fprintf(stderr,"usage %s hostname port\n", argv[0]);
		exit(0);
	}
	//Connecting A Client to A Server
	BASIC_SERVER_PORT = atoi(argv[2]);
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) 
		error("ERROR opening socket");

	server = gethostbyname(argv[1]);
	//error check
	if (server == NULL) {	
		fprintf(stderr,"ERROR, no such host\n");
		exit(0);
	}
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(BASIC_SERVER_PORT);
	if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
		error("ERROR connecting");//error check

	while(1){
		//send the command to the server
		bzero(buffer,1024);
		cin.getline(buffer,1024);
		n = write(sockfd,buffer,1024);
		if (n < 0) 
			error("ERROR writing to socket");
		if(strcmp(buffer,"exit")==0) break;
		//get the result from the server
		bzero(buffer,1024);
		n = read(sockfd,buffer,1024);
    		if (n < 0) 
        		error("ERROR reading from socket");
		printf("%s\n",buffer);
	}
	close(sockfd);
	return 0;
}
