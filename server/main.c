#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include "fs/operations.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <strings.h>
#include <sys/uio.h>
#include <sys/stat.h>

// for input and output messages NOT SURE
#define INDIM 100
#define OUTDIM 100
#define MAX_INPUT_SIZE 100

int numOfThreads = 0;
/* global variables for input and output files */
struct timeval current_time, final_time;
/* file pointers */
FILE *outputFile = NULL;
/* thread id array */
pthread_t *tid;
/* server socket */
struct sockaddr_un server_addr;
int    sockfd;


// complete the struct
int setSockAddrUn(char *path, struct sockaddr_un *addr) {
    // verificar se a estrutura foi criada
    if (addr == NULL)
        return 0;

    // limpar e adicionar as espicificacoes
    bzero((char *)addr, sizeof(struct sockaddr_un));
    addr->sun_family = AF_UNIX;
    strcpy(addr->sun_path, path);

    // retornar o tamanho
    return SUN_LEN(addr);
}


void *applyCommands(void *ptr){
    /* array and it's head, for lock purposes */
    int lock_array[20];    
    int head;

    /* loop while there are commands to be executed */
    while (1){
        /* struct socket client, input and output buffers */
        struct sockaddr_un client_addr;
        char in_buffer[INDIM], out_buffer[OUTDIM];
        socklen_t client_len;
        int command_len;
        int result;

        /* know the size of the socket structure */
        client_len = sizeof(struct sockaddr_un);

        /* Receive message from the client socket and checking for errors */ 
        command_len = recvfrom(sockfd, in_buffer, sizeof(in_buffer)-1, 0,
            (struct sockaddr *)&client_addr, &client_len);
        if(command_len <= 0){ continue;}

        /* Cautious in case client didn't end up the string, add '\0', */ 
        in_buffer[command_len]='\0';


        char token;
        char name[MAX_INPUT_SIZE], type[MAX_INPUT_SIZE];
        /* ATTENTION: we must verify the arguments given by the socket */
        /* IMPORTANT: type is now a string opposed to character */
        int numTokens = sscanf(in_buffer, "%c %s %s", &token, name, type);


        // UNTOUCHED
        if (numTokens < 2) {
            fprintf(stderr, "Error: invalid command in Queue\n");
            exit(EXIT_FAILURE);
        }

        /* reserved for inummbers */
        int searchResult1, searchResult2;

        switch (token) {
            /* create */
            case 'c':
                /* for directories 'd' and files 'f'*/
                switch (*type) {
                    /* files */
                    case 'f':
                        printf("Create file: %s\n", name);
                        /* -1 (non specific inumber) */
                        result = create(name, T_FILE, lock_array, &head, -1);
                        break;

                    /*directories*/
                    case 'd':
                        printf("Create directory: %s\n", name);
                        /* -1 (non specific inumber) */
                        result = create(name, T_DIRECTORY, lock_array, &head, -1);
                        break;

                    /*errors*/
                    default:
                        fprintf(stderr, "Error: invalid node type\n");
                        result = -1;
                        exit(EXIT_FAILURE);
                }
                break;

            /* move */
            case 'm':
                /* searchResult1 will have the value of current inumber */
                searchResult1 = lookup(name, lock_array, &head);
                unlockArray(lock_array, &head);

                /* searchResult2 should have the value of -1, lookup(fail) */
                searchResult2 = lookup(type, lock_array, &head);
                unlockArray(lock_array, &head);

                /* if there is no such file or directory with that name */
                if(searchResult1 < 0){
                    fprintf(stderr, "Move: %s does not exist.\n", name);
                    result = -1;
                    break;
                }
                /* if the target location's already occupied */
                else if(searchResult2 >= 0){
                    fprintf(stderr, "Move: %s already exists.\n", type);
                    result = -1;
                    break;
                }
                /* if all conditions are verified */
                else{
                    printf("Move: %s to %s.\n", name, type);
                    /* if the operation failed */
                    if(move(name, type, lock_array, &head, searchResult1)==-1){
                        fprintf(stderr, "Error: unable to move %s to %s.\n", name, type);
                        result = -1;
                    }
                    else
                        result = 0;
                    break;
                        
                }

            /* lookup */
            case 'l':
                /* searchResult will have the value of the "child" inumber */
                searchResult1 = lookup(name, lock_array, &head);

                /*if the file/directory exits */
                if (searchResult1 >= 0){
                    printf("Search: %s found\n", name);
                    /* MANDATORY (unlock inode locks) */
                    unlockArray(lock_array, &head);
                    result = 0;
                }

                /* if such file/directory doesn't exist */   
                else{
                    printf("Search: %s not found\n", name);
                    /* MANDATORY (unlock inode locks) */
                    unlockArray(lock_array, &head);
                    result = -1;
                }
                break;
            
            /* delete */
            case 'd':
                printf("Delete: %s\n", name);
                result = delete(name, lock_array, &head, false);
                break;

            /* print */
            case 'p':
                printf("Print to: %s\n", name);
                /* open file output and it's error */
                if( (outputFile = fopen(name, "w")) == NULL){
                    perror("output file: fopen error\n");
                    result = -1;
                }
                else{
                    /* calls out the printing fucntion */
                    block_printfs(outputFile);

                    /* close output file and it's error */
                    if( fclose(outputFile) != 0){
                        perror("output file: fclose error\n");
                        result = -1;
                    }
                    else
                        result = 0;
                }
                break;

            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");
                result = -1;
                exit(EXIT_FAILURE);
            }
        }
        /* making the message that will be passed */
        sprintf(out_buffer, "%d\n", result);
        out_buffer[strlen(out_buffer)] = '\0';

        /* sending the message to the client and check for errors */
        if((sendto(sockfd, out_buffer, OUTDIM, 0, (struct sockaddr *)&client_addr, client_len)) < 0){
            perror("client: sendto error\n");
            exit(EXIT_FAILURE);
        }
    }
    return NULL;
}

/* process the arguments */
void processArguments(int numOfArgs, char *arguments[]){
    
    /* check if 3 arguments are passed */
    if(numOfArgs != 3){
        /* ERROR */
        fprintf(stderr,"Number of arguments is not valid. Need 3 arguments to run.\n");
        exit(EXIT_FAILURE);
    }

    /* get number of threads */
    numOfThreads = atoi(arguments[1]);

    /* check if the number of threads given is valid */
    if(numOfThreads <= 0){
        fprintf(stderr, "Number of threads is invalid. Must be a number > 0.\n");
        exit(EXIT_FAILURE);
    }

}


/* create the number of threads given by the user, and the conditional variables */
void createThreads(){
    int i;
    /* allocate necessary memory */
    tid = (pthread_t*)malloc(sizeof(pthread_t)*numOfThreads);
    
    /* create each thread for apply command */
    for (i = 0; i < numOfThreads; i++){
        /* check for errors */
        if(pthread_create(&tid[i], 0, applyCommands, NULL) != 0){
            fprintf(stderr, "Error while creating thread.\n");
            exit(EXIT_FAILURE);
        }
        else{
            printf("The thread was created successfully.\n");
        }
    }
}


int create_and_bind(char * path){
    socklen_t servlen;
    char *path_aux;

    // scoket creation and error 
    if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0) ) < 0) {
        perror("client: can't open socket");
        exit(EXIT_FAILURE);
    }

    path_aux = path;
    
    /* clear path */
    unlink(path_aux);

    /* fills the socket structure and gives us it's length */
    servlen = setSockAddrUn (path, &server_addr);
  
    /* bind and its specifications */ 
    if (bind(sockfd, (struct sockaddr *) &server_addr, servlen) < 0) {
        perror("client: bind error");
        exit(EXIT_FAILURE);
    }

    return servlen;
}  

/* wait for all threads to finish */
void finishThreads (){
    int i=0;
    for(; i<numOfThreads; i++)
        if( pthread_join( tid[i], NULL ) != 0) {
            printf("Error while joining threads\n");
            exit(EXIT_FAILURE);
        }
    printf("All threads finished successfully.\n");
}


int main(int argc, char* argv[]) {
    /* verify and process the number of arguments */
    processArguments(argc, argv);
    
    /* initialize filesystem */
    init_fs();
    
    /* socket creation and bind */
    create_and_bind(argv[2]);

    /* create threads and conditional variables */
    createThreads();

    /* finish the threads */
    finishThreads();    
    
    /* release all allocated memory */
    destroy_fs();
    free(tid);
    exit(EXIT_SUCCESS);
}