#include "fast_forward.h"

uint16_t ff_pack(char *payload, uint16_t payload_len, ff_ftr ftr) {
    *((ff_ftr *)(payload + payload_len)) = ftr;

    return payload_len + sizeof(ff_ftr);
}

ff_ftr ff_unpack(char *pack, uint16_t *pack_len) {
    (*pack_len) -= sizeof(ff_ftr);
    return *((ff_ftr *)(pack + *pack_len));
}
