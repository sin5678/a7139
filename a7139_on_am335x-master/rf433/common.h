//*******************************************************************************
// Copyright (C), RHTECH. Co., Ltd.
// Author:          redfox.qu@qq.com
// Date:            2013/12/25
// Version:         1.0.0
// Description:     The 433Mhz module common function file
//
// History:
//      <author>            <time>      <version>   <desc>
//      redfox.qu@qq.com    2013/12/25  1.0.0       create this file
//
// TODO:
//      2013/12/31          add the common and debug
//*******************************************************************************

#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "list.h"
#include "rf433pkg.h"
#include "applog.h"
#include "buffer.h"

#ifdef UNUSED
#elif defined(__GNUC__)
#define UNUSED(x) UNUSED_ ## x __attribute__((unused))
#elif defined(__LCLINT__)
#define UNUSED(x) /*@unused@*/ x
#else
#define UNUSED(x) x
#endif

#define ARRAY_SIZE(x)           (sizeof(x)/sizeof(x[0]))

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define __LITTLE_ENDIAN_BITFIELD
#elif __BYTE_ORDER == __BIG_ENDIAN
#define __BIG_ENDIAN_BITFIELD
#else
#error	"Byte order error"
#endif

#define BSWAP_16(x)             ((((x) & 0xff00) >> 8) | (((x) & 0x00ff) << 8))
#define BSWAP_32(x)             ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >> 8) |\
                                 (((x) & 0x0000ff00) << 8) | (((x) & 0x000000ff) << 24))
#ifdef __LITTLE_ENDIAN_BITFIELD
#define B16_H2L(X)              (X)             /* 16 bit to little endian */
#define B32_H2L(X)              (X)             /* 32 bit to little endian */
#define B16_H2B(X)              BSWAP_16(X)     /* 16 bit to big endian */
#define B32_H2B(X)              BSWAP_32(X)     /* 32 bit to big endian */
#define B16_L2H(X)              (X)             /* 16 bit little to host */
#define B32_L2H(X)              (X)             /* 32 bit little to host */
#define B16_B2H(X)              BSWAP_16(X)     /* 16 bit big endian to host */
#define B32_B2H(X)              BSWAP_32(X)     /* 32 bit big endian to host */
#define B16_H2N(X)              BSWAP_16(X)     /* 16 bit host to network */
#define B32_H2N(X)              BSWAP_32(X)     /* 32 bit host to network */
#else
#define B16_H2L(X)              BSWAP_16(X)     /* 16 bit to little endian */
#define B32_H2L(X)              BSWAP_32(X)     /* 32 bit to little endian */
#define B16_H2B(X)              (X)             /* 16 bit to big endian */
#define B32_H2B(X)              (X)             /* 32 bit to big endian */
#define B16_L2H(X)              BSWAP_16(X)     /* 16 bit little to host */
#define B32_L2H(X)              BSWAP_32(X)     /* 32 bit little to host */
#define B16_B2H(X)              (X)             /* 16 bit big endian to host */
#define B32_B2H(X)              (X)             /* 32 bit big endian to host */
#define B16_L2N(X)              (X)             /* 16 bit host to network */
#define B32_L2N(X)              (X)             /* 32 bit host to network */
#endif

#define BIT_CLR(x,b)            (x = ((x) & (~b)))
#define BIT_SET(x,b)            (x = ((x) | (b)))
#define BIT_MASK_SET(x,m,b)     (x = ((x) & (~m) | ((m) & (b))))
#define BIT_TST(x,b)            ((x) | (b))

#define SRV_CTL_UDP_PORT        60003
#define CLI_CTL_UDP_PORT        61003

#define MSG_REQ_SET_CFG         0x00000001
#define MSG_REQ_GET_CFG         0x00000002
#define MSG_REQ_READ_CFG        0x00000004
#define MSG_REQ_SAVE_CFG        0x00000008
#define MSG_REQ_SET_LOG         0x00000010
#define MSG_REQ_SET_SE433       0x00000020
#define MSG_REQ_GET_SE433L      0x00000040
#define MSG_RSP_END             0x10000000
#define MSG_RSP_GET_CFG         0x10000002
#define MSG_RSP_GET_SE433L      0x10000040

#define MSG_RET_OK              0x00
#define MSG_RET_ERR             0x01
#define MSG_RET_CMD_NOSUPPORT   0x02

#define RF433_MASK_NET_ID       (1<<0)
#define RF433_MASK_RCV_ADDR     (1<<1)

#define SOCK_MASK_SER_IP        (1<<0)
#define SOCK_MASK_UDP_PORT      (1<<1)

#define SE433_OP_ADD            1
#define SE433_OP_DEL            2

#define MSG_RESP_LEN            256
#define SERVER_PORT_MIN         1024
#define SERVER_PORT_MAX         65500
#define RF433_NETID_BYTE_0      0xaa    /* 0x98 */
#define RF433_NETID_MIN         0x01
#define RF433_NETID_MAX         0xf0
#define RF433_NETID(id)         (((uint16_t)(id&0xff)<<8)|RF433_NETID_BYTE_0)
#define RF433_RCVADDR_MIN       0x00000000
#define RF433_RCVADDR_MAX       0x00ffffff
#define RF433_BROADCAST         0xffffffff
#define SE433_ADDR_MIN          0x01000000
#define SE433_ADDR_MAX          0xfffffffe


#define NAMESIZE                32
struct collate_st {
    char str[NAMESIZE];
    int val;
};


#define RF433_THREAD_MAX        1
#define RF433_LOG_F             "rf433"
#define RF433_SE433_MAX         32

#define UDP_BUF_SIZE            256
#define RF433_BUF_SIZE          256
#define RELAY_POLL_SEC          5

#define RF433_CFG_SRV_IP        "192.168.1.200"
#define RF433_CFG_UDP_PORT      13001
#define RF433_CFG_LOCAL_ADDR    0x00abcdef
#define RF433_CFG_NET_ID        RF433_NETID(RF433_NETID_MIN)
#define RF433_CFG_RATE          A7139_RATE_10K
#define RF433_NET_ID(id)        ((uint8_t)((id >> 8) & 0x00ff))
#define RF433_WFREQ(id)         ((uint8_t)((id >> 8) & 0x0f))

#define RF433_NVR_SRV_IP        "rf433_server_ip"
#define RF433_NVR_UDP_PORT      "rf433_server_port"
#define RF433_NVR_NET_ID        "rf433_net_id"
#define RF433_NVR_LOCAL_ADDR    "rf433_local_addr"

#define RF433_LOG_SOCKCLI       "sockcli rf433 -L"

#define RF433_SHOW_SE_ADDR      "rf433_se_addr"
#define RF433_SHOW_SE_DATA      "rf433_se_data"

#define RF433_RF_DEV_NAME       "/dev/a7139-1"


#pragma pack (1)

typedef struct {
    uint32_t m_mask;
    uint16_t local_port;
    struct in_addr server_ip;
    uint16_t server_port;
} socket_cfg;

typedef struct {
    uint32_t m_mask;
    uint32_t local_addr;
    uint16_t net_id;
    uint8_t rate;
    uint8_t freq;
} rf433_cfg;

typedef struct {
    uint8_t op;
    uint32_t se433_addr;
} se433_cfg;

typedef struct {
    uint32_t se433_addr;
    uint64_t req_cnt;           /* rf433 to udp */
    uint64_t rsp_cnt;           /* udp to rf433 */
} se433_cnt;


#define MSG_CTL_MAGIC_0         0x98
#define MSG_CTL_MAGIC_1         0x43
#define MSG_CTL_VERSION         0x01

struct msg_head {
    uint8_t magic[2];
    uint8_t flags;
    uint8_t version;
};
struct msg_config {
    socket_cfg socket;
    rf433_cfg rf433;
};
struct msg_data {
    unsigned long type;
    uint8_t content[0];
};
struct msg_st {
    struct msg_head h;
    struct msg_data d;
};

#pragma pack ()


int get_foreground_mode(void);

#ifdef USE_APPLOG
#define app_log_open(logname)            applog_open(logname, LOG_ERR, RF433_LOG_SOCKCLI)
#define app_log_printf(level, fmt, ...)     do {    \
                                            if (get_foreground_mode()) { \
                                                sys_printf(level, fmt, ## __VA_ARGS__);     \
                                            } else {                \
                                                applog_print(level, fmt, ## __VA_ARGS__);  \
                                            }   \
                                        } while(0)
#define app_log_close()                 applog_close()
#define app_log_level_set(l)            applog_level_set(l)
#define app_log_level_get()             applog_level_get()
#else
#define app_log_open(logname)           sys_openlog(logname)
#define app_log_printf(level, fmt, ...) sys_printf(level, fmt, ## __VA_ARGS__)
#define app_log_close()                 closelog()
#define app_log_level_set(l)            set_loglevel(l)
#define app_log_level_get()             get_loglevel()
#endif
#define TRACE(fmt, args...)             app_log_printf(LOG_DEBUG, fmt, ##args)

void sys_openlog(char *logname);
void set_loglevel(int n);
int get_loglevel(void);
void sys_printf(int level, const char *format, ...);
int getvalue(char *opt, int *value, int base);
int get_ip(char* ip, struct in_addr *ip_addr);
int get_port(char *str, uint16_t *port);
int get_netid(char *str, uint16_t *netid);
int get_local_addr(char *str, uint32_t *addr);
int get_se433_addr(char *str, uint32_t *addr);
//int get_se433_list(char *str, se433_head *head);
int get_log_level(char *str, int *log_level);

ssize_t safe_write(int fd, const void *buf, size_t count);
ssize_t full_write(int fd, const void *buf, size_t len);

uint8_t crc8(char *data, uint8_t len);

int msg_check(buffer *msg_buf);

#endif  //__COMMON_H__

