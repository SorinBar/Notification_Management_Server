#ifndef TOPICSDATABASE_H
#define TOPICSDATABASE_H

    #include <iostream>
    #include <unordered_map>
    #include <unordered_set>
    #include <string>

    class TopicsDB {
    private:
        std::unordered_map<std::string ,std::unordered_map<std::string, uint8_t>> topics;
    
    public:
        void subscribe(std::string topic, std::string id, uint8_t sf);
        void unsubscribe(std::string topic, std::string id);
        std::unordered_map<std::string, uint8_t> *getTopic(std::string topic);
    };

#endif