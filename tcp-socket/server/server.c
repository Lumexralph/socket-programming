/*
** server.c - a stream socket server
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

#define PORT "5000" // the port users will be connecting to (1024 and below are reserved and needs special privileges from the OS)
#define BACKLOG 10  // number of pending connections the queue can hold

void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;

    errno = saved_errno;
}

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

int main(void) {
    int sockfd, new_fd; // listen for incoming connection on sockfd, handle a connection after accept() on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage client_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN]; // use the largest address space i.e IPv6
    int rv;

    memset(&hints, 0, sizeof hints); // ensure the struct is empty
    hints.ai_family = AF_UNSPEC; // any of IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP sock stream
    hints.ai_flags = AI_PASSIVE; // pick my IP automatically

    // it is time to get the address to connect on the socket
    // NULL is passed as first argument because we have indicated
    // above to fill the IP address by default depending on the host.
    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the linked-list results and bind to any possible first.
    // then, create the stream socket.
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: unable to create socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        // bind the socket to a port to listen from.
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd); // free the socket
            perror("server: unable to bind to PORT");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL) {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    // listen on the binded port and set the allowed pending incoming connections on the queue.
    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("server: waiting for connections...\n");
    char *hello = "Hello from server";

    // main infinite loop to continually accept or take incoming connection from the queue
    while(1) {
        sin_size = sizeof client_addr;
        // new socket file descriptor is returned to handle the current client connection
        new_fd = accept(sockfd, (struct sockaddr *)&client_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(client_addr.ss_family, get_in_addr((struct sockaddr *)&client_addr), s, sizeof s);

        printf("server: got connection from %s\n", s);

        // where the concurrency or the mult-threading happens
        // a child process is created to handle each requests
        // this will avoid the incoming connections being queue or waiting on the queue.
        if (!fork()) {
            close(sockfd); // the current connection session does not need the socket being listened on anymore

            // let's send our data or packet on the new socket created by accept()
            // char data[] = "Hello, Lumex!";
            if (send(new_fd, hello , strlen(hello), 0) == -1) {
                perror("send: ");
            }
                printf("Hello message sent\n"); 

            exit(0);
        }
        //close(new_fd); // parent process doesn't need this.
    }

    return 0;
}