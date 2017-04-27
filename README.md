# chatroom
#BCIT CST NoName Group

#Fred Yang, Maitiu Morton

The goal of this assignment is to design and implement a multi-client chat client/server
application using select() / multiplexed I/O techniques. The application must implement
the following minimum features:

o The server accepts connections on a specified port and once clients have
established a connection with it, it will echo the text strings it receives from each
client to all other clients except the sender.

o The server will maintain a list of all connected clients (host names) and display the
updated list on the console.

o Each client will be capable of sending text strings to the server and displaying the
text sent by all other clients.

o Each chat participant will not only see the text string but also the client (hostname)
it was from. The client information includes hostname, ip address, and file
descriptor connected.

o Optionally, a client can specify (command line argument) that the chat session
also be dumped to a file with CR-LF terminated records.
