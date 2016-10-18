//*******************************************************************************
// Description:     a7139.c  The 433Mhz module chip a7139 linux driver on am335x
// Author:          redfox.qu@qq.com
// Date:            2013/12/25
// Version:         1.0.0
// Copyright:       RHTECH. Co., Ltd.
//
// History:
//      <author>            <time>      <version>   <desc>
//      redfox.qu@qq.com    2013/12/19  1.0.0       create this file
//                                                  add cdev driver frame
//                                                  add soft spi example code
//      yaobyron@gmail.com  2013/12/30  1.0.1       add hardware reg function
//                                                  add interrupt handle function
//      redfox.qu@qq.com    2014/01/09  1.0.2       add ioctl function
//      redfox.qu@qq.com    2014/01/10  1.0.3       use tasklet for interrupt half bottom in read
//      yaobyron@gmail.com  2014/01/17  1.0.4       1.add a7139_dev_init function
//                                                  2.Fix bug: After calling ioctl, unable to receive data again
//                                                      Resolved: modify ioctl function, After calling ioctl, switch to rx mode
//                                                  3.modify freq_cal_tab[5]'s value, modify 480.001MHZ to 480.000MHZ
//      yaobyron@gmail.com  2014/01/21  1.0.5       Fix bug: After the program is running, immediately received an invalid packet
//                                                      Resolved: In the ioctl function, after switching to standby mode, waiting for some time
//      yaobyron@gmail.com  2014/01/23  1.0.6       Fix bug#54: Sometimes equipment has not writable
//                                                      Resolved: In the a7139_tasklet_func function, before the switch to rx mode, to determine whether the rx mode
//      yaobyron@gmail.com  2014/02/11  1.1.0       modify tasklet to workqueue
//
//*******************************************************************************
#include <linux/types.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/ioctl.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <asm/uaccess.h>
#include <linux/workqueue.h>

#include "a7139_rf.h"
#include "a7139.h"

/* Defines by hardware pin */
#define GPIO_TO_PIN(b, g)       (32 * (b) + (g))
#define GPIO_SDIO               GPIO_TO_PIN(3, 17)
#define GPIO_SCK                GPIO_TO_PIN(3, 18)
#define GPIO_SCS                GPIO_TO_PIN(3, 14)
#define GPIO_GIO1               GPIO_TO_PIN(3, 13)
//#define GPIO_GIO2             GPIO_TO_PIN(1, 28)
#define IO_ON                   1
#define IO_OFF                  0

#define gpio_pin_mo_h(gpio)     gpio_direction_output(gpio, IO_ON)
#define gpio_pin_mo_l(gpio)     gpio_direction_output(gpio, IO_OFF)
#define gpio_pin_h(gpio)        gpio_set_value(gpio, IO_ON)
#define gpio_pin_l(gpio)        gpio_set_value(gpio, IO_OFF)
#define gpio_pin_mi(gpio)       gpio_direction_input(gpio)
#define gpio_pin_in(gpio)       gpio_get_value(gpio)
//DELAY
#define spi_ndelay(n)           ndelay(n)
#define spi_mdelay(n)           mdelay(n)
#define spi_defdelay()          spi_ndelay(80)

#define DEV_WRITE_TIMEOUT       1000       /* ms */
#define DEVICE_NAME             "a7139"    /* device name, see it on /proc/devices */
#define A7139_MAJOR             271        /* master device id */
#define RF_BUFSIZE              64

#define VERSION                 "1.1.0"
#define DEBUG
#undef  DEBUG
#ifdef  DEBUG
#define debugf(fmt, args...)    printk(fmt, ##args)
#else
#define debugf(fmt, args...)
#endif

/* global variables */
struct rf_spi_pin {
    int scs;
    int sck;
    int sdio;
    int gio1;
};

struct rf_dev {
    /* struct for kernel platform */
    struct cdev cdev;
    struct class *dev_class;
    char *name_alias;
    int irq;

    /* hardware chip pin configs */
    struct rf_spi_pin pin;

    /* 433 modules configs */
    volatile A7139_MODE rf_currmode;
    A7139_RATE rf_datarate;
    uint8_t rf_freq_ch;
    uint8_t rf_id[RF_IDSIZE];
    //uint32_t rf_dst_addr;
    //uint32_t rf_src_addr;

    /* variables */
    uint8_t txbuf[RF_BUFSIZE];
    uint8_t rxbuf[RF_BUFSIZE];
    uint8_t rx_len;
    uint8_t tx_len;

    struct semaphore sem;
    struct workqueue_struct *work_queue;
    struct work_struct work;
    volatile uint32_t rf_txevt;
    volatile uint32_t rf_rxevt;
    uint32_t opencount;
    wait_queue_head_t r_wait;
    wait_queue_head_t w_wait;
	struct tasklet_struct tasklet;
};

const uint16_t rf_reg_cfg[] =   //470MHz, 10kbps (IFBW = 50KHz, Fdev = 18.75KHz)
{
    0x0823,     // 0x00, SYSTEM CLOCK register @see 9.2.1 System clock (Address: 00h)
                //      Fmsck = 12.8M, DMOS=1
                //      Fcsck = Fmsck/(CSC+1)=3200000, CSC=3
                //      SDR = Fcsck/rate/64-1
    0x0A24,     // 0x01, PLL1 register,
    0xB805,     // 0x02, PLL2 register, 470.001MHz
    0x0000,     // 0x03, PLL3 register,
    0x0A20,     // 0x04, PLL4 register,
    0x0024,     // 0x05, PLL5 register,
    0x0000,     // 0x06, PLL6 register,
    0x0001,     // 0x07, CRYSTAL register,
    0x0000,     // 0x08, PAGEA,
    0x0000,     // 0x09, PAGEB,
    0x1850,     // 0x0a, RX1 register,
                //      0x18D0: IFBW=50KHz, ETH=1
                //      0x1850: IFBW=50KHz, ETH=0
    0x7009,     // 0x0b, RX2 register, by preamble
    0x4400,     // 0x0c, ADC register,
    0x0800,     // 0x0d, PIN CONTROL register, Use Strobe CMD
    0x4845,     // 0x0e, CALIBRATION register,
    0x20C0      // 0x0f, MODE CONTROL register, Use FIFO mode
};

const uint16_t rf_reg_cfg_page_a[] =   //470MHz, 10kbps (IFBW = 50KHz, Fdev = 18.75KHz)
{
    0x1703,     // 0x00, TX1 register, Fdev = 18.75kHz 0x1703--0x1260
                //       0x1706, S//TX1 register, Fdev = 37.5kHz
    0x0000,     // 0x01, WOR1 register,
    0x0000,     // 0x02, WOR2 register,
    0x1187,     // 0x03, RFI register, Enable Tx Ramp up/down
    0x8160,     // 0x04, PM register, CST=1
    0x0302,     // 0x05, RTH register,
    0x400F,     // 0x06, AGC1 register,
    0x0AC0,     // 0x07, AGC2 register,
    0x0041,     // 0x08, GIO register, GIO2=WTR, GIO1=WTR
    0xD281,     // 0x09, CKO register
    0x0004,     // 0x0a, VCB register,
    0x0825,     // 0x0b, CHG1 register, 480MHz
    0x0127,     // 0x0c, CHG2 register, 500MHz
    0x003F,     // 0x0d, FIFO register, FEP=63+1=64bytes
    0x150B,     // 0x0e, CODE register,
                //      0x151F:Preamble=4bytes, ID=4bytes, FEC+CRC;
                //      0x150F:Preamble=4bytes, ID=4bytes, CRC;
                //      0x1519:Preamble=2bytes, ID=2bytes, FEC+CRC;
                //      0x1509:Preamble=2bytes, ID=2bytes, CRC;
                //      0x150B:Preamble=4bytes, ID=2bytes, CRC;
    0x0000      // 0x0f, WCAL register,
};

const uint16_t rf_reg_cfg_page_b[] =   //470MHz, 10kbps (IFBW = 100KHz, Fdev = 18.75KHz)
{
    0x0B37,     // TX2 register, Tx power=20dBm /*0x0B7F-->0x0B37 733 721功率测试*/
    0x8200,     // IF1 register, Enable Auto-IF, IF=100KHz 0x8400,IF=200KHz
    0x0000,     // IF2 register,
    0x0000,     // ACK register,
    0x0000      // ART register,
};

/*
 * 频率计算方法(详细可参见《A7139芯片手册》第40页):
 * 涉及寄存器: 01h(IP[8:0]), 02h(FP[15:0], 09h Page 2(FPA[15:0])
 */
uint16_t freq_cal_tab[RF_FREQ_TAB_MAXSIZE*2] = {
#if 0
    0x0A24, 0xBA05,     // 470.101MHz
    0x0A26, 0x4805,     // 490.001MHz
    0x0A27, 0xD805,     // 510.001MHz
#endif
    0x0A24, 0xBA05,     // 00, 470.101MHz
    0x0A24, 0xE005,     // 01, 472.001MHz
    0x0A25, 0x0805,     // 02, 474.001MHz
    0x0A25, 0x3005,     // 03, 476.001MHz
    0x0A25, 0x5805,     // 04, 478.001MHz
    0x0A25, 0x7FFF,     // 05, 480.000MHz
    0x0A25, 0xA805,     // 06, 482.001MHz
    0x0A25, 0xD005,     // 07, 484.001MHz
    0x0A25, 0xF805,     // 08, 486.001MHz
    0x0A26, 0x2005,     // 09, 488.001MHz
    0x0A26, 0x4805,     // 10, 490.001MHz
    0x0A26, 0x7005,     // 11, 492.001MHz
    0x0A26, 0x9805,     // 12, 494.001MHz
    0x0A26, 0xC005,     // 13, 496.001MHz
    0x0A26, 0xE805,     // 14, 498.001MHz
    0x0A27, 0x1005,     // 15, 500.001MHz
};

/*
 * 波特率计算方法(详细可参见《A7139芯片手册》第12页):
 * 当DMOS(0Ah寄存器)为1时, DataRate = (1/64)*Fcsck/(SDR[6:0]+1); (在这里, 默认DMOS=1)
 * 当DMOS(0Ah寄存器)为0时, DataRate = (1/32)*Fcsck/(SDR[6:0]+1);
 * 其中Fcsck = Fmsck/(CSC[2:0]+1), Fmsck在A7139芯片是12.8MHZ, SDR[6:0]和CSC[2:0]在00h寄存器中
 */
uint16_t rate_cal_tab[] = {
    0x3023,             // 2k
    0x1223,             // 5k
    0x0823,             // 10k
    0x0223,             // 25k
    0x0023,             // 50k
};

static struct rf_dev devs[] = {
    {
        .name_alias = "a7139-1",
        .irq        = -1,
        .pin        = {
            .scs        = GPIO_SCS,
            .sck        = GPIO_SCK,
            .sdio       = GPIO_SDIO,
            .gio1       = GPIO_GIO1,
        },
        .rf_currmode    = A7139_MODE_STANDBY,
        .rf_datarate    = RF_DEF_RATE,
        .rf_freq_ch     = RF_DEF_FREQ_CH,
        .rf_id          = {RF_DEF_ID_D0, RF_DEF_ID_D1},
    },
};


#if 0
const uint8_t bit_count_tab[16] = {0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4};
static uint8_t id_tab[8] = {0xAA,0x75,0xC5,0x8C,0xC7,0x33,0x45,0xE7};   //ID code
#endif


static int a7139_major = A7139_MAJOR;
static struct class *dev_class;


//**********************************************************************************
// 功能描述 : 发送1字节
// 输入参数 : uint8_t src
// 返回参数 : 无
// 说明     :
//**********************************************************************************
static void a7139_byte_send(struct rf_dev *dev, uint8_t src)
{
    uint8_t i;

    for (i = 0; i < 8; i++) {
        if (src & 0x80) {
            gpio_pin_h(dev->pin.sdio);
        } else {
            gpio_pin_l(dev->pin.sdio);
        }

        spi_defdelay();
        gpio_pin_h(dev->pin.sck);
        spi_defdelay();
        gpio_pin_l(dev->pin.sck);

        src = src << 1;
    }
}

//**********************************************************************************
// 功能描述 : 读1字节
// 输入参数 : 无
// 返回参数 : uint8_t
// 说明     :
//**********************************************************************************
static uint8_t a7139_byte_read(struct rf_dev *dev)
{
    uint8_t i, tmp = 0;

    gpio_pin_h(dev->pin.sdio);      // SDIO pull high
    gpio_pin_mi(dev->pin.sdio);     // change SDIO input

    for (i = 0; i < 8; i++)         // Read one byte data
    {
        if (gpio_pin_in(dev->pin.sdio)) {
            tmp = (tmp << 1) | 0x01;
        } else {
            tmp = tmp << 1;
        }

        spi_defdelay();
        gpio_pin_h(dev->pin.sck);
        spi_defdelay();
        gpio_pin_l(dev->pin.sck);
    }

    // 这里应该将SDIO重新置位输出.因为很多其他写入没有置位输出.后续修改
    gpio_pin_mo_l(dev->pin.sdio);

    return tmp;     // Return tmp value.
}

//**********************************************************************************
// 功能描述 : 发送控制命令
// 输入参数 : uint8_t cmd
// 返回参数 : 无
// 说明     :
//**********************************************************************************
static void a7139_send_ctrl(struct rf_dev *dev, uint8_t cmd)
{
    gpio_pin_l(dev->pin.scs);
    gpio_pin_mo_l(dev->pin.sdio);

    a7139_byte_send(dev, cmd);

    gpio_pin_h(dev->pin.scs);
}

//**********************************************************************************
// 功能描述 : 写寄存器
// 输入参数 : uint8_t address地址 uint16_t dataWord数据
// 返回参数 : 无
// 说明     :
//**********************************************************************************
static void a7139_write_reg(struct rf_dev *dev, uint8_t address, uint16_t dataWord)
{
    uint8_t dataWord1;
    uint8_t dataWord0;

    dataWord0 = (dataWord) & 0x00ff;
    dataWord1 = (dataWord >> 8) & 0x00ff;

    gpio_pin_l(dev->pin.scs);       // Set SCS=0 to Enable SPI interface
    gpio_pin_mo_h(dev->pin.sdio);   // change SDIO output

    address |= CMD_CTRLW;           // Enable write operation of control registers
    a7139_byte_send(dev, address);       // Assign control register's address by input variable addr

    spi_defdelay();

    a7139_byte_send(dev, dataWord1);     // Assign control register's value by input variable dataByte
    a7139_byte_send(dev, dataWord0);

    gpio_pin_h(dev->pin.scs);       // Set SCS=1 to disable SPI interface.
}

//**********************************************************************************
// 功能描述 : 读寄存器
// 输入参数 : uint8_t address地址
// 返回参数 : 寄存器值
// 说明     :
//**********************************************************************************
static uint16_t a7139_read_reg(struct rf_dev *dev, uint8_t address)
{
    uint16_t tmp;

    gpio_pin_l(dev->pin.scs);       // Set SCS=0 to enable SPI write function.
    gpio_pin_mo_h(dev->pin.sdio);   // change SDIO output

    address |= CMD_CTRLR;       // Enable read operation of control registers.
    a7139_byte_send(dev, address);

    spi_defdelay();

    tmp = a7139_byte_read(dev) << 8;
    tmp = (tmp & 0xff00) | (a7139_byte_read(dev) & 0x00ff);

    gpio_pin_h(dev->pin.scs);       // Set SCS=1 to disable SPI interface.
    gpio_pin_mo_h(dev->pin.sdio);   /* TRXEM_SDIO_MO是后加上去的*/

    return tmp;                 // Return 16 bit value
}

/************************************************************************
 **  A7139_WritePageA
 ************************************************************************/
static void a7139_write_page_a(struct rf_dev *dev, uint8_t address, uint16_t dataWord)
{
    uint16_t tmp;

    tmp = address;
    tmp = ((tmp << 12) | rf_reg_cfg[CRYSTAL_REG]);  /*it's different here*/

    a7139_write_reg(dev, CRYSTAL_REG, tmp);
    a7139_write_reg(dev, PAGEA_REG, dataWord);
}

/************************************************************************
 **  A7139_ReadPageA
 ************************************************************************/
static uint16_t a7139_read_page_a(struct rf_dev *dev, uint8_t address)
{
    uint16_t tmp;

    tmp = address;
    tmp = ((tmp << 12) | rf_reg_cfg[CRYSTAL_REG]);

    a7139_write_reg(dev, CRYSTAL_REG, tmp);
    tmp = a7139_read_reg(dev, PAGEA_REG);

    /*here lost  A7108_WriteReg(CRYSTAL_REG, temp_Crystal_Reg);
      is it because never change CRYSTAL_REG?*/

    return tmp;
}

/************************************************************************
 **  A7139_WritePageB
 ************************************************************************/
static void a7139_write_page_b(struct rf_dev *dev, uint8_t address, uint16_t dataWord)
{
    uint16_t tmp;

    tmp = address;
    tmp = ((tmp << 7) | rf_reg_cfg[CRYSTAL_REG]);

    a7139_write_reg(dev, CRYSTAL_REG, tmp);
    a7139_write_reg(dev, PAGEB_REG, dataWord);
}

/************************************************************************
 **  A7139_ReadPageB
 ************************************************************************/
static uint16_t a7139_read_page_b(struct rf_dev *dev, uint8_t address)
{
    uint16_t tmp;

    tmp = address;
    tmp = ((tmp << 7) | rf_reg_cfg[CRYSTAL_REG]);

    a7139_write_reg(dev, CRYSTAL_REG, tmp);
    tmp = a7139_read_reg(dev, PAGEB_REG);

    return tmp;
}


static void a7139_reg_dump(struct rf_dev *dev)
{
    printk("RF Register Config:\n");
    printk("%-15s%-6s%-10s%-10s\n", "Reg Name", "R/W", "DefValue", "CurrValue");
    printk("%-15s%-6s0x%04X    0x%04X\n", "SYSTEMCLOCK", "R/W", rf_reg_cfg[SYSTEMCLOCK_REG], a7139_read_reg(dev, SYSTEMCLOCK_REG));
    printk("%-15s%-6s0x%04X    -\n", "PLL1", "W", rf_reg_cfg[PLL1_REG]);
    printk("%-15s%-6s0x%04X    -\n", "PLL2", "W", rf_reg_cfg[PLL2_REG]);
    printk("%-15s%-6s0x%04X    -\n", "PLL3", "W", rf_reg_cfg[PLL3_REG]);
    printk("%-15s%-6s0x%04X    -\n", "PLL4", "W", rf_reg_cfg[PLL4_REG]);
    printk("%-15s%-6s0x%04X    -\n", "PLL5", "W", rf_reg_cfg[PLL5_REG]);
    printk("%-15s%-6s0x%04X    -\n", "PLL6", "W", rf_reg_cfg[PLL6_REG]);
    printk("%-15s%-6s0x%04X    -\n", "CRYSTAL", "W", rf_reg_cfg[CRYSTAL_REG]);
    printk("%-15s%-6s0x%04X    -\n", "RX1", "W", rf_reg_cfg[RX1_REG]);
    printk("%-15s%-6s0x%04X    0x%04X\n", "RX2", "R/W", rf_reg_cfg[RX2_REG], a7139_read_reg(dev, RX2_REG));
    printk("%-15s%-6s0x%04X    0x%04X\n", "ADC", "R/W", rf_reg_cfg[ADC_REG], a7139_read_reg(dev, ADC_REG));
    printk("%-15s%-6s0x%04X    -\n", "PINCTRL", "W", rf_reg_cfg[PIN_REG]);
    printk("%-15s%-6s0x%04X    0x%04X\n", "CALIBRATION", "R/W", rf_reg_cfg[CALIBRATION_REG], a7139_read_reg(dev, CALIBRATION_REG));
    printk("%-15s%-6s0x%04X    0x%04X\n", "MODE", "R/W", rf_reg_cfg[MODE_REG], a7139_read_reg(dev, MODE_REG));

    printk("\nRF PageA Register Config:\n");
    printk("%-15s%-6s%-10s%-10s\n", "Reg Name", "R/W", "DefValue", "CurrValue");
    printk("%-15s%-6s0x%04X    -\n", "TX1", "W", rf_reg_cfg_page_a[TX1_PAGEA]);
    printk("%-15s%-6s0x%04X    0x%04X\n", "WOR1", "R/W", rf_reg_cfg_page_a[WOR1_PAGEA], a7139_read_page_a(dev, WOR1_PAGEA));
    printk("%-15s%-6s0x%04X    -\n", "WOR2", "W", rf_reg_cfg_page_a[WOR2_PAGEA]);
    printk("%-15s%-6s0x%04X    0x%04X\n", "RFI", "R/W", rf_reg_cfg_page_a[RFI_PAGEA], a7139_read_page_a(dev, RFI_PAGEA));
    printk("%-15s%-6s0x%04X    -\n", "PM", "W", rf_reg_cfg_page_a[PM_PAGEA]);
    printk("%-15s%-6s0x%04X    -\n", "RTH", "W", rf_reg_cfg_page_a[RTH_PAGEA]);
    printk("%-15s%-6s0x%04X    0x%04X\n", "AGC1", "R/W", rf_reg_cfg_page_a[AGC1_PAGEA], a7139_read_page_a(dev, AGC1_PAGEA));
    printk("%-15s%-6s0x%04X    0x%04X\n", "AGC2", "R/W", rf_reg_cfg_page_a[AGC2_PAGEA], a7139_read_page_a(dev, AGC2_PAGEA));
    printk("%-15s%-6s0x%04X    -\n", "GIO", "W", rf_reg_cfg_page_a[GIO_PAGEA]);
    printk("%-15s%-6s0x%04X    -\n", "CKO", "W", rf_reg_cfg_page_a[CKO_PAGEA]);
    printk("%-15s%-6s0x%04X    0x%04X\n", "VCB", "R/W", rf_reg_cfg_page_a[VCB_PAGEA], a7139_read_page_a(dev, VCB_PAGEA));
    printk("%-15s%-6s0x%04X    0x%04X\n", "CHG1", "R/W", rf_reg_cfg_page_a[CHG1_PAGEA], a7139_read_page_a(dev, CHG1_PAGEA));
    printk("%-15s%-6s0x%04X    0x%04X\n", "CHG2", "R/W", rf_reg_cfg_page_a[CHG2_PAGEA], a7139_read_page_a(dev, CHG2_PAGEA));
    printk("%-15s%-6s0x%04X    -\n", "FIFO", "W", rf_reg_cfg_page_a[FIFO_PAGEA]);
    printk("%-15s%-6s0x%04X    -\n", "CODE", "W", rf_reg_cfg_page_a[CODE_PAGEA]);
    printk("%-15s%-6s0x%04X    0x%04X\n", "WCAL", "R/W", rf_reg_cfg_page_a[WCAL_PAGEA], a7139_read_page_a(dev, WCAL_PAGEA));

    printk("\nRF PageB Register Config:\n");
    printk("%-15s%-6s%-10s%-10s\n", "Reg Name", "R/W", "DefValue", "CurrValue");
    printk("%-15s%-6s0x%04X    0x%04X\n", "TX2", "R/W", rf_reg_cfg_page_b[TX2_PAGEB], a7139_read_page_b(dev, TX2_PAGEB));
    printk("%-15s%-6s0x%04X    -\n", "IF1", "W", rf_reg_cfg_page_b[IF1_PAGEB]);
    printk("%-15s%-6s0x%04X    -\n", "IF2", "W", rf_reg_cfg_page_b[IF2_PAGEB]);
    printk("%-15s%-6s0x%04X    0x%04X\n", "ACK", "R/W", rf_reg_cfg_page_b[ACK_PAGEB], a7139_read_page_b(dev, ACK_PAGEB));
    printk("%-15s%-6s0x%04X    0x%04X\n", "ART", "R/W", rf_reg_cfg_page_b[ART_PAGEB], a7139_read_page_b(dev, ART_PAGEB));

}

/*********************************************************************
 ** A7139_Config
 *********************************************************************/
static int a7139_config(struct rf_dev *dev)
{
    uint8_t i;
    uint16_t tmp;

    for (i = 0; i < 8; i++) {
        a7139_write_reg(dev, i, rf_reg_cfg[i]);
    }

    for (i = 10; i < 16; i++) {
        a7139_write_reg(dev, i, rf_reg_cfg[i]);
    }

    for (i = 0; i < 16; i++) {
        a7139_write_page_a(dev, i, rf_reg_cfg_page_a[i]);
    }

    for (i = 0; i < 5; i++) {
        a7139_write_page_b(dev, i, rf_reg_cfg_page_b[i]);
    }

    // for check
    tmp = a7139_read_reg(dev, SYSTEMCLOCK_REG);
    if (tmp != rf_reg_cfg[SYSTEMCLOCK_REG]) {
        a7139_reg_dump(dev);
        return -EIO;
    }

    return 0;
}

/************************************************************************
 **  WriteID
 ************************************************************************/
static int a7139_write_id(struct rf_dev *dev, uint8_t *id)
{
    uint8_t i;
    int ret = 0;

    gpio_pin_l(dev->pin.scs);
    a7139_byte_send(dev, CMD_ID_W);

    for (i = 0; i < RF_IDSIZE; i++) {
        dev->rf_id[i] = id[i];
        a7139_byte_send(dev, dev->rf_id[i]);
    }

    gpio_pin_h(dev->pin.scs);
    gpio_pin_l(dev->pin.scs);

    a7139_byte_send(dev, CMD_ID_R);

    for (i = 0; i < RF_IDSIZE; i++) {
        ret += (a7139_byte_read(dev) == dev->rf_id[i] ? 0 : -1);
    }

    gpio_pin_h(dev->pin.scs);

    return ret;
}

/************************************************************************
 **  ReadID
 ************************************************************************/
static int a7139_read_id(struct rf_dev *dev, uint8_t *id)
{
    uint8_t i;
    int ret = 0;

    gpio_pin_l(dev->pin.scs);

    a7139_byte_send(dev, CMD_ID_R);

    for (i = 0; i < RF_IDSIZE; i++) {
        ret += ((id[i] = a7139_byte_read(dev)) == dev->rf_id[i] ? 0 : -1);
    }

    gpio_pin_h(dev->pin.scs);

    return ret;
}

////////////////////////////////////////////////////////////////////////////////
// 功能描述 : 模式切换
// 输入参数 : 无
// 返回参数 : 无
// 说明     :
////////////////////////////////////////////////////////////////////////////////
static void a7139_mode_switch(struct rf_dev *dev, A7139_MODE mode)
{
//    unsigned long flags;

//    local_irq_save(flags);
    if (dev->irq > 0) {
        disable_irq_nosync(dev->irq);
    }

    switch (mode)
    {
        case A7139_MODE_SLEEP:
            dev->rf_currmode = A7139_MODE_SLEEP;
            a7139_send_ctrl(dev, CMD_SLEEP_MODE);
            break;

        case A7139_MODE_IDLE:
            dev->rf_currmode = A7139_MODE_IDLE;
            a7139_send_ctrl(dev, CMD_IDLE_MODE);
            break;

        case A7139_MODE_STANDBY:
            dev->rf_currmode = A7139_MODE_STANDBY;
            a7139_send_ctrl(dev, CMD_STANDBY_MODE);
            break;

        case A7139_MODE_PLL:
            dev->rf_currmode = A7139_MODE_PLL;
            a7139_send_ctrl(dev, CMD_PLL_MODE);
            break;

        case A7139_MODE_RX:
            dev->rf_currmode = A7139_MODE_RX;
            a7139_send_ctrl(dev, CMD_STANDBY_MODE);
            if ( in_interrupt() == 0)
            {
                spi_mdelay(1);
            }
            a7139_send_ctrl(dev, CMD_RX_MODE);
            break;

        case A7139_MODE_RXING:
            dev->rf_currmode = A7139_MODE_RXING;
            a7139_send_ctrl(dev, CMD_STANDBY_MODE);
            break;

        case A7139_MODE_TX:
            dev->rf_currmode = A7139_MODE_TX;
            a7139_send_ctrl(dev, CMD_TX_MODE);
            break;

        case A7139_MODE_TXING:
            dev->rf_currmode = A7139_MODE_TXING;
            a7139_send_ctrl(dev, CMD_STANDBY_MODE);
            break;

        case A7139_MODE_DEEPSLEEPT:
            dev->rf_currmode = A7139_MODE_DEEPSLEEPT;
            a7139_send_ctrl(dev, CMD_DEEP_SLEEP_T);
            break;

        case A7139_MODE_DEEPSLEEPP:
            dev->rf_currmode = A7139_MODE_DEEPSLEEPP;
            a7139_send_ctrl(dev, CMD_DEEP_SLEEP_P);
            break;

        default:
            break;
    }

//    local_irq_restore(flags);
    if (dev->irq > 0) {
        enable_irq(dev->irq);
    }
}

/************************************************************************
 **  DataRateSet
 ************************************************************************/
static int a7139_datarate_set(struct rf_dev *dev, A7139_RATE drate)
{
    if (drate >= A7139_RATE_MAX) {
        return -EINVAL;
    }

    a7139_write_reg(dev, SYSTEMCLOCK_REG, rate_cal_tab[drate]);

    dev->rf_datarate = drate;

    return 0;
}

/************************************************************************
 **  FreqSet
 ************************************************************************/
static int a7139_freq_set(struct rf_dev *dev, uint8_t ch)
{

    if (ch >= RF_FREQ_TAB_MAXSIZE) {
        return -EINVAL;
    }

#if 1
    a7139_write_reg(dev, PLL1_REG, freq_cal_tab[2 * ch]);       // setting PLL1
    a7139_write_reg(dev, PLL2_REG, freq_cal_tab[2 * ch + 1]);   // setting PLL2
#else
    uint16_t freq;
    freq = ch * 16 * 4;                                         // per 4*200k one channel
    a7139_write_page_b(IF2_PAGEB, freq);                        // setting for frequency offset
#endif
    dev->rf_freq_ch = ch;

    return 0;
}

/*********************************************************************
 ** A7139_Cal
 *********************************************************************/
static int a7139_cal(struct rf_dev *dev)
{
    uint8_t fbcf; // IF Filter
    uint8_t vbcf; // VCO Current
    uint8_t vccf; // VCO Band
    uint16_t tmp;

    // IF calibration procedure @STB state
    a7139_write_reg(dev, MODE_REG, rf_reg_cfg[MODE_REG] | 0x0802);   // IF Filter & VCO Current Calibration
    do {
        tmp = a7139_read_reg(dev, MODE_REG);
    } while (tmp & 0x0802);

    // for check(IF Filter)
    tmp = a7139_read_reg(dev, CALIBRATION_REG);
    fbcf = (tmp >> 4) & 0x01;
    if (fbcf) {
        return -EIO;
    }

    // for check(VCO Current)
    tmp = a7139_read_page_a(dev, VCB_PAGEA);
    vccf = (tmp >> 4) & 0x01;
    if (vccf) {
        return -EIO;
    }

    // RSSI Calibration procedure @STB state
    a7139_write_reg(dev, ADC_REG, 0x4C00);           // set ADC average=64
    a7139_write_page_a(dev, WOR2_PAGEA, 0xF800);     // set RSSC_D=40us and RS_DLY=80us
    a7139_write_page_a(dev, TX1_PAGEA, rf_reg_cfg_page_a[TX1_PAGEA] | 0xE000);  // set RC_DLY=1.5ms
    a7139_write_reg(dev, MODE_REG, rf_reg_cfg[MODE_REG] | 0x1000);              // RSSI Calibration

    do {
        tmp = a7139_read_reg(dev, MODE_REG);
    } while (tmp & 0x1000);

    a7139_write_reg(dev, ADC_REG, rf_reg_cfg[ADC_REG]);
    a7139_write_page_a(dev, WOR2_PAGEA, rf_reg_cfg_page_a[WOR2_PAGEA]);
    a7139_write_page_a(dev, TX1_PAGEA, rf_reg_cfg_page_a[TX1_PAGEA]);

    // VCO calibration procedure @STB state
    a7139_write_reg(dev, PLL1_REG, freq_cal_tab[0]);
    a7139_write_reg(dev, PLL2_REG, freq_cal_tab[1]);
    a7139_write_reg(dev, MODE_REG, rf_reg_cfg[MODE_REG] | 0x0004);              // VCO Band Calibration
    do {
        tmp = a7139_read_reg(dev, MODE_REG);
    } while(tmp & 0x0004);

    // for check(VCO Band)
    tmp = a7139_read_reg(dev, CALIBRATION_REG);
    vbcf = (tmp >> 8) & 0x01;
    if (vbcf) {
        return -EIO;
    }

    return 0;
}

//**********************************************************************************
// 功能描述 : RF初始化
// 输入参数 : 无
// 返回参数 : 无
// 说明     :
//**********************************************************************************
static int a7139_chip_init(struct rf_dev *dev)
{
    // init io pin
    gpio_pin_mo_h(dev->pin.scs);
    gpio_pin_mo_l(dev->pin.sck);
    gpio_pin_mo_h(dev->pin.sdio);
    gpio_pin_mi(dev->pin.gio1);

    a7139_send_ctrl(dev, CMD_RF_RST);   // reset  chip

    spi_defdelay();

    if (a7139_config(dev)) {            // config A7139 chip
        printk(KERN_ERR "a7139_config error\n");
        return -EIO;
    }

    spi_defdelay();                     // for crystal stabilized

    if (a7139_write_id(dev, dev->rf_id)) {  // write ID code
        printk(KERN_ERR "a7139_write_id error\n");
        return -EIO;
    }

    if (a7139_cal(dev)) {
        printk(KERN_ERR "a7139_cal error\n");
        return -EIO;                    // IF and VCO calibration
    }

    spi_defdelay();

    a7139_freq_set(dev, dev->rf_freq_ch);
    //a7139_datarate_set(dev, dev->rf_datarate);

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// 功能描述 : 发送数据
// 输入参数 : uint8_t *txBuffer:发送数据存储数组首地址,
//            uint8_t size：发送数据长度
// 返回参数 : 无
// 说明     :
////////////////////////////////////////////////////////////////////////////////
static void a7139_send_packet(struct rf_dev *dev, uint8_t *txBuffer, uint8_t size)
{
    uint8_t i;
    unsigned long flags;

    //a7139_mode_switch(dev, A7139_MODE_STANDBY);     // enter standby mode

    local_irq_save(flags);

    spi_defdelay();
    a7139_send_ctrl(dev, CMD_TFR);                  // TX FIFO address pointer reset

    gpio_pin_l(dev->pin.scs);
    a7139_byte_send(dev, CMD_DATAW);                // TX FIFO write command

    for (i = 0; i < size; i++) {
        a7139_byte_send(dev, txBuffer[i]);
    }
    gpio_pin_h(dev->pin.scs);

    local_irq_restore(flags);

    a7139_mode_switch(dev, A7139_MODE_TX);
}

///*********************************************************************
//** A7139_WriteFIFO
//*********************************************************************/
//void LSD_RF_WriteFIFO(void)
//{
//    uint8_t i;
//
//    a7139_send_ctrl(CMD_TFR);        //TX FIFO address pointer reset
//    spi_scs_l;
//    a7139_byte_send(CMD_DATAW);    //TX FIFO write command
//    for(i=0; i <64; i++)
//    a7139_byte_send(PN9_Tab[i]);
//    spi_scs_h;
//}

////////////////////////////////////////////////////////////////////////////////
// 功能描述 : 接收数据包
// 输入参数 : uint8_t *buf,返回数据存储首地址。 uint8_t len 读的数据长度
// 返回参数 : 无
// 说明     :
////////////////////////////////////////////////////////////////////////////////
static uint8_t a7139_receive_packet(struct rf_dev *dev, uint8_t *buf, uint8_t len)
{
    uint8_t i;
    unsigned long flags;

    local_irq_save(flags);

    a7139_send_ctrl(dev, CMD_RFR);      // RX FIFO address pointer reset

    gpio_pin_l(dev->pin.scs);

    a7139_byte_send(dev, CMD_DATAR);    // RX FIFO read command

    for (i = 0; i < len; i++) {
        buf[i] = a7139_byte_read(dev);
        /* printk("read 0x%x\n", buf[i]); */
    }

    gpio_pin_h(dev->pin.scs);

    local_irq_restore(flags);
    /*
    for (i = 0; i < 64; i++)
    {
        recv = tmpbuf[i];
        tmp = recv ^ PN9_Tab[i];
        if (tmp != 0)
        {
            Err_ByteCnt++;
            Err_BitCnt += (bit_count_tab[tmp>>4] + bit_count_tab[tmp & 0x0F]);
        }
    }
    */

    return i;
}


#if 0
/*********************************************************************
 ** RC Oscillator Calibration
 *********************************************************************/
static void a7139_rcosc_cal(struct rf_dev *dev)
{
    uint16_t tmp;

    a7139_write_page_a(dev, WOR2_PAGEA, rf_reg_cfg_page_a[WOR2_PAGEA] | 0x0010);        //enable RC OSC

    while(1) {
        a7139_write_page_a(dev, WCAL_PAGEA, rf_reg_cfg_page_a[WCAL_PAGEA] | 0x0001);    //set ENCAL=1 to start RC OSC CAL
        do {
            tmp = a7139_read_page_a(dev, WCAL_PAGEA);
        } while(tmp & 0x0001);

        tmp = (a7139_read_page_a(dev, WCAL_PAGEA) & 0x03FF);         // read NUMLH[8:0]
        tmp >>= 1;
        if ((tmp > 186) && (tmp < 198))                         // NUMLH[8:0]~192
        {
            break;
        }
    }
}

/*********************************************************************
 ** WOR_enable_by_preamble
 *********************************************************************/
void WOR_enable_by_preamble(void)
{
    StrobeCMD(CMD_STANDBY_MODE);

    A7139_WritePageA(GIO_PAGEA, 0x004D);    //GIO1=PMDO, GIO2=WTR

    //Real WOR Active Period = (WOR_AC[5:0]+1) x 244us  X'TAL and Regulator Settling Time
    //Note : Be aware that X tal settling time requirement includes initial tolerance,
    //       temperature drift, aging and crystal loading.
    A7139_WritePageA(WOR1_PAGEA, 0x8005);    //setup WOR Sleep time and Rx time

    RCOSC_Cal();         //RC Oscillator Calibration

    A7139_WritePageA(WOR2_PAGEA, A7139Config_PageA[WOR2_PAGEA] | 0x0030);    //enable RC OSC & WOR by preamble

    A7139_WriteReg(MODE_REG, A7139Config[MODE_REG] | 0x0200);        //WORE=1 to enable WOR function

    while(GIO1==0);        //Stay in WOR mode until receiving preamble(preamble ok)

}

/*********************************************************************
 ** WOR_enable_by_sync
 *********************************************************************/
void WOR_enable_by_sync(void)
{
    StrobeCMD(CMD_STANDBY_MODE);

    A7139_WritePageA(GIO_PAGEA, 0x0045);    //GIO1=FSYNC, GIO2=WTR

    //Real WOR Active Period = (WOR_AC[5:0]+1) x 244us  X'TAL and Regulator Settling Time
    //Note : Be aware that Xˇtal settling time requirement includes initial tolerance,
    //       temperature drift, aging and crystal loading.
    A7139_WritePageA(WOR1_PAGEA, 0x8005);    //setup WOR Sleep time and Rx time

    RCOSC_Cal();         //RC Oscillator Calibration

    A7139_WritePageA(WOR2_PAGEA, A7139Config_PageA[WOR2_PAGEA] | 0x0010);    //enable RC OSC & WOR by sync

    A7139_WriteReg(MODE_REG, A7139Config[MODE_REG] | 0x0200);        //WORE=1 to enable WOR function

    while(GIO1==0);        //Stay in WOR mode until receiving ID code(sync ok)

}

/*********************************************************************
 ** WOR_enable_by_carrier
 *********************************************************************/
void WOR_enable_by_carrier(void)
{
    StrobeCMD(CMD_STANDBY_MODE);

    A7139_WritePageA(GIO_PAGEA, 0x0049);    //GIO1=CD, GIO2=WTR

    //Real WOR Active Period = (WOR_AC[5:0]+1) x 244us  X'TAL and Regulator Settling Time
    //Note : Be aware that Xˇtal settling time requirement includes initial tolerance,
    //       temperature drift, aging and crystal loading.
    A7139_WritePageA(WOR1_PAGEA, 0x8005);    //setup WOR Sleep time and Rx time

    RCOSC_Cal();         //RC Oscillator Calibration

    A7139_WritePageA(WOR2_PAGEA, A7139Config_PageA[WOR2_PAGEA] | 0x0410);    //enable RC OSC & WOR by carrier
    A7139_WriteReg(ADC_REG, A7139Config[ADC_REG] | 0x8096);            //ARSSI=1, RTH=150
    A7139_WritePageA(RFI_PAGEA, A7139Config_PageA[RFI_PAGEA] | 0x6000);    //RSSI Carrier Detect plus In-band Carrier Detect
    A7139_WritePageB(ACK_PAGEB, A7139Config_PageB[ACK_PAGEB] | 0x0200);    //CDRS=[01]
    A7139_WritePageA(VCB_PAGEA, A7139Config_PageA[VCB_PAGEA] | 0x4000);    //CDTM=[01]

    A7139_WriteReg(MODE_REG, A7139Config[MODE_REG] | 0x0200);        //WORE=1 to enable WOR function

    while(GIO1==0);        //Stay in WOR mode until carrier signal strength is greater than the value set by RTH[7:0](carrier ok)

}

/*********************************************************************
 ** WOT_enable
 *********************************************************************/
void WOT_enable(void)
{
    StrobeCMD(CMD_STANDBY_MODE);

    A7139_WritePageA(GIO_PAGEA, 0x0045);    //GIO1=FSYNC, GIO2=WTR

    //Real WOR Active Period = (WOR_AC[5:0]+1) x 244us  X'TAL and Regulator Settling Time
    //Note : Be aware that Xˇtal settling time requirement includes initial tolerance,
    //       temperature drift, aging and crystal loading.
    A7139_WritePageA(WOR1_PAGEA, 0x8005);    //setup WOR Sleep time and Rx time

    RCOSC_Cal();             //RC Oscillator Calibration

    A7139_WriteFIFO();        //write data to TX FIFO

    A7139_WriteReg(PIN_REG, A7139Config[PIN_REG] | 0x0400);        //WMODE=1 select WOT function

    A7139_WriteReg(MODE_REG, A7139Config[MODE_REG] | 0x0200);    //WORE=1 to enable WOR function

    while(1);

}

/*********************************************************************
 ** TWOR_enable
 *********************************************************************/
void TWOR_enable(void)
{
    StrobeCMD(CMD_STANDBY_MODE);

    A7139_WritePageA(GIO_PAGEA, 0x0051);    //GIO1=TWOR, GIO2=WTR

    //Real WOR Active Period = (WOR_AC[5:0]+1) x 244us  X'TAL and Regulator Settling Time
    //Note : Be aware that Xˇtal settling time requirement includes initial tolerance,
    //       temperature drift, aging and crystal loading.
    A7139_WritePageA(WOR1_PAGEA, 0x8005);    //setup WOR Sleep time and Rx time

    RCOSC_Cal();             //RC Oscillator Calibration

    A7139_WritePageA(WOR2_PAGEA, A7139Config_PageA[WOR2_PAGEA] | 0x0014);    //start TWOR by WOR_AC
    //A7139_WritePageA(WOR2_PAGEA, A7139Config_PageA[WOR2_PAGEA] | 0x001C);    //start TWOR by WOR_SL

    StrobeCMD(CMD_SLEEP);    //entry sleep mode

    while(1);

}

/*********************************************************************
 ** RSSI_measurement
 *********************************************************************/
void RSSI_measurement(void)
{
    uint16_t tmp;

    StrobeCMD(CMD_STANDBY_MODE);

    A7139_WritePageA(GIO_PAGEA, 0x0045);            //GIO1=FSYNC, GIO2=WTR

    A7139_WriteReg(ADC_REG, A7139Config[ADC_REG] | 0x8000);    //ARSSI=1

    StrobeCMD(CMD_RX_MODE);    //entry RX mode
    Delay10us(1);

    while(GIO1==0)        //Stay in WOR mode until receiving ID code(sync ok)
    {
        tmp = (A7139_ReadReg(ADC_REG) & 0x00FF);    //read RSSI value(environment RSSI)
    }
    tmp = (A7139_ReadReg(ADC_REG) & 0x00FF);        //read RSSI value(wanted signal RSSI)

}

/*********************************************************************
 ** FIFO_extension_Infinite_TX
 *********************************************************************/
void FIFO_extension_Infinite_TX(void)
{
    uint8_t i, n;
    uint16_t j;

    //for Infinite : FEP[13:0]=remainder,  5 < FEP[13:0] < 63
    A7139_WritePageA(VCB_PAGEA, 0x0004);
    A7139_WritePageA(FIFO_PAGEA, 0x0006);

    A7139_WritePageA(GIO_PAGEA, 0x0075);    //GIO1=FPF, GIO2=WTR
    A7139_WritePageA(CKO_PAGEA, 0xD283);    //CKO=DCK

    A7139_WriteReg(PIN_REG, 0x0A00);    //set INFS=1, infinite length

    n=0;

    SCS=0;
    ByteSend(CMD_DATAW);                //write 1st TX FIFO 64bytes
    for(i=0; i<64; i++)
    {
        ByteSend(n);
        if(n==255)
            n=0;
        else
            n++;
    }
    SCS=1;

    StrobeCMD(CMD_TX);            //entry TX mode

    for(j=0; j<500; j++)
    {
        while(GIO1==0);            //wait FPF go high
        while(GIO1==1);            //wait FPF go low

        SCS=0;
        ByteSend(CMD_DATAW);    //write TX FIFO 64bytes
        for(i=0; i <64; i++)
        {
            ByteSend(n);
            if(n==255)
                n=0;
            else
                n++;
        }
        SCS=1;
    }

    //remainder : FEP[13:0]=6, FIFO Length=6+1=7
    while(GIO1==0);                //wait last FPF go high
    while(GIO1==1);                //wait last FPF go low

    SCS=0;
    ByteSend(CMD_DATAW);        //write TX FIFO 7bytes
    for(i=0; i<7; i++)
    {
        ByteSend(n);
        if(n==255)
            n=0;
        else
            n++;
    }
    SCS=1;

    A7139_WriteReg(PIN_REG, 0x0800);    //disable INFS

    while(GIO2);    //wait transmit completed
}

/*********************************************************************
 ** FIFO_extension_Infinite_RX
 *********************************************************************/
void FIFO_extension_Infinite_RX(void)
{
    uint8_t i, n;
    uint8_t recv;
    uint8_t tmp;
    uint16_t j;

    //for Infinite : FEP[13:0]=remainder,  5 < FEP[13:0] < 63
    A7139_WritePageA(VCB_PAGEA, 0x0004);
    A7139_WritePageA(FIFO_PAGEA, 0x0006);

    A7139_WritePageA(GIO_PAGEA, 0x0075);    //GIO1=FPF, GIO2=WTR
    A7139_WritePageA(CKO_PAGEA, 0xD28B);    //CKO=RCK

    A7139_WriteReg(PIN_REG, 0x0A00);        //set INFS=1, infinite length

    Err_ByteCnt=0;
    Err_BitCnt=0;
    n=0;

    StrobeCMD(CMD_RX_MODE);            //entry RX mode

    for(j=0; j<501 ; j++)
    {
        while(GIO1==0);            //wait FPF go high
        while(GIO1==1);            //wait FPF go low

        SCS=0;
        ByteSend(CMD_DATAR);    //read RX FIFO 64bytes
        for(i=0; i<64; i++)
            tmpbuf[i]=ByteRead();
        SCS=1;

        //for check
        for(i=0; i<64; i++)
        {
            recv = tmpbuf[i];
            tmp = recv ^ n;
            if(tmp!=0)
            {
                Err_ByteCnt++;
                Err_BitCnt += (bit_count_tab[tmp>>4] + bit_count_tab[tmp & 0x0F]);
            }

            if(n==255)
                n=0;
            else
                n++;
        }
    }

    A7139_WriteReg(PIN_REG, 0x0800);    //disable INFS

    while(GIO2);    //wait receive completed

    //remainder : FEP[13:0]=6, FIFO Length=6+1=7
    SCS=0;
    ByteSend(CMD_DATAR);        //read RX FIFO 7bytes
    for(i=0; i<7; i++)
        tmpbuf[i]=ByteRead();
    SCS=1;

    //for check
    for(i=0; i<7; i++)
    {
        recv = tmpbuf[i];
        tmp = recv ^ n;
        if(tmp!=0)
        {
            Err_ByteCnt++;
            Err_BitCnt += (bit_count_tab[tmp>>4] + bit_count_tab[tmp & 0x0F]);
        }

        if(n==255)
            n=0;
        else
            n++;
    }
}

/*********************************************************************
 ** Auto_Resend
 *********************************************************************/
void Auto_Resend(void)
{
    StrobeCMD(CMD_STANDBY_MODE);

    A7139_WritePageA(GIO_PAGEA, 0x0071);    //GIO1=VPOAK, GIO2=WTR
    A7139_WritePageA(CKO_PAGEA, 0xD283);    //CKO=DCK

    A7139_WritePageB(ACK_PAGEB, 0x003F);    //enable Auto Resend, by event, fixed, ARC=15
    A7139_WritePageB(ART_PAGEB, 0x001E);    //RND=0, ARD=30
    //ACK format = Preamble + ID = 8bytes
    //data rate=10kbps, Transmission Time=(1/10k)*8*8=6.4ms
    //so set ARD=30, RX time=30*250us=7.75ms

    while(1)
    {
        A7139_WriteFIFO();
        StrobeCMD(CMD_TX);        //entry TX mode
        Delay10us(1);
        while(GIO2);            //wait transmit completed

        //check VPOAK
        if(GIO1)
        {
            //ACK ok
        }
        else
        {
            //NO ACK
        }

        StrobeCMD(CMD_STANDBY_MODE);    //clear VPOAK(GIO1 signal)

        Delay10ms(1);
    }
}

/*********************************************************************
 ** Auto_ACK
 *********************************************************************/
void Auto_ACK(void)
{
    A7139_WritePageA(GIO_PAGEA, 0x0045);    //GIO1=FSYNC, GIO2=WTR
    A7139_WritePageA(CKO_PAGEA, 0xD28B);    //CKO=RCK

    A7139_WritePageB(ACK_PAGEB, 0x0001);    //enable Auto ACK

    while(1)
    {
        StrobeCMD(CMD_RX_MODE);        //entry RX mode
        Delay10us(1);
        while(GIO2);            //wait receive completed

        RxPacket();
    }
}

/*********************************************************************
 ** entry_deep_sleep_mode
 *********************************************************************/
void entry_deep_sleep_mode(void)
{
    StrobeCMD(CMD_RF_RST);            //RF reset

    A7139_WriteReg(PIN_REG, 0x0800);    //SCMDS=1
    A7139_WritePageA(PM_PAGEA, 0x0170);    //QDS=1
    StrobeCMD(CMD_SLEEP);            //entry sleep mode
    Delay100us(6);                //for VDD_A shutdown, C load=0.1uF
    StrobeCMD(CMD_DEEP_SLEEP);        //entry deep sleep mode
    Delay100us(2);                //for VDD_D shutdown, C load=0.1uF
}

/*********************************************************************
 ** wake_up_from_deep_sleep_mode
 *********************************************************************/
void wake_up_from_deep_sleep_mode(void)
{
    StrobeCMD(CMD_STANDBY_MODE);    //wake up (any Strobe CMD)
    Delay100us(3);        //for VDD_D stabilized
    InitRF();
}
#endif

static int a7139_pin_init(struct rf_dev *dev)
{
    int result;

    result = gpio_request_one(dev->pin.scs, GPIOF_OUT_INIT_HIGH, DEVICE_NAME);
    if (result) {
        goto err;
    }
    result = gpio_request_one(dev->pin.sck, GPIOF_OUT_INIT_LOW, DEVICE_NAME);
    if (result) {
        goto err;
    }
    result = gpio_request_one(dev->pin.sdio, GPIOF_IN, DEVICE_NAME);
    if (result) {
        goto err;
    }
    result = gpio_request_one(dev->pin.gio1, GPIOF_IN, DEVICE_NAME);
    if (result) {
        goto err;
    }

    return 0;

err:
    gpio_free(dev->pin.scs);
    gpio_free(dev->pin.sck);
    gpio_free(dev->pin.sdio);
    gpio_free(dev->pin.gio1);

    return -EBUSY;
}

static void a7139_pin_free(struct rf_dev *dev)
{
    gpio_free(dev->pin.scs);
    gpio_free(dev->pin.sck);
    gpio_free(dev->pin.sdio);
    gpio_free(dev->pin.gio1);
}

void a7139_readwork_func(struct work_struct *work)
{
    struct rf_dev *dev;

    dev = container_of(work, struct rf_dev, work);

    down(&dev->sem);

    if (dev->rf_currmode == A7139_MODE_RXING) {
        memset((void*)dev->rxbuf, 0, RF_BUFSIZE);
        dev->rx_len = a7139_receive_packet(dev, dev->rxbuf, RF_BUFSIZE);
        dev->rf_rxevt = 1;
        wake_up_interruptible(&dev->r_wait);
        a7139_mode_switch(dev, A7139_MODE_RX);
    }

    up(&dev->sem);
}

void a7139_writework_func(struct work_struct *work)
{
    struct rf_dev *dev;

    dev = container_of(work, struct rf_dev, work);

    down(&dev->sem);

    if (dev->rf_currmode == A7139_MODE_TXING) {
        a7139_send_packet(dev, dev->txbuf, dev->tx_len);
        memset((void*)dev->txbuf, 0, RF_BUFSIZE);
        dev->rf_txevt = 0;
        dev->tx_len = 0;
    }

    up(&dev->sem);
}

static irqreturn_t a7139_interrupt(int irq, void *dev_id)
{
    struct rf_dev *dev = (struct rf_dev *)dev_id;
    uint16_t status;

    debugf("%s a7139_interrupt: rf_currmode:%d\n", dev->name_alias, dev->rf_currmode);

    if (dev->rf_currmode == A7139_MODE_RX) {

        /* check the hardware crc correct */
        status = a7139_read_reg(dev, MODE_REG);

        if (status & 0x0200) {
            printk(KERN_ERR "%s read crc error\n", dev->name_alias);
            a7139_send_ctrl(dev, CMD_RFR);      // RX FIFO address pointer reset
            a7139_reg_dump(dev);

            a7139_mode_switch(dev, A7139_MODE_RX);
        }
        else {
            a7139_mode_switch(dev, A7139_MODE_RXING);

            INIT_WORK(&dev->work, a7139_readwork_func);
            queue_work(dev->work_queue, &dev->work);
        }
    }
    else if (dev->rf_currmode == A7139_MODE_TX) {
        dev->rf_txevt = 1;
        wake_up_interruptible(&dev->w_wait);

        a7139_mode_switch(dev, A7139_MODE_RX);
    }

    return IRQ_RETVAL(IRQ_HANDLED);
}

//**********************************************************************************
// 功能描述 : RF device初始化
// 输入参数 : 无
// 返回参数 : 无
// 说明     :
//**********************************************************************************
static int a7139_dev_init(struct rf_dev *dev)
{
    dev->rf_datarate = RF_DEF_RATE;
    dev->rf_freq_ch = RF_DEF_FREQ_CH;
    dev->rf_id[0] = RF_DEF_ID_D0;
    dev->rf_id[1] = RF_DEF_ID_D1;
    dev->rx_len = 0;
    dev->tx_len = 0;
    dev->rf_rxevt = 0;
    dev->rf_txevt = 1;

    return 0;
}

static ssize_t a7139_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos)
{
    struct rf_dev *dev = filp->private_data;
    ssize_t ret = 0;
    ssize_t len;

    if (dev->rx_len == 0) {
        wait_event_interruptible(dev->r_wait, dev->rf_rxevt);
    }

    debugf("a7139_read\n");

    down(&dev->sem);

    len = (count > dev->rx_len ? dev->rx_len : count);

    if (copy_to_user(buf, (void*)dev->rxbuf, len)) {
        printk(KERN_ERR "%s: copy_to_user error\n", dev->name_alias);
        ret = -EFAULT;
    } else {
        //printk("read %d bytes from %s\n", dev->rx_len, dev->name_alias);
        ret = len;
    }

    dev->rf_rxevt = 0;
    dev->rx_len = 0;

    up(&dev->sem);

    return ret;
}

static ssize_t a7139_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
    struct rf_dev *dev = filp->private_data;
    ssize_t len;
    int err;

    err = wait_event_interruptible_timeout(dev->w_wait, dev->rf_txevt, msecs_to_jiffies(DEV_WRITE_TIMEOUT));

    debugf("a7139_write\n");

    down(&dev->sem);
    if (err == 0)
    {
        debugf("a7139_write wait_event_interruptible_timeout\n");
        dev->rf_txevt = 1;
        up(&dev->sem);

        return -EAGAIN;
    }
    else if (dev->rf_currmode != A7139_MODE_RX)
    {
        debugf("a7139_write rf_currmode is not A7139_MODE_RX\n");
        up(&dev->sem);

        return -EAGAIN;
    }

    a7139_mode_switch(dev, A7139_MODE_TXING);

    len = (count > RF_BUFSIZE ? RF_BUFSIZE : count);

    if (copy_from_user(dev->txbuf, buf, len)) {
        printk(KERN_ERR "%s: copy_from_user error\n", dev->name_alias);
        up(&dev->sem);

        return -EFAULT;
    }
    dev->tx_len = len;

    INIT_WORK(&dev->work, a7139_writework_func);
    queue_work(dev->work_queue, &dev->work);

    up(&dev->sem);

    return len;
}

static unsigned int a7139_poll(struct file *filp, struct poll_table_struct *wait)
{
    unsigned int mask = 0;
    struct rf_dev *dev = filp->private_data;

    debugf("a7139_poll: rxevt:%d, txevt:%d\n", dev->rf_rxevt, dev->rf_txevt);

    down(&dev->sem);

    poll_wait(filp, &dev->r_wait, wait);
    poll_wait(filp, &dev->w_wait, wait);

    if (dev->rf_rxevt) {
        mask |= POLLIN | POLLRDNORM;
    }
    if (dev->rf_txevt && dev->rf_currmode == A7139_MODE_RX) {
        mask |= POLLOUT | POLLWRNORM;
    }

    up(&dev->sem);

    return mask;
}

static long a7139_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct rf_dev *dev = filp->private_data;
    uint8_t id[RF_IDSIZE];
    uint8_t freq_ch;
    int datarate;
    int ret = 0;

    debugf("a7139_ioctl, cmd=0x%x\n", cmd);

    /* 检测命令的有效性 */
    if (_IOC_TYPE(cmd) != A7139_IOC_MAGIC) {
        return -EINVAL;
    }

    if (_IOC_NR(cmd) > A7139_IOC_MAXNR) {
        return -EINVAL;
    }

    /* 根据命令类型，检测参数空间是否可以访问 */
    if (_IOC_DIR(cmd) & _IOC_READ) {
        ret = !access_ok(VERIFY_WRITE, (void*)arg, _IOC_SIZE(cmd));
    } else if (_IOC_DIR(cmd)&_IOC_WRITE) {
        ret = !access_ok(VERIFY_READ, (void*)arg, _IOC_SIZE(cmd));
    }
    if (ret) {
        return -EACCES;
    }

    ret = 0;
    down(&dev->sem);

    a7139_mode_switch(dev, A7139_MODE_STANDBY);
    spi_mdelay(1);

    switch (cmd) {
        case A7139_IOC_DUMP:
            a7139_reg_dump(dev);
            break;

        case A7139_IOC_RESET:
            cancel_work_sync(&dev->work);
            flush_workqueue(dev->work_queue);
            if (a7139_dev_init(dev)) {
                printk(KERN_ERR "%s:a7139 dev init error!\n", dev->name_alias);

                return -EBUSY;
            }

            if (a7139_chip_init(dev)) {
                printk(KERN_ERR "%s:a7139 chip init error!\n", dev->name_alias);

                return -ENODEV;
            }
            break;

        case A7139_IOC_SETID:
            if (copy_from_user(id, (void __user *)arg, RF_IDSIZE)) {
                ret = -EFAULT;
                goto out;
            }

            if (a7139_write_id(dev, id)) {
                ret = -EINVAL;
                goto out;
            }
            break;

        case A7139_IOC_GETID:
            if (a7139_read_id(dev, id)) {
                ret = -EBUSY;
                goto out;
            }

            if (copy_to_user((void __user *)arg, id, RF_IDSIZE)) {
                ret = -EFAULT;
            }

            break;

        case A7139_IOC_SETFREQ:
            if (get_user(freq_ch, (uint8_t __user *)arg)) {
                ret = -EFAULT;
                goto out;
            }

            if (a7139_freq_set(dev, freq_ch)) {
                ret = -EINVAL;
                goto out;
            }
            break;

        case A7139_IOC_GETFREQ:
            if (put_user(dev->rf_freq_ch, (uint8_t __user *)arg)) {
                ret = -EFAULT;
            }
            break;

        case A7139_IOC_SETRATE:
            if (get_user(datarate, (uint8_t __user *)arg)) {
                ret = -EFAULT;
                goto out;
            }

            if (a7139_datarate_set(dev, (A7139_RATE)datarate)) {
                ret = -EINVAL;
                goto out;
            }
            break;

        case A7139_IOC_GETRATE:
            if (put_user(dev->rf_datarate, (uint8_t __user *)arg)) {
                ret = -EFAULT;
            }
            break;

        default:
            ret = -EINVAL;
            break;
    }

out:
    a7139_mode_switch(dev, A7139_MODE_RX);

    up(&dev->sem);

    return ret;
}

static int a7139_open(struct inode *inode, struct file *filp)
{
    struct rf_dev *dev;
    int result;

    dev = container_of(inode->i_cdev, struct rf_dev, cdev);
    filp->private_data = dev;

    /* must use block mode open */
    if (filp->f_flags & O_NONBLOCK) {
        return -ENOTBLK;
    }

    if (dev->opencount) {
        return -EBUSY;
    }

    dev->opencount++;

    if (a7139_dev_init(dev)) {
        printk(KERN_ERR "%s:a7139 dev init error!\n", dev->name_alias);
        dev->opencount--;
        return -EBUSY;
    }

    if (a7139_chip_init(dev)) {
        printk(KERN_ERR "%s:a7139 chip init error!\n", dev->name_alias);
        dev->opencount--;
        return -ENODEV;
    }

    a7139_mode_switch(dev, A7139_MODE_RX);

    dev->irq = gpio_to_irq(dev->pin.gio1);
    if (dev->irq < 0) {
        printk(KERN_ERR "%s: open - can't get irq no, errno:%d\n", dev->name_alias, dev->irq);
        dev->opencount--;
        return -EINVAL;
    }
	 
    result = request_irq(dev->irq, a7139_interrupt, IRQF_TRIGGER_FALLING | IRQF_DISABLED,
            dev->name_alias, (void *)dev);
    if (result) {
        printk(KERN_ERR "%s: open - can't get irq\n", dev->name_alias);
        dev->opencount--;
        return result;
    }

    debugf("%s opened.\n", dev->name_alias);

    return 0;
}

static int a7139_release(struct inode *inode, struct file *filp)
{
    struct rf_dev *dev = filp->private_data;

    dev->opencount--;

    if (dev->irq > 0) {
        free_irq(dev->irq, (void *)dev);
        dev->irq = -1;
    }

    a7139_mode_switch(dev, A7139_MODE_STANDBY);

    debugf("%s closed.\n", dev->name_alias);

    return 0;
}

static struct file_operations a7139_fops = {
    .owner              = THIS_MODULE,
    .open               = a7139_open,
    .release            = a7139_release,
    .read               = a7139_read,
    .write              = a7139_write,
    .poll               = a7139_poll,
    .unlocked_ioctl     = a7139_ioctl,
};

static int a7139_setup_cdev(struct rf_dev *devs, int dev_nr)
{
    int devno;
    int index, result;
    struct rf_dev *dev;

    devno = MKDEV(a7139_major, 0);
    if (a7139_major) {
        result = register_chrdev_region(devno, dev_nr, DEVICE_NAME);
    } else {
        result = alloc_chrdev_region(&devno, 0, dev_nr, DEVICE_NAME);
        a7139_major = MAJOR(devno);
    }

    if (result < 0) {
        printk(KERN_ERR "alloc chrdev error %d\n", result);
        goto out2;
    }

    dev_class = class_create(THIS_MODULE, DEVICE_NAME);
    if (IS_ERR(dev_class)) {
        printk(KERN_ERR "Error in creating class.\n");
        result = -EFAULT;
        goto out;
    }

    for (index = 0; index < dev_nr; index++) {
        dev = &devs[index];

        /* init a7139 pin */
        if (a7139_pin_init(dev)) {
            printk(KERN_ERR "Init %s pins error\n", dev->name_alias);
            continue;
        }

        devno = MKDEV(a7139_major, index);
        cdev_init(&dev->cdev, &a7139_fops);
        dev->cdev.owner = THIS_MODULE;
        dev->cdev.ops = &a7139_fops;
        result = cdev_add(&dev->cdev, devno, 1);
        if (result) {
            printk(KERN_ERR "Error %d adding %s\n", result, dev->name_alias);
            goto out;
        }

        init_waitqueue_head(&dev->r_wait);
        init_waitqueue_head(&dev->w_wait);

        device_create(dev_class, NULL, devno, NULL, dev->name_alias);

        sema_init(&dev->sem, 1);

        dev->work_queue = create_singlethread_workqueue(dev->name_alias);
        if (!dev->work_queue)
        {
            printk(KERN_ERR "create workqueue fail!\n");
            goto out;
        }
    }

    return 0;

out:
    class_destroy(dev_class);
out2:
    unregister_chrdev_region(devno, dev_nr);
    return result;
}

static int __init a7139_init(void)
{
    printk(KERN_INFO "%s driver init. Version:%s\n", DEVICE_NAME, VERSION);

    if (a7139_setup_cdev(devs, ARRAY_SIZE(devs))) {
        return -EFAULT;
    }

    printk(KERN_INFO "%s driver init successfully.\n", DEVICE_NAME);

    return 0;
}

static void __exit a7139_exit(void)
{
    int index;
    struct rf_dev *dev;

    printk(KERN_INFO "%s exit\n", DEVICE_NAME);

    for (index = 0; index < ARRAY_SIZE(devs); index++) {
        dev = &devs[index];
        if(&dev->cdev)
        {
            cdev_del(&dev->cdev);
            device_destroy(dev_class, MKDEV(a7139_major, index));
        }

        a7139_pin_free(dev);

        destroy_workqueue(dev->work_queue);
    }

    unregister_chrdev_region(MKDEV(a7139_major, 0), ARRAY_SIZE(devs));
    class_destroy(dev_class);
}

module_init(a7139_init);
module_exit(a7139_exit);

MODULE_AUTHOR("redfox.qu@qq.com, yaobyron@gmail.com");
MODULE_DESCRIPTION("a7139 kernel driver for am335x");
MODULE_LICENSE("GPL");

