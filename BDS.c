/*
Name:		BDS.c
Author:		Hu Jingmei
Function:	The source code for Disk Servers in Basic disk-storage system. Get the cylinders, sector and the track-to-track time from the command-line parameter, simulate a disk and store the actual data in a real disk file.
*/
#include "cse356header.h"
using namespace std;

//error check
void error(const char *msg)
{
    perror(msg);
    exit(1);
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
	int sockfd, client_sockfd, BASIC_SERVER_PORT;
	int BLOCKSIZE = 256, SECTORS = atoi(argv[3]), CYLINDERS = atoi(argv[2]), Ttime = atoi(argv[4]);
	socklen_t clilen;
	char buffer[1024];
	char* filename = argv[1];
	struct sockaddr_in serv_addr, cli_addr;
	int n;

	//Open a file
	int fd = open(filename, O_RDWR | O_CREAT, S_IRWXU);
	if (fd < 0) {
	    	printf("Error: Could not open file '%s'.\n", filename);
	    	exit(-1);
	}
	//Stretch the file size to the size of the simulated disk
	long FILESIZE = BLOCKSIZE * SECTORS * CYLINDERS;
	int result = lseek (fd, FILESIZE-1, SEEK_SET);
	if (result == -1) {
		perror ("Error calling lseek() to 'stretch' the file");
		close (fd);
		exit(-1);
	}
	//Write something at the end of the file to ensure the file actually have the new size.
	result = write (fd, "", 1);
	if (result != 1) {
		perror("Error writing last byte of the file");
		close(fd);
		exit(-1);
	}
	//Map the file
	char* diskfile;
	diskfile = (char *) mmap (0, FILESIZE,
		PROT_READ | PROT_WRITE,
		MAP_SHARED, fd, 0);
	if (diskfile == MAP_FAILED){
		close(fd);
		printf("Error: Could not map file.\n");
		exit(-1);
	}

	if (argc < 5) {
	 	fprintf(stderr,"ERROR, no port provided\n");
	 	exit(1);
	}
	//creating socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) 
		error("ERROR opening socket");
	bzero((char *) &serv_addr, sizeof(serv_addr));

	BASIC_SERVER_PORT = atoi(argv[5]);

	//binding socket
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(BASIC_SERVER_PORT);
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
	error("ERROR on binding");//error check

	//listening and accepting connection
	listen(sockfd,5);
	clilen = sizeof(cli_addr);
	client_sockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
	if (client_sockfd < 0) 
	  	error("ERROR on accept");

	//get counter
	bzero(buffer,1024);
	n = read(client_sockfd,buffer,1024);
	if (n < 0) error("ERROR reading from socket");
	int counter = atoi(buffer);	

	int c = 0, s = 0, l = 0;
	int cur_c = c;
	while(1){
		 bzero(buffer,1024);
		 n = read(client_sockfd,buffer,1024);
		 if (n < 0) error("ERROR reading from socket");
		 printf("%s\n", buffer);	
		 char *cmd[10];
		 parseLine(buffer,cmd);
		 //if I command
		 if (cmd[0][0]=='I') {
			printf("cylinders and sectors are: %d %d\n", CYLINDERS, SECTORS);
			bzero(buffer,1024);
			sprintf(buffer,"%d %d", CYLINDERS, SECTORS);
		 	n = write(client_sockfd,buffer,1024);
			if (n < 0) error("ERROR writing to socket");
		 }
		 //if R command
		 else if (cmd[0][0]=='R'){
			c = atoi(cmd[1]);
			s = atoi(cmd[2]);			
			if(c >= 0 && c <=  CYLINDERS-1 && s>=0 && s<=SECTORS-1)
			{	//get the content from the file
				char pbuf[1024] = "";
				for(int i = BLOCKSIZE * (c * SECTORS + s); i < BLOCKSIZE * (c * SECTORS + s+1); i++)
					pbuf[i - (BLOCKSIZE * (c * SECTORS + s))] = diskfile[i];
				pbuf[256] = '\0';
				usleep(Ttime*abs(c-cur_c));
				cur_c = c;
				printf("Yes! %s\n", pbuf);
				// send the result to the client
				bzero(buffer,1024);
				strcat(buffer, "Yes! ");
				strcat(buffer, pbuf);
				n = write(client_sockfd,buffer,1024);
				if (n < 0) error("ERROR writing to socket");
			}
			else {
				printf("No!\n");
				n = write(client_sockfd,"No!",1024);
				if (n < 0) error("ERROR writing to socket");
			}
		 }
		 //if W command
		 else if (cmd[0][0]=='W'){
			c = atoi(cmd[1]);
			s = atoi(cmd[2]);
			l = atoi(cmd[3]);	
			char *data = cmd[4];
			if(c >= 0 && c <=  CYLINDERS-1 && s>=0 && s<=SECTORS-1 && l <= 256)
			{	//write the content to the file
				for(int i = 0; i < l; i++)
					diskfile[BLOCKSIZE * (c * SECTORS + s) + i] = data[i];
				usleep(Ttime*abs(c-cur_c));
				cur_c = c;
				printf("Yes!\n");
				//send the result to the client
				n = write(client_sockfd,"Yes!",1024);
				if (n < 0) error("ERROR writing to socket");
				for(int i = l; i < 256; i++)
				diskfile[BLOCKSIZE * (c * SECTORS + s) + i] = 0;
			}
			else {
				printf("No!\n");
				n = write(client_sockfd,"No!",1024);
				if (n < 0) error("ERROR writing to socket");
			}
		 }
		//if Q command, exit
		else if (cmd[0][0]=='Q')
		{
			munmap(0,FILESIZE); 
			break;
		}
	 }
	close(client_sockfd);
	close(sockfd);
	return 0; 
}
