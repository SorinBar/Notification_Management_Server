#include "clientsDatabase.h"

int ClientsDB::connect(std::string id, int fd) {
    clientData data;
    if (clients.count(id)) {
        clientData data = clients[id];
        // Already connected
        if (data.fd != -1)
            return -1;
        else {
            // Connect client
            data.fd = fd;
            clients[id] = data;
        }
    } else {
        
        clients.insert({});
    }
}