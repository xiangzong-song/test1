#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "audio_manager.h"
#include "hal_os.h"
#include "log.h"
#include "sys_queue.h"
#include "ringbuffer.h"
#include "runtime.h"
#include "platform.h"

#if (PLATFORM_TYPE_ID == PLATFORM_FR8016HA)
#include "audio_sample.h"
#include "fft_fixed.h"
#include "software_fft.h"

#define AVG_NUM 100
#define RM_DC_FILTER_TIMES 63
#define RM_DC_SELECT_TIMES 126
#define BEAT_KEEP_TIMES 15
#endif


typedef struct
{
    uint8_t sample_rate;  // audio sampling rate
    uint32_t sample_size; // audio sampling size
    int gain_level;       // mic gain level
} audio_init_t;

struct audio_entry
{
    audio_info_proc callback;
    void *args;
    TAILQ_ENTRY(audio_entry)
    entry;
};

TAILQ_HEAD(audio_queue, audio_entry);

static audio_init_t audio_init =
    {
        .sample_rate = CODEC_SAMPLE_RATE_8000,
        .sample_size = 256,
        .gain_level = 100};

static audio_para_t audio_para =
    {
        .audio_src = MIC,
        .info_en_bits = BEAT_BIT,
        .window = WINDOW_HANNING,
        .beat_sens = 100,
        .noise_value = 10};

static uint8_t g_audio_init_flag = 0;
static uint8_t audio_start_flag = 0;
static struct audio_queue g_audio_queue;
static audio_info_t audio_info = {0};

#if (PLATFORM_TYPE_ID == PLATFORM_FR8016HA)
static int16_t audio_data_idx = 0;
static int16_t audio_data_cnt = 0;
static uint16_t *audio_data_tab = NULL;
static int16_t audio_time_count = 0;
static int16_t audio_silence_count = 0;
static int *audio_fft_data = NULL;
#endif

#define AUDIO_TASK_INIT_CHECK                                        \
    do                                                               \
    {                                                                \
        if (!g_audio_init_flag)                                      \
        {                                                            \
            TAILQ_INIT(&g_audio_queue);                              \
            LightRuntime_loop_task_register(audio_process, 0, NULL); \
            g_audio_init_flag = 1;                                   \
        }                                                            \
    } while (0)

/****************************************************************
 *  8016 static function
 *****************************************************************/
#if (PLATFORM_TYPE_ID == PLATFORM_FR8016HA)
static int audio_hw_init_8016(audio_init_t init)
{
    if (init.sample_rate != CODEC_SAMPLE_RATE_48000 &&
        init.sample_rate != CODEC_SAMPLE_RATE_44100 &&
        init.sample_rate != CODEC_SAMPLE_RATE_24000 &&
        init.sample_rate != CODEC_SAMPLE_RATE_16000 &&
        init.sample_rate != CODEC_SAMPLE_RATE_8000)
    {
        SDK_PRINT(LOG_ERROR, "Invalid sample_rate.\r\n");
        return -1;
    }

    if (init.sample_size < 256)
    {
        SDK_PRINT(LOG_WARNING, "sample_size should be over 256.\r\n");
    }

    LightSdk_audio_sample_init(init.sample_rate, init.gain_level);
    return 0;
}

static void audio_hw_deinit_8016(void)
{
    LightSdk_audio_sample_deinit();
}

static int audio_manager_start_8016(audio_para_t para)
{
    uint32_t sample_size;

    if (!audio_start_flag)
    {
        audio_start_flag = 1;

        sample_size = audio_init.sample_size;

        if (para.beat_sens > 100)
        {
            para.beat_sens = 100;
            SDK_PRINT(LOG_WARNING, "beat_sens should be [0~100].\r\n");
        }

        memcpy(&audio_para, &para, sizeof(audio_para_t));

        //initialization of variable
        audio_data_idx = 0;
        audio_data_cnt = 0;
        audio_time_count = 0;
        audio_silence_count = 0;

        // buffer malloc
        if (audio_data_tab == NULL)
        {
            audio_data_tab = (int16_t *)HAL_malloc(AVG_NUM * sizeof(int16_t));
            memset(audio_data_tab, 0, sizeof(audio_data_tab));
        }

        if (para.info_en_bits & FFT_BIT)
        {
            LightSdk_fft_init();
            if (audio_fft_data == NULL)
            {
                audio_fft_data = (int *)HAL_malloc(2 * sample_size * sizeof(int));
            }
        }
        LightSdk_audio_sample_ring_buffer_init(1024);

        // audio source hardware start
        LightSdk_audio_sample_start();
    }
}

static int audio_manager_stop_8016(void)
{
    if (audio_start_flag)
    {
        audio_start_flag = 0;

        // audio source hardware stop
        LightSdk_audio_sample_stop();

        // buffer free
        LightSdk_audio_sample_ring_buffer_deinit();
        if (audio_data_tab)
        {
            HAL_free(audio_data_tab);
            audio_data_tab = NULL;
        }

        LightSdk_fft_deinit();
        if (audio_fft_data)
        {
            HAL_free(audio_fft_data);
            audio_fft_data = NULL;
        }
    }
}

static int audio_remove_data_dc_offset(int16_t *sample_data, uint32_t sample_size)
{
    static uint8_t count = 0;
    static int16_t *offset_array = NULL;
    static int offset = 0;

    if (sample_data == NULL)
    {
        SDK_PRINT(LOG_ERROR, "sample_data is NULL!\r\n");
        return offset;
    }

    if (sample_size == 0)
    {
        SDK_PRINT(LOG_ERROR, "sample_size is 0!\r\n");
        return offset;
    }

    if (count < RM_DC_FILTER_TIMES) // filter data
    {
        if (offset_array == NULL)
        {
            uint32_t len = RM_DC_FILTER_TIMES * sizeof(int16_t);
            offset_array = (int16_t *)HAL_malloc(len);
            memset(offset_array, 0, len);
        }
        count++;
    }
    else if (count < RM_DC_SELECT_TIMES) // selection data
    {
        for (uint32_t i = 0; i < sample_size; i++)
        {
            offset += sample_data[i];
        }
        offset = offset / sample_size;
        offset_array[count - RM_DC_FILTER_TIMES] = (int16_t)offset;
        count++;
    }
    else if (count == RM_DC_SELECT_TIMES)
    {
        for (int i = 0; i < RM_DC_FILTER_TIMES; i++)
        {
            offset += offset_array[i];
        }
        offset = offset / RM_DC_FILTER_TIMES;
        count = RM_DC_SELECT_TIMES + 1;

        if (offset_array)
        {
            HAL_free(offset_array);
            offset_array = NULL;
        }
    }
    if (count > RM_DC_SELECT_TIMES)
    {
        for (uint32_t i = 0; i < sample_size; i++)
        {
            sample_data[i] -= offset;
        }
    }

    return offset;
}

static uint16_t audio_data_zero_crossing_get(int16_t *sample_data, uint32_t sample_size)
{
    uint16_t zero = 0;

    if (sample_data == NULL)
    {
        SDK_PRINT(LOG_ERROR, "sample_data is NULL!\r\n");
        return zero;
    }

    if (sample_size == 0)
    {
        SDK_PRINT(LOG_ERROR, "sample_size is 0!\r\n");
        return zero;
    }

    for (uint32_t i = 1; i < sample_size; i++)
    {
        if ((sample_data[i] >= 0 && sample_data[i - 1] <= 0) || (sample_data[i] <= 0 && sample_data[i - 1] >= 0))
            zero++;
    }

    return zero;
}

static uint16_t audio_get_volume_sensitivity(uint8_t sensitivity)
{
    uint16_t volume_sensitivity = 0;

    if (sensitivity > 100)
    {
        sensitivity = 100;
    }

    if (audio_para.noise_value < 10)
    {
        audio_para.noise_value = 10;
    }

    volume_sensitivity = audio_para.noise_value + (100 - sensitivity);

    return volume_sensitivity;
}

static int audio_get_volume_th(int avg_array_sum, int avg_array_max, int time_count)
{
    int th = 0;

    if (avg_array_max > avg_array_sum * 2)
    {
        th = avg_array_sum / 5;
        th = th * 10 / time_count;
    }
    else
    {
        th = (avg_array_max - avg_array_sum) / 6;
        th = th * 15 / time_count;
    }

    if (th < avg_array_sum / 20)
        th = avg_array_sum / 20;

    if (th < 10)
        th = 10;

    return th;
}

static int audio_beat_value_get(int16_t *sample_data, audio_para_t para)
{
    uint32_t sample_size = audio_init.sample_size;
    uint8_t sensitivity = para.beat_sens;
    uint16_t beat_value = 0;
    uint16_t avg = 0;
    uint16_t data_tab_avg = 0;
    uint16_t data_tab_max = 0;
    uint16_t data_recent[4] = {0};
    uint16_t idx = 0;
    int th = 0;

    if (sample_data == NULL)
    {
        SDK_PRINT(LOG_ERROR, "sample_data is null.\r\n");
        return -1;
    }

    if (audio_data_tab == NULL)
    {
        uint32_t len = AVG_NUM * sizeof(int16_t);
        audio_data_tab = (int16_t *)HAL_malloc(len);
        memset(audio_data_tab, 0, len);
    }

    // get avg.
    uint32_t sum = 0;
    for (uint32_t i = 0; i < sample_size; i++)
    {
        sum += sample_data[i] > 0 ? sample_data[i] : -sample_data[i];
    }
    avg = (uint16_t)(sum / sample_size);

    // fill audio_data_tab.
    audio_data_tab[audio_data_idx++] = avg;
    audio_data_idx %= AVG_NUM;
    audio_data_cnt += (audio_data_cnt < AVG_NUM) ? 1 : 0;

    // get data_tab_avg and data_tab_max.
    sum = 0;
    for (int16_t i = 0; i < audio_data_cnt; i++)
    {
        sum += audio_data_tab[i];
        if (audio_data_tab[i] > data_tab_max)
        {
            data_tab_max = audio_data_tab[i];
        }
    }
    data_tab_avg = (uint16_t)(sum / audio_data_cnt);

    // calculate beat para.
    audio_time_count++;
    th = audio_get_volume_th(data_tab_avg, data_tab_max, audio_time_count);
    uint16_t volume_sensitivity = audio_get_volume_sensitivity(sensitivity);
    uint16_t zero = audio_data_zero_crossing_get(sample_data, sample_size);

    // get 4 data_recent.
    for (uint32_t i = 0; i < 4; i++)
    {
        idx = ((int)(audio_data_idx - 1) < 0) ? (AVG_NUM - 1) : (audio_data_idx - 1);
        data_recent[i] = audio_data_tab[idx];
    }

    // check beat
    if (avg > volume_sensitivity)
    {
        if ((data_recent[0] - data_tab_avg > th) ||
            (data_recent[0] - data_recent[1] > th && data_recent[0] - data_recent[2] > th && data_recent[0] - data_recent[3] > th))
        {
            audio_time_count = 0;
            if (avg > 200 || zero > 8) // 200 8
            {
                audio_silence_count = 0;
                beat_value = avg;
            }
            else
            {
                audio_silence_count++;
                if (audio_silence_count > 1 * BEAT_KEEP_TIMES && audio_silence_count < 7 * BEAT_KEEP_TIMES)
                {
                    beat_value = avg;
                }
            }
        }
        else
        {
            audio_silence_count++;
        }
    }
    else
    {
        audio_silence_count++;
    }

    // HAL_printf("y%dy",avg);
    // HAL_printf("r%dr",beat_value);
    // HAL_printf("o%do",volume_sensitivity);
    // HAL_printf("u%du",th);
    // HAL_printf("p%dp",data_tab_avg);

    return beat_value;
}

static int audio_fft_value_get(int16_t *sample_data, int *data, audio_para_t para)
{
    int ret = 0;
    ret = LightSdk_fft_do(sample_data, audio_fft_data, para.window);
    return ret;
}

static uint8_t audio_music_style_get(int16_t *sample_data, audio_para_t para)
{
    uint8_t music_style = 0;

    /*
    ADDING.
    */

    return music_style;
}

static void audio_info_update_8016(void)
{
    uint32_t sample_size = audio_init.sample_size;
    int16_t sample_data[sample_size];
    uint8_t percent = 0;

    if (LightSdk_audio_sample_size() >= sample_size)
    {
        memset(sample_data, 0, sizeof(sample_data));

        LightSdk_audio_sample_read(sample_data, sample_size);
        audio_remove_data_dc_offset(sample_data, sample_size);

        audio_info.update_bits = 0;
        if (audio_para.info_en_bits & FFT_BIT)
        {
            if (audio_fft_value_get(sample_data, audio_fft_data, audio_para) != 0)
            {
                SDK_PRINT(LOG_ERROR, "LightSdk_fft_do fail.\r\n");
                audio_info.fft_data_address = NULL;
                audio_info.fft_data_int_len = 0;
            }
            else
            {
                percent = audio_para.beat_sens * (100 - 90) / 100 + 90;
                for (int i = 0; i < sample_size / 2; i++)
                {
                    if (audio_fft_data[i] > 1000)
                    {
                        audio_fft_data[i] = 0;
                    }
                    audio_fft_data[i] = percent * audio_fft_data[i] / 100;
                }

                audio_info.fft_data_address = audio_fft_data;
                audio_info.fft_data_int_len = sample_size / 2;
                audio_info.update_bits |= FFT_BIT;
            }
        }

        if (audio_para.info_en_bits & BEAT_BIT)
        {
            int beat_value = audio_beat_value_get(sample_data, audio_para);
            if (beat_value > 0)
            {
                audio_info.beat_value = (uint16_t)beat_value;
                audio_info.update_bits |= BEAT_BIT;
            }
        }

        if (audio_para.info_en_bits & STYLE_BIT)
        {
            uint8_t music_style = audio_music_style_get(sample_data, audio_para);
            if (music_style != audio_info.music_style)
            {
                audio_info.music_style = music_style;
                audio_info.update_bits |= STYLE_BIT;
            }
        }
    }
}

/****************************************************************
 *  5089 static function
 *****************************************************************/
#elif (PLATFORM_TYPE_ID == PLATFORM_FR5089D2)
static int audio_hw_init_5089(audio_init_t init)
{

    return 0;
}

static int audio_hw_deinit_5089(void)
{

    return 0;
}

static int audio_manager_start_5089(audio_para_t para)
{
    if (!audio_start_flag)
    {
        audio_start_flag = 1;

        if (para.info_en_bits & FFT_BIT)
        {
            if (audio_fft_data == NULL)
            {
                audio_fft_data = (int *)HAL_malloc(2 * sample_size * sizeof(int));
            }
        }
    }
}

static int audio_manager_stop_5089(void)
{
    if (audio_start_flag)
    {
        audio_start_flag = 0;

        if (audio_fft_data)
        {
            HAL_free(audio_fft_data);
            audio_fft_data = NULL;
        }
    }
}

static void audio_info_update_5089(void)
{

}
#endif

static void audio_process(void *args)
{
    struct audio_entry *var = NULL;

    if (audio_start_flag == 0)
    {
        return;
    }

    #if (PLATFORM_TYPE_ID == PLATFORM_FR8016HA)
    audio_info_update_8016();
    #elif (PLATFORM_TYPE_ID == PLATFORM_FR5089D2 )
    audio_info_update_5089();
    #endif

    if (audio_info.update_bits)
    {
        TAILQ_FOREACH(var, &g_audio_queue, entry)
        {
            if (var->callback)
            {
                (*var->callback)(audio_info, var->args);
            }
        }
    }
}


/****************************************************************
 *  public function
 *****************************************************************/
int LightService_audio_manager_init(void)
{
    #if (PLATFORM_TYPE_ID == PLATFORM_FR8016HA)
    audio_hw_init_8016(audio_init);
    #elif (PLATFORM_TYPE_ID == PLATFORM_FR5089D2)
    audio_hw_init_5089(audio_init);
    #endif

    AUDIO_TASK_INIT_CHECK;

    return 0;
}

void LightService_audio_manager_deinit(void)
{
    #if (PLATFORM_TYPE_ID == PLATFORM_FR8016HA)
    audio_hw_deinit_8016();
    #elif (PLATFORM_TYPE_ID == PLATFORM_FR5089D2)
    audio_hw_deinit_5089();
    #endif
}

int LightService_audio_manager_register(audio_info_proc callback, void *args)
{
    struct audio_entry *element = NULL;

    AUDIO_TASK_INIT_CHECK;

    if (NULL == (element = HAL_malloc(sizeof(struct audio_entry))))
    {
        SDK_PRINT(LOG_ERROR, "Malloc audio entry failed.\r\n");
        return -1;
    }

    element->callback = callback;
    element->args = args;
    TAILQ_INSERT_TAIL(&g_audio_queue, element, entry);

    return 0;
}

void LightService_audio_manager_unregister(audio_info_proc callback)
{
    struct audio_entry *var = NULL;

    TAILQ_FOREACH(var, &g_audio_queue, entry)
    {
        if (var->callback == callback)
        {
            TAILQ_REMOVE(&g_audio_queue, var, entry);
            HAL_free(var);
            break;
        }
    }
}

int LightService_audio_manager_start(audio_para_t para)
{
    #if (PLATFORM_TYPE_ID == PLATFORM_FR8016HA)
    audio_manager_start_8016(para);
    #elif (PLATFORM_TYPE_ID == PLATFORM_FR5089D2)
    audio_manager_start_5089(para);
    #endif

    return 0;
}

int LightService_audio_manager_stop(void)
{
    #if (PLATFORM_TYPE_ID == PLATFORM_FR8016HA)
    audio_manager_stop_8016();
    #elif (PLATFORM_TYPE_ID == PLATFORM_FR5089D2)
    audio_manager_stop_5089();
    #endif

    return 0;
}
