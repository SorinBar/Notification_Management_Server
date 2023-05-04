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

class Subscriber {
private:
     // Full buffer ([2 bytes]+[buf])
    char fullbuf[FULLBUF_SIZE];
    // Data buffer (fullbuf + 2)
    char *buf;
    // Data buffer len
    uint16_t buf_len;
    // Client ID
    std::string id;
    // Server IP
    in_addr_t server_ip;
    // Server PORT
    in_port_t server_port;
    // General purpose address
    struct sockaddr_in addr;
    // Adress lenght
    socklen_t addr_len;
    // Connected TCP socket
    int tcp_sock;
    // Poll
    std::vector<pollfd> poll_fds;


public:
    Subscriber(char *id, char *ip, char *port) {
        this->id = id;
        this->server_ip = inet_addr(ip);
        this->server_port = str_to_port(port);
        if (server_ip == INADDR_NONE) {
            eerror("<IP> ~ [0, 255].[0, 255].[0, 255].[0, 255]");
        }
        buf = fullbuf + sizeof(uint16_t);
        /* General Purpose Address */
        addr = {};
        addr_len = sizeof(addr);
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = server_ip;
        addr.sin_port = htons(server_port);
        /* TCP Socket*/
        tcp_sock = TCPSockSetup();
        /* Poll */
        poll_fds.resize(2);
        poll_fds[0].fd = STDIN_FILENO;
        poll_fds[0].events = POLLIN;
        poll_fds[1].fd = tcp_sock;
        poll_fds[1].events = POLLIN;
    }

    ~Subscriber() {
        // Close TCP socket
        close(tcp_sock);
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
                if (strcmp(buf, "exit") == 0) {
                    send(tcp_sock, buf, 5, 0);
                    break;
                }
            }
            // Check for events on the TCP Listen socket
            if (poll_fds[1].revents & POLLIN) {
                TCP_recv(tcp_sock);
            }
        }
    }

private:
    int TCPSockSetup() {
        /* TCP Listen Socket */
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            eerror("TCP socket creation failed");
        }
        // Deactivate Nagle algorithm
        int flag = 1;
        if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int)) != 0) {
            eerror("TCP socket nodelay failed");
        }
        // Connect to the remote socket
        if (connect(sock, (struct sockaddr *)&addr, addr_len) < 0) {
            eerror("TCP could not connect to remote socket");
        }
        return sock;
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
            std::cout << "\n";
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

                std::cout << "Packet len: " << pack_len << std::endl;
                std::cout << "Recv len: " << recv_len << std::endl;

                // Packet is not fully received
                if (pack_len > recv_len) {
                    n = recv(sock_fd, right, FULLBUF_SIZE - 1, 0);
                    if (n < 0) {
                        eerror("TCP recv error");
                    }
                    right += n;
                } else {
                    // Received enough bytes to process the first packet
                    buf_len = pack_len - sizeof(uint16_t);
                    
                    // TODO
                    recv_tester();

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
    void recv_tester() {
        std::cout << "Message len: " << buf_len << std::endl;
        std::cout << "Message: " << buf << std::endl;
    }
};

int main(int argc, char *argv[]) {
    // disable STDOUT buffering
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    /* Setup */
    if (argc != 4) {
        eerror("./subscriber <ID> <IP> <PORT>");
    }
    Subscriber mySubscriber(argv[1], argv[2], argv[3]);
    mySubscriber.start();

    return 0;
}
