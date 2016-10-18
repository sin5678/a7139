//*******************************************************************************
// Copyright (C), RHTECH. Co., Ltd.
// Author:          redfox.qu@qq.com
// Date:            2013/12/26
// Version:         1.0.0
// Description:     The 433Mhz module library header file
//
// History:
//      <author>            <time>      <version>   <desc>
//      redfox.qu@qq.com    2013/12/26  1.0.0       create this file
//
// TODO:
//      2013/12/31          add the common and debug
//*******************************************************************************


#ifndef __RF433_LIB_H__
#define __RF433_LIB_H__

#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "rfrepeater.h"
#include "buffer.h"
#include "rf433pkg.h"
#include "common.h"
#include "list.h"
#include "drivers/char/a7139.h"

#define RSWP433_CMD_REG_REQ     0xc1        /* 433 wireless sensor register to kj98-f */
#define RSWP433_CMD_REG_RSP     0xc2
#define RSWP433_CMD_DATA_REQ    0xc3
#define RSWP433_CMD_DATA_RSP    0xc4

enum SE433_STATE {
    SE433_STATE_NON             = 0x00,     /* sensor state */
    SE433_STATE_RESET,
    SE433_STATE_REGISTER,
    SE433_STATE_REG_REQ,
    SE433_STATE_REG_RSP,
    SE433_STATE_POLL,
    SE433_STATE_POLL_REQ,
    SE433_STATE_POLL_RSP,
};

enum SE433_TYPE {
    SE433_TYPE_NON              = 0x00,     /* sensor type id code */
    SE433_TYPE_CH4,
    SE433_TYPE_CO,
    SE433_TYPE_TEMP,
    SE433_TYPE_MAX,
};

#define RSWP433_PKG_SP_0        0x5a        /* 软件前导码 */
#define RSWP433_PKG_SP_1        0xa5

#define RSWP433_OFFLINE_CNT     5           /* 传感器超时最大重试次数 */

#define RSWP433_FLAG_MPOW       0x01        /* 传感器供电 (0:主电, 1:备电) */
#define RSWP433_FLAG_PROBE      0x02        /* 传感器探头 (0:正常, 1:异常) */
#define RSWP433_FLAG_BATTERY    0x03        /* 传感器电池 (0:正常, 1:异常) */

#define INST_START              0x00000001
#define INST_STAUTS(i)          (i & INST_START)

typedef struct {                            /* 传感器链表头 */
    struct list_head list;
    int num;
} se433_head;

typedef struct {                            /* 传感器链表结构 */
    struct list_head list;
    enum SE433_STATE state;                 /* 传感器状态 */
    se433_data se433;
} se433_list;

typedef struct {                            /* 433设备模块 */
    socket_cfg socket;
    rf433_cfg rf433;
    se433_head se433;
    pthread_t tid;
    uint32_t status;
    int pipe_fd;
    int sock_fd;
    int rf433_fd;
} rf433_instence;

char *rf433_get_freq_str(A7139_FREQ wfreq);
char *rf433_get_rate_str(A7139_RATE rate);
char *se433_get_type_str(uint8_t type);
char *se433_get_state_str(enum SE433_STATE state);
char *se433_get_vol_str(uint8_t vol);
char *se433_get_batt_str(uint8_t batt);
char *se433_get_flags_str(uint8_t flags);

int open_rf433(char *dev);
int open_socket(void);
void set_non_blk(int fd);
int set_socket_opt(int fd, uint16_t lport);
int set_rf433_opt(int fd, uint16_t netid, uint8_t rate);
int rf433_set_netid(int fd, uint16_t net_id);
int rf433_get_netid(int fd, uint16_t *net_id);
int rf433_set_wfreq(int fd, uint8_t wfreq);
int rf433_get_wfreq(int fd, uint8_t *wfreq);
int rf433_set_rate(int fd, uint8_t rate);
int rf433_get_rate(int fd, uint8_t *rate);

se433_list *se433_find(se433_head *head, uint32_t se433_addr);
se433_list *se433_find_earliest(se433_head *head);
se433_list *se433_find_offline(se433_head *head);
se433_list *se433_add(se433_head *head, uint32_t addr);
int se433_del(se433_head *head, uint32_t se433_addr);
int se433_clean(se433_head *head);
void se433_list_show(se433_head *head);

int rswp433_pkg_analysis(buffer *buf, rswp433_pkg *pkg);
rswp433_pkg *rswp433_pkg_new(void);
void rswp433_pkg_del(rswp433_pkg *pkg);
void rswp433_pkg_clr(rswp433_pkg *pkg);

se433_list *rswp433_reg_req(rswp433_pkg *pkg, rf433_instence *rf433x);
int rswp433_reg_rsp(se433_list *se433, rf433_instence *rf433x, buffer *buf);
int rswp433_data_req(se433_list *se433, rf433_instence *rf433x, buffer *buf);
int rswp433_data_rsp(rswp433_pkg *pkg, rf433_instence *rf433x, buffer *buf);

#endif

