#include "PLATFORM.h"
/****XTAL @11.0592MHz****/
void Delay10ms(Uint8 n)
{
    Uint8 i, j;
    while(n--)
    {
        for(i=0; i<100; i++)
            for(j=0; j<245; j++);
    }
}
/****XTAL @11.0592MHz****/
void Delay10us(Uint8 n)
{
    Uint8 i;
	
	while(n--)
	{
        for(i=0; i<12; i++);
	}
}

void SPIx_WriteByte(Uint8 dat)
{
    Uint8 i;
    for(i=0; i<8; i++)
    {
        if(dat & 0x80)
            SDIO = 1;
        else
            SDIO = 0;
		_nop_();
        SCK=1;
        _nop_();
        SCK=0;
        dat<<=1;
    }
}

void SPIx_WriteWord(Uint16 word)
{
    Uint8 i;
    for(i=0; i<16; i++)
    {
        if(word & 0x8000)
            SDIO = 1;
        else
            SDIO = 0;
        SCK=1;
        _nop_();
        SCK=0;
        word<<=1;
    }
}

Uint8 SPIx_ReadByte(void)
{
	Uint8 i,tmp;
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

Uint16 SPIx_ReadWord(void)
{
	Uint16 i,tmp;
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
    return tmp;
}
