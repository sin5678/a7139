//*******************************************************************************
// Copyright (C), RHTECH. Co., Ltd.
// Author:          redfox.qu@qq.com
// Date:            2014/2/11
// Version:         1.0.0
// Description:     Use udp socket for InnerProgramConnection
//
// History:
//      <author>            <time>      <version>   <desc>
//      redfox.qu@qq.com    2014/02/11  1.0.0       create this file
//
// TODO:
//      2014/02/11          add the common and debug
//*******************************************************************************

#ifndef __SOCK_CLI_H__
#define __SOCK_CLI_H__

#include <stdlib.h>
#include <getopt.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "list.h"

#define SKCLI_PLUGIN_PATH        "/usr/lib/*.skcli"
#define SKCLI_NAME_LEN           256
#define SKCLI_BUF_SIZE           4096

#define SKCLI_RCV_TOUT          3
#define SKCLI_DEF_PATH          "./"
#define SKCLI_DEF_NAME          "def_name"
#define SKCLI_DEF_SRVADDR       "127.0.0.1"
#define SKCLI_DEF_UDPPORT       60000

typedef int sockcli_name(char *name, int size);
typedef int sockcli_addr(struct in_addr *addr, unsigned short *port);
typedef int sockcli_argf(int argc, char **argv);
typedef int sockcli_func(char *buf, int *size);

struct sockcli_root {
    struct list_head list;
    int num;
};

struct sockcli {
    struct list_head list;

    char *app_name;
    char *app_path;
    void *handle;
    sockcli_name *name_func;
    sockcli_addr *addr_func;
    sockcli_argf *argt_func;
    sockcli_func *send_func;
    sockcli_func *recv_func;

    struct in_addr server_ip;
    unsigned short server_port;
    int fd;
    char r_buf[SKCLI_BUF_SIZE];
    char w_buf[SKCLI_BUF_SIZE];
    int r_len, w_len;
};

#endif  // __SOCK_CLI_H__

