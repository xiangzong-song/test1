#ifndef _AUDIO_MANAGER_H_
#define _AUDIO_MANAGER_H_

#include "driver_codec.h"
#include "software_fft.h"

typedef struct
{
    uint8_t         beat_en:1;  //beat enable     
    uint8_t         fft_en:1;   //fft enable 
    uint8_t         style_en:1; //music style recognition enable 
    window_type_e   window;     //fft window
    uint8_t         beat_sens;  //beat sensitivity
} audio_para_t;
   
typedef enum
{
    FFT_BIT     = (1<<0),
    BEAT_BIT    = (1<<1),
    STYLE_BIT   = (1<<2)
} update_bits_e;

typedef enum
{
    BLUES = 1,
    CLASSIC,
    COUNTYR,
    ELECTRONIC,
    HIPHOP,
    JAZZ,
    METAL,
    POP,
    PUNK,
    REGGAE,
    RNB,
    ROCK
} music_style_e;

typedef struct
{
    update_bits_e   update_bits;
    int             *fft_data_address;  
    uint32_t        fft_data_int_len;      
    uint16_t        beat_value;     
    music_style_e   music_style;    
} audio_info_t;

typedef void (*audio_info_proc)(audio_info_t info, void* args);

int  LightService_audio_manager_init(void);
void LightService_audio_manager_deinit(void);
int  LightService_audio_manager_register(audio_info_proc callback, void* args);
void LightService_audio_manager_unregister(audio_info_proc callback);
int  LightService_audio_manager_start(audio_para_t para);
int  LightService_audio_manager_stop(void);

#endif
