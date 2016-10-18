//*******************************************************************************
// Copyright (C), RHTECH. Co., Ltd.
// Author:          redfox.qu@qq.com
// Date:            2014/01/08
// Version:         1.0.0
// Description:     The 433Mhz module package struct header file
//
// History:
//      <author>            <time>      <version>   <desc>
//      redfox.qu@qq.com    2014/01/08  1.0.0       create this file
//
// TODO:
//*******************************************************************************

#ifndef __RF433_PKG_H__
#define __RF433_PKG_H__

#include <stdint.h>

#pragma pack (1)

typedef struct {
    uint8_t sp[2];                          /* 软件前导码 */
    uint8_t len;                            /* 负载长度 */
    uint8_t crc1;                           /* 首部校验 */
} rswp433_pkg_header;

typedef struct {
    uint8_t type;                           /* 传感器类型 */
    float data;                             /* 传感器数据，采用IEEE745浮点数格式 */
    uint8_t vol;                            /* 传感器电压，0xAB表示A.B伏 */
    uint8_t batt;                           /* 电池剩余电量，0-0x64表示0%-100% */
    uint8_t flag;                           /* 状态码 */
    uint8_t watchid;                        /* 巡检遥控器编码 */
} rswp433_data;

typedef struct {
    uint32_t dest_addr;                     /* 433无线目标地址 */
    uint32_t src_addr;                      /* 433无线源地址 */
    uint8_t cmd;                            /* RSWP433无线协议命令字 */
    uint8_t crc2;                           /* RSWP433无线协议报文校验 */
} rswp433_pkg_content;

typedef struct {
    uint32_t dest_addr;                     /* 433无线目标地址 */
    uint32_t src_addr;                      /* 433无线源地址 */
    uint8_t cmd;                            /* RSWP433无线协议命令字 */
    rswp433_data data;                      /* RSWP433无线传感器数据 */
    uint8_t crc2;                           /* RSWP433无线协议报文校验 */
} rswp433_pkg_data_content;

typedef struct {                            /* RSWP433无线协议报文格式 */
    rswp433_pkg_header header;
    union {
        rswp433_pkg_content content;
        rswp433_pkg_data_content data_content;
    } u;
} rswp433_pkg;

typedef struct {
    uint32_t addr;                          /* 433无线传感器地址 */
    rswp433_data data;                      /* 433无线传感器数据 */
    time_t last_req_tm;                     /* 433无线传感器最后请求时戳 */
    int req_cnt;                            /* 433无线传感器请求次数 */
    int rsp_cnt;                            /* 433无线传感器响应次数 */
} se433_data;

#pragma pack ()

#endif  //__RF433_PKG_H__

