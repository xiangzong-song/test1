#include <stdio.h>
#include <string.h>
#include "hal_os.h"
#include "driver_iomux.h"
#include "driver_ssp.h"
#include "driver_gpio.h"
#include "driver_pwm.h"
#include "sys_utils.h"
#include "device_time.h"
#include "runtime.h"
#include "log.h"
#include "led.h"


#define LED_PIN_FUNC_SSP_OUT            0x04
#define LED_PIN_FUNC_PWM                0x03
#define LED_IC_MAX                      255
#define LED_LUMI_IN_GRADUAL             0xf0
#define GV_ABS(x, y)                    ((x > y) ? (x - y) : (y - x))

#define I2C_ADDR                        0xcc
#define I2C_REG                         0x04
#define FRAME_SOFT                      0x3a
#define FRAME_TYPE_COLOR                0x01

typedef enum
{
    LED_FREE = 0,
    LED_BUSY = 1
} led_state_e;

typedef struct
{
    led_mode_t mode;
    uint8_t bright;
    uint8_t delay;
    uint8_t cw;
    uint8_t ww;
    uint8_t* rgb;
} led_data_t;

static const uint8_t g_duty_x[DUTY_CYCLE_MAX] = {0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe};
static led_param_t gt_led_params;
static led_data_t gt_color_dst;
static led_data_t gt_color_now;
static led_data_t gt_color_base;
static uint8_t* gp_gradual_step = NULL;
static uint8_t g_data_new = 0;
static uint8_t g_led_state = LED_FREE;
static uint16_t g_send_index = 0;
static uint8_t* g_iic_data = NULL;


static int led_data_format(uint8_t* data, int len, uint8_t* buffer, uint8_t duty_0, uint8_t duty_1);

__attribute__((section("ram_code"))) void ssp_isr_ram(void)
{
    uint8_t spi_data[64] = {0};
    uint32_t total = gt_led_params.ic_count * 3;
    int len = 0;
    uint8_t duty_0 = gt_led_params.duty_cycle[0];
    uint8_t duty_1 = gt_led_params.duty_cycle[1];
    uint32_t status = ssp_get_isr_status();

    ssp_clear_isr_status(status);
    if (status & SSP_INT_TX_FF)
    {
        if ((g_send_index + 8) >= total)
        {
            len = led_data_format(gt_color_now.rgb + g_send_index, total - g_send_index, spi_data, duty_0, duty_1);
            ssp_put_data_to_fifo(spi_data, len);
            NVIC_DisableIRQ(SSP_IRQn);
            g_led_state = LED_FREE;
        }
        else
        {
            led_data_format(gt_color_now.rgb + g_send_index, 8, spi_data, duty_0, duty_1);
            ssp_put_data_to_fifo(spi_data, 64);
            g_send_index += 8;
        }
    }
}

static void led_data_to_ws2812(uint8_t val, uint8_t* dst, uint8_t duty_0, uint8_t duty_1)
{
    int i = 7;

    for (i = 7; i >= 0; i--)
    {
        if ((val >> i) & 0x01)
        {
            *(dst++) = g_duty_x[duty_1];
        }
        else
        {
            *(dst++) = g_duty_x[duty_0];
        }
    }
}

static int led_data_format(uint8_t* data, int len, uint8_t* buffer, uint8_t duty_0, uint8_t duty_1)
{
    uint8_t* format = buffer;
    int i = len;

    for (i = 0; i < len; i++)
    {
        led_data_to_ws2812(data[i], format, duty_0, duty_1);
        format += 8;
    }

    return len * 8;
}

static void led_iic_write(uint8_t* data, uint16_t size)
{
    uint8_t check_sum = 0;
    int i = 0;

    g_iic_data[0] = FRAME_SOFT;
    g_iic_data[1] = FRAME_TYPE_COLOR;
    g_iic_data[2] = (uint8_t)(size >> 8);
    g_iic_data[3] = (uint8_t)(size & 0x00ff);

    for (i = 0; i < size / 3; i++)
    {
        g_iic_data[4 + i*3] = data[i*3];
        g_iic_data[4 + i*3 + 1] = data[i*3 + 1];
        g_iic_data[4 + i*3 + 2] = data[i*3 + 2];
    }

    for (i = 0; i < size + 4; i++)
    {
        if (gt_led_params.iic_crc == CRC_XOR)
        {
            check_sum ^= g_iic_data[i];
        }
        else
        {
            check_sum += g_iic_data[i];
        }
    }
    g_iic_data[size + 4] = check_sum;

    if (gt_led_params.iic_write)
    {
        (*gt_led_params.iic_write)(I2C_ADDR, I2C_REG, g_iic_data, size + 5);
    }
}

static void led_data_write(led_data_t* pt_data)
{
    uint32_t duty = 0;

    if (gt_led_params.with_ww)
    {
        if (gt_led_params.pwm_cw.enable)
        {
            duty = pt_data->cw * gt_led_params.pwm_cw.duty / 255;
            pwm_update(gt_led_params.pwm_cw.channel, gt_led_params.pwm_cw.duty, duty);
        }

        if (gt_led_params.pwm_ww.enable)
        {
            duty = pt_data->ww * gt_led_params.pwm_ww.duty / 255;
            pwm_update(gt_led_params.pwm_ww.channel, gt_led_params.pwm_ww.duty, duty);
        }
    }

    g_led_state = LED_BUSY;

    if (gt_led_params.led_rgb_type == LED_RGB_SPI)
    {
        g_send_index = 0;

        ssp_enable_interrupt(SSP_INT_TX_FF);
        NVIC_EnableIRQ(SSP_IRQn);
        NVIC_SetPriority(SSP_IRQn, 1);
    }
    else
    {
        led_iic_write(pt_data->rgb, 3*gt_led_params.ic_count);
    }

}

static void led_data_sort(led_data_t* pt_dst, uint8_t* data, uint32_t size, uint8_t bright)
{
    uint32_t single_sum = 0;
    uint32_t color_sum = 0;
    int i = 0;

    for (i = 0; i < size / 3; i++)
    {
        if (gt_led_params.ic_type == LED_GRB)
        {
            pt_dst->rgb[i*3] = (uint8_t)(data[i*3+1] * bright / LED_MAX_BRIGHTNESS);
            pt_dst->rgb[i*3+1] = (uint8_t)(data[i*3] * bright / LED_MAX_BRIGHTNESS);
            pt_dst->rgb[i*3+2] = (uint8_t)(data[i*3+2] * bright / LED_MAX_BRIGHTNESS);
        }
        else if(gt_led_params.ic_type == LED_RGB)
        {
            pt_dst->rgb[i*3] = (uint8_t)(data[i*3] * bright / LED_MAX_BRIGHTNESS);
            pt_dst->rgb[i*3+1] = (uint8_t)(data[i*3+1] * bright / LED_MAX_BRIGHTNESS);
            pt_dst->rgb[i*3+2] = (uint8_t)(data[i*3+2] * bright / LED_MAX_BRIGHTNESS);
        }
        else if(gt_led_params.ic_type == LED_RBG)
        {
            pt_dst->rgb[i*3] = (uint8_t)(data[i*3] * bright / LED_MAX_BRIGHTNESS);
            pt_dst->rgb[i*3+1] = (uint8_t)(data[i*3+2] * bright / LED_MAX_BRIGHTNESS);
            pt_dst->rgb[i*3+2] = (uint8_t)(data[i*3+1] * bright / LED_MAX_BRIGHTNESS);
        }
        else if(gt_led_params.ic_type == LED_GBR)
        {
            pt_dst->rgb[i*3] = (uint8_t)(data[i*3+1] * bright / LED_MAX_BRIGHTNESS);
            pt_dst->rgb[i*3+1] = (uint8_t)(data[i*3+2] * bright / LED_MAX_BRIGHTNESS);
            pt_dst->rgb[i*3+2] = (uint8_t)(data[i*3] * bright / LED_MAX_BRIGHTNESS);
        }
        else if(gt_led_params.ic_type == LED_BRG)
        {
            pt_dst->rgb[i*3] = (uint8_t)(data[i*3+2] * bright / LED_MAX_BRIGHTNESS);
            pt_dst->rgb[i*3+1] = (uint8_t)(data[i*3] * bright / LED_MAX_BRIGHTNESS);
            pt_dst->rgb[i*3+2] = (uint8_t)(data[i*3+1] * bright / LED_MAX_BRIGHTNESS);
        }
        else if(gt_led_params.ic_type == LED_BGR)
        {
            pt_dst->rgb[i*3] = (uint8_t)(data[i*3+2] * bright / LED_MAX_BRIGHTNESS);
            pt_dst->rgb[i*3+1] = (uint8_t)(data[i*3+1] * bright / LED_MAX_BRIGHTNESS);
            pt_dst->rgb[i*3+2] = (uint8_t)(data[i*3] * bright / LED_MAX_BRIGHTNESS);
        }

        single_sum = pt_dst->rgb[3 * i] + pt_dst->rgb[3 * i + 1] + pt_dst->rgb[3 * i + 2];
        if (gt_led_params.limit_type == CURRENT_LIMIT_SINGLE)
        {
            if (single_sum > gt_led_params.current_limit)
            {
                pt_dst->rgb[3 * i] = pt_dst->rgb[3 * i] * gt_led_params.current_limit / single_sum;
                pt_dst->rgb[3 * i + 1] = pt_dst->rgb[3 * i + 1] * gt_led_params.current_limit / single_sum;
                pt_dst->rgb[3 * i + 2] = pt_dst->rgb[3 * i + 2] * gt_led_params.current_limit / single_sum;
            }
        }
        else if (gt_led_params.limit_type == CURRENT_LIMIT_WHOLE)
        {
            color_sum += single_sum;
        }
    }

    if (gt_led_params.limit_type == CURRENT_LIMIT_WHOLE)
    {
        if (color_sum > gt_led_params.current_limit)
        {
            for (i = 0; i < size / 3; i++)
            {
                pt_dst->rgb[3 * i] = pt_dst->rgb[3 * i] *  gt_led_params.current_limit / color_sum;
                pt_dst->rgb[3 * i + 1] = pt_dst->rgb[3 * i + 1] *  gt_led_params.current_limit / color_sum;
                pt_dst->rgb[3 * i + 2] = pt_dst->rgb[3 * i + 2] *  gt_led_params.current_limit / color_sum;
            }
        }
    }
}

static int led_gradual_update(led_data_t* dst, led_data_t* now, led_data_t* base, uint8_t* gradual)
{
    uint32_t interval[3] = {0, 0, 0};
    int i = 0;
    int j = 0;
    uint8_t max = 0;

    now->mode = dst->mode;
    memcpy(base, now, sizeof(led_data_t) - sizeof(uint8_t*));
    memcpy(base->rgb, now->rgb, gt_led_params.ic_count*3);

    interval[0] = GV_ABS(dst->cw, now->cw);
    interval[1] = GV_ABS(dst->ww, now->ww);
    max = MAX(interval[0], interval[1]);
    gradual[0] = (uint8_t)(interval[0] * 255 / max);
    gradual[1] = (uint8_t)(interval[1] * 255 / max);

    for (i = 0; i < gt_led_params.ic_count; i++)
    {
        max = 0;
        memset(interval, 0, 3);
        for (j = 0; j < 3; j++)
        {
            interval[j] = GV_ABS(dst->rgb[3*i+j], now->rgb[3*i+j]);
            if (interval[j] > max)
            {
                max = interval[j];
            }
        }

        for (j = 0; j < 3; j++)
        {
            gradual[i*3 + j + 2] = (uint8_t)(interval[j] * 255 / max);
        }
    }

    return 0;
}

static int led_color_update(led_data_t* dst, led_data_t* now, led_data_t* base, uint8_t* gradual, int times)
{
    int i = 0;
    int j = 0;
    int base_step = base->mode.step;
    int step = 0;
    int count = 0;
    uint8_t lumi_gradual = 0;
    led_data_t* pt_base = now;

    if (now->mode.b_proportion)
    {
        lumi_gradual = 1;
        pt_base = base;
        now->bright = LED_LUMI_IN_GRADUAL;
    }

    step = lumi_gradual ? times*base_step*gradual[0] / 255 : base_step;
    if (dst->cw < now->cw)
    {
        now->cw = (pt_base->cw - step) < dst->cw ? dst->cw : pt_base->cw - step;
        count++;
    }
    else if (dst->cw > now->cw)
    {
        now->cw = (pt_base->cw + step) > dst->cw ? dst->cw : pt_base->cw + step;
        count++;
    }

    step = lumi_gradual ? times*base_step*gradual[1] / 255 : base_step;
    if (dst->ww < now->ww)
    {
        now->ww = (pt_base->ww - step) < dst->ww ? dst->ww : pt_base->ww - step;
        count++;
    }
    else if (dst->ww > now->ww)
    {
        now->ww = (pt_base->ww + step) > dst->ww ? dst->ww : pt_base->ww + step;
        count++;
    }

    for (i = 0; i < gt_led_params.ic_count; i++)
    {
        for (j = 0; j < 3; j++)
        {
            step = lumi_gradual ? times*base_step*gradual[3*i + j + 2] / 255 : base_step;
            if (dst->rgb[3*i + j] < now->rgb[3*i + j])
            {
                now->rgb[3*i + j] = (pt_base->rgb[3*i + j] - step) < dst->rgb[3*i + j] ? dst->rgb[3*i + j] : pt_base->rgb[3*i + j] - step;
                count++;
            }
            else if (dst->rgb[3*i + j] > now->rgb[3*i + j])
            {
                now->rgb[3*i + j] = (pt_base->rgb[3*i + j] + step) > dst->rgb[3*i + j] ? dst->rgb[3*i + j] : pt_base->rgb[3*i + j] + step;
                count++;
            }
        }
    }

    if (count == 0)
    {
        now->bright = dst->bright;
    }

    return (count > 0) ? 0 : 1;
}

static void led_data_process(void* args)
{
    led_data_t* pt_dst = &gt_color_dst;
    led_data_t* pt_now = &gt_color_now;
    led_data_t* pt_base = &gt_color_base;
    static uint8_t count = 0;
    static uint32_t tick = 0;
    static uint8_t update = 0;
    uint8_t* gradual_param = gp_gradual_step;
    uint8_t delay = gt_led_params.refresh_base;
    uint8_t over = 0;
    uint8_t busy_count = 0;

    if (pt_dst->mode.type != LED_DIRECT)
    {
        delay = (100 - pt_dst->mode.speed) + gt_led_params.refresh_base;
    }

    if (g_data_new)
    {
        if (pt_dst->mode.type == LED_DIRECT)
        {
            memcpy(pt_now, pt_dst, sizeof(led_data_t) - sizeof(uint8_t*));
            memcpy(pt_now->rgb, pt_dst->rgb, gt_led_params.ic_count*3);
        }
        else if (pt_dst->mode.type == LED_GRADUAL)
        {
            count = 0;
            led_gradual_update(pt_dst, pt_now, pt_base, gradual_param);
        }

        update = 1;
        g_data_new = 0;
    }

    if (update && LightSdk_time_is_exceed(&tick, delay, 1))
    {
        if (pt_dst->mode.type == LED_GRADUAL)
        {
            count++;
            over = led_color_update(pt_dst, pt_now, pt_base, gradual_param, count);
        }

        if (gt_led_params.led_rgb_type == LED_RGB_SPI)
        {
            led_data_write(pt_now);
            while (g_led_state != LED_FREE)
            {
                co_delay_100us(1);
            }
        }
        else if (gt_led_params.led_rgb_type == LED_RGB_IIC)
        {
            if (gt_led_params.iic_idle)
            {
                while ((*gt_led_params.iic_idle)() != 0)
                {
                    co_delay_100us(1);
                    if (busy_count++ >= 20)
                    {
                        SDK_PRINT(LOG_WARNING, "mcu busy or crashed.\r\n");
                        return;
                    }
                }

                led_data_write(pt_now);
                g_led_state = LED_FREE;
            }
        }

        if (pt_now->mode.type == LED_DIRECT || over)
        {
            update = 0;
            if (pt_now->mode.callback)
            {
                (*pt_now->mode.callback)(pt_now->mode.args);
            }
        }
    }
}

int LightSdk_led_data_set(uint8_t* data, uint32_t size, uint8_t cw, uint8_t ww, uint8_t bright, led_mode_t mode)
{
    led_data_t* pt_data = &gt_color_dst;
    uint32_t brightness = LED_MAX_BRIGHTNESS;

    if (bright > 100)
    {
        return -1;
    }

    brightness = bright ? bright * (LED_MAX_BRIGHTNESS - gt_led_params.lumi_min) / LED_MAX_BRIGHTNESS + gt_led_params.lumi_min : 0;
    led_data_sort(pt_data, data, size, brightness);

    if (mode.speed > 100)
    {
        mode.speed = 100;
    }

    pt_data->mode = mode;
    pt_data->bright = bright;
    pt_data->cw = cw * brightness / LED_MAX_BRIGHTNESS;
    pt_data->ww = ww * brightness / LED_MAX_BRIGHTNESS;

    if (mode.type == LED_WITHOUT_QUEUE)
    {
        memcpy(&gt_color_now, pt_data, sizeof(led_data_t) - sizeof(uint8_t*));
        memcpy(gt_color_now.rgb, pt_data->rgb, gt_led_params.ic_count*3);
        led_data_write(&gt_color_now);
    }
    else
    {
        g_data_new = 1;
    }

    return 0;
}

int LightSdk_led_color_set(led_color_t color, uint8_t bright, led_mode_t mode)
{
    uint8_t buffer[LED_IC_MAX*3] = {0};
    int i = 0;

    for (i = 0; i < gt_led_params.ic_count; i++)
    {
        buffer[3*i] = color.r;
        buffer[3*i + 1] = color.g;
        buffer[3*i + 2] = color.b;
    }

    LightSdk_led_data_set(buffer, gt_led_params.ic_count*3, color.cw, color.ww, bright, mode);

    return 0;
}

int LightSdk_led_init(led_param_t* param)
{
    memset(&gt_color_dst, 0, sizeof(led_data_t));
    memset(&gt_color_now, 0, sizeof(led_data_t));
    memset(&gt_color_base, 0, sizeof(led_data_t));
    gt_led_params = *param;

    if (gt_led_params.led_rgb_type == LED_RGB_SPI)
    {
        system_set_port_mux(gt_led_params.spi_port, gt_led_params.spi_bit, LED_PIN_FUNC_SSP_OUT);
        ssp_init_(8, SSP_FRAME_MOTO, SSP_MASTER_MODE, gt_led_params.spi_speed, 2, NULL);
    }
    else
    {
        g_iic_data = (uint8_t*)HAL_malloc(3*gt_led_params.ic_count + 5);
        if (g_iic_data == NULL)
        {
            SDK_PRINT(LOG_ERROR, "malloc iic send buffer failed.\r\n");
            return -1;
        }
    }

    if (gt_led_params.with_ww)
    {
        if (gt_led_params.pwm_cw.enable)
        {
            system_set_port_mux(gt_led_params.pwm_cw.port, gt_led_params.pwm_cw.bit, LED_PIN_FUNC_PWM);
            pwm_init(gt_led_params.pwm_cw.channel, gt_led_params.pwm_cw.duty, 0);
            pwm_start(gt_led_params.pwm_cw.channel);
        }

        if (gt_led_params.pwm_ww.enable)
        {
            system_set_port_mux(gt_led_params.pwm_ww.port, gt_led_params.pwm_ww.bit, LED_PIN_FUNC_PWM);
            pwm_init(gt_led_params.pwm_ww.channel, gt_led_params.pwm_ww.duty, 0);
            pwm_start(gt_led_params.pwm_ww.channel);
        }
    }

    gp_gradual_step = (uint8_t*)HAL_malloc(3*gt_led_params.ic_count + 2);
    memset(gp_gradual_step, 0, 3*gt_led_params.ic_count + 2);
    gt_color_dst.rgb = (uint8_t*)HAL_malloc(3*gt_led_params.ic_count);
    memset(gt_color_dst.rgb, 0, 3*gt_led_params.ic_count);
    gt_color_now.rgb = (uint8_t*)HAL_malloc(3*gt_led_params.ic_count);
    memset(gt_color_now.rgb, 0, 3*gt_led_params.ic_count);
    gt_color_base.rgb = (uint8_t*)HAL_malloc(3*gt_led_params.ic_count);
    memset(gt_color_base.rgb, 0, 3*gt_led_params.ic_count);
    if (gp_gradual_step == NULL || gt_color_dst.rgb == NULL || gt_color_now.rgb == NULL || gt_color_base.rgb == NULL)
    {
        SDK_PRINT(LOG_ERROR, "malloc led channle buffer failed.\r\n");
        return -1;
    }

    LightRuntime_loop_task_register(led_data_process, 0, NULL);

    return 0;
}

void LightSdk_led_deinit(void)
{
    if (g_iic_data)
    {
        HAL_free(g_iic_data);
        g_iic_data = NULL;
    }

    if (gp_gradual_step)
    {
        HAL_free(gp_gradual_step);
        gp_gradual_step = NULL;
    }

    if (gt_color_dst.rgb)
    {
        HAL_free(gt_color_dst.rgb);
        gt_color_dst.rgb = NULL;
    }

    if (gt_color_now.rgb)
    {
        HAL_free(gt_color_now.rgb);
        gt_color_now.rgb = NULL;
    }

    if (gt_color_base.rgb)
    {
        HAL_free(gt_color_base.rgb);
        gt_color_base.rgb = NULL;
    }
}

