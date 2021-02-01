/*
** client.c - a stream socket client.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT "5000" // port the client will be connecting to

// the maximum size of buffer to store read data we can get at once,
// you can read up to 1KB at once from the socket stream response packet
#define MAXDATASIZE 1024

// get the sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{ // sa a pointer to read the address into
    if (sa->sa_family == AF_INET)
    { // IPv4
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    // IPv6
    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int main(int argc, char *argv[]) {
    int sockfd, numbytes;
    char buf[MAXDATASIZE]; // buffer to store read data from stream socket
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];

    if (argc != 2) {
        fprintf(stderr, "usage: client hostname\n");
        exit(1);
    }

    memset(&hints, 0, sizeof hints); // ensure the struct is empty
    hints.ai_family = AF_UNSPEC; // use any of IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP sock stream

     // it is time to get the address to connect on the socket
    if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the linked-list result and connect to the possible first
    for (p = servinfo; p != NULL; p = p->ai_next) {
        // create the socket to connect on
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        // since we are trying to connect to a remote host, we don't need to bind()
        // NB: If you don't run the server before you connect(), 
        // it will return "Connection refused"
        // This means that the server is not reachable or possibly down.
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd); // free the created socket
            perror("client: connect");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)&p->ai_addr), s, sizeof s);
    printf("client: connecting to %s\n", s);

    freeaddrinfo(servinfo); // we are done with this structure, so deallocate it.

    // receive the packet from the remote stream
    if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0) == -1)) {
        perror("recv: ");
        exit(1);
    }

    // after data has been read into the buffer, add a null indicator
    // to designate the end or boundary of the data.
    buf[numbytes] = '\0';

    // display the received data
    printf("client: received '%d'\n", numbytes);
    printf("client: %s\n", buf);

    close(sockfd);

    return 0;
}
