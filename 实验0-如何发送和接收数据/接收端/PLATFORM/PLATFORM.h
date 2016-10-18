#ifndef  __PLATFORM_H__
#define  __PLATFORM_H__
#include "INTDEF.h"
/**********************************Include****************************************/
/*****Import your header files here,such as reg51.h or stm32f10x.h,and so on******/ 
//#include "xxxxxxx.h"												           /**/ 
#include "AT89X52.h" //for AT89C52											   /**/
#include "intrins.h" //for _nop_()											   /**/
/*********************************************************************************/

/**********The following Macro define must by users to implement******************/
#define SCS   		(P0_2)		//Implement to set or get SPI SCS     		   /**/  
#define SCK         (P0_3) 		//Implement to set or set SPI SCK       	   /**/ 
#define SDIO        (P0_4) 		//Implement to set or get SPI MOSI     		   /**/ 
#define GIO1		(P0_5) 		//Implement to set or get GIO1          	   /**/ 
#define GIO2	  	(P0_6)		//Implement to set or get GIO2                 /**/ 
#define CKO  		(P0_7)	    //Implement to set or get CKO                  /**/ 
/*********************************************************************************/

void Delay10ms(Uint8 n);
void Delay10us(Uint8 n);
//void   Pin_Init(void);
//void   Pin_Mode(Uint8 pin,Uint8 mod);
//void   SPIx_Init(void);
void SPIx_WriteByte(Uint8 dat);
void SPIx_WriteWord(Uint16 wrd);
Uint8 SPIx_ReadByte(void);
Uint16 SPIx_ReadWord(void);
#endif

