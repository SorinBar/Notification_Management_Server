#include "utils.h"

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