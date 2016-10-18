//*******************************************************************************
// Author:          redfox.qu@qq.com
// Date:            2014/03/06
// Version:         1.0.0
// Description:     The UDP packet to CAN frame transmit on am335x
//
// History:
//      <author>            <time>      <version>   <desc>
//      redfox.qu@qq.com    2014/03/27  1.0.0       create this file
//
// TODO:
//      2014/03/27          create base uart communate
//                          blink led
//*******************************************************************************

#include "stm32f10x.h"
#include "platform_config.h"
#include "stm32f10x_usart.h"
#include "misc.h"
#include "stdarg.h"
#include "a7139_mcu.h"


/* Private variables ---------------------------------------------------------*/
uint8_t TxBuffer1[] = "USART Interrupt Example: This is USART1 DEMO";
uint8_t RxBuffer1[], rec_f, tx_flag;

struct a7139_dev rf_dev;

/* Private function prototypes -----------------------------------------------*/
void RCC_Configuration(void);
void GPIO_Configuration(void);
void NVIC_Configuration(void);

void USART_OUT(USART_TypeDef* USARTx, uint8_t *Data, ...);
void USART_Config(USART_TypeDef* USARTx);
char *itoa(int value, char *string, int radix);


GPIO_InitTypeDef GPIO_InitStructure;
USART_InitTypeDef USART_InitStruct;
USART_ClockInitTypeDef USART_ClockInitStruct;


void USART_Config(USART_TypeDef* USARTx)
{
    USART_InitTypeDef usart_init;

    usart_init.USART_BaudRate = 9600;                                           // baudrate  : 9600
    usart_init.USART_WordLength = USART_WordLength_8b;                          // databit   : 8
    usart_init.USART_StopBits = USART_StopBits_1;                               // stopbit   : 1
    usart_init.USART_Parity = USART_Parity_No;                                  // parity    : none
    usart_init.USART_HardwareFlowControl = USART_HardwareFlowControl_None;      // flowctl   : none
    usart_init.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;                      // mode      : rx_tx

    /* Configure USART1 */
    USART_Init(USARTx, &usart_init);

    /* Enable USART1 Receive and Transmit interrupts */
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);                      // enable rx interrupt
    //USART_ITConfig(USART1, USART_IT_TXE, ENABLE);                     // enable tx buffer empty interrupt
    //USART_ITConfig(USART1, USART_IT_TC, ENABLE);                      // enable tx finish interrupt

    /* Enable the USART1 */
    USART_Cmd(USART1, ENABLE);
}


/******************************************************
  整形数据转字符串函数
  char *itoa(int value, char *string, int radix)
  radix=10 标示是10进制    非十进制，转换结果为0;

  例：d=-379;
  执行    itoa(d, buf, 10); 后

  buf="-379"
 **********************************************************/
char *itoa(int value, char *string, int radix)
{
    int i, d;
    int flag = 0;
    char *ptr = string;

    /* This implementation only works for decimal numbers. */
    if (radix != 10)
    {
        *ptr = 0;
        return string;
    }

    if (!value)
    {
        *ptr++ = 0x30;
        *ptr = 0;
        return string;
    }

    /* if this is a negative value insert the minus sign. */
    if (value < 0)
    {
        *ptr++ = '-';

        /* Make the value positive. */
        value *= -1;
    }

    for (i = 10000; i > 0; i /= 10)
    {
        d = value / i;

        if (d || flag)
        {
            *ptr++ = (char)(d + 0x30);
            value -= (d * i);
            flag = 1;
        }
    }

    /* Null terminate the string. */
    *ptr = 0;

    return string;

} /* NCL_Itoa */

/******************************************************
  格式化串口输出函数
  "\r"    回车符       USART_OUT(USART1, "abcdefg\r")
  "\n"    换行符       USART_OUT(USART1, "abcdefg\r\n")
  "%s"    字符串       USART_OUT(USART1, "字符串是：%s","abcdefg")
  "%d"    十进制       USART_OUT(USART1, "a=%d",10)
 **********************************************************/
void USART_OUT(USART_TypeDef* USARTx, uint8_t *Data, ...)
{
    const char *s;
    int d;
    char buf[16];
    va_list ap;

    va_start(ap, Data);

    while (*Data!=0)
    {
        if (*Data=='\\') {                                      // escape
            switch (*++Data){
                case 'r':                                       // enter
                    USART_SendData(USARTx, 0x0d);
                    Data++;
                    break;

                case 'n':                                       // new line
                    USART_SendData(USARTx, 0x0a);
                    Data++;
                    break;

                default:
                    Data++;
                    break;
            }
        }

        else if(*Data=='%') {
            switch (*++Data) {
                case 's':                                          //字符串
                    s = va_arg(ap, const char *);
                    for ( ; *s; s++) {
                        USART_SendData(USARTx, *s);
                        while (USART_GetFlagStatus(USARTx, USART_FLAG_TC) == RESET);
                    }
                    Data++;
                    break;

                case 'd':                                          //十进制
                    d = va_arg(ap, int);
                    itoa(d, buf, 10);
                    for (s = buf; *s; s++) {
                        USART_SendData(USARTx, *s);
                        while (USART_GetFlagStatus(USARTx, USART_FLAG_TC) == RESET);
                    }
                    Data++;
                    break;

                default:
                    Data++;
                    break;
            }
        }

        else {
            USART_SendData(USARTx, *Data++);
        }

        while(USART_GetFlagStatus(USARTx, USART_FLAG_TC) == RESET);
    }
}


void RCC_Configuration(void)
{
    /* Setup the microcontroller system.
     * Initialize the Embedded Flash Interface.
     * Initialize the PLL and update the SystemFrequency variable.
     */
    SystemInit();
}

/**
 * @brief  Configures the different GPIO ports.
 * @param  None
 * @retval : None
 */
void GPIO_Configuration(void)
{
    RCC_APB2PeriphClockCmd(
            RCC_APB2Periph_USART1 |
            RCC_APB2Periph_GPIOA |
            RCC_APB2Periph_GPIOB |
            RCC_APB2Periph_AFIO,
            ENABLE);

    /* led1 gpio PB5 */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* uart1 tx gpio PA9 */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* uart1 rx gpio PA10 */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}

/**
 * @brief  Configures the nested vectored interrupt controller.
 * @param  None
 * @retval : None
 */
void NVIC_Configuration(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);

    /* Enable the USART1 Interrupt */
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}


int main(void)
{
    uint32_t a = 0;

    /* System Clocks Configuration */
    RCC_Configuration();

    /* System interrupt source configuration */
    NVIC_Configuration();

    /* Setup AFIO/LED/UART1 gpio */
    GPIO_Configuration();

    /* USART1 configuration */
    USART_Config(USART1);

    USART_OUT(USART1,"**** STM32 WITH A7139 ****\r\n");
    USART_OUT(USART1,"\r\n");
    USART_OUT(USART1,"\r\n");

    //TIMER_Configuration();
    rf_dev.pin.scs.base  = GPIOB; rf_dev.pin.scs.pin = GPIO_Pin_15;
    rf_dev.pin.sck.base  = GPIOB; rf_dev.pin.sck.pin = GPIO_Pin_13;
    rf_dev.pin.sdio.base = GPIOB; rf_dev.pin.sdio.pin = GPIO_Pin_14;
    rf_dev.pin.gio1.base = GPIOA; rf_dev.pin.gio1.pin = GPIO_Pin_8;
    rf_dev.rf_currmode = A7139_MODE_STANDBY;
    rf_dev.rf_datarate = RF_DEF_RATE;
    rf_dev.rf_freq_ch = RF_DEF_FREQ_CH;
    rf_dev.rf_id[0] = RF_DEF_ID_D0;
    rf_dev.rf_id[1] = RF_DEF_ID_D1;

    a7139_chip_init(&rf_dev);
    a7139_freq_set(&rf_dev, 0);
    a7139_mode_switch(&rf_dev, A7139_MODE_RX);

    while (1)
    {
        delay(0xcffff);
        GPIO_ResetBits(GPIOB, GPIO_Pin_5);

        USART_OUT(USART1,"\r\n %d", a++);

        delay(0xcffff);
        GPIO_SetBits(GPIOB, GPIO_Pin_5);

        if (rec_f == 1)
        {
            rec_f = 0;
            USART_OUT(USART1,"\r\nYour send message is:\r\n");
            USART_OUT(USART1, &TxBuffer1[0]);
        }
    }
}

