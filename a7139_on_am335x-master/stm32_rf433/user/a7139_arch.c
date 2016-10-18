//*****************************************************************************
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
//*****************************************************************************

#include "stm32f10x_gpio.h"
#include "misc.h"
#include "a7139_arch.h"

void gpio_pin_mo_h(struct a7139_stm32_pin *gpio)
{
    GPIO_InitTypeDef gpio_init;

    gpio_init.GPIO_Pin = gpio->pin;
    gpio_init.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(gpio->base, &gpio_init);

    GPIO_SetBits(gpio->base, gpio->pin);
}

void gpio_pin_mo_l(struct a7139_stm32_pin *gpio)
{
    GPIO_InitTypeDef gpio_init;

    gpio_init.GPIO_Pin = gpio->pin;
    gpio_init.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(gpio->base, &gpio_init);

    GPIO_ResetBits(gpio->base, gpio->pin);
}

void gpio_pin_h(struct a7139_stm32_pin *gpio)
{
    GPIO_SetBits(gpio->base, gpio->pin);
}

void gpio_pin_l(struct a7139_stm32_pin *gpio)
{
    GPIO_ResetBits(gpio->base, gpio->pin);
}

void gpio_pin_mi(struct a7139_stm32_pin *gpio)
{
    GPIO_InitTypeDef gpio_init;

    gpio_init.GPIO_Pin = gpio->pin;
    gpio_init.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(gpio->base, &gpio_init);
}

uint8_t gpio_pin_in(struct a7139_stm32_pin *gpio)
{
    return GPIO_ReadInputDataBit(gpio->base, gpio->pin);
}

//DELAY
void delay(volatile uint32_t nCount)
{
    for(; nCount != 0; nCount--);
}

void spi_ndelay(uint32_t n)
{
    delay(n);
}

void spi_mdelay(uint32_t n)
{
    delay(n * 1000);
}

void spi_defdelay(void)
{
    spi_ndelay(80);
}

