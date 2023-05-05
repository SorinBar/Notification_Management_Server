#include "command.h"

uint16_t cmd_pack(char *buf, command *cmd) {
    cmd->protocol = CMD_PROTOCOL;
    *((command *)buf) = *cmd;
    
    return (uint16_t)sizeof(command);
}


command cmd_unpack(char *buf) {
    return *((command *)buf);
}