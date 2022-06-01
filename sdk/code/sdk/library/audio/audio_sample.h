#ifndef _AUDIO_SAMPLE_H_
#define _AUDIO_SAMPLE_H_

#include <stdint.h>

int LightSdk_audio_sample_read(int16_t* buffer, uint32_t sample_size);
int LightSdk_audio_sample_size(void);
int LightSdk_audio_sample_init(uint8_t rate, int gain_level);
void LightSdk_audio_sample_deinit(void);
int LightSdk_audio_sample_start(void);
int LightSdk_audio_sample_stop(void);


#endif
