#ifndef COMMAND_H
#define COMMAND_H

    #include <iostream>
    
    #define CMD_PROTOCOL 1
    #define CMD_CONNECT 0
    #define CMD_SUBSCRIBE 1
    #define CMD_UNSUBSCRIBE 2
    #define CMD_EXIT 3

    typedef struct {
        uint8_t type;
        char id[11];
        char topic [51];
        uint8_t sf;
        uint8_t protocol;
    } __attribute__ ((__packed__)) command;

    /*
        Copy cmd_package struct to the buffer
        Returns the length of the package
    */
    uint16_t cmd_pack(char *buf, command *cmd);
    command cmd_unpack(char *buf);

#endif