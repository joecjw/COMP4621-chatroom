#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include "chatroom.h"

#define MAX 1024 // max buffer size
#define PORT 6789  // port number

static int sockfd;

void generate_menu(){
	printf("Hello dear user pls select one of the following options:\n");
	printf("EXIT\t-\t Send exit message to server - unregister ourselves from server\n");
    printf("WHO\t-\t Send WHO message to the server - get the list of current users except ourselves\n");
    printf("#<user>: <msg>\t-\t Send <MSG>> message to the server for <user>\n");
    printf("Or input messages sending to everyone in the chatroom.\n");
}

void *recv_server_msg_handler() {
    /********************************/
	/* receive message from the server and display on the screen*/
	/**********************************/
 	char receiveBuffer[MAX]; 
	int nbytes;
  while (1){
		bzero(receiveBuffer, sizeof(receiveBuffer));
		if ((nbytes = recv(sockfd, receiveBuffer, sizeof(receiveBuffer), 0)) <= 0) {
			// got error or connection closed by server
			if (nbytes == 0) {
				// connection closed
				close(sockfd);
				pthread_exit(NULL);
			} 
			else {
				perror("recv");
			}
		} 
		printf("%s\n", receiveBuffer);
	}
}

int main(){
  int n;
	int nbytes;
	struct sockaddr_in server_addr, client_addr;
	char buffer[MAX];
	
	/******************************************************/
	/* create the client socket and connect to the server */
	/******************************************************/
	// socket create and verification
  sockfd = socket(AF_INET, SOCK_STREAM, 0);;
  if (sockfd == -1){
		printf("Socket creation failed...\n");
		exit(0);
	}
	else
		printf("Socket successfully created...\n");
		
	bzero(&server_addr, sizeof(server_addr));

  // assign IP, PORT
  server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	server_addr.sin_port = htons(PORT);

	// connect the client socket to the server socket
  if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) != 0) {
		printf("Connection with the server failed...\n");
		exit(0);
	}
	else
		printf("Connected to the server...\n");


	generate_menu();
	/*************************************/
	/* Input the nickname and password (in format name:password, no WHITE SPACE allowed) and send a message to the server */
	/* Note that we concatenate "REGISTER" before the name to notify the server it is the register/login message*/
	/********************************************/
	char reg_info[2*C_NAME_LEN + 10]; 
	int count;
	for(;;){
		//receive message from server for registration
		bzero(buffer, sizeof(buffer));
		if (nbytes = recv(sockfd, buffer, sizeof(buffer), 0)==-1){
			perror("recv");
		}
		printf(buffer);
		
		if((strncmp(buffer, "Welcome", 7) == 0 && strncmp(buffer, "Welcome to the", 14) != 0) || strncmp(buffer, "No offline messages", 19) == 0){
			//registration success, exit loop to main loop
			break;
		}
		else{
			//registration failed, continue loop for registration
			bzero(reg_info, sizeof(reg_info));
			strcpy(reg_info, "REGISTER");
			count = 8;
			while ((reg_info[count++] = getchar()) != '\n');
			reg_info[count-1] = '\0';
			send(sockfd, reg_info, sizeof(reg_info),0);
		}
	}

    /*****************************************************/
	/* Create a thread to receive message from the server*/
	/* pthread_t recv_server_msg_thread;*/
	/*****************************************************/
  pthread_t recv_server_msg_thread ;
  pthread_create(&recv_server_msg_thread, NULL, recv_server_msg_handler, NULL);

	// chat with the server
	for (;;) {
		bzero(buffer, sizeof(buffer));
		n = 0;
		while ((buffer[n++] = getchar()) != '\n');
		buffer[n-1] = '\0';
		n -= 1;
		if ((strncmp(buffer, "EXIT", 4)) == 0) {
			printf("Client Exit...\n");
			/********************************************/
			/* Send exit message to the server and exit */
			/* Remember to terminate the thread and close the socket */
			/********************************************/
			send(sockfd, buffer, sizeof(buffer), 0);
			pthread_join(recv_server_msg_thread, NULL);
			sigsuspend();
		}
		else if (strncmp(buffer, "WHO", 3) == 0) {
			printf("Getting user list, pls hold on...\n");
			if (send(sockfd, buffer, sizeof(buffer), 0)<0){
				puts("Sending MSG_WHO failed");
				exit(1);
			}
			printf("If you want to send a message to one of the users, pls send with the format: '#username:message'\n");
		}
		else if (strncmp(buffer, "#", 1) == 0) {
			// If the user want to send a direct message to another user, e.g., aa wants to send direct message "Hello" to bb, aa needs to input "#bb:Hello"
			if (send(sockfd, buffer, sizeof(buffer), 0)<0){
				printf("Sending direct message failed...");
				exit(1);
			}
		}
		else {
			/*************************************/
			/* Sending broadcast message. The send message should be of the format "username: message"*/
			/**************************************/
			char name[C_NAME_LEN + 1];
			bzero(name, sizeof(name));
			char *temp = strtok(reg_info, ":");
			strncpy(name,temp+8,strlen(temp)-8);
			char buffer_with_name[MAX]; 
			bzero(buffer_with_name, sizeof(buffer_with_name));
			strcpy(buffer_with_name,name);
			strcat(buffer_with_name,":");
			strcat(buffer_with_name, buffer);
			if (send(sockfd, buffer_with_name, sizeof(buffer_with_name), 0)<0){
				printf("Sending broadcast message failed...");
				exit(1);
			}
		}
	}
	return 0;
}

