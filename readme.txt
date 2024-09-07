-   To compile the server :
        gcc server_backup.c -o server -luuid
    To compile the client :
        gcc client.c -o client

    To run the server :
        ./server
    
    To run the clients :
        ./client

- Dependancies :
    POSIX-compliant operating system: The code relies on POSIX-compliant system calls and functions for network communication and file descriptors management. This includes functions like socket(), connect(), recv(), and select() for handling network communication, and FD_SET(), FD_ZERO(), etc., for managing file descriptor sets used in the select() function.

    Standard C Library (libc): This code uses standard C library functions such as printf(), fgets(), strcspn(), and perror() for input/output operations, string manipulation, and error handling. The standard C library is a fundamental dependency for basic runtime services in C programs.

    Networking libraries: The code utilizes networking libraries that are part of the standard C library in POSIX systems, such as <sys/socket.h>, <netinet/in.h>, and <arpa/inet.h>, which provide the necessary data structures and functions for creating sockets, constructing network addresses, and converting between host and network byte orders.

For Task 1 :
1. /active : shows all active clients
2. /send <recipient_id> : sends message to the given uuid
3. /logout : terminates the client

For Task 2:
1. /chatbot login : enters chatbot mode.
2. /chatbot logout : exits chatbot mode.

For Task 3 :
1. /history <recipient_id>: Retrieves the conversation history between the requesting client and the specified recipient.
2. /history_delete <recipient_id>: Deletes the chat history of the specified recipient for the requesting client.
3. /delete_all: Deletes the complete chat history of the requesting client.

Whenever a client sends a message to another client, the server stores the message in the chat history for both clients.
The chat history is stored in memory and will be lost when the server is restarted.


For Task 4 : [Note this takes a few minutes to load the response ...]
1. /chatbot_v2 login : enters gpt2-chatbot mode.
2. /chatbot_v2 logout : exits gpt2-chatbot mode.