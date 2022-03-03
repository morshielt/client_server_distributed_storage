# Distributed Server/Client Files Storage

C++ distributed file storage application consisting of server and client nodes.

Nodes communicate through the network using a TCP/UDP protocol. They can also collaborate by creating node groups (using an IP Multicast address), which they can dynamically join/leave. 

Every node shares the same set of functionalities of a given (server/client) type and all the nodes are equal when it comes to their rights, priorities and permissions in the group.

Client nodes provide user interface enabling uploading new files, downloading them and browsing files managed by the group.

Server nodes on the other hand have to store the designated files.

The project does not use the boost::asio library.
