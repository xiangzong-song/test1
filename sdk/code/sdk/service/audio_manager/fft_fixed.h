#ifndef _FFT_FIXED_H
#define _FFT_FIXED_H

#include <stdint.h>

typedef void* fft_handle;

fft_handle fft_fixed_init(void);
void fft_fixed_uninit(fft_handle handle);
void fft_fixed(fft_handle handle, int *data);

#endif

