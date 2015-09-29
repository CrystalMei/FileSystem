/*
Name:		BDC_random.c
Author:		Hu Jingmei
Function:	The source code for Disk Clients in Basic disk-storage system, which is a random-data driven client. Generate a series of N random requests (W or R) to the disk. Print a single character representing the requests' results to the user.
*/
#include "cse356header.h"
using namespace std;

void error(const char *msg)
{
    perror(msg);
    exit(0);
}
//change a number into a character
void itoa(int i,char*string)
{
	int power,j;
	j=i;
	for(power=1;j>=10;j/=10)
		power*=10;
		for(;power>0;power/=10)
		{
			*string++='0'+i/power;
			i%=power;
		}
	*string='\0';
}

//parse the command line
int parseLine(char *line, char *command_array[]) {
	char *p;
	int count = 0;
	p = strtok(line, " ");
	while (p && strcmp(p,"|")!=0) {
		command_array[count] = p;
		count++;
		p = strtok(NULL," ");
	}
	return count;
}

int main(int argc, char *argv[])
{
	int sockfd, BASIC_SERVER_PORT, n, CYLINDERS, SECTORS, counter;
	struct sockaddr_in serv_addr;
	struct hostent *server;

	char buffer[1024];

	srand((unsigned int)time(0)); 

	if (argc < 3) {
		fprintf(stderr,"usage %s hostname port\n", argv[0]);
		exit(0);
	}
	//Connecting A Client to A Server
	BASIC_SERVER_PORT = atoi(argv[3]);
	counter = atoi(argv[2]);
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) 
		error("ERROR opening socket");

	//error check
	server = gethostbyname(argv[1]);
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

	//send N
	bzero(buffer,1024);
	itoa(counter, buffer);
	n = write(sockfd,buffer,1024);	

	// I command
	bzero(buffer,1024);
	sprintf(buffer,"I");
	n = write(sockfd,buffer,1024);	
	printf("%s\n", buffer);	
	if (n < 0) 
		error("ERROR writing to socket");
	bzero(buffer,1024);
	n = read(sockfd,buffer,1024);
	if (n < 0) 
		error("ERROR reading from socket");

	char *cmd[10];
	parseLine(buffer,cmd);
	CYLINDERS = atoi(cmd[0]);
	SECTORS = atoi(cmd[1]);
	for(int i = 0; i < counter; i++)
	{	
		bzero(buffer,1024);
		int rw = rand() % 2;
		//R command
		if (rw == 0)
		{		
			strcat(buffer, "R ");
			int c = rand() % CYLINDERS, s = rand() % SECTORS;
			sprintf(buffer,"R %d %d",c,s);		
			printf("%s\n", buffer);	
		 	n = write(sockfd,buffer,1024);
			if (n < 0) error("ERROR writing to socket");
		}
		//W command
		else 
		{
			strcat(buffer, "W ");
			int c = rand() % CYLINDERS, s = rand() % SECTORS;
			char data[257] = "";
			char cy[2], se[2];			
			itoa(c,cy);			
			itoa(s,se);
			for(int i = 0; i < 256;i++)
				data[i] = rand() % 10 + '0';	
			data[256] = '\0';
			strcat(buffer, cy);
			strcat(buffer, " ");
			strcat(buffer, se);
			strcat(buffer, " 256 ");
			strcat(buffer, data);		
			printf("%s\n", buffer);	
		 	n = write(sockfd,buffer,1024);
			if (n < 0) error("ERROR writing to socket");
		}		
		bzero(buffer,1024);
		n = read(sockfd,buffer,1024);
    		if (n < 0) 
			error("ERROR reading from socket");
		printf("%s\n",buffer);
	}
	//exit
	sprintf(buffer,"Q");
	write(sockfd, buffer,1024);

	close(sockfd);
	return 0;
}
