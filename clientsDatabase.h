#ifndef CLIENTSDATABASE_H
#define CLIENTSDATABASE_H
    #include <iostream>
    #include <unordered_map>
    #include <string>
    #include <queue>

    typedef struct {
        int fd;
        std::queue<char *> messQueue;
    } clientData;

    class ClientsDB {
    private:
        std::unordered_map<std::string, clientData*> clients;

    public:
        ~ClientsDB();
        int connect(std::string id, int fd);
        int disconnect(std::string id);
        clientData *getClient(std::string id);
    };

#endif