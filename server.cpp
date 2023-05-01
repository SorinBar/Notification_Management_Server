#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <poll.h>
#include <string>
#include <vector>
#include <netinet/tcp.h>

#define BUF_SIZE 2000
#define TIMEOUT -1

// Print message to STDERR and exit
void eerror(std::string message);
// Convert string to in_port_t/uint16_t
in_port_t str_to_port(std::string port_str);
// Process UDP buffer and send notification
void UDP_input(char* buf, ssize_t len, struct sockaddr_in &addr);

int main(int argc, char *argv[]) {
    /* Setup */
    char buf[BUF_SIZE];
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
            eerror("Poll monitoring failed");
        }
        // Check for events on STDIN
        if (poll_fds[0].revents & POLLIN) {
            std::cin.getline(buf, BUF_SIZE);
            if (strcmp(buf, "exit") == 0)
                break;
        }
        // Check for events on the UDP socket
        if (poll_fds[1].revents & POLLIN) {
            ssize_t len = recvfrom(udp_sock, buf, BUF_SIZE, 0, (struct sockaddr *)&addr, &addr_len);
            if (len < 0) {
                std::cerr << "UDP receive failed";
                continue;
            }
            UDP_input(buf, len, addr);
        }
        // Check for events on the TCP Listen socket
        if (poll_fds[2].revents & POLLIN) {
            // Accept incoming connection
            int new_socket = accept(tcp_sock, (struct sockaddr *)&addr, &addr_len);
            
            // Handle new connection
            std::cout << "New client connected" << std::endl;

            send(new_socket, "bine ai venit\n", strlen("bine ai venit\n"), 0);
            recv(new_socket, buf, sizeof(buf), 0);
            std::cout << buf << std::endl;

            // Close socket
            close(new_socket);
        }

    }

    // Close the UDP socket
    close(udp_sock);
    // Close the TCP socket
    close(tcp_sock);

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

    char c;
    std::string topic;

    c = *(buf + 50);
    *(buf + 50) = '\0';
    topic = buf;
    *(buf + 50) = c;


    std::cout << topic << std::endl;
}