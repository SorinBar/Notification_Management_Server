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
#include "fast_forward.h"
#include "command.h"

#define FLAG_SUCCESS 0
#define FLAG_FAIL 1

class Server {
private:
    // Listening port
    in_port_t port;
    // Full buffer ([2 bytes]+[buf])
    char fullbuf[FULLBUF_SIZE];
    // Data buffer (fullbuf + 2)
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
    // Flag
    int flag;

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
        /* Flag */
        flag = FLAG_SUCCESS;
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
                        poll_remove_client(i);
                    }
                }
            }
            // Check for events on the UDP socket
            if (poll_fds[1].revents & POLLIN) {
                uint16_t len = UDP_recv(udp_sock);
                if (len == 0)
                    continue;
                UDP_forward(len);
            }
            // Check for events on the TCP Listen socket
            if (poll_fds[2].revents & POLLIN) {
                // Accept incoming connection
                int new_socket = TCP_accept();
                TCP_recv(new_socket);
                
                // Add the new file descriptor to the poll vector
                poll_add_client(new_socket);
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

    void poll_add_client(int fd) {
        pollfd client;
        client.fd = fd;
        client.events = POLLIN;
        poll_fds.push_back(client);
    }

    void poll_remove_client(int index) {
        // Close the socket
        close(poll_fds[index].fd);
        // Remove the poll entry
        poll_fds.erase(poll_fds.begin() + index);
    }

    uint16_t UDP_recv(int sock_fd) {
        ssize_t n = recvfrom(sock_fd, buf, BUF_SIZE, 0, (struct sockaddr *)&addr, &addr_len);
        if (n < 0) {
            std::cerr << "UDP receive failed";
        }
        return (uint16_t)n;
    }

    void TCP_send(int sock_fd, uint16_t len) {
        ssize_t n;
        char *fbuf = fullbuf;

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

    void TCP_recv(int sock_fd) {
        ssize_t n;
        uint16_t pack_len, recv_len, diff_len;
        char *left, *right;
        
        left = fullbuf;
        n = recv(sock_fd, fullbuf, FULLBUF_SIZE, 0);
        if (n < 0) {
            eerror("TCP recv error");
        }
        right = fullbuf + n;
        
        while(left != right) {
            // Received one byte
            if (right - left == 1) {
                n = recv(sock_fd, right, FULLBUF_SIZE - 1, 0);
                if (n < 0) {
                    eerror("TCP recv error");
                }
                right += n;
            } else {
                // Received at least 2 bytes
                pack_len = ntohs(*((uint16_t *)left));
                recv_len = right - left;

                // Packet is not fully received
                if (pack_len > recv_len) {
                    n = recv(sock_fd, right, FULLBUF_SIZE - 1, 0);
                    if (n < 0) {
                        eerror("TCP recv error");
                    }
                    right += n;
                } else {
                    // Received enough bytes to process the first packet
                    packageProcess();

                    // Move the unprocessed bytes to the left
                    diff_len = recv_len - pack_len;
                    left += pack_len;
                    memmove(fullbuf, left, diff_len);
                    left = fullbuf;
                    right = left + diff_len;
                }
            }
        }
    }

    // TODO
    void packageProcess() {
        command cmd = cmd_unpack(buf);

        // Debug
        std::cout << (int)(cmd.type) << std::endl;
        std::cout << cmd.id << std::endl;
        std::cout << cmd.topic << std::endl;
        std::cout << (int)(cmd.sf) << std::endl;
        std::cout << (int)(cmd.protocol) << std::endl;



    }
    // TODO
    void UDP_forward(uint16_t len) {
        std::string topic;
        ff_ftr ftr;
        char c;

        // Extract the topic
        c = *(buf + 50);
        *(buf + 50) = '\0';
        topic = buf;
        *(buf + 50) = c;

        // Create the fast forward footer
        ftr.s_addr = addr.sin_addr.s_addr;
        ftr.s_port = addr.sin_port;
        ftr.protocol = FF_PROTOCOL;

        len = ff_pack(buf, len, ftr);

        // Sending
        TCP_send(5, len);
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