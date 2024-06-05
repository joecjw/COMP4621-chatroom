#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>
#include "chatroom.h"
#include <poll.h>

#define MAX 1024 // max buffer size
#define PORT 6789 // server port number
#define MAX_USERS 50 // max number of users
static unsigned int users_count = 0; // number of registered users

static user_info_t *listOfUsers[MAX_USERS] = {0}; // list of users

/* Get user name from userList */
char * get_username(int sockfd);
/* Get user sockfd by name */
int get_sockfd(char *name);

/* Determine whether the user has been registered  */
int isNewUser(char* name) {
	unsigned int i;
	int flag = -1;
	/*******************************************/
	/* Compare the name with existing usernames */
	/*******************************************/
	for(i = 0; i < users_count; i++){
		if(strcmp(name, listOfUsers[i]->username) == 0){
			return i;
		}
	}

	return flag;
}

/* Get user name from userList */
char * get_username(int ss){
	unsigned int i;
	int flag = -1;
	static char uname[MAX];
	/*******************************************/
	/* Get the user name by the user's sock fd */
	/*******************************************/
	for(i = 0; i < users_count; i++){
		if(listOfUsers[i]->sockfd == ss){
			flag = 1;
			break;
		}
	}

	if(flag == 1){
		strcpy(uname, listOfUsers[i]->username);
		printf("get user name: %s\n", uname);
		return uname;
	}
	else{
		return NULL;
	}
}

/* Get user sockfd by name */
int get_sockfd(char *name){
	unsigned int i;
	int flag = -1;
	int sock;
	/*******************************************/
	/* Get the user sockfd by the user name */
	/*******************************************/
	for(i = 0; i < users_count; i++){
		if(strcmp(listOfUsers[i]->username, name)  == 0){
			flag = 1;
			break;
		}
	}

	if(flag == 1){
		sock = listOfUsers[i]->sockfd;
		return sock;
	}
	else{
		return flag;
	}
}
// The following two functions are defined for poll()
// Add a new file descriptor to the set
void add_to_pfds(struct pollfd* pfds[], int newfd, int* fd_count, int* fd_size)
{
	// If we don't have room, add more space in the pfds array
	if (*fd_count == *fd_size) {
		*fd_size *= 2; // Double it

		*pfds = realloc(*pfds, sizeof(**pfds) * (*fd_size));
	}

	(*pfds)[*fd_count].fd = newfd;
	(*pfds)[*fd_count].events = POLLIN; // Check ready-to-read

	(*fd_count)++;
}
// Remove an index from the set
void del_from_pfds(struct pollfd pfds[], int i, int* fd_count)
{
	// Copy the one from the end over this one
	pfds[i] = pfds[*fd_count - 1];

	(*fd_count)--;
}



int main(){
	int listener;     // listening socket descriptor
	int newfd;        // newly accept()ed socket descriptor
	int addr_size;     // length of client addr
	struct sockaddr_in server_addr, client_addr;
	
	char buffer[MAX]; // buffer for client data
	int nbytes;
	int fd_count = 0;
	int fd_size = 5;
	struct pollfd* pfds = malloc(sizeof * pfds * fd_size);
	
	int yes=1;        // for setsockopt() SO_REUSEADDR, below
    int i, j, u, rv;

    
	/**********************************************************/
	/*create the listener socket and bind it with server_addr*/
	/**********************************************************/
	// socket create and verification
	listener = socket(AF_INET, SOCK_STREAM, 0);
	if (listener == -1) {
		printf("Socket creation failed...\n");
		exit(0);
	}
	else
		printf("Socket successfully created..\n");
	
	
	bzero(&server_addr, sizeof(server_addr));
	
	// asign IP, PORT
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(PORT);

  // Binding newly created socket to given IP and verification
	if ((bind(listener, (struct sockaddr*)&server_addr, sizeof(server_addr))) != 0) {
		printf("Socket bind failed...\n");
		exit(0);
	}
	else
		printf("Socket successfully binded..\n");

	// Now server is ready to listen and verification
	if ((listen(listener, 5)) != 0) {
		printf("Listen failed...\n");
		exit(3);
	}
	else
		printf("Server listening..\n");
		
	// Add the listener to set
	pfds[0].fd = listener;
	pfds[0].events = POLLIN; // Report ready to read on incoming connection
	fd_count = 1; // For the listener

	// main loop
	for(;;) {
		/***************************************/
		/* use poll function */
		/**************************************/
		int poll_count = poll(pfds, fd_count, -1);
			if (poll_count == -1) {
				perror("poll");
				exit(1);
			}

		// run through the existing connections looking for data to read
		for(i = 0; i < fd_count; i++) {
			if (pfds[i].revents & POLLIN) { // we got one!!
				if (pfds[i].fd == listener) {
					/**************************/
					/* we are the listener and we need to handle new connections from clients */
					/****************************/
					addr_size = sizeof(client_addr);
					newfd = accept(listener, (struct sockaddr*)&client_addr, &addr_size);
					if (newfd == -1) {
						perror("accept");
					} 
					else {
						add_to_pfds(&pfds, newfd, &fd_count, &fd_size);
						printf("pollserver: new connection from %s on socket %d\n",inet_ntoa(client_addr.sin_addr),newfd);
					} 

					// send welcome message
					bzero(buffer, sizeof(buffer));
					strcpy(buffer, "Welcome to the chat room!\nPlease enter your nickname and password\nPlease follow the format name:password, no WHITE SPACE allowed\n");
					if (send(newfd, buffer, sizeof(buffer), 0) == -1){
						perror("send");
					}
				}
			
				else{
					// handle data from a client
					bzero(buffer, sizeof(buffer));
					if ((nbytes = recv(pfds[i].fd, buffer, sizeof(buffer), 0)) <= 0) {
						// got error or connection closed by client
						if (nbytes == 0) {
							// connection closed
							printf("pollserver: socket %d hung up\n", pfds[i].fd);
						} 
						else {
							perror("recv");
						}
						close(pfds[i].fd); // Bye!
						del_from_pfds(pfds, i, &fd_count);
					} 
					else {
						// we got some data from a client
						if (strncmp(buffer, "REGISTER", 8)==0){
							printf("Got register/login message\n");
							/********************************/
							/* Get the user name and add the user to the userlist*/
							/**********************************/
							if(strchr(buffer,':') == NULL || buffer[0] == ':' || buffer[strlen(buffer)-1] == ':'){
								bzero(buffer, sizeof(buffer));
								strcpy(buffer, "Register/Login failed ! Please check the input format and try again!\n");
								if (send(pfds[i].fd, buffer, sizeof(buffer), 0) == -1){
									perror("send");
								}								
								continue;
							}

							char name[C_NAME_LEN + 1];
							bzero(name, sizeof(name));
							char *temp = strtok(buffer, ":");
							strncpy(name,temp+8,strlen(temp)-8);

							if(name[0] == '\0'){
								bzero(buffer, sizeof(buffer));
								strcpy(buffer, "Register/Login failed ! Please check the input format and try again!\n");
								if (send(pfds[i].fd, buffer, sizeof(buffer), 0) == -1){
									perror("send");
								}
								continue;
							}

							char password[C_NAME_LEN + 1];
							bzero(password, sizeof(password));
							temp = strtok(NULL, ":");
							strcpy(password,temp);

							if(password[0] == '\0'){
								bzero(buffer, sizeof(buffer));
								strcpy(buffer, "Register/Login failed ! Please check the input format and try again!\n");
								if (send(pfds[i].fd, buffer, sizeof(buffer), 0) == -1){
									perror("send");
								}
								continue;
							}
							if (isNewUser(name) == -1) {
								/********************************/
								/* it is a new user and we need to handle the registration*/
								/**********************************/
								if(users_count ==  MAX_USERS){
									printf("sorry the system is full, please try again later\n");
									break;
								}

								/***************************/
								/* add the user to the list */
								/**************************/
								listOfUsers[users_count] = malloc(sizeof(user_info_t));
								bzero(listOfUsers[users_count]->username, sizeof(listOfUsers[users_count]->username));
								strcpy(listOfUsers[users_count]->username, name);
								bzero(listOfUsers[users_count]->password, sizeof(listOfUsers[users_count]->password));
								strcpy(listOfUsers[users_count]->password, password);			
								listOfUsers[users_count]->sockfd = pfds[i].fd;
								listOfUsers[users_count]->state = 1;
								users_count++;

								/********************************/
								/* create message box (e.g., a text file) for the new user */
								/**********************************/
								FILE *fp;
								char fileName[C_NAME_LEN + 5];
								bzero(fileName, sizeof(fileName));
								strcpy(fileName, get_username(pfds[i].fd));
								strcat(fileName, ".txt");
								fp = fopen(fileName,"w");
								if(fp == NULL){
									printf( "Could not open source file\n" );
									exit(1);
								} 
								fclose(fp);

								// broadcast the welcome message (send to everyone except the listener)
								bzero(buffer, sizeof(buffer));
								strcpy(buffer, "Welcome ");
								strcat(buffer, name);
								strcat(buffer, " to join the chat room!\n");

								/*****************************/
								/* Broadcast the welcome message*/
								/*****************************/
								for(int k = 0; k < users_count; k++){
									if (send(listOfUsers[k]->sockfd, buffer, sizeof(buffer), 0) == -1){
										perror("send");
									}											
								}

								/*****************************/
								/* send registration success message to the new user*/
								/*****************************/
								bzero(buffer, sizeof(buffer));
								strcpy(buffer, "A new account has been created.");
								if (send(pfds[i].fd, buffer, sizeof(buffer), 0) == -1){
									perror("send");
								}

							}
							else {
								/********************************/
								/* it's an existing user and we need to handle the login. Note the state of user,*/
								/**********************************/
								if(strcmp(listOfUsers[isNewUser(name)]->password,password) == 0){
									//varify password successfully
									listOfUsers[isNewUser(name)]->state = 1;
									listOfUsers[isNewUser(name)]->sockfd = pfds[i].fd;
									
									/********************************/
									/* send the offline messages to the user and empty the message box*/
									/**********************************/
									FILE *fp;
									char* fileName[C_NAME_LEN+5];
									bzero(fileName, sizeof(fileName));
									strcpy(fileName, name);	
									strcat(fileName, ".txt");
									fp = fopen(fileName,"r");
									if (fp != NULL) {
										fseek(fp, 0, SEEK_END);
										long size = ftell(fp);
										if (size == 0) {
											bzero(buffer, sizeof(buffer));
											strcpy(buffer, "No offline messages\n");
											if (send(pfds[i].fd, buffer, sizeof(buffer), 0) == -1){
												perror("send");
											}
											fclose(fp);
										}
										else{
											bzero(buffer, sizeof(buffer));
											strcpy(buffer, "Welcome back! The message box contains:\n");
											if (send(pfds[i].fd, buffer, sizeof(buffer), 0) == -1){
												perror("send");
											}
											bzero(buffer,sizeof(buffer));
											fseek(fp, 0, SEEK_SET);
											while(fgets(buffer, MAX, fp)) {
												buffer[strlen(buffer) - 1] = '\0';
												if (send(pfds[i].fd, buffer, sizeof(buffer), 0) == -1){
													perror("send");
												}
											}
											fclose(fp);
											fp = fopen(fileName,"w");
											fclose(fp);
										}
									}

									// broadcast the welcome message (send to everyone except the listener)
									bzero(buffer, sizeof(buffer));
									strcat(buffer, name);
									strcat(buffer, " is online!");
									/*****************************/
									/* Broadcast the welcome message*/
									/*****************************/
									for(unsigned int k = 0; k < users_count; k++){
										if(listOfUsers[k]->sockfd != pfds[i].fd){			
											if (send(listOfUsers[k]->sockfd, buffer, sizeof(buffer), 0) == -1){
												perror("send");
											}													
										}
									}
								}
								else{
									//varify password failed
									/********************************/
									/* send login failed and try to re-login messages to the user*/
									/**********************************/
									bzero(buffer, sizeof(buffer));
									strcpy(buffer, "Login failed with wrong password! Please check the input format and try again!\n");
									if (send(pfds[i].fd, buffer, sizeof(buffer), 0) == -1){
										perror("send");
									}
								}
							}
						}
						else if (strncmp(buffer, "EXIT", 4)==0){
							printf("Got exit message. Removing user from system\n");
							// send leave message to the other members
															bzero(buffer, sizeof(buffer));
							strcpy(buffer, get_username(pfds[i].fd));
							strcat(buffer, " has left the chatroom");
							/*********************************/
							/* Broadcast the leave message to the other users in the group*/
							/**********************************/
							for(unsigned int k = 0; k < users_count; k++){
								if(listOfUsers[k]->sockfd != pfds[i].fd){					
									if (send(listOfUsers[k]->sockfd, buffer, sizeof(buffer), 0) == -1){
										perror("send");
									}											
									
								}
							}

							/*********************************/
							/* Change the state of this user to offline*/
							/**********************************/
							listOfUsers[isNewUser(get_username(pfds[i].fd))]->state = 0;
							listOfUsers[isNewUser(get_username(pfds[i].fd))]->sockfd = -2;
							
							//close the socket and remove the socket from pfds[]
							close(pfds[i].fd);
							del_from_pfds(pfds, i, &fd_count);
						}

						else if (strncmp(buffer, "WHO", 3)==0){
							// concatenate all the user names except the sender into a char array
							printf("Got WHO message from client.\n");
							char ToClient[MAX];
							bzero(ToClient, sizeof(ToClient));
							/***************************************/
							/* Concatenate all the user names into the tab-separated char ToClient and send it to the requesting client*/
							/* The state of each user (online or offline)should be labelled.*/
							/***************************************/
							strcpy(ToClient,"");
							for(int k = 0; k < users_count; k++){
								if(strcmp(listOfUsers[k]->username, get_username(pfds[i].fd)) != 0){
									strcat(ToClient, listOfUsers[k]->username);
									if(listOfUsers[k]->state == 1){
										strcat(ToClient, "*");
									}							
									if(k != users_count-1){
										strcat(ToClient, ", ");		
									}
								}
							}
							if(ToClient[strlen(ToClient)-2] == ','){
								ToClient[strlen(ToClient)-2] = '\0';
							}
							if (send(pfds[i].fd, ToClient, sizeof(ToClient), 0) == -1){
								perror("send");
							}
						}

						else if (strncmp(buffer, "#", 1)==0){
							// send direct message
							if(strchr(buffer,':') == NULL || buffer[1] == ':' || buffer[strlen(buffer)-1] == ':'){
								bzero(buffer, sizeof(buffer));
								strcpy(buffer, "Send direct message failed ! Please check the input format and try again!\n");
								if (send(pfds[i].fd, buffer, sizeof(buffer), 0) == -1){
									perror("send");
								}								
								continue;
							}		
							
							// get send user name:
							printf("Got direct message.\n");
							// get which client sends the message
							char sendname[MAX];
							bzero(sendname, sizeof(sendname));
							strcpy(sendname, get_username(pfds[i].fd));
							printf("sender:%s\n", sendname);

							// get the destination username
							char destname[MAX];
							bzero(destname, sizeof(destname));
							char *temp = strtok(buffer, ":");
							strncpy(destname,temp+1,strlen(temp)-1);
							printf("receiver:%s\n", destname);

							// get dest sock
							int destsock;
							destsock = get_sockfd(destname);
							printf("des_sock:%d\n", destsock);

							// get the message
							char msg[MAX];
							bzero(msg, sizeof(msg));
							temp = strtok(NULL, ":");
							strcpy(msg, temp);
							printf("msg:%s\n", msg);

							/**************************************/
							/* Get the source name xx, the target username and its sockfd*/
							/*************************************/


							if (destsock == -1) {
								/**************************************/
								/* The target user is not found. Send "no such user..." messsge back to the source client*/
								/*************************************/
								bzero(buffer, sizeof(buffer));
								strcpy(buffer, "There is no such user. Please check your input format.");
								if (send(pfds[i].fd, buffer, sizeof(buffer), 0) == -1){
									perror("send");
								}
							}
							else {
								// The target user exists.
								// concatenate the message in the form "xx to you: msg"
								char sendmsg[MAX];
								strcpy(sendmsg, sendname);
								strcat(sendmsg, " to you: ");
								strcat(sendmsg, msg);

								/**************************************/
								/* According to the state of target user, send the msg to online user or write the msg into offline user's message box*/
								/* For the offline case, send "...Leaving message successfully" message to the source client*/
								/*************************************/
								if(listOfUsers[isNewUser(get_username(destsock))]->state == 0){
									//write the msg into offline user's message box	//
									FILE *fp;
									char* fileName[C_NAME_LEN+5];
									bzero(fileName, sizeof(fileName));
									strcpy(fileName, destname);
									strcat(fileName, ".txt");
									fp = fopen(fileName,"a");
									if (fp != NULL) {
										strcat(sendmsg,"\n");
										fprintf(fp, sendmsg);
										fclose(fp);
									}

									bzero(buffer, sizeof(buffer));
									strcpy(buffer, destname);
									strcat(buffer, " is offline. Leaving message successfully");
									if (send(pfds[i].fd, buffer, sizeof(buffer), 0) == -1){
										perror("send");
									}
								}
								else{
									//send direct online msg to target user
									if (send(destsock, sendmsg, sizeof(sendmsg), 0) == -1){
										perror("send");
									}
								}
							}
						}

						else{
							printf("Got broadcast message from user\n");
							/**********************************************/
							/* Broadcast the message to all users except the one who sent the message*/
							/*********************************************/
							for(int k = 0; k < users_count; k++){
								if(listOfUsers[k]->sockfd != pfds[i].fd && listOfUsers[k]->state != 0){			
									if (send(listOfUsers[k]->sockfd, buffer, sizeof(buffer), 0) == -1){
										perror("send");
									}													
								}
							}
						} 
					}              
				} // end handle data from client
      } // end got new incoming connection
    } // end looping through file descriptors
  } // end for(;;) 
		

	return 0;
}
