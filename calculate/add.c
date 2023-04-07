#include "csapp.h"

int main(void)
{
    char *buf, *p;
    int n1, n2, sum;

    // 从环境变量QUERY_STRING中获取参数
    if ((buf = getenv("QUERY_STRING")) == NULL)
    {
        printf("QUERY_STRING is empty\n");
        exit(0);
    }

    // 解析参数
    if ((p = strchr(buf, '&')) == NULL)
    {
        printf("invalid query string: %s\n", buf);
        exit(0);
    }
    *p = '\0';
    n1 = atoi(buf);
    n2 = atoi(p + 1);

    // 计算和并输出结果
    sum = n1 + n2;
    // printf("Content-type: text/html\r\n\r\n");
    // printf("<html><head><title>Sum</title></head>\n");
    // printf("<body><p>%d + %d = %d</p></body></html>\n", n1, n2, sum);
    printf("HTTP/1.0 200 OK\r\n"); 
    printf("Content-type:application/json\r\n\r\n");
    printf("sum=%d", sum);
    fflush(stdout);

    return 0;
}