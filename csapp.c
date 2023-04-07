#include "csapp.h"

static void sio_reverse(char s[])
{
    int c, i, j;

    for (i = 0, j = strlen(s) - 1; i < j; i++, j--)
    {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

/* sio_ltoa - Convert long to base b string (from K&R) */
static void sio_ltoa(long v, char s[], int b)
{
    int c, i = 0;
    int neg = v < 0;

    if (neg)
        v = -v;
    do
    {
        s[i++] = ((c = (v % b)) < 10) ? c + '0' : c - 10 + 'a';
    } while ((v /= b) > 0);

    if (neg)
        s[i++] = '-';

    s[i] = '\0';
    sio_reverse(s);
}

/* sio_strlen - Return length of string (from K&R) */
static size_t sio_strlen(const char s[])
{
    int i = 0;

    while (s[i] != '\0')
        ++i;
    return i;
}
/* $end sioprivate */

/* Public Sio functions */
/* $begin siopublic */

ssize_t sio_puts(const char s[]) /* Put string */
{
    return write(STDOUT_FILENO, s, sio_strlen(s)); // line:csapp:siostrlen
}

ssize_t sio_putl(long v) /* Put long */
{
    char s[128];

    sio_ltoa(v, s, 10); /* Based on K&R itoa() */ // line:csapp:sioltoa
    return sio_puts(s);
}

void sio_error(const char s[]) /* Put error message and exit */
{
    sio_puts(s);
    _exit(1); // line:csapp:sioexit
}
/* $end siopublic */

/*******************************
 * Wrappers for the SIO routines
 ******************************/
ssize_t Sio_putl(long v)
{
    ssize_t n;

    if ((n = sio_putl(v)) < 0)
        sio_error("Sio_putl error");
    return n;
}

ssize_t Sio_puts(const char s[])
{
    ssize_t n;

    if ((n = sio_puts(s)) < 0)
        sio_error("Sio_puts error");
    return n;
}

void Sio_error(const char s[])
{
    sio_error(s);
}

/********************************
 * Wrappers for Unix I/O routines
 ********************************/

int Open(const char *pathname, int flags, mode_t mode)
{
    int rc;

    if ((rc = open(pathname, flags, mode)) < 0)
        unix_error("Open error");
    return rc;
}

ssize_t Read(int fd, void *buf, size_t count)
{
    ssize_t rc;

    if ((rc = read(fd, buf, count)) < 0)
        unix_error("Read error");
    return rc;
}

ssize_t Write(int fd, const void *buf, size_t count)
{
    ssize_t rc;

    if ((rc = write(fd, buf, count)) < 0)
        unix_error("Write error");
    return rc;
}

off_t Lseek(int fildes, off_t offset, int whence)
{
    off_t rc;

    if ((rc = lseek(fildes, offset, whence)) < 0)
        unix_error("Lseek error");
    return rc;
}

void Close(int fd)
{
    int rc;

    if ((rc = close(fd)) < 0)
        unix_error("Close error");
}

int Select(int n, fd_set *readfds, fd_set *writefds,
           fd_set *exceptfds, struct timeval *timeout)
{
    int rc;

    if ((rc = select(n, readfds, writefds, exceptfds, timeout)) < 0)
        unix_error("Select error");
    return rc;
}

int Dup2(int fd1, int fd2)
{
    int rc;

    if ((rc = dup2(fd1, fd2)) < 0) // 调用系统函数dup2，将fd1的值复制到fd2中，并且将返回的新文件描述符赋值给rc。
        unix_error("Dup2 error");
    return rc;
}

void Stat(const char *filename, struct stat *buf)
{
    if (stat(filename, buf) < 0)
        unix_error("Stat error");
}

void Fstat(int fd, struct stat *buf)
{
    if (fstat(fd, buf) < 0)
        unix_error("Fstat error");
}

/*********************************
 * Wrappers for directory function
 *********************************/

DIR *Opendir(const char *name)
{
    DIR *dirp = opendir(name);

    if (!dirp)
        unix_error("opendir error");
    return dirp;
}

struct dirent *Readdir(DIR *dirp)
{
    struct dirent *dep;

    errno = 0;
    dep = readdir(dirp);
    if ((dep == NULL) && (errno != 0))
        unix_error("readdir error");
    return dep;
}

int Closedir(DIR *dirp)
{
    int rc;

    if ((rc = closedir(dirp)) < 0)
        unix_error("closedir error");
    return rc;
}

/*
 * addr：映射的起始地址，如果传递的是NULL则由系统自动分配。
 * len：映射区域的长度，以字节为单位。
 * prot：映射区域的保护方式，可以是以下值之一或它们的按位或：PROT_READ：可读||PROT_WRITE：可写||PROT_EXEC：可执行||PROT_NONE：不可访问。
 * flags：影响内存映射操作的标志位，可以是以下值之一或它们的按位或：MAP_SHARED：共享映射||MAP_PRIVATE：私有映射||MAP_FIXED：强制指定映射的开始地址||MAP_ANONYMOUS：匿名映射，不需要文件支持。
 * fd：被映射的文件描述符，如果映射的是匿名内存则传递-1。
 * offset：从文件起始位置偏移量，以字节为单位。
 */
void *Mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset)
{
    void *ptr;

    if ((ptr = mmap(addr, len, prot, flags, fd, offset)) == ((void *)-1)) // 使用mmap系统调用将文件或设备映射到内存中
        unix_error("mmap error");
    return (ptr);
}

/*
 * start：要解除映射的内存区域的起始地址，必须是映射操作返回的指针。
 * length：要解除映射的内存区域的长度，以字节为单位。
 */
void Munmap(void *start, size_t length)
{
    if (munmap(start, length) < 0)  // 使用munmap系统调用解除由start指针所指向的内存区域的映射关系，并释放相关的资源
        unix_error("munmap error"); // 如果返回值小于0则表示解除映射失败，因此抛出一个错误
}

/***************************************************
 * Wrappers for dynamic storage allocation functions
 ***************************************************/

void *Malloc(size_t size)
{
    void *p;

    if ((p = malloc(size)) == NULL)
        unix_error("Malloc error");
    return p;
}

void *Realloc(void *ptr, size_t size)
{
    void *p;

    if ((p = realloc(ptr, size)) == NULL)
        unix_error("Realloc error");
    return p;
}

void *Calloc(size_t nmemb, size_t size)
{
    void *p;

    if ((p = calloc(nmemb, size)) == NULL)
        unix_error("Calloc error");
    return p;
}

void Free(void *ptr)
{
    free(ptr);
}

/******************************************
 * Wrappers for the Standard I/O functions.
 ******************************************/
void Fclose(FILE *fp)
{
    if (fclose(fp) != 0)
        unix_error("Fclose error");
}

FILE *Fdopen(int fd, const char *type)
{
    FILE *fp;

    if ((fp = fdopen(fd, type)) == NULL)
        unix_error("Fdopen error");

    return fp;
}

char *Fgets(char *ptr, int n, FILE *stream)
{
    char *rptr;

    if (((rptr = fgets(ptr, n, stream)) == NULL) && ferror(stream))
        app_error("Fgets error");

    return rptr;
}

FILE *Fopen(const char *filename, const char *mode)
{
    FILE *fp;

    if ((fp = fopen(filename, mode)) == NULL)
        unix_error("Fopen error");

    return fp;
}

void Fputs(const char *ptr, FILE *stream)
{
    if (fputs(ptr, stream) == EOF)
        unix_error("Fputs error");
}

size_t Fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    size_t n;

    if (((n = fread(ptr, size, nmemb, stream)) < nmemb) && ferror(stream))
        unix_error("Fread error");
    return n;
}

void Fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    if (fwrite(ptr, size, nmemb, stream) < nmemb)
        unix_error("Fwrite error");
}

/****************************
 * Sockets interface wrappers
 ****************************/
int Socket(int domain, int type, int protocol)
{
    int rc;

    if ((rc = socket(domain, type, protocol)) < 0)
        unix_error("Socket error");
    return rc;
}

int Setsockopt(int s, int level, int optname, const void *optval, int optlen)
{
    int rc;

    if ((rc = setsockopt(s, level, optname, optval, optlen)) < 0)
        unix_error("Setsockopt error");
    return rc;
}

int Bind(int sockfd, struct sockaddr *my_addr, int addrlen)
{
    int rc;

    if ((rc = bind(sockfd, my_addr, addrlen)) < 0)
        unix_error("Bind error");
    return rc;
}

int Listen(int s, int backlog)
{
    int rc;

    if ((rc = listen(s, backlog)) < 0)
        unix_error("Listen error");
    return rc;
}

int Accept(int s, struct sockaddr *addr, socklen_t *addrlen)
{
    int rc;

    if ((rc = accept(s, addr, addrlen)) < 0)
        unix_error("Accept error");
    return rc;
}

int Connect(int sockfd, struct sockaddr *serv_addr, int addrlen)
{
    int rc;

    if ((rc = connect(sockfd, serv_addr, addrlen)) < 0)
        unix_error("Connect error");
    return rc;
}

/*******************************
 * Protocol-independent wrappers
 *******************************/
void Getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res)
{
    int rc;

    if ((rc = getaddrinfo(node, service, hints, res)) != 0)
        gai_error(rc, "Getaddrinfo error");
}

void Getnameinfo(const struct sockaddr *sa, socklen_t salen, char *host, size_t hostlen, char *serv, size_t servlen, int flags)
{
    int rc;

    if ((rc = getnameinfo(sa, salen, host, hostlen, serv, servlen, flags)) != 0)
        gai_error(rc, "Getnameinfo error");
}

void Freeaddrinfo(struct addrinfo *res)
{
    freeaddrinfo(res);
}

void Inet_ntop(int af, const void *src, char *dst, socklen_t size)
{
    if (!inet_ntop(af, src, dst, size))
        unix_error("Inet_ntop error");
}

void Inet_pton(int af, const char *src, void *dst)
{
    int rc;

    rc = inet_pton(af, src, dst);
    if (rc == 0)
        app_error("inet_pton error: invalid dotted-decimal address");
    else if (rc < 0)
        unix_error("Inet_pton error");
}

/******************************************************************
 * DNS interface wrappers.
 *
 * NOTE: These are obsolete because they are not thread safe. Use
 * getaddrinfo and getnameinfo instead
 ******************************************************************/

struct hostent *Gethostbyname(const char *name)
{
    struct hostent *p;

    if ((p = gethostbyname(name)) == NULL)
        dns_error("Gethostbyname error");
    return p;
}

struct hostent *Gethostbyaddr(const char *addr, int len, int type)
{
    struct hostent *p;

    if ((p = gethostbyaddr(addr, len, type)) == NULL)
        dns_error("Gethostbyaddr error");
    return p;
}

/************************************************
 * Wrappers for Pthreads thread control functions
 ************************************************/

void Pthread_create(pthread_t *tidp, const pthread_attr_t *attrp, void *(*routine)(void *), void *argp)
{
    int rc;

    if ((rc = pthread_create(tidp, attrp, routine, argp)) != 0)
        posix_error(rc, "Pthread_create error");
}

void Pthread_cancel(pthread_t tid)
{
    int rc;

    if ((rc = pthread_cancel(tid)) != 0)
        posix_error(rc, "Pthread_cancel error");
}

void Pthread_join(pthread_t tid, void **thread_return)
{
    int rc;

    if ((rc = pthread_join(tid, thread_return)) != 0)
        posix_error(rc, "Pthread_join error");
}

void Pthread_detach(pthread_t tid)
{
    int rc;

    if ((rc = pthread_detach(tid)) != 0)
        posix_error(rc, "Pthread_detach error");
}

void Pthread_exit(void *retval)
{
    pthread_exit(retval);
}

pthread_t Pthread_self(void)
{
    return pthread_self();
}

void Pthread_once(pthread_once_t *once_control, void (*init_function)())
{
    pthread_once(once_control, init_function);
}

/*******************************
 * Wrappers for Posix semaphores
 *******************************/

void Sem_init(sem_t *sem, int pshared, unsigned int value)
{
    if (sem_init(sem, pshared, value) < 0)
        unix_error("Sem_init error");
}

void P(sem_t *sem)
{
    if (sem_wait(sem) < 0)
        unix_error("P error");
}

void V(sem_t *sem)
{
    if (sem_post(sem) < 0)
        unix_error("V error");
}

/****************************************
 * The Rio package - Robust I/O functions
 ****************************************/

ssize_t rio_readn(int fd, void *usrbuf, size_t n)
{
    size_t nleft = n;
    ssize_t nread;
    char *bufp = usrbuf;

    while (nleft > 0)
    {
        if ((nread = read(fd, bufp, nleft)) < 0)
        {
            if (errno == EINTR)            /* Interrupted by sig handler return */
                continue; /* nread = 0; */ /* and call read() again */
            else
                return -1; /* errno set by read() */
        }
        else if (nread == 0)
            break; /* EOF */
        nleft -= nread;
        bufp += nread;
    }
    return (n - nleft); /* Return >= 0 */
}

ssize_t rio_writen(int fd, const void *usrbuf, size_t n)
{
    size_t nleft = n;          // 记录待写入数据的大小
    ssize_t nwritten;          // 用于记录已成功写入的数据大小
    const char *bufp = usrbuf; // 指向要写入的数据缓冲区的指针

    while (nleft > 0)
    {
        if ((nwritten = write(fd, bufp, nleft)) <= 0) // 将bufp所指向的内容写入到套接字缓冲区
        {
            if (errno == EINTR) // 如果被信号处理器打断，则继续写入
                continue;
            else
                return -1;
        }
        nleft -= nwritten; // 减去已经成功写入的数据大小
        bufp += nwritten;  // 将指针移到下一个要写入的位置
    }
    return n;
}

/*
 * rio_read - This is a wrapper for the Unix read() function that
 *    transfers min(n, rio_cnt) bytes from an internal buffer to a user
 *    buffer, where n is the number of bytes requested by the user and
 *    rio_cnt is the number of unread bytes in the internal buffer. On
 *    entry, rio_read() refills the internal buffer via a call to
 *    read() if the internal buffer is empty.
 */
static ssize_t rio_read(rio_t *rp, char *usrbuf, size_t n)
{
    int cnt; // 用于记录实际读取的字节数

    while (rp->rio_cnt <= 0) // 如果内部缓冲区为空，则需要重新填充
    {
        rp->rio_cnt = read(rp->rio_fd, rp->rio_buf, sizeof(rp->rio_buf)); // read函数会从文件描述符rp->rio_fd指向的文件中读取数据，存储到rp->rio_buf指向的内部缓冲区中，并返回读取的字节数，以达到填充内部缓冲区效果

        if (rp->rio_cnt < 0) // 如果read函数返回值小于0，表示读取出错
        {
            if (errno != EINTR) // 如果错误不是由信号中断引起的，则直接返回-1
                return -1;
        }
        else if (rp->rio_cnt == 0) // 如果read函数返回0，表示已到达文件结尾，读取结束
            return 0;
        else
            rp->rio_bufptr = rp->rio_buf; // 缓冲区填充成功，更新内部缓冲区的指针rp->rio_bufptr，将其指向缓冲区的开头
    }

    cnt = n;             // 需要读取的字节数初始化为n
    if (rp->rio_cnt < n) // 如果内部缓冲区剩余字节数小于需要读取的字节数n，只需将所有剩余字节读取到用户缓冲区中
        cnt = rp->rio_cnt;
    memcpy(usrbuf, rp->rio_bufptr, cnt); // 使用memcpy函数将内部缓冲区中的cnt个字节拷贝到用户缓冲区usrbuf中去
    rp->rio_bufptr += cnt;               // 更新缓冲区指针
    rp->rio_cnt -= cnt;                  // 更新缓冲区剩余字节数
    return cnt;                          // 返回实际读取的字节数
}

/*
 * rio_readinitb - Associate a descriptor with a read buffer and reset buffer
 */
void rio_readinitb(rio_t *rp, int fd)
{
    rp->rio_fd = fd;
    rp->rio_cnt = 0;
    rp->rio_bufptr = rp->rio_buf;
}

/*
 * rio_readnb - Robustly read n bytes (buffered)
 */
ssize_t rio_readnb(rio_t *rp, void *usrbuf, size_t n)
{
    size_t nleft = n;
    ssize_t nread;
    char *bufp = usrbuf;

    while (nleft > 0)
    {
        if ((nread = rio_read(rp, bufp, nleft)) < 0)
            return -1; /* errno set by read() */
        else if (nread == 0)
            break; /* EOF */
        nleft -= nread;
        bufp += nread;
    }
    return (n - nleft); /* return >= 0 */
}

ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen)
{
    int n, rc;              // n表示当前已经读取的字符数，rc用于存储每次调用rio_read时返回的值
    char c, *bufp = usrbuf; // c用于暂存每次读取到的单个字符，bufp指向用户缓冲区中当前的位置

    /* 循环读取每个字符 */
    for (n = 1; n < maxlen; n++)
    {
        if ((rc = rio_read(rp, &c, 1)) == 1) // 调用rio_read读取一个字符到c，返回结果保存在rc中
        {
            *bufp++ = c;   // 如果成功读取到一个字符，则将其存储在用户缓冲区中，并更新bufp的指向位置
            if (c == '\n') // 如果当前字符是换行符，则说明已经读取完一行数据了
            {
                ++n;   // 将n加1，因为当前这个字符也被算作是已读取的字符之一
                break; // 跳出循环，结束函数调用
            }
        }
        else if (rc == 0) // 如果返回值rc==0，表示没有读取到任何字符（EOF）
        {
            if (n == 1) // 如果当前已读取的字符数为 1，则表示此次读取操作并未真正读取到任何数据，直接返回 0
                return 0;
            else // 如果已读取的字符数不为 1，则说明已经读取到了一些数据，现在已经读完了，正常退出循环
                break;
        }
        else // 如果返回值小于 0 则表示出现了错误，返回 -1
            return -1;
    }
    *bufp = 0;    // 在用户缓冲区的末尾加上字符串结束符 '\0'
    return n - 1; // 返回读取到的字符数减去最后那个换行符（如果有的话）
}

/**********************************
 * Wrappers for robust I/O routines
 **********************************/

ssize_t Rio_readn(int fd, void *ptr, size_t nbytes)
{
    ssize_t n;

    if ((n = rio_readn(fd, ptr, nbytes)) < 0)
        unix_error("Rio_readn error");
    return n;
}

void Rio_writen(int fd, const void *usrbuf, size_t n)
{
    if (rio_writen(fd, usrbuf, n) != n)
        unix_error("Rio_writen error");
}

void Rio_readinitb(rio_t *rp, int fd)
{
    rio_readinitb(rp, fd);
}

ssize_t Rio_readnb(rio_t *rp, void *usrbuf, size_t n)
{
    ssize_t rc;

    if ((rc = rio_readnb(rp, usrbuf, n)) < 0)
        unix_error("Rio_readnb error");
    return rc;
}

ssize_t Rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen)
{
    ssize_t rc;

    if ((rc = rio_readlineb(rp, usrbuf, maxlen)) < 0)
        unix_error("Rio_readlineb error");
    return rc;
}

/********************************
 * Client/server helper functions
 ********************************/

int open_clientfd(const char *hostname, const char *port)
{
    int clientfd, rc;                  // 定义变量：客户端文件描述符clientfd，返回值rc
    struct addrinfo hints, *listp, *p; // 定义结构体指针类型变量：hints、listp、p

    /* 初始化hints结构体 */
    memset(&hints, 0, sizeof(struct addrinfo));

    /* 设置hints结构体的成员域 */
    hints.ai_socktype = SOCK_STREAM; // 使用TCP协议
    hints.ai_flags = AI_NUMERICSERV; // port参数使用数字形式
    hints.ai_flags |= AI_ADDRCONFIG; // 推荐用于连接

    /* 获取潜在服务器地址列表 */
    if ((rc = getaddrinfo(hostname, port, &hints, &listp)) != 0)
    {
        fprintf(stderr, "getaddrinfo failed (%s:%s): %s\n", hostname, port, gai_strerror(rc));
        return -2; // 函数返回-2表示获取地址列表失败
    }

    /* 遍历潜在服务器地址列表，找到可以成功连接的地址 */
    for (p = listp; p; p = p->ai_next)
    {
        /* 创建一个套接字描述符 */
        if ((clientfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
            continue; // 套接字创建失败，继续下一次循环

        /* 连接服务器 */
        if (connect(clientfd, p->ai_addr, p->ai_addrlen) != -1)
            break; // 连接成功，退出循环

        /* 连接失败，关闭该套接字并尝试下一个地址 */
        if (close(clientfd) < 0)
        {
            fprintf(stderr, "open_clientfd: close failed: %s\n", strerror(errno));
            return -1; // 函数返回-1表示连接失败
        }
    }

    /* 释放内存空间 */
    freeaddrinfo(listp);

    /* 如果所有连接都失败，则函数返回-1；如果最后一个连接成功，则函数返回客户端文件描述符clientfd */
    if (!p)
        return -1;
    else
        return clientfd;
}

int open_listenfd(const char *port)
{
    struct addrinfo hints, *listp, *p;
    int listenfd, rc, optval = 1;

    /* 获取潜在服务器地址列表 */
    memset(&hints, 0, sizeof(struct addrinfo)); // 初始化hints变量所在的内存空间

    /* 设置hints结构体的成员域 */
    hints.ai_socktype = SOCK_STREAM;             // 使用TCP流套接字协议
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG; // 监听任意基于主机可用的IP地址
    hints.ai_flags |= AI_NUMERICSERV;            // 函数中的service参数被解释为数值型端口号

    /* getaddrinfo() 函数是一个用于获取主机名和服务名对应的地址信息的函数，第一个参数传入的是主机名，如果传入 NULL，则表示获取本机的地址信息；第二个参数 port 是一个字符串类型的端口号，表示需要获取的地址信息对应的端口号；第三个参数 hints 是一个指向 addrinfo 结构体的指针，用于设置获取地址信息的选项；第四个参数 listp 是一个指向 addrinfo 结构体指针的指针，用于存储获取到的地址信息列表。因此，该代码的作用是获取本机指定端口号的地址信息，具体选项和获取到的地址信息列表都由 hints 和 listp 两个参数返回。 */
    if ((rc = getaddrinfo(NULL, port, &hints, &listp)) != 0) // 解析主机和服务名以获取地址信息
    {
        fprintf(stderr, "getaddrinfo failed (port %s): %s\n", port, gai_strerror(rc));
        return -2;
    }

    /* 遍历地址列表，尝试绑定 */
    for (p = listp; p; p = p->ai_next)
    {
        /* 创建socket描述符 */
        if ((listenfd = Socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) // 创建套接字
            continue;                                                              // Socket创建失败，尝试下一个

        /* 设置SO_REUSEADDR选项解决“地址已经被使用”的错误 */
        Setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));

        /* 绑定套接字描述符到地址 */
        if (Bind(listenfd, p->ai_addr, p->ai_addrlen) == 0) // 将套接字与本地地址和端口进行绑定
            break;                                          // 成功绑定时退出循环
        if (close(listenfd) < 0)                            // 绑定失败，关闭当前套接字，尝试下一个地址
        {
            fprintf(stderr, "open_listenfd close failed: %s\n", strerror(errno));
            return -1;
        }
    }

    /* 释放内存 */
    freeaddrinfo(listp); // 释放地址信息所占用的内存
    if (!p)              // 没有可用的地址
        return -1;

    /* 将socket变为监听socket以准备接受连接请求 */
    if (Listen(listenfd, LISTENQ) < 0) // 将套接字设置为被动监听状态，等待客户端的连接请求
    {
        close(listenfd);
        return -1;
    }
    return listenfd; // 返回监听套接字描述符
}

/****************************************************
 * Wrappers for reentrant protocol-independent helpers
 ****************************************************/

int Open_clientfd(const char *hostname, const char *port)
{
    int rc;

    if ((rc = open_clientfd(hostname, port)) < 0)
        unix_error("Open_clientfd error");
    return rc;
}

int Open_listenfd(const char *port)
{
    int rc;

    if ((rc = open_listenfd(port)) < 0)
        unix_error("Open_listenfd error");
    return rc;
}