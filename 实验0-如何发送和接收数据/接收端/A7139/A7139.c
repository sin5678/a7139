#include "A7139.h"

const Uint8  CODE ID_Tab[];
const Uint16 CODE Freq_Cal_Tab[];
const Uint16 CODE A7139Config_PageA[];
const Uint16 CODE A7139Config_PageB[];
const Uint16 CODE A7139Config[];

static void A7139_WriteReg(Uint8 regAddr, Uint16 regVal)
{
    SCS=0;
    regAddr |= CMD_Reg_W;
    SPIx_WriteByte(regAddr);
    _nop_();
    SPIx_WriteWord(regVal);
    SCS=1;
}

static Uint16 A7139_ReadReg(Uint8 regAddr)
{
    Uint16 regVal;
    SCS=0;
    regAddr |= CMD_Reg_R;
   	SPIx_WriteByte(regAddr);
    _nop_();
   	regVal=SPIx_ReadWord();
    SCS=1;
    return regVal;
}

static void A7139_WritePageA(Uint8 address, Uint16 dataWord)
{
    Uint16 tmp;
    tmp = address;
    tmp = ((tmp << 12) | A7139Config[CRYSTAL_REG]);
    A7139_WriteReg(CRYSTAL_REG, tmp);
    A7139_WriteReg(PAGEA_REG, dataWord);
}

static void A7139_WritePageB(Uint8 address, Uint16 dataWord)
{
    Uint16 tmp;
    tmp = address;
    tmp = ((tmp << 7) | A7139Config[CRYSTAL_REG]);
    A7139_WriteReg(CRYSTAL_REG, tmp);
    A7139_WriteReg(PAGEB_REG, dataWord);
}

static Uint16 A7139_ReadPageA(Uint8 address)
{
    Uint16 tmp;
    tmp = address;
    tmp = ((tmp << 12) | A7139Config[CRYSTAL_REG]);
    A7139_WriteReg(CRYSTAL_REG, tmp);
    tmp = A7139_ReadReg(PAGEA_REG);
    return tmp;
}

static void A7139_Config(void)
{
	Uint8 i;
    for(i=0; i<8; i++)
        A7139_WriteReg(i, A7139Config[i]);
	for(i=10; i<16; i++)
        A7139_WriteReg(i, A7139Config[i]);
    for(i=0; i<16; i++)
        A7139_WritePageA(i, A7139Config_PageA[i]);
	for(i=0; i<5; i++)
        A7139_WritePageB(i, A7139Config_PageB[i]);
}

static void A7139_Cal(void)
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
        //Err_State();
    }
	//for check(VCO Current)
    tmp = A7139_ReadPageA(VCB_PAGEA);
	vcb = tmp & 0x0F;
	vccf = (tmp>>4) & 0x01;
	if(vccf)
	{
        //Err_State();
    }
    //RSSI Calibration procedure @STB state
	A7139_WriteReg(ADC_REG, 0x4C00);									//set ADC average=64
    A7139_WritePageA(WOR2_PAGEA, 0xF800);								//set RSSC_D=40us and RS_DLY=80us
	A7139_WritePageA(TX1_PAGEA, A7139Config_PageA[TX1_PAGEA] | 0xE000);	//set RC_DLY=1.5ms
    A7139_WriteReg(MODE_REG, A7139Config[MODE_REG] | 0x1000);			//RSSI Calibration
    do{
        tmp = A7139_ReadReg(MODE_REG);
    }while(tmp & 0x1000);
	A7139_WriteReg(ADC_REG, A7139Config[ADC_REG]);
    A7139_WritePageA(WOR2_PAGEA, A7139Config_PageA[WOR2_PAGEA]);
	A7139_WritePageA(TX1_PAGEA, A7139Config_PageA[TX1_PAGEA]);
    //VCO calibration procedure @STB state
    //for(i=0; i<3; i++)
    
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
		//Err_State();
	}
}

void StrobeCmd(Uint8 cmd)
{
    SCS=0;
    SPIx_WriteByte(cmd);
	SCS=1;
}

void A7139_Init(float rfFreq)
{
    SCS  = 1;
    SCK  = 0;
    SDIO = 1;
    CKO	 = 1;
    GIO1 = 1;
    GIO2 = 1;
    StrobeCmd(CMD_RF_RST);	  //reset A7139 chip
    A7139_Config();			  //config A7139 chip
    Delay10ms(1);			  //for crystal stabilized
    A7139_SetCID(0x3475C58C); //set CID code
	A7139_SetFreq(rfFreq);	  //set Freq
    A7139_Cal();		  //IF and VCO calibration
}

void A7139_SetCID(Uint32 id)
{
	SCS=0;
	SPIx_WriteByte(CMD_CID_W);
	SPIx_WriteByte((Uint8)(id>>24));
	SPIx_WriteByte((Uint8)(id>>16));
	SPIx_WriteByte((Uint8)(id>>8));
	SPIx_WriteByte((Uint8)id);
	SCS=1;
}

Uint16 A7139_ReadPID(void)
{
	Uint16 pid;
	Uint16 pagAddr = TX2_PAGEB << 7;
	pagAddr|=A7139Config[CRYSTAL_REG] & 0xF7CF;
	A7139_WriteReg(CRYSTAL_REG, pagAddr);
	pid = A7139_ReadReg(PAGEB_REG);
	return pid;
}

static void A7139_SetFreq(float rfFreq)
{
	 float  divFreq = rfFreq / 12.800f;  
	 Uint8  intFreq = (Uint8)(divFreq); //integer part
	 float  fltFreq = divFreq - intFreq * 1.000f; //fraction part
	 Uint16 fpFreg	= (Uint16)(fltFreq * 65536);  //FP register val
	 Uint16 orgVal;
	 StrobeCmd(CMD_STBY); //enter stand-by mode
			 //AFC[15:15] = 0
	 orgVal = A7139Config[PLL3_REG] & 0x7FFF;
	 A7139_WriteReg(PLL3_REG,orgVal);
	 		//RFC[15:12] = 0000
	 orgVal = A7139Config[PLL6_REG] & 0x0FFF;
	 A7139_WriteReg(PLL6_REG,orgVal);
	 	//MD1[12:12]=0,1
	 if(rfFreq < 860)	//433-510
		orgVal = A7139Config[PLL4_REG] & 0xEFFF;
	 else	 //868-915
		orgVal = A7139Config[PLL4_REG] | 0x1000;
	 A7139_WriteReg(PLL4_REG,orgVal);
	 		//IP[8:0] = intg
	 orgVal = A7139Config[PLL1_REG] & 0xFF00;
	 A7139_WriteReg(PLL1_REG,orgVal|intFreq);
	 		//FP[15:0] =  fpFreg
	 A7139_WriteReg(PLL2_REG,fpFreg); 
			//FPA[15:0]=0x0000
	 A7139_WritePageB(IF2_PAGEB,0x0000);	
}

Uint8 A7139_SetDataRate(Uint8 datRate)
{
	Uint16 segCSC,segSDR,segIFB,segDMO;
	StrobeCmd(CMD_STBY);//enter stand by mode
	segCSC = A7139Config[SYSTEMCLOCK_REG] & 0xFFF8;//CSC_CLR  
	segSDR = A7139Config[SYSTEMCLOCK_REG] & 0x01FF;//SDR_CLR
	segIFB = A7139Config[RX1_REG] & 0xFFF3; //IFBW_CLR
	segDMO = A7139Config[RX1_REG] & 0xFFBF;  //DMOS_CLR
	A7139_WriteReg(RX1_REG,segDMO|0x0040); //DMOS_SET
	switch(datRate)
	{
		case 2:
		{	
					//CSC=011,fcsck=3.2MHz
			A7139_WriteReg(SYSTEMCLOCK_REG,segCSC|0x0003);
					//SDR=0x18	
			A7139_WriteReg(SYSTEMCLOCK_REG,segSDR|0x3000);
					//IFBW=[00] <=> 50kHz
			A7139_WriteReg(RX1_REG,segIFB|0x0000); 		
		}
		break;
		case 10:
		{
					//CSC=011,fcsck=3.2MHz 
		 	A7139_WriteReg(SYSTEMCLOCK_REG,segCSC|0x0003);
					//SDR=0x04
			A7139_WriteReg(SYSTEMCLOCK_REG,segSDR|0x0800);
			 		//IFBW=[00] <=> 50kHz
			A7139_WriteReg(RX1_REG,segIFB|0x0000);
		}
		break;
		case 50:
		{
			 		//CSC=011 ,fcsck=3.2MHz
			A7139_WriteReg(SYSTEMCLOCK_REG,segCSC|0x0003);
					//SDR=0x00
			A7139_WriteReg(SYSTEMCLOCK_REG,segSDR|0x0000);
					//IFBW=[00] <=> 50kHz
			A7139_WriteReg(RX1_REG,segIFB|0x0000); 
		}
		break;
		case 100:
		{
					//CSC=001 ,fcsck=6.4MHz
			A7139_WriteReg(SYSTEMCLOCK_REG,segCSC|0x0001);
					//SDR=0x00			
			A7139_WriteReg(SYSTEMCLOCK_REG,segSDR|0x0000);
					//IFBW=[01] <=> 100kHz
			A7139_WriteReg(RX1_REG,segIFB|0x0004);
		}
		break;
		case 150:
		{
			//must use Pll clk gen,complement in detail later
			//IFBW=[10] <=> 150kHz
			//DCK=150K
			//CSC=000,fcsck=9.6MHz
			//SDR=0x00
			//DMOS=1,IFBW=150KHz
		}
		break;
		default:
		return ERR_PARAM;
	}
	return 0;
}

Uint8 A7139_SetPackLen(Uint8 len)
{
	Uint16 pagVal;
	StrobeCmd(CMD_STBY);
			//FEP[7:0]
	pagVal = A7139Config_PageA[FIFO_PAGEA] & 0xFF00;
	A7139_WritePageA(FIFO_PAGEA,pagVal|(len-1));
			//FEP[13:8]
	pagVal = A7139Config_PageA[VCB_PAGEA] & 0xC0FF;
	A7139_WritePageA(VCB_PAGEA,pagVal);
	return 0;			
}

//BOOL A7139_DetectCarrier()
//{
//	Uint16 pagVal;
//	//must frist enter Rx mode
//	StrobeCmd(CMD_RX);
//	//CMD[8:8] = 1
//	pagVal= A7139Config[ADC_REG] | 0x0100;
//	A7139_WriteReg(ADC_REG,pagVal);
//	//ADCM[0:0] = 1
//	pagVal= A7139Config[MODE_REG] | 0x0001;
//	A7139_WriteReg(MODE_REG,pagVal);
//	//quit Rx mode to enter Stand-by mode
//	StrobeCmd(CMD_STBY);
//	return FALSE;
//}
Uint8 A7139_IsBatteryLow(Uint8 low2_x)
{
	Uint16 pagVal;
	if(low2_x<0 || low2_x>7)
		return ERR_PARAM;
	StrobeCmd(CMD_STBY);
			//BVT[3:1] BDS[0:0]
	pagVal= A7139Config[PM_PAGEA] & 0xFFF0;
	A7139_WritePageA(PM_PAGEA,pagVal | (low2_x << 1) | 0x01);
	Delay10us(1); //delay 5us at least 
			//read VBD[7:7]
	return !((A7139_ReadPageA(WOR1_PAGEA) & 0x0080) >> 7);
}
Uint8 A7139_GetRSSI()
{	
	Uint8  rssi;
	Uint16 t_retry = 0xFFFF;
		//entry RX mode
	StrobeCmd(CMD_RX);	
			//CDM[8:8] = 0
	A7139_WriteReg(ADC_REG,A7139Config[ADC_REG] & 0xFEFF);
			//ADCM[0:0] = 1
	A7139_WriteReg(MODE_REG,A7139_ReadReg(MODE_REG) | 0x0001);
	do
	{
		rssi = A7139_ReadReg(MODE_REG) & 0x0001; //ADCM auto clear when measurement done
			
	}while(t_retry-- && rssi);
	if(t_retry>0)
		rssi=(A7139_ReadReg(ADC_REG) & 0x00FF);  //ADC[7:0] is the value of RSSI
	else
		rssi = ERR_GET_RSSI;
	StrobeCmd(CMD_STBY);
	return rssi;		
}
Uint8 A7139_RCOSC_Cal(void)
{
	  Uint8  retry = 0xFF;
	  Uint16 calbrtVal,t_retry=0xFFFF;
	  		//RCOSC_E[4:4] = 1,enable internal RC Oscillator
	  A7139_WritePageA(WOR2_PAGEA, A7139Config_PageA[WOR2_PAGEA] | 0x0010);
	  do{
		  		//ENCAL[0:0] = 1,then start RC OSC Calbrt
		  A7139_WritePageA(WCAL_PAGEA, A7139Config_PageA[WCAL_PAGEA] | 0x0001);
		  do{
		  	  calbrtVal = A7139_ReadPageA(WCAL_PAGEA) & 0x0001;
		  }while(calbrtVal && t_retry--);
		  		//read NUMLH[9:1]
		  calbrtVal = (A7139_ReadPageA(WCAL_PAGEA) & 0x03FF) >> 1;
		  if(calbrtVal>186 && calbrtVal<198)
			   return OK_RCOSC_CAL;
		}while(retry--);
	  return ERR_RCOSC_CAL;
}
Uint8 A7139_WOT(void)
{
	if(A7139_RCOSC_Cal()==ERR_RCOSC_CAL)
		return 1;
	StrobeCmd(CMD_STBY);
		//GIO1=FSYNC, GIO2=WTR	
	A7139_WritePageA(GIO_PAGEA, 0x0045);
		//setup WOT Sleep time
	A7139_WritePageA(WOR1_PAGEA, 0x027f);
		//WMODE=1 select WOT function
	A7139_WriteReg(PIN_REG, A7139Config[PIN_REG] | 0x0400);
		//WORE=1 to enable WOT function		
	A7139_WriteReg(MODE_REG, A7139Config[MODE_REG] | 0x0200);
	while(1);
}
Uint8 A7139_WOR_BySync(void)
{
	StrobeCmd(CMD_STBY);
			//GIO1=FSYNC, GIO2=WTR	
	A7139_WritePageA(GIO_PAGEA, 0x0045);
			//setup WOR Sleep time and Rx time
	A7139_WritePageA(WOR1_PAGEA, 0xFC05);
	if(A7139_RCOSC_Cal()==ERR_RCOSC_CAL)
		return 1;
			//enable RC OSC & WOR by sync
	A7139_WritePageA(WOR2_PAGEA, A7139Config_PageA[WOR2_PAGEA] | 0x0010);
			//WORE=1 to enable WOR function
	A7139_WriteReg(MODE_REG, A7139Config[MODE_REG] | 0x0200);
			//Stay in WOR mode until receiving ID code(sync ok)
}
Uint8 A7139_WOR_ByPreamble(void)
{
	StrobeCmd(CMD_STBY);
	A7139_WritePageA(GIO_PAGEA, 0x004D);	//GIO1=PMDO, GIO2=WTR

	//Real WOR Active Period = (WOR_AC[5:0]+1) x 244us ¡V X'TAL and Regulator Settling Time
	//Note : Be aware that X¡¦tal settling time requirement includes initial tolerance, 
	//       temperature drift, aging and crystal loading.
	A7139_WritePageA(WOR1_PAGEA, 0x8005);	//setup WOR Sleep time and Rx time
			 	//RC Oscillator Calibration
	if(A7139_RCOSC_Cal()==ERR_RCOSC_CAL)
		return 1;     
	A7139_WritePageA(WOR2_PAGEA, A7139Config_PageA[WOR2_PAGEA] | 0x0030);	//enable RC OSC & WOR by preamble
	
	A7139_WriteReg(MODE_REG, A7139Config[MODE_REG] | 0x0200);				//WORE=1 to enable WOR function
	
	//while(GIO1==0);		//Stay in WOR mode until receiving preamble(preamble ok)
}

const Uint16 CODE A7139Config[]=		//470MHz, 10kbps (IFBW = 100KHz, Fdev = 37.5KHz), Crystal=12.8MHz
{
	0x1221,		//SYSTEM CLOCK register,
	0x0A24,		//PLL1 register,
	0xB805,		//PLL2 register,	470.001MHz
	0x0000,		//PLL3 register,
	0x0A20,		//PLL4 register,
	0x0024,		//PLL5 register,
	0x0000,		//PLL6 register,
	0x0001,		//CRYSTAL register,
	0x0000,		//PAGEA,
	0x0000,		//PAGEB,
	0x18D4,		//RX1 register, 	IFBW=100KHz, ETH=1	
	0x7009,		//RX2 register, 	by preamble
	0x4400,		//ADC register,	   	
	0x0800,		//PIN CONTROL register,		Use Strobe CMD
	0x4845,		//CALIBRATION register,
	0x20C0		//MODE CONTROL register, 	Use FIFO mode
};

const Uint16 CODE A7139Config_PageA[]=   //470MHz, 10kbps (IFBW = 100KHz, Fdev = 37.5KHz), Crystal=12.8MHz
{
	0x1706,		//TX1 register, 	Fdev = 37.5kHz
	0x0000,		//WOR1 register,
	0x0000,		//WOR2 register,
	0x1187,		//RFI register, 	Enable Tx Ramp up/down  
	0x8170,		//PM register,		CST=1
	0x0201,		//RTH register,
	0x400F,		//AGC1 register,	
	0x2AC0,		//AGC2 register, 
	0x0045,		//GIO register, 	GIO2=WTR, GIO1=FSYNC
	0xD281,		//CKO register
	0x0004,		//VCB register,
	0x0021,		//CHG1 register, 	480MHz
	0x0022,		//CHG2 register, 	500MHz
	0x003F,		//FIFO register, 	FEP=63+1=64bytes
	0x1507,		//CODE register, 	Preamble=4bytes, ID=4bytes
	0x0000		//WCAL register,
};

const Uint16 CODE A7139Config_PageB[]=   //470MHz, 10kbps (IFBW = 100KHz, Fdev = 37.5KHz), Crystal=12.8MHz
{
	0x0B37,		//TX2 register,
	0x8400,		//IF1 register, 	Enable Auto-IF, IF=200KHz
	0x0000,		//IF2 register,
	0x0000,		//ACK register,
	0x0000		//ART register,
};



