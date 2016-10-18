/*********************************************************************                                                                   **
**  Device:     A7139
**  File:		main.c
**  Target:		Winbond W77LE58
**  Tools:		ICE
**  Updated:	2013-07-19
**  Description:
**  This file is a sample code for your reference.
**
**  Copyright (C) 2011 AMICCOM Corp.
**
*********************************************************************/
#include "define.h"
#include "w77le58.h"
#include "a7139reg.h"
#include "Uti.h"

/*********************************************************************
**  I/O Declaration
*********************************************************************/
#define SCS 		P1_0 		//SPI SCS
#define SCK 		P1_1 		//SPI SCK
#define SDIO 		P1_2 		//SPI SDIO
#define CKO 		P1_3 		//CKO
#define GIO1 		P1_4 		//GIO1
#define GIO2 		P1_5 		//GIO2

/*********************************************************************
**  Constant Declaration
*********************************************************************/
#define TIMEOUT     100			//100ms
#define t0hrel      1000		//1ms

/*********************************************************************
**  Global Variable Declaration
*********************************************************************/
Uint8   data    timer;
Uint8   data    TimeoutFlag;
Uint16  idata   RxCnt;
Uint32  idata   Err_ByteCnt;
Uint32  idata   Err_BitCnt;
Uint16  idata   TimerCnt0;
Uint8   data    *Uartptr;
Uint8   data    UartSendCnt;
Uint8   data    CmdBuf[11];
Uint8   idata   tmpbuf[64];

const Uint8 code BitCount_Tab[16]={0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4};
const Uint8 code ID_Tab[8]={0x34,0x75,0xC5,0x8C,0xC7,0x33,0x45,0xE7};   //ID code
const Uint8 code PN9_Tab[]=
{   0xFF,0x83,0xDF,0x17,0x32,0x09,0x4E,0xD1,
    0xE7,0xCD,0x8A,0x91,0xC6,0xD5,0xC4,0xC4,
    0x40,0x21,0x18,0x4E,0x55,0x86,0xF4,0xDC,
    0x8A,0x15,0xA7,0xEC,0x92,0xDF,0x93,0x53,
    0x30,0x18,0xCA,0x34,0xBF,0xA2,0xC7,0x59,
    0x67,0x8F,0xBA,0x0D,0x6D,0xD8,0x2D,0x7D,
    0x54,0x0A,0x57,0x97,0x70,0x39,0xD2,0x7A,
    0xEA,0x24,0x33,0x85,0xED,0x9A,0x1D,0xE0
};	// This table are 64bytes PN9 pseudo random code.

const Uint16 code A7139Config[]=		//433MHz, 10kbps (IFBW = 100KHz, Fdev = 37.5KHz), Crystal=12.8MHz
{
	0x1221,		//SYSTEM CLOCK register,
	0x0A21,		//PLL1 register,
	0xDA05,		//PLL2 register,	433.301MHz
	0x0000,		//PLL3 register,
	0x0A20,		//PLL4 register,
	0x0024,		//PLL5 register,
	0x0000,		//PLL6 register,
	0x0011,		//CRYSTAL register,
	0x0000,		//PAGEA,
	0x0000,		//PAGEB,
	0x18D4,		//RX1 register, 	IFBW=100KHz	
	0x7009,		//RX2 register, 	by preamble
	0x4000,		//ADC register,	   	
	0x0800,		//PIN CONTROL register,		Use Strobe CMD
	0x4C45,		//CALIBRATION register,
	0x20C0		//MODE CONTROL register, 	Use FIFO mode
};

const Uint16 code A7139Config_PageA[]=   //433MHz, 10kbps (IFBW = 100KHz, Fdev = 37.5KHz), Crystal=12.8MHz
{
	0xF706,		//TX1 register, 	Fdev = 37.5kHz
	0x0000,		//WOR1 register,
	0xF800,		//WOR2 register,
	0x1107,		//RFI register, 	Enable Tx Ramp up/down  
	0x8170,		//PM register,		CST=1
	0x0201,		//RTH register,
	0x400F,		//AGC1 register,	
	0x2AC0,		//AGC2 register, 
	0x0045,		//GIO register, 	GIO2=WTR, GIO1=FSYNC
	0xD181,		//CKO register
	0x0004,		//VCB register,
	0x0A21,		//CHG1 register, 	430MHz
	0x0022,		//CHG2 register, 	435MHz
	0x003F,		//FIFO register, 	FEP=63+1=64bytes
	0x1507,		//CODE register, 	Preamble=4bytes, ID=4bytes
	0x0000		//WCAL register,
};

const Uint16 code A7139Config_PageB[]=   //433MHz, 10kbps (IFBW = 100KHz, Fdev = 37.5KHz), Crystal=12.8MHz
{
	0x0337,		//TX2 register,
	0x8400,		//IF1 register, 	Enable Auto-IF, IF=200KHz
	0x0000,		//IF2 register,
	0x0000,		//ACK register,
	0x0000		//ART register,
};

/*********************************************************************
**  function Declaration
*********************************************************************/
void Timer0ISR(void);
void UART0Isr(void);
void InitTimer0(void);
void InitUART0(void);
void InitRF(void);
void A7139_Config(void);
void A7139_WriteID(void);
void A7139_Cal(void);
void StrobeCMD(Uint8);
void ByteSend(Uint8);
Uint8 ByteRead(void);
void A7139_WriteReg(Uint8, Uint16);
Uint16 A7139_ReadReg(Uint8);
void A7139_WritePageA(Uint8, Uint16);
Uint16 A7139_ReadPageA(Uint8);
void A7139_WritePageB(Uint8, Uint16);
Uint16 A7139_ReadPageB(Uint8);
void A7139_WriteFIFO(void);
void RxPacket(void);
void Err_State(void);

/*********************************************************************
* main loop
*********************************************************************/
void main(void)
{
    //initsw
    PMR |= 0x01;	//set DME0

    //initHW
    P0 = 0xFF;
    P1 = 0xFF;
    P2 = 0xFF;
    P3 = 0xFF;
    P4 = 0x0F;

    InitTimer0();
    InitUART0();
    TR0=1;	//Timer0 on
    EA=1; 	//enable interrupt

    if((P4 & 0x04)==0x04)	//if P4.2=1, master
    {
        InitRF(); //init RF

        while(1)
        {
            A7139_WriteFIFO(); 	//write data to TX FIFO
            StrobeCMD(CMD_TX);
			Delay10us(1);
            while(GIO2); 		//wait transmit completed
            StrobeCMD(CMD_RX);
			Delay10us(1);

            timer=0;
            TimeoutFlag=0;
            while((GIO2==1)&&(TimeoutFlag==0)); 	//wait receive completed
            if(TimeoutFlag)
            {
				StrobeCMD(CMD_STBY);
            }
			else
			{
            	RxPacket();
            	Delay10ms(10);
			}
        }
    }
    else	//if P4.2=0, slave
    {
        InitRF(); //init RF

        RxCnt = 0;
        Err_ByteCnt = 0;
        Err_BitCnt = 0;

        while(1)
        {
            StrobeCMD(CMD_RX);
			Delay10us(1);
            while(GIO2); 		//wait receive completed
            RxPacket();

            A7139_WriteFIFO(); 	//write data to TX FIFO
            StrobeCMD(CMD_TX);
			Delay10us(1);
            while(GIO2); 		//wait transmit completed

            Delay10ms(9);
        }
    }
}

/************************************************************************
**  Timer0ISR
************************************************************************/
void Timer0ISR(void) interrupt 1
{
    TH0 = (65536-t0hrel)>>8;	//Reload Timer0 high byte, low byte
    TL0 = 65536-t0hrel;

    timer++;
    if(timer >= TIMEOUT)
    {
        TimeoutFlag=1;
    }

    TimerCnt0++;
    if(TimerCnt0 == 500)
    {
        TimerCnt0=0;
        CmdBuf[0]=0xF1;

        memcpy(&CmdBuf[1], &RxCnt, 2);
        memcpy(&CmdBuf[3], &Err_ByteCnt, 4);
        memcpy(&CmdBuf[7], &Err_BitCnt, 4);

        UartSendCnt=11;
		Uartptr=&CmdBuf[0];
        SBUF=CmdBuf[0];
    }
}

/************************************************************************
**  UART0ISR
************************************************************************/
void UART0Isr(void) interrupt 4 using 3
{
    if(TI==1)
    {
        TI=0;
        UartSendCnt--;
        if(UartSendCnt !=0)
        {
            Uartptr++;
            SBUF = *Uartptr;
        }
    }
}

/************************************************************************
**  init Timer0
************************************************************************/
void InitTimer0(void)
{
    TR0 = 0;
    TMOD=(TMOD & 0xF0)|0x01;	//Timer0 mode=1
    TH0 = (65536-t0hrel)>>8;	//setup Timer0 high byte, low byte
    TL0 = 65536-t0hrel;
    TF0 = 0;					//Clear any pending Timer0 interrupts
    ET0 = 1;					//Enable Timer0 interrupt
}

/************************************************************************
**  Init UART0
************************************************************************/
void InitUART0(void)
{
    TH1 = 0xFD; 				//BaudRate 9600;
    TL1 = 0xFD;
    SCON= 0x40;
    TMOD= (TMOD & 0x0F) | 0x20;
    REN = 1;
    TR1 = 1;
    ES  = 1;
}

/*********************************************************************
** Strobe Command
*********************************************************************/
void StrobeCMD(Uint8 cmd)
{
    Uint8 i;

    SCS=0;
    for(i=0; i<8; i++)
    {
        if(cmd & 0x80)
            SDIO = 1;
        else
            SDIO = 0;

        _nop_();
        SCK=1;
        _nop_();
        SCK=0;
        cmd<<=1;
    }
	SCS=1;
}

/************************************************************************
**  ByteSend
************************************************************************/
void ByteSend(Uint8 src)
{
    Uint8 i;

    for(i=0; i<8; i++)
    {
        if(src & 0x80)
            SDIO = 1;
        else
            SDIO = 0;

		_nop_();
        SCK=1;
        _nop_();
        SCK=0;
        src<<=1;
    }
}

/************************************************************************
**  ByteRead
************************************************************************/
Uint8 ByteRead(void)
{
	Uint8 i,tmp;

    //read data code
    SDIO=1;		//SDIO pull high
    for(i=0; i<8; i++)
    {
        if(SDIO)
            tmp = (tmp << 1) | 0x01;
        else
            tmp = tmp << 1;

        SCK=1;
        _nop_();
        SCK=0;
    }
    return tmp;
}

/************************************************************************
**  A7139_WriteReg
************************************************************************/
void A7139_WriteReg(Uint8 address, Uint16 dataWord)
{
    Uint8 i;

    SCS=0;
    address |= CMD_Reg_W;
    for(i=0; i<8; i++)
    {
        if(address & 0x80)
            SDIO = 1;
        else
            SDIO = 0;

        SCK=1;
        _nop_(); 
        SCK=0;
        address<<=1;
    }
    _nop_();

    //send data word
    for(i=0; i<16; i++)
    {
        if(dataWord & 0x8000)
            SDIO = 1;
        else
            SDIO = 0;

        SCK=1;
        _nop_();
        SCK=0;
        dataWord<<=1;
    }
    SCS=1;
}

/************************************************************************
**  A7139_ReadReg
************************************************************************/
Uint16 A7139_ReadReg(Uint8 address)
{
    Uint8 i;
    Uint16 tmp;

    SCS=0;
    address |= CMD_Reg_R;
    for(i=0; i<8; i++)
    {
        if(address & 0x80)
            SDIO = 1;
        else
            SDIO = 0;

		_nop_(); 
        SCK=1;
        _nop_();
        SCK=0;

        address<<=1;
    }
    _nop_();
    
    //read data code
    SDIO=1;		//SDIO pull high
    for(i=0; i<16; i++)
    {
        if(SDIO)
            tmp = (tmp << 1) | 0x01;
        else
            tmp = tmp << 1;

        SCK=1;
        _nop_();
        SCK=0;
    }
    SCS=1;
    return tmp;
}

/************************************************************************
**  A7139_WritePageA
************************************************************************/
void A7139_WritePageA(Uint8 address, Uint16 dataWord)
{
    Uint16 tmp;

    tmp = address;
    tmp = ((tmp << 12) | A7139Config[CRYSTAL_REG]);
    A7139_WriteReg(CRYSTAL_REG, tmp);
    A7139_WriteReg(PAGEA_REG, dataWord);
}

/************************************************************************
**  A7139_ReadPageA
************************************************************************/
Uint16 A7139_ReadPageA(Uint8 address)
{
    Uint16 tmp;

    tmp = address;
    tmp = ((tmp << 12) | A7139Config[CRYSTAL_REG]);
    A7139_WriteReg(CRYSTAL_REG, tmp);
    tmp = A7139_ReadReg(PAGEA_REG);
    return tmp;
}

/************************************************************************
**  A7139_WritePageB
************************************************************************/
void A7139_WritePageB(Uint8 address, Uint16 dataWord)
{
    Uint16 tmp;

    tmp = address;
    tmp = ((tmp << 7) | A7139Config[CRYSTAL_REG]);
    A7139_WriteReg(CRYSTAL_REG, tmp);
    A7139_WriteReg(PAGEB_REG, dataWord);
}

/************************************************************************
**  A7139_ReadPageB
************************************************************************/
Uint16 A7139_ReadPageB(Uint8 address)
{
    Uint16 tmp;

    tmp = address;
    tmp = ((tmp << 7) | A7139Config[CRYSTAL_REG]);
    A7139_WriteReg(CRYSTAL_REG, tmp);
    tmp = A7139_ReadReg(PAGEB_REG);
    return tmp;
}

/*********************************************************************
** initRF
*********************************************************************/
void InitRF(void)
{
    //initial pin
    SCS = 1;
    SCK = 0;
    SDIO= 1;
    CKO	= 1;
    GIO1= 1;
    GIO2= 1;

    StrobeCMD(CMD_RF_RST);	//reset A7139 chip
    A7139_Config();			//config A7139 chip
    Delay100us(8);			//delay 800us for crystal stabilized
    A7139_WriteID();		//write ID code
    A7139_Cal();			//IF and VCO calibration
}

/*********************************************************************
** A7139_Config
*********************************************************************/
void A7139_Config(void)
{
	Uint8 i;
	Uint16 tmp;

    for(i=0; i<8; i++)
        A7139_WriteReg(i, A7139Config[i]);

	for(i=10; i<16; i++)
        A7139_WriteReg(i, A7139Config[i]);

    for(i=0; i<16; i++)
        A7139_WritePageA(i, A7139Config_PageA[i]);

	for(i=0; i<5; i++)
        A7139_WritePageB(i, A7139Config_PageB[i]);

	//for check        
	tmp = A7139_ReadReg(SYSTEMCLOCK_REG);
	if(tmp != A7139Config[SYSTEMCLOCK_REG])
	{
		Err_State();	
	}
}

/************************************************************************
**  WriteID
************************************************************************/
void A7139_WriteID(void)
{
	Uint8 i;
	Uint8 d1, d2, d3, d4;

	SCS=0;
	ByteSend(CMD_ID_W);
	for(i=0; i<4; i++)
		ByteSend(ID_Tab[i]);
	SCS=1;

	SCS=0;
	ByteSend(CMD_ID_R);
	d1=ByteRead();
	d2=ByteRead();
	d3=ByteRead();
	d4=ByteRead();
	SCS=1;
	
    if((d1!=ID_Tab[0]) || (d2!=ID_Tab[1]) || (d3!=ID_Tab[2]) || (d4!=ID_Tab[3]))
    {
        Err_State();
    }
}

/*********************************************************************
** A7139_Cal
*********************************************************************/
void A7139_Cal(void)
{
    Uint8 fb, fcd, fbcf;	//IF Filter
	Uint8 vb,vbcf;			//VCO Current
	Uint8 vcb, vccf;		//VCO Band
	Uint16 tmp;

    //IF calibration procedure @STB state
	A7139_WriteReg(MODE_REG, A7139Config[MODE_REG] | 0x0802);			//IF Filter & VCO Current Calibration
    do{
        tmp = A7139_ReadReg(MODE_REG);
    }while(tmp & 0x0802);
	
    //for check(IF Filter)
    tmp = A7139_ReadReg(CALIBRATION_REG);
    fb = tmp & 0x0F;
	fcd = (tmp>>11) & 0x1F;
    fbcf = (tmp>>4) & 0x01;
    if(fbcf)
    {
        Err_State();
    }

	//for check(VCO Current)
    tmp = A7139_ReadPageA(VCB_PAGEA);
	vcb = tmp & 0x0F;
	vccf = (tmp>>4) & 0x01;
	if(vccf)
	{
        Err_State();
    }
    
    
    //RSSI Calibration procedure @STB state
	A7139_WriteReg(ADC_REG, 0x4C00);									//set ADC average=64
    A7139_WriteReg(MODE_REG, A7139Config[MODE_REG] | 0x1000);			//RSSI Calibration
    do{
        tmp = A7139_ReadReg(MODE_REG);
    }while(tmp & 0x1000);
	A7139_WriteReg(ADC_REG, A7139Config[ADC_REG]);


    //VCO calibration procedure @STB state
	A7139_WriteReg(PLL1_REG, A7139Config[PLL1_REG]);
	A7139_WriteReg(PLL2_REG, A7139Config[PLL2_REG]);
	A7139_WriteReg(MODE_REG, A7139Config[MODE_REG] | 0x0004);		//VCO Band Calibration
	do{
		tmp = A7139_ReadReg(MODE_REG);
	}while(tmp & 0x0004);
	
	//for check(VCO Band)
	tmp = A7139_ReadReg(CALIBRATION_REG);
	vb = (tmp >>5) & 0x07;
	vbcf = (tmp >>8) & 0x01;
	if(vbcf)
	{
		Err_State();
	}
}

/*********************************************************************
** A7139_WriteFIFO
*********************************************************************/
void A7139_WriteFIFO(void)
{
	Uint8 i;

	StrobeCMD(CMD_TFR);		//TX FIFO address pointer reset

    SCS=0;
	ByteSend(CMD_FIFO_W);	//TX FIFO write command
	for(i=0; i <64; i++)
		ByteSend(PN9_Tab[i]);
	SCS=1;
}

/*********************************************************************
** RxPacket
*********************************************************************/
void RxPacket(void)
{
    Uint8 i;
    Uint8 recv;
    Uint8 tmp;

	RxCnt++;

	StrobeCMD(CMD_RFR);		//RX FIFO address pointer reset

    SCS=0;
	ByteSend(CMD_FIFO_R);	//RX FIFO read command
	for(i=0; i <64; i++)
	{
		tmpbuf[i] = ByteRead();
	}
	SCS=1;

	for(i=0; i<64; i++)
	{
		recv = tmpbuf[i];
		tmp = recv ^ PN9_Tab[i];
		if(tmp!=0)
		{
			Err_ByteCnt++;
			Err_BitCnt += (BitCount_Tab[tmp>>4] + BitCount_Tab[tmp & 0x0F]);
		}
    }
}

/*********************************************************************
** Err_State
*********************************************************************/
void Err_State(void)
{
    //ERR display
    //Error Proc...
    //...
    while(1);
}
