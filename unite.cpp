#include "unite.h"

void eerror2(std::string message) {
    std::cerr << message << std::endl;
    exit(EXIT_FAILURE);
}

void unite_send(char *buf, uint16_t len, int tcp_fd) {
    ssize_t n;
    
    // Make space for the len area
    buf -= sizeof(uint16_t);

    *((uint16_t *)buf) = htons(len);
    len += sizeof(uint16_t);
    while(len != 0) {
        n = send(tcp_fd, buf, (size_t)len, 0);
        if (n < 0) {
            eerror2("Unite send error");
        }
        buf += n;
        len -= (uint16_t)n;
    }
}


