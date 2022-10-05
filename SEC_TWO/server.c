/*
** server.c -- a stream socket server demo
** take from Beej's Guide to Network Programming
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

//#define MYPORT "4950"    // the port users will be connecting to

#define MAX_PACKET_BUFFER_SIZE 1500

void ftp(int sockfd,struct sockaddr_storage* their_addr);
// get sockaddr, IPv4 or IPv6:
struct packet{
	unsigned int total_frag;

	unsigned int frag_no;
	unsigned int size;
	char* filename;
	char filedata[1000];
};

struct frame{
	int frame_kind;
	int sq_no;
	int ack;
};

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc,char* argv[])
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    struct sockaddr_storage their_addr;
    char buf[MAX_PACKET_BUFFER_SIZE];
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // set to AF_INET to use IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "server: failed to bind socket\n");
        return 2;
    }

    freeaddrinfo(servinfo);

    printf("server: waiting to recvfrom...\n");

    addr_len = sizeof their_addr;
    if ((numbytes = recvfrom(sockfd, buf,MAX_PACKET_BUFFER_SIZE-1 , 0,
        (struct sockaddr *)&their_addr, &addr_len)) == -1) {
        perror("recvfrom");
        exit(1);
    }

    buf[numbytes] = '\0';
    printf("server: packet contains \"%s\"\n", buf);
    
    if (!strcmp(buf, "ftp")){
        printf("server: return message \"yes\"\n");
        if ((numbytes = sendto(sockfd, "yes", 3, 0,
            (struct sockaddr *)&their_addr, addr_len)) == -1){
            perror("server: sendto");
            exit(1);
        }

	ftp(sockfd,&their_addr);
    }
    else{
        printf("server: return message \"no\"\n");
        if ((numbytes = sendto(sockfd, "no", 3, 0,
            (struct sockaddr *)&their_addr, addr_len)) == -1){
            perror("server: sendto");
            exit(1);
        }
    }


    close(sockfd);

    return 0;
}

void ftp(int sockfd,struct sockaddr_storage* their_addr){
	struct packet* Packet = (struct packet*) malloc(sizeof(struct packet));
	struct frame* Frame = (struct frame*) malloc(sizeof(struct frame));
	char* packetBuf = (char*) malloc(MAX_PACKET_BUFFER_SIZE);
	memset(Packet, 0, sizeof(struct packet));
	memset(Frame, 0, sizeof(struct frame));

	int numbytes = 0;
	FILE* f = NULL;
	while(1){
		socklen_t addr_len = sizeof(*their_addr);
		if((numbytes = recvfrom(sockfd, packetBuf, MAX_PACKET_BUFFER_SIZE, 0, (struct sockaddr *) &(*their_addr), &addr_len)) == -1){
			perror("recvfrom");
			exit(1);
		}
		// printf("\n\n");
		// printf(packetBuf);
		// printf("\n\n");
		char colonChar = 0,delimitedTokens[128];
		int inc = 0;
		char* bufHead = packetBuf, *bufPrev = packetBuf;
		while(colonChar != 4){
			if(*bufHead == ':'){
				colonChar++;
				if(colonChar == 1){
					memcpy(delimitedTokens, bufPrev,inc);
					delimitedTokens[inc] = '\0';
					Packet->total_frag = atoi(delimitedTokens);
				}
				else if(colonChar == 2){
					memcpy(delimitedTokens, bufPrev,inc);
					delimitedTokens[inc] = '\0';
					Packet->frag_no = atoi(delimitedTokens);
				}
				else if(colonChar == 3){
					memcpy(delimitedTokens, bufPrev,inc);
					delimitedTokens[inc] = '\0';
					Packet->size = atoi(delimitedTokens);
				}
				else{
					Packet->filename = bufPrev;
					*bufHead = '\0';
				}
				bufPrev = bufHead+1;
				inc = 0;
			}
			inc++;
			bufHead++;
		}
		
		// printf("\n\n%d\n%d\n%d\n%s\n\n",Packet->total_frag, Packet->frag_no, Packet->size, Packet->filename);

		if(f == NULL){
			sprintf(delimitedTokens,"./sent/%s",Packet->filename);
			f = fopen(delimitedTokens,"wb");
		}
		fwrite(bufHead,1,Packet->size,f);
	
		Frame->sq_no = 0;
		Frame->ack = Packet->frag_no;

		if ((numbytes = sendto(sockfd,(struct frame*) Frame, sizeof(struct frame), 0, (struct sockaddr *)&(*their_addr), addr_len)) == -1){
		    perror("server: sendto");
		    exit(1);
		}
		
		if(Packet->total_frag == Packet->frag_no) break;
	}
	fclose(f);
}

