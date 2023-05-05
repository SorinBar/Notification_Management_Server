#ifndef CLIENTSDATABASE_H
#define CLIENTSDATABASE_H

    #include <iostream>
    #include <unordered_map>
    #include <string>
    #include <queue>

    typedef struct {
        int fd;
        uint8_t sf;
        std::queue<char *> messQueue;
    } clientData;

    class ClientsDB {
    private:
        std::unordered_map<std::string, clientData> clients;

    public:
        int connect(std::string id, int fd);
    };

#endif