//*******************************************************************************
// Copyright (C), RHTECH. Co., Ltd.
// Author:          redfox.qu@qq.com
// Date:            2014/02/12
// Version:         1.0.0
// Description:     Inner Program Connection command line interface
//
// History:
//      <author>            <time>      <version>   <desc>
//      redfox.qu@qq.com    2014/02/12  1.0.0       create this file
//      redfox.qu@qq.com    2014/02/27  1.0.1       improve sockcli self argument,
//                                                  use argument address/port importent
//      redfox.qu@qq.com    2014/03/06  1.0.2       free sockcli list when timeout
//
// TODO:
//      2014/02/12          structure cli frame and process for the sock-ipc
//*******************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/socket.h>
#include <errno.h>
#include <signal.h>
#include <glob.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include "sockcli.h"

#define SKCLI_DEBUG
//#undef  SKCLI_DEBUG

#ifdef SKCLI_DEBUG
#define SKCLI_MSG(fmt, ...)     do { if (sockcli_opt_arg & SKCLI_ARG_VERBOSE) fprintf(stdout, fmt, ## __VA_ARGS__); } while(0)
#else
#define SKCLI_MSG(fmt, ...)
#endif

#define SKCLI_ERR_MSG           "###sockcli### "

#define SKCLI_USAGE   " \
Usage: sockcli [-I <address>] [-P <port>] [-T <timeout>] [-V] [-L] <appname> [options] \n \
    -I <address>                set server ip address \n \
    -P <port>                   set server udp port \n \
    -T <timeout>                set receive message timeout \n \
    -L                          list all plugin files \n \
    -V                          show the verbose message \n \
    <appname>                   connection to app \n \
    [options]                   options for the app\n \
\n "

#define SKCLI_ARG_ADDRESS       0x00000001
#define SKCLI_ARG_PORT          0x00000002
#define SKCLI_ARG_VERBOSE       0x00000004
#define SKCLI_ARG_LIST          0x00000008


char *sockcli_srv_addr_def      = SKCLI_DEF_SRVADDR;
short sockcli_srv_port_def      = SKCLI_DEF_UDPPORT;
int sockcli_rcv_timeout         = SKCLI_RCV_TOUT;
int sockcli_opt_arg             = 0;
int sockcli_argc_def            = 0;
char **sockcli_argv_def         = NULL;

struct sockcli_root sockcli_sipc_root;


void sockcli_data_dump(char *buf, int size)
{
    int i;

    printf("\n");
    for (i = 0; i < size; i++) {
        printf("%02x(%c) ", buf[i], (isprint(buf[i]) ? buf[i] : '.'));
        if ((i+1) % 16 == 0) {
            printf("\n");
        }
    }
    printf("\n");
}

int sockcli_def_app_name(char *name, int size)
{
    strncpy(name, SKCLI_DEF_NAME, sizeof(SKCLI_DEF_NAME));
    return 0;
}

int sockcli_def_srv_addr(struct in_addr *addr, unsigned short *port)
{
    if (inet_aton(sockcli_srv_addr_def, addr) == 0) {
        return -1;
    }

    *port = htons(sockcli_srv_port_def);

    return 0;
}

int sockcli_def_argt_func(int argc, char **argv)
{
    SKCLI_MSG("argc=%d\n", argc);
    sockcli_argc_def = argc;
    sockcli_argv_def = argv;
    return 1;
}

int sockcli_def_send_func(char *buf, int *size)
{
    int i;

    *size = 0;
    for (i = 0; i < sockcli_argc_def; i++) {
        *size += sprintf(buf + *size, "%s ", sockcli_argv_def[i]);
    }

    return 0;
}

int sockcli_def_recv_func(char *buf, int *size)
{
    printf("%s\n", buf);
    return 0;
}

struct sockcli *sockcli_loaddef(void)
{
    struct sockcli *sipc = NULL;

    sipc = malloc(sizeof(struct sockcli));
    if (sipc == NULL) {
        fprintf(stderr, SKCLI_ERR_MSG "malloc() failed");
        return NULL;
    }

    bzero(sipc, sizeof(struct sockcli));

    INIT_LIST_HEAD(&sipc->list);
    sipc->app_path = strdup(SKCLI_DEF_PATH);
    sipc->app_name = strdup(SKCLI_DEF_NAME);
    sipc->handle = NULL;
    sipc->name_func = sockcli_def_app_name;
    sipc->addr_func = sockcli_def_srv_addr;
    sipc->argt_func = sockcli_def_argt_func;
    sipc->send_func = sockcli_def_send_func;
    sipc->recv_func = sockcli_def_recv_func;

    list_add(&sipc->list, &sockcli_sipc_root.list);
    sockcli_sipc_root.num++;

    return sipc;
}

/*****************************************************************************
* Function Name  : alrm_handler
* Description    : time out alarm and exit function
* Input          : int
* Output         : None
* Return         : None
*                  - 0:ok
*****************************************************************************/
void sockcli_alrm_handler(int s)
{
    fprintf(stderr, SKCLI_ERR_MSG "recv message time out(%d)!\n", sockcli_rcv_timeout);

    /* free the sockcli list */
    sockcli_free(&sockcli_sipc_root);
    SKCLI_MSG("sockcli_free ok\n");

    exit(10);
}


int sockcli_init(struct sockcli_root *sipcr)
{
    INIT_LIST_HEAD(&sipcr->list);
    sipcr->num = 0;

    return 0;
}

int sockcli_prob(struct sockcli_root *sipcr)
{
    glob_t res;
    int ret, i;
    char *error;
    char name[SKCLI_NAME_LEN];
    struct sockcli *sipc = NULL;

    ret = glob(SKCLI_PLUGIN_PATH, 0, NULL, &res);
    if (ret == GLOB_NOMATCH) {
        return 0;
    } else if (ret != 0) {
        fprintf(stderr, SKCLI_ERR_MSG "glob(): %s\n", strerror(errno));
        return 0;
    }

    SKCLI_MSG("found %d plugins\n", res.gl_pathc);

    for (i = 0; i < res.gl_pathc; i++) {

        SKCLI_MSG("plugin files: %s\n", res.gl_pathv[i]);

        sipc = malloc(sizeof(struct sockcli));
        if (sipc == NULL) {
            fprintf(stderr, SKCLI_ERR_MSG "new sockcli malloc() failed");
            return -1;
        }

        bzero(sipc, sizeof(struct sockcli));

        INIT_LIST_HEAD(&sipc->list);
        sipc->app_path = strdup(res.gl_pathv[i]);

        sipc->handle = NULL;
        sipc->app_name = NULL;
        sipc->argt_func = NULL;
        sipc->send_func = NULL;
        sipc->recv_func = NULL;

        /* open the sockcli plugin */
        sipc->handle = dlopen(sipc->app_path, RTLD_LAZY);
        if (!sipc->handle) {
            fprintf(stderr, SKCLI_ERR_MSG "dlopen() failed: %s\n", dlerror());
            return -1;
        }
        dlerror();  /* clear any existing error */

        /* get the sockcli plugin name */
        sipc->name_func = dlsym(sipc->handle, "app_name");
        if ((error = dlerror()) != NULL) {
            fprintf(stderr, SKCLI_ERR_MSG "dlsym(name) error: %s\n", error);
            return -1;
        }
        dlerror();  /* clear any existing error */

        sipc->name_func(name, SKCLI_NAME_LEN-1);
        sipc->app_name = strdup(name);
        SKCLI_MSG("plug name: %s\n", sipc->app_name);

        list_add(&sipc->list, &sipcr->list);
        sipcr->num++;
    }

    return sipcr->num;
}

int sockcli_free(struct sockcli_root *sipcr)
{
    struct sockcli *sipc0, *sipc1;

    list_for_each_entry_safe(sipc0, sipc1, &sipcr->list, list) {
        list_del(&sipc0->list);
        free(sipc0->app_name);
        free(sipc0->app_path);
        if (sipc0->handle) {
            dlclose(sipc0->handle);
        }
        free(sipc0);
    }

    return 0;
}

struct sockcli *sockcli_find(struct sockcli_root *sipcr, char *appname)
{
    struct sockcli *sipc;

    list_for_each_entry(sipc, &sipcr->list, list) {
        if (strcmp(sipc->app_name, appname) == 0) {
            return sipc;
        }
    }

    return NULL;
}

int sockcli_open(struct sockcli *sipc)
{
    int val = 1;
    int ret = 0;

    sipc->fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sipc->fd < 0) {
        fprintf(stderr, SKCLI_ERR_MSG "socket() error: %s", strerror(errno));
        return -1;
    }

    ret = setsockopt(sipc->fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    if (ret < 0) {
        fprintf(stderr, SKCLI_ERR_MSG "setsockopt() error: %s", strerror(errno));
        return -1;
    }

    return sipc->fd;
}

int sockcli_close(struct sockcli *sipc)
{
    if (sipc->fd > 0) {
        close(sipc->fd);
        sipc->fd = -1;
    }

    return 0;
}

int sockcli_load(struct sockcli *sipc)
{
    char *error;

    SKCLI_MSG("load plugin files: %s\n", sipc->app_path);

    if (sipc->handle == NULL) {
        fprintf(stderr, SKCLI_ERR_MSG "handle not open\n");
        return -1;
    }

    /* get the argument analizying function */
    sipc->argt_func = dlsym(sipc->handle, "argt_func");
    //MESSAGE("argt_func in memory %p\n", sipc->argt_func);
    if ((error = dlerror()) != NULL) {
        sipc->argt_func = sockcli_def_argt_func;
        SKCLI_MSG("dlsym(argt_func) error: %s, use def_argt_func\n", error);
    };
    dlerror();  /* clear any existing error */

    /* get the argument analizying function */
    sipc->send_func = dlsym(sipc->handle, "send_func");
    //MESSAGE("send_func in memory %p\n", sipc->send_func);
    if ((error = dlerror()) != NULL) {
        sipc->send_func = sockcli_def_send_func;
        SKCLI_MSG("dlsym(send_func) error: %s, use def_send_func\n", error);
    };
    dlerror();  /* clear any existing error */

    /* get the argument analizying function */
    sipc->recv_func = dlsym(sipc->handle, "recv_func");
    //MESSAGE("recv_func in memory %p\n", sipc->recv_func);
    if ((error = dlerror()) != NULL) {
        sipc->recv_func = sockcli_def_recv_func;
        SKCLI_MSG("dlsym(recv_func) error: %s, use def_recv_func\n", error);
    };
    dlerror();  /* clear any existing error */

    sipc->addr_func = dlsym(sipc->handle, "srv_addr");
    if ((error = dlerror()) != NULL) {
        sipc->addr_func = sockcli_def_srv_addr;
        SKCLI_MSG("dlsym(addr_func) error: %s, use def_srv_addr\n", error);
    }
    dlerror();  /* clear any existing error */


    sipc->fd = -1;
    inet_aton(SKCLI_DEF_SRVADDR, &sipc->server_ip);
    sipc->server_port = SKCLI_DEF_UDPPORT;
    bzero(&sipc->r_buf[0], SKCLI_BUF_SIZE);
    bzero(&sipc->w_buf[0], SKCLI_BUF_SIZE);
    sipc->r_len = 0;
    sipc->w_len = 0;

    return 0;
}

int sockcli_unload(struct sockcli *sipc)
{
    sockcli_close(sipc);
    bzero(&sipc->r_buf[0], SKCLI_BUF_SIZE);
    bzero(&sipc->w_buf[0], SKCLI_BUF_SIZE);
    sipc->r_len = sipc->w_len = 0;

    return 0;
}


int sockcli_list(struct sockcli_root *sipcr)
{
    struct sockcli *sipc;

    list_for_each_entry(sipc, &sipcr->list, list) {
        printf("sockcli app: %s, path: %s\n", sipc->app_name, sipc->app_path);
    }

    return 0;
}


/*****************************************************************************
* Function Name  : main
* Description    : rfcli main function
* Input          : int, char**
* Output         : None
* Return         : int
*                  - 0: ok
*                  - -1: error
*****************************************************************************/
int main(int argc, char **argv)
{
    struct sockaddr_in srvaddr;
    socklen_t sock_len;
    int ret, I = 1;
    struct sockcli *sipc;

    if (argc < 2) {
        printf(SKCLI_USAGE);
        exit(EXIT_FAILURE);
    }

    for (I = 1; I < argc; I++) {
        if (argv[I][0] == '-') {
            switch (argv[I][1]) {
                case 'I':
                    sockcli_opt_arg |= SKCLI_ARG_ADDRESS;
                    sockcli_srv_addr_def = argv[I+1];
                    I++;
                    break;

                case 'P':
                    sockcli_opt_arg |= SKCLI_ARG_PORT;
                    sockcli_srv_port_def = (short)atoi(argv[I+1]);
                    I++;
                    break;

                case 'T':
                    sockcli_rcv_timeout = atoi(argv[I+1]);
                    I++;
                    break;

                case 'V':
                    sockcli_opt_arg |= SKCLI_ARG_VERBOSE;
                    break;

                case 'L':
                    sockcli_opt_arg |= SKCLI_ARG_LIST;
                    break;

                default:
                    fprintf(stderr, SKCLI_ERR_MSG "Unkown argument %s\n", argv[I]);
                    fprintf(stderr, SKCLI_USAGE);
                    exit(EXIT_FAILURE);
            }
        } else {
            break;
        }
    }

    if (signal(SIGALRM, sockcli_alrm_handler) == SIG_ERR) {
        fprintf(stderr, SKCLI_ERR_MSG "signal(SIGALRM) error\n");
        exit(EXIT_FAILURE);
    }

    if (sockcli_init(&sockcli_sipc_root) < 0) {
        exit(EXIT_FAILURE);
    }
    SKCLI_MSG("sockcli_init ok\n");

    if (sockcli_prob(&sockcli_sipc_root) < 0) {
        exit(EXIT_FAILURE);
    }
    SKCLI_MSG("sockcli_prob ok\n");

    if (sockcli_opt_arg & SKCLI_ARG_LIST) {
        sockcli_list(&sockcli_sipc_root);
        goto err1;
    }

    sipc = sockcli_find(&sockcli_sipc_root, argv[I]);
    if (sipc != NULL) {
        SKCLI_MSG("sockcli_find %s ok\n", argv[I]);
        ret = sockcli_load(sipc);
        if (ret < 0) {
            goto err;
        }
        SKCLI_MSG("sockcli_load %s ok\n", argv[I]);
    } else {
        fprintf(stderr, SKCLI_ERR_MSG "%s not found, use default function\n", argv[I]);
        sipc = sockcli_loaddef();
        if (sipc == NULL) {
            SKCLI_MSG("sockcli_loaddef error!\n");
            goto err;
        }
        SKCLI_MSG("sockcli_loaddef ok\n");
    }


    ret = sipc->argt_func(argc - I, argv + I);
    if (ret <= 0) {
        goto err;
    }

    SKCLI_MSG("sipc->argt_func ok\n");

    // create msg quere
    ret = sockcli_open(sipc);
    if (ret < 0) {
        fprintf(stderr, SKCLI_ERR_MSG "sockcli_open error\n");
        goto err;
    }
    SKCLI_MSG("sockcli_open ok\n");


    ret = sipc->addr_func(&sipc->server_ip, &sipc->server_port);
    if (ret < 0) {
        fprintf(stderr, SKCLI_ERR_MSG "sipc->addr_func error\n");
        goto err;
    }
    if (sockcli_opt_arg & SKCLI_ARG_ADDRESS) {
        inet_aton(sockcli_srv_addr_def, &sipc->server_ip);
    }
    if (sockcli_opt_arg & SKCLI_ARG_PORT) {
        sipc->server_port = htons(sockcli_srv_port_def);
    }
    SKCLI_MSG("server_ip=%s, server_port=%d\n", inet_ntoa(sipc->server_ip), ntohs(sipc->server_port));

    ret = sipc->send_func(&sipc->w_buf[0], &sipc->w_len);
    if (ret < 0) {
        fprintf(stderr, SKCLI_ERR_MSG "call %s sipc->send_func error\n", argv[I]);
        goto err;
    }
    SKCLI_MSG("sipc->send_func ok\n");

    sock_len = sizeof(srvaddr);
    bzero(&srvaddr, sock_len);

    srvaddr.sin_addr = sipc->server_ip;
    srvaddr.sin_port = sipc->server_port;
    srvaddr.sin_family = AF_INET;

    ret = sendto(sipc->fd, sipc->w_buf, sipc->w_len, 0,
            (struct sockaddr*)&srvaddr, sock_len);
    if (ret < 0) {
        fprintf(stderr, SKCLI_ERR_MSG "sendto() error: (%d)%s\n", ret, strerror(errno));
        goto err;
    }
    SKCLI_MSG("sendto %s:%d len=%d\n", inet_ntoa(sipc->server_ip),
            ntohs(sipc->server_port), ret);
#ifdef SKCLI_DEBUG
    if (sockcli_opt_arg & SKCLI_ARG_VERBOSE) {
        sockcli_data_dump(&sipc->w_buf[0], sipc->w_len);
    }
#endif

    /* set to time out process */
    ret = alarm(sockcli_rcv_timeout);
    SKCLI_MSG("set timeout %d for receive message\n", sockcli_rcv_timeout);

    sock_len = sizeof(srvaddr);
    ret = recvfrom(sipc->fd, sipc->r_buf, SKCLI_BUF_SIZE, 0,
            (struct sockaddr*)&srvaddr, &sock_len);

    if (ret <= 0) {
        fprintf(stderr, SKCLI_ERR_MSG "recvfrom() error: (%d)%s\n", ret, strerror(errno));
        goto err;
    }
    SKCLI_MSG("recvfrom %s:%d len=%d\n", inet_ntoa(srvaddr.sin_addr),
            ntohs(srvaddr.sin_port), ret);

    sipc->r_len = ret;

#ifdef SKCLI_DEBUG
    if (sockcli_opt_arg & SKCLI_ARG_VERBOSE) {
        sockcli_data_dump(sipc->r_buf, sipc->r_len);
    }
#endif

    ret = sipc->recv_func(sipc->r_buf, &sipc->r_len);
    if (ret < 0) {
        fprintf(stderr, SKCLI_ERR_MSG "call %s sipc->recv_func error\n", argv[I]);
        goto err;
    }
    SKCLI_MSG("sipc->recv_func ok\n");

err:
    alarm(0);
    SKCLI_MSG("clear timeout\n");
    sockcli_unload(sipc);
    SKCLI_MSG("sockcli_unload ok\n");
err1:
    sockcli_free(&sockcli_sipc_root);
    SKCLI_MSG("sockcli_free ok\n");
    exit(ret);
}



