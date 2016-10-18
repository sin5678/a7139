//*****************************************************************************
//
// a7139.h  The 433Mhz module chip a7139 linux driver on am335x
//
// author:  yaobyron@gmail.com
// update:  2013/12/27
// version: v1.0
//
//*****************************************************************************

/******************************************************************************
 * TODO:
 *
 * History:
 *  2013/12/27      create this file
 ******************************************************************************/

#ifndef __A7139_H__
#define __A7139_H__

#define A7139_IOC_MAGIC         'A'
#define A7139_IOC_MAXNR         8

#define A7139_IOC_DUMP          _IO(A7139_IOC_MAGIC, 1)
#define A7139_IOC_RESET         _IO(A7139_IOC_MAGIC, 2)
#define A7139_IOC_SETFREQ       _IOW(A7139_IOC_MAGIC, 3, uint8_t)
#define A7139_IOC_GETFREQ       _IOR(A7139_IOC_MAGIC, 4, uint8_t)
#define A7139_IOC_SETID         _IOW(A7139_IOC_MAGIC, 5, uint16_t)
#define A7139_IOC_GETID         _IOR(A7139_IOC_MAGIC, 6, uint16_t)
#define A7139_IOC_SETRATE       _IOW(A7139_IOC_MAGIC, 7, uint8_t)
#define A7139_IOC_GETRATE       _IOR(A7139_IOC_MAGIC, 8, uint8_t)

#define RF_FREQ_TAB_MAXSIZE     16
#define RF_IDSIZE               2

typedef enum a7139_rate {
    A7139_RATE_2K       = 0,    // 00
    A7139_RATE_5K,              // 01
    A7139_RATE_10K,             // 02
    A7139_RATE_25K,             // 03
    A7139_RATE_50K,             // 04
    A7139_RATE_MAX,             // MAX
} A7139_RATE;

typedef enum {
    A7139_MODE_SLEEP,
    A7139_MODE_IDLE,
    A7139_MODE_STANDBY,
    A7139_MODE_PLL,
    A7139_MODE_RX,
    A7139_MODE_RXING,
    A7139_MODE_TX,
    A7139_MODE_TXING,
    A7139_MODE_DEEPSLEEPT,
    A7139_MODE_DEEPSLEEPP,
} A7139_MODE;

typedef enum {
    A7139_FREQ_470M,            // 00
    A7139_FREQ_472M,            // 01
    A7139_FREQ_474M,            // 02
    A7139_FREQ_476M,            // 03
    A7139_FREQ_478M,            // 04
    A7139_FREQ_480M,            // 05
    A7139_FREQ_482M,            // 06
    A7139_FREQ_484M,            // 07
    A7139_FREQ_486M,            // 08
    A7139_FREQ_488M,            // 09
    A7139_FREQ_490M,            // 10
    A7139_FREQ_492M,            // 11
    A7139_FREQ_494M,            // 12
    A7139_FREQ_496M,            // 13
    A7139_FREQ_498M,            // 14
    A7139_FREQ_500M,            // 15
    A7139_FREQ_MAX,             // MAX
} A7139_FREQ;

#if 0
typedef enum {
    A7139_FREQ_470_1M,          // 00
    A7139_FREQ_470_9M,          // 01
    A7139_FREQ_471_7M,          // 02
    A7139_FREQ_472_5M,          // 03
    A7139_FREQ_473_3M,          // 04
    A7139_FREQ_474_1M,          // 05
    A7139_FREQ_474_9M,          // 06
    A7139_FREQ_475_7M,          // 07
    A7139_FREQ_476_5M,          // 08
    A7139_FREQ_477_3M,          // 09
    A7139_FREQ_478_1M,          // 10
    A7139_FREQ_478_9M,          // 11
    A7139_FREQ_479_7M,          // 12
    A7139_FREQ_480_5M,          // 13
    A7139_FREQ_481_3M,          // 14
    A7139_FREQ_482_1M,          // 15
    A7139_FREQ_MAX,             // MAX
} A1739_FREQ;
#endif

#define RF_DEF_ID_D0            0xAA
#define RF_DEF_ID_D1            0x01
#define RF_DEF_FREQ_CH          A7139_FREQ_470M
#define RF_DEF_RATE             A7139_RATE_2K

#endif

