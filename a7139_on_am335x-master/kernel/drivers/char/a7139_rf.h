//*****************************************************************************
//
// a7139.h  The 433Mhz module chip a7139 linux driver on am335x
//
// author:  redfox.qu@qq.com
// update:  2013/12/18
// version: v1.0
//
//*****************************************************************************

/******************************************************************************
 * TODO:
 *  2013/12/18
 *
 * History:
 *  2013/12/18      create this file
 ******************************************************************************/

#ifndef __A7139_RF_H__
#define __A7139_RF_H__

#define SYSTEMCLOCK_REG     0x00
#define PLL1_REG            0x01
#define PLL2_REG            0x02
#define PLL3_REG            0x03
#define PLL4_REG            0x04
#define PLL5_REG            0x05
#define PLL6_REG            0x06
#define CRYSTAL_REG         0x07
#define PAGEA_REG           0x08
#define PAGEB_REG           0x09
#define RX1_REG             0x0A
#define RX2_REG             0x0B
#define ADC_REG             0x0C
#define PIN_REG             0x0D
#define CALIBRATION_REG     0x0E
#define MODE_REG            0x0F

#define TX1_PAGEA           0x00
#define WOR1_PAGEA          0x01
#define WOR2_PAGEA          0x02
#define RFI_PAGEA           0x03
#define PM_PAGEA            0x04
#define RTH_PAGEA           0x05
#define AGC1_PAGEA          0x06
#define AGC2_PAGEA          0x07
#define GIO_PAGEA           0x08
#define CKO_PAGEA           0x09
#define VCB_PAGEA           0x0A
#define CHG1_PAGEA          0x0B
#define CHG2_PAGEA          0x0C
#define FIFO_PAGEA          0x0D
#define CODE_PAGEA          0x0E
#define WCAL_PAGEA          0x0F

#define TX2_PAGEB           0x00
#define IF1_PAGEB           0x01
#define IF2_PAGEB           0x02
#define ACK_PAGEB           0x03
#define ART_PAGEB           0x04


/*********************************************************************
 **  Constant Declaration
 *********************************************************************/
#define CMD_CTRLW           0x00    // 000x,xxxx    control register write
#define CMD_CTRLR           0x80    // 100x,xxxx    control register read
#define CMD_ID_W            0x20    // 001x,xxxx    ID write
#define CMD_ID_R            0xA0    // 101x,xxxx    ID Read
#define CMD_DATAW           0x40    // 010x,xxxx    TX FIFO Write
#define CMD_DATAR           0xC0    // 110x,xxxx    RX FIFO Read
#define CMD_TFR             0x60    // 0110,xxxx    TX FIFO reset
#define CMD_RFR             0xE0    // 1110,xxxx    RX FIFO reset
#define CMD_RF_RST          0x70    // x111,xxxx    RF reset

#define CMD_SLEEP_MODE      0x10    // 0001,0000    Sleep mode
#define CMD_IDLE_MODE       0x12    // 0001,0010    Idle mode
#define CMD_STANDBY_MODE    0x14    // 0001,0100    Standby mode
#define CMD_PLL_MODE        0x16    // 0001,0110    PLL mode
#define CMD_RX_MODE         0x18    // 0001,1000    RX mode
#define CMD_TX_MODE         0x1a    // 0001,1010    TX mode
#define CMD_DEEP_SLEEP_T    0x1c    // 0001,1100    Deep Sleep mode(tri-state)
#define CMD_DEEP_SLEEP_P    0x1f    // 0001,1111    Deep Sleep mode(pull-high)

#endif //__A7139_RF_H__

