#include "csapp.h"

/* 函数声明 */
void doit(int fd); // 请求处理

void read_requesthdrs(rio_t *rp); // 读取并忽略请求头部

int parse_uri(const char *uri, char *filename, char *cgiargs); // 解析URI

void serve_static(int fd, const char *filename, int filesize); // 处理静态内容请求

void get_filetype(const char *filename, char *filetype); // 获取请求文件的MIME类型

void serve_dynamic(int fd, const char *filename, const char *cgiargs); // 处理动态内容请求

void clienterror(int fd, const char *cause, const char *errnum, const char *shortmsg, const char *longmsg); // 发送错误响应给客户端

void *handle_client(void *arg)
{
    int fd = *(int *)arg;

    pthread_t tid = pthread_self();        // 获取当前线程 ID
    printf("Thread ID: %ld\n", (long)tid); // 打印当前线程号

    pthread_detach(tid);

    doit(fd); // 调用doit函数处理客户端请求
    close(fd);
    return NULL;
}

/* 处理HTTP请求 */
void doit(int fd)
{
    int is_static;    // 标记是否为静态内容请求
    struct stat sbuf; // 标记文件状态

    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE]; // 定义字符数组用于存储HTTP请求的内容
    char filename[MAXLINE], cgiargs[MAXLINE];                           // 定义字符数组用于存储服务器上要读取或执行的文件名和CGI参数
    rio_t rio;                                                          // 定义一个rio结构体，用于读取HTTP请求的内容

    Rio_readinitb(&rio, fd); // 初始化rio结构体

    /* 解析请求行 */
    if (!Rio_readlineb(&rio, buf, MAXLINE)) // 读取HTTP请求的第一行，如果没有读到数据，直接返回
        return;
    printf("%s", buf);                             // 在服务器上打印HTTP请求行
    sscanf(buf, "%s %s %s", method, uri, version); // 解析HTTP请求行，将请求行的三个元素分别存储到method、uri、version数组中

    if (!strcasecmp(method, "GET")) // HTTP请求方法为GET
    {
        read_requesthdrs(&rio); // 读取HTTP请求头部信息

        is_static = parse_uri(uri, filename, cgiargs); // 解析URI，获取文件名和CGI参数，根据返回值判断请求是否为静态内容请求

        if (is_static) // 处理静态内容请求
        {
            if (stat(filename, &sbuf) < 0) // 获取文件状态结构体，如果失败，返回404状态码
            {
                clienterror(fd, filename, "404", "Not found", "Book couldn't find this file");
                return;
            }
            if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) // 判断文件是否为普通文件,并且当前用户是否拥有读取该文件的权限
            {
                clienterror(fd, filename, "403", "Forbidden", "Book sever couldn't read the file");
                return;
            }
            serve_static(fd, filename, sbuf.st_size); // 处理静态内容请求
        }
        else // 处理动态内容请求
        {
            // if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) // 判断文件是否为普通文件,并且当前用户是否拥有读取该文件的权限
            // {
            //     clienterror(fd, filename, "403", "Forbidden", "Book sever couldn't run the CGI program");
            //     return;
            // }
            serve_dynamic(fd, filename, cgiargs); // 处理动态内容请求
        }
    }
    else if (!strcasecmp(method, "POST")) // HTTP请求方法为POST
    {
        sqlite3 *db;
        char *err_msg = NULL;
        int rc, n;

        char *type = NULL;
        int length;
        char *username = NULL, *password = NULL, *email = NULL, *email_suffix = NULL;

        /* 打开数据库 */
        if (rc = sqlite3_open("userinfo.db", &db) != SQLITE_OK)
        {
            fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
            sqlite3_close(db);
            exit(1);
        }

        /* 读取HTTP请求的信息体 */
        for (;;)
        {
            Rio_readlineb(&rio, buf, MAXLINE);
            printf("%s", buf);                           // 打印读取的行
            if (!strcmp(buf, "\r\n"))
                break;
            if (strstr(buf, "Content-Length: ") != NULL) // 抓取接收的表单长度
            {
                char *temp;
                type = index(buf, ':'); // 在buf中定位":"字符的位置
                strcpy(temp, type + 2); // 把": "后面的部分保存下来(注意：有个空格)
                length = atoi(temp);
            }
        }
        Rio_readlineb(&rio, buf, length + 1); // 读取HTTP请求的最后一行,即用户信息
        printf("%s\n\n", buf);

        if (!strcmp(uri, "/home.html"))
        {
            /* 在 HTTP 请求中查找用户名和密码 */
            username = strstr(buf, "username=");
            password = strstr(buf, "password=");

            if (username && password) // 如果在请求表单中找到了用户名和密码
            {
                /* 解析表单信息，提取用户名和密码 */
                char *user = strtok(username, "=");
                user = strtok(NULL, "&");

                char *pass = strtok(password, "=");
                pass = strtok(NULL, "&");

                char *sql1 = "SELECT * FROM users WHERE username=? AND password=?;"; // 构造查询语句
                sqlite3_stmt *stmt;
                sqlite3_prepare_v2(db, sql1, -1, &stmt, NULL); // 编译查询语句

                /* 绑定参数 */
                sqlite3_bind_text(stmt, 1, user, -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 2, pass, -1, SQLITE_STATIC);

                rc = sqlite3_step(stmt); // 执行sql查询语句
                sqlite3_finalize(stmt);  // 释放占用资源

                if (rc != SQLITE_ROW) // 若未查询到账号或密码
                {
                    clienterror(fd, "用户名或密码错误！！！", "401", "Unauthorized", "登录失败");
                    return;
                }
            }
            else
            {
                clienterror(fd, "服务器未能识别用户名或密码！！！", "400", "Bad Request", "请求失败");
                return;
            }

            parse_uri(uri, filename, cgiargs); // 解析URI，获取文件名

            if (stat(filename, &sbuf) < 0) // 获取文件状态结构体，如果失败，返回404状态码
            {
                clienterror(fd, filename, "404", "Not Found", "Book sever couldn't find this file");
                return;
            }
            if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) // 判断文件是否为普通文件,并且当前用户是否拥有读取该文件的权限
            {
                clienterror(fd, filename, "409", "Forbidden", "Book sever couldn't read the file");
                return;
            }
            serve_static(fd, filename, sbuf.st_size); // 作为静态文件处理
        }
        else if (!strcmp(uri, "/user.html"))
        {
            /* 在 HTTP 请求中查找用户名、密码、邮箱名、邮箱后缀 */
            username = strstr(buf, "username=");
            password = strstr(buf, "password=");
            email = strstr(buf, "emailname=");
            email_suffix = strstr(buf, "email_suffix=");

            if (username && password && email && email_suffix)
            {
                char *user = strtok(username, "=");
                user = strtok(NULL, "&");

                char *pass = strtok(password, "=");
                pass = strtok(NULL, "&");

                char *em = strtok(email, "=");
                em = strtok(NULL, "&");

                /* 测过程试中发现无法识别‘@’符号，‘@’符号被转为‘%40’，于是由服务器进行截取拼接处理*/
                char *em_su = strtok(email_suffix, "=");
                em_su = strtok(NULL, "&");
                strcpy(em_su, em_su + 3);

                /* 拼接邮箱 */
                strcat(em, "@");
                strcat(em, em_su);

                char *sql2 = "SELECT * FROM users WHERE username=?;"; // 构造查询语句，判断该用户是否已存在
                sqlite3_stmt *stmt1;
                if (rc = sqlite3_prepare_v2(db, sql2, -1, &stmt1, NULL) != SQLITE_OK) // 编译查询语句
                {
                    fprintf(stderr, "无法编译 SQL 语句: %s\n", sqlite3_errmsg(db));
                    sqlite3_close(db);
                }
                sqlite3_bind_text(stmt1, 1, user, -1, SQLITE_STATIC); // 绑定参数

                rc = sqlite3_step(stmt1); // 执行查询
                sqlite3_finalize(stmt1);  // 释放占用资源

                if (rc != SQLITE_ROW) // 若用户已存在
                {
                    char *sql3 = "INSERT INTO users (username, password, email) VALUES (?, ?, ?)"; // 构建 SQL 语句
                    sqlite3_stmt *stmt2;

                    if (rc = sqlite3_prepare_v2(db, sql3, -1, &stmt2, NULL) != SQLITE_OK) // 编译插入语句
                    {
                        fprintf(stderr, "无法编译 SQL 语句: %s\n", sqlite3_errmsg(db));
                        sqlite3_close(db);
                    }

                    /* 绑定参数 */
                    sqlite3_bind_text(stmt2, 1, user, -1, SQLITE_STATIC);
                    sqlite3_bind_text(stmt2, 2, pass, -1, SQLITE_STATIC);
                    sqlite3_bind_text(stmt2, 3, em, -1, SQLITE_STATIC);

                    rc = sqlite3_step(stmt2);
                    sqlite3_finalize(stmt2);

                    if (rc != SQLITE_DONE)
                    {
                        fprintf(stderr, "无法执行 SQL 语句: %s\n", sqlite3_errmsg(db));
                        sqlite3_close(db);
                        clienterror(fd, "服务器错误！！！", "500", "Internal Server Error", "注册失败");
                        return;
                    }
                    else
                    {
                        strcpy(filename, "register_success.html");
                        stat(filename, &sbuf);
                        serve_static(fd, filename, sbuf.st_size);
                    }
                }
                else
                {
                    clienterror(fd, "该用户已存在！！！", "409", "Conflict", "注册失败");
                    return;
                }
            }
            else
            { // 查找失败
                clienterror(fd, "服务器未解析到用户名、密码或邮箱！！！", "400", "Bad Request", "请求失败");
                return;
            }
        }
        else
        {
            clienterror(fd, uri, "404", "Not Found", "Book sever couldn't find this file");
            return;
        }
    }
    else
    {
        /* 未知请求 */
        clienterror(fd, method, "501", "Not Implemented", "Book sever does not implement this method");
        return;
    }
}

void read_requesthdrs(rio_t *rp)
{
    char buf[MAXLINE];

    Rio_readlineb(rp, buf, MAXLINE); // 读取HTTP请求的第二行
    printf("%s", buf);               // 打印输出读取的第二行
    while (strcmp(buf, "\r\n"))      // 判断当前请求行是否为单独的换行符，以表示当前请求命令输入完
    {
        Rio_readlineb(rp, buf, MAXLINE); // 继续读取HTTP请求的下一行
        printf("%s", buf);               // 打印输出读取的行
    }
    return;
}

// 解析URI并将解析结果存储到filename和cgiargs指向的字符串中
// 参数uri是待解析的URI字符串
// 参数filename是存储解析出来的文件路径的字符串指针
// 参数cgiargs是存储解析出来的CGI参数的字符串指针
// 返回值为一个整型数，表示是否解析出了CGI程序
int parse_uri(const char *uri, char *filename, char *cgiargs)
{
    char *ptr;

    if (!strstr(uri, "calculate/add?")) // 如果URI中不包含"?"子串，说明是静态内容
    {
        /* 静态内容 */
        strcpy(cgiargs, "");   // 清空CGI参数
        strcpy(filename, "."); // 将"."作为文件路径的起始点
        strcat(filename, uri); // 将URI拼接到文件路径后面

        if (uri[strlen(uri) - 1] == '/')    // 如果URI以"/"结尾
            strcat(filename, "index.html"); // 在文件路径后面拼接上默认的文件名"index.html",即主页
        return 1;                           // 返回1，表示解析出的是静态内容
    }
    else
    {
        /* 动态内容 */
        ptr = index(uri, '?'); // 在URI中定位"?"字符的位置

        if (ptr) // 如果“？”存在
        {
            strcpy(cgiargs, ptr + 1); // 把"?"后面的部分作为CGI参数保存下来
            *ptr = '\0';              // 将"?"替换为NULL，截断URI字符串
        }
        else
            strcpy(cgiargs, ""); // 清空CGI参数
        strcpy(filename, ".");   // 将"."作为文件路径的起始点
        strcat(filename, uri);   // 将URI拼接到文件路径后面
        return 0;                // 返回0，表示解析出的是动态内容
    }
}

void serve_static(int fd, const char *filename, int filesize)
{
    int srcfd; // 存储打开文件的文件描述符
    char *srcp, filetype[MAXLINE], buf[MAXBUF];

    /* 发送响应报头给客户端 */
    get_filetype(filename, filetype);                                 // 获取文件类型，存储到filetype数组中
    sprintf(buf, "HTTP/1.0 200 OK\r\n");                              // 构造响应报头：状态行
    Rio_writen(fd, buf, strlen(buf));                                 // 将响应报头写入套接字缓冲区
    sprintf(buf, "Server: Book Web Server\r\n");                      // 构造响应报头：服务器信息
    Rio_writen(fd, buf, strlen(buf));                                 // 将响应报头写入套接字缓冲区
    sprintf(buf, "Content-length: %d\r\n", filesize);                 // 构造响应报头：文件长度
    Rio_writen(fd, buf, strlen(buf));                                 // 将响应报头写入套接字缓冲区
    snprintf(buf, sizeof(buf), "Content-type: %s\r\n\r\n", filetype); // 构造响应报头：文件类型
    Rio_writen(fd, buf, strlen(buf));                                 // 将响应报头写入套接字缓冲区

    /* 发送响应正文给客户端 */
    srcfd = Open(filename, O_RDONLY, 0);                        // 以只读方式打开请求的文件，返回文件描述符
    srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0); // 将文件映射到进程的地址空间中
    Close(srcfd);                                               // 关闭文件描述符
    Rio_writen(fd, srcp, filesize);                             // 发送文件数据到客户端
    Munmap(srcp, filesize);                                     // 取消文件映射
}

/* 获取请求文件类型 */
void get_filetype(const char *filename, char *filetype)
{
    if (strstr(filename, ".html"))
        strcpy(filetype, "text/html");
    else if (strstr(filename, ".gif"))
        strcpy(filetype, "image/gif");
    else if (strstr(filename, ".png"))
        strcpy(filetype, "image/png");
    else if (strstr(filename, ".jpg"))
        strcpy(filetype, "image/jpeg");
    else if (strstr(filename, ".ico"))
        strcpy(filetype, "image/ico");
    else
        strcpy(filetype, "text/plain");
}

void serve_dynamic(int fd, const char *filename, const char *cgiargs)
{
    char buf[MAXLINE], *emptylist[] = {NULL};

    if (Fork() == 0) // 如果fork()的返回值为0，说明当前处于子进程中
    {
        /* 子进程 */
        /* 在真实的服务器中，需要在此处设置所有 CGI 环境变量 */
        setenv("QUERY_STRING", cgiargs, 1);   // 调用setenv()函数来设置QUERY_STRING环境变量，以便将查询字符串传递给CGI程序。
        Dup2(fd, STDOUT_FILENO);              // 将标准输出重定向到客户端套接字描述符
        Execve(filename, emptylist, environ); // 运行CGI程序
    }
    Wait(NULL); // 父进程等待子进程结束并回收其资源
}

void clienterror(int fd, const char *cause, const char *errnum, const char *shortmsg, const char *longmsg)
{
    char buf[MAXLINE];
    /* 发送响应报头给客户端 */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n\r\n");
    Rio_writen(fd, buf, strlen(buf));

    /* 发送错误响应给客户端 */
    sprintf(buf, "<!DOCTYPE html>\n"
                 "<html>\n"
                 "<head>\n"
                 "<title>Eerror</title>\n"
                 "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\" />\n"
                 "<style>\n"
                 "body {\n"
                 "  font-family: Arial, sans-serif;\n"
                 "  background: linear-gradient(to bottom right, #c2b5b9, #397f91);\n"
                 "  background-repeat: no-repeat;\n"
                 "  background-position: center center;\n"
                 "  background-size: cover;\n"
                 "  background-attachment: fixed;"
                 "  animation: gradient 15s ease infinite;\n"
                 "}\n"
                 "@keyframes gradient {\n"
                 "  0% { background-position: 0% 50%; }\n"
                 "  50% { background-position: 100% 50%; }\n"
                 "  100% { background-position: 0% 50%; }\n"
                 "}\n"
                 "h1 {\n"
                 "  font-family: \"宋体\", STSongti, serif;\n"
                 "  color: #333;\n"
                 "  font-size: 35px;\n"
                 "  text-align: center;\n"
                 "  margin-bottom: 20px;\n"
                 "}\n"
                 "form {\n"
                 "  display: flex;\n"
                 "  flex-direction: column;\n"
                 "  margin-top: 50px;\n"
                 "  background-color: rgb(198, 217, 198);\n"
                 "  padding: 20px;\n"
                 "  border-radius: 5px;\n"
                 "  box-shadow: 0px 4px 6px rgba(0, 0, 0, 0.1);\n"
                 "}\n"
                 "span {\n"
                 "  font-family: \"宋体\", STSongti, serif;\n"
                 "  color: #000000;\n"
                 "  font-size: 40px;\n"
                 "}\n"
                 "p {\n"
                 "  font-size: 25px;\n"
                 "}\n"
                 ".center {\n"
                 "  margin: 0 auto;\n"
                 "  text-align: center;\n"
                 "}\n"
                 ".error {\n"
                 "  color: red;\n"
                 "  margin-bottom: 20px;\n"
                 "}\n"
                 "</style>\n"
                 "<script>\n"
                 "  window.onload = function () {\n"
                 "    var countDown = 5;\n"
                 "    var countdownElement = document.getElementById('countdown');\n"
                 "    var intervalId = setInterval(function () {\n"
                 "      countDown--;\n"
                 "      if (countDown <= 0) {\n"
                 "        clearInterval(intervalId);\n"
                 "        window.location.href = 'index.html';\n"
                 "      }\n"
                 "      countdownElement.innerHTML = countDown;\n"
                 "    }, 1000);\n"
                 "  };\n"
                 "</script>\n"
                 "</head>\n"
                 "<body>\n"
                 "<div class=\"center\" id=\"register-form\" style=\"height:50%;width:35%;\">\n"
                 "<form>\n"
                 "<h1 class=\"error\">%s: %s</h1>\n"
                 "<p class=\" center \">%s: %s</p>\n"
                 "<p class=\"center\" > <span id = \"countdown\">5</span> S</p>\n "
                 "<p class=\" center \">5秒后将自动返回主页</p>\n"
                 "<hr><em>The Book Web server</em>\r\n"
                 "</form>\n"
                 "</div>\n"
                 "</body>\n"
                 "</html>\n",
            errnum, shortmsg, longmsg, cause);
    Rio_writen(fd, buf, strlen(buf));
}