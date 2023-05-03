#ifndef UTILS_H
#define UTILS_H

    #include <iostream>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <cstring>

    // Print message to STDERR and exit
    void eerror(std::string message);
    // Convert string to in_port_t/uint16_t
    in_port_t str_to_port(std::string port_str);

#endif