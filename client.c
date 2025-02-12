#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define MSG_LEN     1024
#define NAME_LEN    20

void* send_message(void *arg);
void* receive_message(void *arg);
void error_handler(const char *err_msg);

char name[NAME_LEN] = "no_name";
char message[MSG_LEN];

int main(int argc, char *argv[])
{
    int sock;
    struct sockaddr_in sv_addr;
    pthread_t send_thread, receive_thread;
    void *thread_return;

    if (argc != 4)
    {
        printf("Usage: %s <IP> <port> <username>\n", argv[0]);
        exit(1);
    }

    snprintf(name, sizeof(name), "%s", argv[3]);

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1)
        error_handler("socket() error");

    memset(&sv_addr, 0, sizeof(sv_addr));
    sv_addr.sin_family      = AF_INET;
    sv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    sv_addr.sin_port        = htons(atoi(argv[2]));

    if (connect(sock, (struct sockaddr*)&sv_addr, sizeof(sv_addr)) == -1)
        error_handler("connect() error");

    printf("Group chat application connected. (Type 'exit' to quit, 'LIST' to see who is connected)\n");

    write(sock, name, strlen(name));

    pthread_create(&send_thread, NULL, send_message, (void*)&sock);
    pthread_create(&receive_thread, NULL, receive_message, (void*)&sock);

    pthread_join(send_thread, &thread_return);
    pthread_join(receive_thread, &thread_return);

    close(sock);
    return 0;
}

void* send_message(void *arg)
{
    int sock = *((int*)arg);

    while (1)
    {
        if (fgets(message, MSG_LEN, stdin) == NULL)
            continue;

        if (!strcmp(message, "exit\n"))
        {
            write(sock, message, strlen(message));
            break;
        }

        write(sock, message, strlen(message));
    }


    close(sock);
    exit(0);
    return NULL;
}

void* receive_message(void *arg)
{
    int sock = *((int*)arg);
    char incoming_msg[MSG_LEN + NAME_LEN + 50];
    int str_len;

    while (1)
    {
        str_len = read(sock, incoming_msg, sizeof(incoming_msg) - 1);
        if (str_len == -1)
            return (void*)-1;
        if (str_len == 0)
        {
            printf("Server disconnected.\n");
            exit(0);
        }
        incoming_msg[str_len] = '\0';

        fputs(incoming_msg, stdout);
    }

    return NULL;
}

void error_handler(const char *err_msg)
{
    fputs(err_msg, stderr);
    fputc('\n', stderr);
    exit(1);
}
