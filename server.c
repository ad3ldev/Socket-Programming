#include "common.h"


int main(int argc, char const *argv[])
{
    int listen_fd, conn_fd, n;
    SA_IN serv_addr;
    uint8_t buff[MAX_LINE+1];
    uint8_t recv_line[MAX_LINE + 1];

    if((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        err_n_die("Socket error.");

        bzero(&serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        serv_addr.sin_port = htons(SERVER_PORT);

    if((bind(listen_fd, (SA*) &serv_addr, sizeof(serv_addr))) < 0)
        err_n_die("Bind error.");

    if((listen(listen_fd, 10)) < 0)
        err_n_die("Listen error.");

    while(true){
        SA_IN addr;
        socklen_t addr_len;

        printf("Waiting for connection on port %d\n", SERVER_PORT);
        fflush(stdout);
        conn_fd = accept(listen_fd, (SA *) NULL, NULL);

        memset(recv_line, 0, MAX_LINE);
        while((n = recv(conn_fd, recv_line, MAX_LINE-1, 0))> 0){
            fprintf(stdout, "\n%s\n\n%s", bin2hex(recv_line, n), recv_line);
            if(recv_line[n-1]=='\n'){
                break;
            }
            if(n < MAX_LINE - 1){
                break;
            }
            memset(recv_line, 0, MAX_LINE);
        }
        if(n < 0){
            err_n_die("Read error");
        }

        snprintf((char*) buff, sizeof(buff), "HTTP/1.0 200 OK \r\n\r\nHello, World");

        write(conn_fd, (char *) buff, strlen((char *)buff));
        close(conn_fd);
    }
    return 0;
}
