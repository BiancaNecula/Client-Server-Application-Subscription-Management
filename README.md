Communication Protocols Course \
HOMEWORK 2 - Router forwarding 

April, 2021

Bianca Necula \
Faculty of Automatic Control and Computer Science \
325CA 

# Info
```bash
git clone https://github.com/BiancaNecula/Client-Server-Application-Subscription-Management.git
```

# About the code:

In this project I have implemented a client-server application using TCP and UDP protocols. The application manages the messages received from TCP clients (implemented by me in subscriber.c) and UDP clients (already implemented) through a server.  
TCP clients can connect and reconnect (the second involving saving messages) to the server and can subscribe / unsubscribe to certain topics. These topics are given through messages received from UDP clients in the form of structures that contain the topic, data type and content. When the client disconnects, he has the option when he subscribes to save his messages received from the respective topic while he is disconnected. Thus, each client is represented by an id structure, the list of topics to which he subscribed, the list of messages he will receive, etc. All these lists (and including the client list) are kept in a Linked List data structure (its implementation is done by me in the SD laboratory in year 1). We implemented the communication protocol between TCP clients and server through some helpful structures: client_msg (role: send information to the server about what the client wants - subscribe, unsubscribe, exit), pack_to_send (role: send information to the client related to packets / messages from udp clients and where they come from + check if it is an exit message or not). In addition to structures, at the beginning of a connection with a new client, the client id is also sent via a string.  
The project is not currently fully functional through the checker but I am still working on some details.
