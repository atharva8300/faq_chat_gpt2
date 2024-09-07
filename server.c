
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <uuid/uuid.h>
#include <ctype.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024
#define MAX_FAQS 100
#define MAX_HISTORY_SIZE 100

typedef struct {
    char question[BUFFER_SIZE];
    char answer[BUFFER_SIZE];
} FAQ;

typedef struct {
    int sockfd;
    char id[37];
    int chatbot_active;  // 0 for regular chat, 1 for GPT-2 chatbot, 2 for FAQ chatbot
    char history[MAX_CLIENTS][MAX_HISTORY_SIZE][BUFFER_SIZE];
    int history_count[MAX_CLIENTS];
} Client;

Client clients[MAX_CLIENTS];
FAQ faqs[MAX_FAQS];
int num_clients = 0;
int num_faqs = 0;

void load_faqs(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Unable to open the FAQs file");
        return;
    }

    char question_buffer[BUFFER_SIZE];
    char answer_buffer[BUFFER_SIZE];

    while (fscanf(file, " %[^|] ||| %[^\n]", question_buffer, answer_buffer) != EOF) {
        int question_len = strlen(question_buffer);
        while (question_len > 0 && isspace(question_buffer[question_len - 1])) {
            question_buffer[--question_len] = '\0';
        }

        strncpy(faqs[num_faqs].question, question_buffer, sizeof(faqs[num_faqs].question));
        strncpy(faqs[num_faqs].answer, answer_buffer, sizeof(faqs[num_faqs].answer));
        
        faqs[num_faqs].question[sizeof(faqs[num_faqs].question) - 1] = '\0';
        faqs[num_faqs].answer[sizeof(faqs[num_faqs].answer) - 1] = '\0';

        num_faqs++;
        if (num_faqs >= MAX_FAQS) break;
    }

    fclose(file);
}

void send_to_client(int sockfd, const char *message) {
    send(sockfd, message, strlen(message), 0);
}

#define RESPONSE_SIZE (BUFFER_SIZE + 100)
void handle_gpt2_chatbot_message(const char *message, int client_index) {
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "python3 gpt2_chatbot.py \"%s\"", message);

    FILE *fp = popen(cmd, "r");
    if (fp == NULL) {
        printf("Failed to run command\n");
        return;
    }

    char response[RESPONSE_SIZE];
    while (fgets(response, sizeof(response), fp) != NULL) {
        send_to_client(clients[client_index].sockfd, response);
    }

    pclose(fp);
}

void handle_chatbot_message(const char *message, int client_index) {
    printf("Received chatbot message: '%s'\n", message);

    int found = 0;
    for (int i = 0; i < num_faqs; i++) {
        if (strcasecmp(message, faqs[i].question) == 0) {
            found = 1;
            char response[BUFFER_SIZE + 100];
            snprintf(response, sizeof(response), "stupidbot> %s\n", faqs[i].answer);
            send_to_client(clients[client_index].sockfd, response);
            break;
        }
    }

    if (!found) {
        send_to_client(clients[client_index].sockfd, "stupidbot> System Malfunction, I couldn't understand your query.\n");
    }
}

void handle_message(char *buffer, int client_index) {
    // If the GPT-2 chatbot is active for the client
    if (clients[client_index].chatbot_active == 1) {
        if (strncmp(buffer, "/chatbot_v2 logout", 17) == 0) {
            clients[client_index].chatbot_active = 0;
            send_to_client(clients[client_index].sockfd, "gpt2bot> Bye! Have a nice day and hope you do not have any complaints about me\n");
        } else {
            handle_gpt2_chatbot_message(buffer, client_index);
        }
        return;
    }

    // If the FAQ chatbot is active for the client
    if (clients[client_index].chatbot_active == 2) {
        if (strncmp(buffer, "/chatbot logout", 15) == 0) {
            clients[client_index].chatbot_active = 0;
            send_to_client(clients[client_index].sockfd, "stupidbot> Bye! Have a nice day and do not complain about me\n");
        } else {
            handle_chatbot_message(buffer, client_index);
        }
        return;
    }

    char *command = strtok(buffer, " \n");

    if (strcmp(command, "/chatbot") == 0) {
        char *subcommand = strtok(NULL, " \n");
        if (subcommand && strcmp(subcommand, "login") == 0) {
            clients[client_index].chatbot_active = 2;
            send_to_client(clients[client_index].sockfd, "stupidbot> Hi, I am stupid bot, I am able to answer a limited set of your questions\n");
        }
    } else if (strcmp(command, "/chatbot_v2") == 0) {
        char *subcommand = strtok(NULL, " \n");
        if (subcommand && strcmp(subcommand, "login") == 0) {
            clients[client_index].chatbot_active = 1;
            send_to_client(clients[client_index].sockfd, "gpt2bot> Hi, I am updated bot, I am able to answer any question be it correct or incorrect\n");
        }
    } else if (strcmp(command, "/active") == 0) {
        char active_clients_list[BUFFER_SIZE] = "Active clients:\n";
        for (int i = 0; i < num_clients; i++) {
            strcat(active_clients_list, clients[i].id);
            strcat(active_clients_list, "\n");
        }
        send_to_client(clients[client_index].sockfd, active_clients_list);
    } else if (strcmp(command, "/send") == 0) {
        char *dest_id = strtok(NULL, " ");
        char *msg = strtok(NULL, "");
        if (dest_id && msg) {
            for (int i = 0; i < num_clients; i++) {
                if (strcmp(clients[i].id, dest_id) == 0) {
                    char full_message[BUFFER_SIZE];
                    snprintf(full_message, sizeof(full_message), "From %s: %s", clients[client_index].id, msg);
                    send_to_client(clients[i].sockfd, full_message);

                    // Store message in chat history for both clients
                    strncpy(clients[client_index].history[i][clients[client_index].history_count[i]++], full_message, BUFFER_SIZE);
                    strncpy(clients[i].history[client_index][clients[i].history_count[client_index]++], full_message, BUFFER_SIZE);
                    break;
                }
            }
        }
    } else if (strcmp(command, "/logout") == 0) {
        send_to_client(clients[client_index].sockfd, "Bye!! Have a nice day\n");
        close(clients[client_index].sockfd);
        memmove(&clients[client_index], &clients[client_index + 1], sizeof(Client) * (num_clients - client_index - 1));
        num_clients--;
    } else if (strcmp(command, "/history") == 0) {
        char *dest_id = strtok(NULL, " \n");
        if (dest_id) {
            int dest_index = -1;
            for (int i = 0; i < num_clients; i++) {
                if (strcmp(clients[i].id, dest_id) == 0) {
                    dest_index = i;
                    break;
                }
            }
            if (dest_index != -1) {
                char history_buffer[BUFFER_SIZE * MAX_HISTORY_SIZE] = "";
                for (int i = 0; i < clients[client_index].history_count[dest_index]; i++) {
                    strcat(history_buffer, clients[client_index].history[dest_index][i]);
                    strcat(history_buffer, "\n");
                }
                send_to_client(clients[client_index].sockfd, history_buffer);
            } else {
                send_to_client(clients[client_index].sockfd, "Invalid recipient ID.\n");
            }
        } else {
            send_to_client(clients[client_index].sockfd, "Usage: /history <recipient_id>\n");
        }
    } else if (strcmp(command, "/history_delete") == 0) {
        char *dest_id = strtok(NULL, " \n");
        if (dest_id) {
            int dest_index = -1;
            for (int i = 0; i < num_clients; i++) {
                if (strcmp(clients[i].id, dest_id) == 0) {
                    dest_index = i;
                    break;
                }
            }
            if (dest_index != -1) {
                clients[client_index].history_count[dest_index] = 0;
                send_to_client(clients[client_index].sockfd, "Chat history with the specified recipient has been deleted.\n");
            } else {
                send_to_client(clients[client_index].sockfd, "Invalid recipient ID.\n");
            }
        } else {
            send_to_client(clients[client_index].sockfd, "Usage: /history_delete <recipient_id>\n");
        }
    } else if (strcmp(command, "/delete_all") == 0) {
        for (int i = 0; i < num_clients; i++) {
            clients[client_index].history_count[i] = 0;
        }
        send_to_client(clients[client_index].sockfd, "Complete chat history has been deleted.\n");
    } else {
        send_to_client(clients[client_index].sockfd, "Unknown command.\n");
    }
}

int main() {
    load_faqs("FAQs.txt");
    int server_sockfd, new_sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len;
    char buffer[BUFFER_SIZE];
    fd_set read_fds;

    server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sockfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8888);

    if (bind(server_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(server_sockfd, MAX_CLIENTS) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server started. Listening on port 8888...\n");

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(server_sockfd, &read_fds);

        for (int i = 0; i < num_clients; i++) {
            FD_SET(clients[i].sockfd, &read_fds);
        }

        if (select(FD_SETSIZE, &read_fds, NULL, NULL, NULL) < 0) {
            perror("select");
            continue;
        }

        if (FD_ISSET(server_sockfd, &read_fds)) {
            addr_len = sizeof(client_addr);
            new_sockfd = accept(server_sockfd, (struct sockaddr *)&client_addr, &addr_len);

            if (new_sockfd < 0) {
                perror("accept");
                continue;
            }

            if (num_clients < MAX_CLIENTS) {
                uuid_t uuid;
                uuid_generate_random(uuid);
                char new_id[37];
                uuid_unparse(uuid, new_id);

                clients[num_clients].sockfd = new_sockfd;
                strcpy(clients[num_clients].id, new_id);
                clients[num_clients].chatbot_active = 0;
                num_clients++;

                printf("New client connected: %s\n", new_id);
                send_to_client(new_sockfd, new_id);
            } else {
                send_to_client(new_sockfd, "Server full\n");
                close(new_sockfd);
            }
        }

        for (int i = 0; i < num_clients; i++) {
            if (FD_ISSET(clients[i].sockfd, &read_fds)) {
                int bytes_received = recv(clients[i].sockfd, buffer, BUFFER_SIZE - 1, 0);

                if (bytes_received <= 0) {
                    printf("Client %s disconnected\n", clients[i].id);
                    close(clients[i].sockfd);
                    memmove(&clients[i], &clients[i + 1], sizeof(Client) * (num_clients - i - 1));
                    num_clients--;
                } else {
                    buffer[bytes_received] = '\0';
                    handle_message(buffer, i);
                }
            }
        }
    }

    close(server_sockfd);
    return 0;
}