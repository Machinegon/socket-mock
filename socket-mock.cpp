#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <deque>
#include <sys/types.h>
#include <cerrno>
#include <unistd.h>
#include <getopt.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <fcntl.h>


using namespace std;
#define APP_VERSION "1.0.0.4"

// ========== VARIABLES ============
unsigned int port = 0;
char *strFilePath;
char *strServerIp;

static const struct option options[] = {
	{"help", no_argument, NULL, 'h'},
	{"ip", required_argument, NULL, 'i'},
	{"port", required_argument, NULL, 'p'},
	{"file", required_argument, NULL, 'f'},
	{NULL, 0, 0, 0}
};

struct commandResponse
{
	string strCommand;
	string strResponse;
};

// ============ FUNCTIONS ===============
void printHelp()
{
	printf("Usage: socket-mock [--port=p] [--ip <ip>] [--file <path>]\n");
}

volatile sig_atomic_t flag = 0;
void signalHandler(int sig)
{
	flag = sig;
}


// ============= MAIN ===================
int main(int argc, char **argv)
{
	int n = 0;
	bool bNoFile = false;
	deque<commandResponse> dqCommandResponses;
	string delimiter = "||";
	string wildcard = "*";

	if(argc < 3) {
		printHelp();
		return -1;
	}
	// =========== Get args ===============
	while (n >= 0) 
	{
		n = getopt_long(argc, argv, "i:hsap:d:Dr:C:K:A:R:vu:g:S", (struct option *)options, NULL);
		if(n < 0)
		{
			continue;
		}

		switch(n)
		{
		case 'h':
			printHelp();
			break;
		case 'p':
			port = atoi(optarg);
			break;
		case 'f':
			strFilePath = optarg;
			break;
		case 'i':
			strServerIp = optarg;
			break;
		}
	}

	// ========= Initialize socket server ==========
	if(port <= 0 || strServerIp == NULL) {
		printf("Port and IP are required\n");
		printHelp();
		return -1;
	}

	printf("Socket-mock version: %s\n", APP_VERSION);
	printf("Initializing with ip: %s, port: %i\n", strServerIp, port);
	if(strFilePath != NULL) {
		if( access( strFilePath, F_OK ) == -1) {
			printf("File: %s does not exists or is inaccessible.\n", strFilePath);
			return -1;
		}
	}
	else {
		bNoFile = true;
	}

	// Parse command/response file
	if(!bNoFile) {
		ifstream infile(strFilePath);
		string line;
		int iCount = 1;
		while (getline(infile, line)) 
		{
			if(count(line.begin(), line.end(), '*') > 2) { // Validate that there's no more than 2
				printf("command/response file syntax invalid. You're restricted to two wildcard (*) per line.\n");
				return -1;
			}
			
			struct commandResponse command;
			size_t pos = line.find(delimiter);	
			command.strCommand = line.substr(0, pos);
			command.strResponse = line.substr(pos + delimiter.length(), line.length());
			dqCommandResponses.push_front(command);
			iCount++;
		}
	}

	int sockfd, newsock;
	char buffer[2048];
	memset(buffer, 0, sizeof(buffer));

	/*Creating a new socket*/
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("Socket error");
		exit(1);
	}

	fcntl(sockfd, F_SETFL, O_NONBLOCK);

	/*Creating socket address structure*/
	struct sockaddr_in server_addr, client_address;

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = inet_addr(strServerIp);
	bzero(&(server_addr.sin_zero),8);

	/*Binding*/
	if(bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		perror("Bind error");
		exit(1);
	}

	/*Listening for new connection; maximum 5*/
	if(listen(sockfd, 5) < 0)
	{
		perror("Listen error");
	}

	// ========= Listen for incoming requests ========
	printf("Listening on port: %i responses file: %s \n", port, strFilePath);
	signal(SIGINT, signalHandler);
	while(1)
	{
		if(flag == SIGINT || flag == SIGABRT) {
			printf("Releasing socket...bye\n");
			close(sockfd);
			return 0;
		}

		socklen_t len = sizeof(client_address);
		if((newsock = accept(sockfd, (struct sockaddr *)&client_address, &len)) < 0)
		{
			if(errno == EWOULDBLOCK) {
				nanosleep((const struct timespec[]){{0, 50000000L}}, NULL);
				continue;
			}
			else {
				perror("Accept error");
				exit(1);
			}
		}

		int i;
		if((i = read(newsock, buffer, sizeof(buffer))) < 0)
		{
			perror("Receive error");
			exit(1);
		}
		if(i == 0)
		{
			printf("Socket closed remotely\n");
		}
		if(i > 0)
		{
			printf("Received %d bytes from client: %s \n",i, inet_ntoa(client_address.sin_addr));
			printf("data: %s\n",buffer);
		}

		if(!bNoFile) {
			// Check if command is valid and answer
			for(deque<commandResponse>::iterator it = dqCommandResponses.begin(); it!= dqCommandResponses.end(); it++) 
			{
				string command = (const char *) &buffer[0];
				bool bFindStr = false;
				
				// Check if command is handled
				struct commandResponse cmd = (commandResponse) *it;
				string command2 = "";
				if(cmd.strCommand.find(wildcard) != -1) {  // Wildcard ??
					size_t pos = cmd.strCommand.find(wildcard);
					if(count(cmd.strCommand.begin(), cmd.strCommand.end(), '*') == 1) { // One wildcard
						if(pos != 0) { // Wildcard at the end of string
							command2 = cmd.strCommand.substr(0, (cmd.strCommand.length() - 1));
							command = command.substr(0, command2.length());
						}
						else { // Wildcard at the start of string
							command2 = cmd.strCommand.substr(1, cmd.strCommand.length());
							int cl = command.length()  < command2.length() ? 0: command.length() - command2.length();
							command = command.substr(cl, command.length());
						}
					}
					else {
						// 2 Wildcards, get string in-between
						command2 = cmd.strCommand.substr(1, (cmd.strCommand.length() - 2));
						bFindStr = true;
					}
				}
				else {
					command2 = cmd.strCommand;
				}

				if(command.compare(command2.c_str()) == 0 || (bFindStr && command.find(command2) != -1))
				{
					// Command match, return response
					if(write(newsock, cmd.strResponse.c_str(), cmd.strResponse.length()) < 0) {
						perror("Error writing response");
					}
					else  {
						if(cmd.strResponse.length() > 50)
						{
							printf("Response: %s...\n", cmd.strResponse.substr(0, 50).c_str());
						}
						else {
							printf("Response: %s\n", cmd.strResponse.c_str());
						}
					}
					break;
				}
			}
		}

		memset(buffer, 0, sizeof(buffer));
		close(newsock);
	}
	close(sockfd);

	return 0;
}