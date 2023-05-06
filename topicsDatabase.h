#ifndef TOPICSDATABASE_H
#define TOPICSDATABASE_H

    #include <iostream>
    #include <unordered_map>
    #include <unordered_set>
    #include <string>

    class TopicsDB {
    private:
        std::unordered_map<std::string ,std::unordered_set<std::string>> topics;
    
    public:
        void subscribe(std::string topic, std::string id);
        void unsubscribe(std::string topic, std::string id);
        std::unordered_set<std::string> *getTopic(std::string id);
    };

#endif