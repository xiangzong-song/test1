#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "hal_os.h"
#include "fft_fixed.h"


#define FFT_POINT                       256
#define FFT_BASE                        8
#define FIXMUL_32X15(a,b)               ((int)(((int64_t)(a) *(int64_t)(b))>>(15)))


static const short g_cos_ang[FFT_POINT] =
{
     32767,  32758,  32729,  32679,  32610,  32522,  32413,  32286,  32138,  31972,  31786,  31581,  31357,  31114,  30853,  30572,
     30274,  29957,  29622,  29269,  28899,  28511,  28106,  27684,  27246,  26791,  26320,  25833,  25330,  24812,  24279,  23732,
     23170,  22595,  22006,  21403,  20788,  20160,  19520,  18868,  18205,  17531,  16846,  16151,  15447,  14733,  14010,  13279,
     12540,  11793,  11039,  10279,   9512,   8740,   7962,   7180,   6393,   5602,   4808,   4011,   3212,   2411,   1608,    804,
         0,   -804,  -1608,  -2411,  -3212,  -4011,  -4808,  -5602,  -6393,  -7180,  -7962,  -8740,  -9512, -10279, -11039, -11793,
    -12540, -13279, -14010, -14733, -15447, -16151, -16846, -17531, -18205, -18868, -19520, -20160, -20788, -21403, -22006, -22595,
    -23170, -23732, -24279, -24812, -25330, -25833, -26320, -26791, -27246, -27684, -28106, -28511, -28899, -29269, -29622, -29957,
    -30274, -30572, -30853, -31114, -31357, -31581, -31786, -31972, -32138, -32286, -32413, -32522, -32610, -32679, -32729, -32758,
    -32767, -32758, -32729, -32679, -32610, -32522, -32413, -32286, -32138, -31972, -31786, -31581, -31357, -31114, -30853, -30572,
    -30274, -29957, -29622, -29269, -28899, -28511, -28106, -27684, -27246, -26791, -26320, -25833, -25330, -24812, -24279, -23732,
    -23170, -22595, -22006, -21403, -20788, -20160, -19520, -18868, -18205, -17531, -16846, -16151, -15447, -14733, -14010, -13279,
    -12540, -11793, -11039, -10279,  -9512,  -8740,  -7962,  -7180,  -6393,  -5602,  -4808,  -4011,  -3212,  -2411,  -1608,   -804,
         0,    804,   1608,   2411,   3212,   4011,   4808,   5602,   6393,   7180,   7962,   8740,   9512,  10279,  11039,  11793,
     12540,  13279,  14010,  14733,  15447,  16151,  16846,  17531,  18205,  18868,  19520,  20160,  20788,  21403,  22006,  22595,
     23170,  23732,  24279,  24812,  25330,  25833,  26320,  26791,  27246,  27684,  28106,  28511,  28899,  29269,  29622,  29957,
     30274,  30572,  30853,  31114,  31357,  31581,  31786,  31972,  32138,  32286,  32413,  32522,  32610,  32679,  32729,  32758
};

static const short g_sin_ang[FFT_POINT] =
{
         0,    804,   1608,   2411,   3212,   4011,   4808,   5602,   6393,   7180,   7962,   8740,   9512,  10279,  11039,  11793,
     12540,  13279,  14010,  14733,  15447,  16151,  16846,  17531,  18205,  18868,  19520,  20160,  20788,  21403,  22006,  22595,
     23170,  23732,  24279,  24812,  25330,  25833,  26320,  26791,  27246,  27684,  28106,  28511,  28899,  29269,  29622,  29957,
     30274,  30572,  30853,  31114,  31357,  31581,  31786,  31972,  32138,  32286,  32413,  32522,  32610,  32679,  32729,  32758,
     32767,  32758,  32729,  32679,  32610,  32522,  32413,  32286,  32138,  31972,  31786,  31581,  31357,  31114,  30853,  30572,
     30274,  29957,  29622,  29269,  28899,  28511,  28106,  27684,  27246,  26791,  26320,  25833,  25330,  24812,  24279,  23732,
     23170,  22595,  22006,  21403,  20788,  20160,  19520,  18868,  18205,  17531,  16846,  16151,  15447,  14733,  14010,  13279,
     12540,  11793,  11039,  10279,   9512,   8740,   7962,   7180,   6393,   5602,   4808,   4011,   3212,   2411,   1608,    804,
         0,   -804,  -1608,  -2411,  -3212,  -4011,  -4808,  -5602,  -6393,  -7180,  -7962,  -8740,  -9512, -10279, -11039, -11793,
    -12540, -13279, -14010, -14733, -15447, -16151, -16846, -17531, -18205, -18868, -19520, -20160, -20788, -21403, -22006, -22595,
    -23170, -23732, -24279, -24812, -25330, -25833, -26320, -26791, -27246, -27684, -28106, -28511, -28899, -29269, -29622, -29957,
    -30274, -30572, -30853, -31114, -31357, -31581, -31786, -31972, -32138, -32286, -32413, -32522, -32610, -32679, -32729, -32758,
    -32767, -32758, -32729, -32679, -32610, -32522, -32413, -32286, -32138, -31972, -31786, -31581, -31357, -31114, -30853, -30572,
    -30274, -29957, -29622, -29269, -28899, -28511, -28106, -27684, -27246, -26791, -26320, -25833, -25330, -24812, -24279, -23732,
    -23170, -22595, -22006, -21403, -20788, -20160, -19520, -18868, -18205, -17531, -16846, -16151, -15447, -14733, -14010, -13279,
    -12540, -11793, -11039, -10279,  -9512,  -8740,  -7962,  -7180,  -6393,  -5602,  -4808,  -4011,  -3212,  -2411,  -1608,   -804
};

static const short g_bit_rvs[FFT_POINT] =
{
     0, 128,  64, 192,  32, 160,  96, 224,  16, 144,  80, 208,  48, 176, 112, 240,
     8, 136,  72, 200,  40, 168, 104, 232,  24, 152,  88, 216,  56, 184, 120, 248,
     4, 132,  68, 196,  36, 164, 100, 228,  20, 148,  84, 212,  52, 180, 116, 244,
    12, 140,  76, 204,  44, 172, 108, 236,  28, 156,  92, 220,  60, 188, 124, 252,
     2, 130,  66, 194,  34, 162,  98, 226,  18, 146,  82, 210,  50, 178, 114, 242,
    10, 138,  74, 202,  42, 170, 106, 234,  26, 154,  90, 218,  58, 186, 122, 250,
     6, 134,  70, 198,  38, 166, 102, 230,  22, 150,  86, 214,  54, 182, 118, 246,
    14, 142,  78, 206,  46, 174, 110, 238,  30, 158,  94, 222,  62, 190, 126, 254,
     1, 129,  65, 193,  33, 161,  97, 225,  17, 145,  81, 209,  49, 177, 113, 241,
     9, 137,  73, 201,  41, 169, 105, 233,  25, 153,  89, 217,  57, 185, 121, 249,
     5, 133,  69, 197,  37, 165, 101, 229,  21, 149,  85, 213,  53, 181, 117, 245,
    13, 141,  77, 205,  45, 173, 109, 237,  29, 157,  93, 221,  61, 189, 125, 253,
     3, 131,  67, 195,  35, 163,  99, 227,  19, 147,  83, 211,  51, 179, 115, 243,
    11, 139,  75, 203,  43, 171, 107, 235,  27, 155,  91, 219,  59, 187, 123, 251,
     7, 135,  71, 199,  39, 167, 103, 231,  23, 151,  87, 215,  55, 183, 119, 247,
    15, 143,  79, 207,  47, 175, 111, 239,  31, 159,  95, 223,  63, 191, 127, 255
};


typedef struct  _fft_fixed_ctx_t {
    int base;
    int length;
    int *fft_work;
    short *bit_rvs;
    short *cos_ang;
    short *sin_ang;
} fft_fixed_ctx_t;


/*
    decimation-in-freq radix-2 in-place butterfly
    data:   (array of int , order below)
    re(0),im(0),re(1),im(1),...,re(size-1),im(size-1)
    means size=array_length/2 !!

    useage:
    intput in normal order
    output in bit-reversed order
*/
static void dif_butterfly_fixed(int *data, long size, short *cos_ang, short *sin_ang)
{

    long  angle, astep, dl;
    int   xr, yr, xi, yi, dr, di;
    short wr, wi;
    int   *l1, *l2, *end, *ol2;


    astep = 1;
    end   = data + size + size;
    for (dl = size; dl > 1; dl >>= 1, astep += astep) {
        l1 = data;
        l2 = data + dl;
        for (; l2 < end; l1 = l2, l2 = l2 + dl) {
            ol2 = l2;
            for (angle = 0; l1 < ol2; l1 += 2, l2 += 2) {
                wr = cos_ang[angle];
                wi = -sin_ang[angle];

                xr = *l1     + *l2;
                xi = *(l1+1) + *(l2+1);
                dr = *l1     - *l2;
                di = *(l1+1) - *(l2+1);

                yr = FIXMUL_32X15(dr, wr) - FIXMUL_32X15(di, wi);
                yi = FIXMUL_32X15(dr, wi) + FIXMUL_32X15(di, wr);

                *(l1) = xr; *(l1 + 1) = xi;
                *(l2) = yr; *(l2 + 1) = yi;
                angle += astep;
            }
        }
    }
}


/*
    in-place Radix-2 FFT for complex values
    data:   (array of int, below order)
    re(0),im(0),re(1),im(1),...,re(size-1),im(size-1)
    means size=array_length/2 !!

    output is in similar order, normalized by array length
*/
void fft_fixed(fft_handle handle, int *data)
{
    int i,bit;
    fft_fixed_ctx_t *f = (fft_fixed_ctx_t *)handle;
    int *temp      = f->fft_work;
    int size       = f->length;
    short *cos_ang = f->cos_ang;
    short *sin_ang = f->sin_ang;

    dif_butterfly_fixed(data,size,cos_ang,sin_ang);

    for (i = 0 ; i < size ; i++) {
        bit = f->bit_rvs[i];
        temp[i+i] = data[bit+bit]; temp[i+i+1] = data[bit+bit+1];
    }

    for (i = 0; i < size ; i++) {
        data[i+i] = temp[i+i];
        data[i+i+1] = temp[i+i+1];
    }
}

fft_handle fft_fixed_init(void)
{
    fft_fixed_ctx_t *f = NULL;

    f = (fft_fixed_ctx_t *)HAL_malloc(sizeof(fft_fixed_ctx_t));
    memset(f, 0, sizeof(fft_fixed_ctx_t));

    f->length  = FFT_POINT;
    f->base    = FFT_BASE;

    if ((1<<f->base) < FFT_POINT)
        f->base += 1;

    f->fft_work = (int *)HAL_malloc(sizeof(int)*FFT_POINT*2);
    f->cos_ang  = (short*)g_cos_ang;
    f->sin_ang  = (short*)g_sin_ang;
    f->bit_rvs = (short*)g_bit_rvs;

    return (fft_handle)f;
}

void fft_fixed_uninit(fft_handle handle)
{
    fft_fixed_ctx_t *f = (fft_fixed_ctx_t *)handle;

    if (f->fft_work)
    {
        HAL_free(f->fft_work);
        f->fft_work = NULL;
    }

    if (f)
    {
        HAL_free(f);
        f = NULL;
    }
}

