#ifndef FAST_FORWARD_H
#define FAST_FORWARD_H

    #include <iostream>
    
    #define FF_PROTOCOL 0

    typedef struct {
        uint32_t s_addr;
        uint16_t s_port;
        uint8_t protocol;
    } __attribute__ ((__packed__)) ff_ftr;

    /* 
        Adds footer at the end of payload
        You need to make sure than another 7 bytes are allocated
        at the end of the payload buffer
        Returns the length of the updated package
    */
    uint16_t ff_pack(char *payload, uint16_t payload_len, ff_ftr ftr);
    /* 
        Returns the package footer
        Updates pack_len to payload length
    */
    ff_ftr ff_unpack(char *pack, uint16_t *pack_len);


#endif