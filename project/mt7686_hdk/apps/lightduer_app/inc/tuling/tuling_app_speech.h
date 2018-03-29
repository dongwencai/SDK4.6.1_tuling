#ifndef _TULING_APP_SPEECH_H_
#define _TULING_APP_SPEECH_H_

#define TULING_EVENT_QUEUE_LENGTH 20

extern QueueHandle_t g_tuling_speech_queue_handle;

typedef enum{
	TULING_MASSAGE_NONE = 0,
    TULING_MASSAGE_RECORD_START,
	TULING_MASSAGE_REQUEST_AND_RESPONSE,
}EN_TULING_MASSAGE_t;

typedef struct
{
	char token[64];
	char user_id[17];
	char aes_key[17];
	char api_key[33];
} TulingUser_t;

extern void tuling_app_speech_send_queue(EN_TULING_MASSAGE_t message);


#endif
