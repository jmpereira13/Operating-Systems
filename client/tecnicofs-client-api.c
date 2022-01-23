#include "tecnicofs-client-api.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>

/* sockfd of client's socket */
int sockfd;
/* the server's socket info*/
struct sockaddr_un server_addr;
socklen_t server_len;


/**
 * set's the socket address of type unix
 *
 * Input:
 * - path: path of the socket
 * - addr: its adress
 * Return:
 * - SUN_LEN: the length 
 */
int setSockAddrUn(char *path, struct sockaddr_un *addr) {
  	// se a estrutura nao foi criada
  	if (addr == NULL)
    	return 0;

  	// limpar e adicionar as especificacoes
  	bzero((char *)addr, sizeof(struct sockaddr_un));
  	addr->sun_family = AF_UNIX;
  	strcpy(addr->sun_path, path);

  	// returning its size
  	return SUN_LEN(addr);
}


/**
 * CREATE, sending the command to the server,
 * and receiving its answer (result) 
 *
 *	Input: 
 * - filename: path of new node
 * - nodeType: type of node (f=file / d=directory)
 * Return: 
 * - result (0=success / -1=fail)
 */
int tfsCreate(char *filename, char nodeType) {
	char result[3];

	// creating message to send to the server's socket 
	char message[100] = "c ";
	strcat(message, filename);
	strcat(message, " ");
	strcat(message, &nodeType);

	// sending message
	if(sendto(sockfd, message, strlen(message)+1, 0, (struct sockaddr *) &server_addr, server_len) < 0){
		perror("client: sendto error");
   		exit(EXIT_FAILURE);
	}

  	// receiving the result 
  	if (recvfrom(sockfd, result, sizeof(result)-1, 0, (struct sockaddr *) &server_addr, &server_len) < 0) {
    	perror("client: recvfrom error");
    	exit(EXIT_FAILURE);
  	} 

  	// passing int result instead of char
  	return atoi(result);
}


/**
 * DELETE, sending the command to the server,
 * and receiving its answer (result) 
 *
 *	Input: 
 * - path: path of node to eliminate
 * Return: 
 * - result (0=success / -1=fail)
 */
int tfsDelete(char *path) {
	char result[3];

	// creating message to send to the server's socket
	char message[100] = "d ";
	strcat(message, path);

	// sending message
	if(sendto(sockfd, message, strlen(message)+1, 0, (struct sockaddr *) &server_addr, server_len) < 0){
		perror("client: sendto error");
   		exit(EXIT_FAILURE);
	}

	// receiving the result 
  	if (recvfrom(sockfd, result, sizeof(result)-1, 0, (struct sockaddr *) &server_addr, &server_len) < 0) {
    	perror("client: recvfrom error");
    	exit(EXIT_FAILURE);
  	} 

  	// passing int result instead of char
  	return atoi(result);
}

/**
 * MOVE, sending the command to the server,
 * and receiving its answer (result) 
 *
 *	Input: 
 * - from: path of node to move
 * - to: path to move to
 * Return: 
 * - result (0=success / -1=fail)
 */
int tfsMove(char *from, char *to) {
	char result[3];

	// creating message to send to the server's socket
	char message[100] = "m ";
	strcat(message, from);
	strcat(message, " ");
	strcat(message, to);
	
	// sending message
	if(sendto(sockfd, message, strlen(message)+1, 0, (struct sockaddr *) &server_addr, server_len) < 0){
		perror("client: sendto error");
   	exit(EXIT_FAILURE);
	}
  
  	// receiving the result 
  	if (recvfrom(sockfd, result, sizeof(result)-1, 0, (struct sockaddr *) &server_addr, &server_len) < 0) {
    	perror("client: recvfrom error");
    	exit(EXIT_FAILURE);
  	} 

  	// passing int result instead of char
  	return atoi(result);
}


/**
 * LOOKUP, sending the command to the server,
 * and receiving its answer (result) 
 *
 *	Input: 
 * - path: path of node to lookup
 * Return: 
 * - result (0=success / -1=fail)
 */
int tfsLookup(char *path) {
	char result[3];

	// creating message to send to the server's socket 
	char message[100] = "l ";
	strcat(message, path);

	// sending message
	if(sendto(sockfd, message, strlen(message)+1, 0, (struct sockaddr *) &server_addr, server_len) < 0){
		perror("client: sendto error");
   		exit(EXIT_FAILURE);
	}
  
  	// receiving the result 
  	if (recvfrom(sockfd, result, sizeof(result)-1, 0, (struct sockaddr *) &server_addr, &server_len) < 0) {
    	perror("client: recvfrom error");
    	exit(EXIT_FAILURE);
  	} 

  	// passing int result instead of char
  	return atoi(result);
}

/**
 * PRINT, sending the command to the server,
 * and receiving its answer (result) 
 *
 *	Input: 
 * - path: path of file to print output
 * Return: 
 * - result (0=success / -1=fail)
 */
int tfsPrint(char *path){
	char result[3];

	char message[100] = "p ";
	strcat(message, path);

	// sending message
	if(sendto(sockfd, message, strlen(message)+1, 0, (struct sockaddr *) &server_addr, server_len) < 0){
		perror("client: sendto error");
		exit(EXIT_FAILURE);
	}

	// receiving the result 
  	if (recvfrom(sockfd, result, sizeof(result)-1, 0, (struct sockaddr *) &server_addr, &server_len) < 0) {
    	perror("client: recvfrom error");
    	exit(EXIT_FAILURE);
  	} 

  	return atoi(result);
}

/**
	* creating, setting socket adreess and binding the 
	* client's socket. Also setting socket adress of the 
	* server's socket
	*
	*	Input: 
	* - sockPath: path of server's socket
 	* Return: 
	* - 0 = SUCCESS
	*/
int tfsMount(char * sockPath) {
	char client_path[11] = "/tmp/client";
	struct sockaddr_un client_addr;
	socklen_t client_len;

	// creating the socket of the client
	if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
    	perror("client: can't open socket");
    	exit(EXIT_FAILURE);
  	}
  
  	// unlink to make sure its safe
  	unlink(client_path);

  	// set socket unix adress for the client
  	client_len = setSockAddrUn (client_path, &client_addr);

  	// bind for the client
  	if (bind(sockfd, (struct sockaddr *) &client_addr, client_len) < 0) {
    	perror("client: bind error");
    	exit(EXIT_FAILURE);
  	}  

  	// set the socket structure of the server 
  	if( (server_len = setSockAddrUn(sockPath, &server_addr) ) <= 0 ){
  		perror("client: can't associate server's socket");
  		exit(EXIT_FAILURE);
  	}

  	return 0;
}


/**
 * closign the socket to end communication with 
 * the server's socket
 * return:
 * - 0 = SUCCESS
 */
int tfsUnmount() {
	// closing socket
  	close(sockfd);
  	return 0;
}
