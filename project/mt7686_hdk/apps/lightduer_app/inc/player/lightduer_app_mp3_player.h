#ifndef __LIGHTDUER_MP3_PLAYER__
#define __LIGHTDUER_MP3_PLAYER__
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "task.h"
#include "mp3_codec.h"
#include "hal_audio.h"
#include "hal_log.h"
#include "ff.h"



#ifdef __cplusplus
extern "C" {

#endif

#define MP3_PLAY_PLAYING	  	1
#define MP3_PLAY_FINISH 		2
#define MP3_PLAY_STOP 			3
#define MP3_PLAY_PAUSE 			4
#define MP3_PLAY_RESUME 		5
#define MP3_PLAY_NEXT_TRACK 	6





typedef void (*mp3_player_getdata_callback_t)(void* buff,int32_t btr,int32_t *br);

typedef void (*mp3_player_state_callback_t)(uint8_t player_state);


typedef struct {
	mp3_codec_media_handle_t *mp3_player_hdl;
	mp3_player_getdata_callback_t callbackfordata;
	mp3_player_state_callback_t callbackstate;
	uint32_t retmain_file_size;
} lightduer_mp3_player_context_t;


void lightduer_mp3_player_callback(mp3_player_getdata_callback_t callback,uint32_t file_size,mp3_player_state_callback_t player_state);

bool lightduer_mp3_player_play();

void lightduer_mp3_player_pause();

void lightduer_mp3_player_resume();

void lightduer_mp3_player_stop();
void lightduer_mp3_player_init();
void lightduer_mp3_player_deinit();


#ifdef __cplusplus
}
#endif

#endif









