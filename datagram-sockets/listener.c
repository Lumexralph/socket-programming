/*
** listener sits on a machine waiting for an incoming packet on port 4950.
talker sends a packet to that port, on the specified machine, that contains whatever the user enters on the command line.

listener.c -- a datagram socket server
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

#define MYPORT "5000" // port to listen for incoming packets/traffic
#define MAXBUFLEN 100 // the size of the buffer to hold the data

// get sockaddr, IPv4 or IPv6
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) { // IPv4
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

     return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void) {
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    struct sockaddr_storage their_addr;
    char buf[MAXBUFLEN];
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // set the IP dynamically
    hints.ai_socktype = SOCK_DGRAM; // datagram sockets - UDP
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, MYPORT, &hints, &servinfo)) != 0) { // if there was an error from setting the socket structs
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    }

    // loop through all the results and bind to the first node in the list we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("listener: socket");
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd); // free the computer resources used to create the socket descriptor(file descriptor)
            perror("listener: bind");
            continue;
        }
        // if we were able to successfully bind to a node, exit
        break;
    }

    if (p == NULL) {
        fprintf(stderr, "listener: failed to bind scoket\n");
        return 2;
    }
    // free the struct allocated in memory
    freeaddrinfo(servinfo);

    printf("listener: waiting to recvfrom...\n");

    addr_len = sizeof their_addr;
    if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN - 1, 0, (struct sockaddr *)&their_addr, &addr_len)) == -1) {
        perror("recvfrom");
        exit(1);
    }

    printf("listener: got packet from %s\n", inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s));
    printf("listener: packet is %d bytes long\n", numbytes);
    buf[numbytes] = '\0';
    printf("listener: packet contains \"%s\"\n", buf);

    close(sockfd);

    return 0;
}

