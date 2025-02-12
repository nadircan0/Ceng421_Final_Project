    Multi-Client Chat Application Over TCP

Scope:
  This project aims to implement a multi-client chat application using TCP sockets, where multiple clients (users)
  can connect to a single server simultaneously to exchange messages. The server accepts new connections,
  assigns a communication thread for each client, and broadcasts any received message to all connected clients.
  The client side receives user input, sends it to the server, and displays incoming messages from the server.
  The scope includes:
    • A server application (server.c)
    • A client application (client.c)
    • A multi-user chat infrastructure over TCP
    • Basic features such as username handling, listing online users (with the LIST command), and exiting
    (exit command)

System Architecture
• Server
  o A main thread uses accept() to listen for new client connections.
  o For each new client, the server spawns a separate thread that handles incoming messages from
  that specific client.
  o Shared arrays (protected by a mutex) store the sockets and usernames of connected clients.
  o The very first message received from a client is taken as that client’s username; subsequent
  messages are either commands (LIST, exit) or regular chat messages.
• Client
  o Connects to the server via a single socket.
  o Right after the connection is established, the client sends its username as the very first
  message.
  o Afterwards, every line typed by the user is sent to the server (could be LIST, exit, or a normal
  message).
  o Simultaneously, another thread listens for messages from the server and prints them to the
  screen.    

This architecture enables multiple clients (up to a defined maximum 5) to chat with each other through the
server.

Protocol
1. Connection Setup
  o The client calls connect() to the server’s IP and port.
  o Once connected, the client immediately sends its username as the first line.
2. Messaging Phase
  o The client forwards every user input to the server.
  o The server identifies the first incoming line as the “username,” and subsequent lines as either
  normal messages or commands.
  o The server broadcasts each normal message to all connected clients in the format username:
  message.
3. Commands
  o LIST: The server responds with a list of all connected clients, including their IP addresses and
  usernames, but only sends this list back to the requesting client.
  o exit: The client disconnects by sending exit. The server broadcasts a message like ----
  username has left the chat ----, and removes the user from the list of connected clients.

Server Functions
  • socket() and bind(): Create a server socket and bind it to the specified port.
  • listen(): Wait for incoming client connections.
  • accept(): Accept a new client connection and return a socket descriptor for that client.
  • receive_message(): Runs in a thread dedicated to a single client, reads incoming data:
    o The first line is taken as the username.
    o Subsequent lines are either commands (LIST, exit) or normal chat messages.
    o If exit is received, the client is removed from the list, and a departure message is broadcast.
  • send_message(): Forwards the received messages to all connected clients (broadcast).
  • send_client_list(): Sends the list of connected clients (IPs and usernames) back to the requesting client
  when LIST is received.

Client Functions
  • connect(): Connects to the server using the given IP and port.
  • Send Username: Immediately after the connection is established, the client sends its username as the
  first line.
  • send_message(): Sends the user’s typed input to the server (whether it’s a command or a normal
  message).
  • receive_message(): Listens for messages from the server in a separate thread and displays them. If the
  server disconnects, the client also exits.

