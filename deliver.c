/*
** client.c -- a stream socket client demo
** Taken from Beej's Guide to Network Programming
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define SERVERPORT "4950"    // the port users will be connecting to

int main(int argc, char *argv[])
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;

    if (argc != 3) {
        fprintf(stderr,"usage: client hostname message\n");
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // set to AF_INET to use IPv4
    hints.ai_socktype = SOCK_DGRAM;

    if ((rv = getaddrinfo(argv[1], SERVERPORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and make a socket
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to create socket\n");
        return 2;
    }

    if ((numbytes = sendto(sockfd, argv[2], strlen(argv[2]), 0,
             p->ai_addr, p->ai_addrlen)) == -1) {
        perror("client: sendto");
        exit(1);
    }

    freeaddrinfo(servinfo);

    printf("client: sent %d bytes to %s\n", numbytes, argv[1]);
    close(sockfd);

    char response[4];

    socklen_t addr_len = sizeof p->ai_addr;
    if ((numbytes = recvfrom(sockfd, response, 3 , 0,
        p->ai_addr, &addr_len)) == -1){
        perror("recvfrom");
        exit(1);
    }

    printf("client: packet is %d bytes long\n", numbytes);
    response[numbytes] = '\0';
    printf("client: packet contains \"%s\"\n", response);
    
    char yes[] = "yes";
    if (!strcmp(response, yes)){
        printf("A file transfer can start");
    }
    else{ // no
        exit(1); 
    }
    return 0;
}

