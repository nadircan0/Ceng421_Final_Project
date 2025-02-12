#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define MSG_LEN     2048
#define MAX_CLIENT  5
#define NAME_LEN    20

void* receive_message(void *arg);
void send_message(char *msg, int len);
void send_client_list(int client_socket);
void error_handler(const char *err_msg);

int number_of_clients = 0;
int client_sockets[MAX_CLIENT];

char client_ips[MAX_CLIENT][INET_ADDRSTRLEN]; 
char client_names[MAX_CLIENT][NAME_LEN]; 

pthread_mutex_t mutex;

int main(int argc, char *argv[])
{
    int server_socket, client_socket;
    struct sockaddr_in server_address, client_address;
    socklen_t client_address_size;
    pthread_t thread_id;

    if (argc != 2)
    {
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    pthread_mutex_init(&mutex, NULL);

    server_socket = socket(PF_INET, SOCK_STREAM, 0);

    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family      = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port        = htons(atoi(argv[1]));

    if (bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) == -1)
        error_handler("bind() error");

    if (listen(server_socket, 5) == -1)
        error_handler("listen() error");

    printf("Group chat server started on port %s\n", argv[1]);

    while (1)
    {
        client_address_size = sizeof(client_address);
        client_socket = accept(server_socket, (struct sockaddr*)&client_address, &client_address_size);

        if (client_socket == -1) {
            perror("accept() error");
            continue;
        }

        pthread_mutex_lock(&mutex);
        client_sockets[number_of_clients] = client_socket;

        inet_ntop(AF_INET, &client_address.sin_addr,
                  client_ips[number_of_clients], INET_ADDRSTRLEN);

        number_of_clients++;
        pthread_mutex_unlock(&mutex);

        pthread_create(&thread_id, NULL, receive_message, (void*)&client_socket);
        pthread_detach(thread_id);

        printf("Connected client IP: %s\n", inet_ntoa(client_address.sin_addr));
    }

    close(server_socket);
    return 0;
}


void* receive_message(void *arg)
{
    int client_socket = *((int*)arg);
    int str_len = 0;
    int i;

    pthread_mutex_lock(&mutex);
    int client_index = number_of_clients - 1;
    pthread_mutex_unlock(&mutex);

    char msg[MSG_LEN];
    char userName[NAME_LEN] = "Unknown";

    str_len = read(client_socket, userName, NAME_LEN - 1);
    if (str_len <= 0) {
        goto DISCONNECT;
    }
    userName[str_len] = '\0';

    pthread_mutex_lock(&mutex);
    strncpy(client_names[client_index], userName, NAME_LEN - 1);
    client_names[client_index][NAME_LEN - 1] = '\0';
    pthread_mutex_unlock(&mutex);

    {
        char joinedMsg[MSG_LEN];
        snprintf(joinedMsg, sizeof(joinedMsg),
                 "---- %s has joined the chat ----\n", userName);
        send_message(joinedMsg, strlen(joinedMsg));
    }

    while ((str_len = read(client_socket, msg, sizeof(msg) - 1)) != 0)
    {
        if (str_len < 0) {
            perror("read() error");
            break;
        }
        msg[str_len] = '\0';

        if (strncmp(msg, "LIST", 4) == 0 || strncmp(msg, "list", 4) == 0)
        {
            send_client_list(client_socket);
        }
        else if (strncmp(msg, "exit", 4) == 0 || strncmp(msg, "EXIT", 4) == 0)
        {
            char leftMsg[MSG_LEN];
            snprintf(leftMsg, sizeof(leftMsg),
                     "---- %s has left the chat ----\n", userName);
            send_message(leftMsg, strlen(leftMsg));
            break;
        }
        else
        {
            char chatMsg[MSG_LEN + NAME_LEN + 5];
            snprintf(chatMsg, sizeof(chatMsg), "%s: %s", userName, msg);
            send_message(chatMsg, strlen(chatMsg));
        }
    }

DISCONNECT:
    pthread_mutex_lock(&mutex);
    for (i = 0; i < number_of_clients; i++)
    {
        if (client_sockets[i] == client_socket)
        {
            while (i < number_of_clients - 1)
            {
                client_sockets[i] = client_sockets[i + 1];
                strcpy(client_ips[i], client_ips[i + 1]);
                strcpy(client_names[i], client_names[i + 1]);
                i++;
            }
            number_of_clients--;
            break;
        }
    }
    pthread_mutex_unlock(&mutex);

    close(client_socket);
    return NULL;
}

void send_client_list(int client_socket)
{
    char list_msg[MSG_LEN] = "Connected clients:\n";

    pthread_mutex_lock(&mutex);

    for (int i = 0; i < number_of_clients; i++)
    {
        char line[128];
        snprintf(line, sizeof(line), "[%s] %s\n", client_ips[i], client_names[i]);
        strcat(list_msg, line);
    }

    pthread_mutex_unlock(&mutex);

    write(client_socket, list_msg, strlen(list_msg));
}

void send_message(char *msg, int len)
{
    pthread_mutex_lock(&mutex);

    for (int i = 0; i < number_of_clients; i++)
    {
        write(client_sockets[i], msg, len);
    }

    pthread_mutex_unlock(&mutex);
}

void error_handler(const char *err_msg)
{
    fputs(err_msg, stderr);
    fputc('\n', stderr);
    exit(1);
}
