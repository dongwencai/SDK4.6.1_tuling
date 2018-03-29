#ifndef __LIGHTDUER_APP_IDLE__
#define __LIGHTDUER_APP_IDLE__

#include "lightduer_app_key.h"
#include "wifi_api.h"
#include "baidu_json.h"


#ifdef __cplusplus
extern "C" {

#endif


typedef enum {

	VOICE_RESULT_NOTIFY_IND				= 0x01,
	LIGHTDUER_REC_START 				= 0x02,
	LIGHTDUER_REC_DATA					= 0x03,
	LIGHTDUER_REC_STOP					= 0x04,
	LIGHTDUER_START_RECORD_THREAD		= 0x05,
	LIGHTDUER_START_LIGHTDUER_ENGINE 	= 0x06,
	LIGHTDUER_AUDIO_STOP				= 0x07,
	LIGHTDUER_SPEAK_HANDLER_IND			= 0x08,
	LIGHTDUER_PLAY_HANDLER_IND			= 0x09,
	LIGHTDUER_AUDIO_STOP_IND			= 0x0a,	
	LIGHTDUER_AUDIO_RESUME_IND			= 0x0b, 
	LIGHTDUER_AUDIO_PAUSE_IND			= 0x0c,
	LIGHTDUER_AUDIO_STOP_LISTEN_IND		= 0x0d,
	LIGHTDUER_AUDIO_START_LISTEN_IND	= 0x0e,

} idle_msg_type_t;


typedef struct {

	int offset;
    idle_msg_type_t msg_id;

}lightduer_app_idle_msg_t;


typedef enum {
	LIGHTDUER_IDLE_ACTION_NONE				= 0x00,
	LIGHTDUER_IDLE_ACTION_START_RECORD		= 0x01,
	LIGHTDUER_IDLE_ACTION_STOP_RECORD		= 0x02,	
	LIGHTDUER_IDLE_ACTION_START_PLAY		= 0x03,	
	LIGHTDUER_IDLE_ACTION_PREV				= 0x04,		
	LIGHTDUER_IDLE_ACTION_NEXT				= 0x05,			
	LIGHTDUER_IDLE_ACTION_SEND_DATA			= 0x06,		
	LIGHTDUER_IDLE_ACTION_SMART_LINK		= 0x07,	
} idle_action_type_t;


typedef struct {
    lightduer_app_key_value_t  key_value;    /**<  Key value, which key is pressed. */
    lightduer_app_key_action_t key_action;   /**<  Key action, the state of the key. */
    idle_action_type_t       idle_action;  /**<  Play action, which action of sink service will be executed. */
} idle_action_srv_table_t;


/**  This enum define the queue events. */
typedef enum {
	LIGHTDUER_APP_NONE_STATE = 0,
	LIGHTDUER_APP_IDLE_STATE = 1,		
	LIGHTDUER_APP_RECORD_STATE = 2,	
	LIGHTDUER_APP_PLAY_STATE = 3,
	LIGHTDUER_APP_OPERATE_SERVER_STATE = 4,
	LIGHTDUER_APP_SMART_LINK_STATE = 5,
    LIGHTDUER_APP_TOTAL_STATE = 6,
} lightduer_app_system_state_t;

typedef enum {
	LIGHTDUER_SERVER_DISCONNECT = 0,	
	LIGHTDUER_SERVER_CONNECT = 1,
} lightduer_app_connect_server_state_t;
typedef enum {
	DUER_RECORD_STOP = 0,	
	DUER_RECORD_RECORDING = 1,
	DUER_ON_LINE_SPEECH = 2,
	DUER_ON_LINE_PLAY = 3,
	DUER_RETRY_RECORD = 4,
	DUER_ONLINE_PLAY_PAUSE = 5,

} duer_operate_state_t;
typedef enum {
	DUER_WIFI_DISCONNECT = 0,	
	DUER_WIFI_CONNECT = 1,

} duer_wifi_state_t;

typedef struct {

	lightduer_app_system_state_t system_state;
	lightduer_app_connect_server_state_t server_state;
	duer_operate_state_t duer_operate_state;
	uint32_t retry_connect_server_timer;
	duer_wifi_state_t wifi_state;
	bool manual_stop_play;
	bool bduerEngineerInit;
} lightduer_app_idle_context_t;


void lightduer_app_idle_task(void *arg);
int32_t lightduer_app_wifi_connected(wifi_event_t event,uint8_t *payload,uint32_t length);
int32_t lightduer_app_wifi_disconnected(wifi_event_t event,uint8_t *payload,uint32_t length);
void lightduer_app_common_key_action(lightduer_app_key_value_t key_value,lightduer_app_key_action_t key_action);
void lightduer_app_set_system_state(lightduer_app_system_state_t state);
void lightduer_app_idle_isr_send_message(idle_msg_type_t msg);
void lightduer_app_idle_send_queue(idle_msg_type_t msg,const char *url,int offset);
duer_operate_state_t lightduer_app_idle_get_duer_record_state();
extern lightduer_app_idle_context_t g_duer_idle_cntx;

#ifdef __cplusplus
}
#endif

#endif

