#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <poll.h>
#include <string>
#include <vector>
#include <netinet/tcp.h>

#define BUF_SIZE 1600
#define TIMEOUT -1

// Print message to STDERR and exit
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

int main(int argc, char *argv[]) {
    /* Setup */
    char buf[BUF_SIZE];
    if (argc != 2) {
        eerror("./server <PORT>");
    }
    in_port_t port = str_to_port(argv[1]);
    // disable STDOUT buffering
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);


    /* Server Address*/
    struct sockaddr_in server_addr = {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);


    /* UDP Socket */
    int udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_sock < 0) {
        eerror("UDP socket creation failed");
    }
    // Bind
    if (bind(udp_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
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
	if (bind(tcp_sock, (struct sockaddr *) &server_addr, sizeof(struct sockaddr)) < 0) {
        eerror("TCP socket bind failed");
    }
    // Listen
    // TODO - Might change 1 to a higher number
	if (listen(tcp_sock, 1) < 0) {
        eerror("TCP socket listen failed");
    }


    /* Poll */
    std::vector<pollfd> poll_fds(2);
    poll_fds[0].fd = udp_sock;
    poll_fds[0].events = POLLIN;
    poll_fds[1].fd = tcp_sock;
    poll_fds[1].events = POLLIN;


    /* Wait for events */
    while (true) {
        if (poll(poll_fds.data(), poll_fds.size(), TIMEOUT) == -1) {
            eerror("Poll monitoring failed");
        }

        // Check for events on the UDP socket
        if (poll_fds[0].revents & POLLIN) {
            ssize_t n = recvfrom(udp_sock, buf, BUF_SIZE, 0, nullptr, nullptr);
            if (n < 0) {
                perror("UDP receive failed");
                continue;
            }
            buf[n] = '\0';
            std::cout << "Received data from UDP socket: " << buf << '\n';
            if (strcmp(buf, "exit") == 0)
                break;
        }

    }

    // Close the UDP socket
    close(udp_sock);

    return 0;
}