//*******************************************************************************
// Copyright (C), RHTECH. Co., Ltd.
// Author:          redfox.qu@qq.com
// Date:            2013/12/25
// Version:         1.0.0
// Description:     The 433Mhz module library file
//
// History:
//      <author>            <time>      <version>   <desc>
//      redfox.qu@qq.com    2013/12/25  1.0.0       create this file
//      redfox.qu@qq.com    2014/01/02  1.0.1       add the function common
//
// TODO:
//      2014/01/02          add the common and debug
//*******************************************************************************

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "common.h"
#include "rf433lib.h"

char *a7139_wfreq_str[] = {
    "A7139_FREQ_470M",
    "A7139_FREQ_472M",
    "A7139_FREQ_474M",
    "A7139_FREQ_476M",
    "A7139_FREQ_478M",
    "A7139_FREQ_480M",
    "A7139_FREQ_482M",
    "A7139_FREQ_484M",
    "A7139_FREQ_486M",
    "A7139_FREQ_488M",
    "A7139_FREQ_490M",
    "A7139_FREQ_492M",
    "A7139_FREQ_494M",
    "A7139_FREQ_496M",
    "A7139_FREQ_498M",
    "A7139_FREQ_500M",
};

char *a7139_rate_str[] = {
    "A7139_RATE_2K",
    "A7139_RATE_5K",
    "A7139_RATE_10K",
    "A7139_RATE_25K",
    "A7139_RATE_50K",
};

char *se433_type_str[] = {
    "NON",
    "CH4",
    "CO",
    "TEMP",
};

char *se433_state_str[] = {
    "SE433_STATE_NON",
    "SE433_STATE_RESET",
    "SE433_STATE_REGISTER",
    "SE433_STATE_REG_REQ",
    "SE433_STATE_REG_RSP",
    "SE433_STATE_POLL",
    "SE433_STATE_POLL_REQ",
    "SE433_STATE_POLL_RSP",
};


char *rf433_get_freq_str(A7139_FREQ wfreq)
{
    if (wfreq >= A7139_FREQ_MAX) {
        return NULL;
    }

    return a7139_wfreq_str[wfreq];
}

char *rf433_get_rate_str(A7139_RATE rate)
{
    if (rate >= A7139_RATE_MAX) {
        return NULL;
    }

    return a7139_rate_str[rate];
}

char *se433_get_type_str(uint8_t type)
{
    if (type >= ARRAY_SIZE(se433_type_str)) {
        return NULL;
    }

    return se433_type_str[type];
}

char *se433_get_state_str(enum SE433_STATE state)
{
    if (state >= ARRAY_SIZE(se433_state_str)) {
        return NULL;
    }

    return se433_state_str[state];
}

char *se433_get_vol_str(uint8_t vol)
{
    static char vol_str[16];

    snprintf(vol_str, 16, "%d.%dV", (vol&0xf0)>>4, (vol&0x0f));

    return vol_str;
}

char *se433_get_batt_str(uint8_t batt)
{
    static char batt_str[16];

    snprintf(batt_str, 16, "%d%%", (batt > 100) ? 100 : batt);

    return batt_str;
}

char *se433_get_flags_str(uint8_t flags)
{
    static char flags_str[1024];
    int p = 0;

    p += snprintf(flags_str + p, 1024 - p, "%s", flags & RSWP433_FLAG_MPOW ? "PP" : "BP");
    if (flags & RSWP433_FLAG_PROBE) {
        p += snprintf(flags_str + p, 1024 - p, ",E_PROB");
    }
    if (flags & RSWP433_FLAG_BATTERY) {
        p += snprintf(flags_str + p, 1024 - p, ",E_BATT");
    }

    return flags_str;
}

/*****************************************************************************
* Function Name  : open_rf433
* Description    : open the rf433 device
* Input          : char*
* Output         : None
* Return         : int(fd)
*****************************************************************************/
int open_rf433(char *dev)
{
    int fd;

    fd = open(dev, O_RDWR);
    if (fd == -1) {
        app_log_printf(LOG_ERR, "open_rf433(%s) Error: %s\n", dev, strerror(errno));
        return -1;
    }

    return fd;
}

/*****************************************************************************
* Function Name  : open_socket
* Description    : open and config the udp socket
* Input          : None
* Output         : None
* Return         : int(fd)
*****************************************************************************/
int open_socket(void)
{
    int fd;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1) {
        app_log_printf(LOG_ERR, "socket() error: %s", strerror(errno));
        return -1;
    }

    return fd;
}


/*****************************************************************************
* Function Name  : set_non_blk
* Description    : set the fd to nonblock config
* Input          : int
* Output         : None
* Return         : void
*****************************************************************************/
void set_non_blk(int fd)
{
	if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
		if (errno == ENODEV) {
			/* Some devices (like /dev/null redirected in)
			 * can't be set to non-blocking */
			app_log_printf(LOG_DEBUG, "ignoring ENODEV for setnonblocking");
		} else {
			app_log_printf(LOG_DEBUG, "Couldn't set nonblocking");
		}
	}
}

/*****************************************************************************
* Function Name  : set_socket_opt
* Description    : set the socket config
* Input          : int, uint16_t
* Output         : None
* Return         : int
*****************************************************************************/
int set_socket_opt(int fd, uint16_t lport)
{
    struct sockaddr_in srvaddr;
    int val = 1;
    int ret = 0;

    //set_non_blk(fd);

    ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    if (lport != 0) {
        bzero(&srvaddr, sizeof(srvaddr));
        srvaddr.sin_family = AF_INET;
        srvaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        srvaddr.sin_port = htons(lport);

        ret = bind(fd, (struct sockaddr*)&srvaddr, sizeof(srvaddr));
        if (ret == -1) {
            app_log_printf(LOG_ERR, "open_socket() error: %s", strerror(errno));
            return -1;
        }
        TRACE("%-20s: %d", "sockopt.local_port", lport);
    }
    else {
        TRACE("no listen port set");
    }

    return ret;
}

int set_rf433_opt(int fd, uint16_t netid, uint8_t rate)
{
    A7139_FREQ wfreq;

    wfreq = RF433_WFREQ(netid);

    rf433_set_netid(fd, netid);
    rf433_set_wfreq(fd, wfreq);
    rf433_set_rate(fd, rate);

    TRACE("%-20s: 0x%x", "rf433opt.netid", netid);
    TRACE("%-20s: 0x%x(%s)", "rf433opt.wfreq", wfreq, rf433_get_freq_str(wfreq));
    TRACE("%-20s: 0x%x(%s)", "rf433opt.rate", rate, rf433_get_rate_str(rate));

    return 0;
}

/*****************************************************************************
* Function Name  : se433_find
* Description    : find the se433 elementent
* Input          : se433_head*, uint32_t
* Output         : None
* Return         : se433_list*
*****************************************************************************/
se433_list *se433_find(se433_head *head, uint32_t addr)
{
    se433_list *se433l;

    list_for_each_entry(se433l, &head->list, list) {
        if (se433l->se433.addr == addr) {
            return se433l;
        }
    }

    return NULL;
}

/*****************************************************************************
* Function Name  : se433_find_earliest
* Description    : find the earliest se433 elementent
* Input          : se433_head*
* Output         : None
* Return         : se433_list*
*****************************************************************************/
se433_list *se433_find_earliest(se433_head *head)
{
    se433_list *se433l, *se433_e;
    time_t last_req_tm;

    last_req_tm = time(NULL);
    se433_e = NULL;

    list_for_each_entry(se433l, &head->list, list) {
        if (se433l->se433.last_req_tm < last_req_tm) {
            se433_e = se433l;
            last_req_tm = se433l->se433.last_req_tm;
        }
    }

    return se433_e;
}

/*****************************************************************************
* Function Name  : se433_find_offline
* Description    : find the offline se433 elementent
* Input          : se433_head*
* Output         : None
* Return         : se433_list*
*****************************************************************************/
se433_list *se433_find_offline(se433_head *head)
{
    se433_list *se433l;

    list_for_each_entry(se433l, &head->list, list) {
        if ((se433l->se433.req_cnt - se433l->se433.rsp_cnt) >= RSWP433_OFFLINE_CNT) {
            return se433l;
        }
    }

    return NULL;
}

/*****************************************************************************
* Function Name  : se433_add
* Description    : add se433 elementent to the list
* Input          : se433_head*, uint32_t
* Output         : None
* Return         : se433_list*
*****************************************************************************/
se433_list *se433_add(se433_head *head, uint32_t addr)
{
    se433_list *se433l;


    if (head->num >= RF433_SE433_MAX) {
        app_log_printf(LOG_ERR, "se433 list full");
        return NULL;
    }

    /* check se433 address validity */
    if (addr < SE433_ADDR_MIN || addr > SE433_ADDR_MAX) {
        app_log_printf(LOG_ERR, "se433 address 0x%08x invalid", addr);
        return NULL;
    }

    if ((se433l = se433_find(head, addr)) != NULL) {
        app_log_printf(LOG_WARNING, "se433 0x%08x duplicate", addr);
        return se433l;
    }

    se433l = (se433_list*)malloc(sizeof(se433_list));
    if (se433l == NULL) {
        app_log_printf(LOG_ERR, "new se433 list memory error");
        return NULL;
    }
    memset((char*)se433l, 0, sizeof(se433_list));

    INIT_LIST_HEAD(&se433l->list);
    se433l->state = SE433_STATE_REG_REQ;
    se433l->se433.addr = addr;

    list_add(&se433l->list, &head->list);
    head->num++;

    app_log_printf(LOG_DEBUG, "se433_add 0x%08x\n", se433l->se433.addr);

    return se433l;
}

/*****************************************************************************
* Function Name  : se433_del
* Description    : del se433 elementent from the list
* Input          : se433_head*, uint32_t
* Output         : None
* Return         : int(0:ok, -1:error)
*****************************************************************************/
int se433_del(se433_head *head, uint32_t addr)
{
    se433_list *se433l, *se433n;

    list_for_each_entry_safe(se433l, se433n, &head->list, list) {
        if (se433l->se433.addr == addr) {
            list_del(&se433l->list);
            head->num--;

            app_log_printf(LOG_DEBUG, "se433_del 0x%08x\n", se433l->se433.addr);

            free(se433l);
            return 0;
        }
    }

    app_log_printf(LOG_WARNING, "cannot find se433 %d to del\n", addr);

    return -1;
}

int se433_clean(se433_head *head)
{
    se433_list *se433l, *se433n;

    list_for_each_entry_safe(se433l, se433n, &head->list, list) {
        list_del(&se433l->list);
        head->num--;
        app_log_printf(LOG_DEBUG, "se433_del 0x%08x\n", se433l->se433.addr);
        free(se433l);
    }

    return 0;
}

int se433_data_add(se433_list *se433l, se433_data *data)
{
    memcpy(&se433l->se433.data, data, sizeof(rswp433_data));
#if 0
    /**
     * TODO: qjn@2014.1.14
     *  create a data list, save about least 2000 datas
     */
#endif
    return 0;
}

void se433_list_show(se433_head *head)
{
    se433_list *se433l;

    list_for_each_entry(se433l, &head->list, list) {
        app_log_printf(LOG_DEBUG, "addr=0x%08x, data=%.3f, req_cnt=%d, rsp_cnt=%d\n",
                se433l->se433.addr,
                se433l->se433.data.data,
                se433l->se433.req_cnt,
                se433l->se433.rsp_cnt);
    }
}


/*****************************************************************************
* Function Name  : rf433_set_netid
* Description    : set the rf433 wireless ID code
* Input          : int, uint16_t
* Output         : None
* Return         : int(0:ok, -1:error)
*****************************************************************************/
int rf433_set_netid(int fd, uint16_t net_id)
{
    return ioctl(fd, A7139_IOC_SETID, &net_id);
}

/*****************************************************************************
* Function Name  : rf433_get_netid
* Description    : set the rf433 wireless ID code
* Input          : int, uint16_t
* Output         : None
* Return         : int(0:ok, -1:error)
*****************************************************************************/
int rf433_get_netid(int fd, uint16_t *net_id)
{
    return ioctl(fd, A7139_IOC_GETID, net_id);
}

/*****************************************************************************
* Function Name  : rf433_set_wfreq
* Description    : set the rf433 wireless freqence
* Input          : int, uint8_t
* Output         : None
* Return         : int(0:ok, -1:error)
*****************************************************************************/
int rf433_set_wfreq(int fd, uint8_t wfreq)
{
    return ioctl(fd, A7139_IOC_SETFREQ, &wfreq);
}

/*****************************************************************************
* Function Name  : rf433_set_wfreq
* Description    : set the rf433 wireless freqence
* Input          : int, uint8_t
* Output         : None
* Return         : int(0:ok, -1:error)
*****************************************************************************/
int rf433_get_wfreq(int fd, uint8_t *wfreq)
{
    return ioctl(fd, A7139_IOC_GETFREQ, wfreq);
}

/*****************************************************************************
* Function Name  : rf433_set_rate
* Description    : set the rf433 wireless speed
* Input          : int, uint8_t
* Output         : None
* Return         : int(0:ok, -1:error)
*****************************************************************************/
int rf433_set_rate(int fd, uint8_t rate)
{
    return ioctl(fd, A7139_IOC_SETRATE, &rate);
}

/*****************************************************************************
* Function Name  : rf433_get_rate
* Description    : set the rf433 wireless speed
* Input          : int, uint8_t
* Output         : None
* Return         : int(0:ok, -1:error)
*****************************************************************************/
int rf433_get_rate(int fd, uint8_t *rate)
{
    return ioctl(fd, A7139_IOC_GETRATE, rate);
}

/*****************************************************************************
* Function Name  : rswp433_pkg_new
* Description    : new and return a rswp433 packet
* Input          : void
* Output         : None
* Return         : rswp433_pkg*
*****************************************************************************/
rswp433_pkg *rswp433_pkg_new(void)
{
    rswp433_pkg *pkg;

    pkg = (rswp433_pkg*)malloc(sizeof(rswp433_pkg));
    if (pkg == NULL) {
        return NULL;
    }

    memset((char*)pkg, 0, sizeof(rswp433_pkg));

    return pkg;
}

/*****************************************************************************
* Function Name  : rswp433_pkg_del
* Description    : del and free a rswp433 packet
* Input          : rswp433_pkg*
* Output         : None
* Return         : void
*****************************************************************************/
void rswp433_pkg_del(rswp433_pkg *pkg)
{
    memset((char*)pkg, 0, sizeof(rswp433_pkg));
    free(pkg);
}

/*****************************************************************************
* Function Name  : rswp433_pkg_clr
* Description    : clear the rswp433 packet memory
* Input          : rswp433_pkg*
* Output         : None
* Return         : void
*****************************************************************************/
void rswp433_pkg_clr(rswp433_pkg *pkg)
{
    memset((char*)pkg, 0, sizeof(rswp433_pkg));
}

/*****************************************************************************
* Function Name  : rswp433_reg_req
* Description    : rswp433 protocol, a se433 register request
* Input          : rswp433_pkg*, rf433_instence*
* Output         : None
* Return         : se433_list*
*****************************************************************************/
se433_list *rswp433_reg_req(rswp433_pkg *pkg, rf433_instence *rf433x)
{
    return se433_add(&rf433x->se433, pkg->u.content.src_addr);
}

/*****************************************************************************
* Function Name  : rswp433_reg_rsp
* Description    : rswp433 protocol, a se433 register response
* Input          : se433_list*, rf433_instence*, buffer*
* Output         : None
* Return         : int
*****************************************************************************/
int rswp433_reg_rsp(se433_list *se433l, rf433_instence *rf433x, buffer *buf)
{
    rswp433_pkg *pkg;

    buf_clean(buf);

    pkg = (rswp433_pkg*)buf_data(buf);
    pkg->header.sp[0] = RSWP433_PKG_SP_0;
    pkg->header.sp[1] = RSWP433_PKG_SP_1;
    pkg->header.len = sizeof(rswp433_pkg_content);
    pkg->header.crc1 = crc8((char*)&pkg->header, sizeof(rswp433_pkg_header) - 1);

    pkg->u.content.dest_addr = se433l->se433.addr;
    pkg->u.content.src_addr = rf433x->rf433.local_addr;
    pkg->u.content.cmd = RSWP433_CMD_REG_RSP;
    pkg->u.content.crc2 = crc8((char*)&pkg->u.content, sizeof(rswp433_pkg_content) - 1);

    buf_incrlen(buf, sizeof(rswp433_pkg));

    se433l->se433.last_req_tm = time(NULL);
    se433l->state = SE433_STATE_REG_RSP;

    return 0;
}

/*****************************************************************************
* Function Name  : rswp433_data_req
* Description    : rswp433 protocol, a se433 data-poll request
* Input          : se433_list*, rf433_instence*, buffer*
* Output         : None
* Return         : int
*****************************************************************************/
int rswp433_data_req(se433_list *se433l, rf433_instence *rf433x, buffer *buf)
{
    rswp433_pkg *pkg;

    buf_clean(buf);

    pkg = (rswp433_pkg*)buf_data(buf);
    pkg->header.sp[0] = RSWP433_PKG_SP_0;
    pkg->header.sp[1] = RSWP433_PKG_SP_1;
    pkg->header.len = sizeof(rswp433_pkg_content);
    pkg->header.crc1 = crc8((char*)&pkg->header, sizeof(rswp433_pkg_header) - 1);

    pkg->u.content.dest_addr = se433l->se433.addr;
    pkg->u.content.src_addr = rf433x->rf433.local_addr;
    pkg->u.content.cmd = RSWP433_CMD_DATA_REQ;
    pkg->u.content.crc2 = crc8((char*)&pkg->u.content, sizeof(rswp433_pkg_content) - 1);

    buf_incrlen(buf, sizeof(rswp433_pkg));

    se433l->se433.req_cnt++;
    se433l->se433.last_req_tm = time(NULL);
    se433l->state = SE433_STATE_POLL_REQ;

    return 0;
}

/*****************************************************************************
* Function Name  : rswp433_data_rsp
* Description    : rswp433 protocol, a se433 data-poll response
* Input          : rswp433_pkg*, rf433_instence*, buffer*
* Output         : None
* Return         : int
*****************************************************************************/
int rswp433_data_rsp(rswp433_pkg *pkg, rf433_instence *rf433x, buffer *buf)
{
    se433_list *se433l;
    rswp433_pkg net_pkg;
    uint32_t data;

    /* update new data to se433 */
    se433l = se433_find(&rf433x->se433, pkg->u.data_content.src_addr);
    if (se433l == NULL) {
        app_log_printf(LOG_WARNING, "cannot find se433 0x%08x to add data\n", pkg->u.data_content.src_addr);
        return -1;
    }

    if (se433l->state != SE433_STATE_POLL_REQ) {
        app_log_printf(LOG_WARNING, "se433 0x%08x state error got(%d) want(%d)\n",
                se433l->se433.addr, se433l->state, SE433_STATE_POLL_REQ);

        /* offline the error state se433 */
        app_log_printf(LOG_INFO, "se433 0x%08x offline, req_cnt=%d, rsp_cnt=%d",
                se433l->se433.addr, se433l->se433.req_cnt, se433l->se433.rsp_cnt);
        se433_del(&rf433x->se433, pkg->u.data_content.src_addr);

        return -1;
    }

    se433l->state = SE433_STATE_POLL_RSP;
    se433l->se433.rsp_cnt++;

    memcpy(&se433l->se433.data, &pkg->u.data_content.data, sizeof(rswp433_data));
    //se433_data_add(se433l, &pkg->u.data_content.data);

    app_log_printf(LOG_DEBUG, "poll response dest_addr=0x%08x, src_addr=0x%08x, data=%.2f\n",
            pkg->u.content.dest_addr,
            pkg->u.content.src_addr,
            pkg->u.data_content.data.data);

    /* translate to network endian */
    memcpy(&net_pkg, pkg, sizeof(rswp433_pkg));
    net_pkg.u.data_content.dest_addr = B32_H2N(net_pkg.u.data_content.dest_addr);
    net_pkg.u.data_content.src_addr = B32_H2N(net_pkg.u.data_content.src_addr);
    memcpy(&data, &net_pkg.u.data_content.data.data, sizeof(uint32_t));
    data = B32_H2N(data);
    memcpy(&net_pkg.u.data_content.data.data, &data, sizeof(uint32_t));

    /* copy data to udp buffer */
    buf_clean(buf);
    buf_append(buf, (char*)&net_pkg.u.data_content, sizeof(rswp433_pkg_data_content));

    return 0;
}

/*****************************************************************************
* Function Name  : rswp433_pkg_analysis
* Description    : rswp433 protocol, a package analysis
* Input          : buffer*, rswp433_pkg*
* Output         : None
* Return         : int
*****************************************************************************/
int rswp433_pkg_analysis(buffer *buf, rswp433_pkg *pkg)
{
    int i, len;
    char *data;
    int ret;

    data = buf->data + buf->pos;
    len = buf->len - buf->pos;

    for (i = 0; i <(len - 1); i++) {

        /* check the soft preamble */
        if (data[i] == RSWP433_PKG_SP_0 && data[i+1] == RSWP433_PKG_SP_1) {

            /* check the header crc1 */
            if ((ret = crc8(&data[i], sizeof(rswp433_pkg_header))) == 0) {

                /* correct */
                memcpy((char*)pkg, &data[i], sizeof(rswp433_pkg));

                if ((ret = crc8((char*)&pkg->u.content, pkg->header.len)) == 0) {

                    /* got a rswp433 valid package */
                    return 1;

                } else {
                    /* crc2 check error */
                    app_log_printf(LOG_DEBUG, "check rcr2 error 0x%02x\n", ret);

                    /* rswp433 package content crc error, discard all package */
                    return 0;
                }

            } else {
                /* crc1 check error */
                app_log_printf(LOG_DEBUG, "check crc1 error 0x%02x\n", ret);

                /* skip 2 byte */
                i = i + 1;
                continue;
            }

        } else {
            /* check next byte */
            continue;
        }
    }

    /* all bytes data check over, no rswp433 package found */
    return 0;
}


