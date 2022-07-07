#include <stdint.h>
#include <string.h>
#include "driver_plf.h"
#include "driver_system.h"
#include "driver_i2s.h"
#include "driver_pmu.h"
#include "driver_codec.h"
#include "driver_frspim.h"
#include "driver_codec.h"
#include "log.h"
#include "audio_sample.h"
#include "ringbuffer.h"
#include "platform.h"


#define AUDIO_SAMPLE_LEN                2
#define I2S_IRQ_PRIO                    4
#define codec_write(addr, data)         frspim_wr(FR_SPI_CODEC_CHAN, addr, 1, (uint32_t)data)


static uint8_t g_adc_flag = 0;
static LR_handler gt_sample_lr = NULL;


extern void codec_adc_init(uint8_t sample_rate);

__attribute__((section("ram_code"))) void i2s_isr_ram(void)
{
    int16_t audio_data[I2S_FIFO_DEPTH/2];

    if (i2s_get_int_status() & I2S_INT_STATUS_RX_HFULL)
    {
        i2s_get_data(audio_data, I2S_FIFO_DEPTH/2, I2S_DATA_MONO);
        Lite_ring_buffer_write_data(gt_sample_lr, (uint8_t*)audio_data, I2S_FIFO_DEPTH);
    }
}

int LightSdk_audio_sample_read(int16_t* buffer, uint32_t sample_size)
{
    int data_len = 0;

    if (buffer == NULL)
    {
        SDK_PRINT(LOG_DEBUG, "buffer is null.\r\n");
        return -1;
    }

    if (gt_sample_lr == NULL)
    {
        SDK_PRINT(LOG_DEBUG, "gt_sample_lr is null.\r\n");
        return -1;
    }

    data_len = Lite_ring_buffer_size_get(gt_sample_lr);
    if (data_len < sample_size*AUDIO_SAMPLE_LEN)
    {
        SDK_PRINT(LOG_DEBUG, "audio data is not enough.\r\n");
        return -1;
    }
    Lite_ring_buffer_read_data(gt_sample_lr, (uint8_t*)buffer, sample_size*AUDIO_SAMPLE_LEN);

    return 0;
}

int LightSdk_audio_sample_size(void)
{
    if (gt_sample_lr == NULL)
    {
        SDK_PRINT(LOG_DEBUG, "gt_sample_lr is null.\r\n");
        return -1;
    }

    int data_size = Lite_ring_buffer_size_get(gt_sample_lr);

    return data_size / AUDIO_SAMPLE_LEN;
}


int LightSdk_audio_sample_init(uint8_t rate, int gain_level)
{
    int level = gain_level > 100 ? 100 : gain_level;
    uint32_t gain = 0;

    pmu_codec_power_enable();
    codec_adc_init(rate);
    codec_write(0x18, 0x1C); //PGA  P/N exchange, P enable  N enable
    codec_enable_adc();

    i2s_init(I2S_DIR_RX, 8000, 1);
    i2s_start();

    gain = level*0x3F/100;
    codec_write(0x19, gain);

    return 0;
}

void LightSdk_audio_sample_deinit(void)
{
    i2s_stop();
}

int LightSdk_audio_sample_ring_buffer_init(int buffer_size)
{
    int size = (buffer_size < 512) ? 512 : buffer_size;
    gt_sample_lr = Lite_ring_buffer_init(size);
    if (gt_sample_lr == NULL)
    {
        SDK_PRINT(LOG_ERROR, "audio ring buffer init fail\r\n");
        return -1;
    }
    return 0;
}

int LightSdk_audio_sample_ring_buffer_deinit(void)
{
    if (gt_sample_lr)
    {
        Lite_ring_buffer_deinit(gt_sample_lr);
        gt_sample_lr = NULL;
    }
    return 0;
}

int LightSdk_audio_sample_start(void)
{
    if (!g_adc_flag)
    {
        NVIC_SetPriority(I2S_IRQn, I2S_IRQ_PRIO);
        NVIC_EnableIRQ(I2S_IRQn);
        g_adc_flag = 1;
    }

    return 0;
}

int LightSdk_audio_sample_stop(void)
{
    if (g_adc_flag)
    {
        NVIC_DisableIRQ(I2S_IRQn);
        g_adc_flag = 0;
    }

    return 0;
}


