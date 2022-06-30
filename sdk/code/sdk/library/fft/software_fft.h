#ifndef _SOFTWARE_FFT_H_
#define _SOFTWARE_FFT_H_

typedef enum
{
    WINDOW_NONE = 0,
    WINDOW_HANNING
} window_type_e;

int LightSdk_fft_init(void);
void LightSdk_fft_deinit(void);
int LightSdk_fft_do(int16_t* src, int* dst, window_type_e window);


#endif

