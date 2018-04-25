#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <cstdlib>
#include <ctype.h>
#include <string>
#include <vector>
#include <stdexcept>

#include "err.h"
#include "server.h"

using namespace std;

#define BUFFER_SIZE   2000
#define QUEUE_LENGTH     5
#define DEBUG false

int sock,port;
int msg_sock;

vector<string> menu_b_vector = {"Opcja B1","Opcja B2","Wstecz"};
vector<string> menu_vector = {"Opcja A","Opcja B","Koniec"};
Menu menu = Menu(menu_vector,0);

int main(int argc, char *argv[]) {

    check_args(argc, argv);

    initialize_socket();

    for (;;) {
        menu = Menu(menu_vector,0);
        wait_for_client();
        negotiate();
        display(menu,"");

        u_int8_t buffer[BUFFER_SIZE];
        ssize_t len;
        vector<u_int8_t > data_received;
        bool finish = false;
        do {
            len = read(msg_sock, buffer, sizeof(buffer));
            if (len < 0)
                syserr("reading from client socket");
            else {
                buffer[len]=0;

                for(int i=0;i<len;i++)
                    data_received.push_back(buffer[i]);

                finish = onKeyPressed(data_received);
                if (DEBUG) {
                    fprintf(stderr, "read from socket: %zd bytes: ", len);
                    for (int i = 0; i < len; i++) {
                        fprintf(stderr, "%x ", buffer[i] & 0xff);
                    }
                    fprintf(stderr, "\n");
                }
            }
        } while (!finish);

        printf("ending connection\n");
        if (close(msg_sock) < 0)
            syserr("close");
    }
}

void display(const Menu& menu, string text) {
    string buffer;
    const char *const move_cursor_to_left_upper_corner = "\033[2J\033[0;0H";
    buffer+= move_cursor_to_left_upper_corner;

    for(int i=0;i<= menu.max_field;i++) {
        buffer+="\r";
        if (menu.current_field == i) {
            buffer+="  *  ";
        } else {
            buffer+="     ";
        }
        buffer+=menu.fields[i] + "\n";
    }

    buffer+="\n" + text + "\n";
    const char *c_buffer = buffer.c_str();
    ssize_t snd_len = write(msg_sock, c_buffer, strlen(c_buffer));
    if (snd_len != strlen(c_buffer))
        syserr("writing to client socket");
}

vector<u_int8_t > up = {0x1b, 0x5b, 0x41};
vector<u_int8_t > down = {0x1b, 0x5b, 0x42};
vector<u_int8_t > enter = {0xd,0};

bool is_prefix(vector<u_int8_t> word, vector<u_int8_t> prefix) {
    if (prefix.size() > word.size())
        return false;
    for(unsigned i=0;i<prefix.size();i++) {
        if (prefix[i] != word[i])
            return false;
    }
    return true;
}

bool onKeyPressed(vector<u_int8_t>& key) {

    unsigned long previous_size = 100000;
    while(key.size() < previous_size ) {
        previous_size = key.size();
        if (is_prefix(key, up)) {
            key.erase(key.begin(), key.begin() + up.size());
            menu.current_field = max(menu.min_field, menu.current_field - 1);
            display(menu, "");
        } else if (is_prefix(key, down)) {
            key.erase(key.begin(), key.begin() + down.size());
            menu.current_field = min(menu.max_field, menu.current_field + 1);
            display(menu, "");
        } else if (is_prefix(key, enter)) {

            key.erase(key.begin(), key.begin() + enter.size());
            if (menu.type == 0) {
                switch (menu.current_field) {
                    case 0:
                        display(menu, "A");
                        break;
                    case 1:
                        menu = Menu(menu_b_vector, 1);
                        display(menu, "");
                        break;
                    case 2:
                        return true;
                }
            } else {
                switch (menu.current_field) {
                    case 0:
                        display(menu, "B1");
                        break;
                    case 1:
                        display(menu, "B2");
                        break;
                    case 2:
                        menu = Menu(menu_vector, 0);
                        display(menu, "");
                        break;
                }
            }
        } else if (key.size() >= 3) {
            if (DEBUG)
                fprintf(stderr, "%s", "Wrong data received\n");
            key.clear();
        }
    }
    return false;
}
void negotiate() {
    u_int8_t negotiation_message[] = {255, 251, 1,
                                      255, 251, 3,
                                      255, 252, 34, 0};
    const int length = 10;
    ssize_t snd_len = write(msg_sock, negotiation_message, length);
    if (snd_len != length)
            syserr("writing to client socket");
}

void wait_for_client() {
    sockaddr_in client_address;
    socklen_t client_address_len;

    client_address_len = sizeof(client_address);

    // get client connection from the socket
    msg_sock = accept(sock, (struct sockaddr *) &client_address,
                          &client_address_len);
    printf("Client has connected\n");
    if (msg_sock < 0)
            syserr("accept");
}

bool is_number(const std::string& s)
{
    std::string::const_iterator it = s.begin();
    while (it != s.end() && std::isdigit(*it)) ++it;
    return !s.empty() && it == s.end();
}

void wrong_args() {
    printf("Usage: ./server <port> \n");
    exit(1);
}

void check_args(int argc, char *const *argv) {
    if (argc != 2) {
        wrong_args();
        return;
    }
    if (!is_number(string(argv[1]))) {
        wrong_args();
        return;
    }
    port = atoi(argv[1]);
    if (port <= 0 || port > 65536) {
        wrong_args();
        return;
    }
}

void initialize_socket() {
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
