#include "topicsDatabase.h"

void TopicsDB::subscribe(std::string topic, std::string id) {
    topics[topic].insert(id);
}

void TopicsDB::unsubscribe(std::string topic, std::string id) {
    if (topics.count(topic))
        topics[topic].erase(id);
}

std::unordered_set<std::string> *TopicsDB::getTopic(std::string topic) {
    if (topics.count(topic))
        return &(topics[topic]);
    
    return NULL;
}