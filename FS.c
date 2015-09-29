/*
Name:		FS.c
Author:		Hu Jingmei
Function:	The source code for File System Servers in File System. Be the server for the file system, also be the client for the disk-storage system server. Get the commands from the FC, handle them and send the command to BDS in order to store the information in real disk file.
*/
#include "cse356header.h"
using namespace std;

int CYLINDERS = 0;	//the number of cylinders
int SECTORS = 0;	//the number of sectors

char data[1024] = "";
char filedata[1024] = "";

int sockfd1, client_sockfd, BASIC_SERVER_PORT1;
int sockfd2, BASIC_SERVER_PORT2;

int tempc, temps, inodelistc, inodelists;
int cur_inode = 0, cur_c, cur_s;
int BLOCKSIZE = 256;
char buffer[1025];
char newbuffer[1025];
char filebuffer[1025];
char singlebuffer[1025];
int n;

int cs_change(int &c, int &s);
void readblock(int C,int S,char data[257]);
void writeblock(int C,int S,char data[257]);
void error(const char *msg);
int parseLine(char *line, char *command_array[]);

int main(int argc, char *argv[])
{
	//socket1
	socklen_t clilen;
	struct sockaddr_in serv_addr1;
	struct sockaddr_in cli_addr;

	//creating socket1
	BASIC_SERVER_PORT1 = atoi(argv[3]);
	sockfd1 = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd1 < 0)
		error("ERROR opening socket");
	bzero((char *) &serv_addr1, sizeof(serv_addr1));

	//binding socket
	serv_addr1.sin_family = AF_INET;
	serv_addr1.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr1.sin_port = htons(BASIC_SERVER_PORT1);
	if (bind(sockfd1, (struct sockaddr *) &serv_addr1, sizeof(serv_addr1)) < 0)
		error("ERROR on binding");//error check

	//listening and accepting connection
	listen(sockfd1,5);
	clilen = sizeof(cli_addr);
	client_sockfd = accept(sockfd1, (struct sockaddr *) &cli_addr, &clilen);
	if (client_sockfd < 0)
	  	error("ERROR on accept");

	//socket2
	struct sockaddr_in serv_addr2;
	struct hostent *server1;
	//creating socket2

	//Connecting A Client to A Server
	BASIC_SERVER_PORT2 = atoi(argv[2]);
	sockfd2 = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd2 < 0)
		error("ERROR opening socket");
	bzero((char *) &serv_addr2, sizeof(serv_addr2));

	server1 = gethostbyname(argv[1]);
	if (server1 == NULL) {	//error check
		fprintf(stderr,"ERROR, no such host\n");
		exit(0);
	}
	bzero((char *) &serv_addr2, sizeof(serv_addr2));
	serv_addr2.sin_family = AF_INET;
	bcopy((char *)server1->h_addr, (char *)&serv_addr2.sin_addr.s_addr, server1->h_length);
	serv_addr2.sin_port = htons(BASIC_SERVER_PORT2);
	if (connect(sockfd2,(struct sockaddr *) &serv_addr2,sizeof(serv_addr2)) < 0)
		error("ERROR connecting");//error check

	n = write(sockfd2,"0",2);	//send N	//useless for this problem

	//I command in order to get the cylinder and sector
	n = send(sockfd2, "I", 2, 0);
	if (n < 0)
		error("ERROR writing to socket");
	bzero(buffer,1024);
	n = recv(sockfd2, buffer,1024,0);
	if (n < 0)
		error("ERROR reading from socket");

	char *cmd[10];
	parseLine(buffer,cmd);
	CYLINDERS = atoi(cmd[0]);
	SECTORS = atoi(cmd[1]);

 	//initial the file
	cur_c = 0; cur_s = 3;//the root directory
	//get the current used inode list
	bzero(data,1024);
	readblock(0,2,data);
	for(temps = CYLINDERS*SECTORS-1; temps >=0; temps--) if(data[temps]=='1') break;
	cs_change(tempc, temps);
	inodelistc = tempc;
	inodelists = temps;

	while(1){

		 bzero(buffer,1024);
		 n = recv(client_sockfd,buffer,1024,0);
		 if (n < 0) error("ERROR reading from socket");
		 char *cmd[10];
		 parseLine(buffer,cmd);
// f
		 if (strcmp(cmd[0], "f")==0) {
			cout << "Formatting... " << endl << endl;
			//superblock
			strcpy(data,"");
			for(int i=0;i<256;++i) strcat(data,"0");
			writeblock(0,0,data);

			//inode bitmap--> 256 inodes
			strcpy(data,"");
			strcat(data,"1");
			for(int i=1;i<256;++i) strcat(data,"0");
			writeblock(0,1,data);

			//file block bitmap
			strcpy(data,"");
			for(int i = 0; i < CYLINDERS*SECTORS; i++) strcat(data,"0");//0-->not used
			for(int i = CYLINDERS*SECTORS; i < 256; i++) strcat(data,"1");//1-->used or disabled
			writeblock(0,2,data);

			//inode list
			tempc = 0; temps = 3+32;
			cs_change(tempc, temps);
			strcpy(data,"");
			for(int i=0; i<256;++i) strcat(data,"0");
			for(int i = 0; i <= tempc; i ++)
			{
				for(int j = 0; (i!=tempc&&j<SECTORS)||(i==tempc&&j<temps); j++)
				{
					if(i == 0 && j < 3) continue;
					else if( i == 0 && j == 3) {data[6] = tempc+'0'; data[7] = temps+'0';}
					else for(int i = 0; i < 256;i++) data[i] = '0';
					writeblock(i,j,data);
				}
			}
			inodelistc = tempc;
			inodelists = temps;

			//root directory
			strcpy(data,"");
			for(int i=0;i<256;++i) strcat(data,"0");
			for(int i = 0; i < 8; i++)
			{
				for(int j = 0; j < 3; j++)
					data[i*32+j] = '0';//0&0 for CYLINDERS and SECTORD, 0 for dir or file
				for(int j = 3; j < 32; j++)
					data[i*32+j] = '@';
			}
			writeblock(tempc,temps,data);


			//update file block bitmap
			strcpy(data,"");
			for(int i=0;i<256;++i) strcat(data,"0");
			for(int i = 0; i <= tempc*SECTORS+temps; i++)	data[i] = '1';
			for(int i = tempc*SECTORS+temps+1; i < CYLINDERS*SECTORS; i++) data[i] = '0';
			for(int i = CYLINDERS*SECTORS; i < 256; i++) data[i] = '1';
			writeblock(0,2,data);

			cout << "Formatting successfully!" << endl;
			n = send(client_sockfd,"Y",2,0);
		 	cur_c = 0; cur_s = 3;
		 }
// mk
		 else if (strcmp(cmd[0], "mk")==0)
		{
			cout << "Making file..."<< endl;

			char init[30] = "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@";
			int len = strlen(cmd[1]);
			strncat(cmd[1], init, 29-len);
			char name[33];
			for(int i = 0; i < 33; i++) name[i] = cmd[1][i];

			int flag = 0;//initial
			int empty = 0, single = 0;
			int empty_c = 0, empty_s = 0, pos = 0, rc = 0, rs = 0, fc = 0, fs = 0, fpos1 = 0, fpos2 = 0,fpos3=0;
			bzero(data,1024);
			readblock(cur_c, cur_s, data);
			if(data[4] == '0' && data[5] == '0')
			{//create in root dir
				for(int i = 0; i <= inodelistc; i ++)
				{
					for(int j = 0; (i!=inodelistc&&j<SECTORS)||(i==inodelistc&&j<inodelists); j++)
					{
						if(i == 0 && j < 3) continue;
						if(flag == 2) goto exist1;
						bzero(data,1024);
						readblock(i, j, data);
						//find the position
						for(int k = 0; k < 8; k++)//32 inodes per block
						{
							for(int m = 6; m < 30; m=m+2)//12 direct inodes
							{
								if(flag == 2) goto exist1; //the file name exist
								rc = data[k*32+m]-'0'; rs = data[k*32+m+1]-'0';//get c & s
								if(rc == 0 && rs == 0) //not setup yet
								{fc = i; fs = j; fpos1 = k; fpos2 = m; goto exist1;}//get the father inode
								else {
									bzero(filebuffer,1024);
									readblock(rc,rs,filebuffer);
									char existname[33] = "";
									for(int l = 0; l < 8; l ++)
									//8 direct inode file per block, 32 bit for each file name
									{
										for(int p = 3; p < 32; p++) existname[p-3]=filebuffer[l*32+p];
										existname[32] = '\0';
										if((strcmp(existname,name)==0)&&(filebuffer[l*32+2]=='0')) {flag = 2;} //2 --> found the same name file
										if((strcmp(existname, init) ==0) && empty==0)
										{empty++;empty_c=rc;empty_s=rs;pos=l;fc=i;fs=j;fpos1 = k;}
										//empty !=  --> found a empty space
									}
								}
							}
							if(flag != 2)//1 single indirect inode
							{
								rc = data[k*32+30]-'0'; rs = data[k*32+31]-'0';//get c & s
								if(rc == 0 && rs == 0) goto exist1;
								else {
									bzero(singlebuffer,1024);
									readblock(rc,rs,singlebuffer);
									for(int q = 0; q < 8; q++)//32 inodes per block
									{
									for(int m = 6; m < 30; m=m+2)//12 direct inodes
									{	rc = singlebuffer[q*32+m]-'0';
										rs = singlebuffer[q*32+m+1]-'0';
										if(rc == 0 && rs == 0) //not setup yet
									{fc=cur_c;fs=cur_s;fpos1=k;fpos2=q;fpos3=m;single=1;goto exist1;}
										bzero(filebuffer,1024);
										readblock(rc,rs,filebuffer);
										char existname[33] = "";
										for(int l = 0; l < 8; l ++)
										//8 direct inode file per block, 32 bit for each file name
										{
											for(int p=3;p<32;p++)existname[p-3]=filebuffer[l*32+p];
											existname[32] = '\0';
											if((strcmp(existname, name)==0)&&(filebuffer[l*32+2]=='0')) {flag = 2;}
											//2 --> found the same name file
											if((strcmp(existname, init) ==0) && empty==0)
										{empty++;empty_c=rc;empty_s=rs;pos=l;fc=i; fs = j;fpos1 = k;}
											//empty !=  --> found a empty space
										}
									}
									}
								}
							}
						}
					}
				}
			}
			else //create in current dir
			{
				bzero(data,1024);
				readblock(cur_c, cur_s, data);
				//find
				for(int k = 0; k < 8; k++)//32 inodes per block
				{
					for(int m = 6; m < 30; m=m+2)//12 direct inodes
					{
						if(flag == 2) goto exist1; //the file name exist
						rc = data[k*32+m]-'0'; rs = data[k*32+m+1]-'0';//get c & s
						if(rc == 0 && rs == 0) //not setup yet
							{fc = cur_c; fs = cur_s; fpos1 = k; fpos2 = m; goto exist1;}//get the father inode
						else {
							bzero(filebuffer,1024);
							readblock(rc,rs,filebuffer);//get the exist file name
							char existname[33] = "";
							for(int l = 0; l < 8; l ++)
							//8 direct inode file per block, 32 bit for each file name
							{	for(int p = 3; p < 32; p++) existname[p-3] = filebuffer[l*32+p];
								existname[32] = '\0';
								if((strcmp(existname, name)==0)&&(filebuffer[l*32+2]=='0')) {flag = 2;}
								//2 --> found the same name file
								if((strcmp(existname, init) ==0) && empty==0)
								{empty++;empty_c = rc;empty_s = rs;pos = l;fc = cur_c;fs = cur_s;fpos1 = k;}
								//empty !=  --> found a empty space
							}
						}
					}
					if(flag != 2)//1 single indirect inode
					{
						rc = data[k*32+30]-'0'; rs = data[k*32+31]-'0';//get c & s
						if(rc == 0 && rs == 0) goto exist1;
						else {
							bzero(singlebuffer,1024);
							readblock(rc,rs,singlebuffer);

							for(int q = 0; q < 8; q++)//32 inodes per block
							{
							for(int m = 6; m < 30; m=m+2)//12 direct inodes
							{	rc = singlebuffer[q*32+m]-'0';
								rs = singlebuffer[q*32+m+1]-'0';
								if(rc == 0 && rs == 0) //not setup yet
								{fc=cur_c;fs=cur_s;fpos1=k;fpos2=q;fpos3=m;single=1;goto exist1;}
								bzero(filebuffer,1024);
								readblock(rc,rs,filebuffer);//get the exist file name
								char existname[33] = "";
								for(int l = 0; l < 8; l ++)
								//8 direct inode file per block, 32 bit for each file name
								{	for(int p = 3; p < 32; p++) existname[p-3] = filebuffer[l*32+p];
									existname[32] = '\0';
									if((strcmp(existname, name)==0)&&(filebuffer[l*32+2]=='0')) {flag = 2;}
									//2 --> found the same name file
									if((strcmp(existname, init) ==0) && empty==0)
									{empty++;empty_c=rc;empty_s=rs;pos=l;fc=cur_c;fs=cur_s;fpos1 = k;}
									//empty !=  --> found a empty space
								}
							}
							}
						}
					}
				}
			}

exist1: ;
			//check the empty file block
			bzero(data,1024);
			readblock(0,2,data);
			int ep=0;
			for(int ct = CYLINDERS*SECTORS-1; ct >=0; ct--) if(data[ct]=='0') {ep=1; break;}
			if(ep==0) empty=0;

			if(empty_c == 0 && empty_s == 0 && single == 0)//new inode in inode list
			{
				empty++;
				//update tempc and temps
				temps++;
				cs_change(tempc, temps);
				//another root directory
				strcpy(data,"");
				for(int i=0;i<256;++i) strcat(data,"0");
				for(int i = 0; i < 8; i++)
				{
					for(int j = 0; j < 3; j++)
						data[i*32+j] = '0';//00 for CYLINDERS and SECTORD, 0 for dir or file
					for(int j = 3; j < 32; j++)
						data[i*32+j] = '@';
				}
				writeblock(tempc,temps,data);

				//update file block bitmap
				strcpy(data,"");
				for(int i=0;i<256;++i) strcat(data,"0");
				for(int i = 0; i <= tempc*SECTORS+temps; i++)	data[i] = '1';
				for(int i = tempc*SECTORS+temps+1; i < CYLINDERS*SECTORS; i++) data[i] = '0';
				for(int i = CYLINDERS*SECTORS; i < 256; i++) data[i] = '1';
				writeblock(0,2,data);

				//update inode bitmap
				bzero(data, 1024);
				readblock(0, 1, data);
				data[++cur_inode] = '1';
				writeblock(0,1,data);

				//update inode list
				bzero(data, 1024);
				readblock(fc, fs, data);
				data[fpos1*32+fpos2] = tempc+'0';
				data[fpos1*32+fpos2+1] = temps+'0';
				writeblock(fc,fs,data);

				empty_c = tempc; empty_s = temps; pos = 0;
			}
			else if(empty_c == 0 && empty_s == 0 && single == 1)//new indirect inode list in inode list
			{
				empty++;
				//update tempc and temps
				temps++;
				cs_change(tempc, temps);
				//indirect inode list
 				strcpy(data,"");
				for(int i=0;i<256;++i) strcat(data,"0");
				writeblock(tempc,temps,data);


				//update tempc and temps
				temps++;
				cs_change(tempc, temps);

				//update inode list
				bzero(data, 1024);
				readblock(fc, fs, data);
				data[fpos1*32+30] = tempc+'0';
				data[fpos1*32+31] = temps+'0';
				writeblock(fc,fs,data);
				fc = tempc; fs = temps;

				//another root directory
				strcpy(data,"");
				for(int i=0;i<256;++i) strcat(data,"0");
				for(int i = 0; i < 8; i++)
				{
					for(int j = 0; j < 3; j++)
						data[i*32+j] = '0';//0 & 0 for CYLINDERS and SECTORD, 0 for dir or file
					for(int j = 3; j < 32; j++)
						data[i*32+j] = '@';
				}
				writeblock(tempc,temps,data);

				//update file block bitmap
				strcpy(data,"");
				for(int i=0;i<256;++i) strcat(data,"0");
				for(int i = 0; i <= tempc*SECTORS+temps; i++)	data[i] = '1';
				for(int i = tempc*SECTORS+temps+1; i < CYLINDERS*SECTORS; i++) data[i] = '0';
				for(int i = CYLINDERS*SECTORS; i < 256; i++) data[i] = '1';
				writeblock(0,2,data);

				//update inode list
				bzero(data, 1024);
				readblock(fc, fs, data);
				data[fpos2*32+fpos3] = tempc+'0';
				data[fpos2*32+fpos3+1] = temps+'0';
				writeblock(fc,fs,data);

				empty_c = tempc; empty_s = temps; pos = 0;
			}
			if(flag == 2) n = send(client_sockfd,"1",2,0);//exist
			else if(flag == 0 && empty != 0)//not exist and have space to create
			{
				bzero(filebuffer,1024);
				readblock(empty_c,empty_s,filebuffer);//get the non-exist file name

				//update tempc and temps
				temps++;
				cs_change(tempc, temps);
				int inode_c = tempc, inode_s = temps;

				//change the empty inode
				for(int ii = pos*32+3; ii < (pos+1)*32; ii++) filebuffer[ii] = name[ii-(pos*32+3)];
				filebuffer[pos*32] = tempc+'0'; filebuffer[pos*32+1] = temps+'0'; filebuffer[pos*32+2] = '0';
				writeblock(empty_c, empty_s, filebuffer);

				//update tempc and temps
				temps++;
				cs_change(tempc, temps);
				int file_c = tempc, file_s = temps;

				//setup the file inode
				strcpy(data,"");
				for(int ii=0;ii<256;++ii) strcat(data,"0");
				for(int ii = 0; ii < 256; ii++) data[ii] = '0';
				for(int ii = 0; ii < 8; ii++)
					{data[4+ii*32] = fc+'0'; data[5+ii*32] = fs+'0';}//the father inode
				data[6] = file_c+'0'; data[7] = file_s+'0';//the file block inode
				data[256] = '\0';
				writeblock(inode_c, inode_s, data);

				//setup the file content block
				strcpy(data,"");
				for(int ii=0;ii<256;++ii) strcat(data,"0");
				writeblock(file_c, file_s, data);

				//update inode list
				bzero(data, 1024);
				readblock(fc, fs, data);
				data[fpos1*32+3] = data[fpos1*32+3] + 32;
				if(data[fpos1*32+3] > '9')
				{data[fpos1*32+2] += (data[fpos1*32+3]-'0')/10; data[fpos1*32+3] = (data[fpos1*32+3]-'0')%10+'0'; }
				if(data[fpos1*32+2] > '9')
				{data[fpos1*32+1] += (data[fpos1*32+2]-'0')/10; data[fpos1*32+2] = (data[fpos1*32+2]-'0')%10+'0'; }
				writeblock(fc, fs, data);

				//update file block bitmap
				strcpy(data,"");
				for(int i=0;i<256;++i) strcat(data,"0");
				for(int i = 0; i <= tempc*SECTORS+temps; i++)	data[i] = '1';
				for(int i = tempc*SECTORS+temps+1; i < CYLINDERS*SECTORS; i++) data[i] = '0';
				for(int i = CYLINDERS*SECTORS; i < 256; i++) data[i] = '1';
				writeblock(0,2,data);

				n = send(client_sockfd,"0",2,0);//create successfully
			}
			else n = send(client_sockfd,"2",2,0);//fail

			cout << endl << "Making file finished"<< endl;

		 }
//mkdir
		 else if (strcmp(cmd[0], "mkdir")==0)
		 {
			cout << "Making directory..."<< endl;

			char init[30] = "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@";
			int len = strlen(cmd[1]);
			strncat(cmd[1], init, 29-len);
			char name[33];
			for(int i = 0; i < 33; i++) name[i] = cmd[1][i];

			strcpy(data,"");
			for(int i=0;i<256;++i) strcat(data,"0");
			int flag = 0;//initial
			int empty = 0, single =0;
			int empty_c = 0, empty_s = 0, pos = 0, rc = 0, rs = 0, fc = 0, fs = 0, fpos1 = 0, fpos2 = 0, fpos3=0;
			bzero(data,1024);
			readblock(cur_c, cur_s, data);
			if(data[4] == '0' && data[5] == '0')
			{//create in root dir
				for(int i = 0; i <= inodelistc; i ++)
				{
					for(int j = 0; (i!=inodelistc&&j<SECTORS)||(i==inodelistc&&j<inodelists); j++)
					{
						if(i == 0 && j < 3) continue;
						if(flag == 2) goto exist2;
						readblock(i, j, data);

						//find
						for(int k = 0; k < 8; k++)//32 inodes per block
						{
							for(int m = 6; m < 30; m=m+2)//12 direct inodes
							{
								if(flag == 2) goto exist2;
								rc = data[k*32+m]-'0'; rs = data[k*32+m+1]-'0';//get c & s
								if(rc == 0 && rs == 0) //not setup yet
								{fc = i; fs = j; fpos1 = k; fpos2 = m; goto exist2;}//get the father inode
								else {
									bzero(filebuffer,1024);
									readblock(rc,rs,filebuffer);//get the exist file name
									char existname[33] = "";
									for(int l = 0; l < 8; l ++)
									//8 direct inode file per block, 32 bit for each file name
									{
										for(int p = 3;p<32; p++) existname[p-3] = filebuffer[l*32+p];
										existname[32] = '\0';
										if((strcmp(existname, name)==0)&&(filebuffer[l*32+2]=='1')) {flag = 2;}
										//2 --> found the same name file
										if((strcmp(existname, init) ==0) && empty==0)
										{empty++;empty_c=rc;empty_s=rs;pos=l;fc=i;fs = j;fpos1 = k;}
										//empty !=  --> found a empty space
									}
								}
							}
							if(flag != 2)//1 single indirect inode
							{
								rc = data[k*32+30]-'0'; rs = data[k*32+31]-'0';//get c & s
								//cout << " c = " << rc << " s = " << rs << endl;
								if(rc == 0 && rs == 0) goto exist2;
								else {
									bzero(singlebuffer,1024);
									readblock(rc,rs,singlebuffer);

									for(int q = 0; q < 8; q++)//32 inodes per block
									{
									for(int m = 6; m < 30; m=m+2)//12 direct inodes
									{	rc = singlebuffer[q*32+m]-'0';
										rs = singlebuffer[q*32+m+1]-'0';
										if(rc == 0 && rs == 0) //not setup yet
									{fc=cur_c;fs=cur_s;fpos1=k;fpos2=q;fpos3=m;single=1;goto exist2;}
										bzero(filebuffer,1024);
										readblock(rc,rs,filebuffer);//get the exist file name
										char existname[33] = "";
										for(int l = 0; l < 8; l ++)
										//8 direct inode file per block, 32 bit for each file name
										{	for(int p = 3; p < 32; p++) existname[p-3] = filebuffer[l*32+p];
											existname[32] = '\0';
											if((strcmp(existname, name)==0)&&(filebuffer[l*32+2]=='1')) {flag = 2;}
											//2 --> found the same name file
											if((strcmp(existname, init) ==0) && empty==0)
											{empty++; empty_c = rc; empty_s = rs; pos = l; fc = i; fs = j;fpos1 = k;}
											//empty !=  --> found a empty space
										}
									}
									}
								}
							}
						}
					}
				}
			}
			else //create in current dir
			{
				//find
				for(int k = 0; k < 8; k++)//32 inodes per block
				{
					for(int m = 6; m < 30; m=m+2)//12 direct inodes
					{
						if(flag == 2) goto exist2;
						rc = data[k*32+m]-'0'; rs = data[k*32+m+1]-'0';//get c & s
						if(rc == 0 && rs == 0) //not setup yet
						{fc = cur_c; fs = cur_s; fpos1 = k; fpos2 = m; goto exist2;}//get the father inode
						else {
							bzero(filebuffer,1024);
							readblock(rc,rs,filebuffer);//get the exist file name
							char existname[33] = "";
							for(int l = 0; l < 8; l ++)
							//8 direct inode file per block, 32 bit for each file name
							{	for(int p = 3; p < 32; p++) existname[p-3] = filebuffer[l*32+p];
								existname[32] = '\0';
								if((strcmp(existname, name)==0)&&(filebuffer[l*32+2]=='0')) {flag = 2;}
								//2 --> found the same name file
								if((strcmp(existname, init) ==0) && empty==0)
								{empty++; empty_c = rc; empty_s = rs;pos = l;fc = cur_c;fs = cur_s;fpos1 = k;}
								//empty !=  --> found a empty space
							}
						}
					}
					if(flag != 2)//1 single indirect inode
					{
						rc = data[k*32+30]-'0'; rs = data[k*32+31]-'0';//get c & s
						if(rc == 0 && rs == 0) goto exist2;
						else {
							bzero(singlebuffer,1024);
							readblock(rc,rs,singlebuffer);

							for(int q = 0; q < 8; q++)//32 inodes per block
							{
							for(int m = 6; m < 30; m=m+2)//12 direct inodes
							{	rc = singlebuffer[q*32+m]-'0';
								rs = singlebuffer[q*32+m+1]-'0';
								if(rc == 0 && rs == 0) //not setup yet
									{fc=cur_c;fs=cur_s;fpos1=k;fpos2=q;fpos3=m;single=1;goto exist2;}
								bzero(filebuffer,1024);
								readblock(rc,rs,filebuffer);//get the exist file name
								char existname[33] = "";
								for(int l = 0; l < 8; l ++)
								//8 direct inode file per block, 32 bit for each file name
								{	for(int p = 3; p < 32; p++) existname[p-3] = filebuffer[l*32+p];
									existname[32] = '\0';
									if((strcmp(existname, name)==0)&&(filebuffer[l*32+2]=='0')) {flag = 2;}
									//2 --> found the same name file
									if((strcmp(existname, init) ==0) && empty==0)
									{empty++;empty_c = rc;empty_s = rs;pos=l;fc=cur_c;fs=cur_s;fpos1 = k;}
									//empty !=  --> found a empty space
								}
							}
							}
						}
					}
				}
			}
exist2: ;
			//check the empty file block
			bzero(data,1024);
			readblock(0,2,data);
			int ep=0;
			for(int ct = CYLINDERS*SECTORS-1; ct >=0; ct--) if(data[ct]=='0') {ep=1; break;}
			if(ep==0) empty=0;

			if(empty_c == 0 && empty_s == 0 && single == 0)//new inode in inode list
			{
				empty++;
				//update tempc and temps
				temps++;
				cs_change(tempc, temps);
				//another root directory
				strcpy(data,"");
				for(int i=0;i<256;++i) strcat(data,"0");
				for(int i = 0; i < 8; i++)
				{
					for(int j = 0; j < 2; j++)
						data[i*32+j] = '0';//00 for CYLINDERS and SECTORD, 1 for dir
					data[i*32+2] = '1';
					for(int j = 3; j < 32; j++)
						data[i*32+j] = '@';
				}
				writeblock(tempc,temps,data);


				//update inode bitmap
				bzero(data, 1024);
				readblock(0, 1, data);
				data[++cur_inode] = '1';
				writeblock(0,1,data);

				//update inode list
				bzero(data, 1024);
				readblock(fc, fs, data);
				data[fpos1*32+fpos2] = tempc+'0';
				data[fpos1*32+fpos2+1] = temps+'0';
				data[fpos1*32]++; //blocknum++
				writeblock(fc,fs,data);

				//update file block bitmap
				strcpy(data,"");
				for(int i=0;i<256;++i) strcat(data,"0");
				for(int i = 0; i <= tempc*SECTORS+temps; i++)	data[i] = '1';
				for(int i = tempc*SECTORS+temps+1; i < CYLINDERS*SECTORS; i++) data[i] = '0';
				for(int i = CYLINDERS*SECTORS; i < 256; i++) data[i] = '1';
				writeblock(0,2,data);

				empty_c = tempc; empty_s = temps; pos = 0;
			}
			else if(empty_c == 0 && empty_s == 0 && single == 1)//new indirect inode list in inode list
			{
				empty++;
				//update tempc and temps
				temps++;
				cs_change(tempc, temps);
				//indirect inode list
 				strcpy(data,"");
				for(int i=0;i<256;++i) strcat(data,"0");
				writeblock(tempc,temps,data);


				//update tempc and temps
				temps++;
				cs_change(tempc, temps);

				//update inode list
				bzero(data, 1024);
				readblock(fc, fs, data);
				data[fpos1*32+30] = tempc+'0';
				data[fpos1*32+31] = temps+'0';
				writeblock(fc,fs,data);
				fc = tempc; fs = temps;

				//another root directory
				strcpy(data,"");
				for(int i=0;i<256;++i) strcat(data,"0");
				for(int i = 0; i < 8; i++)
				{
					for(int j = 0; j < 3; j++)
						data[i*32+j] = '0';//0 & 0 for CYLINDERS and SECTORD, 0 for dir or file
					for(int j = 3; j < 32; j++)
						data[i*32+j] = '@';
				}
				writeblock(tempc,temps,data);

				//update file block bitmap
				strcpy(data,"");
				for(int i=0;i<256;++i) strcat(data,"0");
				for(int i = 0; i <= tempc*SECTORS+temps; i++)	data[i] = '1';
				for(int i = tempc*SECTORS+temps+1; i < CYLINDERS*SECTORS; i++) data[i] = '0';
				for(int i = CYLINDERS*SECTORS; i < 256; i++) data[i] = '1';
				writeblock(0,2,data);

				//update inode list
				bzero(data, 1024);
				readblock(fc, fs, data);
				data[fpos2*32+fpos3] = tempc+'0';
				data[fpos2*32+fpos3+1] = temps+'0';
				writeblock(fc,fs,data);

				empty_c = tempc; empty_s = temps; pos = 0;
			}
			if(flag == 2) n = send(client_sockfd,"1",2,0);//exist
			else if(flag == 0 && empty != 0)//not exist and have space to create
			{
				bzero(filebuffer,1024);
				readblock(empty_c,empty_s,filebuffer);//get the non-exist file name

				//update tempc and temps
				temps++;
				cs_change(tempc, temps);
				int inode_c = tempc, inode_s = temps;

				//change the empty inode
				for(int ii = pos*32+3; ii < (pos+1)*32; ii++) filebuffer[ii] = name[ii-(pos*32+3)];
				filebuffer[pos*32] = tempc+'0'; filebuffer[pos*32+1] = temps+'0'; filebuffer[pos*32+2] = '1';
				writeblock(empty_c, empty_s, filebuffer);

				//update tempc and temps
				temps++;
				cs_change(tempc, temps);
				int dir_c = tempc, dir_s = temps;

				//setup the file inode
				strcpy(data,"");
				for(int ii=0;ii<256;++ii) strcat(data,"0");
				for(int ii = 0; ii < 256; ii++) data[ii] = '0';
				for(int ii = 0; ii < 8; ii++)
					{data[4+ii*32] = fc+'0'; data[5+ii*32] = fs+'0';}//the father inode
				data[6] = dir_c+'0'; data[7] = dir_s+'0';//the file block inode
				data[256] = '\0';
				writeblock(inode_c, inode_s, data);

				//setup the subdirectory block
				strcpy(data,"");
				for(int i=0;i<256;++i) strcat(data,"0");
				for(int i = 0; i < 8; i++)
				{
					for(int j = 0; j < 3; j++)
						data[i*32+j] = '0';//00 for CYLINDERS and SECTORD, 0 for file
					for(int j = 3; j < 32; j++)
						data[i*32+j] = '@';
				}
				writeblock(dir_c, dir_s,data);

				//update inode list
				bzero(data, 1024);
				readblock(fc, fs, data);
				data[fpos1*32+3] = data[fpos1*32+3] + 32;
				if(data[fpos1*32+3] > '9')
					{data[fpos1*32+2] += (data[fpos1*32+3]-'0')/10; data[fpos1*32+3] = (data[fpos1*32+3]-'0')%10+'0'; }
				if(data[fpos1*32+2] > '9')
					{data[fpos1*32+1] += (data[fpos1*32+2]-'0')/10; data[fpos1*32+2] = (data[fpos1*32+2]-'0')%10+'0'; }
				writeblock(fc, fs, data);

				//update file block bitmap
				strcpy(data,"");
				for(int i=0;i<256;++i) strcat(data,"0");
				for(int i = 0; i <= tempc*SECTORS+temps; i++)	data[i] = '1';
				for(int i = tempc*SECTORS+temps+1; i < CYLINDERS*SECTORS; i++) data[i] = '0';
				for(int i = CYLINDERS*SECTORS; i < 256; i++) data[i] = '1';
				writeblock(0,2,data);

				n = send(client_sockfd,"0",2,0);//create successfully
			}
			else n = send(client_sockfd,"2",2,0);//fail

			cout << endl << "Making directory finished"<< endl;

		 }
//rm
		else if (strcmp(cmd[0], "rm")==0)
		{
			cout << "Deleting file..."<< endl;

			char init[30] = "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@";
			char ori[33] = "000@@@@@@@@@@@@@@@@@@@@@@@@@@@@@";
			int len = strlen(cmd[1]);
			strncat(cmd[1], init, 29-len);
			char name[33];
			for(int i = 0; i < 33; i++) name[i] = cmd[1][i];

			bzero(data,1024);
			readblock(cur_c, cur_s, data);
			int flag = 0, pos = 0, pos2 = 0, rc = 0, rs = 0, fc = 0, fs = 0, filec = 0, files = 0;
			if(data[4] == '0' && data[5] == '0')
			{//delete in root dir
				for(int i = 0; i <= inodelistc; i ++)
				{
					for(int j = 0; (i!=inodelistc&&j<SECTORS)||(i==inodelistc&&j<inodelists); j++)
					{
						if(i == 0 && j < 3) continue;
						if(flag == 2) goto delfile;
						bzero(data,1024);
						readblock(i, j, data);

						//find
						for(int k = 0; k < 8; k++)//32 inodes per block
						{
							for(int m = 6; m < 30; m=m+2)//12 direct inodes
							{
								if(flag == 2) goto delfile;
								rc = data[k*32+m]-'0'; rs = data[k*32+m+1]-'0';//get c & s
								if(rc == 0 && rs == 0) goto delfile;
								else {
									bzero(filebuffer,1024);
									readblock(rc,rs,filebuffer);//get the exist file name
									char existname[33] = "";
									for(int l = 0; l < 8; l ++)
									//8 direct inode file per block, 32 bit for each file name
									{	for(int p = 3; p < 32; p++) existname[p-3] = filebuffer[l*32+p];
										existname[32] = '\0';
										if((strcmp(existname, name)==0)&&(filebuffer[l*32+2]=='0')) 												{flag = 2; pos = l; pos2 = k; fc = i; fs = j;}
									}
								}
							}
							if(flag != 2)//1 single indirect inode
							{
								rc = data[k*32+30]-'0'; rs = data[k*32+31]-'0';//get c & s
								if(rc == 0 && rs == 0) goto delfile;
								else {
									bzero(singlebuffer,1024);
									readblock(rc,rs,singlebuffer);

									for(int q = 0; q < 8; q++)//32 inodes per block
									{
									for(int m = 6; m < 30; m=m+2)//12 direct inodes
									{	rc = singlebuffer[q*32+m]-'0';
										rs = singlebuffer[q*32+m+1]-'0';
										if(rc == 0 && rs == 0) goto delfile;
										bzero(filebuffer,1024);
										readblock(rc,rs,filebuffer);//get the exist file name
										char existname[33] = "";
										for(int l = 0; l < 8; l ++)
										//8 direct inode file per block, 32 bit for each file name
										{
										for(int p=3;p<32;p++)existname[p-3]=filebuffer[l*32+p];
											existname[32] = '\0';
											if((strcmp(existname,name)==0)&&(filebuffer[l*32+2]=='0')) //exist
												{flag = 2; pos = l; pos2 = k;fc = i; fs = j;}
										}
									}
									}
								}
							}
						}
					}
				}
			}
			else //delete in current dir
			{
				//find
				for(int k = 0; k < 8; k++)//32 inodes per block
				{
					for(int m = 6; m < 30; m=m+2)//12 direct inodes
					{
						if(flag == 2) goto delfile;
						rc = data[k*32+m]-'0'; rs = data[k*32+m+1]-'0';//get c & s
						if(rc == 0 && rs == 0) goto delfile;
						else {
							bzero(filebuffer,1024);
							readblock(rc,rs,filebuffer);//get the exist file name
							char existname[33] = "";
							for(int l = 0; l < 8; l ++)
							//8 direct inode file per block, 32 bit for each file name
							{	for(int p = 3; p < 32; p++) existname[p-3] = filebuffer[l*32+p];
								existname[32] = '\0';
								if((strcmp(existname, name)==0)&&(filebuffer[l*32+2]=='0'))
									{flag = 2; pos = l; pos2 = k;fc = cur_c; fs = cur_s;}
							}
						}
					}
					if(flag != 2)//1 single indirect inode
					{
						rc = data[k*32+30]-'0'; rs = data[k*32+31]-'0';//get c & s
						if(rc == 0 && rs == 0) goto delfile;
						else {
							bzero(singlebuffer,1024);
							readblock(rc,rs,singlebuffer);

							for(int q = 0; q < 8; q++)//32 inodes per block
							{
							for(int m = 6; m < 30; m=m+2)//12 direct inodes
							{	rc = singlebuffer[q*32+m]-'0';
								rs = singlebuffer[q*32+m+1]-'0';
								if(rc == 0 && rs == 0) goto delfile;
								bzero(filebuffer,1024);
								readblock(rc,rs,filebuffer);//get the exist file name
								char existname[33] = "";
								for(int l = 0; l < 8; l ++)
								//8 direct inode file per block, 32 bit for each file name
								{	for(int p = 3; p < 32; p++) existname[p-3] = filebuffer[l*32+p];
									existname[32] = '\0';
									if((strcmp(existname, name)==0)&&(filebuffer[l*32+2]=='0'))
										{flag = 2;pos = l; pos2 = k;fc = cur_c; fs = cur_s;}
								}
							}
							}
						}
					}
				}
			}
			delfile: ;
			if(flag == 2)
			{
				n = send(client_sockfd,"0",2,0);//exist and delete
				for(int i = pos*32; i < (pos+1)*32; i++) filebuffer[i] = ori[i-pos*32];
				writeblock(rc, rs, filebuffer);

				//update inode list
				bzero(data, 1024);
				readblock(fc, fs, data);
				data[pos2*32+3] = (data[pos2*32+1]-'0')*100+(data[pos2*32+2]-'0')*10+data[pos2*32+3] - 32;
				data[pos2*32+1] = '0'; data[pos2*32+2] = '0';
				if(data[pos2*32+3] > '9')
					{data[pos2*32+2] += (data[pos2*32+3]-'0')/10; data[pos2*32+3] = (data[pos2*32+3]-'0')%10+'0'; }
				if(data[pos2*32+2] > '9')
					{data[pos2*32+1] += (data[pos2*32+2]-'0')/10; data[pos2*32+2] = (data[pos2*32+2]-'0')%10+'0'; }
				writeblock(fc, fs, data);

			}
			else if(flag == 0) n = send(client_sockfd,"1",2,0);//not exist
			else n = send(client_sockfd,"2",2,0);//failed

			cout << endl << "Deleting file finished"<< endl;
		 }
//cd
		else if (strcmp(cmd[0], "cd")==0)
		{
			cout << "Changing directory..."<< endl;
			int initBit=0;
			int returnflag=1;
			bzero(data, 1024);
			int fc = 0, fs = 0;
			int flag = 0, rc = 0, rs = 0;
			char filename[33]={0};
			int count=0;
			// cd . or cd ..
			if(cmd[1][0]=='.')//  ./path
			{
				if(cmd[1][1]=='.')//	../path
				{
					readblock(cur_c, cur_s, data);
					cur_c = data[4]-'0'; cur_s = data[5]-'0';
					initBit=2;
				}
				else initBit=1;
			}
			for(int i=initBit; cmd[1][i]!=0; i++)
			{
				if(cmd[1][i]=='/') continue;
				filename[count++] = cmd[1][i];
				if(cmd[1][i+1]=='/' || cmd[1][i+1]=='\0')
				{

					for(int i=0;i<29;i++) filename[i] = (filename[i]==0)?'@':filename[i];

			bzero(data,1024);
			readblock(cur_c, cur_s, data);
			if(data[4] == '0' && data[5] == '0')
			{//change in root dir
				for(int i = 0; i <= inodelistc; i ++)
				{
					for(int j = 0; (i!=inodelistc&&j<SECTORS)||(i==inodelistc&&j<inodelists); j++)
					{
						if(i == 0 && j < 3) continue;
						if(flag == 2) goto change;
						bzero(data,1024);
						readblock(i, j, data);

						//find
						for(int k = 0; k < 8; k++)//32 inodes per block
						{
							for(int m = 6; m < 30; m=m+2)//12 direct inodes
							{
								if(flag == 2) goto change;
								rc = data[k*32+m]-'0'; rs = data[k*32+m+1]-'0';//get c & s
								if(rc == 0 && rs == 0)  goto change;
								else {
									bzero(filebuffer,1024);
									readblock(rc,rs,filebuffer);//get the exist file name
									char existname[33] = "";
									for(int l = 0; l < 8; l ++)
									//8 direct inode file per block, 32 bit for each file name
									{	for(int p = 3; p < 32; p++) existname[p-3] = filebuffer[l*32+p];
										existname[32] = '\0';
										if((strcmp(existname,filename)==0)&&(filebuffer[l*32+2]=='1')) 											{flag = 1;fc=filebuffer[l*32]-'0'; fs =filebuffer[l*32+1]-'0';}
									}
								}
							}
							if(flag != 1)//1 single indirect inode
							{
								rc = data[k*32+30]-'0'; rs = data[k*32+31]-'0';//get c & s
								if(rc == 0 && rs == 0) goto change;
								else
								{
									bzero(singlebuffer,1024);
									readblock(rc,rs,singlebuffer);

									for(int q = 0; q < 8; q++)//32 inodes per block
									{
									for(int m = 6; m < 30; m=m+2)//12 direct inodes
									{	rc = singlebuffer[q*32+m]-'0';
										rs = singlebuffer[q*32+m+1]-'0';
										if(rc == 0 && rs == 0) goto change;
										bzero(filebuffer,1024);
										readblock(rc,rs,filebuffer);//get the exist file name
										char existname[33] = "";
										for(int l = 0; l < 8; l ++)
										//8 direct inode file per block, 32 bit for each file name
										{	for(int p = 3; p < 32; p++) existname[p-3] = filebuffer[l*32+p];
											existname[32] = '\0';
										if((strcmp(existname,filename)==0)&&(filebuffer[l*32+2]=='1'))
										{flag = 1;fc=filebuffer[l*32]-'0';fs=filebuffer[l*32+1]-'0';}
										}
									}
									}
								}
							}
						}
					}
				}
			}
			else //change in current dir
			{
				//find
				for(int k = 0; k < 8; k++)//32 inodes per block
				{
					for(int m = 6; m < 30; m=m+2)//12 direct inodes
					{
						if(flag == 2) goto change;
						rc = data[k*32+m]-'0'; rs = data[k*32+m+1]-'0';//get c & s

						if(rc == 0 && rs == 0) goto change;
						else {
							bzero(filebuffer,1024);
							readblock(rc,rs,filebuffer);//get the exist file name
							char existname[33] = "";
							for(int l = 0; l < 8; l ++)
							//8 direct inode file per block, 32 bit for each file name
							{	for(int p = 3; p < 32; p++) existname[p-3] = filebuffer[l*32+p];
								existname[32] = '\0';
								if((strcmp(existname,filename)==0)&&(filebuffer[l*32+2]=='1'))
									{flag = 1; fc=filebuffer[l*32]-'0'; fs =filebuffer[l*32+1]-'0';}
							}
						}
					}
					if(flag != 1)//1 single indirect inode
					{
						rc = data[k*32+30]-'0'; rs = data[k*32+31]-'0';//get c & s
						if(rc == 0 && rs == 0) goto change;
						else {
							bzero(singlebuffer,1024);
							readblock(rc,rs,singlebuffer);

							for(int q = 0; q < 8; q++)//32 inodes per block
							{
							for(int m = 6; m < 30; m=m+2)//12 direct inodes
							{	rc = singlebuffer[q*32+m]-'0';
								rs = singlebuffer[q*32+m+1]-'0';
								if(rc == 0 && rs == 0) goto change;
								bzero(filebuffer,1024);
								readblock(rc,rs,filebuffer);//get the exist file name
								char existname[33] = "";
								for(int l = 0; l < 8; l ++)
								//8 direct inode file per block, 32 bit for each file name
								{	for(int p = 3; p < 32; p++) existname[p-3] = filebuffer[l*32+p];
									existname[32] = '\0';
									if((strcmp(existname,filename)==0)&&(filebuffer[l*32+2]=='1'))
									{flag =1;fc=filebuffer[l*32]-'0'; fs =filebuffer[l*32+1]-'0';}
								}
							}
							}
						}
					}
				}
			}
change: ;
			if(flag == 1)
			{
				cur_c = fc; cur_s = fs;
				count=0;
				strcpy(filename,"");
			}
			else
			{
				returnflag = 0;
				break;
			}
				}
			}
			cout << "current directionry: " << cur_c << " " << cur_s << endl;
			bzero(buffer, 1024);
			if(returnflag) strcpy(buffer,"Y");
			else strcpy(buffer,"N");
			send(client_sockfd, buffer,1024,0);


			cout << endl << "Changing directory finished"<< endl;
		 }
//rmdir
		else if (strcmp(cmd[0], "rmdir")==0)
		{
			cout << "Deleting directory..."<< endl;

			char init[30] = "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@";
			char ori[33] = "000@@@@@@@@@@@@@@@@@@@@@@@@@@@@@";
			int len = strlen(cmd[1]);
			strncat(cmd[1], init, 29-len);
			char name[33];
			for(int i = 0; i < 33; i++) name[i] = cmd[1][i];

			bzero(data,1024);
			readblock(cur_c, cur_s, data);
			int flag = 0, pos = 0, pos2 = 0, rc = 0, rs = 0, fc = 0, fs = 0, filec = 0, files = 0;
			if(data[4] == '0' && data[5] == '0')
			{//delete in root dir
				for(int i = 0; i <= inodelistc; i ++)
				{
					for(int j = 0; (i!=inodelistc&&j<SECTORS)||(i==inodelistc&&j<inodelists); j++)
					{
						if(i == 0 && j < 3) continue;
						if(flag == 2) goto deldir;
						bzero(data,1024);
						readblock(i, j, data);

						//find
						for(int k = 0; k < 8; k++)//32 inodes per block
						{
							for(int m = 6; m < 30; m=m+2)//12 direct inodes
							{
								if(flag == 2) goto deldir;
								rc = data[k*32+m]-'0'; rs = data[k*32+m+1]-'0';//get c & s
								if(rc == 0 && rs == 0) goto deldir;
								else {
									bzero(filebuffer,1024);
									readblock(rc,rs,filebuffer);//get the exist file name
									char existname[33] = "";
									for(int l = 0; l < 8; l ++)
									//8 direct inode file per block, 32 bit for each file name
									{
										for(int p=3; p < 32; p++) existname[p-3] = filebuffer[l*32+p];
										existname[32] = '\0';
										if((strcmp(existname, name)==0)&&(filebuffer[l*32+2]=='1')) 												{flag = 2; pos = l; pos2 = k; fc = i; fs = j;}
									}
								}
							}
							if(flag != 2)//1 single indirect inode
							{
								rc = data[k*32+30]-'0'; rs = data[k*32+31]-'0';//get c & s
								if(rc == 0 && rs == 0) goto deldir;
								else {
									bzero(singlebuffer,1024);
									readblock(rc,rs,singlebuffer);

									for(int q = 0; q < 8; q++)//32 inodes per block
									{
									for(int m = 6; m < 30; m=m+2)//12 direct inodes
									{	rc = singlebuffer[q*32+m]-'0';
										rs = singlebuffer[q*32+m+1]-'0';
										if(rc == 0 && rs == 0) goto deldir;
										bzero(filebuffer,1024);
										readblock(rc,rs,filebuffer);//get the exist file name
										char existname[33] = "";
										for(int l = 0; l < 8; l ++)
										//8 direct inode file per block, 32 bit for each file name
										{
											for(int p=3;p<32;p++)existname[p-3]=filebuffer[l*32+p];
											existname[32] = '\0';
											if((strcmp(existname, name)==0)&&(filebuffer[l*32+2]=='1'))
												{flag = 2; pos = l; pos2 = k;fc = i; fs = j;}
										}
									}
									}
								}
							}
						}
					}
				}
			}
			else //delete in current dir
			{
				//find
				for(int k = 0; k < 8; k++)//32 inodes per block
				{
					for(int m = 6; m < 30; m=m+2)//12 direct inodes
					{
						if(flag == 2) goto deldir;
						rc = data[k*32+m]-'0'; rs = data[k*32+m+1]-'0';//get c & s
						if(rc == 0 && rs == 0)  goto deldir;
						else {
							bzero(filebuffer,1024);
							readblock(rc,rs,filebuffer);//get the exist file name
							char existname[33] = "";
							for(int l = 0; l < 8; l ++)
							//8 direct inode file per block, 32 bit for each file name
							{	for(int p = 3; p < 32; p++) existname[p-3] = filebuffer[l*32+p];
								existname[32] = '\0';
								if((strcmp(existname, name)==0)&&(filebuffer[l*32+2]=='1'))
									{flag = 2; pos = l; pos2 = k;fc = cur_c; fs = cur_s;}
							}
						}
					}
					if(flag != 2)//1 single indirect inode
					{
						rc = data[k*32+30]-'0'; rs = data[k*32+31]-'0';//get c & s
						if(rc == 0 && rs == 0) goto deldir;
						else {
							bzero(singlebuffer,1024);
							readblock(rc,rs,singlebuffer);

							for(int q = 0; q < 8; q++)//32 inodes per block
							{
							for(int m = 6; m < 30; m=m+2)//12 direct inodes
							{	rc = singlebuffer[q*32+m]-'0';
								rs = singlebuffer[q*32+m+1]-'0';
								if(rc == 0 && rs == 0) goto deldir;
								bzero(filebuffer,1024);
								readblock(rc,rs,filebuffer);//get the exist file name
								char existname[33] = "";
								for(int l = 0; l < 8; l ++)
								//8 direct inode file per block, 32 bit for each file name
								{	for(int p = 3; p < 32; p++) existname[p-3] = filebuffer[l*32+p];
									existname[32] = '\0';
									if((strcmp(existname, name)==0)&&(filebuffer[l*32+2]=='1'))
										{flag = 2;pos = l; pos2 = k;fc = cur_c; fs = cur_s;}
								}
							}
							}
						}
					}
				}
			}
			deldir: ;
			if(flag == 2)
			{
				n = send(client_sockfd,"0",2,0);//exist and delete
				for(int i = pos*32; i < (pos+1)*32; i++) filebuffer[i] = ori[i-pos*32];
				writeblock(rc, rs, filebuffer);

				//update inode list
				bzero(data, 1024);
				readblock(fc, fs, data);
				data[pos2*32+3] = (data[pos2*32+1]-'0')*100+(data[pos2*32+2]-'0')*10+data[pos2*32+3] - 32;
				data[pos2*32+1] = '0'; data[pos2*32+2] = '0';
				if(data[pos2*32+3] > '9')
					{data[pos2*32+2] += (data[pos2*32+3]-'0')/10; data[pos2*32+3] = (data[pos2*32+3]-'0')%10+'0'; }
				if(data[pos2*32+2] > '9')
					{data[pos2*32+1] += (data[pos2*32+2]-'0')/10; data[pos2*32+2] = (data[pos2*32+2]-'0')%10+'0'; }
				writeblock(fc, fs, data);

			}
			else if(flag == 0) n = send(client_sockfd,"1",2,0);//not exist
			else n = send(client_sockfd,"2",2,0);//failed

			cout << endl << "Deleting directory finished"<< endl;
		 }
//ls
		 else if(strcmp(cmd[0],"ls")==0)
		 {
			cout << "Listing directory... "<< endl;
			int fg = 0;
			if(strcmp(cmd[1],"1")==0) fg = 1;
			bzero(data,1024);
			readblock(cur_c, cur_s, data);
			int rc = 0, rs = 0, count = 0;
			char filename[1000][1024] = {0};
			for(int i = 0; i < 8; i++)//32 inodes per block
			{
				for(int j = 6; j < 30; j=j+2)//12 direct inodes
				{
					rc = data[i*32+j]-'0'; rs = data[i*32+j+1]-'0';//get c & s
					if(rc==0 && rs==0) goto gen;
					else {
						bzero(filename[count],1024);
						readblock(rc,rs,filename[count++]);
					}
				}
				rc = data[i*32+30]-'0'; rs = data[i*32+31]-'0';//get c & s
				if(rc == 0 && rs == 0) goto gen;
				else {
					bzero(singlebuffer,1024);
					readblock(rc,rs,singlebuffer);
					for(int q = 0; q < 8; q++)//32 inodes per block
					{
					for(int m = 6; m < 30; m=m+2)//12 direct inodes
					{	rc = singlebuffer[q*32+m]-'0';
						rs = singlebuffer[q*32+m+1]-'0';
						if(rc == 0 && rs == 0) goto gen;
						bzero(filename[count],1024);
						readblock(rc,rs,filename[count++]);
					}
					}
				}
			}

gen:;
			bzero(buffer,1024);
			strcpy(buffer,"");
//ls 0
			if(!fg)
			{
				for(int i=0;i<count;i++)
				{
					for(int j=0;j<8;j++)
					{
						if(filename[i][j*32+3]=='@' || filename[i][j*32+3]=='\0')continue;
						else
						{	char tempstr[30]={0};
							memcpy(tempstr,&filename[i][j*32+3],29*sizeof(char));
							for(int k=0;k<29;k++)  tempstr[k] = (tempstr[k]=='@') ?0:tempstr[k];
							strcat(buffer,tempstr);
							strcat(buffer,"\n");
						}
					}
				}
			}
//ls 1
			else
			{
				for(int i=0;i<count;i++)
				{
					for(int j=0;j<8;j++)
					{
						if(filename[i][j*32+3]=='@' || filename[i][j*32+3]=='\0')continue;
						else{
							char tempstr[30]={0};
							int filesize = 0;
							char fsize[32] = {0};
							memcpy(tempstr,&filename[i][j*32+3],29*sizeof(char));
							for(int k=0;k<29;k++)  tempstr[k] = (tempstr[k]=='@') ?0:tempstr[k];
							strcat(buffer,tempstr);
							strcat(buffer," ");
							if(filename[i][j*32+2]=='1') {strcat(buffer,"\n");continue;}
							bzero(data,1024);
							readblock(filename[i][j*32]-'0', filename[i][j*32+1]-'0', data);
							for(int i = 0; i < 8; i++)
							{filesize+=((data[i*32+1]-'0')*100+(data[i*32+2]-'0')*10+(data[i*32+3]-'0'));
							filesize+=((data[i*32]-'0')*256);}
							sprintf(fsize,"length: (%d)",filesize);
							strcat(buffer,fsize);
							strcat(buffer,"\n");
						}
					}
				}
			}
			n = send(client_sockfd,buffer,1024,0);
			cout << endl << "Listing directory finished"<< endl;
		}
//cat
		else if(strcmp(cmd[0],"cat")==0)
		{
			cout << "Catching file..." << endl;

			//search for file
			char name[33]={0};
			strcpy(name,cmd[1]);
			for(int i=0;i<29;++i) name[i] = (name[i]==0)?'@':name[i];

			bzero(data,1024);
			readblock(cur_c, cur_s, data);
			int flag = 0, rc = 0, rs = 0, fc = 0, fs = 0, count = 0, filesize = 0;
			char filename[1000][1024] = {0};
			if(data[4] == '0' && data[5] == '0')
			{//catch in root dir
				for(int i = 0; i <= inodelistc; i ++)
				{
					for(int j = 0; (i!=inodelistc&&j<SECTORS)||(i==inodelistc&&j<inodelists); j++)
					{
						if(i == 0 && j < 3) continue;
						if(flag == 2) goto catchf;
						bzero(data,1024);
						readblock(i, j, data);

						//find
						for(int k = 0; k < 8; k++)//32 inodes per block
						{
							for(int m = 6; m < 30; m=m+2)//12 direct inodes
							{
								if(flag == 2) goto catchf;
								rc = data[k*32+m]-'0'; rs = data[k*32+m+1]-'0';//get c & s
								if(rc == 0 && rs == 0) goto catchf;
								else {
									bzero(filebuffer,1024);
									readblock(rc,rs,filebuffer);//get the exist file name
									char existname[33] = "";
									for(int l = 0; l < 8; l ++)
									//8 direct inode file per block, 32 bit for each file name
									{
										for(int p=3; p < 32; p++) existname[p-3] = filebuffer[l*32+p];
										existname[32] = '\0';
										if((strcmp(existname, name)==0)&&(filebuffer[l*32+2]=='0')) 										{flag = 2; fc = filebuffer[l*32]-'0'; fs = filebuffer[l*32+1]-'0';}
									}
								}
							}
							if(flag != 2)//1 single indirect inode
							{
								rc = data[k*32+30]-'0'; rs = data[k*32+31]-'0';//get c & s
								if(rc == 0 && rs == 0) goto catchf;
								else {
									bzero(singlebuffer,1024);
									readblock(rc,rs,singlebuffer);

									for(int q = 0; q < 8; q++)//32 inodes per block
									{
									for(int m = 6; m < 30; m=m+2)//12 direct inodes
									{	rc = singlebuffer[q*32+m]-'0';
										rs = singlebuffer[q*32+m+1]-'0';
										if(rc == 0 && rs == 0)  goto catchf;
										bzero(filebuffer,1024);
										readblock(rc,rs,filebuffer);//get the exist file name
										char existname[33] = "";
										for(int l = 0; l < 8; l ++)
										//8 direct inode file per block, 32 bit for each file name
										{	for(int p = 3; p < 32; p++) existname[p-3] = filebuffer[l*32+p];
											existname[32] = '\0';
											if((strcmp(existname, name)==0)&&(filebuffer[l*32+2]=='0'))
									{flag = 2; fc = filebuffer[l*32]-'0'; fs = filebuffer[l*32+1]-'0';}
										}
									}
									}
								}
							}
						}
					}
				}
			}
			else //catch in current dir
			{
				//find
				for(int k = 0; k < 8; k++)//32 inodes per block
				{
					for(int m = 6; m < 30; m=m+2)//12 direct inodes
					{
						if(flag == 2) goto catchf;
						rc = data[k*32+m]-'0'; rs = data[k*32+m+1]-'0';//get c & s
						if(rc == 0 && rs == 0) goto catchf;
						else {
							bzero(filebuffer,1024);
							readblock(rc,rs,filebuffer);//get the exist file name
							char existname[33] = "";
							for(int l = 0; l < 8; l ++)
							//8 direct inode file per block, 32 bit for each file name
							{	for(int p = 3; p < 32; p++) existname[p-3] = filebuffer[l*32+p];
								existname[32] = '\0';
								if((strcmp(existname, name)==0)&&(filebuffer[l*32+2]=='1'))
									{flag = 2; fc = filebuffer[l*32]-'0'; fs = filebuffer[l*32+1]-'0';}
							}
						}
					}
					if(flag != 2)//1 single indirect inode
					{
						rc = data[k*32+30]-'0'; rs = data[k*32+31]-'0';//get c & s
						if(rc == 0 && rs == 0) goto catchf;
						else {
							bzero(singlebuffer,1024);
							readblock(rc,rs,singlebuffer);

							for(int q = 0; q < 8; q++)//32 inodes per block
							{
							for(int m = 6; m < 30; m=m+2)//12 direct inodes
							{	rc = singlebuffer[q*32+m]-'0';
								rs = singlebuffer[q*32+m+1]-'0';
								if(rc == 0 && rs == 0)   goto catchf;
								bzero(filebuffer,1024);
								readblock(rc,rs,filebuffer);//get the exist file name
								char existname[33] = "";
								for(int l = 0; l < 8; l ++)
								//8 direct inode file per block, 32 bit for each file name
								{	for(int p = 3; p < 32; p++) existname[p-3] = filebuffer[l*32+p];
									existname[32] = '\0';
									if((strcmp(existname, name)==0)&&(filebuffer[l*32+2]=='1'))
									{flag = 2; fc = filebuffer[l*32]-'0'; fs = filebuffer[l*32+1]-'0';}
								}
							}
							}
						}
					}
				}
			}
catchf: ;
			if(flag == 0)
			{
				sprintf(buffer,"1");
			}
			else if(flag == 2)
			{
				bzero(data, 1024);
				readblock(fc, fs, data);

				for(int i = 0; i < 8; i++)//32 inodes per block
				{
					filesize += ((data[i*32+1]-'0')*100+(data[i*32+2]-'0')*10+(data[i*32+3]-'0'));
					for(int j = 6; j < 30; j=j+2)//12 direct inodes
					{
						rc = data[i*32+j]-'0'; rs = data[i*32+j+1]-'0';//get c & s
						if(rc==0 && rs==0) goto res;
						else {
							bzero(filename[count],1024);
							readblock(rc,rs,filename[count++]);
						}
					}
					rc = data[i*32+30]-'0'; rs = data[i*32+31]-'0';//get c & s
					if(rc == 0 && rs == 0) goto res;
					else {
						bzero(singlebuffer,1024);
						readblock(rc,rs,singlebuffer);
						for(int q = 0; q < 8; q++)//32 inodes per block
						{
						for(int m = 6; m < 30; m=m+2)//12 direct inodes
						{	rc = singlebuffer[q*32+m]-'0';
							rs = singlebuffer[q*32+m+1]-'0';
							if(rc == 0 && rs == 0)  goto res;
							bzero(filename[count],1024);
							readblock(rc,rs,filename[count++]);
						}
						}
					}
				}
res: ;
				sprintf(buffer,"0 %d ", filesize);

				for(int i=0;i<count;++i)
				{
					strcat(buffer,filename[i]);
				}
			}
			else sprintf(buffer,"2");
			n = send(client_sockfd,buffer,1024,0);
			cout << endl << "Catching file finished"<< endl;
		}
// w
		else if(strcmp(cmd[0],"w")==0)
		{
			cout << "Writing file finished"<< endl;
			//search for file
			char name[33]={0};
			strcpy(name,cmd[1]);
			for(int i=0;i<29;++i) name[i] = (name[i]==0)?'@':name[i];

			bzero(data,1024);
			readblock(cur_c, cur_s, data);
			int flag = 0, rc = 0, rs = 0, fc = 0, fs = 0, count = 0, filesize = 0;
			char filename[1000][1024] = {0};
			if(data[4] == '0' && data[5] == '0')
			{//catch in root dir
				for(int i = 0; i <= inodelistc; i ++)
				{
					for(int j = 0; (i!=inodelistc&&j<SECTORS)||(i==inodelistc&&j<inodelists); j++)
					{
						if(i == 0 && j < 3) continue;
						if(flag == 2) goto catchf1;
						bzero(data,1024);
						readblock(i, j, data);

						//find
						for(int k = 0; k < 8; k++)//32 inodes per block
						{
							for(int m = 6; m < 30; m=m+2)//12 direct inodes
							{
								if(flag == 2) goto catchf1;
								rc = data[k*32+m]-'0'; rs = data[k*32+m+1]-'0';//get c & s
								if(rc == 0 && rs == 0)  goto catchf1;
								else {
									bzero(filebuffer,1024);
									readblock(rc,rs,filebuffer);//get the exist file name
									char existname[33] = "";
									for(int l = 0; l < 8; l ++)
									//8 direct inode file per block, 32 bit for each file name
									{	for(int p = 3; p < 32; p++) existname[p-3] = filebuffer[l*32+p];
										existname[32] = '\0';
										if((strcmp(existname, name)==0)&&(filebuffer[l*32+2]=='0')) 										{flag = 2; fc = filebuffer[l*32]-'0'; fs = filebuffer[l*32+1]-'0';}
									}
								}
							}
							if(flag != 2)//1 single indirect inode
							{
								rc = data[k*32+30]-'0'; rs = data[k*32+31]-'0';//get c & s
								if(rc == 0 && rs == 0) goto catchf1;
								else {
									bzero(singlebuffer,1024);
									readblock(rc,rs,singlebuffer);

									for(int q = 0; q < 8; q++)//32 inodes per block
									{
									for(int m = 6; m < 30; m=m+2)//12 direct inodes
									{	rc = singlebuffer[q*32+m]-'0';
										rs = singlebuffer[q*32+m+1]-'0';
										if(rc == 0 && rs == 0)  goto catchf1;
										bzero(filebuffer,1024);
										readblock(rc,rs,filebuffer);//get the exist file name
										char existname[33] = "";
										for(int l = 0; l < 8; l ++)
										//8 direct inode file per block, 32 bit for each file name
										{	for(int p=3;p<32;p++)existname[p-3]=filebuffer[l*32+p];
											existname[32] = '\0';
											if((strcmp(existname, name)==0)&&(filebuffer[l*32+2]=='0'))
									{flag = 2; fc = filebuffer[l*32]-'0'; fs = filebuffer[l*32+1]-'0';}
										}
									}
									}
								}
							}
						}
					}
				}
			}
			else //catch in current dir
			{
				//find
				for(int k = 0; k < 8; k++)//32 inodes per block
				{
					for(int m = 6; m < 30; m=m+2)//12 direct inodes
					{
						if(flag == 2) goto catchf1;
						rc = data[k*32+m]-'0'; rs = data[k*32+m+1]-'0';//get c & s
						if(rc == 0 && rs == 0) goto catchf1;
						else {
							bzero(filebuffer,1024);
							readblock(rc,rs,filebuffer);//get the exist file name
							char existname[33] = "";
							for(int l = 0; l < 8; l ++)
							//8 direct inode file per block, 32 bit for each file name
							{	for(int p = 3; p < 32; p++) existname[p-3] = filebuffer[l*32+p];
								existname[32] = '\0';
								if((strcmp(existname, name)==0)&&(filebuffer[l*32+2]=='1'))
									{flag = 2; fc = filebuffer[l*32]-'0'; fs = filebuffer[l*32+1]-'0';}
							}
						}
					}
					if(flag != 2)//1 single indirect inode
					{
						rc = data[k*32+30]-'0'; rs = data[k*32+31]-'0';//get c & s
						if(rc == 0 && rs == 0) goto catchf1;
						else {
							bzero(singlebuffer,1024);
							readblock(rc,rs,singlebuffer);

							for(int q = 0; q < 8; q++)//32 inodes per block
							{
							for(int m = 6; m < 30; m=m+2)//12 direct inodes
							{	rc = singlebuffer[q*32+m]-'0';
								rs = singlebuffer[q*32+m+1]-'0';
								if(rc == 0 && rs == 0)  goto catchf1;
								bzero(filebuffer,1024);
								readblock(rc,rs,filebuffer);//get the exist file name
								char existname[33] = "";
								for(int l = 0; l < 8; l ++)
								//8 direct inode file per block, 32 bit for each file name
								{	for(int p = 3; p < 32; p++) existname[p-3] = filebuffer[l*32+p];
									existname[32] = '\0';
									if((strcmp(existname, name)==0)&&(filebuffer[l*32+2]=='1'))
									{flag = 2; fc = filebuffer[l*32]-'0'; fs = filebuffer[l*32+1]-'0';}
								}
							}
							}
						}
					}
				}
			}
catchf1: ;
			if(flag == 0)
			{
				sprintf(buffer,"1");
			}
			else if(flag == 2)
			{
				//find the file inode
				bzero(data, 1024);
				readblock(fc, fs, data);
				int total = atoi(cmd[2]);
				char filecontent[2048] = {0};
				for(int i = 0; i < total; i++) filecontent[i] = cmd[3][i];

				while(total>0)
				{
					//write file
					for(int i = 0; i < 8&&total>0; i++)//32 inodes per block
					{
						for(int j = 6; j < 30&&total>0; j=j+2)//12 direct inodes
						{
							rc = data[i*32+j]-'0'; rs = data[i*32+j+1]-'0';//get c & s
							if(rc==0 && rs==0) //create a new file block
							{
								//check the empty file block
								bzero(filedata,1024);
								readblock(0,2,filedata);
								int ep=0;
							for(int ct = CYLINDERS*SECTORS-1; ct >=0; ct--) if(filedata[ct]=='0') {ep=1; break;}
								if(ep==0) goto noempty1;

								//update tempc and temps
								temps++;
								cs_change(tempc, temps);
								int file_c = tempc, file_s = temps;

								data[i*32+j] = file_c+'0'; data[i*32+j+1] = file_s+'0';
								writeblock(fc, fs, data);

								//setup the file content block
								strcpy(data,"");
								for(int ii=0;ii<256;++ii) strcat(data,"0");
								writeblock(file_c, file_s, data);
								rc = file_c; rs = file_s;

								//update file block bitmap
								strcpy(data,"");
								for(int i=0;i<256;++i) strcat(data,"0");
								for(int i = 0; i <= tempc*SECTORS+temps; i++)	data[i] = '1';
								for(int i = tempc*SECTORS+temps+1; i < CYLINDERS*SECTORS; i++) data[i] = '0';
								for(int i = CYLINDERS*SECTORS; i < 256; i++) data[i] = '1';
								writeblock(0,2,data);
							}
							bzero(filebuffer,1024);
							readblock(rc,rs,filebuffer);
							if(total-256>0)
							{
								for(int k = 0; k < 256; k++) filebuffer[k] = filecontent[k];
								total-=256;
								for(int k = 0; k < total; k++) filecontent[k] = filecontent[k+256];
								//update inode list
								bzero(data, 1024);
								readblock(fc, fs, data);
								data[i*32]++;
								writeblock(fc, fs, data);
							}
							else
							{
								for(int k = 0; k < total; k++) filebuffer[k] = filecontent[k];
								//update inode list
								bzero(data, 1024);
								readblock(fc, fs, data);
								if(total>256-(data[i*32+3]-'0'))
								{data[i*32]++; data[i*32+3] = data[i*32+3]-'0'+(total-256);}
								else data[i*32+3] = data[i*32+3]-'0'+ total;
								data[i*32+2] -= '0';
								data[i*32+1] -= '0';
								if(data[i*32+3] > 9)
								{data[i*32+2] += (data[i*32+3])/10; data[i*32+3] = (data[i*32+3])%10; }
								if(data[i*32+2] > 9)
								{data[i*32+1] += (data[i*32+2])/10; data[i*32+2] = (data[i*32+2])%10;}

								data[i*32+3]+='0';
								data[i*32+2]+='0';
								data[i*32+1]+='0';
								writeblock(fc, fs, data);
								total=0;
							}
							writeblock(rc, rs, filebuffer);
						}
						if(total>0)
						{
							rc = data[i*32+30]-'0'; rs = data[i*32+31]-'0';//get c & s
							if(rc==0 && rs==0) //create a new file block
							{
								//check the empty file block
								bzero(filedata,1024);
								readblock(0,2,filedata);
								int ep=0;
							for(int ct = CYLINDERS*SECTORS-1; ct >=0; ct--) if(filedata[ct]=='0') {ep=1; break;}
								if(ep==0) goto noempty1;

								//update tempc and temps
								temps++;
								cs_change(tempc, temps);
								int file_c = tempc, file_s = temps;

								data[i*32+30] = file_c+'0'; data[i*32+31] = file_s+'0';
								writeblock(fc, fs, data);

								//setup the file content block
								strcpy(data,"");
								for(int ii=0;ii<256;++ii) strcat(data,"0");
								writeblock(file_c, file_s, data);
								rc = file_c; rs = file_s;

								//update file block bitmap
								strcpy(data,"");
								for(int i=0;i<256;++i) strcat(data,"0");
								for(int i = 0; i <= tempc*SECTORS+temps; i++)	data[i] = '1';
								for(int i = tempc*SECTORS+temps+1; i < CYLINDERS*SECTORS; i++) data[i] = '0';
								for(int i = CYLINDERS*SECTORS; i < 256; i++) data[i] = '1';
								writeblock(0,2,data);
							}
							bzero(singlebuffer,1024);
							readblock(rc,rs,singlebuffer);
							for(int q = 0; q < 8; q++)//32 inodes per block
							{
							for(int m = 6; m < 30&&total>0; m=m+2)//12 direct inodes
							{	rc = singlebuffer[q*32+m]-'0';
								rs = singlebuffer[q*32+m+1]-'0';
								if(rc == 0 && rs == 0) //create a new file block
								{
								//check the empty file block
								bzero(filedata,1024);
								readblock(0,2,filedata);
								int ep=0;
								for(int ct = CYLINDERS*SECTORS-1; ct >=0; ct--)
									if(filedata[ct]=='0') {ep=1; break;}
								if(ep==0) goto noempty1;

									//update tempc and temps
									temps++;
									cs_change(tempc, temps);
									int file_c = tempc, file_s = temps;

									data[q*32+m] = file_c+'0'; data[q*32+m+1] = file_s+'0';
									writeblock(fc, fs, data);

									//setup the file content block
									strcpy(data,"");
									for(int ii=0;ii<256;++ii) strcat(data,"0");
									writeblock(file_c, file_s, data);
									rc = file_c; rs = file_s;

								//update file block bitmap
								strcpy(data,"");
								for(int i=0;i<256;++i) strcat(data,"0");
								for(int i = 0; i <= tempc*SECTORS+temps; i++)	data[i] = '1';
								for(int i = tempc*SECTORS+temps+1; i < CYLINDERS*SECTORS; i++) data[i] = '0';
								for(int i = CYLINDERS*SECTORS; i < 256; i++) data[i] = '1';
								writeblock(0,2,data);
								}
								bzero(filebuffer,1024);
								readblock(rc,rs,filebuffer);
								if(total-256>0)
								{
									for(int k = 0; k < 256; k++) filebuffer[k] = filecontent[k];
									total-=256;
									for(int k = 0; k < total; k++) filecontent[k] = filecontent[k+256];
									//update inode list
									bzero(data, 1024);
									readblock(fc, fs, data);
									data[i*32]++;
									writeblock(fc, fs, data);
								}
								else
								{
									for(int k = 0; k < total; k++) filebuffer[k] = filecontent[k];
								//update inode list
								bzero(data, 1024);
								readblock(fc, fs, data);
								if(total>256-(data[i*32+3]-'0'))
								{data[i*32]++;data[i*32+3] = data[i*32+3]-'0'+(total-256);}
								else data[i*32+3] = data[i*32+3]-'0'+ total;
								data[i*32+2] -= '0';
								data[i*32+1] -= '0';
								if(data[i*32+3] > 9)
								{data[i*32+2] += (data[i*32+3])/10; data[i*32+3] = (data[i*32+3])%10; }
								if(data[i*32+2] > 9)
								{data[i*32+1] += (data[i*32+2])/10; data[i*32+2] = (data[i*32+2])%10;}

								data[i*32+3]+='0';
								data[i*32+2]+='0';
								data[i*32+1]+='0';
								writeblock(fc, fs, data);
									total=0;
								}
								writeblock(rc, rs, filebuffer);
							}
							}
						}
					}
				}
				sprintf(buffer,"0");
			}
			else
		noempty1: 	sprintf(buffer,"2");
			n = send(client_sockfd,buffer,1024,0);
			cout << endl << "Writing file finished"<< endl;
		}
// a
		else if(strcmp(cmd[0],"a")==0)
		{
			cout << "Appending to file finished"<< endl;
			//search for file
			char name[33]={0};
			strcpy(name,cmd[1]);
			for(int i=0;i<29;++i) name[i] = (name[i]==0)?'@':name[i];

			bzero(data,1024);
			readblock(cur_c, cur_s, data);
			int flag = 0, rc = 0, rs = 0, fc = 0, fs = 0, count = 0, filesize = 0;
			char filename[1000][1024] = {0};
			if(data[4] == '0' && data[5] == '0')
			{//catch in root dir
				for(int i = 0; i <= inodelistc; i ++)
				{
					for(int j = 0; (i!=inodelistc&&j<SECTORS)||(i==inodelistc&&j<inodelists); j++)
					{
						if(i == 0 && j < 3) continue;
						if(flag == 2) goto catcha;
						bzero(data,1024);
						readblock(i, j, data);

						//find
						for(int k = 0; k < 8; k++)//32 inodes per block
						{
							for(int m = 6; m < 30; m=m+2)//12 direct inodes
							{
								if(flag == 2) goto catcha;
								rc = data[k*32+m]-'0'; rs = data[k*32+m+1]-'0';//get c & s
								if(rc == 0 && rs == 0)	goto catcha;
								else {
									bzero(filebuffer,1024);
									readblock(rc,rs,filebuffer);//get the exist file name
									char existname[33] = "";
									for(int l = 0; l < 8; l ++)
									//8 direct inode file per block, 32 bit for each file name
									{	for(int p = 3; p < 32; p++) existname[p-3] = filebuffer[l*32+p];
										existname[32] = '\0';
										if((strcmp(existname, name)==0)&&(filebuffer[l*32+2]=='0')) 										{flag = 2; fc = filebuffer[l*32]-'0'; fs = filebuffer[l*32+1]-'0';}
									}
								}
							}
							if(flag != 2)//1 single indirect inode
							{
								rc = data[k*32+30]-'0'; rs = data[k*32+31]-'0';//get c & s
								if(rc == 0 && rs == 0) goto catcha;
								else {
									bzero(singlebuffer,1024);
									readblock(rc,rs,singlebuffer);

									for(int q = 0; q < 8; q++)//32 inodes per block
									{
									for(int m = 6; m < 30; m=m+2)//12 direct inodes
									{	rc = singlebuffer[q*32+m]-'0';
										rs = singlebuffer[q*32+m+1]-'0';
										if(rc == 0 && rs == 0)   goto catcha;
										bzero(filebuffer,1024);
										readblock(rc,rs,filebuffer);//get the exist file name
										char existname[33] = "";
										for(int l = 0; l < 8; l ++)
										//8 direct inode file per block, 32 bit for each file name
										{	for(int p = 3; p < 32; p++) existname[p-3] = filebuffer[l*32+p];
											existname[32] = '\0';
											if((strcmp(existname, name)==0)&&(filebuffer[l*32+2]=='0'))
									{flag = 2; fc = filebuffer[l*32]-'0'; fs = filebuffer[l*32+1]-'0';}
										}
									}
									}
								}
							}
						}
					}
				}
			}
			else //catch in current dir
			{
				//find
				for(int k = 0; k < 8; k++)//32 inodes per block
				{
					for(int m = 6; m < 30; m=m+2)//12 direct inodes
					{
						if(flag == 2) goto catcha;
						rc = data[k*32+m]-'0'; rs = data[k*32+m+1]-'0';//get c & s
						if(rc == 0 && rs == 0)
							goto catcha;
						else {
							bzero(filebuffer,1024);
							readblock(rc,rs,filebuffer);//get the exist file name
							char existname[33] = "";
							for(int l = 0; l < 8; l ++)
							//8 direct inode file per block, 32 bit for each file name
							{	for(int p = 3; p < 32; p++) existname[p-3] = filebuffer[l*32+p];
								existname[32] = '\0';
								if((strcmp(existname, name)==0)&&(filebuffer[l*32+2]=='1'))
									{flag = 2; fc = filebuffer[l*32]-'0'; fs = filebuffer[l*32+1]-'0';}
							}
						}
					}
					if(flag != 2)//1 single indirect inode
					{
						rc = data[k*32+30]-'0'; rs = data[k*32+31]-'0';//get c & s
						if(rc == 0 && rs == 0) goto catcha;
						else {
							bzero(singlebuffer,1024);
							readblock(rc,rs,singlebuffer);

							for(int q = 0; q < 8; q++)//32 inodes per block
							{
							for(int m = 6; m < 30; m=m+2)//12 direct inodes
							{	rc = singlebuffer[q*32+m]-'0';
								rs = singlebuffer[q*32+m+1]-'0';
								if(rc == 0 && rs == 0)   goto catcha;
								bzero(filebuffer,1024);
								readblock(rc,rs,filebuffer);//get the exist file name
								char existname[33] = "";
								for(int l = 0; l < 8; l ++)
								//8 direct inode file per block, 32 bit for each file name
								{	for(int p = 3; p < 32; p++) existname[p-3] = filebuffer[l*32+p];
									existname[32] = '\0';
									if((strcmp(existname, name)==0)&&(filebuffer[l*32+2]=='1'))
									{flag = 2; fc = filebuffer[l*32]-'0'; fs = filebuffer[l*32+1]-'0';}
								}
							}
							}
						}
					}
				}
			}
catcha: ;
			if(flag == 0)
			{
				sprintf(buffer,"1");
			}
			else if(flag == 2)
			{
				//find the file inode
				bzero(data, 1024);
				readblock(fc, fs, data);
				int total = atoi(cmd[2]);
				char filecontent[2048] = {0};
				for(int i = 0; i < total; i++) filecontent[i] = cmd[3][i];

				int pos_c = 0, pos_s = 0, posc1 = 0, poss1 = 0, posc2 = 0, poss2 = 0, fsize = 0;
				int posf_c = 0, posf_s = 0, posfs1 = 0, posfs2 = 0, posfs = 0, posfc = 0;
				//get the current position
				for(int i = 0; i < 8; i++)
				{	if((data[i*32+1]-'0')*100+(data[i*32+2]-'0')*10+(data[i*32+3]-'0')==0&&(data[i*32]-'0')==0)
						{posfc=i; posfs=posfs2=6;break;}
					fsize = (data[i*32+1]-'0')*100+(data[i*32+2]-'0')*10+(data[i*32+3]-'0');
					for(int j = 26; j >=6; j=j-2)//12 direct inodes
					{
						rc = data[i*32+j]-'0'; rs = data[i*32+j+1]-'0';//get c & s
						if(rc==0 && rs==0) continue;
						posc1 = rc; poss1 = rs; posfs1 = j; posfc = i;
						posc2 = data[i*32+j+2]-'0'; rs = data[i*32+j+3]-'0';posfs2 = j+2;
						break;
					}
					rc = data[i*32+30]-'0'; rs = data[i*32+31]-'0';//get c & s
					if(rc==0 && rs==0) continue;

					bzero(singlebuffer,1024);
					readblock(rc,rs,singlebuffer);
					for(int q = 0; q < 8; q++)//32 inodes per block
					{
					for(int m = 6; m < 28; m=m+2)//12 direct inodes
					{	rc = singlebuffer[q*32+m]-'0';
						rs = singlebuffer[q*32+m+1]-'0';
						if(rc == 0 && rs == 0) continue;
						posc1 = rc; poss1 = rs;posfs1 = m;posfc = i;
						posc2 = data[q*32+m+2]-'0'; rs = data[q*32+m+3]-'0';posfs1 = m+2;posfc = i;
						break;
					}
                    			}
                		}
				if(fsize!=0){pos_c = posc1; pos_s = poss1;posfs = posfs1;}
				else {pos_c = posc2; pos_s = poss2;posfs = posfs2;}
cout << posfs << endl;
				while(total>0)
				{
					//add file
					for(int i = posfc; i < 8&&total>0; i++)//32 inodes per block
					{
						for(int j = posfs; j < 30&&total>0; j=j+2)//12 direct inodes
						{
							rc = data[i*32+j]-'0'; rs = data[i*32+j+1]-'0';//get c & s
							if(rc==0 && rs==0) //create a new file block
							{
								//check the empty file block
								bzero(filedata,1024);
								readblock(0,2,filedata);
								int ep=0;
							for(int ct = CYLINDERS*SECTORS-1; ct >=0; ct--) if(filedata[ct]=='0') {ep=1; break;}
								if(ep==0) goto noempty2;

								//update tempc and temps
								temps++;
								cs_change(tempc, temps);
								int file_c = tempc, file_s = temps;

								data[i*32+j] = file_c+'0'; data[i*32+j+1] = file_s+'0';
								writeblock(fc, fs, data);

								//setup the file content block
								strcpy(data,"");
								for(int ii=0;ii<256;++ii) strcat(data,"0");
								writeblock(file_c, file_s, data);
								rc = file_c; rs = file_s;

								//update file block bitmap
								strcpy(data,"");
								for(int i=0;i<256;++i) strcat(data,"0");
								for(int i = 0; i <= tempc*SECTORS+temps; i++)	data[i] = '1';
								for(int i = tempc*SECTORS+temps+1; i < CYLINDERS*SECTORS; i++) data[i] = '0';
								for(int i = CYLINDERS*SECTORS; i < 256; i++) data[i] = '1';
								writeblock(0,2,data);
							}
							bzero(filebuffer,1024);
							readblock(rc,rs,filebuffer);
							if(i == posfc && j == posfs)
							{	if(total-(256-fsize)>0)
								{
								for(int kk = 0; fsize+kk < 256; kk++) filebuffer[kk+fsize] = filecontent[kk];
									total-=(256-fsize);
								for(int kk = 0; kk < total; kk++) filecontent[kk]=filecontent[kk+(256-fsize)];
									writeblock(rc, rs, filebuffer);
									//update inode list
									bzero(data, 1024);
									readblock(fc, fs, data);
									data[i*32]++;
									data[i*32+1]='0';
									data[i*32+2]='0';
									data[i*32+3]='0';

									writeblock(fc, fs, data);
								}
								else
								{
									for(int k = 0; k < total; k++) filebuffer[k+fsize] = filecontent[k];
									writeblock(rc, rs, filebuffer);
									//update inode list
									bzero(data, 1024);
									readblock(fc, fs, data);
									if(total>256-(data[i*32+3]-'0'))
									{data[i*32]++;data[i*32+3] = data[i*32+3]-'0'+(total-256);}
									else data[i*32+3] = data[i*32+3]-'0'+ total;
									data[i*32+2] -= '0';
									data[i*32+1] -= '0';
									if(data[i*32+3] > 9)
									{data[i*32+2] += (data[i*32+3])/10; data[i*32+3] = (data[i*32+3])%10;}
									if(data[i*32+2] > 9)
									{data[i*32+1] += (data[i*32+2])/10; data[i*32+2] = (data[i*32+2])%10;}

									data[i*32+3]+='0';
									data[i*32+2]+='0';
									data[i*32+1]+='0';
									writeblock(fc, fs, data);
									total=0;
								}
							}
							else if(total-256>0)
							{
								for(int k = 0; k < 256; k++) filebuffer[k] = filecontent[k];
								total-=256;
								for(int k = 0; k < total; k++) filecontent[k] = filecontent[k+256];
								writeblock(rc, rs, filebuffer);
								//update inode list
								bzero(data, 1024);
								readblock(fc, fs, data);
								data[i*32]++;
								writeblock(fc, fs, data);
							}
							else
							{
								for(int k = 0; k < total; k++) filebuffer[k] = filecontent[k];
								writeblock(rc, rs, filebuffer);
								//update inode list
								bzero(data, 1024);
								readblock(fc, fs, data);
								if(total>256-(data[i*32+3]-'0'))
								{data[i*32]++;data[i*32+3] = data[i*32+3]-'0'+(total-256);}
								else data[i*32+3] = data[i*32+3]-'0'+ total;
								data[i*32+2] -= '0';
								data[i*32+1] -= '0';
								if(data[i*32+3] > 9)
								{data[i*32+2] += (data[i*32+3])/10; data[i*32+3] = (data[i*32+3])%10; }
								if(data[i*32+2] > 9)
								{data[i*32+1] += (data[i*32+2])/10; data[i*32+2] = (data[i*32+2])%10;}

								data[i*32+3]+='0';
								data[i*32+2]+='0';
								data[i*32+1]+='0';
								writeblock(fc, fs, data);
								total=0;
							}
						}
						if(total>0)
						{
							rc = data[i*32+30]-'0'; rs = data[i*32+31]-'0';//get c & s
							if(rc==0 && rs==0) //create a new file block
							{
								//check the empty file block
								bzero(filedata,1024);
								readblock(0,2,filedata);
								int ep=0;
							for(int ct = CYLINDERS*SECTORS-1; ct >=0; ct--) if(filedata[ct]=='0') {ep=1; break;}
								if(ep==0) goto noempty2;

								//update tempc and temps
								temps++;
								cs_change(tempc, temps);
								int file_c = tempc, file_s = temps;

								data[i*32+30] = file_c+'0'; data[i*32+31] = file_s+'0';
								writeblock(fc, fs, data);

								//setup the file content block
								strcpy(data,"");
								for(int ii=0;ii<256;++ii) strcat(data,"0");
								writeblock(file_c, file_s, data);
								rc = file_c; rs = file_s;

								//update file block bitmap
								strcpy(data,"");
								for(int i=0;i<256;++i) strcat(data,"0");
								for(int i = 0; i <= tempc*SECTORS+temps; i++)	data[i] = '1';
								for(int i = tempc*SECTORS+temps+1; i < CYLINDERS*SECTORS; i++) data[i] = '0';
								for(int i = CYLINDERS*SECTORS; i < 256; i++) data[i] = '1';
								writeblock(0,2,data);
							}
							bzero(singlebuffer,1024);
							readblock(rc,rs,singlebuffer);
							for(int q = 0; q < 8; q++)//32 inodes per block
							{
							for(int m = 6; m < 30&&total>0; m=m+2)//12 direct inodes
							{	rc = singlebuffer[q*32+m]-'0';
								rs = singlebuffer[q*32+m+1]-'0';
								if(rc == 0 && rs == 0) //create a new file block
								{
								//check the empty file block
									bzero(filedata,1024);
									readblock(0,2,filedata);
									int ep=0;
									for(int ct = CYLINDERS*SECTORS-1; ct >=0; ct--) 										if(filedata[ct]=='0') {ep=1; break;}
									if(ep==0) goto noempty2;

									//update tempc and temps
									temps++;
									cs_change(tempc, temps);
									int file_c = tempc, file_s = temps;

									data[q*32+m] = file_c+'0'; data[q*32+m+1] = file_s+'0';
									writeblock(fc, fs, data);

									//setup the file content block
									strcpy(data,"");
									for(int ii=0;ii<256;++ii) strcat(data,"0");
									writeblock(file_c, file_s, data);
									rc = file_c; rs = file_s;

									//update file block bitmap
									strcpy(data,"");
									for(int i=0;i<256;++i) strcat(data,"0");
									for(int i = 0; i <= tempc*SECTORS+temps; i++)	data[i] = '1';
									for(int i=tempc*SECTORS+temps+1;i<CYLINDERS*SECTORS;i++)data[i] = '0';
									for(int i = CYLINDERS*SECTORS; i < 256; i++) data[i] = '1';
									writeblock(0,2,data);
								}
								bzero(filebuffer,1024);
								readblock(rc,rs,filebuffer);
							if(i == posfc && m == posfs)
							{	if(total-(256-fsize)>0)
								{
								for(int k = 0; fsize+k < 256; k++) filebuffer[k+fsize] = filecontent[k];
									total-=(256-fsize);
								for(int k = 0; k < total; k++) filecontent[k] = filecontent[k+(256-fsize)];
									writeblock(rc, rs, filebuffer);
									//update inode list
									bzero(data, 1024);
									readblock(fc, fs, data);
									data[i*32]++;
									writeblock(fc, fs, data);
								}
								else
								{
									for(int k = 0; k < total; k++) filebuffer[k+fsize] = filecontent[k];
									writeblock(rc, rs, filebuffer);
									//update inode list
									bzero(data, 1024);
									readblock(fc, fs, data);
									if(total>256-(data[i*32+3]-'0'))
									{data[i*32]++;data[i*32+3] = data[i*32+3]-'0'+(total-256);}
									else data[i*32+3] = data[i*32+3]-'0'+ total;
									data[i*32+2] -= '0';
									data[i*32+1] -= '0';
									if(data[i*32+3] > 9)
									{data[i*32+2] += (data[i*32+3])/10; data[i*32+3] = (data[i*32+3])%10;}
									if(data[i*32+2] > 9)
									{data[i*32+1] += (data[i*32+2])/10; data[i*32+2] = (data[i*32+2])%10;}

									data[i*32+3]+='0';
									data[i*32+2]+='0';
									data[i*32+1]+='0';
									writeblock(fc, fs, data);
									total=0;
								}
							}
							else if(total-256>0)
							{
								for(int k = 0; k < 256; k++) filebuffer[k] = filecontent[k];
								total-=256;
								for(int k = 0; k < total; k++) filecontent[k] = filecontent[k+256];
								writeblock(rc, rs, filebuffer);
								//update inode list
								bzero(data, 1024);
								readblock(fc, fs, data);
								data[i*32]++;
								writeblock(fc, fs, data);
							}
							else
							{
								for(int k = 0; k < total; k++) filebuffer[k] = filecontent[k];
								writeblock(rc, rs, filebuffer);
								//update inode list
								bzero(data, 1024);
								readblock(fc, fs, data);
								if(total>256-(data[i*32+3]-'0'))
								{data[i*32]++;data[i*32+3] = data[i*32+3]-'0'+(total-256);}
								else data[i*32+3] = data[i*32+3]-'0'+ total;
								data[i*32+2] -= '0';
								data[i*32+1] -= '0';
								if(data[i*32+3] > 9)
								{data[i*32+2] += (data[i*32+3])/10; data[i*32+3] = (data[i*32+3])%10; }
								if(data[i*32+2] > 9)
								{data[i*32+1] += (data[i*32+2])/10; data[i*32+2] = (data[i*32+2])%10;}

								data[i*32+3]+='0';
								data[i*32+2]+='0';
								data[i*32+1]+='0';
								writeblock(fc, fs, data);
								total=0;
							}
							}
							}
						}
					}
				}
				sprintf(buffer,"0");
			}
			else
noempty2:			sprintf(buffer,"2");
			n = send(client_sockfd,buffer,1024,0);
			cout << endl << "Appending to file finished"<< endl;
		}
//exit
		else if(strcmp(cmd[0],"exit")==0)
		{
			send(sockfd2, "Q",1024,0);
			sprintf(buffer,"Q");
			send(client_sockfd, buffer,1024,0);
			break;
		}
	}
	close(client_sockfd);
	close(sockfd1);
	close(sockfd2);
	return 0;
}
//change the current data to cylinder and sector
int cs_change(int &c, int &s)
{
	while(s>=SECTORS)
	{
		c++;
		s -= SECTORS;
	}
	if(c>=CYLINDERS) return 1;
	else	return 0;

}
//get the data from the server
void readblock(int C,int S,char data[])
{
	char buffer[1024]={0};
	cs_change(C,S);
	sprintf(buffer,"R %d %d",C,S);

	send(sockfd2, buffer,1024,0);
	recv(sockfd2, buffer, 1024,0);
	memcpy(data,&buffer[5],256*sizeof(char));
}
//send the data to the server
void writeblock(int C,int S,char data[])
{
	char buffer[1024]={0};
	cs_change(C,S);
	sprintf(buffer,"W %d %d 256 ",C,S);
	strcat(buffer,data);
	send(sockfd2, buffer,1024,0);
	recv(sockfd2, buffer, 1024,0);
}
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
