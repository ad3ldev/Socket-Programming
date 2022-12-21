#include "common.h"

int main(int argc, char const *argv[])
{
    int sockfd,n;
    int send_bytes;
    SA_IN serv_addr;
    char send_line[MAX_LINE];
    char recv_line[MAX_LINE];

    if(argc != 2)
        err_n_die("usage: %s <server address>", argv[0]);

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        err_n_die("Error while creating the socket!");

    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, argv[1], &serv_addr.sin_addr) <=0)
        err_n_die("inet_pton error for %s", argv[1]);

    if (connect(sockfd, (SA *) &serv_addr, sizeof(serv_addr)) < 0)
        err_n_die("Connect failed!");

    sprintf(send_line, "GET / HTTP/1.1\r\n\r\n");
    send_bytes = strlen(send_line);

    if(send(sockfd, send_line, send_bytes, 0) != send_bytes)
        err_n_die("Write error");

    while((n = recv(sockfd, recv_line, MAX_LINE - 1, 0)) > 0){
        printf("%s", recv_line);
        memset(recv_line, 0, MAX_LINE);
        if(n < MAX_LINE - 1){
            break;
        }
    }
    if(n<0)
        err_n_die("Read Error");

    return 0;
}
