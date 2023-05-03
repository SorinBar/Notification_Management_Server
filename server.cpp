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
#include "utils.h"

#define FULLBUF_SIZE 4000
#define BUF_SIZE 3998
#define TIMEOUT -1

class Server {
private:
    // Listening port
    in_port_t port;
    // Full buffer ([byte]+[buf]) used for TCP Manager
    char fullbuf[FULLBUF_SIZE];
    // Data buffer (fullbuf + 1)
    char *buf;
    // General purpose address
    struct sockaddr_in addr;
    // General purpose adress len
    socklen_t addr_len;
    // UDP Socket
    int udp_sock;
    // TCP listen socket
    int tcp_sock;
    // Poll
    std::vector<pollfd> poll_fds;

public:
    Server(char *port) {
        /* Buf */
        buf = fullbuf + 2;
        /* Port */
        this->port = str_to_port(port);
        /* General Purpose Address */
        addr = {};
        addr_len = sizeof(addr);
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons(this->port);
        /* UDP Socket */
        udp_sock = UDPSockSetup();
        /* TCP Socket */
        tcp_sock = TCPSockSetup();
        /* Poll */
        poll_fds.resize(3);
        poll_fds[0].fd = STDIN_FILENO;
        poll_fds[0].events = POLLIN;
        poll_fds[1].fd = udp_sock;
        poll_fds[1].events = POLLIN;
        poll_fds[2].fd = tcp_sock;
        poll_fds[2].events = POLLIN;
    }

    ~Server() {
        // Close the remaining sockets
        for (size_t i = 1; i < poll_fds.size(); i++) {
            close(poll_fds[i].fd);
        }
    }

    void start() {
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
                        TCP_remove_client(i);
                }
            }
            // Check for events on the UDP socket
            if (poll_fds[1].revents & POLLIN) {
                ssize_t n = recvfrom(udp_sock, buf, BUF_SIZE, 0, (struct sockaddr *)&addr, &addr_len);
                if (n < 0) {
                    std::cerr << "UDP receive failed";
                    continue;
                }
                std::cout << n;
                UDP_input(buf, n, addr);
            }
            // Check for events on the TCP Listen socket
            if (poll_fds[2].revents & POLLIN) {
                // Accept incoming connection
                int new_socket = accept(tcp_sock, (struct sockaddr *)&addr, &addr_len);
                
                if (new_socket < 0) {
                    eerror("TCP client socket creation failed");
                }
                // // New connection message
                // std::cout << "New client <id here> connected from ";
                // std::cout << inet_ntoa(addr.sin_addr) << ":";
                // std::cout << ntohs(addr.sin_port) << std::endl;

                // strcpy(buf, "bine ai veniit!");
                // unite_send(buf, strlen(buf) + 1, new_socket);

                // Add the new file descriptor to the poll vector
                TCP_add_client(new_socket);
            }
        }
    }

private:
    int UDPSockSetup() {
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0) {
            eerror("UDP socket creation failed");
        }
        if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            eerror("UDP socket bind failed");
        }
        return sock;
    }

    int TCPSockSetup() {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            eerror("TCP socket creation failed");
        }
        // Deactivate Nagle algorithm
        int flag = 1;
        if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int)) != 0) {
            eerror("TCP socket nodelay failed");
        }
        if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            eerror("TCP socket bind failed");
        }
        // TODO - Might change 1 to a higher number
        if (listen(sock, 1) < 0) {
            eerror("TCP socket listen failed");
        }
        return sock;
    }

    void TCP_add_client(int fd) {
        pollfd client;
        client.fd = fd;
        client.events = POLLIN;
        poll_fds.push_back(client);
    }

    void TCP_remove_client(int index) {
        // Close the socket
        close(poll_fds[index].fd);
        // Remove the poll entry
        poll_fds.erase(poll_fds.begin() + index);
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

};

int main(int argc, char *argv[]) {
    // disable STDOUT buffering
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    if (argc != 2) {
        eerror("./server <PORT>");
    }
    Server myServer(argv[1]);
    myServer.start();

    return 0;
}