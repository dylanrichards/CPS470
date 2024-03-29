#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>

#define MAX_CLIENTS 4
#define BUFFER_SZ 2048

static _Atomic unsigned int cli_count = 0;
static int uid = 10; // Client or user id

/* Client structure */
typedef struct{
	struct sockaddr_in address; // Socket address
	int sockfd;	// Client socket file descriptor
	int uid; // Current ID
	char name[32];
} client_t;

// thread pool
client_t *thread_pool[MAX_CLIENTS];

// add a thread to the pool
int pool_add(client_t *cl) {
	// add cl to thread pool to the first null
	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (thread_pool[i] == NULL) {
			thread_pool[i] = cl;
			return 1;
		}
	}

	return 0;
}

// remove thread from the pool
void pool_remove(client_t *cl) {
	// assign null to thread pool where uid == cl uid
	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (thread_pool[i] != NULL && thread_pool[i]->uid == cl->uid) {
			thread_pool[i] = NULL;
			break;
		}
	}
}

/* Handle all communication with the client */
void *handle_client(void *arg){
	char buff_out[BUFFER_SZ]; // Hold sending message
	char name[32]; // Client name
	int leave_flag = 0; // Loop termination flag

	client_t *cli = (client_t *)arg; // Define client structure from arguments

	cli_count++;

	// Receive a message from a socket
	recv(cli->sockfd, name, 32, 0);
	
	// Store client's name
	strcpy(cli->name, name);
	
	// Promote client joining message
	sprintf(buff_out, "<<<%s has joined>>>\n", cli->name);
	printf("%s", buff_out);
	
	// Keep receiving client's messages until client exits
	while(leave_flag == 0){
		
		// Erases the data in buffer_out
		bzero(buff_out, BUFFER_SZ);
	
		// Receive a message from a socket
		int receive = recv(cli->sockfd, buff_out, BUFFER_SZ, 0);
		
		// Receive sucessfully
		if (receive > 0){
			// Receive none empty message 
			if(strlen(buff_out) > 0){
				// Remove the newline character '\n' in the end of the message
				buff_out[strlen(buff_out)-1] = '\0';
				printf(">%s\n", buff_out);
			}
		// Check "exit" message
		} else if (receive == 0 || strncmp(buff_out, "exit", 4) == 0){
			sprintf(buff_out, "<<<%s has left>>>\n", cli->name);
			printf("%s", buff_out);
			leave_flag = 1;
		} else {
			printf("ERROR: -1\n");
			leave_flag = 1;
		}
	}

	/* Delete client from pool and yield thread */
	pool_remove(cli);
	close(cli->sockfd);
	free(cli);
	cli_count--;
	pthread_exit(0);
	
	return NULL;
}

int main(int argc, char **argv){

	// Check user arguments
	if(argc != 2){
		fprintf(stderr, "Usage: %s <port>\n", argv[0]);
		return EXIT_FAILURE;
	}

	char *ip = "127.0.0.1"; // Use local host as server address
	int port = atoi(argv[1]); // Accept user input as port number
	int option = 1;
	int listenfd = 0;	// Server socket file descriptor
    int connfd = 0;	// return value of connect()
	struct sockaddr_in serv_addr; // Define address format for server
	struct sockaddr_in cli_addr; // Define address format for client

	pthread_t tid[MAX_CLIENTS]; /* the thread identifier */
	pthread_attr_t attr; /* set of thread attributes */
	
	// Socket settings 
	// Linux IPv4 protocol implementation, run "man 7 ip" for details
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	// Address format
	serv_addr.sin_family = AF_INET; // address family: AF_INET
	serv_addr.sin_addr.s_addr = inet_addr(ip); // port in network byte order, inet_addr converts host address into binary
	serv_addr.sin_port = htons(port); //converts the an integer from host byte order to network byte order

	// Get and set options on sockets 
	// Allow immediate reuse of the port and address
	setsockopt(listenfd, SOL_SOCKET, (SO_REUSEPORT | SO_REUSEADDR), (char*)&option, sizeof(option));

	// Bind a name to a socket
	bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

	// Listen for connections on a socket
	listen(listenfd, 10);
	
	// Promote Welcome message
	printf("=== WELCOME TO THE CHATROOM ===\n");
	
	/* set the default attributes of the thread */ 
	pthread_attr_init(&attr);

	while (1) {
		// Accept client connection 
		socklen_t clilen = sizeof(cli_addr); // Client address
		connfd = accept(listenfd, (struct sockaddr*)&cli_addr, &clilen);

		// Client settings 
		client_t *cli = (client_t *)malloc(sizeof(client_t));
		cli->address = cli_addr;
		cli->sockfd = connfd;
		cli->uid = uid++;

		while (pool_add(cli) == 0);

		// add the client to the pool
		// Handle client connection
		/* create the thread */
		pthread_create(&tid[cli_count], &attr, handle_client, (void *)cli);
	}
	
	return EXIT_SUCCESS;
}
