#include "csapp.h"

int listenfd;
// #define PORT 80 // 服务器默认端口号

/* 函数声明 */
void *handle_client(void *arg);
void sigint_handler(int sig)
{
    close(listenfd);
    printf("\nProgram is terminated.\n");
    exit(0);
}
int main(int argc, char **argv)
{
    signal(SIGTSTP, sigint_handler);
    signal(SIGINT, sigint_handler);
    int connfd;                            // 监听套接字和连接套接字描述符char hostname[MAXLINE], port[MAXLINE]; // 客户端主机名与端口号
    char hostname[MAXLINE], port[MAXLINE]; // 客户端主机名与端口号
    socklen_t clientlen;                   // 记录客户端地址长度
    struct sockaddr_storage clientaddr;    // 存储客户端地址信息的结构体

    if (argc != 2) // 命令行参数检查
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]); // 输出错误提示信息
        exit(1);
    }

    listenfd = Open_listenfd(argv[1]); // 创建监听套接字并返回描述符
    while (1)                          // 循环监听并处理客户端请求
    {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);                       // 接受客户端请求，返回连接描述符
        Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0); // 获取客户端主机名和端口号
        printf("Accepted connection from (%s, %s)\n", hostname, port);                  // 打印客户端信息

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, &connfd) != 0)
        {
            perror("pthread_create failed");
            exit(EXIT_FAILURE);
        }
    }
}