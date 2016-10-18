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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sockcli.h"
#include "common.h"
#include "rf433lib.h"

#define APP_NAME    "rf433"
#define SRV_ADDR    "127.0.0.1"
#define SRV_PORT    SRV_CTL_UDP_PORT

#define USAGE   " \
Usage: rf433 [options] \n \
    -h, --help                  Show this USAGE \n \
    -v, --version               Show version \n \
    -s, --save                  Save config to file \n \
    -r, --read                  Read config from file \n \
    -c, --show                  Show the current config \n \
    -l, --selist                Show se433 sensor list \n \
    -L, --loglevel              choose loglevel \n \
                                0:LOG_EMERG,    1:LOG_ALERT \n \
                                2:LOG_CRIT,     3:LOG_ERR \n \
                                4:LOG_WARING,   5:LOG_NOTICE \n \
                                6:LOG_INFO,     7:LOG_DEBUG \n \
config options: \n \
    -N, --id=netid              433 network id \n \
    -A, --addr=433addr          433 receive address \n \
    -I, --rip=ipaddr            set remote server ip address \n \
    -P, --rport=port            set remote server udp port \n \
\n "

se433_cfg se433_set;
struct msg_config cfg;
int log_level, opt_id = 0;

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

    if (argc <= 1) {
        printf(USAGE);
        ret = 0;
        goto err;
    }

    while ((opt = getopt_long(argc, argv, "hvsrclL:N:A:I:P:", longopts, NULL)) != -1) {
        switch (opt) {
            case 's':                   // save
                opt_id = MSG_REQ_SAVE_CFG;
                break;

            case 'r':                   // read
                opt_id = MSG_REQ_READ_CFG;
                break;

            case 'c':                   // show
                opt_id = MSG_REQ_GET_CFG;
                break;

            case 'l':                   // se433 list
                opt_id = MSG_REQ_GET_SE433L;
                break;

            case 'L':                   // loglevel
                if ((get_log_level(optarg, &log_level)) == -1) {
                    fprintf(stderr, "\n invalid value (%s)\n\n", optarg);
                    ret = -1;
                    goto err;
                }
                opt_id = MSG_REQ_SET_LOG;
                break;

            case 'N':                   // netid
                if ((get_netid(optarg, &cfg.rf433.net_id)) == -1) {
                    fprintf(stderr, "\n invalid value (%s)\n\n", optarg);
                    ret = -1;
                    goto err;
                }
                cfg.rf433.m_mask |= RF433_MASK_NET_ID;
                opt_id = MSG_REQ_SET_CFG;
                break;

            case 'A':                   // addr
                if ((get_local_addr(optarg, &cfg.rf433.local_addr)) == -1) {
                    fprintf(stderr, "\n invalid value (%s)\n\n", optarg);
                    ret = -1;
                    goto err;
                }
                cfg.rf433.m_mask |= RF433_MASK_RCV_ADDR;
                opt_id = MSG_REQ_SET_CFG;
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
                opt_id = MSG_REQ_SET_CFG;
                break;

            case 'P':                   // rport
                if ((get_port(optarg, &cfg.socket.server_port)) == -1) {
                    fprintf(stderr, "\n invalid value (%s)\n\n", optarg);
                    ret = -1;
                    goto err;
                }
                cfg.socket.m_mask |= SOCK_MASK_UDP_PORT;
                opt_id = MSG_REQ_SET_CFG;
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
    struct msg_st *wmsg = (struct msg_st *)buf;

    wmsg->h.magic[0] = MSG_CTL_MAGIC_0;
    wmsg->h.magic[1] = MSG_CTL_MAGIC_1;
    wmsg->h.flags = 0;
    wmsg->h.version = MSG_CTL_VERSION;

    wmsg->d.type = opt_id;
    switch (opt_id) {
        case MSG_REQ_SET_CFG:
            memcpy(&wmsg->d.content[0], (char*)&cfg, sizeof(cfg));
            *size = sizeof(wmsg->h) + sizeof(wmsg->d.type) + sizeof(cfg);
            break;

        case MSG_REQ_SET_LOG:
            memcpy(&wmsg->d.content[0], (char*)&log_level, sizeof(log_level));
            *size = sizeof(wmsg->h) + sizeof(wmsg->d.type) + sizeof(log_level);
            break;

        case MSG_REQ_SET_SE433:
            memcpy(&wmsg->d.content[0], (char*)&se433_set, sizeof(se433_set));
            *size = sizeof(wmsg->h) + sizeof(wmsg->d.type) + sizeof(se433_set);
            break;

        case MSG_REQ_GET_CFG:
        case MSG_REQ_READ_CFG:
        case MSG_REQ_SAVE_CFG:
        case MSG_REQ_GET_SE433L:
            *size = sizeof(wmsg->h) + sizeof(wmsg->d.type);
            break;

        default:
            return -1;
    }

    return 0;
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
        printf("addr=0x%08x, req_cnt=%d, rsp_cnt=%d, type=%s, " \
                "data=%.3f, vol=%s, batt=%s, flag=%s, whid=%d\n",
                se433data->addr,
                se433data->req_cnt,
                se433data->rsp_cnt,
                se433_get_type_str(se433data->data.type),
                se433data->data.data,
                se433_get_vol_str(se433data->data.vol),
                se433_get_batt_str(se433data->data.batt),
                se433_get_flags_str(se433data->data.flag),
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

int recv_func(char *buf, int *size)
{
    struct msg_st *rmsg = (struct msg_st *)buf;

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
    return 0;
}





