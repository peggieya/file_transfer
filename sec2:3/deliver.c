#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <math.h>

#define TIMER
#define DEBUG
#define MAXDATALEN 1000

struct packet {
    unsigned int total_frag;
    unsigned int frag_no;
    unsigned int size;
    char* filename;
    char filedata[1000];
};

int main(int argc, char *argv[]) {
    int clientSocket, bytes;
    char buffer1[1024];
    char filename[1024];
    char command[1024];
    int err;

    struct addrinfo hints, *servinfo, *temp; //the server structure
    int status;

    if (argc != 3) {
        fprintf(stderr, "Usage: wrong argument number, exiting... \n");
        exit(1);
    }

    if (strcmp(argv[0], "./deliver") != 0) {
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
    bzero(filename, 1024);
    bzero(command, 1024);

    fprintf(stderr, "Client: say 'ftp', then a file name: \n");
    scanf("%s", command);
    scanf("%s", filename);

    //if (strcmp(command, "ftp") != 0) {
    //    fprintf(stderr, "Client: you did not specify ftp! Try again \n");
    //     continue;
    // }

    if (access(filename, F_OK) == -1) {
        fprintf(stderr, "There is no such file! Exiting... \n");
        exit(0);
    }
    
    /*set the timer to measure the round trip time*/
    int msec = 0, tigger = 10;
    clock_t before = clock();

    /*Send message to server*/
    sendto(clientSocket, command, sizeof command, 0, temp->ai_addr, temp->ai_addrlen);
    fprintf(stderr, "Client: confirming file name: %s\n", filename);

    bzero(buffer1, 1024);

    /*Receive message from server*/
    recvfrom(clientSocket, buffer1, 1024, 0, NULL, NULL);

    clock_t difference = clock()-before;
    msec = difference * 1000000 / CLOCKS_PER_SEC;
    printf("Time taken %d microseconds for the round trip\n", msec);

    if (strcmp(buffer1, "yes") != 0) {
        fprintf(stderr, "Client: server declined the file transfer. Exiting...\n");
        exit(1);
    }

    fprintf(stderr, "A file transfer can start.\n");


    //open the file
    FILE * file = fopen(filename, "r");
    if (!file) perror("cannot open the file");

    //get the file size in bytes
    fseek(file, 0L, SEEK_END);
    long int file_size = ftell(file);
    fseek(file, 0L, SEEK_SET); //seek backk

    //total packet number
    int total_frag = ceil(((double) file_size) / MAXDATALEN);
    int last_packet_size = file_size % MAXDATALEN;

    //initialize packet info
    char dummy[200]; //used to count packet header bytes
    bzero(dummy, sizeof (char)*200);
    int frag_no = 1; //sequence number, starting from 1
    int data_size;
    if (total_frag > 1) data_size = MAXDATALEN;
    else data_size = last_packet_size;

    //create the packet to be sent
    int offset_header = sprintf(dummy, "%d:%d:%d:%s:", total_frag,
            frag_no, data_size, filename);
    int max_header = offset_header;
    int buffer_size = max_header + MAXDATALEN;
    char* buffer = malloc(sizeof (char)*buffer_size);
    bzero(buffer, buffer_size);
    //pump the header lines
    sprintf(buffer, "%d:%d:%d:%s:", total_frag,
            frag_no, data_size, filename);

    //for debugging
    int total_bytes;


    //read the file
    while (!feof(file)) {
        //read data from file
        int bytes = fread((buffer + offset_header), sizeof (char), MAXDATALEN, file);
        total_bytes += bytes;

        //finanlly send the packet
        int len = buffer_size * sizeof (char);
        err = sendto(clientSocket, buffer, len, 0,
                temp->ai_addr, temp->ai_addrlen);
        if (err == -1) {
            perror("sendto2");
            return 1;
        }

        //wait for ACK/NACK from server
        int done = 1;
        struct sockaddr new_source;
        socklen_t addrlen = sizeof (new_source);
        char ack[5] = {'\0'};
        err = recvfrom(clientSocket, ack, 5, 0, &new_source, &addrlen);
        int errsv = errno;
        if (err < 0) {
            if (errsv == EAGAIN)//timeout has occured
            {
                printf("timeout: ACK/NACK\n");
                //resend the packet
                total_bytes -= bytes;
                //seek back the file position
                fseek(file, -bytes, SEEK_CUR);
                continue;
            } else {
                perror("recvfrom--waiting for ACK/NACK");
                exit(1);
            }
        }

check_ack:
        if (strcmp(ack, "NACK") == 0) {
            //resend the current packet
            total_bytes -= bytes;
            //seek back the file position
            fseek(file, -bytes, SEEK_CUR);
            printf("NACK! Resending the last packet\n");
            continue;
        } else if (strcmp(ack, "ACK") == 0)
            printf("ACK\n");
            goto check_ack;



        /* update packet header and buffer size */
        frag_no += 1;
        if (frag_no == total_frag) data_size = last_packet_size;

        offset_header = sprintf(buffer, "%d:%d:%d:%s:", total_frag,
                frag_no, data_size, filename);
        //in case the size of header increases,
        //expand the buffer.
        //this may never be executed -- can be removed by giving
        //a large inital buffer size.
        if (offset_header > max_header) {
#ifdef DEBUG
            printf("Buffer size is expanded from %d", buffer_size);
#endif
            max_header = offset_header;
            free(buffer);
            buffer_size = max_header + MAXDATALEN;
            buffer = malloc(sizeof (char)*buffer_size);
            bzero(buffer, buffer_size);
            //pump the header lines
            sprintf(buffer, "%d:%d:%d:%s:", total_frag,
                    frag_no, data_size, filename);

#ifdef DEBUG
            printf("to %d\n", buffer_size);
#endif
        }
    }

    printf("Report by client: original file size is: "
            "%ld\n%d are actually sent.\n", file_size, total_bytes);

    close(clientSocket);
    fclose(file);
    free(buffer);

    return 0;
}


