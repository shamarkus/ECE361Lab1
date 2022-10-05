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

#define MAX_PACKET_BUFFER_SIZE 1500
void ftp(int sockfd,struct addrinfo *p,char* filename);

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

    if ((rv = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0) {
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

    char cmd[128];
    char filename[128];
	printf("Enter \"ftp\" followed by filename: ");
    scanf("%s %s",cmd,filename);

    if(!strcmp("ftp",cmd)){
	if(access(filename, F_OK) == 0){
		//file does exist
	    if ((numbytes = sendto(sockfd, "ftp", 3, 0, p->ai_addr, p->ai_addrlen)) == -1) {
		perror("client: sendto");
		exit(1);
	    }

	    char response[4];
	    socklen_t addr_len = sizeof p->ai_addr;
	    if ((numbytes = recvfrom(sockfd, response, 3 , 0,
		p->ai_addr, &addr_len)) == -1){
		perror("recvfrom");
		exit(1);
	    }
	    response[numbytes] = '\0';
	    if (!strcmp(response, "yes")){
		printf("A file transfer can start\n");
		ftp(sockfd,p,filename);
	    }
	    else{ // no
		exit(1); 
	    }
	}
	else{
		//file doesnt exist
		printf("File doesnt exist\n");
		exit(1);
	}
    }
    else{
	    perror("command: invalid");
	    exit(1);
    }

    freeaddrinfo(servinfo);
    close(sockfd);
    return 0;
}

void ftp(int sockfd,struct addrinfo *p,char* filename){
	struct packet* Packet = (struct packet*) malloc(sizeof(struct packet));
	struct frame* Frame = (struct frame*) malloc(sizeof(struct frame));
	char* packetBuf = (char*) malloc(MAX_PACKET_BUFFER_SIZE);
	memset(Packet, 0, sizeof(struct packet));
	memset(Frame, 0, sizeof(struct frame));

	FILE* f = fopen(filename,"rb");
	fseek(f, 0L, SEEK_END);
	int sz = ftell(f);
	rewind(f);
	Packet->total_frag = ((int) (sz - 1)/1000) + 1;
	Packet->filename = filename;
	Packet->frag_no = 1;

	int numbytes = 0,ack_recv = 1;

	while(1){
		if(ack_recv == 1){
			if(!feof(f)){
				fread(Packet->filedata,1,1000,f);
				Packet->size = ((sz - 1000*(Packet->frag_no-1)) >= 1000) ? 1000 : (sz - 1000*(Packet->frag_no-1));
				sprintf(packetBuf,"%d:%d:%d:%s:", Packet->total_frag, Packet->frag_no, Packet->size, Packet->filename);
				int headerSize = strlen(packetBuf);
				memcpy(packetBuf+strlen(packetBuf),Packet->filedata,1000);

				if ((numbytes = sendto(sockfd, packetBuf, Packet->size + headerSize, 0, p->ai_addr, p->ai_addrlen)) == -1) {
					perror("client: sendto");
					exit(1);
				}
				printf("Sent Packet #%d\n",Packet->frag_no);

				memset(Packet->filedata,0,1000);
			}
			else break;
		}

		socklen_t addr_len = sizeof p->ai_addr;
		if((numbytes = recvfrom(sockfd, Frame, sizeof(struct frame), 0, p->ai_addr, &addr_len)) == -1){
			perror("recvfrom");
			exit(1);
		}

		if(numbytes > 0 && Frame->sq_no == 0 && Frame->ack == Packet->frag_no){
			printf("ACK Received for Packet #%d\n",Packet->frag_no);
			ack_recv = 1;
		}
		else{
			printf("ACK NOT Received for Packet #%d\n",Packet->frag_no);
			ack_recv = 0;
		}

		Packet->frag_no++;
	}
}

