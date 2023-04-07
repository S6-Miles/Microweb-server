#ifndef __CSAPP_H__
#define __CSAPP_H__

#include <stdio.h>      //标准输入输出
#include <stdlib.h>     //常用函数库
#include <stdarg.h>     //可变参数函数
#include <unistd.h>     //POSIX标准的Unix API的头文件
#include <string.h>     // 字符串处理
#include <ctype.h>      // 字符分类和转换
#include <setjmp.h>     // 非局部跳转
#include <signal.h>     // 信号处理
#include <dirent.h>     // 目录操作
#include <sys/time.h>   // 时间相关操作
#include <sys/types.h>  // 系统数据类型
#include <sys/wait.h>   // 进程控制
#include <sys/stat.h>   // 文件状态
#include <fcntl.h>      // 文件控制
#include <sys/mman.h>   // 内存管理
#include <errno.h>      // 错误码
#include <math.h>       // 数学函数
#include <pthread.h>    // 多线程
#include <semaphore.h>  // 信号量
#include <sys/socket.h> // Socket编程
#include <netdb.h>      // 网络相关
#include <netinet/in.h> // IP地址相关
#include <arpa/inet.h>  // 网络相关
#include <sqlite3.h>    //sqlite3数据库头文件
#include <utmp.h>

#define DEF_MODE S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH // 默认权限模式
#define DEF_UMASK S_IWGRP | S_IWOTH                                        // 默认掩码

/* 常量 */
#define RIO_BUFSIZE 8192 // 缓冲区大小
#define MAXLINE 8192     // 文本行最大长度
#define MAXBUF 8192      // I/O缓存区最大长度
#define LISTENQ 1024     // listen函数第二个参数的最大值，设定请求队列的最大长度

typedef struct sockaddr SA; // 自定义结构体名SA代替struct sockaddr
typedef struct              // 自定义结构体rio_t，提供Robust I/O操作
{
    int rio_fd;                // 描述符
    int rio_cnt;               // 当前未读取的字节数
    char *rio_bufptr;          // 下一个待读取的字符位置
    char rio_buf[RIO_BUFSIZE]; // 存放缓冲区数据
} rio_t;

/* 定义任务结构体 */
typedef struct task
{
    void (*func)(void *arg); // 任务函数指针
    void *arg;               // 任务参数
    struct task *next;       // 指向下一个任务的指针
} task_t;

/* 定义线程池结构体 */
typedef struct threadpool
{
    int thread_count;      // 线程数量
    pthread_t *threads;    // 线程数组首地址
    task_t *head;          // 指向任务队列的头指针
    task_t *tail;          // 指向任务队列的尾指针
    pthread_mutex_t lock;  // 互斥锁，用于访问任务队列
    pthread_cond_t notify; // 条件变量，用于通知工作线程有新的任务
    int shutdown;          // 是否关闭线程池
} threadpool_t;

extern int h_errno;    // DNS错误的全局变量
extern char **environ; // 环境变量指针数组

/* 自定义错误处理函数 */
void unix_error(const char *msg);            // Unix系统调用错误
void posix_error(int code, const char *msg); // POSIX函数错误
void dns_error(const char *msg);             // DNS解析错误
void gai_error(int code, const char *msg);   // getaddrinfo()错误
void app_error(const char *msg);             // 应用程序错误

/* 进程控制函数封装 */
pid_t Fork(void);
void Execve(const char *filename, char *const argv[], char *const envp[]);
pid_t Wait(int *status);
pid_t Waitpid(pid_t pid, int *iptr, int options);
void Kill(pid_t pid, int signum);
unsigned int Sleep(unsigned int secs);
void Pause(void);
unsigned int Alarm(unsigned int seconds);
void Setpgid(pid_t pid, pid_t pgid);
pid_t Getpgrp();

/* 信号处理函数封装 */
typedef void handler_t(int);
handler_t *Signal(int signum, handler_t *handler);
void Sigprocmask(int how, const sigset_t *set, sigset_t *oldset);
void Sigemptyset(sigset_t *set);
void Sigfillset(sigset_t *set);
void Sigaddset(sigset_t *set, int signum);
void Sigdelset(sigset_t *set, int signum);
int Sigismember(const sigset_t *set, int signum);
int Sigsuspend(const sigset_t *set);

/* Signal-safe I/O函数的封装 */
ssize_t sio_puts(const char s[]);
ssize_t sio_putl(long v);
void sio_error(const char s[]);

/* Sio wrappers */
ssize_t Sio_puts(const char s[]);
ssize_t Sio_putl(long v);
void Sio_error(const char s[]);

/* Unix I/O函数封装 */
int Open(const char *pathname, int flags, mode_t mode);
ssize_t Read(int fd, void *buf, size_t count);
ssize_t Write(int fd, const void *buf, size_t count);
off_t Lseek(int fildes, off_t offset, int whence);
void Close(int fd);
int Select(int n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
int Dup2(int fd1, int fd2);
void Stat(const char *filename, struct stat *buf);
void Fstat(int fd, struct stat *buf);

/* 目录操作函数封装 */
DIR *Opendir(const char *name);
struct dirent *Readdir(DIR *dirp);
int Closedir(DIR *dirp);

/* 内存映射函数封装 */
void *Mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset);
void Munmap(void *start, size_t length);

/* 标准I/O函数封装 */
void Fclose(FILE *fp);
FILE *Fdopen(int fd, const char *type);
char *Fgets(char *ptr, int n, FILE *stream);
FILE *Fopen(const char *filename, const char *mode);
void Fputs(const char *ptr, FILE *stream);
size_t Fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
void Fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);

/* 动态存储分配函数封装 */
void *Malloc(size_t size);
void *Realloc(void *ptr, size_t size);
void *Calloc(size_t nmemb, size_t size);
void Free(void *ptr);

/* Socket编程函数封装 */
int Socket(int domain, int type, int protocol);
int Setsockopt(int s, int level, int optname, const void *optval, int optlen);
int Bind(int sockfd, struct sockaddr *my_addr, int addrlen);
int Listen(int s, int backlog);
int Accept(int s, struct sockaddr *addr, socklen_t *addrlen);
int Connect(int sockfd, struct sockaddr *serv_addr, int addrlen);

/* Protocol independent wrappers */
void Getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res);
void Getnameinfo(const struct sockaddr *sa, socklen_t salen, char *host, size_t hostlen, char *serv, size_t servlen, int flags);
void Freeaddrinfo(struct addrinfo *res);
void Inet_ntop(int af, const void *src, char *dst, socklen_t size);
void Inet_pton(int af, const char *src, void *dst);

/* DNS wrappers */
struct hostent *Gethostbyname(const char *name);
struct hostent *Gethostbyaddr(const char *addr, int len, int type);

/* Pthreads thread control wrappers */
void Pthread_create(pthread_t *tidp, const pthread_attr_t *attrp, void *(*routine)(void *), void *argp);
void Pthread_join(pthread_t tid, void **thread_return);
void Pthread_cancel(pthread_t tid);
void Pthread_detach(pthread_t tid);
void Pthread_exit(void *retval);
pthread_t Pthread_self(void);
void Pthread_once(pthread_once_t *once_control, void (*init_function)());

/* POSIX semaphore wrappers */
void Sem_init(sem_t *sem, int pshared, unsigned int value);
void P(sem_t *sem);
void V(sem_t *sem);

/* Rio (Robust I/O) package */
ssize_t rio_readn(int fd, void *usrbuf, size_t n);
ssize_t rio_writen(int fd, const void *usrbuf, size_t n);
void rio_readinitb(rio_t *rp, int fd);
ssize_t rio_readnb(rio_t *rp, void *usrbuf, size_t n);
ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen);

/* Wrappers for Rio package */
ssize_t Rio_readn(int fd, void *usrbuf, size_t n);
void Rio_writen(int fd, const void *usrbuf, size_t n);
void Rio_readinitb(rio_t *rp, int fd);
ssize_t Rio_readnb(rio_t *rp, void *usrbuf, size_t n);
ssize_t Rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen);

/* Reentrant protocol-independent client/server helpers */
int open_clientfd(const char *hostname, const char *port);
int open_listenfd(const char *port);

/* Wrappers for reentrant protocol-independent client/server helpers */
int Open_clientfd(const char *hostname, const char *port);
int Open_listenfd(const char *port);

#endif /* __CSAPP_H__ */