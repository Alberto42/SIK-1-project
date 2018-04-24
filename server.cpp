#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <cstdlib>
#include <ctype.h>

#include "err.h"
#include "server.h"

#define BUFFER_SIZE   2000
#define QUEUE_LENGTH     5

int sock,port;
int msg_sock;
sockaddr_in client_address;
socklen_t client_address_len;
char buffer[2000];
ssize_t len;
ssize_t snd_len;

int main(int argc, char *argv[]) {

    check_args(argc, argv);

    prepare_socket();

    for (;;) {
        wait_for_client();
        char negotiation_message[] = {(char)255,(char)251,(char)1,
                                      (char)255,(char)251,(char)3,
                                      (char)255,(char)252,(char)34,(char)0};

        strcpy(buffer,negotiation_message);
        snd_len = write(msg_sock, buffer, strlen(negotiation_message));
        if (snd_len != strlen(negotiation_message))
            syserr("writing to client socket");
        do {
            len = read(msg_sock, buffer, sizeof(buffer));
            if (len < 0)
                syserr("reading from client socket");
            else {
                printf("read from socket: %zd bytes: %.*s\n", len, (int) len,
                       buffer);
//                snd_len = write(msg_sock, buffer, len);
//                if (snd_len != len)
//                    syserr("writing to client socket");
            }
        } while (len > 0);
        printf("ending connection\n");
        if (close(msg_sock) < 0)
            syserr("close");
    }

    return 0;
}

void wait_for_client() {
    client_address_len = sizeof(client_address);
    // get client connection from the socket
    msg_sock = accept(sock, (struct sockaddr *) &client_address,
                          &client_address_len);
    if (msg_sock < 0)
            syserr("accept");
}

void check_args(int argc, char *const *argv) {
    if (argc != 2) {
        printf("Usage: ./server <port> \n");
        exit(-1);
    }
    port = atoi(argv[1]);
    if (port < 0 || port > 65536) {
        printf("Usage: ./server <port> \n");
        exit(-1);
    }
}

void prepare_socket() {
    int msg_sock;
    struct sockaddr_in server_address;
    socklen_t client_address_len;

    char buffer[BUFFER_SIZE];
    ssize_t len, snd_len;

    sock = socket(PF_INET, SOCK_STREAM, 0); // creating IPv4 TCP socket
    if (sock < 0)
        syserr("socket");
    // after socket() call; we should close(sock) on any execution path;
    // since all execution paths exit immediately, sock would be closed when program terminates

    server_address.sin_family = AF_INET; // IPv4
    server_address.sin_addr.s_addr = htonl(
            INADDR_ANY); // listening on all interfaces
    server_address.sin_port = htons(port); // listening on port PORT_NUM

    // bind the socket to a concrete address
    if (bind(sock, (struct sockaddr *) &server_address,
             sizeof(server_address)) < 0)
        syserr("bind");

    // switch to listening (passive open)
    if (listen(sock, QUEUE_LENGTH) < 0)
        syserr("listen");

    printf("accepting client connections on port %hu\n",
           ntohs(server_address.sin_port));
}
