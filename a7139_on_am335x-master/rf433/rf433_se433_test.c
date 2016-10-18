//*******************************************************************************
// Copyright (C), RHTECH. Co., Ltd.
// Author:          redfox.qu@qq.com
// Date:            2013/01/04
// Version:         1.0.0
// Description:     rf433 se433 sensor simulator
//
// History:
//      <author>            <time>      <version>   <desc>
//      redfox.qu@qq.com    2014/01/04  1.0.0       create this file
//      redfox.qu@qq.com    2014/01/06  1.0.1       add rswp433 protocol process
//      redfox.qu@qq.com    2014/03/04  1.1.0       upgrade to multi se433 sensor
//
// TODO:
//*******************************************************************************

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>
#include <errno.h>
#include <pthread.h>
#include "rf433lib.h"
#include "common.h"

#define SE433_ADDR      (SE433_ADDR_MIN+1)
#define RF433_ADDR      RF433_BROADCAST
#define RF433_RATE      A7139_RATE_10K

#define SE433_USAGE     "-d <rf433-dev> [-a se433-addr] [-N se433-num] [-n rf433-netid] [-i kj98-rf433-addr] [-r rf433-rate]"

#define DEBUG
//#undef  DEBUG
#ifdef DEBUG
#define debugf(fmt, args...)        fprintf(stderr, fmt, ##args)
#else
#define debugf(fmt, args...)
#endif

struct se433_st {
    pthread_t pid;
    uint32_t se433_addr;
    uint8_t  se433_state;
    uint8_t  se433_type;
    uint16_t se433_timer;
    float    se433_data;
    uint32_t reg_cnt;
    uint32_t poll_cnt;
};


char *se433_devname     = "/dev/rf433-1";
uint32_t rf433_addr     = RF433_ADDR;
uint16_t se433_netid    = RF433_NETID(RF433_NETID_MIN);
uint8_t  se433_rate     = RF433_RATE;
uint32_t se433_addr_0   = SE433_ADDR;
uint8_t  se433_num      = 1;
int rf433_fd;

uint8_t se_vol = 0x33;
uint8_t se_batt = 0x64;
uint8_t se_flag = 0x00;
uint8_t se_whid = 0x00;


struct se433_st *se433_x;
pthread_mutex_t se433_mutex;

void se433_buf_dump(char *buf, int len)
{
    int i;

    for (i = 0; i < len; i++)
    {
        if (i % 16 == 0)
            fprintf(stderr, "\n");
        fprintf(stderr, "%02x ", buf[i]);
    }

    fprintf(stderr, "\n\n");
}

#if 0
int se433_reset(int rf433_fd, struct se433_st *se433i)
{
    struct timeval tv;
    int ret;

    debugf("0x%08x enter se433_reset\n", se433i->se433_addr);

#if 1
    /* RESET DELAY ALGORITHM */
    srandom(se433i->se433_addr);
    tv.tv_sec = 5 + (random() % 50);
    tv.tv_usec = 0;
#else
    tv.tv_sec = 5;
    tv.tv_usec = 0;
#endif

    debugf("0x%08x [reset] ALGORITHM delay %d(s)\n", se433i->se433_addr, (int)tv.tv_sec);

    ret = select(rf433_fd + 1, NULL, NULL, NULL, &tv);

    /* error or break out */
    if (ret == 0) {
        se433i->se433_state = SE433_STATE_REGISTER;
        return 0;
    }

    debugf("0x%08x leave se433_reset\n", se433i->se433_addr);

    /* may be error */
    return ret;
}

int se433_register(int rf433_fd, struct se433_st *se433i)
{
    buffer *se433_buf;
    rswp433_pkg *pkg;
    fd_set rset, wset;
    struct timeval tv;
    unsigned long reg_cnt = 0;
    int ret;

    debugf("0x%08x [register] enter se433_register\n", se433i->se433_addr);

    se433_buf = buf_new_max();
    if (se433_buf == NULL) {
        debugf("0x%08x [register] buf_new_max error\n", se433i->se433_addr);
        return -1;
    }

    /* reset the rf433 register addr */
    rf433_addr = RF433_ADDR;

    do {
        /* first write a register request */
        FD_ZERO(&rset);
        FD_ZERO(&wset);
        FD_SET(rf433_fd, &wset);

        tv.tv_sec = 1;
        tv.tv_usec = 0;

        debugf("\n");
        debugf("0x%08x [register] write cnt %lu\n", se433i->se433_addr, reg_cnt);

        ret = select(rf433_fd + 1, NULL, &wset, NULL, &tv);

        /* error or time out */
        if (ret <= 0) {
            debugf("0x%08x [register] write error or timeout\n", se433i->se433_addr);
            continue;
        }

        if (FD_ISSET(rf433_fd, &wset)) {

            /* send a register request */
            buf_clean(se433_buf);

            pkg = (rswp433_pkg*)buf_data(se433_buf);
            pkg->header.sp[0] = RSWP433_PKG_SP_0;
            pkg->header.sp[1] = RSWP433_PKG_SP_1;
            pkg->header.len = sizeof(rswp433_pkg_content);
            pkg->header.crc1 = crc8((char*)&pkg->header, sizeof(rswp433_pkg_header) - 1);

            pkg->u.content.dest_addr = rf433_addr;
            pkg->u.content.src_addr = se433i->se433_addr;
            pkg->u.content.cmd = RSWP433_CMD_REG_REQ;
            pkg->u.content.crc2 = crc8((char*)&pkg->u.content, sizeof(rswp433_pkg_content) - 1);

            buf_incrlen(se433_buf, sizeof(rswp433_pkg));

            debugf("0x%08x [register] to netid=0x%04x dest=0x%08x, src=0x%08x, cmd=0x%02x\n",
                    se433i->se433_addr, se433_netid, rf433_addr, se433i->se433_addr, RSWP433_CMD_REG_REQ);

            buf_dump(se433_buf);

            pthread_mutex_lock(&se433_mutex);
            buf_write(rf433_fd, se433_buf);
            pthread_mutex_unlock(&se433_mutex);
            reg_cnt++;
        }

        /* second read a register response */
        FD_ZERO(&rset);
        FD_ZERO(&wset);
        FD_SET(rf433_fd, &rset);

        /* REGISTER DELAY ALGORITHM */
#ifndef DEBUG
        srandom(time(NULL));
        tv.tv_sec = 7 * (reg_cnt < 30 ? reg_cnt : 30) + (random() % 50);
        tv.tv_usec = 0;
#else
        /* test */
        tv.tv_sec = 5;
        tv.tv_usec = 0;
#endif

        debugf("0x%08x [register] ALGORITHM delay %d(s)\n", se433i->se433_addr, (int)tv.tv_sec);

        ret = select(rf433_fd + 1, &rset, NULL, NULL, &tv);

        /* error or time out */
        if (ret <= 0) {
            debugf("0x%08x [register] read error or timeout\n", se433i->se433_addr);
            continue;
        }

        if (FD_ISSET(rf433_fd, &rset)) {

            /* receive a register response */
            buf_clean(se433_buf);

            pthread_mutex_lock(&se433_mutex);
            ret = buf_read(rf433_fd, se433_buf);
            pthread_mutex_unlock(&se433_mutex);

            if (ret <= 0) {
                debugf("0x%08x [register] read error return %d\n", se433i->se433_addr, ret);
                continue;
            }

            debugf("0x%08x [register] read %d bytes from 433\n", se433i->se433_addr, ret);
            /* se433_buf_dump(se433_buf); */

            pkg = rswp433_pkg_new();
            if (pkg == NULL) {
                debugf("0x%08x [register] rswp433_pkg_new error\n", se433i->se433_addr);
                goto out;
            }

            if (rswp433_pkg_analysis(se433_buf, pkg) == 1) {

                if (pkg->u.content.cmd != RSWP433_CMD_REG_RSP) {
                    debugf("0x%08x [register] invalid cmd 0x%x on state %d\n", se433i->se433_addr,
                            pkg->u.content.cmd, se433i->se433_state);
                    rswp433_pkg_del(pkg);
                    continue;
                }

                /* ok, my register request is passed,
                 * so change to poll state
                 */
                rf433_addr = pkg->u.content.src_addr;
                se433i->se433_state = SE433_STATE_POLL;

                debugf("0x%08x [register] got a valid pkg, cmd=0x%x\n", se433i->se433_addr, pkg->u.content.cmd);
                debugf("0x%08x [register] se433_state=%d\n", se433i->se433_addr, se433i->se433_state);
                debugf("0x%08x [register] rf433_addr=0x%x\n", se433i->se433_addr, rf433_addr);
            }
            else {
                debugf("0x%08x [register] got a invalid pkg, continue register. \n", se433i->se433_addr);
                buf_dump(se433_buf);
                buf_clean(se433_buf);
            }

            rswp433_pkg_del(pkg);
        }
    } while (se433i->se433_state == SE433_STATE_REGISTER);

out:
    buf_free(se433_buf);

    debugf("0x%08x [register] leave se433_register\n", se433i->se433_addr);

    return ret;
}

int se433_poll(int rf433_fd, struct se433_st *se433i)
{
    buffer *se433_buf;
    rswp433_pkg *rpkg, *wpkg;
    fd_set rset, wset;
    struct timeval tv;
    uint8_t se_vol = 0x33;
    uint8_t se_batt = 0x64;
    uint8_t se_flag = 0x00;
    uint8_t se_whid = 0x00;
    unsigned long poll_cnt = 0;
    int ret;

    debugf("0x%08x [poll] enter se433_poll\n", se433i->se433_addr);

    se433_buf = buf_new_max();
    if (se433_buf == NULL) {
        debugf("0x%08x [poll] buf_new_max error\n", se433i->se433_addr);
        return -1;
    }

    rpkg = rswp433_pkg_new();
    if (rpkg == NULL) {
        debugf("0x%08x [poll] rswp433_pkg_new error\n", se433i->se433_addr);
        return -1;
    }

    do {
        /* first read a poll request */
        FD_ZERO(&rset);
        FD_ZERO(&wset);
        FD_SET(rf433_fd, &rset);

        /* POLL OFFLINE DELAY ALGORITHM */
        tv.tv_sec = RF433_SE433_MAX * RSWP433_OFFLINE_CNT;
        tv.tv_usec = 0;

        debugf("0x%08x [poll] ALGORITHM delay %d(s)\n", se433i->se433_addr, (int)tv.tv_sec);

        ret = select(rf433_fd + 1, &rset, NULL, NULL, &tv);

        /* error */
        if (ret < 0) {
            debugf("0x%08x [poll] read error\n", se433i->se433_addr);
            continue;
        }

        /* time out */
        else if (ret == 0) {
            se433i->se433_state = SE433_STATE_RESET;
            debugf("0x%08x [poll] time out se433_state=%d\n", se433i->se433_addr, se433i->se433_state);
            goto out;
        }

        if (FD_ISSET(rf433_fd, &rset)) {

            /* receive a poll request */
            buf_clean(se433_buf);

            pthread_mutex_lock(&se433_mutex);
            ret = buf_read(rf433_fd, se433_buf);
            pthread_mutex_unlock(&se433_mutex);

            if (ret <= 0) {
                debugf("0x%08x [poll] read return error %d\n", se433i->se433_addr, ret);
                continue;
            }

            debugf("0x%08x [poll] read %d bytes from 433\n", se433i->se433_addr, ret);
            /* se433_buf_dump(se433_buf); */


            if (rswp433_pkg_analysis(se433_buf, rpkg) == 1) {
                if (rpkg->u.content.cmd != RSWP433_CMD_DATA_REQ) {
                    debugf("0x%08x [poll] invalid cmd 0x%x on state %d\n", se433i->se433_addr,
                            rpkg->u.content.cmd, se433i->se433_state);
                    rswp433_pkg_del(rpkg);
                    continue;
                }

                if (rpkg->u.content.src_addr != rf433_addr) {
                    debugf("0x%08x [poll] invalid host kj98-f addr 0x%x on state %d\n",
                            se433i->se433_addr, rpkg->u.content.src_addr, se433i->se433_state);
                    rswp433_pkg_del(rpkg);
                    continue;
                }

                /* ok, got a data poll request,
                 * so prepare data to response
                 */
                buf_clean(se433_buf);

                debugf("0x%08x [poll] got a valid rswp433 pkg, cmd=0x%x, poll_cnt=%lu\n",
                        se433i->se433_addr, rpkg->u.content.cmd, poll_cnt++);

            } else {
                debugf("0x%08x [poll] invalid rswp433_pkg on state %d\n", se433i->se433_addr, se433i->se433_state);
                buf_dump(se433_buf);
                buf_clean(se433_buf);
                continue;
            }
        }

        /* next write a data response */
        FD_ZERO(&rset);
        FD_ZERO(&wset);
        FD_SET(rf433_fd, &wset);

        /* POLL OFFLINE DELAY ALGORITHM */
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        debugf("0x%08x [poll] write delay %d(s)\n", se433i->se433_addr, (int)tv.tv_sec);

        ret = select(rf433_fd + 1, NULL, &wset, NULL, &tv);

        /* error */
        if (ret <= 0) {
            debugf("0x%08x [poll] write error or timeout\n", se433i->se433_addr);
            continue;
        }

        if (FD_ISSET(rf433_fd, &wset)) {

            /* send a register request */
            buf_clean(se433_buf);
            srandom(time(NULL));

            wpkg = (rswp433_pkg*)buf_data(se433_buf);
            wpkg->header.sp[0] = RSWP433_PKG_SP_0;
            wpkg->header.sp[1] = RSWP433_PKG_SP_1;
            wpkg->header.len = sizeof(rswp433_pkg_data_content);
            wpkg->header.crc1 = crc8((char*)&wpkg->header, sizeof(rswp433_pkg_header) - 1);

            wpkg->u.data_content.dest_addr = rf433_addr;
            wpkg->u.data_content.src_addr = se433i->se433_addr;
            wpkg->u.data_content.cmd = RSWP433_CMD_DATA_RSP;
            wpkg->u.data_content.data.type = se433i->se433_type;
            wpkg->u.data_content.data.data = (se433i->se433_data += (((float)(random() % 5) - 2) / 100));
            wpkg->u.data_content.data.vol = se_vol;
            wpkg->u.data_content.data.batt = se_batt;
            wpkg->u.data_content.data.flag = se_flag;
            wpkg->u.data_content.data.watchid = se_whid;
            wpkg->u.data_content.crc2 = crc8((char*)&wpkg->u.data_content, sizeof(rswp433_pkg_data_content) - 1);

            buf_incrlen(se433_buf, sizeof(rswp433_pkg));

            debugf("0x%08x [poll] response dest_addr=0x%08x, src_addr=0x%08x, data=%.2f, crc1=0x%02x, crc2=0x%02x\n",
                    se433i->se433_addr,
                    wpkg->u.data_content.dest_addr,
                    wpkg->u.data_content.src_addr,
                    wpkg->u.data_content.data.data,
                    wpkg->header.crc1,
                    wpkg->u.data_content.crc2);

            pthread_mutex_lock(&se433_mutex);
            buf_write(rf433_fd, se433_buf);
            pthread_mutex_unlock(&se433_mutex);
        }
    } while (se433i->se433_state == SE433_STATE_POLL);

out:
    buf_free(se433_buf);
    rswp433_pkg_del(rpkg);

    return ret;
}

void *se433_job(void *arg)
{
    struct se433_st *se433i;

    se433i = se433_x + (int)arg;

    while (1) {
        se433_reset(rf433_fd, se433i);
        se433_register(rf433_fd, se433i);
        se433_poll(rf433_fd, se433i);
    }
}

#endif

struct se433_st *se433_test_init(int num)
{
    struct se433_st *se433_i;
    int i;

    if (num <= 0 || num >= RF433_SE433_MAX * 2) {
        fprintf(stderr, "se433 num %d error!\n", num);
        return NULL;
    }

    se433_i = (struct se433_st*)malloc(sizeof(struct se433_st) * num);
    if (se433_i == NULL) {
        return NULL;
    }

    bzero(se433_i, sizeof(struct se433_st) * num);


    for (i = 0; i < num; i++) {
        se433_i[i].pid = -1;
        se433_i[i].se433_addr = se433_addr_0 + i;
        se433_i[i].se433_state = SE433_STATE_RESET;
        se433_i[i].se433_type = (random() % (SE433_TYPE_MAX - 1)) + 1;
        se433_i[i].se433_data = 0.0;

        srandom(se433_i[i].se433_addr);
        se433_i[i].se433_timer = 5 + (random() % 50);

        debugf("new se433 addr=0x%08x, type=%d, reset_tm=%d\n",
                se433_i[i].se433_addr, se433_i[i].se433_type, se433_i[i].se433_timer);
    }

    return se433_i;
}

void se433_test_list(void)
{
    int i;

    for (i = 0; i < se433_num; i++) {
        debugf("[list] 0x%08x state=%s timer=%d, reg_cnt=%d poll_cnt=%d\n",
                se433_x[i].se433_addr,
                se433_get_state_str(se433_x[i].se433_state),
                se433_x[i].se433_timer,
                se433_x[i].reg_cnt,
                se433_x[i].poll_cnt);
    }
}


int main(int argc, char *argv[])
{
    int opt;
    int i, ret;
    fd_set rset, wset;
    struct timeval tv;
    buffer *se433_buf;
    rswp433_pkg *pkg;

    if (argc < 2) {
        fprintf(stderr, "%s: %s\n", argv[0], SE433_USAGE);
        exit(EXIT_FAILURE);
    }

    while ((opt = getopt(argc, argv, "d:a:n:i:r:N:")) != -1) {
        switch (opt) {
            case 'd':
                se433_devname = optarg;
                break;

            case 'a':
                /*se433_addr_0 = atoi(optarg);*/
                sscanf(optarg, "%x", &se433_addr_0);
                break;

            case 'n':
                se433_netid = RF433_NETID(atoi(optarg));
                break;

            case 'i':
                rf433_addr = atoi(optarg);
                break;

            case 'r':
                se433_rate = atoi(optarg);
                break;

            case 'N':
                se433_num = atoi(optarg);
                break;

            default: /* '?' */
                fprintf(stderr, "%s: %s\n", argv[0], SE433_USAGE);
                exit(EXIT_FAILURE);
        }
    }
#if 0
    if (optind >= argc) {
        fprintf(stderr, "Expected argument after options\n");
        exit(EXIT_FAILURE);
    }
#endif

    set_loglevel(LOG_DEBUG);

    rf433_fd = open(se433_devname, O_RDWR);
    if (rf433_fd == -1) {
        fprintf(stderr, "open %s error, %s\n", se433_devname, strerror(errno));
        exit(EXIT_FAILURE);
    }

    debugf("%s opend\n", se433_devname);

    if (set_rf433_opt(rf433_fd, se433_netid, se433_rate) < 0) {
        fprintf(stderr, "set rf433 config error\n");
        exit(EXIT_FAILURE);
    }
    debugf("set netid=0x%x, rate=%d\n", se433_netid, se433_rate);

    se433_x = se433_test_init(se433_num);

    se433_buf = buf_new_max();
    if (se433_buf == NULL) {
        debugf("buf_new_max error\n");
        return -1;
    }

    while (1) {

        /* first write a register request */
        FD_ZERO(&rset);
        FD_ZERO(&wset);

        for (i = 0; i < se433_num; i++) {
            if ((se433_x[i].se433_state == SE433_STATE_REG_REQ) ||
                (se433_x[i].se433_state == SE433_STATE_POLL_RSP)) {
                FD_SET(rf433_fd, &wset);
            }
            if ((se433_x[i].se433_state == SE433_STATE_REG_RSP) ||
                (se433_x[i].se433_state == SE433_STATE_POLL_REQ)) {
                FD_SET(rf433_fd, &rset);
            }
        }

        tv.tv_sec = 1;
        tv.tv_usec = 0;

        ret = select(rf433_fd + 1, &rset, &wset, NULL, &tv);

        /* select error */
        if (ret < 0) {
            perror("select");
            continue;
        }

        /* time out */
        else if (ret == 0) {


            for (i = 0; i < se433_num; i++) {

                /* each se433 timer decrease */
                se433_x[i].se433_timer--;

                if (se433_x[i].se433_timer == 0) {

                    srandom(se433_x[i].se433_addr);

                    switch (se433_x[i].se433_state) {
                        case SE433_STATE_RESET:
                            se433_x[i].se433_state = SE433_STATE_REG_REQ;
                            se433_x[i].se433_timer = 7 * (se433_x[i].reg_cnt < 30 ? se433_x[i].reg_cnt : 30) + (random() % 50);
                            debugf("0x%08x [state] se433_state=%s\n", se433_x[i].se433_addr, se433_get_state_str(se433_x[i].se433_state));
                            break;
                        case SE433_STATE_REG_RSP:
                            se433_x[i].se433_state = SE433_STATE_REG_REQ;
                            se433_x[i].se433_timer = 7 * (se433_x[i].reg_cnt < 30 ? se433_x[i].reg_cnt : 30) + (random() % 50);
                            debugf("0x%08x [state] se433_state=%s\n", se433_x[i].se433_addr, se433_get_state_str(se433_x[i].se433_state));
                            break;
                        case SE433_STATE_POLL_REQ:
                            se433_x[i].se433_state = SE433_STATE_RESET;
                            se433_x[i].se433_timer = 5 + (random() % 50);
                            debugf("0x%08x [state] se433_state=%s\n", se433_x[i].se433_addr, se433_get_state_str(se433_x[i].se433_state));
                            break;
                        default:
                            break;
                    }
                }
            }

            se433_test_list();

            continue;
        }

        if (FD_ISSET(rf433_fd, &wset)) {

            for (i = 0; i < se433_num; i++) {
                if (se433_x[i].se433_state == SE433_STATE_POLL_RSP) {
                    /* send a register request */
                    buf_clean(se433_buf);
                    srandom(time(NULL));

                    pkg = (rswp433_pkg*)buf_data(se433_buf);
                    pkg->header.sp[0] = RSWP433_PKG_SP_0;
                    pkg->header.sp[1] = RSWP433_PKG_SP_1;
                    pkg->header.len = sizeof(rswp433_pkg_data_content);
                    pkg->header.crc1 = crc8((char*)&pkg->header, sizeof(rswp433_pkg_header) - 1);

                    pkg->u.data_content.dest_addr = rf433_addr;
                    pkg->u.data_content.src_addr = se433_x[i].se433_addr;
                    pkg->u.data_content.cmd = RSWP433_CMD_DATA_RSP;
                    pkg->u.data_content.data.type = se433_x[i].se433_type;
                    pkg->u.data_content.data.data = (se433_x[i].se433_data += (((float)(random() % 5) - 2) / 100));
                    pkg->u.data_content.data.vol = se_vol;
                    pkg->u.data_content.data.batt = se_batt;
                    pkg->u.data_content.data.flag = se_flag;
                    pkg->u.data_content.data.watchid = se_whid;
                    pkg->u.data_content.crc2 = crc8((char*)&pkg->u.data_content, sizeof(rswp433_pkg_data_content) - 1);

                    buf_incrlen(se433_buf, sizeof(rswp433_pkg));

                    debugf("0x%08x [poll] response dest_addr=0x%08x, src_addr=0x%08x, data=%.2f, crc1=0x%02x, crc2=0x%02x\n",
                            se433_x[i].se433_addr,
                            pkg->u.data_content.dest_addr,
                            pkg->u.data_content.src_addr,
                            pkg->u.data_content.data.data,
                            pkg->header.crc1,
                            pkg->u.data_content.crc2);

                    buf_dump(se433_buf);
                    buf_write(rf433_fd, se433_buf);
                    se433_x[i].se433_state = SE433_STATE_POLL_REQ;
                    se433_x[i].se433_timer = 300;

                    break;
                }

                else if (se433_x[i].se433_state == SE433_STATE_REG_REQ) {
                    /* send a register request */
                    buf_clean(se433_buf);

                    pkg = (rswp433_pkg*)buf_data(se433_buf);
                    pkg->header.sp[0] = RSWP433_PKG_SP_0;
                    pkg->header.sp[1] = RSWP433_PKG_SP_1;
                    pkg->header.len = sizeof(rswp433_pkg_content);
                    pkg->header.crc1 = crc8((char*)&pkg->header, sizeof(rswp433_pkg_header) - 1);

                    pkg->u.content.dest_addr = RF433_BROADCAST;
                    pkg->u.content.src_addr = se433_x[i].se433_addr;
                    pkg->u.content.cmd = RSWP433_CMD_REG_REQ;
                    pkg->u.content.crc2 = crc8((char*)&pkg->u.content, sizeof(rswp433_pkg_content) - 1);

                    buf_incrlen(se433_buf, sizeof(rswp433_pkg));

                    debugf("0x%08x [register] to netid=0x%04x dest=0x%08x, src=0x%08x, cmd=0x%02x\n",
                            se433_x[i].se433_addr, se433_netid, rf433_addr, se433_x[i].se433_addr, RSWP433_CMD_REG_REQ);

                    buf_dump(se433_buf);
                    buf_write(rf433_fd, se433_buf);
                    se433_x[i].reg_cnt++;
                    se433_x[i].se433_state = SE433_STATE_REG_RSP;

                    break;
                }
            }
        }

        if (FD_ISSET(rf433_fd, &rset)) {

            /* receive a register response */
            buf_clean(se433_buf);

            /* read a packet first */
            ret = buf_read(rf433_fd, se433_buf);

            if (ret <= 0) {
                debugf("read error return %d\n", ret);
                continue;
            }

            pkg = rswp433_pkg_new();
            if (pkg == NULL) {
                debugf("rswp433_pkg_new error\n");
                goto out;
            }

            /* check the packet valid */
            if (rswp433_pkg_analysis(se433_buf, pkg) != 1) {
                debugf("got a invalid pkg\n");
                buf_dump(se433_buf);
                buf_clean(se433_buf);
                goto read_out;
            }

            for (i = 0; i < se433_num; i++) {

                /* find the se433 */
                if (pkg->u.content.dest_addr == se433_x[i].se433_addr) {

                    /* save the rf433 kj98-f address */
                    rf433_addr = pkg->u.content.src_addr;

                    switch (se433_x[i].se433_state) {

                        case SE433_STATE_REG_RSP:
                            if (pkg->u.content.cmd != RSWP433_CMD_REG_RSP) {
                                debugf("0x%08x [register] invalid cmd 0x%x on state %d\n", se433_x[i].se433_addr,
                                        pkg->u.content.cmd, se433_x[i].se433_state);
                                goto read_out;
                            }

                            /* ok, my register request is passed,
                             * so change to poll state
                             */
                            se433_x[i].se433_state = SE433_STATE_POLL_REQ;
                            se433_x[i].se433_timer = 300;

                            debugf("0x%08x [register] got a valid pkg, cmd=0x%x\n", se433_x[i].se433_addr, pkg->u.content.cmd);
                            debugf("0x%08x [register] se433_state=%d\n", se433_x[i].se433_addr, se433_x[i].se433_state);
                            debugf("0x%08x [register] rf433_addr=0x%x\n", se433_x[i].se433_addr, rf433_addr);

                            goto read_out;

                            break;

                        case SE433_STATE_POLL_REQ:
                            if (pkg->u.content.cmd != RSWP433_CMD_DATA_REQ) {
                                debugf("0x%08x [poll] invalid cmd 0x%x on state %d\n", se433_x[i].se433_addr,
                                        pkg->u.content.cmd, se433_x[i].se433_state);
                                goto read_out;
                            }

                            if (pkg->u.content.src_addr != rf433_addr) {
                                debugf("0x%08x [poll] invalid host kj98-f addr 0x%x on state %d\n",
                                        se433_x[i].se433_addr, pkg->u.content.src_addr, se433_x[i].se433_state);
                                goto read_out;
                            }

                            /* ok, got a data poll request,
                             * so prepare data to response
                             */
                            se433_x[i].se433_state = SE433_STATE_POLL_RSP;

                            debugf("0x%08x [poll] got a valid rswp433 pkg, cmd=0x%x, poll_cnt=%u\n",
                                    se433_x[i].se433_addr, pkg->u.content.cmd, se433_x[i].poll_cnt++);
                            debugf("0x%08x [poll] se433_state=%d\n", se433_x[i].se433_addr, se433_x[i].se433_state);
                            debugf("0x%08x [poll] rf433_addr=0x%x\n", se433_x[i].se433_addr, rf433_addr);

                            goto read_out;

                            break;

                        default:
                            debugf("0x%08x [xxx-xxx] invalid cmd 0x%x on state %d\n", se433_x[i].se433_addr,
                                    pkg->u.content.cmd, se433_x[i].se433_state);
                            break;
                    }
                }
            }
read_out:
            rswp433_pkg_del(pkg);

        }

    }

out:
    free(se433_x);
    close(rf433_fd);

    exit(0);
}


