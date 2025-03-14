Multithreaded Chat Application
Build a client-server chat application that handles multiple
clients simultaneously.

cd build cmake --build .

eco server 

nc localhost 12345 check with ncat

ncat - connect to server
The server should send a client list, with one client.
Then it should send ping messages.

connect to the server from another ncat instance in another terminal
The server should send both clients a new client list with two clients.

Disconnect one of the ncat terminals from the server.
The other client should receive a new client list with just one client.