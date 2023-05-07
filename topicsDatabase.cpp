#include "topicsDatabase.h"

void TopicsDB::subscribe(std::string topic, std::string id, uint8_t sf) {
    topics[topic][id] = sf;
}

void TopicsDB::unsubscribe(std::string topic, std::string id) {
    if (topics.count(topic))
        topics[topic].erase(id);
}

std::unordered_map<std::string, uint8_t> *TopicsDB::getTopic(std::string topic) {
    if (topics.count(topic))
        return &(topics[topic]);
    
    return NULL;
}