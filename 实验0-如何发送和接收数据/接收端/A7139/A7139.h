#ifndef  __A7139_H__
#define  __A7139_H__
#include "INTDEF.h"
#include "PLATFORM.h"

#ifdef PLATEFORM_C51
	#define DATA  data
	#define IDATA idata
	#define CODE  code
	#define XDATA xdata
#else
	#define DATA
	#define IDATA
	#define CODE
	#define XDATA
#endif

static void A7139_Cal(void);
static void A7139_Config(void);
static void A7139_WriteReg(Uint8 regAddr, Uint16 regVal);
static Uint16 A7139_ReadReg(Uint8 regAddr);
static Uint16 A7139_ReadPageA(Uint8 pagAddr);
static void A7139_WritePageA(Uint8 pagAddr, Uint16 pagVal);
static void A7139_WritePageB(Uint8 pagAddr, Uint16 pagVal);


void StrobeCmd(Uint8 cmd);
void   A7139_Init(float rfFreq);
Uint16 A7139_ReadPID(void);
void   A7139_SetCID(Uint32 id);
static void A7139_SetFreq(float rfFreq);
Uint8  A7139_GetRSSI(void);
Uint8  A7139_RCOSC_Cal(void);
Uint8  A7139_WOR_ByPreamble(void);
Uint8  A7139_WOR_BySync(void);


#define SYSTEMCLOCK_REG 	0x00
#define PLL1_REG 			0x01
#define PLL2_REG 			0x02
#define PLL3_REG 			0x03
#define PLL4_REG			0x04
#define PLL5_REG			0x05
#define PLL6_REG			0x06
#define CRYSTAL_REG			0x07
#define PAGEA_REG	  		0x08
#define PAGEB_REG			0x09
#define RX1_REG  			0x0A
#define RX2_REG  			0x0B
#define ADC_REG  			0x0C
#define PIN_REG 			0x0D
#define CALIBRATION_REG  	0x0E
#define MODE_REG  			0x0F

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
#define FIFO_PAGEA			0x0D
#define CODE_PAGEA			0x0E
#define WCAL_PAGEA			0x0F

#define TX2_PAGEB			0x00
#define	IF1_PAGEB			0x01
#define IF2_PAGEB			0x02
#define	ACK_PAGEB			0x03
#define	ART_PAGEB			0x04


#define CMD_Reg_W			0x00	//000x,xxxx	control register write
#define CMD_Reg_R			0x80	//100x,xxxx	control register read
#define CMD_CID_W			0x20	//001x,xxxx	ID write
#define CMD_CID_R			0xA0	//101x,xxxx	ID Read
#define CMD_FIFO_W			0x40	//010x,xxxx	TX FIFO Write
#define CMD_FIFO_R			0xC0	//110x,xxxx	RX FIFO Read
#define CMD_RF_RST			0x70	//x111,xxxx	RF reset
#define CMD_TFR				0x60	//0110,xxxx	TX FIFO address pointrt reset
#define CMD_RFR				0xE0	//1110,xxxx	RX FIFO address pointer reset

#define CMD_SLEEP			0x10	//0001,0000	SLEEP mode
#define CMD_IDLE			0x12	//0001,0010	IDLE mode
#define CMD_STBY			0x14	//0001,0100	Standby mode
#define CMD_PLL				0x16	//0001,0110	PLL mode
#define CMD_RX				0x18	//0001,1000	RX mode
#define CMD_TX				0x1A	//0001,1010	TX mode
//#define CMD_DEEP_SLEEP	0x1C	//0001,1100 Deep Sleep mode(tri-state)
#define CMD_DEEP_SLEEP		0x1F	//0001,1111 Deep Sleep mode(pull-high)

#define ERR_PARAM 			0x01
#define ERR_PLATFORM		0x02
#define ERR_UNK				0x03
#define ERR_TMO				0xFF		
#define ERR_RCOSC_CAL		0x04
#define OK_RCOSC_CAL		0x05
#define ERR_GET_RSSI		0x00
#endif
