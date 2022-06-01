#include <string.h>
#include <stdlib.h>
#include "mcu_i2c.h"
#include "driver_gpio.h"
#include "sys_utils.h"


typedef struct _mcu_i2c_port_t
{
    hal_port_t SCL;
    hal_port_t SDA;
} i2c_port_t;

static i2c_port_t gt_mcu_i2c = {{GPIO_PORT_D,GPIO_BIT_0}, {GPIO_PORT_D,GPIO_BIT_1}};

#define PORT_FUNC_GPIO      0x00

#define I2C_SCL_PIN		    gt_mcu_i2c.SCL.GPIO_Pin
#define SCL_SET_OUTPUT()	(*(volatile uint32_t *)(GPIO_PORTD_DIR) &= ~(1<<I2C_SCL_PIN)) // gpio_set_dir((I2C_SCL_PIN/8),(I2C_SCL_PIN%8),0)
#define SCL_SET_INPUT()	    (*(volatile uint32_t *)(GPIO_PORTD_DIR) |= (1<<I2C_SCL_PIN))
#define SCL_SET_HIGH()	    (*(volatile uint32_t *)(GPIO_PORTD_DATA) |= (1<<I2C_SCL_PIN))
#define SCL_SET_LOW()	    (*(volatile uint32_t *)(GPIO_PORTD_DATA) &= ~(1<<I2C_SCL_PIN))
#define SCL_GET_VAL()	    (*(volatile uint32_t *)(GPIO_PORTD_DATA) & (1<<I2C_SCL_PIN))

#define I2C_SDA_PIN		    gt_mcu_i2c.SDA.GPIO_Pin
#define SDA_SET_OUTPUT()	(*(volatile uint32_t *)(GPIO_PORTD_DIR) &= ~(1<<I2C_SDA_PIN)) // gpio_set_dir((I2C_SCL_PIN/8),(I2C_SCL_PIN%8),0)
#define SDA_SET_INPUT()	    (*(volatile uint32_t *)(GPIO_PORTD_DIR) |= (1<<I2C_SDA_PIN))
#define SDA_SET_HIGH()	    (*(volatile uint32_t *)(GPIO_PORTD_DATA) |= (1<<I2C_SDA_PIN))
#define SDA_SET_LOW()	    (*(volatile uint32_t *)(GPIO_PORTD_DATA) &= ~(1<<I2C_SDA_PIN))
#define SDA_GET_VAL()	    (*(volatile uint32_t *)(GPIO_PORTD_DATA) & (1<<I2C_SDA_PIN))
#define I2C_DELAY()         {__NOP();}

#define I2C_ACK_OP()        SCL_SET_HIGH();I2C_DELAY();I2C_DELAY();I2C_DELAY();I2C_DELAY();I2C_DELAY();\
                            I2C_DELAY();I2C_DELAY();I2C_DELAY();I2C_DELAY();I2C_DELAY();SCL_SET_LOW() //;I2C_DELAY()
#define SDA_SET_HIGH_EXT()  SDA_SET_HIGH();I2C_ACK_OP()
#define SDA_SET_LOW_EXT()   SDA_SET_LOW();I2C_ACK_OP()


static void i2c_start(void)
{
    // Make sure both SDA and SCL high.
    SDA_SET_HIGH();
    I2C_DELAY();
    SCL_SET_HIGH();
    I2C_DELAY();

    // SDA: \__
    SDA_SET_LOW();
    I2C_DELAY();

    // SCL: \__
    SCL_SET_LOW();//delay_us(2);
    I2C_DELAY();
}

static void i2c_stop(void)
{
    // Make sure SDA low.
    SDA_SET_LOW();
    I2C_DELAY();

    // SCL: __/
    SCL_SET_HIGH();
    I2C_DELAY();
    // SDA: __/
    SDA_SET_HIGH();
    I2C_DELAY();
}

static void i2c_read_byte(uint8_t *data, uint8_t ack)
{
    uint8_t i, byte_read = 0;

    // Before call the function, SCL should be low.
    // Make sure SDA is an input
    SDA_SET_INPUT();

    // MSB first
    for (i = 0x80; i != 0; i >>= 1)
    {
        SCL_SET_HIGH();
        I2C_DELAY();

        if (1 == (uint8_t)(SDA_GET_VAL()))
        {
            byte_read |= i;
        }

        SCL_SET_LOW();
        I2C_DELAY();
    }

    // Make sure SDA is an output before we exit the function
    SDA_SET_OUTPUT();

    *data = byte_read;

    // Send ACK bit
    // SDA high == NACK, SDA low == ACK
    if (ack)
    {
        SDA_SET_LOW();
    }
    else
    {
        SDA_SET_HIGH();
    }

    // Let SDA line settle for a moment
    I2C_DELAY();

    // Drive SCL high to start ACK/NACK bit transfer
    SCL_SET_HIGH();
    I2C_DELAY();

    // Finish ACK/NACK bit clock cycle and give slave a moment to react
    SCL_SET_LOW();
    I2C_DELAY();
}

static uint8_t i2c_write_byte(uint8_t data)
{
    uint8_t ret = 1;
    uint8_t i;

    for (i = 0x80; i != 0; i >>= 1)
    {
        if (data & i)
        {
            SDA_SET_HIGH();
        }
        else
        {
            SDA_SET_LOW();
        }
        I2C_DELAY();

        SCL_SET_HIGH();
        I2C_DELAY();
        I2C_DELAY();
        I2C_DELAY();
        I2C_DELAY();
        SCL_SET_LOW();
        I2C_DELAY();
    }

    // Configure SDA pin as input for receiving the ACK bit
    //SDA_SET_HIGH();
    SDA_SET_INPUT();
    I2C_DELAY();
    SCL_SET_HIGH();

    //raed ack
    if (1==(uint8_t)(SDA_GET_VAL()))
    {
        ret = 0;
    }

    // Finish ACK/NACK bit clock cycle and give slave a moment to release control
    // of the SDA line
    SCL_SET_LOW();//delay_us(2);
    I2C_DELAY();

    // Configure SDA pin as output as other module functions expect that
    SDA_SET_HIGH();
    SDA_SET_OUTPUT();

    return ret;
}

static uint8_t i2c_write_byte_ext(uint8_t data)
{
    uint8_t ret = 1;

    // Before call the function, SCL should be low.
    if (data & 0x80)
    {
        SDA_SET_HIGH_EXT();
    }
    else
    {
        SDA_SET_LOW_EXT();
    }

    if (data & 0x40)
    {
        SDA_SET_HIGH_EXT();
    }
    else
    {
        SDA_SET_LOW_EXT();
    }

    if (data & 0x20)
    {
        SDA_SET_HIGH_EXT();
    }
    else
    {
        SDA_SET_LOW_EXT();
    }

    if (data & 0x10)
    {
        SDA_SET_HIGH_EXT();
    }
    else
    {
        SDA_SET_LOW_EXT();
    }

    if (data & 0x08)
    {
        SDA_SET_HIGH_EXT();
    }
    else
    {
        SDA_SET_LOW_EXT();
    }

    if (data & 0x04)
    {
        SDA_SET_HIGH_EXT();
    }
    else
    {
        SDA_SET_LOW_EXT();
    }

    if (data & 0x02)
    {
        SDA_SET_HIGH_EXT();
    }
    else
    {
        SDA_SET_LOW_EXT();
    }

    if (data & 0x01)
    {
        SDA_SET_HIGH_EXT();
    }
    else
    {
        SDA_SET_LOW_EXT();
    }

    // Configure SDA pin as input for receiving the ACK bit
    //SDA_SET_HIGH();
    SDA_SET_INPUT();
    //I2C_DELAY();
    SCL_SET_HIGH();

    //raed ack
    if (1 == (uint8_t)(SDA_GET_VAL()))
    {
        ret = 0;
    }

    // Finish ACK/NACK bit clock cycle and give slave a moment to release control
    // of the SDA line
    SCL_SET_LOW();//delay_us(2);
    I2C_DELAY();

    // Configure SDA pin as output as other module functions expect that
    SDA_SET_OUTPUT();
    SDA_SET_HIGH();

    return ret;
}

static void drv_i2c_write_gpio(uint8_t i2cAddr, uint8_t reg, uint8_t *pBuf, uint16_t len)
{
    i2c_start();
    i2c_write_byte_ext(i2cAddr);
    i2c_write_byte_ext(0);
    i2c_write_byte_ext(reg);

    while (len --)
    {
        i2c_write_byte_ext(*pBuf ++);
    }

    i2c_stop();
}

static void drv_i2c_read_gpio(uint8_t i2cAddr, uint8_t reg, uint8_t *pBuf, uint8_t len)
{
    i2c_start();
    i2c_write_byte(i2cAddr);
    i2c_write_byte(reg);

    i2c_start();
    i2c_write_byte(i2cAddr + 1);

    while (1)
    {
        if (len <= 1)
        {
            i2c_read_byte(pBuf, 0);
            break;
        }
        else
        {
            i2c_read_byte(pBuf, 1);
            len --;
        }

        pBuf ++;
        co_delay_10us(1);
    }

    i2c_stop();
}


int LightSdk_mcu_i2c_init(hal_port_t scl, hal_port_t sda, uint16_t rate)
{
    gt_mcu_i2c.SCL = scl;
    gt_mcu_i2c.SDA = sda;

    system_set_port_mux(gt_mcu_i2c.SCL.GPIO_Port, gt_mcu_i2c.SCL.GPIO_Pin, PORT_FUNC_GPIO);
    gpio_set_dir(gt_mcu_i2c.SCL.GPIO_Port, gt_mcu_i2c.SCL.GPIO_Pin, GPIO_DIR_OUT);
    system_set_port_pull(GPIO2PORT(gt_mcu_i2c.SCL.GPIO_Port, gt_mcu_i2c.SCL.GPIO_Pin), true);
    gpio_set_pin_value(gt_mcu_i2c.SCL.GPIO_Port, gt_mcu_i2c.SCL.GPIO_Pin, 1);

    system_set_port_mux(gt_mcu_i2c.SDA.GPIO_Port, gt_mcu_i2c.SDA.GPIO_Pin, PORT_FUNC_GPIO);
    gpio_set_dir(gt_mcu_i2c.SDA.GPIO_Port, gt_mcu_i2c.SDA.GPIO_Pin, GPIO_DIR_OUT);
    system_set_port_pull(GPIO2PORT(gt_mcu_i2c.SDA.GPIO_Port, gt_mcu_i2c.SDA.GPIO_Pin), true);
    gpio_set_pin_value(gt_mcu_i2c.SDA.GPIO_Port, gt_mcu_i2c.SDA.GPIO_Pin, 1);
    return 0;
}

int LightSdk_mcu_i2c_write(uint8_t addr, uint8_t reg, uint8_t* buffer, uint32_t len)
{
    drv_i2c_write_gpio(addr, reg, buffer, (uint16_t)len&0xffff);

    return 0;
}

