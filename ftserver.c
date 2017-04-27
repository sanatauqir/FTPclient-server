/******
Author: Sana Tauqir
References: Linux How-Tos http://www.linuxhowtos.org/data/6/server.c
OSU CS372 - server.c code
Project2 - Server/Client FTP
http://pubs.opengroup.org/onlinepubs/7908799/xsh/sysstat.h.html
******/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>


void error(const char *msg) { perror(msg); exit(1); } // Error function used for reporting issues
void startControl(int portNo);
void setupData(char* hostname, int dataport, char* command, char* filename);
void sendDirectory();
void sendFile(char*filename);

int listenSocketFD, establishedConnectionFD, portNumber, charsRead, charsWritten, socketFD;
socklen_t sizeOfClientInfo;
char buffer[256];
struct sockaddr_in serverAddress, clientAddress, javaAddress;
struct hostent *serverHostInfo;

int main(int argc, char *argv[])
{

	if (argc < 2) { fprintf(stderr,"USAGE: %s port\n", argv[0]); exit(1); }

	// Set up the address struct for the server
	memset((char *)&serverAddress, '\0', sizeof(serverAddress));
	portNumber = atoi(argv[1]);
	//signal(SIGPIPE,SIG_IGN); 
	startControl(portNumber);

	while (1){
		// Accept a connection, blocking if one is not available until one connects
		sizeOfClientInfo = sizeof(clientAddress); // Get the size of the address for the client that will connect
		establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo);
		if (establishedConnectionFD < 0) error("ERROR on accept");
		
		// Get the message from the client and display it
		memset(buffer, '\0', 256);
		
		printf("connection set up and ready to receive\n");
		charsRead = recv(establishedConnectionFD, buffer, 255, 0); // Read the client's message from the socket
		if (charsRead < 0) error("ERROR reading from socket");
		//printf("SERVER: I received this from the client: \"%s\"\n", buffer);

		char *token; int dataport; char* filename; char* hostname;
		token = strtok(buffer, "#");
		if (strcmp(token, "-l") == 0){
			charsWritten = send(establishedConnectionFD, "list command", 13, 0);
			if (charsRead < 0) error("ERROR writing to socket");
			hostname = strtok(NULL, "#");
			//printf("hostname %s\n", hostname);
			dataport = atoi(strtok(NULL, "#"));
			//printf("datanum %d\n", dataport);
			close(establishedConnectionFD);
			filename = "";
			setupData(hostname, dataport, token, filename);
		}
		else if (strcmp(token, "-g") ==0){
			charsWritten = send(establishedConnectionFD, "file command", 13, 0);
			if (charsRead < 0) error("ERROR writing to socket");
			hostname = strtok(NULL, "#");
			//printf("hostname %s\n", hostname);
			dataport = atoi(strtok(NULL, "#"));
			//printf("datanum %d\n", dataport);
			filename = strtok(NULL, "#");
			//printf("file %s\n", filename);
			close(establishedConnectionFD);
			setupData(hostname, dataport, token, filename);
		}
		else {
			// send invalid
			charsWritten = send(establishedConnectionFD, "invalid command", 16, 0);
			if (charsRead < 0) error("ERROR writing to socket");
			close(establishedConnectionFD);		
		}

		//close(establishedConnectionFD); // Close the existing socket which is connected to the client

	}
	close(listenSocketFD); // Close the listening socket
	return 0; 
}

void startControl(int portNo){

	// Set up the socket
	listenSocketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (listenSocketFD < 0) error("ERROR opening socket");

	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(portNo);
	serverAddress.sin_addr.s_addr = INADDR_ANY;

	// Enable the socket to begin listening
	if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to port
		error("ERROR on binding");
	listen(listenSocketFD, 5); // Flip the socket on - it can now receive up to 5 connections
	//printf("returning from startcontol right now\n");
}

void setupData(char* hostname, int dataport, char* command, char* filename){
// Set up the server address struct
	memset((char*)&javaAddress, '\0', sizeof(javaAddress)); // Clear out the address struct
	portNumber = dataport; // Get the port number, convert to an integer from a string
	javaAddress.sin_family = AF_INET; // Create a network-capable socket
	javaAddress.sin_port = htons(portNumber); // Store the port number
	serverHostInfo = gethostbyname(hostname); // Convert the machine name into a special form of address
	if (serverHostInfo == NULL) { fprintf(stderr, "CLIENT: ERROR, no such host\n"); exit(0); }
	memcpy((char*)&javaAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length);

	// Set up the socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (socketFD < 0) error("CLIENT: ERROR opening socket");
	int enable = 1;
	if (setsockopt(socketFD, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    		error("setsockopt(SO_REUSEADDR) failed");
	int connectionAttempts = 0; int status = 0;
	do {
		connectionAttempts++;
		status = connect(socketFD, (struct sockaddr *) &javaAddress, sizeof(javaAddress));
	} while (status == -1 && connectionAttempts < 10);
	//printf("calling mini fucntions now \n");
	if (strcmp(command, "-l") == 0){
		sendDirectory();
		//printf("sendDir success\n");
	}
	else{
		sendFile(filename);
	}
	//printf("returned from this\n");
}

void sendDirectory(){
	//printf("in send directory\n");
	char fileList[1000];
	memset(fileList, '\0', sizeof(fileList));
	
	DIR *dir = opendir(".");
	if (dir == NULL){
		perror("could not open directory");
		exit(1);
	}
	struct dirent *ent; struct stat info;
	int numFiles = 0;
	while((ent = readdir(dir)) != NULL) {
		//printf("in send directory while\n");
		//printf("filename%s\n", ent->d_name);
		
		stat(ent->d_name, &info);	
		//skip over directories
		if (S_ISDIR(info.st_mode)){
			continue;
		}
		else if ((strcmp(ent->d_name, ".") == 0) && (strcmp(ent->d_name, "..") == 0)){
			continue;
		}
		else {
			//printf("filename%s\n", ent->d_name);
			//keep adding the filenames to the filelist
			strcat(fileList, ent->d_name);
			strcat(fileList, ",");
			numFiles++;
			//charsWritten = write(socketFD, ent->d_name, 100);
		}

	}
	//printf("fileList %s\n", fileList);
	//printf("length %d\n", strlen(fileList));
	//int i = 0;
	//while (i < 10) {
		charsWritten = write(socketFD, fileList, strlen(fileList));
	//	i++;
	//}
	//printf("charsWritten success\n");
	charsWritten = write(socketFD, "", 1);
	closedir(dir);
	exit(1);

}

void sendFile(char* filename){
	//check if the file exists
		DIR *dir = opendir(".");
	if (dir == NULL){
		perror("could not open directory");
		exit(1);
	}
	struct dirent *ent; struct stat info;
	int numFiles = 0; int found = 0;
	while((ent = readdir(dir)) != NULL) {
		//printf("in send directory while\n");
		//printf("filename%s\n", ent->d_name);
		
		stat(ent->d_name, &info);	
		//skip over directories
		if (S_ISDIR(info.st_mode)){
			continue;
		}
		else if ((strcmp(ent->d_name, ".") == 0) && (strcmp(ent->d_name, "..") == 0)){
			continue;
		}
		else if (strcmp(ent->d_name, filename) == 0){
			found = 1;
		}
	}
	closedir(dir);

	if (found)
	{
		char fileBuffer[1000]; int readBytes;
		memset(fileBuffer, '\0',sizeof(fileBuffer));
		//open the file and send
		FILE *fp = fopen(filename, "r+");
		if (!fp){
			printf("could not open file");
			exit(1);
		}
		int offset = 0;
		while (!feof(fp)){
			readBytes = fread(fileBuffer, 1, sizeof(fileBuffer), fp);
			do{
				charsWritten = write(socketFD, &fileBuffer[offset], readBytes - offset);
			
				if (charsWritten < 1){
					perror("error reading file");
					fclose(fp);
					exit(1);			
				}	

				offset += charsWritten;
			}while (offset < readBytes);
		}
		fclose(fp);
	}
	else {
		charsWritten = write(socketFD, "FILE NOT FOUND", 25);
	}
	charsWritten = write(socketFD, "", 1);
	
	//close(socketFD);
	exit(1);
}
