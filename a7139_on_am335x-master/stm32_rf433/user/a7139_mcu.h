//*******************************************************************************
// Author:          redfox.qu@qq.com
// Date:            2014/03/27
// Version:         1.0.0
// Description:     a7139(rf433) chip on stm32f103
//
// History:
//      <author>            <time>      <version>   <desc>
//      redfox.qu@qq.com    2014/03/27  1.0.0       create this file
//
// TODO:
//      2014/03/27
//*******************************************************************************

#ifndef __A7139_H__
#define __A7139_H__

#include "misc.h"
#include "a7139_arch.h"

#define RF_FREQ_TAB_MAXSIZE         16
#define RF_IDSIZE                   2
#define RF_BUFSIZE                  64

typedef enum a7139_rate {
    A7139_RATE_2K       = 0,        // 00
    A7139_RATE_5K,                  // 01
    A7139_RATE_10K,                 // 02
    A7139_RATE_25K,                 // 03
    A7139_RATE_50K,                 // 04
    A7139_RATE_MAX,                 // MAX
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
    A7139_FREQ_470M,                // 00
    A7139_FREQ_472M,                // 01
    A7139_FREQ_474M,                // 02
    A7139_FREQ_476M,                // 03
    A7139_FREQ_478M,                // 04
    A7139_FREQ_480M,                // 05
    A7139_FREQ_482M,                // 06
    A7139_FREQ_484M,                // 07
    A7139_FREQ_486M,                // 08
    A7139_FREQ_488M,                // 09
    A7139_FREQ_490M,                // 10
    A7139_FREQ_492M,                // 11
    A7139_FREQ_494M,                // 12
    A7139_FREQ_496M,                // 13
    A7139_FREQ_498M,                // 14
    A7139_FREQ_500M,                // 15
    A7139_FREQ_MAX,                 // MAX
} A7139_FREQ;

#if 0
typedef enum {
    A7139_FREQ_470_1M,              // 00
    A7139_FREQ_470_9M,              // 01
    A7139_FREQ_471_7M,              // 02
    A7139_FREQ_472_5M,              // 03
    A7139_FREQ_473_3M,              // 04
    A7139_FREQ_474_1M,              // 05
    A7139_FREQ_474_9M,              // 06
    A7139_FREQ_475_7M,              // 07
    A7139_FREQ_476_5M,              // 08
    A7139_FREQ_477_3M,              // 09
    A7139_FREQ_478_1M,              // 10
    A7139_FREQ_478_9M,              // 11
    A7139_FREQ_479_7M,              // 12
    A7139_FREQ_480_5M,              // 13
    A7139_FREQ_481_3M,              // 14
    A7139_FREQ_482_1M,              // 15
    A7139_FREQ_MAX,                 // MAX
} A1739_FREQ;
#endif

#define RF_DEF_ID_D0                0xAA
#define RF_DEF_ID_D1                0x01
#define RF_DEF_FREQ_CH              A7139_FREQ_470M
#define RF_DEF_RATE                 A7139_RATE_2K


/*********************************************************************
**  A7139 Register Address
*********************************************************************/
#define SYSTEMCLOCK_REG             0x00
#define PLL1_REG                    0x01
#define PLL2_REG                    0x02
#define PLL3_REG                    0x03
#define PLL4_REG                    0x04
#define PLL5_REG                    0x05
#define PLL6_REG                    0x06
#define CRYSTAL_REG                 0x07
#define PAGEA_REG                   0x08
#define PAGEB_REG                   0x09
#define RX1_REG                     0x0A
#define RX2_REG                     0x0B
#define ADC_REG                     0x0C
#define PIN_REG                     0x0D
#define CALIBRATION_REG             0x0E
#define MODE_REG                    0x0F

#define TX1_PAGEA                   0x00
#define WOR1_PAGEA                  0x01
#define WOR2_PAGEA                  0x02
#define RFI_PAGEA                   0x03
#define PM_PAGEA                    0x04
#define RTH_PAGEA                   0x05
#define AGC1_PAGEA                  0x06
#define AGC2_PAGEA                  0x07
#define GIO_PAGEA                   0x08
#define CKO_PAGEA                   0x09
#define VCB_PAGEA                   0x0A
#define CHG1_PAGEA                  0x0B
#define CHG2_PAGEA                  0x0C
#define FIFO_PAGEA                  0x0D
#define CODE_PAGEA                  0x0E
#define WCAL_PAGEA                  0x0F

#define TX2_PAGEB                   0x00
#define IF1_PAGEB                   0x01
#define IF2_PAGEB                   0x02
#define ACK_PAGEB                   0x03
#define ART_PAGEB                   0x04


/*********************************************************************
**  A7139 MODE
*********************************************************************/
#define CMD_SLEEP_MODE              0x10 // 0001,0000 Sleep mode
#define CMD_IDLE_MODE               0x12 // 0001,0010 Idle mode
#define CMD_STANDBY_MODE            0x14 // 0001,0100 Standby mode
#define CMD_PLL_MODE                0x16 // 0001,0110 PLL mode
#define CMD_RX_MODE                 0x18 // 0001,1000 RX mode
#define CMD_TX_MODE                 0x1a // 0001,1010 TX mode
#define CMD_DEEP_SLEEP_T            0x1c // 0001,1100 Deep Sleep mode(tri-state)
#define CMD_DEEP_SLEEP_P            0x1f // 0001,1111 Deep Sleep mode(pull-high)


/*********************************************************************
**  Constant Declaration
*********************************************************************/
#define CMD_CTRLW                   0x00 // 000x,xxxx control register write
#define CMD_CTRLR                   0x80 // 100x,xxxx control register read
#define CMD_IDW                     0x20 // 001x,xxxx ID write
#define CMD_IDR                     0xA0 // 101x,xxxx ID Read
#define CMD_DATAW                   0x40 // 010x,xxxx TX FIFO Write
#define CMD_DATAR                   0xC0 // 110x,xxxx RX FIFO Read
#define CMD_TFR                     0x60 // 0110,xxxx TX FIFO reset
#define CMD_RFR                     0xE0 // 1110,xxxx RX FIFO reset
#define CMD_RFRST                   0x70 // x111,xxxx RF reset


/* global variables */
struct a7139_spi_pin {
    struct a7139_stm32_pin scs;
    struct a7139_stm32_pin sck;
    struct a7139_stm32_pin sdio;
    struct a7139_stm32_pin gio1;
};

struct a7139_dev {
    /* hardware chip pin configs */
    struct a7139_spi_pin pin;

    /* 433 modules configs */
    volatile A7139_MODE rf_currmode;
    A7139_RATE rf_datarate;
    uint8_t rf_freq_ch;
    uint8_t rf_id[RF_IDSIZE];

    /* variables */
    uint8_t txbuf[RF_BUFSIZE];
    uint8_t rxbuf[RF_BUFSIZE];
    uint8_t rx_len;
    uint8_t tx_len;
};

///*********************************************************************
//**  function Declaration
//*********************************************************************/
void a7139_byte_send(struct a7139_dev *dev, uint8_t src);
uint8_t a7139_byte_read(struct a7139_dev *dev);
void a7139_send_ctrl(struct a7139_dev *dev, uint8_t cmd);
void a7139_write_reg(struct a7139_dev *dev, uint8_t address, uint16_t dataWord);
uint16_t a7139_read_reg(struct a7139_dev *dev, uint8_t address);
void a7139_write_page_a(struct a7139_dev *dev, uint8_t address, uint16_t dataWord);
uint16_t a7139_read_page_a(struct a7139_dev *dev, uint8_t address);
void a7139_write_page_b(struct a7139_dev *dev, uint8_t address, uint16_t dataWord);
uint16_t a7139_read_page_b(struct a7139_dev *dev, uint8_t address);
int a7139_config(struct a7139_dev *dev);
int a7139_write_id(struct a7139_dev *dev, uint8_t *id);
int a7139_read_id(struct a7139_dev *dev, uint8_t *id);
void a7139_mode_switch(struct a7139_dev *dev, A7139_MODE mode);
int a7139_datarate_set(struct a7139_dev *dev, A7139_RATE drate);
int a7139_freq_set(struct a7139_dev *dev, uint8_t ch);
int a7139_cal(struct a7139_dev *dev);
int a7139_chip_init(struct a7139_dev *dev);
void a7139_send_packet(struct a7139_dev *dev, uint8_t *txBuffer, uint8_t size);
uint8_t a7139_receive_packet(struct a7139_dev *dev, uint8_t *buf, uint8_t len);

#endif  //__A7139_H__

