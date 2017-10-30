#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>

int main(int argc, char *argv[]) {
    int clientSocket, bytes;
    char buffer[1024];
    char fileName[1024];
    char command[1024];

    struct addrinfo hints, *servinfo, *temp; //the server structure
    int status;

    FILE *fp;

    if (argc != 3) {
        fprintf(stderr, "Usage: wrong argument number, exiting... \n");
        exit(1);
    }

    if (strcmp(argv[0], "./deliver") != 0){
        fprintf(stderr, "Usage: did not specify keyword 'deliver', exiting...");
        exit(1);
    }

    //load up address structs

    memset(&hints, 0, sizeof hints);
    
    hints.ai_family = AF_INET; //IPv4
    hints.ai_socktype = SOCK_DGRAM; //datagram socket


    if ((status = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 1;
    }


    // loop through all the results and connect to the first we can
    for (temp = servinfo; temp != NULL; temp = temp->ai_next) {
        if ((clientSocket = socket(temp->ai_family, temp->ai_socktype,
                temp->ai_protocol)) == -1) {
            perror("socket");
            continue;
        }

        break; // if we get here, we must have connected successfully
    }

    if (temp == NULL) {
        // looped off the end of the list with no connection
        fprintf(stderr, "failed to connect\n");
        exit(2);
    }


    //clientSocket = socket(AF_UNIX, SOCK_DGRAM, 0);

    while (1) {
        bzero(buffer, 1024);
        bzero(command, 1024);
        
        fprintf(stderr, "Client: say 'ftp', then a file name: \n");
        scanf("%s", command);
        scanf("%s", buffer);

        //if (strcmp(command, "ftp") != 0) {
        //    fprintf(stderr, "Client: you did not specify ftp! Try again \n");
       //     continue;
       // }
        
        if (buffer == NULL) {
            fprintf(stderr, "Client: invalid name. Try again\n");
            continue;
        }
        

        if (access(buffer, F_OK) == -1) {
            fprintf(stderr, "There is no such file! Exiting... \n");
            exit(0);
        }

        /*Send message to server*/
        sendto(clientSocket, command, sizeof command, 0, temp->ai_addr, temp->ai_addrlen);
        fprintf(stderr, "Client: confirming file name: %s\n", buffer);

        bzero(buffer, 1024);

        /*Receive message from server*/
        bytes = recvfrom(clientSocket, buffer, 1024, 0, NULL, NULL);
  

        if (strcmp(buffer, "yes") == 0) {
            fprintf(stderr, "A file transfer can start.\n");
            
            continue;
        } else {
            fprintf(stderr, "Client: server declined the file transfer. Exiting...\n");
            exit(1);
        }
            

    }

    return 0;
}


