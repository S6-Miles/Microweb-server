#include "csapp.h"

/* Unix-style error */
void unix_error(char const *msg)
{
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(0);
}

/* Posix-style error */
void posix_error(int code, const char *msg)
{
    fprintf(stderr, "%s: %s\n", msg, strerror(code));
    exit(0);
}

/* Getaddrinfo-style error */
void gai_error(int code, const char *msg)
{
    fprintf(stderr, "%s: %s\n", msg, gai_strerror(code));
    exit(0);
}

/* Application error */
void app_error(const char *msg)
{
    fprintf(stderr, "%s\n", msg);
    exit(0);
}

/* Obsolete gethostbyname error */
void dns_error(const char *msg)
{
    fprintf(stderr, "%s\n", msg);
    exit(0);
}