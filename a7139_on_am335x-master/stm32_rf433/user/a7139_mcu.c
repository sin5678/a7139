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

#include "a7139_mcu.h"


const uint16_t a7139_reg_config[] =
{
    0x0823,     // 0x00, SYSTEM CLOCK register @see 9.2.1 System clock (Address: 00h)
                //      Fmsck = 12.8M, DMOS=1
                //      Fcsck = Fmsck/(CSC+1)=3200000, CSC=3
                //      SDR = Fcsck/rate/64-1
    0x0A24,     // 0x01, PLL1 register,
    0xB805,     // 0x02, PLL2 register, 470.001MHz
    0x0000,     // 0x03, PLL3 register,
    0x0A20,     // 0x04, PLL4 register,
    0x0024,     // 0x05, PLL5 register,
    0x0000,     // 0x06, PLL6 register,
    0x0001,     // 0x07, CRYSTAL register,
    0x0000,     // 0x08, PAGEA,
    0x0000,     // 0x09, PAGEB,
    0x1850,     // 0x0a, RX1 register,
                //      0x18D0: IFBW=50KHz, ETH=1
                //      0x1850: IFBW=50KHz, ETH=0
    0x7009,     // 0x0b, RX2 register, by preamble
    0x4400,     // 0x0c, ADC register,
    0x0800,     // 0x0d, PIN CONTROL register, Use Strobe CMD
    0x4845,     // 0x0e, CALIBRATION register,
    0x20C0      // 0x0f, MODE CONTROL register, Use FIFO mode
};

const uint16_t a7139_reg_config_page_a[] =
{
    0x1703,     // 0x00, TX1 register, Fdev = 18.75kHz 0x1703--0x1260
                //       0x1706, S//TX1 register, Fdev = 37.5kHz
    0x0000,     // 0x01, WOR1 register,
    0x0000,     // 0x02, WOR2 register,
    0x1187,     // 0x03, RFI register, Enable Tx Ramp up/down
    0x8160,     // 0x04, PM register, CST=1
    0x0302,     // 0x05, RTH register,
    0x400F,     // 0x06, AGC1 register,
    0x0AC0,     // 0x07, AGC2 register,
    0x0041,     // 0x08, GIO register, GIO2=WTR, GIO1=WTR
    0xD281,     // 0x09, CKO register
    0x0004,     // 0x0a, VCB register,
    0x0825,     // 0x0b, CHG1 register, 480MHz
    0x0127,     // 0x0c, CHG2 register, 500MHz
    0x003F,     // 0x0d, FIFO register, FEP=63+1=64bytes
    0x150B,     // 0x0e, CODE register,
                //      0x151F:Preamble=4bytes, ID=4bytes, FEC+CRC;
                //      0x150F:Preamble=4bytes, ID=4bytes, CRC;
                //      0x1519:Preamble=2bytes, ID=2bytes, FEC+CRC;
                //      0x1509:Preamble=2bytes, ID=2bytes, CRC;
                //      0x150B:Preamble=4bytes, ID=2bytes, CRC;
    0x0000      // 0x0f, WCAL register,
};

const uint16_t a7139_reg_config_page_b[] =
{
    0x0B37,     // TX2 register, Tx power=20dBm /*0x0B7F-->0x0B37 733 721功率测试*/
    0x8200,     // IF1 register, Enable Auto-IF, IF=100KHz 0x8400,IF=200KHz
    0x0000,     // IF2 register,
    0x0000,     // ACK register,
    0x0000      // ART register,
};

const uint16_t a7139_rate_cal_table[] = {
    0x3023,     // 2k
    0x1223,     // 5k
    0x0823,     // 10k
    0x0223,     // 25k
    0x0023,     // 50k
};

const uint16_t a7139_freq_cal_table[RF_FREQ_TAB_MAXSIZE*2] = {
#if 0
    0x0A24, 0xBA05,     // 470.101MHz
    0x0A26, 0x4805,     // 490.001MHz
    0x0A27, 0xD805,     // 510.001MHz
#endif
    0x0A24, 0xBA05,     // 00, 470.101MHz
    0x0A24, 0xE005,     // 01, 472.001MHz
    0x0A25, 0x0805,     // 02, 474.001MHz
    0x0A25, 0x3005,     // 03, 476.001MHz
    0x0A25, 0x5805,     // 04, 478.001MHz
    0x0A25, 0x7FFF,     // 05, 480.000MHz
    0x0A25, 0xA805,     // 06, 482.001MHz
    0x0A25, 0xD005,     // 07, 484.001MHz
    0x0A25, 0xF805,     // 08, 486.001MHz
    0x0A26, 0x2005,     // 09, 488.001MHz
    0x0A26, 0x4805,     // 10, 490.001MHz
    0x0A26, 0x7005,     // 11, 492.001MHz
    0x0A26, 0x9805,     // 12, 494.001MHz
    0x0A26, 0xC005,     // 13, 496.001MHz
    0x0A26, 0xE805,     // 14, 498.001MHz
    0x0A27, 0x1005,     // 15, 500.001MHz
};


void a7139_byte_send(struct a7139_dev *dev, uint8_t src)
{
    uint8_t i;

    for (i = 0; i < 8; i++) {
        if (src & 0x80) {
            gpio_pin_h(&dev->pin.sdio);
        } else {
            gpio_pin_l(&dev->pin.sdio);
        }

        spi_defdelay();
        gpio_pin_h(&dev->pin.sck);
        spi_defdelay();
        gpio_pin_l(&dev->pin.sck);

        src = src << 1;
    }
}

uint8_t a7139_byte_read(struct a7139_dev *dev)
{
    uint8_t i, tmp = 0;

    gpio_pin_h(&dev->pin.sdio);
    gpio_pin_mi(&dev->pin.sdio);

    for (i = 0; i < 8; i++)
    {
        if (gpio_pin_in(&dev->pin.sdio)) {
            tmp = (tmp << 1) | 0x01;
        } else {
            tmp = tmp << 1;
        }

        spi_defdelay();
        gpio_pin_h(&dev->pin.sck);
        spi_defdelay();
        gpio_pin_l(&dev->pin.sck);
    }

    gpio_pin_mo_l(&dev->pin.sdio);

    return tmp;
}

void a7139_send_ctrl(struct a7139_dev *dev, uint8_t cmd)
{
    gpio_pin_l(&dev->pin.scs);
    gpio_pin_mo_l(&dev->pin.sdio);

    a7139_byte_send(dev, cmd);

    gpio_pin_h(&dev->pin.scs);
}

void a7139_write_reg(struct a7139_dev *dev, uint8_t address, uint16_t dataWord)
{
    uint8_t dataWord1;
    uint8_t dataWord0;

    dataWord0 = (dataWord) & 0x00ff;
    dataWord1 = (dataWord >> 8) & 0x00ff;

    gpio_pin_l(&dev->pin.scs);
    gpio_pin_mo_h(&dev->pin.sdio);

    address |= CMD_CTRLW;
    a7139_byte_send(dev, address);

    spi_defdelay();

    a7139_byte_send(dev, dataWord1);
    a7139_byte_send(dev, dataWord0);

    gpio_pin_h(&dev->pin.scs);
}

uint16_t a7139_read_reg(struct a7139_dev *dev, uint8_t address)
{
    uint16_t tmp;

    gpio_pin_l(&dev->pin.scs);
    gpio_pin_mo_h(&dev->pin.sdio);

    address |= CMD_CTRLR;
    a7139_byte_send(dev, address);

    spi_defdelay();

    tmp = a7139_byte_read(dev) << 8;
    tmp = (tmp & 0xff00) | (a7139_byte_read(dev) & 0x00ff);

    gpio_pin_h(&dev->pin.scs);
    gpio_pin_mo_h(&dev->pin.sdio);

    return tmp;
}

void a7139_write_page_a(struct a7139_dev *dev, uint8_t address, uint16_t dataWord)
{
    uint16_t tmp;

    tmp = address;
    tmp = ((tmp << 12) | a7139_reg_config[CRYSTAL_REG]);

    a7139_write_reg(dev, CRYSTAL_REG, tmp);
    a7139_write_reg(dev, PAGEA_REG, dataWord);
}

uint16_t a7139_read_page_a(struct a7139_dev *dev, uint8_t address)
{
    uint16_t tmp;

    tmp = address;
    tmp = ((tmp << 12) | a7139_reg_config[CRYSTAL_REG]);

    a7139_write_reg(dev, CRYSTAL_REG, tmp);
    tmp = a7139_read_reg(dev, PAGEA_REG);

    return tmp;
}

void a7139_write_page_b(struct a7139_dev *dev, uint8_t address, uint16_t dataWord)
{
    uint16_t tmp;

    tmp = address;
    tmp = ((tmp << 7) | a7139_reg_config[CRYSTAL_REG]);

    a7139_write_reg(dev, CRYSTAL_REG, tmp);
    a7139_write_reg(dev, PAGEB_REG, dataWord);
}

uint16_t a7139_read_page_b(struct a7139_dev *dev, uint8_t address)
{
    uint16_t tmp;

    tmp = address;
    tmp = ((tmp << 7) | a7139_reg_config[CRYSTAL_REG]);

    a7139_write_reg(dev, CRYSTAL_REG, tmp);
    tmp = a7139_read_reg(dev, PAGEB_REG);

    return tmp;
}

int a7139_config(struct a7139_dev *dev)
{
    uint8_t i;
    uint16_t tmp;

    for (i = 0; i < 8; i++) {
        a7139_write_reg(dev, i, a7139_reg_config[i]);
    }

    for (i = 10; i < 16; i++) {
        a7139_write_reg(dev, i, a7139_reg_config[i]);
    }

    for (i = 0; i < 16; i++) {
        a7139_write_page_a(dev, i, a7139_reg_config_page_a[i]);
    }

    for (i = 0; i < 5; i++) {
        a7139_write_page_b(dev, i, a7139_reg_config_page_b[i]);
    }

    // for check
    tmp = a7139_read_reg(dev, SYSTEMCLOCK_REG);
    if (tmp != a7139_reg_config[SYSTEMCLOCK_REG]) {
        //a7139_reg_dump(dev);
        return -1;
    }

    return 0;
}

int a7139_write_id(struct a7139_dev *dev, uint8_t *id)
{
    uint8_t i;
    int ret = 0;

    gpio_pin_l(&dev->pin.scs);
    a7139_byte_send(dev, CMD_IDW);

    for (i = 0; i < RF_IDSIZE; i++) {
        dev->rf_id[i] = id[i];
        a7139_byte_send(dev, dev->rf_id[i]);
    }

    gpio_pin_h(&dev->pin.scs);
    gpio_pin_l(&dev->pin.scs);

    a7139_byte_send(dev, CMD_IDR);

    for (i = 0; i < RF_IDSIZE; i++) {
        ret += (a7139_byte_read(dev) == dev->rf_id[i] ? 0 : -1);
    }

    gpio_pin_h(&dev->pin.scs);

    return ret;
}

int a7139_read_id(struct a7139_dev *dev, uint8_t *id)
{
    uint8_t i;
    int ret = 0;

    gpio_pin_l(&dev->pin.scs);

    a7139_byte_send(dev, CMD_IDR);

    for (i = 0; i < RF_IDSIZE; i++) {
        ret += ((id[i] = a7139_byte_read(dev)) == dev->rf_id[i] ? 0 : -1);
    }

    gpio_pin_h(&dev->pin.scs);

    return ret;
}

void a7139_mode_switch(struct a7139_dev *dev, A7139_MODE mode)
{
    switch (mode)
    {
        case A7139_MODE_SLEEP:
            a7139_send_ctrl(dev, CMD_SLEEP_MODE);
            dev->rf_currmode = A7139_MODE_SLEEP;
            break;

        case A7139_MODE_IDLE:
            a7139_send_ctrl(dev, CMD_IDLE_MODE);
            dev->rf_currmode = A7139_MODE_IDLE;
            break;

        case A7139_MODE_STANDBY:
            a7139_send_ctrl(dev, CMD_STANDBY_MODE);
            dev->rf_currmode = A7139_MODE_STANDBY;
            break;

        case A7139_MODE_PLL:
            a7139_send_ctrl(dev, CMD_PLL_MODE);
            dev->rf_currmode = A7139_MODE_PLL;
            break;

        case A7139_MODE_RX:
            a7139_send_ctrl(dev, CMD_STANDBY_MODE);
            spi_mdelay(1);
            a7139_send_ctrl(dev, CMD_RX_MODE);
            dev->rf_currmode = A7139_MODE_RX;
            break;

        case A7139_MODE_RXING:
            a7139_send_ctrl(dev, CMD_STANDBY_MODE);
            dev->rf_currmode = A7139_MODE_RXING;
            break;

        case A7139_MODE_TX:
            a7139_send_ctrl(dev, CMD_TX_MODE);
            dev->rf_currmode = A7139_MODE_TX;
            break;

        case A7139_MODE_TXING:
            a7139_send_ctrl(dev, CMD_STANDBY_MODE);
            dev->rf_currmode = A7139_MODE_TXING;
            break;

        case A7139_MODE_DEEPSLEEPT:
            a7139_send_ctrl(dev, CMD_DEEP_SLEEP_T);
            dev->rf_currmode = A7139_MODE_DEEPSLEEPT;
            break;

        case A7139_MODE_DEEPSLEEPP:
            a7139_send_ctrl(dev, CMD_DEEP_SLEEP_P);
            dev->rf_currmode = A7139_MODE_DEEPSLEEPP;
            break;

        default:
            break;
    }
}

int a7139_datarate_set(struct a7139_dev *dev, A7139_RATE drate)
{
    if (drate >= A7139_RATE_MAX) {
        return -1;
    }

    a7139_write_reg(dev, SYSTEMCLOCK_REG, a7139_rate_cal_table[drate]);

    dev->rf_datarate = drate;

    return 0;
}

int a7139_freq_set(struct a7139_dev *dev, uint8_t ch)
{

    if (ch >= RF_FREQ_TAB_MAXSIZE) {
        return -1;
    }

#if 1
    a7139_write_reg(dev, PLL1_REG, a7139_freq_cal_table[2 * ch]);       // setting PLL1
    a7139_write_reg(dev, PLL2_REG, a7139_freq_cal_table[2 * ch + 1]);   // setting PLL2
#else
    uint16_t freq;
    freq = ch * 16 * 4;                                         // per 4 * 200k one channel
    a7139_write_page_b(IF2_PAGEB, freq);                        // setting for frequency offset
#endif
    dev->rf_freq_ch = ch;

    return 0;
}

int a7139_cal(struct a7139_dev *dev)
{
    uint8_t fbcf; // IF Filter
    uint8_t vbcf; // VCO Current
    uint8_t vccf; // VCO Band
    uint16_t tmp;

    // IF calibration procedure @STB state
    // IF Filter & VCO Current Calibration
    a7139_write_reg(dev, MODE_REG, a7139_reg_config[MODE_REG] | 0x0802);
    do {
        tmp = a7139_read_reg(dev, MODE_REG);
    } while (tmp & 0x0802);

    // for check(IF Filter)
    tmp = a7139_read_reg(dev, CALIBRATION_REG);
    fbcf = (tmp >> 4) & 0x01;
    if (fbcf) {
        return -2;
    }

    // for check(VCO Current)
    tmp = a7139_read_page_a(dev, VCB_PAGEA);
    vccf = (tmp >> 4) & 0x01;
    if (vccf) {
        return -2;
    }

    // RSSI Calibration procedure @STB state
    a7139_write_reg(dev, ADC_REG, 0x4C00);           // set ADC average=64
    a7139_write_page_a(dev, WOR2_PAGEA, 0xF800);     // set RSSC_D=40us and RS_DLY=80us
    a7139_write_page_a(dev, TX1_PAGEA, a7139_reg_config_page_a[TX1_PAGEA] | 0xE000);  // set RC_DLY=1.5ms
    a7139_write_reg(dev, MODE_REG, a7139_reg_config[MODE_REG] | 0x1000);              // RSSI Calibration

    do {
        tmp = a7139_read_reg(dev, MODE_REG);
    } while (tmp & 0x1000);

    a7139_write_reg(dev, ADC_REG, a7139_reg_config[ADC_REG]);
    a7139_write_page_a(dev, WOR2_PAGEA, a7139_reg_config_page_a[WOR2_PAGEA]);
    a7139_write_page_a(dev, TX1_PAGEA, a7139_reg_config_page_a[TX1_PAGEA]);

    // VCO calibration procedure @STB state
    a7139_write_reg(dev, PLL1_REG, a7139_freq_cal_table[0]);
    a7139_write_reg(dev, PLL2_REG, a7139_freq_cal_table[1]);
    a7139_write_reg(dev, MODE_REG, a7139_reg_config[MODE_REG] | 0x0004);              // VCO Band Calibration
    do {
        tmp = a7139_read_reg(dev, MODE_REG);
    } while(tmp & 0x0004);

    // for check(VCO Band)
    tmp = a7139_read_reg(dev, CALIBRATION_REG);
    vbcf = (tmp >> 8) & 0x01;
    if (vbcf) {
        return -2;
    }

    return 0;
}

int a7139_chip_init(struct a7139_dev *dev)
{
    // init io pin
    gpio_pin_mo_h(&dev->pin.scs);
    gpio_pin_mo_l(&dev->pin.sck);
    gpio_pin_mo_h(&dev->pin.sdio);
    gpio_pin_mi(&dev->pin.gio1);

    a7139_send_ctrl(dev, CMD_RFRST);   // reset  chip

    spi_defdelay();

    if (a7139_config(dev)) {            // config A7139 chip
        return -2;
    }

    spi_defdelay();                     // for crystal stabilized

    if (a7139_write_id(dev, dev->rf_id)) {  // write ID code
        return -2;
    }

    if (a7139_cal(dev)) {
        return -2;                    // IF and VCO calibration
    }

    spi_defdelay();

    a7139_freq_set(dev, dev->rf_freq_ch);
    //a7139_datarate_set(dev, dev->rf_datarate);

    return 0;
}

void a7139_send_packet(struct a7139_dev *dev, uint8_t *txBuffer, uint8_t size)
{
    uint8_t i;

    a7139_mode_switch(dev, A7139_MODE_STANDBY);     // enter standby mode

    spi_defdelay();
    a7139_send_ctrl(dev, CMD_TFR);                  // TX FIFO address pointer reset

    gpio_pin_l(&dev->pin.scs);
    a7139_byte_send(dev, CMD_DATAW);                // TX FIFO write command

    for (i = 0; i < size; i++) {
        a7139_byte_send(dev, txBuffer[i]);
    }
    gpio_pin_h(&dev->pin.scs);

    a7139_mode_switch(dev, A7139_MODE_TX);
}

uint8_t a7139_receive_packet(struct a7139_dev *dev, uint8_t *buf, uint8_t len)
{
    uint8_t i;

    a7139_send_ctrl(dev, CMD_RFR);      // RX FIFO address pointer reset

    gpio_pin_l(&dev->pin.scs);

    a7139_byte_send(dev, CMD_DATAR);    // RX FIFO read command

    for (i = 0; i < len; i++) {
        buf[i] = a7139_byte_read(dev);
    }

    gpio_pin_h(&dev->pin.scs);

    return i;
}


