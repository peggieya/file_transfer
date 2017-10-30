#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>

int main(int argc, char* argv[]) {

    int status, u_socket, bytes;
    struct addrinfo hints;
    struct addrinfo *servinfo, *temp; //will point to the results
    struct sockaddr_storage client_address; //client address

    if (strcmp(argv[0], "./server") != 0){
        fprintf(stderr, "Usage: did not specify keyword 'server', terminating...");
        exit(1);
   }
    
    int port_num = atoi(argv[1]);
    socklen_t length;

    char buffer[1024];

    memset(&hints, 0, sizeof hints); // make sure the struct is empty

    hints.ai_family = AF_INET; // IPv4 only
    hints.ai_socktype = SOCK_DGRAM; // TCP datagram sockets

    //calling getaddrinfo
    if ((status = getaddrinfo("127.0.0.1", argv[1], &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        exit(1);
    }

    // loop through all the results and connect to the first we can
    for (temp = servinfo; temp != NULL; temp = temp->ai_next) {
        if ((u_socket = socket(temp->ai_family, temp->ai_socktype,
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


    bind(u_socket, temp->ai_addr, temp->ai_addrlen);

    length = sizeof client_address;

    fprintf(stderr, "Server: listening to port %d. \n", port_num);
    while (1) {
        /* Try to receive any incoming UDP datagram. Address and port of 
        requesting client will be stored on client_address variable */
        bytes = recvfrom(u_socket, buffer, 1024, 0, &client_address, &length);      

        fprintf(stderr, "Server: received '%s' \n", buffer); 
        
        if (strcmp(buffer, "ftp") == 0) {
            bzero(buffer, 1024);
            strcpy(buffer, "yes");
        }
        else {
            bzero(buffer, 1024);
            strcpy(buffer, "no");
        }

        /*Send uppercase message back to client, using serverStorage as the address*/
        sendto(u_socket, buffer, bytes, 1024, &client_address, length);

    }

    freeaddrinfo(servinfo); // free the linked-list

    return 0;
}


/******************************************LAB 1 CONTENT ABOVE*****************************************/
