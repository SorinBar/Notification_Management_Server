#ifndef UTILS_H
#define UTILS_H

    #include <iostream>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <cstring>

    #define FULLBUF_SIZE 4000
    #define BUF_SIZE 3998
    #define TIMEOUT -1

    // Print message to STDERR and exit
    void eerror(std::string message);
    // Convert string to in_port_t/uint16_t
    in_port_t str_to_port(std::string port_str);

#endif