#include "common.h"
#include <pthread.h>

#define BACKLOG 100
#define OK_MESSAGE "HTTP/1.1 200 OK\r\n\r\n"
#define NOTFOUND_MESSAGE "HTTP/1.1 404 Not Found\r\n\r\n"

void sendMessage(int client_socket, char *msg){
    check(send(client_socket, msg, strlen(msg), 0), "Unable to send HTTP message!");
}

void sendFile(char *path, int client_socket){
    FILE *fp = fopen(path, "rb");
    if(fp == NULL){
        printf("ERROR(open): %s\n", path);
        send(client_socket, NOTFOUND_MESSAGE, strlen(NOTFOUND_MESSAGE), 0);
        close(client_socket);
        return;
    }
    sendMessage(client_socket, OK_MESSAGE);
    printf("Sending File %s...\n", path);

    char buffer[BUFFERSIZE];
    size_t bytes_read;
    memset(buffer,0,BUFFERSIZE);
    while ((bytes_read = fread(buffer, 1, BUFFERSIZE, fp))> 0){
        printf("sending %zu bytes\n", bytes_read);
        send(client_socket, buffer, bytes_read, 0);
        memset(buffer,0,BUFFERSIZE);
        if(bytes_read < BUFFERSIZE){
            close(client_socket);
            fclose(fp);
            break;
        }
    }
    printf("File Sent Successfully.\n");
    close(client_socket);
    fclose(fp);
    printf("closing connection\n");
}

void receiveFile(char *buffer, char *path, int client_socket){
    FILE *fp = fopen(path, "wb");
    if(fp == NULL){
        printf("ERROR(open): %s\n", path);
        send(client_socket, NOTFOUND_MESSAGE, strlen(NOTFOUND_MESSAGE), 0);
        close(client_socket);
        return;
    }
    printf("Receiving File %s...\n", path);

    char * writing_buffer = strstr(buffer,"\r\n\r\n");
    size_t bytes_read = strlen(writing_buffer);
    if(strlen(buffer) < BUFFERSIZE){
        printf("receiving %zu bytes\n", bytes_read);
                fwrite(buffer, 1, bytes_read, fp);
                memset(buffer,0,BUFFERSIZE);
                if(bytes_read < BUFFERSIZE){
                    close(client_socket);
                    fclose(fp);
                }
    }else{
        while ((bytes_read = recv(client_socket, buffer, BUFFERSIZE, 0))> 0){
                printf("receiving %zu bytes\n", bytes_read);
                fwrite(buffer, 1, bytes_read, fp);
                memset(buffer,0,BUFFERSIZE);
                if(bytes_read < BUFFERSIZE){
                    close(client_socket);
                    fclose(fp);
                    break;
                }
            }
    }
    
    printf("File Written Successfully.\n");
    sendMessage(client_socket, OK_MESSAGE);
    close(client_socket);
    fclose(fp);
    printf("closing connection\n");
}

void handleGET(char *path, int client_socket){
    sendFile(path, client_socket);
}
void handlePOST(char *buffer,char *path, int client_socket){
    receiveFile(buffer,path, client_socket);
}

void parseMessage(char *msg, char *type, char *path){
    char * temp = malloc(strlen(msg));
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

    free(relative);
}

void * handle_connection(void * p_client_socket){
    int client_socket =*((int *)p_client_socket);
    free(p_client_socket);
    char buffer[BUFFERSIZE];
    size_t bytes_read;
    int msgsize = 0;
    char type[256], path[PATH_MAX+1];
    char actual_path[PATH_MAX+1];

    while((bytes_read = recv(client_socket, buffer+msgsize, sizeof(buffer)-msgsize-1, 0))>0){
        msgsize += bytes_read;
        if(msgsize < BUFFERSIZE - 1 || buffer[msgsize-1] == '\n'){
            break;
        }
    }
    check(bytes_read, "recv error");
    buffer[msgsize -1] = 0;

    printf("REQUEST: %s\n", buffer);
    fflush(stdout);

    parseMessage(buffer, type, path);
    if(strcmp(type, "POST") == 0){
        realpath(path, actual_path);
        handlePOST(buffer,actual_path, client_socket);
    }
    else if(strcmp(type, "GET") == 0){
        if(realpath(path, actual_path) == NULL){
            printf("ERROR(bad path): %s\n", actual_path);
            close(client_socket);
            return NULL;
        }
        handleGET(actual_path, client_socket);
    }
    return NULL;
}

int main(int argc, char const *argv[]){
    if(argc != 2){
        printf("Invalid num of arguments\n");
        exit(1);
    }
    int server_socket, client_socket, addr_size;
    SA_IN server_addr, client_addr;

    check((server_socket = socket(AF_INET, SOCK_STREAM, 0)), "Failed to create Socket.");

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(atoi(argv[1]));

    check(bind(server_socket,(SA *)&server_addr, sizeof(server_addr)), "Bind Failed!");
    check(listen(server_socket, BACKLOG), "Listen Failed!");

    while(true){
        printf("Waiting for connections...\n");
        addr_size = sizeof(SA_IN);
        check(client_socket = accept(server_socket, (SA*)&client_addr, (socklen_t*)&addr_size), "Accept Failed!");
        printf("Connected!\n");

        pthread_t t;
        int * p_client = malloc(sizeof(int));
        *p_client = client_socket;
        pthread_create(&t, NULL, handle_connection, p_client);
    }


    return 0;
}
