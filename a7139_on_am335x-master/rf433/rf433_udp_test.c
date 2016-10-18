//*******************************************************************************
// Copyright (C), RHTECH. Co., Ltd.
// Author:          redfox.qu@qq.com
// Date:            2014/01/04
// Version:         1.0.0
// Description:     rf433 KJ98 udp packet simulator
//
// History:
//      <author>            <time>      <version>   <desc>
//      redfox.qu@qq.com    2014/01/04  1.0.0       create this file
//
// TODO:
//*******************************************************************************

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "rf433lib.h"
#include "common.h"

struct se_list {
    uint32_t se_addr;
    uint8_t se_type;
    float se_data;
};

struct se_list se433_array[] = {
    {   /* 1 */
        .se_addr = 0x55555501,
        .se_type = SE433_TYPE_CH4,
        .se_data = 0.0,
    },
    {   /* 2 */
        .se_addr = 0x55555502,
        .se_type = SE433_TYPE_CO,
        .se_data = 10,
    },
    {   /* 3 */
        .se_addr = 0x55555503,
        .se_type = SE433_TYPE_CH4,
        .se_data = 0.0,
    },
    {   /* 4 */
        .se_addr = 0x55555504,
        .se_type = SE433_TYPE_TEMP,
        .se_data = 10,
    },
    {   /* 5 */
        .se_addr = 0x55555505,
        .se_type = SE433_TYPE_TEMP,
        .se_data = 14,
    },
    {   /* 6 */
        .se_addr = 0x55555506,
        .se_type = SE433_TYPE_TEMP,
        .se_data = 14,
    },
    {   /* 7 */
        .se_addr = 0x55555507,
        .se_type = SE433_TYPE_TEMP,
        .se_data = 14,
    },
    {   /* 8 */
        .se_addr = 0x55555508,
        .se_type = SE433_TYPE_TEMP,
        .se_data = 14,
    },
    {   /* 9 */
        .se_addr = 0x55555509,
        .se_type = SE433_TYPE_TEMP,
        .se_data = 14,
    },
    {   /* 10 */
        .se_addr = 0x5555550a,
        .se_type = SE433_TYPE_TEMP,
        .se_data = 14,
    },
    {   /* 11 */
        .se_addr = 0x5555550b,
        .se_type = SE433_TYPE_TEMP,
        .se_data = 14,
    },
    {   /* 12 */
        .se_addr = 0x5555550c,
        .se_type = SE433_TYPE_TEMP,
        .se_data = 14,
    },
    {   /* 13 */
        .se_addr = 0x5555550d,
        .se_type = SE433_TYPE_TEMP,
        .se_data = 14,
    },
    {   /* 14 */
        .se_addr = 0x5555550e,
        .se_type = SE433_TYPE_TEMP,
        .se_data = 14,
    },
    {   /* 15 */
        .se_addr = 0x5555550f,
        .se_type = SE433_TYPE_TEMP,
        .se_data = 14,
    },
    {   /* 16 */
        .se_addr = 0x55555510,
        .se_type = SE433_TYPE_TEMP,
        .se_data = 14,
    },
    {   /* 17 */
        .se_addr = 0x55555511,
        .se_type = SE433_TYPE_CH4,
        .se_data = 0.0,
    },
    {   /* 18 */
        .se_addr = 0x55555512,
        .se_type = SE433_TYPE_TEMP,
        .se_data = 14,
    },
    {   /* 19 */
        .se_addr = 0x55555513,
        .se_type = SE433_TYPE_CO,
        .se_data = 10,
    },
    {   /* 20 */
        .se_addr = 0x55555514,
        .se_type = SE433_TYPE_CH4,
        .se_data = 0.0,
    },
    {   /* 21 */
        .se_addr = 0x55555515,
        .se_type = SE433_TYPE_TEMP,
        .se_data = 10,
    },
    {   /* 22 */
        .se_addr = 0x55555516,
        .se_type = SE433_TYPE_TEMP,
        .se_data = 14,
    },
    {   /* 23 */
        .se_addr = 0x55555517,
        .se_type = SE433_TYPE_TEMP,
        .se_data = 14,
    },
    {   /* 24 */
        .se_addr = 0x55555518,
        .se_type = SE433_TYPE_TEMP,
        .se_data = 14,
    },
    {   /* 25 */
        .se_addr = 0x55555519,
        .se_type = SE433_TYPE_TEMP,
        .se_data = 14,
    },
    {   /* 26 */
        .se_addr = 0x5555551a,
        .se_type = SE433_TYPE_TEMP,
        .se_data = 14,
    },
    {   /* 27 */
        .se_addr = 0x5555551b,
        .se_type = SE433_TYPE_TEMP,
        .se_data = 14,
    },
    {   /* 28 */
        .se_addr = 0x5555551c,
        .se_type = SE433_TYPE_TEMP,
        .se_data = 14,
    },
    {   /* 29 */
        .se_addr = 0x5555551d,
        .se_type = SE433_TYPE_TEMP,
        .se_data = 14,
    },
    {   /* 30 */
        .se_addr = 0x5555551e,
        .se_type = SE433_TYPE_TEMP,
        .se_data = 14,
    },
    {   /* 31 */
        .se_addr = 0x5555551f,
        .se_type = SE433_TYPE_TEMP,
        .se_data = 14,
    },
    {   /* 32 */
        .se_addr = 0x55555520,
        .se_type = SE433_TYPE_TEMP,
        .se_data = 14,
    },
};

int main(int argc, char *argv[])
{
    struct sockaddr_in srv_addr;
    rswp433_pkg_data_content *rswp433_data;
    char udp_buf[1024];
    int sockfd;
    int len, ret, seid = 0;

    if (argc != 3) {
        fprintf(stderr, "%s: <ip-addr> <udp-port>\n", argv[0]);
        exit(-1);
    }

    sockfd = open_socket();
    if (sockfd < 0) {
        fprintf(stderr, "open socket error");
        exit(-1);
    }

    rswp433_data = (rswp433_pkg_data_content*)&udp_buf[0];

    len = sizeof(srv_addr);
    srv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    srv_addr.sin_port = htons(atoi(argv[2]));
    srv_addr.sin_family = AF_INET;

    while (1) {
        srandom(time(NULL));

        rswp433_data->dest_addr = RF433_CFG_LOCAL_ADDR;
        rswp433_data->src_addr = se433_array[seid].se_addr;
        rswp433_data->cmd = RSWP433_CMD_DATA_RSP;
        rswp433_data->data.type = se433_array[seid].se_type;
        rswp433_data->data.data = (se433_array[seid].se_data += (((float)(random() % 5) - 2) / 100));
        rswp433_data->data.vol = 0x33;
        rswp433_data->data.batt = 0x64;
        rswp433_data->data.flag = 0x00;
        rswp433_data->data.watchid = 0x01;
        rswp433_data->crc2 = 0x00;

        fprintf(stderr, "se:0x%08x type:%02d data:%4.1f vol:0x%02x batt:0x%02x flag:0x%02x\n",
                rswp433_data->src_addr, rswp433_data->data.type, rswp433_data->data.data,
                rswp433_data->data.vol, rswp433_data->data.batt, rswp433_data->data.flag);

        ret = sendto(sockfd, udp_buf, sizeof(rswp433_pkg_data_content), 0, (struct sockaddr*)&srv_addr, len);

        sleep(RF433_SE433_MAX / ARRAY_SIZE(se433_array));

        seid = (seid+1) % ARRAY_SIZE(se433_array);
    }


    close(sockfd);

    exit(0);
}

