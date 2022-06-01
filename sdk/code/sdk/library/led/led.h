#ifndef _LED_H_
#define _LED_H_

#include <stdint.h>
#include <unistd.h>
#include "hal_gpio.h"
#include "hal_pwm.h"

#define LED_MAX_BRIGHTNESS                  100


typedef uint8_t (*iic_idle_cb)(void);
typedef int (*iic_write_cb)(uint8_t addr, uint8_t reg, uint8_t* data, size_t size);
typedef void (*gradual_over_cb)(void* args);

typedef enum
{
    LED_RGB = 0,
    LED_RBG,
    LED_GBR,
    LED_GRB,
    LED_BGR,
    LED_BRG
} led_ic_e;

typedef enum
{
    LED_RGB_SPI = 0,
    LED_RGB_IIC
} led_control_e;

typedef enum
{
    CRC_SUM = 0,
    CRC_XOR
} led_crc_e;

typedef enum
{
    DUTY_CYCLE_1_7 = 0,
    DUTY_CYCLE_2_6,
    DUTY_CYCLE_3_5,
    DUTY_CYCLE_4_4,
    DUTY_CYCLE_5_3,
    DUTY_CYCLE_6_2,
    DUTY_CYCLE_7_1,
    DUTY_CYCLE_MAX
} led_duty_cycle_e;

typedef enum
{
    LED_DIRECT = 0,
    LED_GRADUAL,
    LED_WITHOUT_QUEUE = 0xff
} led_gradual_e;

typedef struct
{
    uint8_t enable;
    hal_port_e port;
    hal_port_bit_e bit;
    hal_pwm_channel_e channel;
    uint16_t duty;
} led_pwm_t;

typedef struct
{
    uint8_t ic_count;
    led_ic_e ic_type;
    uint8_t with_ww;
    uint8_t lumi_min;
    uint8_t refresh_base;
    uint32_t current_limit;
    led_pwm_t pwm_cw;
    led_pwm_t pwm_ww;
    led_control_e led_rgb_type;
    led_duty_cycle_e duty_cycle[2];
    hal_port_e spi_port;
    hal_port_bit_e spi_bit;
    uint32_t spi_speed;
    led_crc_e iic_crc;            //1: ^     0: +
    iic_idle_cb  iic_idle;
    iic_write_cb  iic_write;
} led_param_t;

typedef struct
{
    led_gradual_e type;
    uint8_t step;
    uint8_t speed;
    uint8_t b_proportion;
    gradual_over_cb callback;
    void* args;
} led_mode_t;

typedef struct
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t cw;
    uint8_t ww;
} led_color_t;


int LightSdk_led_data_set(uint8_t* data, uint32_t size, uint8_t cw, uint8_t ww, uint8_t bright, led_mode_t mode);
int LightSdk_led_color_set(led_color_t color, uint8_t bright, led_mode_t mode);
int LightSdk_led_init(led_param_t* param);
void LightSdk_led_deinit(void);

#endif
