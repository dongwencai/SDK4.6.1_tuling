#include "stdint.h"
#include "stdio.h"
#include "FreeRTOS.h"
#include "task.h"
#include "wifi_api.h"
#include "lightduer_app_idle.h"
#include "lightduer_app_player.h"
#include "lightduer_connagent.h"
#include "lightduer_app_i2s_record.h"
#include "baidu_json.h"
#include "lightduer_dcs.h"
#include "timers.h"
#include "hal_gpt.h"
#include "tuling_app_speech.h"
#include "smart_link.h"

#define IDLE_EVENT_QUEUE_LENGTH 50 // 0 
//#define _ENABLE_IDLE_DEBUG_
lightduer_app_idle_context_t g_duer_idle_cntx;
//static QueueHandle_t g_duerapp_idle_queue_handle;
static TimerHandle_t g_TimerofConnectServer = NULL;

#ifdef _ENABLE_IDLE_DEBUG_
#include "ff.h"
FIL record_fp;
FRESULT res;
#endif
char g_url[1024] = "http://dlbcdn.ocm.ainirobot.com/manual/dba8cb2d2d9a3eebd43de1fbcd968e3a.mp3";

//#define PROFILE_PATH "profile"

const char* profile = "{\"configures\":\"{}\",\"bindToken\":\"837613b8dffbd556e4da6fd1b1ab1d34\",\"coapPort\":443,\"token\":\"FgY1K66S\",\"serverAddr\":\"device.iot.baidu.com\",\"lwm2mPort\":443,\"uuid\":\"0a170000000001\",\"rsaCaCrt\":\"-----BEGIN CERTIFICATE-----\nMIIDUDCCAjgCCQCmVPUErMYmCjANBgkqhkiG9w0BAQUFADBqMQswCQYDVQQGEwJD\nTjETMBEGA1UECAwKU29tZS1TdGF0ZTEOMAwGA1UECgwFYmFpZHUxGDAWBgNVBAMM\nDyouaW90LmJhaWR1LmNvbTEcMBoGCSqGSIb3DQEJARYNaW90QGJhaWR1LmNvbTAe\nFw0xNjAzMTEwMzMwNDlaFw0yNjAzMDkwMzMwNDlaMGoxCzAJBgNVBAYTAkNOMRMw\nEQYDVQQIDApTb21lLVN0YXRlMQ4wDAYDVQQKDAViYWlkdTEYMBYGA1UEAwwPKi5p\nb3QuYmFpZHUuY29tMRwwGgYJKoZIhvcNAQkBFg1pb3RAYmFpZHUuY29tMIIBIjAN\nBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAtbhIeiN7pznzuMwsLKQj2xB02+51\nOvCJ5d116ZFLjecp9qtllqOfN7bm+AJa5N2aAHJtsetcTHMitY4dtGmOpw4dlGqx\nluoz50kWJWQjVR+z6DLPnGE4uELOS8vbKHUoYPPQTT80eNVnl9S9h/l7DcjEAJYC\nIYJbf6+K9x+Ti9VRChvWcvgZQHMRym9j1g/7CKGMCIwkC+6ihkGD/XG40r7KRCyH\nbD53KnBjBO9FH4IL3rGlZWKWzMw3zC6RTS2ekfEsgAtYDvROKd4rNs+uDU9xaBLO\ndXTl5uxgudH2VnVzWtj09OUbBtXcQFD2IhmOl20BrckYul+HEIMR0oDibwIDAQAB\nMA0GCSqGSIb3DQEBBQUAA4IBAQCzTTH91jNh/uYBEFekSVNg1h1kPSujlwEDDf/W\npjqPJPqrZvW0w0cmYsYibNDy985JB87MJMfJVESG/v0Y/YbvcnRoi5gAenWXQNL4\nh2hf08A5wEQfLO/EaD1GTH3OIierKYZ6GItGrz4uFKHV5fTMiflABCdu37ALGjrA\nrIjwjxQG6WwLr9468hkKrWNG3dMBHKvmqO8x42sZOFRJMkqBbKzaBd1uW4xY5XwM\nS1QX56tVrgO0A3S+4dEg5uiLVN4YVP/Vqh4SMtYkL7ZZiZAxD9GtNnhRyFsWlC2r\nOVSdXs1ttZxEaEBGUl7tgsBte556BIvufZX+BXGyycVJdBu3\n-----END CERTIFICATE-----\n\",\"macId\":\"\",\"version\":3591}";

static const idle_action_srv_table_t g_idle_action_mapping_table[] =
{
	{
		LIGHTDUER_KEY_PLAY,
		LIGHTDUER_KEY_ACT_PRESS_UP,
		LIGHTDUER_IDLE_ACTION_START_PLAY
	},
	{
		LIGHTDUER_KEY_RECORD,
		LIGHTDUER_KEY_ACT_PRESS_UP,
		LIGHTDUER_IDLE_ACTION_START_RECORD
	},
	{
		LIGHTDUER_KEY_NEXT,
		LIGHTDUER_KEY_ACT_PRESS_UP,
		//LIGHTDUER_IDLE_ACTION_SEND_DATA
		LIGHTDUER_IDLE_ACTION_NEXT
	},
	{
		LIGHTDUER_KEY_PREV,
		LIGHTDUER_KEY_ACT_PRESS_UP,
		LIGHTDUER_IDLE_ACTION_PREV
	},
	{
        LIGHTDUER_KEY_PLAY,
        LIGHTDUER_KEY_ACT_LONG_PRESS_DOWN,
        LIGHTDUER_IDLE_ACTION_SMART_LINK
    },
};
#ifdef _ENABLE_IDLE_DEBUG_
void lightduer_app_open_file()
{
	FRESULT res;
	res = f_open(&record_fp, "SD:/1.pcm", FA_CREATE_ALWAYS | FA_WRITE | FA_READ);
	if(res == FR_OK)
	{
		printf("record file open ok\n");
	}
	else
	{
		printf("record file open fail\n");
	}
}
void lightduer_app_close_file()
{
	FRESULT res;
	res = f_close(&record_fp);
	if(res == FR_OK)
	{
		printf("record file close ok\n");
	}
	else
	{
		printf("record file close fail\n");
	}
}
void lightduer_app_write_file(const void* buff,UINT btw)
{
	FRESULT res;
	UINT write_lenght;
	res = f_write(&record_fp,buff,btw,&write_lenght);
	if(res != FR_OK)
	{
		printf("record file write fail\n");
	}
}

#endif

void duer_dcs_init(void)
{
	static bool is_first_time = true;
	if(is_first_time)
	{
		LOG_I(common,"duer_dcs_init\n\r");
		duer_dcs_framework_init();
		duer_dcs_voice_input_init();
		duer_dcs_voice_output_init();
		duer_dcs_speaker_control_init();
		duer_dcs_audio_player_init();
		duer_dcs_sync_state();
		is_first_time = false;
	}
}
void lightduer_app_idle_send_queue(idle_msg_type_t msg,const char *url,int offset)
{
	lightduer_app_idle_msg_t message;
	message.msg_id = msg;
	message.offset = offset;
	if (url != NULL)
	{
		memset(g_url,0,strlen(g_url));
		memcpy(g_url,url,strlen(url));
	}
	//xQueueSend(g_duerapp_idle_queue_handle, (void*)&message, 0);
}
void lightduer_app_idle_isr_send_message(idle_msg_type_t msg)
{
	lightduer_app_idle_msg_t msgs;
	BaseType_t xHigherPriorityTaskWoken;
	/* We have not woken a task at the start of the ISR*/
	xHigherPriorityTaskWoken = pdFALSE;
	msgs.msg_id    = msg;
	//while (xQueueSendFromISR(g_duerapp_idle_queue_handle, &msgs, &xHigherPriorityTaskWoken) != pdTRUE);

	/* Now the buffer is empty we can switch context if necessary.*/
	if (xHigherPriorityTaskWoken)
	{
		/*Actual macro used here is port specific.*/
		portYIELD_FROM_ISR(pdTRUE);
	}
}

int32_t lightduer_app_wifi_connected(wifi_event_t event,uint8_t *payload,uint32_t length)
{
	printf("lightduer_app_wifi_connected !!!\n\r");

	return 1;
}
int32_t lightduer_app_wifi_disconnected(wifi_event_t event,uint8_t *payload,uint32_t length)
{
	printf("lightduer_app_wifi_connected !!!\n\r");
	g_duer_idle_cntx.wifi_state = DUER_WIFI_DISCONNECT;
	return 1;
}

static void lightduer_app_reconnect_duerserver(TimerHandle_t pxTimer)
{
	if (g_duer_idle_cntx.wifi_state == DUER_WIFI_CONNECT)
	{
		lightduer_app_idle_send_queue(LIGHTDUER_START_LIGHTDUER_ENGINE,NULL,0);
	}
	if (g_TimerofConnectServer)
	{
		xTimerStop(g_TimerofConnectServer, 0);
		xTimerDelete(g_TimerofConnectServer, 0);
		g_TimerofConnectServer = NULL;
	}

}

void lightduer_app_event_hook(duer_event_t *event)
{
	if (!event)
	{
		LOG_I(common, "NULL event!!!!!\n\r");
	}
	LOG_I(common,"event: %d", event->_event);
	switch (event->_event)
	{
	case DUER_EVENT_STARTED:
		// Initialize the DCS API
		LOG_I(common,"Duer connect to server success\n\r");
		duer_dcs_init();
		g_duer_idle_cntx.server_state = LIGHTDUER_SERVER_CONNECT;

		break;
	case DUER_EVENT_STOPPED:
		LOG_I(common,"Duer connect to server failure\n\r");
		g_duer_idle_cntx.server_state = LIGHTDUER_SERVER_DISCONNECT;
		if (g_duer_idle_cntx.wifi_state == DUER_WIFI_CONNECT)
		{
			if (g_TimerofConnectServer == NULL)
			{
				g_TimerofConnectServer = xTimerCreate("xTimerofConnectServer",		/* Just a text name, not used by the kernel. */
                ((g_duer_idle_cntx.retry_connect_server_timer * 1000) / portTICK_PERIOD_MS),	   /* The timer period in ticks. */
                pdFALSE, 	   /* The timers will auto-reload themselves when they expire. */
                NULL,   /* Assign each timer a unique id equal to its array index. */
                lightduer_app_reconnect_duerserver /* Each timer calls the same callback when it expires. */
                );
			}
		}
		break;
	}
}

duer_operate_state_t lightduer_app_idle_get_duer_record_state()
{
	return g_duer_idle_cntx.duer_operate_state;
}

static void lightduer_app_idle_handle_common_event(void)
{

	lightduer_app_idle_msg_t msgs;

	while (1)
	{
		//if (xQueueReceive(g_duerapp_idle_queue_handle, &msgs, portMAX_DELAY))
		{
			switch (msgs.msg_id)
			{
			case LIGHTDUER_START_LIGHTDUER_ENGINE:
			{
				printf("Wifi Connected, start up duer engine !!!\n\r");
				if (g_duer_idle_cntx.bduerEngineerInit)
				{
					g_duer_idle_cntx.bduerEngineerInit = false;
				}
				duer_start(profile, strlen(profile));
				g_duer_idle_cntx.wifi_state = DUER_WIFI_CONNECT;
			}
			break;
			case LIGHTDUER_PLAY_HANDLER_IND:
			{

				LOG_I(common,"LIGHTDUER_PLAY_HANDLER_IND:%d",g_duer_idle_cntx.duer_operate_state);
				if (g_duer_idle_cntx.duer_operate_state == DUER_RECORD_STOP)
				{
					lightduer_app_player_play(g_url,NET_PLAY,0);
					//g_duer_idle_cntx.duer_operate_state = DUER_ON_LINE_PLAY;
					g_duer_idle_cntx.system_state = LIGHTDUER_APP_OPERATE_SERVER_STATE;
				}

			}
			break;
			case LIGHTDUER_SPEAK_HANDLER_IND:
			{
				LOG_I(common,"LIGHTDUER_SPEAK_HANDLER_IND:%d",g_duer_idle_cntx.duer_operate_state);
				if (g_duer_idle_cntx.duer_operate_state == DUER_RECORD_STOP)
				{
					g_duer_idle_cntx.system_state = LIGHTDUER_APP_IDLE_STATE;
					lightduer_app_player_play(g_url,NET_PLAY,0);
					g_duer_idle_cntx.system_state = LIGHTDUER_APP_OPERATE_SERVER_STATE;
					g_duer_idle_cntx.duer_operate_state = DUER_ON_LINE_SPEECH;
				}

			}
			break;
			case LIGHTDUER_REC_START:
			{
				if (g_duer_idle_cntx.server_state == LIGHTDUER_SERVER_CONNECT)
				{
					int ret = DUER_OK;
					ret = duer_dcs_on_listen_started();
					if (ret == DUER_OK)
					{
						duer_voice_start(16000);
						lightduer_app_idle_send_queue(LIGHTDUER_START_RECORD_THREAD,NULL,0);
					}
					else
					{
						LOG_E(common,"The server is disconnect!!!");
					}

				}
#ifdef _ENABLE_IDLE_DEBUG_
				lightduer_app_open_file();
#endif

			}
			break;
			case LIGHTDUER_REC_DATA:
			{
				int32_t length;
				int32_t req_length;
				uint8_t *ring_buf = NULL;
				if (g_duer_idle_cntx.duer_operate_state == DUER_RECORD_RECORDING)
				{
					length = http_download_ring_buf_data_count();
					http_download_ring_buf_get_read_pointer(&ring_buf,&req_length);
					duer_voice_send(ring_buf,req_length);
					//	LOG_I(common,"data[0x%x,0x%x,0x%x,0x%x,0x%x,0x%x]", *ring_buf,*(ring_buf + 1), *(ring_buf + 2),*(ring_buf + 3),*(ring_buf + 4),*(ring_buf + 5));
#ifdef _ENABLE_IDLE_DEBUG_
					lightduer_app_write_file(ring_buf,req_length);
#endif
					http_download_ring_buf_update_read_pointer(req_length);
					if (length < req_length)
					{
						http_download_ring_buf_get_read_pointer(&ring_buf,&req_length);
						duer_voice_send(ring_buf,req_length);
#ifdef _ENABLE_IDLE_DEBUG_
						lightduer_app_write_file(ring_buf,req_length);
#endif
						http_download_ring_buf_update_read_pointer(req_length);
					}
					//			LOG_I(common,"LIGHTDUER_REC_DATA,req_length = %d", req_length);
				}
			}
			break;
			case LIGHTDUER_REC_STOP:
			{
				if (g_duer_idle_cntx.duer_operate_state == DUER_RECORD_RECORDING)
				{
					LOG_I(common,"stop send voice");
					duer_voice_stop();
				}
#ifdef _ENABLE_IDLE_DEBUG_
				lightduer_app_close_file();
#endif

			}
			break;
			case LIGHTDUER_START_RECORD_THREAD:
			{
				lightduer_app_rec_send_queue(I2S_REC_QUEUE_EVENT_START_RECORD,NULL);
				g_duer_idle_cntx.duer_operate_state = DUER_RECORD_RECORDING;
				g_duer_idle_cntx.system_state = LIGHTDUER_APP_RECORD_STATE;
			}
			break;
			case LIGHTDUER_AUDIO_STOP:
			{
                printf("g_duer_idle_cntx.duer_operate_state = %d\r\n",g_duer_idle_cntx.duer_operate_state);
				if (g_duer_idle_cntx.duer_operate_state == DUER_ON_LINE_SPEECH)
				{
					LOG_I(common,"LIGHTDUER_AUDIO_STOP,enter duer_dcs_speech_on_finished");
					g_duer_idle_cntx.duer_operate_state = DUER_RECORD_STOP;
					duer_dcs_speech_on_finished();
				}
				else if (g_duer_idle_cntx.duer_operate_state == DUER_ON_LINE_PLAY)
				{
					if (!g_duer_idle_cntx.manual_stop_play)
					{
						LOG_I(common,"LIGHTDUER_AUDIO_STOP,enter duer_dcs_audio_on_finished");
						g_duer_idle_cntx.duer_operate_state = DUER_RECORD_STOP;
						duer_dcs_audio_on_finished();
					}
					else
					{
						g_duer_idle_cntx.manual_stop_play = false;

					}
				}
				else if (g_duer_idle_cntx.duer_operate_state == DUER_RETRY_RECORD)
				{
					LOG_I(common,"start enter LIGHTDUER_REC_START!!");
					lightduer_app_idle_send_queue(LIGHTDUER_REC_START,NULL,0);
				}
				else
				{
					LOG_I(common,"LIGHTDUER_AUDIO_STOP!!");
				}
				g_duer_idle_cntx.system_state = LIGHTDUER_APP_IDLE_STATE;
			}
			break;
			case LIGHTDUER_AUDIO_STOP_IND:
			{
				LOG_I(common,"LIGHTDUER_AUDIO_STOP_IND");
			}
			break;
			case LIGHTDUER_AUDIO_PAUSE_IND:
			{
				LOG_I(common,"LIGHTDUER_AUDIO_PAUSE_IND");
			}
			break;
			case LIGHTDUER_AUDIO_RESUME_IND:
			{
				LOG_I(common,"LIGHTDUER_AUDIO_RESUME_IND");

			}
			break;
			case LIGHTDUER_AUDIO_STOP_LISTEN_IND:
			{
				LOG_I(common,"LIGHTDUER_AUDIO_STOP_LISTEN_IND: %d",g_duer_idle_cntx.system_state);
				if (g_duer_idle_cntx.system_state == LIGHTDUER_APP_RECORD_STATE)
				{
					lightduer_app_rec_send_queue(I2S_REC_QUEUE_EVENT_STOP_RECORD,NULL);
					duer_voice_stop();
					g_duer_idle_cntx.duer_operate_state = DUER_RECORD_STOP;
					g_duer_idle_cntx.system_state = LIGHTDUER_APP_IDLE_STATE;
				}
			}
			break;
			case LIGHTDUER_AUDIO_START_LISTEN_IND:
			{
				LOG_I(common,"LIGHTDUER_AUDIO_START_LISTEN_IND");
			}
			break;

			default:
				break;
			}
		}

	}
}

void lightduer_app_set_system_state(lightduer_app_system_state_t state)
{
	g_duer_idle_cntx.system_state = state;

}

static void lightduer_app_idle_key_action(lightduer_app_key_value_t key_value,lightduer_app_key_action_t key_action)
{
	uint32_t index = 0;
	const idle_action_srv_table_t *mapping_table = g_idle_action_mapping_table;
	idle_action_type_t idle_action = LIGHTDUER_IDLE_ACTION_NONE;
    printf("enter lightduer_app_idle_key_action! \n");
	while (LIGHTDUER_KEY_NONE != mapping_table[index].key_value)
	{
		if ((key_value == mapping_table[index].key_value) &&
		        (key_action == mapping_table[index].key_action))
		{
			idle_action = mapping_table[index].idle_action;
			break;
		}
		index++;
	}
    printf("idle_action = %d \n",idle_action);
	switch (idle_action)
	{
	case LIGHTDUER_IDLE_ACTION_START_RECORD:
	{
		if (g_duer_idle_cntx.duer_operate_state == DUER_ON_LINE_PLAY)
		{
		    printf("zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz_re_record\n");
		    g_duer_idle_cntx.duer_operate_state = DUER_RECORD_STOP;
			lightduer_app_player_stop();
            
			//g_duer_idle_cntx.duer_operate_state = DUER_RETRY_RECORD;
			g_duer_idle_cntx.system_state = LIGHTDUER_APP_RECORD_STATE;           
			tuling_app_speech_send_queue(TULING_MASSAGE_RECORD_START);
		}
		else if (g_duer_idle_cntx.system_state == LIGHTDUER_APP_NONE_STATE)
		{
		    printf("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx_start to record\n");
			//lightduer_app_idle_send_queue(LIGHTDUER_REC_START,NULL,0);
			g_duer_idle_cntx.system_state = LIGHTDUER_APP_RECORD_STATE;            
			tuling_app_speech_send_queue(TULING_MASSAGE_RECORD_START);

		}
		else if (g_duer_idle_cntx.system_state == LIGHTDUER_APP_RECORD_STATE)
		{
		    printf("yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy_end record\n");
			lightduer_app_record_close();
			//lightduer_app_idle_send_queue(LIGHTDUER_REC_STOP,NULL,0);
			g_duer_idle_cntx.system_state = LIGHTDUER_APP_NONE_STATE;
            g_duer_idle_cntx.duer_operate_state = DUER_ON_LINE_PLAY;          
			tuling_app_speech_send_queue(TULING_MASSAGE_REQUEST_AND_RESPONSE);
		}

	}
	break;
	case LIGHTDUER_IDLE_ACTION_START_PLAY:
	{
		if (g_duer_idle_cntx.duer_operate_state == DUER_ON_LINE_PLAY)
		{
			duer_dcs_send_play_control_cmd(DCS_PAUSE_CMD);
			g_duer_idle_cntx.manual_stop_play = true;
			LOG_I(common,"Enter lightduer_app_player_stop");
			//lightduer_app_player_stop();
			//g_duer_idle_cntx.duer_operate_state = DUER_ONLINE_PLAY_PAUSE;
		}
		else
		{
			if (g_duer_idle_cntx.system_state == LIGHTDUER_APP_IDLE_STATE)
			{
				lightduer_app_play_local_play();
				g_duer_idle_cntx.system_state = LIGHTDUER_APP_PLAY_STATE;
			}
			if (g_duer_idle_cntx.system_state == LIGHTDUER_APP_RECORD_STATE)
			{
				lightduer_app_record_close();
				lightduer_app_idle_send_queue(LIGHTDUER_REC_STOP,NULL,0);
				lightduer_app_play_local_play();
				g_duer_idle_cntx.system_state = LIGHTDUER_APP_PLAY_STATE;

			}
		}

	}
	break;
	case LIGHTDUER_IDLE_ACTION_SEND_DATA:
	{
		//only debug
		lightduer_app_player_play(g_url,NET_PLAY,0);
	}
	break;
	case LIGHTDUER_IDLE_ACTION_NEXT:
	{
		if (g_duer_idle_cntx.server_state == LIGHTDUER_SERVER_CONNECT)
		{
			if (g_duer_idle_cntx.duer_operate_state == DUER_ON_LINE_PLAY)
			{
				LOG_I(common,"TX DCS_NEXT_CMD");
				duer_dcs_send_play_control_cmd(DCS_NEXT_CMD);
			}
		}
	}
	break;
	case LIGHTDUER_IDLE_ACTION_PREV:
	{
		if (g_duer_idle_cntx.server_state == LIGHTDUER_SERVER_CONNECT)
		{
			if (g_duer_idle_cntx.duer_operate_state == DUER_ON_LINE_PLAY)
			{
				LOG_I(common,"TX DCS_PREVIOUS_CMD");
				duer_dcs_send_play_control_cmd(DCS_PREVIOUS_CMD);
			}
		}
	}
	break;
	case LIGHTDUER_IDLE_ACTION_SMART_LINK:{
				LOG_I(common,"smart link start");
				//lightduer_app_set_system_state(LIGHTDUER_APP_SMART_LINK_STATE);
				airkiss_start();
				}break;

	default:
		break;
	}
}

void lightduer_app_common_key_action(lightduer_app_key_value_t key_value,lightduer_app_key_action_t key_action)
{
	LOG_I(common,"system_state: %d, server_state:%d,operate_state:%d",g_duer_idle_cntx.system_state,g_duer_idle_cntx.server_state,g_duer_idle_cntx.duer_operate_state);
	switch (g_duer_idle_cntx.system_state)
	{
	case LIGHTDUER_APP_OPERATE_SERVER_STATE:
	case LIGHTDUER_APP_IDLE_STATE:
    case 0: // 
	{
		lightduer_app_idle_key_action(key_value,key_action);
	}
	break;

	case LIGHTDUER_APP_RECORD_STATE:
	{
		lightduer_app_idle_key_action(key_value,key_action);
	}
	break;

	case LIGHTDUER_APP_PLAY_STATE:
	{
		lightduer_app_play_key_action(key_value,key_action);
	}
	break;

	default:
		break;

	}

}

void lightduer_app_idle_task(void *arg)
{
	// Set the voice interaction result
	g_duer_idle_cntx.system_state = LIGHTDUER_APP_IDLE_STATE;
	g_duer_idle_cntx.wifi_state = DUER_WIFI_DISCONNECT;
	g_duer_idle_cntx.retry_connect_server_timer = 20;
	g_duer_idle_cntx.manual_stop_play = false;
	g_duer_idle_cntx.bduerEngineerInit = true;
	duer_initialize();
	duer_set_event_callback(lightduer_app_event_hook);
	//g_duerapp_idle_queue_handle = xQueueCreate(IDLE_EVENT_QUEUE_LENGTH, sizeof(lightduer_app_idle_msg_t));
	while(1)
	{
		lightduer_app_idle_handle_common_event();
		vTaskDelay(1000);
	}

}
