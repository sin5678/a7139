//*******************************************************************************
// Copyright (C), RHTECH. Co., Ltd.
// Author:          redfox.qu@qq.com
// Date:            2013/12/25
// Version:         1.0.0
// Description:     The 433Mhz module wireless to ethernet transmit on am335x
//
// History:
//      <author>            <time>      <version>   <desc>
//      redfox.qu@qq.com    2013/12/25  1.0.0       create this file
//      redfox.qu@qq.com    2014/2/11   1.1.0       use udp-socket replace msg for IPC
//
// TODO:
//      2013/12/31          add the common and debug
//*******************************************************************************
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <pthread.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <syslog.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <signal.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include "common.h"
#include "rfrepeater.h"
#include "rf433lib.h"
#include "nvram.h"
#include "list.h"

#define USAGE   " \
rfrepeater - Sub 1G 433Mhz wireless to ethernet transmitter \n \
\n \
Version: 1.0.0 \n \
Usage: rfrepeater [-h] [-v] [-d] \n \
    -h                          Show this USAGE \n \
    -v                          Show version \n \
    -d                          Running in Debug mode \n \
"


extern int loglevel;
extern int foreground_mode;
extern int exitflag;

rf433_instence rf433i;
int spipefd[RF433_THREAD_MAX];
int cleanup_pop_arg = 0;

/*****************************************************************************
* Function Name  : default_init
* Description    : init rf433i instence default value
* Input          : void
* Output         : None
* Return         : int
*                  - 0:ok
*****************************************************************************/
int default_init(void)
{
    /* set default configure */
    rf433i.socket.local_port = RF433_CFG_UDP_PORT;
    rf433i.socket.server_ip.s_addr = inet_addr(RF433_CFG_SRV_IP);
    rf433i.socket.server_port = RF433_CFG_UDP_PORT;

    rf433i.rf433.net_id = RF433_CFG_NET_ID;
    rf433i.rf433.local_addr = RF433_CFG_LOCAL_ADDR;
    rf433i.rf433.rate = RF433_CFG_RATE;
    rf433i.rf433.freq = RF433_WFREQ(rf433i.rf433.net_id);

    INIT_LIST_HEAD(&rf433i.se433.list);
    rf433i.se433.num = 0;

    rf433i.tid = -1;
    rf433i.pipe_fd = -1;
    rf433i.sock_fd = -1;
    rf433i.rf433_fd = -1;

    return 0;
}

/*****************************************************************************
* Function Name  : load_config
* Description    : load config from nvram
* Input          : void
* Output         : None
* Return         : int
*                  - 0:ok
*****************************************************************************/
int load_config(void)
{
    int set = 0;
    char *val;
    char buff[32] = { 0 };

    /* get server ip address */
    if (((val = nvram_get(NVRAM_TYPE_NVRAM, RF433_NVR_SRV_IP)) != NULL) &&
        (get_ip(val, &rf433i.socket.server_ip) == 0)) {
        app_log_printf(LOG_DEBUG, "%-20s: %s", "server_ip", inet_ntoa(rf433i.socket.server_ip));
    } else {
        nvram_set(NVRAM_TYPE_NVRAM, RF433_NVR_SRV_IP, inet_ntoa(rf433i.socket.server_ip));
        app_log_printf(LOG_ERR, "failed to get %s's value and set to it's default value %s",
                RF433_NVR_SRV_IP, inet_ntoa(rf433i.socket.server_ip));
        set++;
    }

    /* get server udp port */
    if (((val = nvram_get(NVRAM_TYPE_NVRAM, RF433_NVR_UDP_PORT)) != NULL) &&
        (get_port(val, &rf433i.socket.server_port) == 0)) {
        app_log_printf(LOG_DEBUG, "%-20s: %d", "server_port", rf433i.socket.server_port);
    } else {
        snprintf(buff, 32, "%d", rf433i.socket.server_port);
        nvram_set(NVRAM_TYPE_NVRAM, RF433_NVR_UDP_PORT, buff);
        app_log_printf(LOG_ERR, "failed to get %s's value and set to it's default value %d",
                RF433_NVR_UDP_PORT, rf433i.socket.server_port);
        set++;
    }

    /* get rf433 wireless net id */
    if (((val = nvram_get(NVRAM_TYPE_NVRAM, RF433_NVR_NET_ID)) != NULL) &&
        (get_netid(val, &rf433i.rf433.net_id) == 0)) {
        app_log_printf(LOG_DEBUG, "%-20s: 0x%04x", "netid", rf433i.rf433.net_id);
    } else {
        snprintf(buff, 32, "%d", RF433_NET_ID(rf433i.rf433.net_id));
        nvram_set(NVRAM_TYPE_NVRAM, RF433_NVR_NET_ID, buff);
        app_log_printf(LOG_ERR, "failed to get %s's value and set to it's default value %d",
                RF433_NVR_NET_ID, RF433_NET_ID(rf433i.rf433.net_id));
        set++;
    }

    /* get rf433 wireless recveive address */
    if (((val = nvram_get(NVRAM_TYPE_NVRAM, RF433_NVR_LOCAL_ADDR)) != NULL) &&
        (get_local_addr(val, &rf433i.rf433.local_addr) == 0)) {
        app_log_printf(LOG_DEBUG, "%-20s: 0x%08x", "local_addr", rf433i.rf433.local_addr);
    } else {
        /* fixup bug#86, save local_addr by 4byte align hex with filled 0*/
        snprintf(buff, 32, "0x%08x", rf433i.rf433.local_addr);
        nvram_set(NVRAM_TYPE_NVRAM, RF433_NVR_LOCAL_ADDR, buff);
        app_log_printf(LOG_ERR, "failed to get %s's value and set to it's default value 0x%08x",
                RF433_NVR_LOCAL_ADDR, rf433i.rf433.local_addr);
        set++;
    }

    if (set) {
        nvram_commit(NVRAM_TYPE_NVRAM);
    }

    app_log_printf(LOG_INFO, "parameters are as follows: "
            "%s=%s, %s=%d, %s=%d, %s=0x%08x",
            RF433_NVR_SRV_IP, inet_ntoa(rf433i.socket.server_ip),
            RF433_NVR_UDP_PORT, rf433i.socket.server_port,
            RF433_NVR_NET_ID, RF433_NET_ID(rf433i.rf433.net_id),
            RF433_NVR_LOCAL_ADDR, rf433i.rf433.local_addr);


    app_log_printf(LOG_INFO, "rfrepeater initialized successfully.");

    return 0;
}

/*****************************************************************************
* Function Name  : save_config
* Description    : save config to nvram
* Input          : void
* Output         : None
* Return         : int
*                  - 0:ok
*****************************************************************************/
int save_config(void)
{
    char buff[32] = { 0 };

    nvram_set(NVRAM_TYPE_NVRAM, RF433_NVR_SRV_IP, inet_ntoa(rf433i.socket.server_ip));

    snprintf(buff, 32, "%d", rf433i.socket.server_port);
    nvram_set(NVRAM_TYPE_NVRAM, RF433_NVR_UDP_PORT, buff);

    snprintf(buff, 32, "%d", RF433_NET_ID(rf433i.rf433.net_id));
    nvram_set(NVRAM_TYPE_NVRAM, RF433_NVR_NET_ID, buff);

    /* fixup bug#86, save local_addr 4byte align hex filled by 0*/
    snprintf(buff, 32, "0x%08x", rf433i.rf433.local_addr);
    nvram_set(NVRAM_TYPE_NVRAM, RF433_NVR_LOCAL_ADDR, buff);

    nvram_commit(NVRAM_TYPE_NVRAM);

    return 0;
}

/*****************************************************************************
* Function Name  : relay_rf433
* Description    : rswp433 protocol implement and udp data send
* Input          : rf433_instence
* Output         : None
* Return         : int
*                  - 0:ok
*****************************************************************************/
int relay_rf433(rf433_instence *rf433x)
{
    fd_set rset, wset;
    int max_fd, ret;
    int r_433, w_433, r_udp, w_udp, p_proc;
    struct timeval time;
    buffer *rf433_rbuf, *rf433_wbuf, *udp_rbuf, *udp_wbuf;
    se433_list *se433l;
    rswp433_pkg *pkg;

#define MS(rr,wr,ru,wu,p) do { r_433=rr; w_433=wr; r_udp=ru; w_udp=wu; p_proc=p; } while(0)

    rf433_rbuf = rf433_wbuf = udp_rbuf = udp_wbuf = NULL;

    rf433_rbuf = buf_new_max();
    if (rf433_rbuf == NULL) {
        app_log_printf(LOG_ERR, "new rf433_rbuf memory error");
        goto out;
    }

    rf433_wbuf = buf_new_max();
    if (rf433_wbuf == NULL) {
        app_log_printf(LOG_ERR, "new rf433_wbuf memory error");
        goto out;
    }

    udp_rbuf = buf_new_max();
    if (udp_rbuf == NULL) {
        app_log_printf(LOG_ERR, "new udp_rbuf memory error");
        goto out;
    }

    udp_wbuf = buf_new_max();
    if (udp_wbuf == NULL) {
        app_log_printf(LOG_ERR, "new udp_wbuf memory error");
        goto out;
    }

    pkg = rswp433_pkg_new();
    if (pkg == NULL) {
        app_log_printf(LOG_ERR, "new rswp433_pkg memory error");
        goto out;
    }


    MS(1, 0, 0, 0, 0);

    while (!exitflag) {

        TRACE("MachineState(rf=%d,wf=%d,ru=%d,wu=%d,p=%d)\n", r_433, w_433, r_udp, w_udp, p_proc);

        FD_ZERO(&rset);
        FD_ZERO(&wset);

        /* caculate the delay timer */
        if (foreground_mode) {
            time.tv_sec = 5;
            time.tv_usec = 0;
        } else {
            int se433_tnr = (rf433x->se433.num == 0 ? 1 : rf433x->se433.num);
            time.tv_sec = RF433_SE433_MAX / se433_tnr;
            time.tv_usec = (long)((RF433_SE433_MAX % se433_tnr) / (float)se433_tnr * 1000);
        }

        /*
         * check pipe read for main process signal
         */
        FD_SET(rf433x->pipe_fd, &rset);
        max_fd = max(0, rf433x->pipe_fd);

        if (r_433) {
            FD_SET(rf433x->rf433_fd, &rset);
        }
        if (w_433) {
            FD_SET(rf433x->rf433_fd, &wset);
        }
        max_fd = max(max_fd, rf433x->rf433_fd);

        if (r_udp) {
            FD_SET(rf433x->sock_fd, &rset);
        }
        if (w_udp) {
            FD_SET(rf433x->sock_fd, &wset);
        }
        max_fd = max(max_fd, rf433x->sock_fd);


        TRACE("select ALGORITHM delay %d(s)\n", (int)time.tv_sec);

        /* monitor all file discription with read and write operate
         * time out is caculate by se433 number
         */
        ret = select(max_fd + 1, &rset, &wset, NULL, &time);

        if (ret == -1) {
            /* signal interrupt the select */
            if (errno == EAGAIN || errno == EINTR)
                continue;

            app_log_printf(LOG_ERR, "relay_rf433():select error:%s", strerror(errno));

            /* exit the thread */
            goto out;
        }

        /* select is time out
         * process timeout funcion
         */
        if (ret == 0) {

            TRACE("select time out\n");

            /* test clean_up */
            //TRACE("test pthread clean up\n");
            //pthread_exit((void*)1);

            /* test se433 list */
            //se433_list_show(&rf433x->se433);

            /* remove offline se433 in the list */
            se433l = se433_find_offline(&rf433x->se433);
            if (se433l != NULL) {
                app_log_printf(LOG_INFO, "se433 0x%08x offline, req_cnt=%d, rsp_cnt=%d",
                        se433l->se433.addr, se433l->se433.req_cnt, se433l->se433.rsp_cnt);
                se433_del(&rf433x->se433, se433l->se433.addr);
            }

            /* request new data for the earliest se433 */
            se433l = se433_find_earliest(&rf433x->se433);
            if (se433l == NULL) {
                app_log_printf(LOG_DEBUG, "can not find the earliest se433");
                continue;
            }

            TRACE("request new data for the se433 0x%08x\n", se433l->se433.addr);
            rswp433_data_req(se433l, rf433x, rf433_wbuf);

            /* next data request write to rf433 */
            MS(0, 1, 0, 0, 0);

            continue;
        }

        /* new msg from main process */
        if (FD_ISSET(rf433x->pipe_fd, &rset)) {
            char x;

            app_log_printf(LOG_INFO, "got main process write pipe signal");

            while (read(rf433x->pipe_fd, &x, 1) > 0) {}

            /* exit the thread */
            goto out;
        }

        /* read from 433 */
        if (FD_ISSET(rf433x->rf433_fd, &rset)) {

            TRACE("rf433_fd can be read\n");

            ret = buf_read(rf433x->rf433_fd, rf433_rbuf);

            if (ret == -1) {
                app_log_printf(LOG_ERR, "read(rf433_fd) error: %s", strerror(errno));

                /* exit the thread */
                goto out;

            } else if (ret == 0) {
                /* maybe error */
                app_log_printf(LOG_ERR, "read(rf433_fd) closed");

                /* exit the thread */
                goto out;

            } else {
                app_log_printf(LOG_DEBUG, "read %d bytes from 433", ret);

                /* next process data from rf433 rbuf */
                MS(0, 0, 0, 0, 1);
            }
        }

        /* write to 433 */
        if (FD_ISSET(rf433x->rf433_fd, &wset)) {

            TRACE("rf433_fd can be write\n");

            ret = buf_write(rf433x->rf433_fd, rf433_wbuf);

            if (ret == -1) {
                app_log_printf(LOG_ERR, "write() error: %s", strerror(errno));

                /* exit the thread */
                goto out;

            } else {
                app_log_printf(LOG_DEBUG, "write %d bytes to 433", ret);

                /* next read data to rf433 rbuf */
                MS(1, 0, 0, 0, 0);
            }
        }

#if 0
        /**
         * no Requirements data from udp
         */

        /* read from udp */
        if (FD_ISSET(rf433x->sock_fd, &rset)) {

            /*
            len = sizeof(cliaddr);
            ret = recvfrom(rf433x->sock_fd, udp_wbuf, UDP_BUF_SIZE - len_u2r, 0,
                    (struct sockaddr*)&cliaddr, &len);
            */
            ret = buf_read(rf433x->sock_fd, udp_wbuf);

            if (ret == -1) {
                sys_printf(LOG_ERR, "recvfrom(sock_fd) error: %s", strerror(errno));

                /* exit the thread */
                goto out;

            } else if (ret == 0) {
                /* maybe error */
                sys_printf(LOG_ERR, "read(sock_fd) closed");

                /* exit the thread */
                goto out;

            } else {
                sys_printf(LOG_INFO, "read %d bytes from UDP", ret);

                if (loglevel == LOG_DEBUG) {
                    buf_dump(udp_wbuf);
                }

                MS(0, 1, 0, 0, 0);
            }
        }
#endif

        /* write to udp */
        if (FD_ISSET(rf433x->sock_fd, &wset)) {
            socklen_t len;
            struct sockaddr_in cliaddr;

            TRACE("sock_fd can be write\n");

            len = sizeof(cliaddr);
            bzero(&cliaddr, len);

            memcpy(&cliaddr.sin_addr, &rf433x->socket.server_ip, sizeof(cliaddr.sin_addr));
            cliaddr.sin_port = htons(rf433x->socket.server_port);
            cliaddr.sin_family = AF_INET;

            ret = sendto(rf433x->sock_fd, buf_data(udp_wbuf), buf_len(udp_wbuf), 0,
                    (struct sockaddr*)&cliaddr, len);
            /*
            ret = buf_write(rf433x->sock_fd, rf433_rbuf);
            */

            if (ret == -1) {
                app_log_printf(LOG_ERR, "write(udp_fd) error: %s", strerror(errno));

                /* exit the thread */
                goto out;

            } else {
                app_log_printf(LOG_DEBUG, "write %d bytes to UDP %s:%d", ret,
                        inet_ntoa(rf433x->socket.server_ip), rf433x->socket.server_port);

                /* next read data to rf433 rbuf */
                MS(1, 0, 0, 0, 0);
            }

            buf_clean(udp_wbuf);
        }

        /**
         * some leave packet to process
         */
        if (p_proc) {

            TRACE("some leave packet to process\n");

            /* got a rswp433 packet */
            if (rswp433_pkg_analysis(rf433_rbuf, pkg) == 1) {

                TRACE("got a valid rswp433 pkg\n");

                /* check the rswp433 command */
                switch (pkg->u.content.cmd) {

                    case RSWP433_CMD_REG_REQ:

                        /* check the address */
                        if (pkg->u.content.dest_addr != RF433_BROADCAST) {

                            app_log_printf(LOG_WARNING, "0x%08x is not register addr, drop this packet\n",
                                pkg->u.content.dest_addr);

                            /* next read data to rf433 rbuf */
                            MS(1, 0, 0, 0, 0);
                        }

                        else {

                            /* first register the se433 */
                            if ((se433l = rswp433_reg_req(pkg, rf433x)) != NULL) {

                                /* log the message */
                                app_log_printf(LOG_INFO, "se433 0x%08x online\n", se433l->se433.addr);

                                /* second, response the reg_ok message to se433 */
                                rswp433_reg_rsp(se433l, rf433x, rf433_wbuf);

                                /* next write response data to rf433 */
                                MS(0, 1, 0, 0, 0);

                            } else {

                                /* register error, next reread the rf433 data */
                                MS(1, 0, 0, 0, 0);
                            }
                        }

                        break;

                    case RSWP433_CMD_DATA_RSP:

                        /* check if my address */
                        if (pkg->u.content.dest_addr != rf433x->rf433.local_addr) {

                            app_log_printf(LOG_WARNING, "0x%08x is not my addr, drop this packet\n",
                                pkg->u.content.dest_addr);

                            /* next read data to rf433 rbuf */
                            MS(1, 0, 0, 0, 0);

                        } else {

                            /* got the se433 sensor data */
                            if (rswp433_data_rsp(pkg, rf433x, udp_wbuf) < 0) {
                                MS(1, 0, 0, 0, 0);
                            }

                            /* next write rswp433 protocol data to udp */
                            else {
                                MS(0, 0, 0, 1, 0);
                            }
                        }

                        break;

                    default:
                        TRACE("invalid rswp433 cmd 0x%x, drop the packet\n", pkg->u.content.cmd);
                        MS(1, 0, 0, 0, 0);

                        break;
                }

                buf_clean(rf433_rbuf);
                rswp433_pkg_clr(pkg);

                continue;

            } else {

                TRACE("got a invalid rswp433 pkg\n");

                if (loglevel == LOG_DEBUG) {
                    buf_dump(rf433_rbuf);
                }

                /* not a valid rswp433 packet */
                buf_clean(rf433_rbuf);
                rswp433_pkg_clr(pkg);
                MS(1, 0, 0, 0, 0);

            }

        }
    }

out:
    buf_free(rf433_rbuf);
    buf_free(rf433_wbuf);
    buf_free(udp_rbuf);
    buf_free(udp_wbuf);
    rswp433_pkg_del(pkg);
    return 0;

}

void thread_cleanup(void *arg)
{
    rf433_instence *rf433x = (rf433_instence*)arg;

    app_log_printf(LOG_ERR, "Called clean-up handler");

    write(rf433x->pipe_fd, "0", 1);
}


/*****************************************************************************
* Function Name  : thread_job_rf433
* Description    : rf433 repeater pthread
* Input          : (rf433_instence*)void*
* Output         : None
* Return         : void *
*                  - 0:ok
*****************************************************************************/
void *thread_job_rf433(void *arg)
{
    rf433_instence *rf433x;

    TRACE("enter rf433_repeater_job");

    pthread_cleanup_push(thread_cleanup, arg);

    rf433x = (rf433_instence*)arg;

    while (INST_STAUTS(rf433x->status) == INST_START) {

        rf433x->rf433_fd = open_rf433(RF433_RF_DEV_NAME);
        if (rf433x->rf433_fd == -1) {
            app_log_printf(LOG_ERR, "open_rf433 error");
            pthread_exit((void*)1);
        }
        set_rf433_opt(rf433x->rf433_fd, rf433x->rf433.net_id, rf433x->rf433.rate);

        rf433x->sock_fd = open_socket();
        if (rf433x->sock_fd == -1) {
            app_log_printf(LOG_ERR, "open_socket error");
            pthread_exit((void*)1);
        }
        set_socket_opt(rf433x->sock_fd, rf433x->socket.local_port);
        TRACE("%-20s: %s", "sockopt.server_ip", inet_ntoa(rf433x->socket.server_ip));
        TRACE("%-20s: %d", "sockopt.server_port", rf433x->socket.server_port);

        relay_rf433(rf433x);

        close(rf433x->rf433_fd);
        close(rf433x->sock_fd);

        rf433x->rf433_fd = -1;
        rf433x->sock_fd = -1;

        /* wait 3 second for while(1) */
        //sleep(3);
    }

    close(rf433x->pipe_fd);
    rf433x->pipe_fd = -1;

    TRACE("leave rf433_repeater_job");

    pthread_cleanup_pop(cleanup_pop_arg);

    pthread_exit((void*)0);
}

/*****************************************************************************
* Function Name  : thread_create
* Description    : create rf433 pthread
* Input          : void
* Output         : None
* Return         : int
*                  - 0:ok
*****************************************************************************/
int thread_create(void)
{
    int err;
    int childpipe[2];

    if (pipe(childpipe) < 0) {
        app_log_printf(LOG_ERR, "error creating rf433 daemon pipe");
        return -1;
    }

    spipefd[0] = childpipe[0];
    rf433i.pipe_fd = childpipe[1];

    rf433i.status = INST_START;

    err = pthread_create(&rf433i.tid, NULL, thread_job_rf433, (void*)&rf433i);

    if (err == -1) {
        app_log_printf(LOG_ERR, "rfrepeater thread create faild!");
        return err;
    }

    app_log_printf(LOG_INFO, "rfrepeater tid %ld thread has started", (long)rf433i.tid);

    return 0;
}

/*****************************************************************************
* Function Name  : thread_close
* Description    : close rf433 pthread
* Input          : void
* Output         : None
* Return         : int
*                  - 0:ok
*****************************************************************************/
int thread_close(void)
{
    int thread_ret = 0;

    app_log_printf(LOG_ALERT, "close the pthread tid is %ld", (long)rf433i.tid);

    rf433i.status &= (~INST_START);

    write(spipefd[0], "0", 1);
    close(spipefd[0]);
    spipefd[0] = -1;

    thread_ret = pthread_join(rf433i.tid, NULL);
    app_log_printf(LOG_ALERT, "closed pthread %ld ret is %d",
            (long)rf433i.tid, thread_ret);

    rf433i.tid = 0;
    rf433i.pipe_fd = -1;
    rf433i.sock_fd = -1;
    rf433i.rf433_fd = -1;

    /* clean se433 list */
    se433_clean(&rf433i.se433);

    return 0;
}

/*****************************************************************************
* Function Name  : msg_to_resp
* Description    : response message to client
* Input          : client_id, string format
* Output         : None
* Return         : int
*                  -  0:ok
*                  - -1:error
*****************************************************************************/
int msg_to_resp(buffer *msg_wbuf, uint8_t ret, char *str, ...)
{
    struct msg_st *wmsg = (struct msg_st *)buf_data(msg_wbuf);
    char tmpline[MSG_RESP_LEN] = "";
    va_list args;

    va_start(args, str);
    vsnprintf(tmpline, MSG_RESP_LEN - 1, str, args);
    va_end(args);

    app_log_printf(LOG_DEBUG, "response for the message");

    buf_clean(msg_wbuf);

    wmsg->h.magic[0] = MSG_CTL_MAGIC_0;
    wmsg->h.magic[1] = MSG_CTL_MAGIC_1;
    wmsg->h.flags = 0;
    wmsg->h.version = MSG_CTL_VERSION;

    wmsg->d.type = MSG_RSP_END;
    wmsg->d.content[0] = ret;

    buf_incrlen(msg_wbuf, sizeof(struct msg_head) + 5);

    buf_append(msg_wbuf, tmpline, strlen(tmpline));

    return 0;
}

/*****************************************************************************
* Function Name  : msg_to_read_config
* Description    : receive message to read config
* Input          : void
* Output         : None
* Return         : int
*                  - 0:ok
*****************************************************************************/
int msg_to_read_config(buffer *msg_rbuf, buffer *msg_wbuf)
{
    int ret;

    thread_close();

    ret = load_config();

    if (ret < 0) {
        app_log_printf(LOG_ALERT, "read config error, use default config");
        default_init();
    }

    if (thread_create()) {
        msg_to_resp(msg_wbuf, MSG_RET_ERR, "restart thread error");
        return -1;
    }

    msg_to_resp(msg_wbuf, MSG_RET_OK, "");

    return ret;
}

/*****************************************************************************
* Function Name  : msg_to_save_config
* Description    : receive message to save config
* Input          : void
* Output         : None
* Return         : int
*                  - 0:ok
*****************************************************************************/
int msg_to_save_config(buffer *msg_rbuf, buffer *msg_wbuf)
{
    int ret;

    ret = save_config();

    msg_to_resp(msg_wbuf, MSG_RET_OK, "");

    return ret;
}

/*****************************************************************************
* Function Name  : msg_to_get_config
* Description    : receive message to get current config
* Input          : message client id
* Output         : None
* Return         : int
*                  - 0:ok
*****************************************************************************/
int msg_to_get_config(buffer *msg_rbuf, buffer *msg_wbuf)
{
    struct msg_st *wmsg = (struct msg_st *)buf_data(msg_wbuf);
    struct msg_config *cfg;

    buf_clean(msg_wbuf);

    wmsg->h.magic[0] = MSG_CTL_MAGIC_0;
    wmsg->h.magic[1] = MSG_CTL_MAGIC_1;
    wmsg->h.flags = 0;
    wmsg->h.version = MSG_CTL_VERSION;

    wmsg->d.type = MSG_RSP_GET_CFG;
    wmsg->d.content[0] = MSG_RET_OK;

    buf_incrlen(msg_wbuf, sizeof(struct msg_head) + 5);

    cfg = (struct msg_config *)(&wmsg->d.content[1]);
    memcpy(&cfg->socket, &rf433i.socket, sizeof(cfg->socket));
    memcpy(&cfg->rf433, &rf433i.rf433, sizeof(cfg->rf433));

    buf_incrlen(msg_wbuf, sizeof(cfg->socket) + sizeof(cfg->rf433));

    return 0;
}

/*****************************************************************************
* Function Name  : msg_to_get_se433list
* Description    : receive message to get se433 list data
* Input          : message client id
* Output         : None
* Return         : int
*                  - 0:ok
*****************************************************************************/
int msg_to_get_se433list(buffer *msg_rbuf, buffer *msg_wbuf)
{
    struct msg_st *wmsg = (struct msg_st *)buf_data(msg_wbuf);
    se433_list *se433l;

    buf_clean(msg_wbuf);

    wmsg->h.magic[0] = MSG_CTL_MAGIC_0;
    wmsg->h.magic[1] = MSG_CTL_MAGIC_1;
    wmsg->h.flags = 0;
    wmsg->h.version = MSG_CTL_VERSION;

    wmsg->d.type = MSG_RSP_GET_SE433L;
    wmsg->d.content[0] =  MSG_RET_OK;
    wmsg->d.content[1] = 0;

    buf_incrlen(msg_wbuf, sizeof(struct msg_head) + 6);

    /* send sensor 433 address and count */
    list_for_each_entry(se433l, &rf433i.se433.list, list) {
        wmsg->d.content[1]++;
        buf_append(msg_wbuf, (char*)&se433l->se433, sizeof(se433_data));
    }

    return 0;
}

/*****************************************************************************
* Function Name  : msg_to_rf433
* Description    : receive message to set rf433 config
* Input          : rf433_cfg*
* Output         : None
* Return         : int
*                  -  0:ok
*****************************************************************************/
int msg_to_set_cfg(buffer *msg_rbuf, buffer *msg_wbuf)
{
    struct msg_st *rmsg = (struct msg_st *)buf_data(msg_rbuf);

    struct msg_config *cfg = (struct msg_config *)&rmsg->d.content[0];

    thread_close();

    /* set the rf433 config */
    if (cfg->rf433.m_mask & RF433_MASK_NET_ID) {
        app_log_printf(LOG_INFO, "set netid to %d", RF433_NET_ID(cfg->rf433.net_id));
        rf433i.rf433.net_id = cfg->rf433.net_id;
    }

    if (cfg->rf433.m_mask & RF433_MASK_RCV_ADDR) {
        app_log_printf(LOG_INFO, "set local_addr to 0x%08x", cfg->rf433.local_addr);
        rf433i.rf433.local_addr = cfg->rf433.local_addr;
    }

    /* set the socket config */
    if (cfg->socket.m_mask & SOCK_MASK_SER_IP) {
        app_log_printf(LOG_INFO, "set server_ip to %s", inet_ntoa(cfg->socket.server_ip));
        rf433i.socket.server_ip = cfg->socket.server_ip;
    }

    if (cfg->socket.m_mask & SOCK_MASK_UDP_PORT) {
        app_log_printf(LOG_INFO, "set server_port to %d", cfg->socket.server_port);
        rf433i.socket.server_port = cfg->socket.server_port;
    }

    if (thread_create()) {
        msg_to_resp(msg_wbuf, MSG_RET_OK, "restart thread error");
        return -1;
    }
    msg_to_resp(msg_wbuf, MSG_RET_OK, "");

    return 0;
}

/*****************************************************************************
* Function Name  : msg_to_se433
* Description    : receive message to set se433 config
* Input          : se433_cfg*
* Output         : None
* Return         : int
*                  -  0:ok
*****************************************************************************/
int msg_to_se433(buffer *msg_rbuf, buffer *msg_wbuf)
{
    struct msg_st *rmsg = (struct msg_st *)buf_data(msg_rbuf);
    se433_cfg *se433 = (se433_cfg *)&rmsg->d.content[0];
    uint8_t ret;

    switch (se433->op) {
        case SE433_OP_ADD:
            if (se433_add(&rf433i.se433, se433->se433_addr)) {
                ret = MSG_RET_OK;
            } else {
                ret = MSG_RET_ERR;
            }
            break;

        case SE433_OP_DEL:
            if (se433_del(&rf433i.se433, se433->se433_addr) < 0) {
                ret = MSG_RET_OK;
            } else {
                ret = MSG_RET_ERR;
            }
            break;

        default:
            app_log_printf(LOG_ERR, "se433 op %d unsupport", se433->op);
            ret = MSG_RET_ERR;
            return -1;
    }
    msg_to_resp(msg_wbuf, ret, "");

    return 0;
}

/*****************************************************************************
* Function Name  : msg_to_log_level
* Description    : receive message to set loglevel
* Input          : loglevel
* Output         : None
* Return         : int
*                  -  0:ok
*****************************************************************************/
int msg_to_log_level(buffer *msg_rbuf, buffer *msg_wbuf)
{
    struct msg_st *rmsg = (struct msg_st *)buf_data(msg_rbuf);
    uint32_t log = (uint32_t)rmsg->d.content[0];

    app_log_printf(LOG_INFO, "set loglevel to %d", log);

    app_log_level_set(log);

    msg_to_resp(msg_wbuf, MSG_RET_OK, "");

    return 0;
}


int msg_process(buffer *msg_rbuf, buffer *msg_wbuf)
{
    struct msg_st *rmsg;

    rmsg = (struct msg_st *)buf_data(msg_rbuf);

    switch (rmsg->d.type) {

        case MSG_REQ_SET_CFG:
            app_log_printf(LOG_INFO, "MSG_REQ_SET_CFG");
            if (msg_to_set_cfg(msg_rbuf, msg_wbuf) < 0) {
                app_log_printf(LOG_ERR, "msg_to_rf433 error");
            }
            break;

        case MSG_REQ_GET_CFG:
            app_log_printf(LOG_INFO, "MSG_REQ_GET_CFG");
            if (msg_to_get_config(msg_rbuf, msg_wbuf) == -1) {
                app_log_printf(LOG_ERR, "msg_to_get_config error");
            }
            break;

        case MSG_REQ_READ_CFG:
            app_log_printf(LOG_INFO, "MSG_REQ_READ_CFG");
            if (msg_to_read_config(msg_rbuf, msg_wbuf) < 0) {
                app_log_printf(LOG_ERR, "msg_to_read_config error");
            }
            break;

        case MSG_REQ_SAVE_CFG:
            app_log_printf(LOG_INFO, "MSG_REQ_SAVE_CFG");
            if (msg_to_save_config(msg_rbuf, msg_wbuf) < 0) {
                app_log_printf(LOG_ERR, "msg_to_save_config error");
            }
            break;

        case MSG_REQ_SET_LOG:
            app_log_printf(LOG_INFO, "MSG_REQ_SET_LOG");
            if (msg_to_log_level(msg_rbuf, msg_wbuf) == -1) {
                app_log_printf(LOG_ERR, "msg_to_se433 error");
            }
            break;

        case MSG_REQ_SET_SE433:
            app_log_printf(LOG_INFO, "MSG_REQ_SET_SE433");
            if (msg_to_se433(msg_rbuf, msg_wbuf) == -1) {
                app_log_printf(LOG_ERR, "msg_to_se433 error");
            }
            break;

        case MSG_REQ_GET_SE433L:
            app_log_printf(LOG_INFO, "MSG_RSP_SE433L");
            if (msg_to_get_se433list(msg_rbuf, msg_wbuf) == -1) {
                app_log_printf(LOG_ERR, "msg_to_get_se433list error");
            }
            break;

        default:
            app_log_printf(LOG_WARNING, "Unknown MSG command %ld", rmsg->d.type);
            msg_to_resp(msg_wbuf, MSG_RET_CMD_NOSUPPORT, "");
            break;
    }

    return 0;
}

/*****************************************************************************
* Function Name  : sigintterm_handler
* Description    : catch ctrl-c or sigterm
* Input          : None
* Output         : None
* Return         : void
*****************************************************************************/
static void sigintterm_handler(int UNUSED(unused)) {

	exitflag = 1;
    app_log_printf(LOG_EMERG, "rfrepeater daemon shutdown");
    //remove(LOCKFILE);
    app_log_close();
	exit(EXIT_FAILURE);
}

/*****************************************************************************
* Function Name  : sigsegv_handler
* Description    : catch any segvs
* Input          : None
* Output         : None
* Return         : void
*****************************************************************************/
static void sigsegv_handler(int UNUSED(unused)) {
	app_log_printf(LOG_EMERG, "Aiee, segfault! You should probably report "
			"this as a bug to the developer\n");
    app_log_close();
	exit(EXIT_FAILURE);
}

/*****************************************************************************
* Function Name  : main
* Description    : rfrepeater main function
* Input          : argc, argv
* Output         : None
* Return         : int
*                  -  0:ok
*****************************************************************************/
int main(int argc, char *argv[])
{
    int i, ret;
    int ctl_fd, max_fd;
    fd_set rset, wset;
    buffer *ctl_rbuf, *ctl_wbuf;
    socklen_t len;
    struct sockaddr_in ctl_cli;

    for (i = 1; i < (unsigned int)argc; i++)
    {
        if (argv[i][0] == '-') {
            switch (argv[i][1]) {
                case 'd':
                    foreground_mode = 1;
                    break;
                case 'v':
                case 'h':
                    fprintf(stderr, USAGE);
                    exit(EXIT_FAILURE);
                    break;
                default:
                    fprintf(stderr, "Unknown argument %s\n", argv[i]);
                    exit(EXIT_FAILURE);
                    break;
            }
        }
    }

    if (foreground_mode == 1) {
        set_loglevel(LOG_DEBUG);
    } else {
        app_log_open(RF433_LOG_F);
        set_loglevel(LOG_ERR);
    }

    /* set up cleanup handler */
    if (signal(SIGINT, sigintterm_handler) == SIG_ERR ||
            signal(SIGTERM, sigintterm_handler) == SIG_ERR ||
            signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        fprintf(stderr, "signal()1 error\n");
        app_log_close();
        exit(EXIT_FAILURE);
    }

    if (signal(SIGSEGV, sigsegv_handler) == SIG_ERR) {
        fprintf(stderr, "signal()2 error\n");
        app_log_close();
        exit(EXIT_FAILURE);
    }

    if (foreground_mode == 0) {
        if (daemon(0, 0) < 0) {
            fprintf(stderr, "Failed to daemonize: %s", strerror(errno));
            app_log_close();
            exit(EXIT_FAILURE);
        }
    }

    app_log_printf(LOG_INFO, "rfrepeater daemon start");
    //atexit(exit_job);
    //make_pid_file();

    default_init();

    if (load_config()) {
        app_log_printf(LOG_WARNING, "load_config() error, will use default config");
    }

    if (thread_create()) {
        app_log_printf(LOG_ERR, "thread_create() error");
        goto out;
    }

    ctl_rbuf = buf_new_max();
    if (ctl_rbuf == NULL) {
        app_log_printf(LOG_ERR, "new ctl_rbuf memory error");
        goto out1;
    }

    ctl_wbuf = buf_new_max();
    if (ctl_wbuf == NULL) {
        app_log_printf(LOG_ERR, "new ctl_wbuf memory error");
        goto out1;
    }

    ctl_fd = open_socket();
    if (ctl_fd < 0) {
        app_log_printf(LOG_ERR, "open ctl_fd error");
        goto out1;
    }
    set_socket_opt(ctl_fd, SRV_CTL_UDP_PORT);

    while (1)
    {
        TRACE("poll the ctrl process\n");

#ifdef SOCKCLI
        FD_ZERO(&rset);
        FD_ZERO(&wset);

        max_fd = 0;
        for (i = 0; i < RF433_THREAD_MAX; i++) {
            FD_SET(spipefd[i], &rset);
            max_fd = max(max_fd, spipefd[i]);
        }

        FD_SET(ctl_fd, &rset);
        max_fd = max(max_fd, ctl_fd);

        if (buf_is_used(ctl_wbuf)) {
            FD_SET(ctl_fd, &wset);
        }

        ret = select(max_fd + 1, &rset, &wset, NULL, NULL);

        if (ret == -1) {
            /* signal interrupt the select */
            if (errno == EAGAIN || errno == EINTR) {
                continue;
            }

            app_log_printf(LOG_ERR, "main():select error:%s", strerror(errno));

            /* exit the thread */
            goto out1;
        }

        if (FD_ISSET(ctl_fd, &rset)) {

            TRACE("ctl_fd can be read\n");

            len = sizeof(ctl_cli);
            ret = recvfrom(ctl_fd, buf_data(ctl_rbuf), buf_space(ctl_rbuf), 0,
                    (struct sockaddr*)&ctl_cli, &len);

            if (ret <= 0) {
                /* maybe error */
                app_log_printf(LOG_ERR, "read(ctl_fd) error: %s", strerror(errno));

                /* exit the thread */
                goto out1;

            } else {

                buf_incrlen(ctl_rbuf, ret);

                TRACE("ctl_fd read %d bytes\n", ret);
                if (foreground_mode) {
                    buf_dump(ctl_rbuf);
                }

                /* check message validate */
                if (msg_check(ctl_rbuf) == 0) {

                    /* process the command */
                    msg_process(ctl_rbuf, ctl_wbuf);

                } else {

                    /* read invalid message */
                    app_log_printf(LOG_WARNING, "got a invalid ctrl message\n");

                }
            }

            buf_clean(ctl_rbuf);
        }

        if (FD_ISSET(ctl_fd, &wset)) {

            len = sizeof(ctl_cli);
            ret = sendto(ctl_fd, buf_data(ctl_wbuf), buf_len(ctl_wbuf), 0,
                    (struct sockaddr*)&ctl_cli, len);

            if (ret <= 0) {
                /* maybe error */
                app_log_printf(LOG_ERR, "write(ctl_fd) error: %s", strerror(errno));

                /* exit the thread */
                goto out1;
            }

            TRACE("ctl_fd write %d bytes\n", ret);
            if (foreground_mode) {
                buf_dump(ctl_wbuf);
            }

            buf_clean(ctl_wbuf);
        }

        for (i = 0; i < RF433_THREAD_MAX; i++) {
            if (FD_ISSET(spipefd[i], &rset)) {
                //char x;
                //while (read(spipefd[i], &x, 1) > 0) {}
                app_log_printf(LOG_EMERG, "Aiee, thread error! You should probably report "
                        "this as a bug to the developer\n");
                goto out1;
            }
        }
#else
        sleep(1);
#endif
    }


out1:
    if (ctl_fd > 0) {
        close(ctl_fd);
    }
    buf_free(ctl_rbuf);
    buf_free(ctl_wbuf);
out:
    app_log_printf(LOG_INFO, "rf433 daemon terminaled!");
    app_log_close();
    exit(0);
}

