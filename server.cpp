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

class Server {
private:
    // Listening port
    in_port_t port;
    // Full buffer ([2 bytes]+[buf])
    char fullbuf[FULLBUF_SIZE];
    // Data buffer (fullbuf + 2)
    char *buf;
    // Data buffer len
    uint16_t buf_len;
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
        buf = fullbuf + sizeof(uint16_t);
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
                    if (strcmp(buf, "exit") == 0) {
                        TCP_remove_client(i);
                        std::cout << "disconnected\n";
                    }
                }
            }
            // Check for events on the UDP socket
            if (poll_fds[1].revents & POLLIN) {
                UDP_recv(udp_sock);
                if (buf_len == 0)
                    continue;
                UDP_process();
            }
            // Check for events on the TCP Listen socket
            if (poll_fds[2].revents & POLLIN) {
                // Accept incoming connection
                int new_socket = TCP_accept();

                // // New connection message
                // std::cout << "New client <id here> connected from ";
                // std::cout << inet_ntoa(addr.sin_addr) << ":";
                // std::cout << ntohs(addr.sin_port) << std::endl;

                // strcpy(buf, "bine ai veniit!");
                // unite_send(buf, strlen(buf) + 1, new_socket);
                char message1[100] = "Welcome1!";
                char message2[100] = "Welcome2!";
                strcpy(buf, message1);
                buf_len = strlen(message1) + 1;
                TCP_send(new_socket);

                strcpy(buf, message2);
                buf_len = strlen(message2) + 1;
                TCP_send(new_socket);

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
        if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int)) != 0) {
            eerror("TCP socket nodelay failed");
        }
        // Make socket reusable
        if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int)) < 0)
            eerror("TCO socket reusable failed");

        if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            eerror("TCP socket bind failed");
        }
        // TODO - Might change 1 to a higher number
        if (listen(sock, 1) < 0) {
            eerror("TCP socket listen failed");
        }
        return sock;
    }

    int TCP_accept() {
        // Accept incoming connection
        int new_socket = accept(tcp_sock, (struct sockaddr *)&addr, &addr_len);
        if (new_socket < 0) {
            eerror("TCP client socket creation failed");
        }
        // Deactivate Nagle algorithm
        int flag = 1;
        if (setsockopt(new_socket, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int)) != 0) {
            eerror("TCP socket nodelay failed");
        }
        return new_socket;
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

    void UDP_recv(int sock_fd) {
        ssize_t n = recvfrom(sock_fd, buf, BUF_SIZE, 0, (struct sockaddr *)&addr, &addr_len);
        if (n < 0) {
            std::cerr << "UDP receive failed";
        }
        buf_len = (uint16_t)n;
    }

    void TCP_send(int sock_fd) {
        ssize_t n;
        char *fbuf = fullbuf;
        uint16_t len = buf_len;

        len +=  sizeof(uint16_t);
        *((uint16_t *)fbuf) = htons(len);
        while(len != 0) {
            n = send(sock_fd, fbuf, (size_t)len, 0);
            if (n < 0) {
                eerror("TCP send error");
            }
            fbuf += n;
            len -= (uint16_t)n;
        }
    }

    // TODO
    void UDP_process() {
        std::cout << inet_ntoa(addr.sin_addr) << ":";
        std::cout << ntohs(addr.sin_port) << std::endl;
        std::cout << buf_len << std::endl;

        char c;
        std::string topic;

        c = *(buf + 50);
        *(buf + 50) = '\0';
        topic = buf;
        *(buf + 50) = c;

        std::cout << topic << std::endl;
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