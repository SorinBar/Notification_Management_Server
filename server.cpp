#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define PORT 9090
#define BUFFER_SIZE 1024

int main() {
    int sockfd;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in servaddr, cliaddr;
    socklen_t len = sizeof(cliaddr);

    // Create a UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set up the server address
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET; // IPv4
    servaddr.sin_addr.s_addr = INADDR_ANY; // any available interface
    servaddr.sin_port = htons(PORT);

    // Bind the socket to the server address
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Receive UDP data from the client
    while (1) {
        // Wait for data to arrive on the socket
        ssize_t n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&cliaddr, &len);

        // Check if there was an error receiving data
        if (n < 0) {
            perror("recvfrom failed");
            exit(EXIT_FAILURE);
        }

        // Null-terminate the received data
        buffer[n] = '\0';

        if (strcmp(buffer, "exit") == 0)
            break;

        // Print the received data to stdout
        printf("Received data from client: %s\n", buffer);
    }


    // Close the socket
    close(sockfd);

    return 0;
}
