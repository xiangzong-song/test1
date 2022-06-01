#include <stdlib.h>
#include "driver_iomux.h"
#include "driver_gpio.h"
#include "sys_utils.h"
#include "soft_i2c.h"


static hal_port_t g_scl_pin = {GPIO_PORT_A, GPIO_BIT_0};
static hal_port_t g_sda_pin = {GPIO_PORT_A, GPIO_BIT_1};

#define PORT_FUNC_GPIO      0x00

#define SCL_SET_OUTPUT()    gpio_set_dir(g_scl_pin.GPIO_Port, g_scl_pin.GPIO_Pin, GPIO_DIR_OUT)
#define SCL_SET_INPUT()     gpio_set_dir(g_scl_pin.GPIO_Port, g_scl_pin.GPIO_Pin, GPIO_DIR_IN)
#define SCL_SET_HIGH()      gpio_set_pin_value(g_scl_pin.GPIO_Port, g_scl_pin.GPIO_Pin, 1)
#define SCL_SET_LOW()       gpio_set_pin_value(g_scl_pin.GPIO_Port, g_scl_pin.GPIO_Pin, 0)
#define SCL_GET_VAL()       gpio_get_pin_value(g_scl_pin.GPIO_Port, g_scl_pin.GPIO_Pin)
#define SDA_SET_OUTPUT()    gpio_set_dir(g_sda_pin.GPIO_Port, g_sda_pin.GPIO_Pin, GPIO_DIR_OUT)
#define SDA_SET_INPUT()     gpio_set_dir(g_sda_pin.GPIO_Port, g_sda_pin.GPIO_Pin, GPIO_DIR_IN)
#define SDA_SET_HIGH()      SDA_SET_INPUT()     //use hardware pull up, avoid middle electrical level
#define SDA_SET_LOW()       SDA_SET_OUTPUT(); gpio_set_pin_value(g_sda_pin.GPIO_Port, g_sda_pin.GPIO_Pin, 0)
#define SDA_GET_VAL()       gpio_get_pin_value(g_sda_pin.GPIO_Port, g_sda_pin.GPIO_Pin)
#define I2C_DELAY()         {__NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP();\
                             __NOP();__NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP();\
                             __NOP();__NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP();\
                             __NOP();__NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP();\
                             __NOP();__NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP();\
                             __NOP();__NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP();\
                             __NOP();__NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP();}


static void i2c_start(void)
{
    // Make sure both SDA and SCL high.
    SDA_SET_HIGH();
    I2C_DELAY();
    SCL_SET_HIGH();
    I2C_DELAY();
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
    I2C_DELAY();
    // SDA: __/
    SDA_SET_HIGH();
    I2C_DELAY();
}

static void i2c_read_byte(uint8_t *data, uint8_t ack)
{
    uint8_t i, byte_read = 0;

    SDA_SET_INPUT();

    for (i = 0x80; i != 0; i >>= 1)
    {
        SCL_SET_HIGH();
        I2C_DELAY();
        I2C_DELAY();
        __NOP();__NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP();\
        if (1 == (uint8_t)(SDA_GET_VAL()))
        {
            byte_read |= i;
        }

        SCL_SET_LOW();
        I2C_DELAY();
        I2C_DELAY();
        __NOP();__NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP();
    }

    SDA_SET_OUTPUT();
    *data = byte_read;

    if (ack)
    {
        SDA_SET_LOW();
    }
    else
    {
        SDA_SET_HIGH();
    }

    I2C_DELAY();

    SCL_SET_HIGH();
    I2C_DELAY();
    I2C_DELAY();
    __NOP();__NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP();
    SCL_SET_LOW();
    I2C_DELAY();
}

static uint8_t i2c_write_byte(uint8_t data)
{
    uint8_t i, ret = 1;

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
    SDA_SET_INPUT();
    I2C_DELAY();
    SCL_SET_HIGH();
    I2C_DELAY();
    I2C_DELAY();
    __NOP();__NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP(); __NOP();
    //raed ack
    if (1 == (uint8_t)(SDA_GET_VAL()))
    {
        ret = 0;
    }

    // Finish ACK/NACK bit clock cycle and give slave a moment to release control
    // of the SDA line
    SCL_SET_LOW();
    I2C_DELAY();

    // Configure SDA pin as output as other module functions expect that
    SDA_SET_HIGH();
    I2C_DELAY();
    SDA_SET_OUTPUT();

    return ret;
}

int LightSdk_software_i2c_init(hal_port_t scl, hal_port_t sda, uint32_t speed)
{
    g_scl_pin = scl;
    g_sda_pin = sda;

    system_set_port_mux(g_scl_pin.GPIO_Port, g_scl_pin.GPIO_Pin, PORT_FUNC_GPIO);
    system_set_port_mux(g_sda_pin.GPIO_Port, g_sda_pin.GPIO_Pin, PORT_FUNC_GPIO);
    system_set_port_pull(GPIO2PORT(g_scl_pin.GPIO_Port, g_scl_pin.GPIO_Pin), true);
    system_set_port_pull(GPIO2PORT(g_sda_pin.GPIO_Port, g_sda_pin.GPIO_Pin), true);
    gpio_set_dir(g_scl_pin.GPIO_Port, g_scl_pin.GPIO_Pin, GPIO_DIR_OUT);
    gpio_set_dir(g_sda_pin.GPIO_Port, g_sda_pin.GPIO_Pin, GPIO_DIR_OUT);

    return 0;
}

int LightSdk_software_i2c_write(uint8_t addr, uint8_t reg, uint8_t* data, size_t size)
{
    i2c_start();
    i2c_write_byte(addr);
    i2c_write_byte(reg);

    while (size--)
    {
        i2c_write_byte(*data++);
    }

    i2c_stop();

    return 0;
}

int LightSdk_software_i2c_read(uint8_t addr, uint8_t reg, uint8_t* buffer, size_t size)
{
    i2c_start();
    i2c_write_byte(addr);
    i2c_write_byte(reg);

    i2c_start();
    i2c_write_byte(addr + 1);

    while (1)
    {
        if (size <= 1)
        {
            i2c_read_byte(buffer, 0);
            break;
        }
        else
        {
            i2c_read_byte(buffer, 1);
            size--;
        }

        buffer++;

        co_delay_10us(1);
    }

    i2c_stop();

    return 0;
}
