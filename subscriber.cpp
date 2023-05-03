#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <poll.h>
#include <string>
#include <vector>
#include <netinet/tcp.h>
#include "utils.h"

#define BUF_SIZE 2000
#define TIMEOUT -1

int main(int argc, char *argv[]) {
    /* Setup */
    char buf[BUF_SIZE];
    if (argc != 4) {
        eerror("./subscriber <ID> <IP> <PORT>");
    }
    std::string id = argv[1];
    in_addr_t server_ip = inet_addr(argv[2]);
    in_port_t server_port = str_to_port(argv[3]);
    if (server_ip == INADDR_NONE) {
        eerror("<IP> ~ [0, 255].[0, 255].[0, 255].[0, 255]");
    }
    
    // disable STDOUT buffering
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    /* General Purpose Address */
    struct sockaddr_in addr = {};
    socklen_t addr_len = sizeof(addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = server_ip;
    addr.sin_port = htons(server_port);


    /* TCP Listen Socket */
    int tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_sock < 0) {
        eerror("TCP socket creation failed");
    }
    // Deactivate Nagle algorithm
    int flag = 1;
    if (setsockopt(tcp_sock, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int)) != 0) {
        eerror("TCP socket nodelay failed");
    }
    // Connect to the remote socket
    if (connect(tcp_sock, (struct sockaddr *)&addr, addr_len) < 0) {
        eerror("TCP could not connect to remote socket");
    }

    /* Poll */
    std::vector<pollfd> poll_fds(2);
    poll_fds[0].fd = STDIN_FILENO;
    poll_fds[0].events = POLLIN;
    poll_fds[1].fd = tcp_sock;
    poll_fds[1].events = POLLIN;


    /* Wait for events */
    while (true) {
        if (poll(poll_fds.data(), poll_fds.size(), TIMEOUT) == -1) {
            eerror("Poll failed");
        }
        // Check for events on STDIN
        if (poll_fds[0].revents & POLLIN) {
            std::cin.getline(buf, BUF_SIZE);
            if (strcmp(buf, "exit") == 0) {
                send(tcp_sock, buf, 5, 0);
                break;
            }
        }
        // Check for events on the TCP Listen socket
        if (poll_fds[1].revents & POLLIN) {
            ssize_t n = recv(poll_fds[1].fd, buf, sizeof(buf), 0);
            std::cout << "Recv bytes: " << n << std::endl;
            uint16_t len = *((uint16_t *)buf);
            len = ntohs(len);
            std::cout << "Data-len: " << len << std::endl;
            std::cout << "Real-data-len: " << strlen(buf + 2) + 1 << std::endl;
            std::cout << "Data: " << buf + 2 << std::endl;
        }
    }
    // Close TCP socket
    close(tcp_sock);

    return 0;
}
