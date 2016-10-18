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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include <syslog.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "common.h"
#include "drivers/char/a7139.h"

int loglevel = LOG_DEBUG;
int foreground_mode = 0;
int exitflag = 0;

struct collate_st avail_rate_col[] = {
    { "2k",     A7139_RATE_2K   },
    { "5k",     A7139_RATE_5K   },
    { "10k",    A7139_RATE_10K  },
    { "25k",    A7139_RATE_25K  },
    { "50k",    A7139_RATE_50K  },
};

struct collate_st avail_wfreq_col[] = {
    { "470M",   A7139_FREQ_470M },
    { "472M",   A7139_FREQ_472M },
    { "474M",   A7139_FREQ_474M },
    { "476M",   A7139_FREQ_476M },
    { "478M",   A7139_FREQ_478M },
    { "480M",   A7139_FREQ_480M },
    { "482M",   A7139_FREQ_482M },
    { "484M",   A7139_FREQ_484M },
    { "486M",   A7139_FREQ_486M },
    { "488M",   A7139_FREQ_488M },
    { "490M",   A7139_FREQ_490M },
    { "492M",   A7139_FREQ_492M },
    { "494M",   A7139_FREQ_494M },
    { "496M",   A7139_FREQ_496M },
    { "498M",   A7139_FREQ_498M },
    { "500M",   A7139_FREQ_500M },
};

unsigned char CRC8_TAB[256] = {
    0x00, 0x5e, 0xbc, 0xe2, 0x61, 0x3f, 0xdd, 0x83,
    0xc2, 0x9c, 0x7e, 0x20, 0xa3, 0xfd, 0x1f, 0x41,
    0x9d, 0xc3, 0x21, 0x7f, 0xfc, 0xa2, 0x40, 0x1e,
    0x5f, 0x01, 0xe3, 0xbd, 0x3e, 0x60, 0x82, 0xdc,
    0x23, 0x7d, 0x9f, 0xc1, 0x42, 0x1c, 0xfe, 0xa0,
    0xe1, 0xbf, 0x5d, 0x03, 0x80, 0xde, 0x3c, 0x62,
    0xbe, 0xe0, 0x02, 0x5c, 0xdf, 0x81, 0x63, 0x3d,
    0x7c, 0x22, 0xc0, 0x9e, 0x1d, 0x43, 0xa1, 0xff,
    0x46, 0x18, 0xfa, 0xa4, 0x27, 0x79, 0x9b, 0xc5,
    0x84, 0xda, 0x38, 0x66, 0xe5, 0xbb, 0x59, 0x07,
    0xdb, 0x85, 0x67, 0x39, 0xba, 0xe4, 0x06, 0x58,
    0x19, 0x47, 0xa5, 0xfb, 0x78, 0x26, 0xc4, 0x9a,
    0x65, 0x3b, 0xd9, 0x87, 0x04, 0x5a, 0xb8, 0xe6,
    0xa7, 0xf9, 0x1b, 0x45, 0xc6, 0x98, 0x7a, 0x24,
    0xf8, 0xa6, 0x44, 0x1a, 0x99, 0xc7, 0x25, 0x7b,
    0x3a, 0x64, 0x86, 0xd8, 0x5b, 0x05, 0xe7, 0xb9,
    0x8c, 0xd2, 0x30, 0x6e, 0xed, 0xb3, 0x51, 0x0f,
    0x4e, 0x10, 0xf2, 0xac, 0x2f, 0x71, 0x93, 0xcd,
    0x11, 0x4f, 0xad, 0xf3, 0x70, 0x2e, 0xcc, 0x92,
    0xd3, 0x8d, 0x6f, 0x31, 0xb2, 0xec, 0x0e, 0x50,
    0xaf, 0xf1, 0x13, 0x4d, 0xce, 0x90, 0x72, 0x2c,
    0x6d, 0x33, 0xd1, 0x8f, 0x0c, 0x52, 0xb0, 0xee,
    0x32, 0x6c, 0x8e, 0xd0, 0x53, 0x0d, 0xef, 0xb1,
    0xf0, 0xae, 0x4c, 0x12, 0x91, 0xcf, 0x2d, 0x73,
    0xca, 0x94, 0x76, 0x28, 0xab, 0xf5, 0x17, 0x49,
    0x08, 0x56, 0xb4, 0xea, 0x69, 0x37, 0xd5, 0x8b,
    0x57, 0x09, 0xeb, 0xb5, 0x36, 0x68, 0x8a, 0xd4,
    0x95, 0xcb, 0x29, 0x77, 0xf4, 0xaa, 0x48, 0x16,
    0xe9, 0xb7, 0x55, 0x0b, 0x88, 0xd6, 0x34, 0x6a,
    0x2b, 0x75, 0x97, 0xc9, 0x4a, 0x14, 0xf6, 0xa8,
    0x74, 0x2a, 0xc8, 0x96, 0x15, 0x4b, 0xa9, 0xf7,
    0xb6, 0xe8, 0x0a, 0x54, 0xd7, 0x89, 0x6b, 0x35
};

int get_foreground_mode(void)
{
    return foreground_mode;
}

/*****************************************************************************
* Function Name  : set_loglevel
* Description    : set rfrepeater deamon syslog level
* Input          : int
* Output         : None
* Return         : void
*****************************************************************************/
void set_loglevel(int n)
{
    if (n > LOG_EMERG && n <= LOG_DEBUG)
    {
        loglevel = n;
        return;
    }

    return;
}

/*****************************************************************************
* Function Name  : get_loglevel
* Description    : get rfrepeater deamon syslog level
* Input          : int
* Output         : None
* Return         : void
*****************************************************************************/
int get_loglevel(void)
{
    return loglevel;
}

/*****************************************************************************
* Function Name  : sys_openlog
* Description    : open syslog
* Input          : char*, ...
* Output         : none
* Return         : void
*****************************************************************************/
void sys_openlog(char *logname)
{
    openlog(logname, LOG_PID, LOG_DAEMON);
}

/*****************************************************************************
* Function Name  : sys_printf
* Description    : print system message to syslog or display
* Input          : int, char*, ...
* Output         : display, syslog
* Return         : void
*****************************************************************************/
void sys_printf(int level, const char* format, ...)
{
    char tmpline[1024];
    va_list args;

    if (loglevel < level)
        return;

    memset(tmpline, 0, 1024);

    va_start(args, format);
    vsnprintf(tmpline, 1023, format, args);
    va_end(args);

    if (foreground_mode) {
        time_t t;
        struct tm tmt;
        t = time(NULL);
        localtime_r(&t, &tmt);
        fprintf(stderr, "[%04d/%02d/%02d %02d:%02d:%02d] %s\n",
                tmt.tm_year+1900,
                tmt.tm_mon,
                tmt.tm_mday,
                tmt.tm_hour,
                tmt.tm_min,
                tmt.tm_sec,
                tmpline);
    } else {
        syslog(level, tmpline);
    }
}

/*****************************************************************************
* Function Name  : getvalue
* Description    : get value from string
* Input          : char*, int*
* Output         : None
* Return         : int(0:ok, -1:error)
*****************************************************************************/
int getvalue(char *opt, int *value, int base)
{
    int ret = 0;

    if (opt) {
        switch (base) {
            case 8:
                ret = sscanf(opt, "%o", value);
                break;
            case 16:
                ret = sscanf(opt, "%x", value);
                break;
            case 10:
            default:
                ret = sscanf(opt, "%d", value);
                break;
        }
    }
    return ret;
}

/*****************************************************************************
* Function Name  : get_ip
* Description    : get ip from string
* Input          : char*, struct in_addr*
* Output         : None
* Return         : int(0:ok, -1:error)
*****************************************************************************/
int get_ip(char* ip, struct in_addr *ip_addr)
{
    int ulIpv4[4] = {0,0,0,0};
    int len;

    len = sscanf(ip, "%d.%d.%d.%d", &ulIpv4[3], &ulIpv4[2], &ulIpv4[1], &ulIpv4[0]);
    if (len != 4) {
        return -1;
    }
    if (ulIpv4[3] == 127) {
        return -1;
    }
    if (ulIpv4[3] >= 224) {
        return -1;
    }
    if (inet_aton(ip, ip_addr) == 0) {
        return -1;
    }

    return 0;
}

/*****************************************************************************
* Function Name  : get_port
* Description    : get port from string
* Input          : char*, uint16_t*
* Output         : None
* Return         : int(0:ok, -1:error)
*****************************************************************************/
int get_port(char *str, uint16_t *port)
{
    int val;

    if (getvalue(str, &val, 10)) {
        if (val >= SERVER_PORT_MIN && val < SERVER_PORT_MAX) {
            *port = (uint16_t)val;
            return 0;
        }
    }
    return -1;
}

/*****************************************************************************
* Function Name  : get_netid
* Description    : get netid from string
* Input          : char*, uint8_t*
* Output         : None
* Return         : int(0:ok, -1:error)
*****************************************************************************/
int get_netid(char *str, uint16_t *netid)
{
    int val;

    if (getvalue(str, &val, 10)) {
        if (val >= RF433_NETID_MIN && val < RF433_NETID_MAX) {
            *netid = RF433_NETID(val);
            return 0;
        }
    }
    return -1;
}

int get_rate(char *str, uint8_t *rate)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(avail_rate_col); i++) {
        if (strcmp(avail_rate_col[i].str, str) == 0) {
            *rate = (uint8_t)avail_rate_col[i].val;
            return 0;
        }
    }

    return -1;
}

/*****************************************************************************
* Function Name  : get_local_addr
* Description    : get rf433 local address from string
* Input          : char*, uint32_t*
* Output         : None
* Return         : int(0:ok, -1:error)
*****************************************************************************/
int get_local_addr(char *str, uint32_t *addr)
{
    int val;

    if (getvalue(str, &val, 16)) {
        if (val >= RF433_RCVADDR_MIN && val < RF433_RCVADDR_MAX) {
            *addr = (uint32_t)val;
            return 0;
        }
    }
    return -1;
}

/*****************************************************************************
* Function Name  : get_se433_addr
* Description    : get se433 address from string
* Input          : char*, uint32_t*
* Output         : None
* Return         : int(0:ok, -1:error)
*****************************************************************************/
int get_se433_addr(char *str, uint32_t *addr)
{
    int val;

    if (getvalue(str, &val, 16)) {
        if (val >= SE433_ADDR_MIN && val < SE433_ADDR_MAX) {
            *addr = (uint32_t)val;
            return 0;
        }
    }
    return -1;
}

/*
int get_se433_list(char *str, se433_head *head)
{
    se433_list *se433;
    char tmpstr[8];
    char *p, *q;

    for (q = str, p = strchr(q, ','); p != NULL; q = p + 1, p = strchr(q, ',')) {
        bzero(tmpstr, 8);
        strncpy(tmpstr, q, min(p-q, 8));
        tmpstr[7] = '\0';

        se433 = (se433_list*)malloc(sizeof(se433_list));
        if (se433 == NULL) {
            sys_printf(LOG_ERR, "get_se433_id() alloc memory error");
            continue;
        }

        INIT_LIST_HEAD(&se433->list);

        if (get_se433_addr(tmpstr, &se433->se_addr) == -1) {
            sys_printf(LOG_ERR, "get_se433_id():get_se433_addr():config(%s) error", tmpstr);
            free(se433);
        } else {
            list_add(&se433->list, &head->list);
            head->num++;
        }

    }

    return 0;
}
*/

/*****************************************************************************
* Function Name  : get_log_level
* Description    : get system log level from string
* Input          : char*, int*
* Output         : None
* Return         : int(0:ok, -1:error)
*****************************************************************************/
int get_log_level(char *str, int *log_level)
{
    int val;
    if (getvalue(str, &val, 10)) {
        if (val >= LOG_EMERG && val <= LOG_DEBUG) {
            *log_level = val;
            return 0;
        }
    }
    return -1;
}

/*****************************************************************************
* Function Name  : safe_write
* Description    : safe write to fd
* Input          : int, void*, size_t
* Output         : None
* Return         : ssize_t(write size)
*****************************************************************************/
ssize_t safe_write(int fd, const void *buf, size_t count)
{
    ssize_t n;

    do {
        n = write(fd, buf, count);
    } while (n < 0 && (errno == EINTR || errno == EAGAIN));

    return n;
}

/*****************************************************************************
* Function Name  : full_write
* Description    : full write to fd
* Input          : int, void*, size_t
* Output         : None
* Return         : ssize_t(write size)
*****************************************************************************/
ssize_t full_write(int fd, const void *buf, size_t len)
{
    ssize_t cc;
    ssize_t total;

    total = 0;

    while (len) {
        cc = safe_write(fd, buf, len);

        if (cc < 0) {
            if (total) {
                /* we already wrote some! */
                /* user can do another write to know the error code */
                return total;
            }
            return cc;  /* write() returns -1 on failure. */
        }

        total += cc;
        buf = ((const char *)buf) + cc;
        len -= cc;
    }

    return total;
}

/*****************************************************************************
* Function Name  : crc8
* Description    : Cyclic Redundancy Check 8 bit
* Input          : char*, int
* Output         : None
* Return         : uint8_t
*****************************************************************************/
uint8_t crc8(char *data, uint8_t len)
{
    uint8_t i, crc8_ret = 0;

    while (len--) {
        i = crc8_ret ^ (*data++);
        crc8_ret = CRC8_TAB[i];
    }
    return (crc8_ret);
}

int msg_check(buffer *msg_buf)
{
    struct msg_st *msg;

    if (buf_len(msg_buf) <= sizeof(struct msg_head)) {
        return -1;
    }

    msg = (struct msg_st *)buf_data(msg_buf);
    if (msg->h.magic[0] != MSG_CTL_MAGIC_0 || msg->h.magic[1] != MSG_CTL_MAGIC_1) {
        return -1;
    }

    if (msg->h.version != MSG_CTL_VERSION) {
        return -1;
    }

    return 0;
}


