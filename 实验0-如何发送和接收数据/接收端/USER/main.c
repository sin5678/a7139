#include "AT89X52.h"
#include "INTDEF.h"
#include "A7139.h"
#include "PLATFORM.h"

#define LED (P3_6) //LED与P3.6相连，低电平有效（点亮）
#define KEY (P3_7) //KEY与P3.7相连，低电平有效（按下）
Uint8 idata tmpbuf[64]; //数组用于存放接收到的数据
/*XTAL @11.0592*/
static void LED_Flash_Twice(void)
{
	LED=0;
	Delay10ms(2);
	LED=1;
	Delay10ms(2);
	LED=0;
	Delay10ms(2);
	LED=1;
}

void A7139_WriteFIFO(void)
{
	Uint8 i;
	StrobeCmd(CMD_TFR);	//重置Tx数据指针位置	
    SCS=0;
	SPIx_WriteByte(CMD_FIFO_W);//送出“写FIFO缓存区”的命令
	for(i=0; i <64; i++)
		SPIx_WriteByte(i+64);//送出待发送的数据
	SCS=1;					 //将64-127之间的数字作为接收端反馈数据而写入A7139内部的FIFO缓冲区中，反馈给发送端				      
							 //这里只为演示简单，故采取用64-127之间的数字，用户可自行修改需要发送的反馈数据
}

void A7139_ReadFIFO(void)
{
    Uint8 i;
	StrobeCmd(CMD_RFR); //重置Rx数据指针位置
    SCS=0;
	SPIx_WriteByte(CMD_FIFO_R);//送出“读FIFO缓存区”的命令
	for(i=0; i <64; i++)
		tmpbuf[i] = SPIx_ReadByte(); //从A7139内部的FIFO缓冲区中读取数据，并将接收到数据暂存入数组tmpbuf中以备它用
	SCS=1;
}
void C51_SendData(void)
{	
	A7139_WriteFIFO();
	StrobeCmd(CMD_TX);
	while(GIO2); //循环等待，数据发送完毕后，GIO2引脚将输出一个正脉冲信号
}
void C51_RecvData(void)
{
	StrobeCmd(CMD_RX);
	while(GIO2); //循环等待，数据接收完毕后，GIO2引脚将输出一个正脉冲信号
	A7139_ReadFIFO();
}
void main(void)
{
	A7139_Init(433.921f); //A7139初始化
	LED_Flash_Twice();//上电LED闪烁2次，作为指示灯
	while(1)
	{
	    C51_RecvData(); 
		LED_Flash_Twice();
		C51_SendData();
	}
}


