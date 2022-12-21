#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <stdbool.h>
#include <limits.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define BUFFERSIZE 4096
#define SOCKET_ERROR (-1)
#define BACKLOG 100
#define OK_MESSAGE "HTTP/1.1 200 OK\r\n"
#define NOTFOUND_MESSAGE "HTTP/1.1 404 Not Found\r\n"

typedef struct sockaddr_in SA_IN;
typedef struct sockaddr SA;

long check(long exp, const char *msg){
    if(exp == SOCKET_ERROR){
        perror(msg);
        exit(1);
    }
    return exp;
}

void sendMessage(int client_socket, char *msg){
    check(send(client_socket, msg, strlen(msg), 0), "Unable to send!");
}

void sendFile(char *path, int client_socket){
    FILE *fp = fopen(path, "r");
    if(fp == NULL){
        printf("ERROR(open): %s\n", path);
        send(client_socket, NOTFOUND_MESSAGE, strlen(NOTFOUND_MESSAGE), 0);
        return;
    }
    sendMessage(client_socket, OK_MESSAGE);
    printf("\n[+] Sending File %s...\n\n", path);

    char buffer[BUFFERSIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, BUFFERSIZE, fp))> 0){
        printf("sending %zu bytes\n", bytes_read);
        check(send(client_socket, buffer, bytes_read, 0), "Failed to send file!");
    }
    printf("\n[+] File Sent Successfully.\n\n");
    fclose(fp);
}

void receiveFile(char *path, int client_socket)
{
    FILE *fp = fopen(path, "w");
    if(fp == NULL){
        printf("ERROR(write): %s\n", path);
        send(client_socket, NOTFOUND_MESSAGE, strlen(NOTFOUND_MESSAGE), 0);
        return;
    }
    
    char buffer[BUFFERSIZE];
    size_t bytes_recv;
    while ((bytes_recv = read(client_socket, buffer, BUFFERSIZE))> 0){
        printf("receiving %zu bytes\n", bytes_recv);
        if ((fwrite(buffer, sizeof(char), bytes_recv, fp)) < bytes_recv){
            perror("File write failed! ");
            exit(EXIT_FAILURE);
        }
    }
    fclose(fp);
    printf("\n[+] File Received Successfully.\n\n");
    sendMessage(client_socket, OK_MESSAGE);
}

void handleGET(char *path, int client_socket){
    sendFile(path, client_socket);
}
void handlePOST(char *path, int client_socket){
    sendMessage(client_socket, OK_MESSAGE);
    receiveFile(path, client_socket);
}

void parseMessage(char *msg, char *type, char *path){
    char *temp = malloc(strlen(msg));
    strcpy(temp, msg);

    char * dot = ".";
    char *t = strtok(temp, " ");
    char *p = strtok(NULL, " ");
    char* relative;

    relative = malloc(strlen(p)+1);
    strcpy(relative, dot);
    strcat(relative, p);
    strcpy(type, t);
    strcpy(path, relative);

    free(temp);
    free(relative);
}

void * handle_connection(void * p_client_socket){
    int client_socket = *((int *)p_client_socket);
    free(p_client_socket);
    char buffer[BUFFERSIZE];
    size_t bytes_read;
    int msgsize = 0;
    char type[256], path[PATH_MAX+1];
    char actual_path[PATH_MAX+1];
    
    while((bytes_read = recv(client_socket, buffer+msgsize, sizeof(buffer)-msgsize-1, 0))> 0){
        msgsize += bytes_read;
        if(bytes_read > BUFFERSIZE - 1 || buffer[msgsize-1] == '\n'){
            break;
        }
    }
    
    check((int)bytes_read, "recv error");
    buffer[msgsize-1] = 0;
    
    parseMessage(buffer, type, path);
    fflush(stdout);
    
    if(realpath(path, actual_path) == NULL){
        printf("ERROR(bad path): %s\n", buffer);
        close(client_socket);
        return NULL;
    }
    if(strcmp(type, "POST") == 0){
        handlePOST(actual_path, client_socket);
    }
    else if(strcmp(type, "GET") == 0){
        handleGET(actual_path, client_socket);
    }
    close(client_socket);
    printf("closing connection\n");
    return NULL;
}

int main(int argc, const char * argv[]) {

    int server_socket, client_socket, addr_size;
    SA_IN server_address, client_addr;

    check(
        (server_socket = socket(AF_INET, SOCK_STREAM, 0)),
        "Failed to create Socket!");
    
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(atoi(argv[1]));
    
    check(
        bind(
            server_socket, 
            (SA*)&server_address,
            sizeof(server_address)
            ), 
        "Bind Failed!");

    check(
        listen(server_socket, BACKLOG), 
        "Listen Failed!");

    while(true){
        printf("Waiting for connections...\n");
        addr_size = sizeof(SA_IN);
        check(
            client_socket = accept(
                server_socket,
                (SA*)&client_addr,
                (socklen_t*)&addr_size), 
            "Accept Failed");
        printf("Connected!\n");
        
        pthread_t t;
        int *p_client = malloc(sizeof(int));
        *p_client = client_socket;
        pthread_create(&t, NULL, handle_connection, p_client);
    }
    return 0;
}
