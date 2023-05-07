#include "clientsDatabase.h"

ClientsDB::~ClientsDB() {
    for (auto &pair:clients) {
        while (!pair.second->messQueue.empty()) {
            delete [](pair.second->messQueue.front());
            pair.second->messQueue.pop();
        }
        delete pair.second;
    }
}

int ClientsDB::connect(std::string id, int fd) {
    clientData *data;
    if (clients.count(id)) {
        data = clients[id];
        // Already connected
        if (data->fd != -1)
            return -1;
        else {
            // Connect the client
            data->fd = fd;
        }
    } else {
        clientData *data = new clientData;
        data->fd = fd;

        clients.insert({id, data});
    }

    return 0;
}

clientData *ClientsDB::getClient(std::string id) {
    if(clients.count(id))
        return clients[id];
    else
        return NULL;
}

int ClientsDB::disconnect(std::string id) {
    if (clients.count(id)) {
        clients[id]->fd = -1;   
        return 0;
    }
    return 1;
}