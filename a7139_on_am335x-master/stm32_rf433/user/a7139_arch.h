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

#ifndef __A7139_ARCH_H__
#define __A7139_ARCH_H__

#include "misc.h"
#include "stm32f10x_gpio.h"

struct a7139_stm32_pin {
    GPIO_TypeDef *base;
    uint16_t pin;
};

void gpio_pin_mo_h(struct a7139_stm32_pin *gpio);
void gpio_pin_mo_l(struct a7139_stm32_pin *gpio);
void gpio_pin_h(struct a7139_stm32_pin *gpio);
void gpio_pin_l(struct a7139_stm32_pin *gpio);
void gpio_pin_mi(struct a7139_stm32_pin *gpio);
uint8_t gpio_pin_in(struct a7139_stm32_pin *gpio);
void delay(uint32_t nCount);
void spi_ndelay(uint32_t n);
void spi_mdelay(uint32_t n);
void spi_defdelay(void);

#endif  //__A7139_ARCH_H__

