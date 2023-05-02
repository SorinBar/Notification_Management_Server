#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <poll.h>
#include <string>
#include <vector>
#include <netinet/tcp.h>
#include "unite.h"

#define FULLBUF_SIZE 2000
#define BUF_SIZE 1998
#define TIMEOUT -1

// Print message to STDERR and exit
void eerror(std::string message);
// Convert string to in_port_t/uint16_t
in_port_t str_to_port(std::string port_str);
// Process UDP buffer and send notification
void UDP_input(char* buf, ssize_t len, struct sockaddr_in &addr);
// Add TCP client socket to poll
void TCP_add_client(std::vector<pollfd> &poll_fds, int client_fd);
// Remove TCP client socket from poll
void TCP_remove_client(std::vector<pollfd> &poll_fds, int index);

int main(int argc, char *argv[]) {
    /* Setup */
    char fullbuf[FULLBUF_SIZE];
    char *buf = fullbuf + sizeof(uint16_t);
    if (argc != 2) {
        eerror("./server <PORT>");
    }
    in_port_t port = str_to_port(argv[1]);
    // disable STDOUT buffering
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);


    /* General Purpose Address */
    struct sockaddr_in addr = {};
    socklen_t addr_len = sizeof(addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);


    /* UDP Socket */
    int udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_sock < 0) {
        eerror("UDP socket creation failed");
    }
    // Bind
    if (bind(udp_sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        eerror("UDP socket bind failed");
    }


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
    // Bind
	if (bind(tcp_sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        eerror("TCP socket bind failed");
    }
    // Listen
    // TODO - Might change 1 to a higher number
	if (listen(tcp_sock, 1) < 0) {
        eerror("TCP socket listen failed");
    }

    /* Poll */
    std::vector<pollfd> poll_fds(3);
    poll_fds[0].fd = STDIN_FILENO;
    poll_fds[0].events = POLLIN;
    poll_fds[1].fd = udp_sock;
    poll_fds[1].events = POLLIN;
    poll_fds[2].fd = tcp_sock;
    poll_fds[2].events = POLLIN;


    /* Wait for events */
    while (true) {
        if (poll(poll_fds.data(), poll_fds.size(), TIMEOUT) == -1) {
            eerror("Poll failed");
        }
        // Check for events on STDIN
        if (poll_fds[0].revents & POLLIN) {
            std::cin.getline(buf, BUF_SIZE);
            if (strcmp(buf, "exit") == 0)
                break;
        }
        // Check messages from clients
        for (size_t i = 3; i < poll_fds.size(); i++) {
            if (poll_fds[i].revents & POLLIN) {
                ssize_t n = recv(poll_fds[i].fd, buf, BUF_SIZE, 0);
                std::cout << "Recv bytes: " << n << std::endl;
                std::cout << "Data: " << buf << std::endl;
                
                // TCP_input
                
                // Client disconnected
                if (strcmp(buf, "exit") == 0)
                    TCP_remove_client(poll_fds, i);
            }
        }
        // Check for events on the UDP socket
        if (poll_fds[1].revents & POLLIN) {
            ssize_t n = recvfrom(udp_sock, buf, BUF_SIZE, 0, (struct sockaddr *)&addr, &addr_len);
            if (n < 0) {
                std::cerr << "UDP receive failed";
                continue;
            }
            UDP_input(buf, n, addr);
        }
        // Check for events on the TCP Listen socket
        if (poll_fds[2].revents & POLLIN) {
            // Accept incoming connection
            int new_socket = accept(tcp_sock, (struct sockaddr *)&addr, &addr_len);
            
            if (new_socket < 0) {
                eerror("TCP client socket creation failed");
            }
            // New connection message
            std::cout << "New client <id here> connected from ";
            std::cout << inet_ntoa(addr.sin_addr) << ":";
            std::cout << ntohs(addr.sin_port) << std::endl;

            strcpy(buf, "bine ai venit fram");
            unite_send(buf, strlen(buf), new_socket);

            // Add the new file descriptor to the poll vector
            TCP_add_client(poll_fds, new_socket);
        }
    }
    // Close the remaining sockets
    for (size_t i = 1; i < poll_fds.size(); i++) {
        close(poll_fds[i].fd);
    }

    return 0;
}

void eerror(std::string message) {
    std::cerr << message << std::endl;
    exit(EXIT_FAILURE);
}

in_port_t str_to_port(std::string port_str) {
    int port;
    try {
        port = std::stoi(port_str);
    } catch (const std::exception& e) {
        eerror("<PORT> ~ [0, 65535]");
    }
    if (port < 0 || UINT16_MAX < port) {
        eerror("<PORT> ~ [0, 65535]");
    }

    return (in_port_t)port;
}

void UDP_input(char* buf, ssize_t len, struct sockaddr_in &addr) {
    std::cout << inet_ntoa(addr.sin_addr) << std::endl;
    std::cout << ntohs(addr.sin_port) << std::endl;
    std::cout << len << std::endl;

    char c;
    std::string topic;

    c = *(buf + 50);
    *(buf + 50) = '\0';
    topic = buf;
    *(buf + 50) = c;

    std::cout << topic << std::endl;

    // rem warning
    len++;
}

void TCP_add_client(std::vector<pollfd> &poll_fds, int client_fd) {
    pollfd client;
    client.fd = client_fd;
    client.events = POLLIN;
    poll_fds.push_back(client);
}

void TCP_remove_client(std::vector<pollfd> &poll_fds, int index) {
    // Close the socket
    close(poll_fds[index].fd);
    // Remove the poll entry
    poll_fds.erase(poll_fds.begin() + index);
}