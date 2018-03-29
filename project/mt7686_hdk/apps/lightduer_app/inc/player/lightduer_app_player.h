#ifndef __LIGHTDUER_APP_PLAYER__
#define __LIGHTDUER_APP_PLAYER__
#include "http_download.h"
#include "ff.h"
#include "lightduer_app_mp3_player.h"
#include "audio_file_manager.h"
#include "lightduer_app_key.h"

#ifdef __cplusplus
extern "C" {

#endif

#define AUDIO_PLAYER_ERR_SUCCESS_7TH            (7)
#define AUDIO_PLAYER_ERR_SUCCESS_6TH            (6)
#define AUDIO_PLAYER_ERR_SUCCESS_5TH            (5)
#define AUDIO_PLAYER_ERR_SUCCESS_4TH            (4)
#define AUDIO_PLAYER_ERR_SUCCESS_3RD            (3)
#define AUDIO_PLAYER_ERR_SUCCESS_2ND            (2)
#define AUDIO_PLAYER_ERR_SUCCESS_1ST            (1)
#define AUDIO_PLAYER_ERR_SUCCESS_OK             (0)
#define AUDIO_PLAYER_ERR_FAIL_1ST               (-1)
#define AUDIO_PLAYER_ERR_FAIL_2ND               (-2)
#define AUDIO_PLAYER_ERR_FAIL_3RD               (-3)
#define AUDIO_PLAYER_ERR_FAIL_4TH               (-4)
#define AUDIO_PLAYER_ERR_FAIL_5TH               (-5)
#define AUDIO_PLAYER_ERR_FAIL_6TH               (-6)
#define AUDIO_PLAYER_ERR_FAIL_7TH               (-7)
#define AUD_FOLDER_BREAK_POINT_LEN   8
#define AUDIO_PLAYER_FILE_PATH      ("0:/music")
#define AUDIO_PLAYER_DEFAULT_FOLDER_ID 0x01
#define AUDIO_PLAYER_DEFAULT_FILE_ID 0x01

typedef enum {
    FILE_TYPE_NONE   = 0,
    MP3_FILE_TYPE    = 1,
    WAV_FILE_TYPE    = 2,
	AAC_FILE_TYPE 	 = 3,
    FILE_TYPE_TOAL
} play_file_type_t;



typedef enum {
	AUDIO_PLAYER_ACTION_NONE		= 0x00,
	AUDIO_PLAYER_ACTION_PLAY		= 0x01,
	AUDIO_PLAYER_ACTION_PAUSE		= 0x02,	
	AUDIO_PLAYER_ACTION_NEXT_TRACK	= 0x03,
	AUDIO_PLAYER_ACTION_PRE_TRACK	= 0x04,	
	AUDIO_PLAYER_ACTION_NEXT_ALBUM 	= 0x05, 
	AUDIO_PLAYER_ACTION_PRE_ALBUM	= 0x06,	
	AUDIO_PLAYER_ACTION_RECORD		= 0x07, 

} player_action_type_t;

typedef struct {
    lightduer_app_key_value_t  key_value;    /**<  Key value, which key is pressed. */
    lightduer_app_key_action_t key_action;   /**<  Key action, the state of the key. */
    player_action_type_t       player_action;  /**<  Play action, which action of sink service will be executed. */
} player_action_srv_table_t;

typedef enum {
    PLAYER_ACTION_SRV_STATUS_SUCCESS        =     0,    /**< The sink service status: success. */
    PLAYER_ACTION_SRV_STATUS_FAIL           =    -1,    /**< The sink service status: fail. */
} player_action_status_t;


typedef enum {
	ONLINE_PLAYER_MSG_START_DOWNLOAD	= 0x01,
	ONLINE_PLAYER_MSG_WAIT_DATA			= 0x02,	
	ONLINE_PLAYER_MSG_START_PLAYING		= 0x03,
	ONLINE_PLAYER_MSG_STOP				= 0x04,	
	ONLINE_PLAYER_MSG_PAUSE 			= 0x05, 
	ONLINE_PLAYER_MSG_RESUME			= 0x06,	
	ONLINE_PLAYER_MSG_CLOSE				= 0x07,	
} player_msg_type_t;

typedef enum {
	PLAYER_IDLE					= 0x00,	
	ONLINE_PLAYER_DOWNLOAD		= 0x01,
	PLAYER_START_PLAY			= 0x02,
	PLAYER_PLAYING				= 0x03,
	PLAYER_PAUSE				= 0x04,
	PLAYER_RESUME				= 0x05,
	PLAYER_STOP					= 0x06,	
	PLAYER_STOPPING				= 0x07,		
	PLAYER_CLOSE				= 0x08,	
	PLAYER_ERROR				= 0x09,		
} player_state_t;

typedef struct {
    uint16_t file_id;
    uint16_t pad;
    uint32_t offset;
}audio_player_breakpoint_data_t;


typedef struct {
    char *src_mod;
    player_msg_type_t msg_id;

}player_msg_t;

typedef void (*online_play_finish_callback_t)(player_state_t state);

typedef enum {
    NEXT_TRACK = 0,
    PREVIOUS_TRACK,
    NEXT_ALBUM,
    PREVIOUS_ALBUM,
    NORMAL_TRACK,
    TRACK_TOTAL
} track_change_type_t;


#define AUDIO_PLAYER_FLAG_START_PLAY        (1 << 0) /* Indicate player start a sinlge song, only stop the playing behavior shuold reset the flag */
#define AUDIO_PLAYER_FLAG_NEED_RESUME       (1 << 1) /* Indicate player under pause status */
#define AUDIO_PLAYER_FLAG_FILE_OPEN         (1 << 2) /* Indicate player already open a file */
#define AUDIO_PLAYER_FLAG_PLAYING           (1 << 4) /* Indicate player under plyaing status, only change when pause/stop/close/interrupt */
#define AUDIO_PLAYER_FLAG_LOCAL_PLAYING     (1 << 5) /* Indicate player whether is playing when mode switch. */

typedef enum {
    LOCAL_PLAY = 0,
	NET_PLAY,
} play_type_t;


typedef struct {

	play_type_t play_mode; // 0: local play, 1: netplay
	play_file_type_t file_type;
	uint8_t first_time_f_mount;
	FATFS fatfs;
	FIL fdst;
	player_state_t player_state;
	char *online_play_url;
	int file_size;
    TCHAR curr_file_path[MAX_FILE_PATH_LEN];
    uint16_t file_id;
    uint16_t file_total;
    uint16_t folder_id;
	uint32_t offset;
	uint32_t playing_pos;
	uint32_t lseek_position;	
	bool isSeeking;
} lightduer_app_player_context_t;


void lightduer_app_play_task(void *arg);
player_action_status_t lightduer_app_play_key_action(lightduer_app_key_value_t key_value,
        lightduer_app_key_action_t key_action);
int32_t lightduer_app_play_local_play(void);
FRESULT lightduer_app_file_lseek(uint32_t pos);
bool lightduer_app_file_over();
int32_t lightduer_app_player_play(char *url,play_type_t mode, uint32_t offset);


#ifdef __cplusplus
}
#endif

#endif

