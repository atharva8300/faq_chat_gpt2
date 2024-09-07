
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define BUFFER_SIZE 1024

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    char message[BUFFER_SIZE];
    fd_set read_fds;
    int chatbot_active = 0;

    // Create a socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set up the server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8888);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connect to the server
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    printf("Connected to server. Type '/chatbot login' to talk to the chatbot, or '/logout' to exit.\n");

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(sockfd, &read_fds);
        FD_SET(STDIN_FILENO, &read_fds);

        // Wait for incoming data or user input
        if (select(sockfd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            perror("Select error");
            continue;
        }

        // Handle data from the server
        if (FD_ISSET(sockfd, &read_fds)) {
            int bytes_received = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
            if (bytes_received <= 0) {
                printf("Server disconnected\n");
                break;
            }
            buffer[bytes_received] = '\0';
            printf("%s\n", buffer);
            if (strstr(buffer, "stupidbot> Hi, I am stupid bot")) {
                chatbot_active = 1;
            }
            if (strstr(buffer, "stupidbot> Bye! Have a nice day")) {
                chatbot_active = 0;
            }
            if (strstr(buffer, "gpt2bot> Hi, I am updated bot")) {
                chatbot_active = 2;
            }
            if (strstr(buffer, "gpt2bot> Bye! Have a nice day")) {
                chatbot_active = 0;
            }
        }

        // Handle user input
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            fgets(message, BUFFER_SIZE, stdin);
            message[strcspn(message, "\n")] = 0; // Remove newline character

            if (send(sockfd, message, strlen(message), 0) < 0) {
                perror("Send error");
                break;
            }

            if (strcmp(message, "/logout") == 0) {
                printf("Logging out...\n");
                break;
            }
        }
    }

    close(sockfd);
    return 0;
}