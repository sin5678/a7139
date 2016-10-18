#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>

#define SRV_TEST_UDP_PORT           60002

#define SOCK_CLI_TEST_DEBUG
#ifdef  SOCK_CLI_TEST_DEBUG
#define DEBUG(fmt, ...)           do { fprintf(stdout, fmt, ## __VA_ARGS__); } while(0)
#else
#define DEBUG(fmt, ...)
#endif


int scli_daemon_open(short port)
{
    int ctl_fd;
    int ret, val = 1;
    struct sockaddr_in srv_addr;

    ctl_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (ctl_fd == -1) {
        fprintf(stderr, "socket() error: %s", strerror(errno));
        return -1;
    }

    ret = setsockopt(ctl_fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    if (ret == -1) {
        fprintf(stderr, "setsockopt() error: %s", strerror(errno));
        return -1;
    }

    bzero(&srv_addr, sizeof(srv_addr));
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    srv_addr.sin_port = htons(port);

    ret = bind(ctl_fd, (struct sockaddr*)&srv_addr, sizeof(srv_addr));
    if (ret == -1) {
        fprintf(stderr, "bind() error: %s", strerror(errno));
        return -1;
    }

    return ctl_fd;
}

int scli_daemon_read(int fd, char *buf, int size, struct sockaddr_in *cli_addr)
{
    int ret;
    socklen_t len;

    bzero(buf, size);

    len = sizeof(struct sockaddr_in);
    ret = recvfrom(fd, buf, size, 0, (struct sockaddr*)cli_addr, &len);

    if (ret <= 0) {
        fprintf(stderr, "read(ctl_fd) error: %s", strerror(errno));
    }

    return ret;
}

int scli_daemon_write(int fd, char *buf, int size, struct sockaddr_in *cli_addr)
{
    int ret;
    socklen_t len;

    len = sizeof(struct sockaddr_in);
    ret = sendto(fd, buf, size, 0, (struct sockaddr*)cli_addr, len);

    if (ret <= 0) {
        fprintf(stderr, "write(ctl_fd) error: %s\n", strerror(errno));
    }

    return ret;
}

int scli_daemon_proc(char *rbuf, int rlen, char *wbuf, int *wlen)
{
    int i;

    for (i = 0; i < rlen; i++) {
        wbuf[i] = rbuf[i] + 1;
    }
    *wlen = rlen;

    return *wlen;
}

int main(int argc, char *argv[])
{
    int ctl_fd;
    char rbuf[1024], wbuf[1024];
    int rlen, wlen;
    struct sockaddr_in cli_addr;


    if ((ctl_fd = scli_daemon_open(SRV_TEST_UDP_PORT)) < 0) {
        perror("scli_daemon_open");
        exit(-1);
    }

    DEBUG("scli_daemon_open ok, port=%d\n", SRV_TEST_UDP_PORT);

    while (1)
    {
        if ((rlen = scli_daemon_read(ctl_fd, rbuf, 1024, &cli_addr)) <= 0) {
            fprintf(stderr, "scli_daemon_read error\n");
            exit(-1);
        }

        DEBUG("scli_daemon_read '%s'\n", rbuf);
        DEBUG("scli_daemon_read from %s:%d\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

        if (scli_daemon_proc(rbuf, rlen, wbuf, &wlen) < 0) {
            fprintf(stderr, "scli_daemon_proc error\n");
            exit(-1);
        }

        DEBUG("scli_daemon_write '%s'\n", wbuf);

        if ((wlen = scli_daemon_write(ctl_fd, wbuf, wlen, &cli_addr)) <= 0) {
            fprintf(stderr, "scli_daemon_write error\n");
            exit(-1);
        }

    }

    exit(0);
}



