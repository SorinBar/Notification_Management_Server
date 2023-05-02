#ifndef UNITE_H
#define UNITE_H

    #include <iostream>
    #include <unistd.h>
    #include <netinet/tcp.h>
    #include <arpa/inet.h>

    void unite_send(char *buf, uint16_t len, int tcp_fd);

#endif