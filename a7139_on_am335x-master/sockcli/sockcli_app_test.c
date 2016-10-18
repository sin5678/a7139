//*******************************************************************************
// Copyright (C), RHTECH. Co., Ltd.
// Author:          redfox.qu@qq.com
// Date:            2014/2/11
// Version:         1.0.0
// Description:     Use udp socket for InnerProgramConnection app plugin demo
//
// History:
//      <author>            <time>      <version>   <desc>
//      redfox.qu@qq.com    2014/02/25  1.0.0       create this file
//
// TODO:
//      2014/02/11          add the common and debug
//*******************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sockcli.h"

#define APP_NAME    "app_test"
#define SRV_ADDR    "127.0.0.1"
#define SRV_PORT    60001

#define USAGE   " \
Usage: app_test [options] \n \
    -h, --help                  Show this USAGE \n \
    -v, --version               Show version \n \
    -L, --loglevel              choose loglevel \n \
                                1:LOG_EMERG,    2:LOG_ALERT \n \
                                3:LOG_CRIT,     4:LOG_ERR \n \
                                5:LOG_WARING,   6:LOG_NOTICE \n \
                                7:LOG_INFO,     8:LOG_DEBUG \n \
\n "

struct app_msg {
    int loglevel;
};

int loglevel;

int app_name(char *name, int size)
{
    strncpy(name, APP_NAME, sizeof(APP_NAME));
    return 0;
}

int srv_addr(struct in_addr *addr, unsigned short *port)
{
    if (inet_aton(SRV_ADDR, addr) == 0) {
        return -1;
    }

    *port = htons(SRV_PORT);

    return 0;
}

int argt_func(int argc, char **argv)
{
    int opt, ret = 1;

    struct option longopts[] = {
        { "help",           0, NULL, 'h' }, //1
        { "version",        0, NULL, 'v' }, //2
        { "loglevel",       1, NULL, 'L' }, //3
        { 0, 0, 0, 0 },
    };

    while ((opt = getopt_long(argc, argv, "hvL:", longopts, NULL)) != -1) {
        switch (opt) {
            case 'L':                   // loglevel
                loglevel = atoi(optarg);
                break;

            case 'h':
            case 'v':
                printf(USAGE);
                ret = 0;
                goto err;
                break;

            default:
                fprintf(stderr, "%s: invalid opt (%c)\n\n", argv[0], opt);
                printf(USAGE);
                ret = 0;
                goto err;
                break;
        }
    }

err:
    return ret;
}

int send_func(char *buf, int *size)
{
    struct app_msg *appmsg = (struct app_msg *)buf;

    appmsg->loglevel = loglevel;

    *size = sizeof(struct app_msg);

    return 0;
}

int show_resp(struct app_msg *appmsg)
{
    printf("loglevel %d\n", appmsg->loglevel);

    return 0;
}

int recv_func(char *buf, int *size)
{
    struct app_msg *appmsg = (struct app_msg *)buf;

    show_resp(appmsg);

    return 0;
}





