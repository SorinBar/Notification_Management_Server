#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <poll.h>
#include <string>
#include <vector>
#include <netinet/tcp.h>
#include <cmath>
#include <iomanip>
#include "utils.h"
#include "fast_forward.h"
#include "command.h"

#define INT 0
#define SHORT_REAL 1
#define FLOAT 2
#define STRING 3

#define FLAG_DEFAULT 1
#define FLAG_EXIT 0
#define BAD_INPUT -1

class Subscriber {
private:
    // Full buffer ([2 bytes]+[buf])
    char fullbuf[FULLBUF_SIZE];
    // Data buffer (fullbuf + 2)
    char *buf;
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
    // CMD Structure
    command cmd;
    // Flag
    int flag;

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
        /* Command */
        cmd = {};
        strcpy(cmd.id, this->id.c_str());
        /* Flag */
        flag = FLAG_DEFAULT;
    }

    ~Subscriber() {
        // Close TCP socket
        close(tcp_sock);
    }

    void start() {
        int8_t type;
        std::string topic;
        uint8_t sf;

        /* Send connect message to server */
        CMD_connect();

        /* Wait for events */
        while (flag) {
            if (poll(poll_fds.data(), poll_fds.size(), TIMEOUT) == -1) {
                eerror("Poll failed");
            }
            // Check for events on STDIN
            if (poll_fds[0].revents & POLLIN) {
                type = userInput(&topic, &sf);
                if (type == CMD_EXIT) {
                    CMD_exit();
                }
                
                if (type == CMD_SUBSCRIBE) {
                    CMD_subscribe(&topic, sf);
                }
                
                if (type == CMD_UNSUBSCRIBE) {
                    CMD_unsubscribe(&topic);
                }

                if (type == BAD_INPUT) {
                    std::cout << "subscribe <TOPIC> <SF>" << std::endl;
                    std::cout << "unsubscribe <TOPIC>" << std::endl;
                    std::cout << "exit" << std::endl;
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

                // Packet is not fully received
                if (pack_len > recv_len) {
                    n = recv(sock_fd, right, FULLBUF_SIZE - 1, 0);
                    if (n < 0) {
                        eerror("TCP recv error");
                    }
                    right += n;
                } else {
                    // Received enough bytes to process the first packet
                    packageProcess(pack_len - sizeof(uint16_t));

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

    void packageProcess(uint16_t len) {
        // Check the protocol of the package
        uint8_t protocol = *((uint8_t *)(buf + len - sizeof(uint8_t)));
        if (protocol == FF_PROTOCOL) {
            ff_ftr ftr;
            struct in_addr addr;
            std::string topic;
            char *buf_p;
            char c;

            buf_p = buf;
            ftr = ff_unpack(buf_p, &len);

            // Print UDP sender address and port
            addr.s_addr = ftr.s_addr;
            std::cout << inet_ntoa(addr) << ":" << ntohs(ftr.s_port);
            std::cout << " - ";

            // Extract the topic
            c = *(buf_p + 50);
            *(buf_p + 50) = '\0';
            topic = buf_p;
            *(buf_p + 50) = c;

            std::cout << topic;
            std::cout << " - ";

            // Extract the type
            buf_p += 50;
            uint8_t type = (*((uint8_t *)buf_p));
            
            // Extract the content
            buf_p += sizeof(uint8_t);
            
            if (type == INT) {
                std::cout << "INT - ";

                uint8_t sign;
                uint32_t digits;

                sign = *((uint8_t *)buf_p);
                buf_p += sizeof(uint8_t);
                digits = ntohl(*((uint32_t *)buf_p));
                buf_p += sizeof(uint32_t);


                if (sign) {
                    std::cout << "-" << digits;
                } else {
                    std::cout << digits;
                }

            } else if (type == SHORT_REAL) {
                std::cout << "SHORT REAL - ";

                uint16_t digits;

                digits = ntohs(*((uint16_t *)buf_p));
                buf_p += sizeof(uint16_t);

                std::cout << std::fixed;
                std::cout  << std::setprecision(2) << (double)digits / 100;

            } else if (type == FLOAT) {
                std::cout << "FLOAT - ";

                uint8_t sign;
                uint32_t digits;
                uint8_t neg_pow;
                double number;

                sign = *((uint8_t *)buf_p);
                buf_p += sizeof(uint8_t);
                digits = ntohl(*((uint32_t *)buf_p));
                buf_p += sizeof(uint32_t);
                neg_pow = *((uint8_t *)buf_p);
                buf_p += sizeof(uint8_t);


                number = (double)digits;
                if (sign) {
                    number *= -1;
                }
                number /= pow(10, neg_pow);
                std::cout << std::fixed;
                std::cout  << std::setprecision(neg_pow) << number;

            } else if (type == STRING) {
                std::cout << "STRING - ";

                c = *(buf_p + len);
                *(buf_p + len) = '\0';

                std::cout << buf_p;
                
                *(buf_p + len) = c;
            }

            std::cout << std::endl;
        } else if (protocol == CMD_PROTOCOL) {
            cmd = cmd_unpack(buf);

            if (cmd.type == CMD_EXIT)
                flag = FLAG_EXIT;
        }
    }

    int8_t userInput(std::string *topic, uint8_t *sf) {
        char *token;
        // Exit
        std::cin.getline(buf, BUF_SIZE);
        if (strcmp(buf, "exit") == 0) {
            return CMD_EXIT;
        }
        token = strtok(buf, "  ");
        // Subscribe
        if (strcmp(token, "subscribe") == 0) {
            // Topic
            token = strtok(NULL, " ");
            if (token == NULL)
                return BAD_INPUT;
            if (strlen(token) > 10)
                return BAD_INPUT;
            *topic = token;
            // SF
            token = strtok(NULL, " ");
            if (token == NULL)
                return BAD_INPUT;
            if (strlen(token) != 1)
                return BAD_INPUT;
            if (*token != '0' && *token != '1')
                return BAD_INPUT;
            *sf = *token - '0';
            // Number of arguments
            token = strtok(NULL, " ");
            if (token == NULL) {
                return CMD_SUBSCRIBE;
            }
            else {
                while (token != NULL) {
                    token = strtok(NULL, " ");
                }
                return BAD_INPUT;
            }
        }
        // Unsubscribe
        if (strcmp(token, "unsubscribe") == 0) {
            // Topic
            token = strtok(NULL, " ");
            if (token == NULL)
                return BAD_INPUT;
            if (strlen(token) > 10)
                return BAD_INPUT;
            *topic = token;
            // Number of arguments
            token = strtok(NULL, " ");
            if (token == NULL) {
                return CMD_UNSUBSCRIBE;
            }
            else {
                while (token != NULL) {
                    token = strtok(NULL, " ");
                }
                return BAD_INPUT;
            }
        }
        // Bad input
        while (token != NULL) {
            token = strtok(NULL, " ");
        }
        return BAD_INPUT;
    }

    void CMD_connect() {
        cmd.type = CMD_CONNECT;
        cmd_pack(buf, &cmd);
        TCP_send(tcp_sock, (uint16_t)sizeof(command));
    }

    void CMD_exit() {
        cmd.type = CMD_EXIT;
        cmd_pack(buf, &cmd);
        TCP_send(tcp_sock, sizeof(command));
        flag = FLAG_EXIT;
    }

    void CMD_subscribe(std::string *topic, uint8_t sf) {
        cmd.type = CMD_SUBSCRIBE;
        strcpy(cmd.topic, (*topic).c_str());
        cmd.sf = sf;
        cmd_pack(buf, &cmd);
        TCP_send(tcp_sock, sizeof(command));
    }

    void CMD_unsubscribe(std::string *topic) {
        cmd.type = CMD_UNSUBSCRIBE;
        strcpy(cmd.topic, (*topic).c_str());
        cmd_pack(buf, &cmd);
        TCP_send(tcp_sock, sizeof(command));
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
