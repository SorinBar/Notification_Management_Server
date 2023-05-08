# Client-Server Notification Management Platform
## PCom 2nd Homework

Usage:
    Start:
    ./server <PORT_SERVER>
    ./subscriber <ID_CLIENT> <IP_SERVER> <PORT_SERVER>
    
    Subscriber:
        subscribe <TOPIC> <SF>
        unsubscribe <TOPIC>
        exit
    Server:
        exit
        

Reasoning behind the custom protocols:
	-The main concern is the TCP fragmentation and merging of the packets,
     so we need a solution to keep track of them.
	-Adding the length as a uint16_t in front of the packet is the most
     effective method.
	-To make the interaction between UDP receive and the TCP send as fast as
     possible will use a buffer(char *) to send and buffer + sizeof(uint16_t)
     to receive.
	-In this way, we don't need to copy or move the whole UDP packet in order
     to make space for the 2 bytes on which the TCP protocol depends.
	-To forward the messages as fast as possible and decrease the load on
     the server, we will redirect the message from the UDP client to the
     TCP client with Fast Forward Protocol.

    *Note: Fast Forward Protocol and Command Protocol are used on top
          of the TCP send and TCP recv.
    

    # Fast Forward Protocol
        -The main concern is speed.
        -In this case, we will treat the UDP-received packet as a black
         box and redirect it to the client.
        -Fast Forward Protocol adds a footer to the packet, containing the
         UDP packet sender address, port, and the protocol field
        
        *Note: The protocol field is used to differentiate a Fast Forward
               packet from a Command packet.
               0 - Fast Forward Protocol
               1 - Command protocol

         0                   1                   2                   3
         0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |                         Sender Address                        |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |          Sender Port          |    Protocol   |           
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

        Sender Adress: 32 bits
        Sender Port: 16 bits
        Protocol: 8 bits (0)
    

    -We also want to server and client to communicate commands such as: 
     connect, subscribe, unsubscribe, and exit.
    -Keeping in mind that:
        -The client id is a string with a maximum length of 10.
	    -The topic is a string as well, with a maximum length of 50.
    -We can create a simple structure that handles the communication between
     client and server.


    # Command Protocol
        -Simple, reliable, and fast.
        -It is a structure that encapsulates all the possible commands.

           1 byte                        11 bytes
        -----------------------------------------------------------------
        |    Type    |                       Id                         | 
        -----------------------------------------------------------------
                                    51 bytes                           
        -----------------------------------------------------------------
        |                             Topic                             |
        -----------------------------------------------------------------
           1 byte     1 byte 
        -----------------------
        |    SF    | Protocol |
        -----------------------

        Type: 8 bits
        Id: 88 bits
        Topic: 408 bits
        SF: 8 bits
        Protocol: 8 bits (1)


How does the server handle topics and subscribers?
    -Using an efficient clients database and topic database
    
    # Clients Database

    -O(1) time complexity for connect and disconnect:
     unordered_map<string, clientData*>
    - Client data will be accessed with the id:
    - clientData:
        int fd:
            -1 => client is disconnected
             _ => client is connected and linked to that file 
                  descriptor
        std::queue<char *> messQueue:
            -Contain the incoming messages from the topics where the client
             subscribed with SF enabled, for the time the client is disconnected.

    # Topics Database
    -O(1) time complexity for subscribe and unsubscribe:
     unordered_map<string ,unordered_map<string, uint8_t>>
    
    *Note: topicsDB[topic_id][client_id] = clinet_SF,
           SF = 0, disabled
           SF = 1, enabled