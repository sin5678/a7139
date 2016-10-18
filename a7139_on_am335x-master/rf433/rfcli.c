//*******************************************************************************
// Copyright (C), RHTECH. Co., Ltd.
// Author:          redfox.qu@qq.com
// Date:            2013/12/26
// Version:         1.0.0
// Description:     The 433Mhz module command line interface source file
//
// History:
//      <author>            <time>      <version>   <desc>
//      redfox.qu@qq.com    2013/12/26  1.0.0       create this file
//      redfox.qu@qq.com    2014/2/11   1.1.0       use udp-socket replace msg for IPC
//
// TODO:
//      2013/12/31          add the common and debug
//*******************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <getopt.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/socket.h>
#include <errno.h>
#include <signal.h>
#include <sys/ioctl.h>
#include "common.h"
#include "rfcli.h"
#include "rf433lib.h"

/*****************************************************************************
* Function Name  : alrm_handler
* Description    : time out alarm and exit function
* Input          : int
* Output         : None
* Return         : None
*                  - 0:ok
*****************************************************************************/
void alrm_handler(int s)
{
    printf("###Error recv msg time out(%d)!\n", RECV_MSG_TIME_OUT);
    exit(10);
}

/*****************************************************************************
* Function Name  : show_config
* Description    : show rfrepeater configure
* Input          : msg_config*
* Output         : display
* Return         : None
*****************************************************************************/
void show_config(struct msg_st *rmsg)
{
    struct msg_config *cfg;
    uint8_t ret;

    ret = rmsg->d.content[0];
    if (ret != MSG_RET_OK) {
        printf("####Error %d\n", ret);
        return;
    }

    cfg = (struct msg_config *)&rmsg->d.content[1];

    printf("%s=%d\n", RF433_NVR_NET_ID, RF433_NET_ID(cfg->rf433.net_id));
    printf("%s=0x%08x\n", RF433_NVR_LOCAL_ADDR, cfg->rf433.local_addr);
    printf("%s=%s\n", RF433_NVR_SRV_IP, inet_ntoa(cfg->socket.server_ip));
    printf("%s=%d\n", RF433_NVR_UDP_PORT, cfg->socket.server_port);
}

/*****************************************************************************
* Function Name  : show_se433_list
* Description    : show the se433 list and data
* Input          : uint32_t*
* Output         : display
* Return         : None
*****************************************************************************/
void show_se433_list(struct msg_st *rmsg)
{
    se433_data *se433data;
    uint8_t ret, num, i;

    ret = rmsg->d.content[0];
    if (ret != MSG_RET_OK) {
        printf("####Error %d\n", ret);
        return;
    }

    num = rmsg->d.content[1];
    se433data = (se433_data *)&rmsg->d.content[2];

    for (i = 0; i < num; i++) {
        printf("addr=0x%08x, req_cnt=%d, rsp_cnt=%d, type=0x%02x, " \
                "data=%.3f, vol=0x%02x, batt=0x%02x, flag=0x%02x, whid=0x%02x\n",
                se433data->addr,
                se433data->req_cnt,
                se433data->rsp_cnt,
                se433data->data.type,
                se433data->data.data,
                se433data->data.vol,
                se433data->data.batt,
                se433data->data.flag,
                se433data->data.watchid);
        se433data++;
    }
}

/*****************************************************************************
* Function Name  : show_resp
* Description    : show the other message response
* Input          : char*
* Output         : display
* Return         : None
*****************************************************************************/
int show_resp(struct msg_st *rmsg)
{
    /* all success response will no message,
     * and all error response will say some word like 'XXX error'.
     * if the verbose message not in the response
     * please see the syslog
     */
    uint8_t ret;
    char *resp;

    ret = rmsg->d.content[0];
    resp = (char*)&rmsg->d.content[1];

    if (ret != MSG_RET_OK) {
        printf("####Error %s\n", resp);
    }

    return 0;
}

/*****************************************************************************
* Function Name  : send_msg
* Description    : send the message to rfrepeater deamon
* Input          : char*
* Output         : display
* Return         : None
*****************************************************************************/
int send_msg(int fd, buffer *msg_wbuf, int type, void *p, int len)
{
    struct msg_st *wmsg = (struct msg_st *)buf_data(msg_wbuf);
    struct sockaddr_in cliaddr;
    socklen_t sock_len;
    int ret;

    buf_clean(msg_wbuf);

    wmsg->h.magic[0] = MSG_CTL_MAGIC_0;
    wmsg->h.magic[1] = MSG_CTL_MAGIC_1;
    wmsg->h.flags = 0;
    wmsg->h.version = MSG_CTL_VERSION;

    wmsg->d.type = type;

    buf_incrlen(msg_wbuf, sizeof(struct msg_head) + 4);

    buf_append(msg_wbuf, (char*)p, len);

    /*
    printf("send message:\n");
    buf_dump(msg_wbuf);
    */

    sock_len = sizeof(cliaddr);
    bzero(&cliaddr, sock_len);

    cliaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    cliaddr.sin_port = htons(SRV_CTL_UDP_PORT);
    cliaddr.sin_family = AF_INET;

    ret = sendto(fd, buf_data(msg_wbuf), buf_len(msg_wbuf), 0,
            (struct sockaddr*)&cliaddr, sock_len);
    if (ret == -1) {
        printf("send_msg(): sendto() err: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

/*****************************************************************************
* Function Name  : recv_msg
* Description    : recveive the message from rfrepeater deamon
* Input          : void
* Output         : display
* Return         : int
*                  - 0: ok
*****************************************************************************/
int recv_msg(int ctl_fd, buffer *msg_rbuf)
{
    int ret = 0;
    struct sockaddr_in ctl_cli;
    socklen_t sock_len;
    struct msg_st *rmsg = (struct msg_st *)buf_data(msg_rbuf);

    if (signal(SIGALRM, alrm_handler) == SIG_ERR) {
        printf("signal(SIGALRM) error\n");
        goto out;
    }

    alarm(RECV_MSG_TIME_OUT);

    sock_len = sizeof(ctl_cli);
    ret = recvfrom(ctl_fd, buf_data(msg_rbuf), buf_space(msg_rbuf), 0,
            (struct sockaddr*)&ctl_cli, &sock_len);

    if (ret <= 0) {
        /* maybe error */
        printf("recvfrom(ctl_fd) error: (%d)%s", ret, strerror(errno));

        /* exit the thread */
        goto out;
    }

    buf_incrlen(msg_rbuf, ret);

    /*
    printf("recive message:\n");
    buf_dump(msg_rbuf);
    */

    if (msg_check(msg_rbuf) < 0) {
        printf("got a invalid ctrl message\n");
        goto out;
    }

    switch (rmsg->d.type) {
        case MSG_RSP_GET_CFG:
            show_config(rmsg);
            break;

        case MSG_RSP_GET_SE433L:
            show_se433_list(rmsg);
            break;

        case MSG_RSP_END:
            show_resp(rmsg);
            break;

        default:
            printf("Unknown MSG command %ld", rmsg->d.type);
            break;
    }

out:
    alarm(0);
    return ret;
}


#define USAGE   " \
Usage: rfcli [options] \n \
    -h, --help                  Show this USAGE \n \
    -v, --version               Show version \n \
    -s, --save                  Save config to file \n \
    -r, --read                  Read config from file \n \
    -c, --show                  Show the current config \n \
    -l, --selist                Show se433 sensor list \n \
    -L, --loglevel              choose loglevel \n \
                                1:LOG_EMERG,    2:LOG_ALERT \n \
                                3:LOG_CRIT,     4:LOG_ERR \n \
                                5:LOG_WARING,   6:LOG_NOTICE \n \
                                7:LOG_INFO,     8:LOG_DEBUG \n \
config options: \n \
    -N, --id=netid              433 network id \n \
    -A, --addr=433addr          433 receive address \n \
    -I, --rip=ipaddr            set remote server ip address \n \
    -P, --rport=port            set remote server udp port \n \
\n "

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
    int opt, log_level, opt_id = 0;
    struct msg_config cfg;
    int ret = 0;
    int ctl_fd = -1;
    buffer *ctl_rbuf = NULL, *ctl_wbuf = NULL;
    se433_cfg se433_set;

    struct option longopts[] = {
        { "help",           0, NULL, 'h' }, //1
        { "version",        0, NULL, 'v' }, //2
        { "save",           0, NULL, 's' }, //3
        { "read",           0, NULL, 'r' }, //4
        { "show",           0, NULL, 'c' }, //5
        { "selist",         0, NULL, 'l' }, //6
        { "loglevel",       1, NULL, 'L' }, //7
        { "id",             1, NULL, 'N' }, //8
        { "addr",           1, NULL, 'A' }, //9
        { "rip",            1, NULL, 'I' }, //10
        { "rport",          1, NULL, 'P' }, //11
        { 0, 0, 0, 0 },
    };

    cfg.rf433.net_id        = RF433_CFG_NET_ID;
    cfg.rf433.local_addr    = RF433_CFG_LOCAL_ADDR;
    cfg.rf433.m_mask        = 0;

    cfg.socket.server_ip.s_addr = inet_addr(RF433_CFG_SRV_IP);
    cfg.socket.server_port  = RF433_CFG_UDP_PORT;
    cfg.socket.m_mask       = 0;

    se433_set.se433_addr    = 0;
    se433_set.op            = 0;

    if (argc == 1) {
        printf(USAGE);
        exit(EXIT_FAILURE);
    }

    while ((opt = getopt_long(argc, argv, "hvsrclL:N:A:I:P:", longopts, NULL)) != -1) {
        switch (opt) {
            case 's':                   // save
                opt_id |= MSG_REQ_SAVE_CFG;
                break;

            case 'r':                   // read
                opt_id |= MSG_REQ_READ_CFG;
                break;

            case 'c':                   // show
                opt_id |= MSG_REQ_GET_CFG;
                break;

            case 'l':                   // se433 list
                opt_id |= MSG_REQ_GET_SE433L;
                break;

            case 'L':                   // loglevel
                if ((get_log_level(optarg, &log_level)) == -1) {
                    fprintf(stderr, "\n invalid value (%s)\n\n", optarg);
                    ret = -1;
                    goto err;
                }
                opt_id |= MSG_REQ_SET_LOG;
                break;

            case 'N':                   // netid
                if ((get_netid(optarg, &cfg.rf433.net_id)) == -1) {
                    fprintf(stderr, "\n invalid value (%s)\n\n", optarg);
                    ret = -1;
                    goto err;
                }
                cfg.rf433.m_mask |= RF433_MASK_NET_ID;
                break;

            case 'A':                   // addr
                if ((get_local_addr(optarg, &cfg.rf433.local_addr)) == -1) {
                    fprintf(stderr, "\n invalid value (%s)\n\n", optarg);
                    ret = -1;
                    goto err;
                }
                cfg.rf433.m_mask |= RF433_MASK_RCV_ADDR;
                break;
#if 0
            case 'E':                   // seadd
                if ((get_se433_addr(optarg, &se433_set.se433_addr)) == -1) {
                    fprintf(stderr, "\n invalid value (%s)\n\n", optarg);
                    ret = -1;
                    goto err;
                }
                se433_set.op = SE433_OP_ADD;
                break;

            case 'D':                   // sedel
                if ((get_se433_addr(optarg, &se433_set.se433_addr)) == -1) {
                    fprintf(stderr, "\n invalid value (%s)\n\n", optarg);
                    ret = -1;
                    goto err;
                }
                se433_set.op = SE433_OP_DEL;
                break;
#endif
            case 'I':                   // rip
                if ((get_ip(optarg, &cfg.socket.server_ip)) == -1) {
                    fprintf(stderr, "\n invalid value (%s)\n\n", optarg);
                    ret = -1;
                    goto err;
                }
                cfg.socket.m_mask |= SOCK_MASK_SER_IP;
                break;

            case 'P':                   // rport
                if ((get_port(optarg, &cfg.socket.server_port)) == -1) {
                    fprintf(stderr, "\n invalid value (%s)\n\n", optarg);
                    ret = -1;
                    goto err;
                }
                cfg.socket.m_mask |= SOCK_MASK_UDP_PORT;
                break;

            case 'h':
            case 'v':
                printf(USAGE);
                exit(EXIT_SUCCESS);
                break;

            default:
                fprintf(stderr, "%s: invalid opt (%c)\n\n", argv[0], opt);
                printf(USAGE);
                exit(EXIT_FAILURE);
                break;
        }
    }


    ctl_rbuf = buf_new_max();
    if (ctl_rbuf == NULL) {
        printf("new ctl_rbuf memory error");
        goto err;
    }

    ctl_wbuf = buf_new_max();
    if (ctl_wbuf == NULL) {
        printf("new ctl_wbuf memory error");
        goto err;
    }

    // create msg quere
    ctl_fd = open_socket();
    if (ctl_fd < 0) {
        printf("open ctl_fd error");
        goto err;
    }
    set_socket_opt(ctl_fd, CLI_CTL_UDP_PORT);


    /* send and receive message */
    if (cfg.rf433.m_mask || cfg.socket.m_mask) {
        ret = send_msg(ctl_fd, ctl_wbuf, MSG_REQ_SET_CFG, &cfg, sizeof(cfg));
        if (ret != 0)
            goto err;
        ret = recv_msg(ctl_fd, ctl_rbuf);
        if (ret != 0)
            goto err;
    }

    if (opt_id & MSG_REQ_GET_CFG) {
        ret = send_msg(ctl_fd, ctl_wbuf, MSG_REQ_GET_CFG, NULL, 0);
        if (ret != 0)
            goto err;
        ret = recv_msg(ctl_fd, ctl_rbuf);
        if (ret != 0)
            goto err;
    }

    if (opt_id & MSG_REQ_READ_CFG) {
        ret = send_msg(ctl_fd, ctl_wbuf, MSG_REQ_READ_CFG, NULL, 0);
        if (ret != 0)
            goto err;
        ret = recv_msg(ctl_fd, ctl_rbuf);
        if (ret != 0)
            goto err;
    }

    if (opt_id & MSG_REQ_SAVE_CFG) {
        ret = send_msg(ctl_fd, ctl_wbuf, MSG_REQ_SAVE_CFG, NULL, 0);
        if (ret != 0)
            goto err;
        ret = recv_msg(ctl_fd, ctl_rbuf);
        if (ret != 0)
            goto err;
    }

    if (opt_id & MSG_REQ_SET_LOG) {
        ret = send_msg(ctl_fd, ctl_wbuf, MSG_REQ_SET_LOG, &log_level, sizeof(log_level));
        if (ret != 0)
            goto err;
        ret = recv_msg(ctl_fd, ctl_rbuf);
        if (ret != 0)
            goto err;
    }

    if (se433_set.op) {
        ret = send_msg(ctl_fd, ctl_wbuf, MSG_REQ_SET_SE433, &se433_set, sizeof(se433_set));
        if (ret != 0)
            goto err;
        ret = recv_msg(ctl_fd, ctl_rbuf);
        if (ret != 0)
            goto err;
    }

    if (opt_id & MSG_REQ_GET_SE433L) {
        ret = send_msg(ctl_fd, ctl_wbuf, MSG_REQ_GET_SE433L, NULL, 0);
        if (ret != 0)
            goto err;
        ret = recv_msg(ctl_fd, ctl_rbuf);
        if (ret != 0)
            goto err;
    }

err:
    buf_free(ctl_rbuf);
    buf_free(ctl_wbuf);
    if (ctl_fd > 0) {
        close(ctl_fd);
    }
    exit(ret);
}


