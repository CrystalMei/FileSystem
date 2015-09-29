/*
Name:		IDS_FCFS.c
Author:		Hu Jingmei
Function:	The source code for Disk Servers in Intelligent disk-storage system. Use FCFS scheduler to calculate the track-to-track delay time.
*/
#include "cse356header.h"
using namespace std;

//error check
void error(const char *msg)
{
    perror(msg);
    exit(1);
}
//quicksort
int qsort(int l, int r, int a[])
{
	int i(l), j(r), k(a[(l + r) / 2]);
	do
	{
		while (i <= j && a[i] < k) i++;
		while (i <= j && a[j] > k) j--;
		if (i <= j)
		{
			 int tmp = a[i]; a[i] = a[j]; a[j] = tmp;
			 i++; j--;
		}
	} while (i <= j);
	if (l < j) qsort(l, j, a);
	if (i < r) qsort(i, r, a);
}

//findmin, return the index
int findmin(int num, int pos, int a[], int b[])
{
	int minnum, i = 0;
	for(i = 0; i < num; i++) if(b[i] == 0) {minnum = i; break;}
	for(i = 0; i < num; i++) if(a[i] < a[minnum] && b[i] == 0) {minnum = i;}
	return minnum;
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
	int sockfd, client_sockfd, BASIC_SERVER_PORT;
	int BLOCKSIZE = 255, SECTORS = atoi(argv[3]), CYLINDERS = atoi(argv[2]), Ttime = atoi(argv[4]), cnt_n = atoi(argv[6]);
	socklen_t clilen;
	char buffer[1024];
	char* filename = argv[1];
	struct sockaddr_in serv_addr, cli_addr;
	int n;

	//Open a file
	int fd = open(filename, O_RDWR | O_CREAT, 0);
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

	 bzero(buffer,1024);
	 n = read(client_sockfd,buffer,1024);
	 if (n < 0) error("ERROR reading from socket");
	 char *cmd[10];
	 parseLine(buffer,cmd);
	 //if I command
	 if (cmd[0][0]=='I') {
		printf("cylinders and sectors are: %d %d\n", CYLINDERS, SECTORS);
	 	char out1[10], out2[10];
		bzero(buffer,1024);
		itoa(CYLINDERS,out1);
		itoa(SECTORS,out2);
		strcat(buffer, out1);
		strcat(buffer, " ");
		strcat(buffer, out2);
	 	n = write(client_sockfd,buffer,1024);
		if (n < 0) error("ERROR writing to socket");
	 }
	
	int c = 0, s = 0, l = 0, cnt = 0, cur_c = c;
	int *cylinders_n = new int [counter];
	for(int i = 0; i < (counter/cnt_n); i++)
	{		
		for(int j = 0; j < cnt_n; j++)
		{
			 bzero(buffer,1024);
			 n = read(client_sockfd,buffer,1024);
			 if (n < 0) error("ERROR reading from socket");
			 char *cmd[10];
			 parseLine(buffer,cmd);
		 	//if R command
			 if (cmd[0][0]=='R')
			{
				c = atoi(cmd[1]);
				s = atoi(cmd[2]);
				cylinders_n[cnt++] = c;			
				if(c >= 0 && c <=  CYLINDERS-1 && s>=0 && s<=SECTORS-1)
				{
					char pbuf[1024] = "";
					for(int i = BLOCKSIZE * (c * SECTORS + s); i < BLOCKSIZE * (c * SECTORS + s+1); i++)
						pbuf[i - (BLOCKSIZE * (c * SECTORS + s))] = diskfile[i];
					n = write(client_sockfd,"G",2);
					if (n < 0) error("ERROR writing to socket");
				}
			 }
			 //if W command
			 else if (cmd[0][0]=='W')
			{
				c = atoi(cmd[1]);
				s = atoi(cmd[2]);
				l = atoi(cmd[3]);	
				char *data = cmd[4];			
				cylinders_n[cnt++] = c;	
				if(c >= 0 && c <=  CYLINDERS-1 && s>=0 && s<=SECTORS-1 && l <= 256)
				{
					for(int i = 0; i < l; i++)
						diskfile[BLOCKSIZE * (c * SECTORS + s) + i] = data[i];
					for(int i = l; i < 256; i++)
						diskfile[BLOCKSIZE * (c * SECTORS + s) + i] = 0;
					n = write(client_sockfd,"G",2);
					if (n < 0) error("ERROR writing to socket");
				}
			 }else if (strcmp(cmd[0],"Q")==0) goto exist;
		}
	}
	
	if(counter%cnt_n != 0)
	{
		for(int j = 0; j < counter%cnt_n; j++)
		{
			 bzero(buffer,1024);
			 n = read(client_sockfd,buffer,1024);
			 if (n < 0) error("ERROR reading from socket");
			 char *cmd[10];
			 parseLine(buffer,cmd);
			 // if R command
			 if (cmd[0][0]=='R')
			{
				c = atoi(cmd[1]);
				s = atoi(cmd[2]);
				cylinders_n[cnt++] = c;			
				if(c >= 0 && c <=  CYLINDERS-1 && s>=0 && s<=SECTORS-1)
				{
					char pbuf[1024] = "";
					for(int i = BLOCKSIZE * (c * SECTORS + s); i < BLOCKSIZE * (c * SECTORS + s+1); i++)
						pbuf[i - (BLOCKSIZE * (c * SECTORS + s))] = diskfile[i];
					n = write(client_sockfd,"G",2);
					if (n < 0) error("ERROR writing to socket");
				}
			 }
			//if W command
			 else if (cmd[0][0]=='W')
			{
				c = atoi(cmd[1]);
				s = atoi(cmd[2]);
				l = atoi(cmd[3]);	
				char *data = cmd[4];			
				cylinders_n[cnt++] = c;	
				if(c >= 0 && c <=  CYLINDERS-1 && s>=0 && s<=SECTORS-1 && l <= 256)
				{
					for(int i = 0; i < l; i++)
						diskfile[BLOCKSIZE * (c * SECTORS + s) + i] = data[i];
					for(int i = l; i < 256; i++)
						diskfile[BLOCKSIZE * (c * SECTORS + s) + i] = 0;
					n = write(client_sockfd,"G",2);
					if (n < 0) error("ERROR writing to socket");
				}
			 }else if (strcmp(cmd[0],"Q")==0) goto exist;
		}
	}

	exist: ;
	int fcfs_time = Ttime*abs(cylinders_n[0] - 0);
	for(int i = 0; i < cnt-1; i++) fcfs_time += Ttime*abs(cylinders_n[i+1] - cylinders_n[i]);

	usleep(Ttime*abs(fcfs_time));
	
	printf("FCFS queue: ");
	for(int i = 0; i < counter; i++) cout << cylinders_n[i] << " "; cout << endl;
	printf("FCFS time: %d\n", fcfs_time);	

	
	n = write(client_sockfd,"All is OK!",11);
	if (n < 0) error("ERROR writing to socket");
		
	
	delete [] cylinders_n;
	
	close(client_sockfd);
	close(sockfd);
	return 0; 
}
