#include <iostream>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>

int main() {
    int listen_fd, conn_fd;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t cli_len = sizeof(cli_addr);

    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return 1;
    }

    int optval = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        std::cerr << "Failed to set socket options" << std::endl;
        return 1;
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(9090);

    if (bind(listen_fd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Failed to bind socket to address" << std::endl;
        return 1;
    }

    if (listen(listen_fd, 10) < 0) {
        std::cerr << "Failed to listen on socket" << std::endl;
        return 1;
    }

    struct pollfd fds[1];
    fds[0].fd = listen_fd;
    fds[0].events = POLLIN;

    while (true) {
        int num_fds = poll(fds, 1, -1);
        if (num_fds < 0) {
            std::cerr << "Failed to poll sockets" << std::endl;
            return 1;
        }

        if (fds[0].revents & POLLIN) {
            conn_fd = accept(listen_fd, (struct sockaddr*) &cli_addr, &cli_len);
            if (conn_fd < 0) {
                std::cerr << "Failed to accept connection" << std::endl;
                return 1;
            }

            char buffer[1024];
            int bytes_read = read(conn_fd, buffer, sizeof(buffer) - 1);
            if (bytes_read < 0) {
                std::cerr << "Failed to read data from socket" << std::endl;
                return 1;
            }

            buffer[bytes_read] = '\0';
            std::cout << "Received: " << buffer << std::endl;

            send(conn_fd, "BINE MA\n", 9, 0);

            close(conn_fd);
        }
    }

    close(listen_fd);
    return 0;
}
