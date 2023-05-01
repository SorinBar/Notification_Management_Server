#ifndef UDP_NOTIFICATION
#define UDP_NOTIFICATION
    
    #include <string>
    #include <unistd.h>

    typedef struct notification
    {
        std::string topic;
        std::string content;
        std::string str;
    };
    
#endif