#include <string.h>
#include "hal_os.h"
#include "fft_fixed.h"
#include "software_fft.h"

#define SHIFT_BITS      7   //(GOVEE_NFFT / 2 = 256 / 2 = 128 = 2^7)
#define NFFT_POINT      256
#define WINDOW_TIMES    5000
#define ABS(x)          ((x)>0?(x):-(x))
#define eps             1


static const int16_t g_hanning_window[NFFT_POINT] =
{
       0,    1,    4,    7,   13,   19,   28,   38,   49,   62,   76,   92,  109,  128,  148,  169,
     192,  217,  242,  269,  298,  328,  359,  391,  425,  460,  496,  534,  572,  612,  653,  695,
     738,  782,  828,  874,  921,  969, 1019, 1069, 1120, 1171, 1224, 1277, 1331, 1386, 1442, 1498,
    1554, 1612, 1670, 1728, 1787, 1846, 1906, 1966, 2026, 2087, 2147, 2209, 2270, 2331, 2393, 2454,
    2516, 2577, 2639, 2700, 2762, 2823, 2884, 2945, 3005, 3065, 3125, 3185, 3244, 3302, 3360, 3418,
    3475, 3531, 3587, 3642, 3697, 3750, 3803, 3856, 3907, 3957, 4007, 4056, 4104, 4150, 4196, 4241,
    4285, 4327, 4369, 4409, 4448, 4486, 4523, 4559, 4593, 4626, 4658, 4688, 4717, 4745, 4772, 4797,
    4820, 4843, 4863, 4883, 4901, 4917, 4932, 4946, 4958, 4969, 4978, 4985, 4991, 4996, 4999, 5000,
    5000, 4999, 4996, 4991, 4985, 4978, 4969, 4958, 4946, 4932, 4917, 4901, 4883, 4863, 4843, 4820,
    4797, 4772, 4745, 4717, 4688, 4658, 4626, 4593, 4559, 4523, 4486, 4448, 4409, 4369, 4327, 4285,
    4241, 4196, 4150, 4104, 4056, 4007, 3957, 3907, 3856, 3803, 3751, 3697, 3642, 3587, 3531, 3475,
    3418, 3360, 3302, 3244, 3185, 3125, 3065, 3005, 2945, 2884, 2823, 2762, 2700, 2639, 2577, 2516,
    2454, 2393, 2331, 2270, 2209, 2147, 2087, 2026, 1966, 1906, 1846, 1787, 1728, 1670, 1612, 1554,
    1498, 1442, 1386, 1331, 1277, 1224, 1171, 1120, 1069, 1019,  969,  921,  874,  828,  782,  738,
     695,  653,  612,  572,  534,  496,  460,  425,  391,  359,  328,  298,  269,  242,  217,  192,
     169,  148,  128,  109,   92,   76,   62,   49,   38,   28,   19,   13,    7,    4,    1,    0
};

static fft_handle gt_fft_handle = NULL;

static int fft_sqrt(int x)
{
    int low, up, mid;

    low = 0, up = (x >= 1 ? x : 1);
    mid = (low + up) / 2;

    do
    {
        if (mid*mid>x)
        {
            up = mid;
        }
        else
        {
            low = mid;
        }
        mid = (up + low) / 2;
    } while (ABS(mid - low) > eps);

    return mid;
}

int LightSdk_fft_init(void)
{
    if (NULL == gt_fft_handle)
    {
        gt_fft_handle = fft_fixed_init();
    }

    return gt_fft_handle ? 0 : -1;
}

void LightSdk_fft_deinit(void)
{
    if (gt_fft_handle != NULL)
    {
        fft_fixed_uninit(gt_fft_handle);
        gt_fft_handle = NULL;
    }
}

int LightSdk_fft_do(int16_t* src, int* dst, window_type_e window)
{
    int i = 0;
    int16_t* p_window = NULL;

    if (src == NULL || dst==NULL)
    {
        return -1;
    }

    if (gt_fft_handle == NULL)
    {
        LightSdk_fft_deinit();
        if (LightSdk_fft_init() != 0)
        {
            return -1;
        }
    }

    memset(dst, 0, 2*NFFT_POINT*sizeof(int));
    for (i = 0; i < NFFT_POINT; i++)
    {
        if (window == WINDOW_NONE)
        {
            dst[2*i] = src[i];
        }
        else
        {
            p_window = (int16_t*)g_hanning_window;
            dst[2*i] = (int)src[i]*p_window[i] / WINDOW_TIMES;
        }
    }

    fft_fixed(gt_fft_handle, dst);

    for (i = 0; i < NFFT_POINT / 2; i++)
    {
        dst[i] = fft_sqrt((dst[2*i]>>SHIFT_BITS)*(dst[2*i]>>SHIFT_BITS) + (dst[2*i + 1]>>SHIFT_BITS)*(dst[2*i+1]>>SHIFT_BITS));
    }

    return 0;
}

