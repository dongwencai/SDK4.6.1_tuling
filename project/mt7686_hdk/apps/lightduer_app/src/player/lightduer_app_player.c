#include "stdint.h"
#include "stdio.h"
#include "FreeRTOS.h"
#include "task.h"
#include "lightduer_app_player.h"
#include "queue.h"
#include "audio_file_manager.h"
#include "semphr.h"
#include "lightduer_app_i2s_record.h"
#include "lightduer_app_idle.h"

#define PLAYER_EVENT_QUEUE_LENGTH 10 // 20 

//#define RECORD_FILE_FOR_DEBUG

#if defined(RECORD_FILE_FOR_DEBUG)
static bool isOpen = false;
static FATFS fatfs;
static FIL fp;
static UINT length_write;
static FRESULT res;
#endif


lightduer_app_player_context_t g_player_context;
static QueueHandle_t g_player_event_queue_handle;
static SemaphoreHandle_t g_player_event_semaphore = NULL;

static int32_t lightduer_app_change_track(uint8_t type);

static const player_action_srv_table_t g_player_action_mapping_table[] =
{
	{
		LIGHTDUER_KEY_PLAY,
		LIGHTDUER_KEY_ACT_PRESS_UP,
		AUDIO_PLAYER_ACTION_PLAY
	},
	{
		LIGHTDUER_KEY_NEXT,
		LIGHTDUER_KEY_ACT_PRESS_UP,
		AUDIO_PLAYER_ACTION_NEXT_TRACK
	},
	{
		LIGHTDUER_KEY_PREV,
		LIGHTDUER_KEY_ACT_PRESS_UP,
		AUDIO_PLAYER_ACTION_PRE_TRACK
	},
	{
		LIGHTDUER_KEY_RECORD,
		LIGHTDUER_KEY_ACT_PRESS_UP,
		AUDIO_PLAYER_ACTION_RECORD
	}

};

static void lightduer_player_take_semaphore(void)
{
	xSemaphoreTake(g_player_event_semaphore, portMAX_DELAY);
}


static void lightduer_player_give_semaphore(void)
{
	xSemaphoreGive(g_player_event_semaphore);
}


static void lightduer_player_give_semaphore_from_isr(void)
{
	BaseType_t  x_higher_priority_task_woken = pdFALSE;
	xSemaphoreGiveFromISR(g_player_event_semaphore, &x_higher_priority_task_woken);
	portYIELD_FROM_ISR( x_higher_priority_task_woken );
}

void lightduer_app_player_send_queue(player_msg_type_t msg)
{
	player_msg_t message;
	message.msg_id	 = msg;
	xQueueSend(g_player_event_queue_handle, (void*)&message, 0);
}

int32_t lightduer_app_player_get_progress()
{
	return g_player_context.playing_pos;
}

void lightduer_app_player_init()
{

}
void lightduer_app_read_data(void* buff,int32_t btr,int32_t *br)
{

	if (g_player_context.play_mode == LOCAL_PLAY)
	{
		audio_file_manager_read(&g_player_context.fdst,buff, btr, br,0,NULL);
	}
	else
	{
		if (g_player_context.isSeeking)
		{
			if (http_download_ring_buf_data_count() > (g_player_context.lseek_position - g_player_context.playing_pos))
			{
				http_download_ring_buf_update_read_pointer(g_player_context.lseek_position - g_player_context.playing_pos);
				g_player_context.playing_pos = g_player_context.lseek_position;
				g_player_context.isSeeking = false;
			}
		}
		else
		{
			*br = http_download_ring_buf_get_data(buff,btr);
			if ((btr > *br) && (g_player_context.file_size == 4 * 1024))
			{
				memset(buff + *br, 0,btr - *br);
				*br = btr;
			}
#ifdef RECORD_FILE_FOR_DEBUG
			if (isOpen)
			{
				res = f_write(&fp, buff, *br, &length_write);
				if(res != FR_OK)
				{
					LOG_I(common,"write fail\n");
				}
			}

			if (isOpen)
			{
				if (g_player_context.playing_pos >= 1024 * 1024)
				{
					isOpen = false;
					res = f_close(&fp);
					if(res != FR_OK)
					{
						LOG_I(common,"close file fail\n");
					}
					else
					{
						LOG_I(common,"close file success\n");
					}
				}
			}
#endif

			g_player_context.playing_pos += *br;
//			LOG_I(common,"read data length = %d,request data size = %d", *br,btr);
//			LOG_I(common,"g_player_context.playing_pos = %d,g_player_context.file_size = %d", g_player_context.playing_pos,g_player_context.file_size);
		}
	}
}
void lightduer_app_get_player_state(uint8_t player_state)
{
	if (g_player_context.file_type == MP3_FILE_TYPE)
	{
		if (g_player_context.play_mode == LOCAL_PLAY)
		{
			if (player_state == MP3_PLAY_NEXT_TRACK)
			{
				//	audio_file_manager_close(&g_player_context.fdst);
				LOG_I(common,"[Player]play next song!!!");
				lightduer_app_change_track(NORMAL_TRACK);
			}
		}
		else
		{
			if (player_state == MP3_PLAY_FINISH)
			{
				lightduer_app_player_send_queue(ONLINE_PLAYER_MSG_STOP);
			}
			else if (player_state == MP3_PLAY_NEXT_TRACK)
			{
				lightduer_app_player_send_queue(ONLINE_PLAYER_MSG_STOP);
			}
		}

	}
}
bool lightduer_app_file_over()
{
	bool ret = false;;
	if (g_player_context.play_mode == LOCAL_PLAY)
	{
		//local
		ret = audio_file_manager_eof(&g_player_context.fdst,0,NULL);
	}
	else
	{
		//net
		if (g_player_context.playing_pos >= g_player_context.file_size)
		{
			ret = true;
		}
		else
		{
			if (http_is_connect())
			{
				ret = false;
			}
			else if (http_download_ring_buf_data_count() == 0)
			{
				ret = true;
			}
		}
	}
	return ret;
}


FRESULT lightduer_app_file_lseek(uint32_t pos)
{
	uint8_t count;
	FRESULT res = FR_NO_FILE;
#ifdef RECORD_FILE_FOR_DEBUG
	uint8_t buffer[1024 * 50];
#endif
	LOG_I(common,"[lightduer_app_file_lseek] pos = %d", pos);
	if (g_player_context.play_mode == LOCAL_PLAY)
	{
		res = audio_file_manager_lseek(&g_player_context.fdst, pos,NULL);
		if (res != FR_OK)
		{
			LOG_I(common,"[Player] seek file failure");
		}
	}
	else
	{
		g_player_context.lseek_position = pos;
		if (pos < g_player_context.playing_pos)
		{
			http_download_ring_buf_lseek(pos);
			g_player_context.playing_pos = pos;
			return res;
		}
		for (count = 0; count < 50; count ++)
		{
			if (http_download_ring_buf_data_count() < (pos - g_player_context.playing_pos))
			{
				vTaskDelay(100/portTICK_RATE_MS);
			}
			else
			{
#ifdef RECORD_FILE_FOR_DEBUG
				if (isOpen)
				{
					res = f_write(&fp, buffer, pos - g_player_context.playing_pos, &length_write);
					if(res != FR_OK)
					{
						LOG_I(common,"write fail\n");
					}
				}
#endif
				http_download_ring_buf_update_read_pointer(pos - g_player_context.playing_pos);
				//LOG_I(common,"[Player] seek count = %d,g_player_context.file_pointer = %d", count,g_player_context.file_pointer);
				g_player_context.playing_pos = pos;
				break;
			}
		}

	}
	return res;

}

static int16_t lightduer_app_open_file_from_sd(FIL *fp, const TCHAR *path, uint32_t offset)
{
	FRESULT res;
	uint32_t filesize;
	LOG_I(common,"[Player] File path = %s,offset = %d",path,offset);
	res = audio_file_manager_open(fp, path);
	if(res == FR_OK)
	{
		filesize = audio_file_manager_size(fp,0);
		if (filesize > offset)
		{
			res = audio_file_manager_lseek(fp, offset,NULL);
			if (res != FR_OK)
			{
				LOG_I(common,"[Player] seek file failure");
			}
		}

	}
	else
	{
		LOG_I(common,"[Player] open file failure");
	}
	return res;
}
play_file_type_t lightduer_app_get_file_type(char *file_path)
{
	play_file_type_t type;

	int32_t len = 0;
	len = ucs2_wcslen(file_path);
	if ((ucs2_wcsncmp(file_path + len - 4, (const TCHAR *)".MP3", 4)) == 0)
	{
		g_player_context.file_type = MP3_FILE_TYPE;
		type = MP3_FILE_TYPE;

	}
	else if ((ucs2_wcsncmp(file_path + len - 4, (const TCHAR *)".WAV", 4)) == 0)
	{
		type = WAV_FILE_TYPE;
		g_player_context.file_type = WAV_FILE_TYPE;
	}
	else
	{
		type = FILE_TYPE_NONE;
	}

	return type;
}


int32_t lightduer_app_player_play(char *url,play_type_t mode, uint32_t offset)
{
	int32_t result = AUDIO_PLAYER_ERR_FAIL_1ST;
	g_player_context.play_mode = mode;
	if (g_player_context.play_mode == LOCAL_PLAY)
	{
		//local play
		switch (lightduer_app_get_file_type(url))
		{
		case MP3_FILE_TYPE:
			if (lightduer_app_open_file_from_sd(&g_player_context.fdst,url, offset) == 0)
			{
				lightduer_mp3_player_callback(lightduer_app_read_data,audio_file_manager_size(&g_player_context.fdst,0),lightduer_app_get_player_state);
				lightduer_mp3_player_play();
				g_player_context.player_state = PLAYER_PLAYING;
				result = AUDIO_PLAYER_ERR_SUCCESS_OK;
			}
			break;
		case WAV_FILE_TYPE:
			break;
		default:
			break;
		}

	}
	else
	{
		//http play
		if (url != NULL)
		{
			if (g_player_context.player_state == PLAYER_IDLE)
			{
				http_download_ring_buf_init();
				g_player_context.online_play_url = url;
				lightduer_app_player_send_queue(ONLINE_PLAYER_MSG_START_DOWNLOAD);
				g_player_context.playing_pos = 0;
				g_player_context.player_state = ONLINE_PLAYER_DOWNLOAD;
				result = AUDIO_PLAYER_ERR_SUCCESS_OK;

			}
		}
	}
	return result;
}
void lightduer_app_player_pause()
{
	if (g_player_context.file_type == MP3_FILE_TYPE)
	{
		lightduer_mp3_player_pause();

	}
	g_player_context.player_state = PLAYER_PAUSE;
}

void lightduer_app_player_resume()
{
	if (g_player_context.file_type == MP3_FILE_TYPE)
	{
		lightduer_mp3_player_resume();
	}
	g_player_context.player_state = PLAYER_RESUME;

}
void lightduer_app_player_auto_stop()
{
	if (g_player_context.play_mode == LOCAL_PLAY)
	{
		if (g_player_context.file_type == MP3_FILE_TYPE)
		{
			lightduer_mp3_player_stop();
			audio_file_manager_close(&g_player_context.fdst);
			g_player_context.player_state = PLAYER_STOP;
		}
	}
	else
	{
		lightduer_app_player_send_queue(ONLINE_PLAYER_MSG_STOP);
	}
	g_player_context.player_state = PLAYER_STOP;

}

void lightduer_app_player_stop()
{
	if (g_player_context.play_mode == LOCAL_PLAY)
	{
		local_mp3_breakpoint_data_t bp_data;
		if (g_player_context.file_type == MP3_FILE_TYPE)
		{
			lightduer_mp3_player_stop();
		}
		g_player_context.offset = audio_file_manager_tell(&g_player_context.fdst, NULL);
		audio_file_manager_close(&g_player_context.fdst);
		bp_data.offset = g_player_context.offset;
		bp_data.file_id = g_player_context.file_id;
		bp_data.folder_id = g_player_context.folder_id;
		audio_file_manager_set_breakpoint((void *)&bp_data);
		g_player_context.player_state = PLAYER_STOP;
	}
	else
	{
		lightduer_app_player_send_queue(ONLINE_PLAYER_MSG_STOP);
	}
}
void lightduer_app_player_deinit()
{
	if (g_player_context.play_mode == LOCAL_PLAY)
	{
		if (g_player_context.file_type == MP3_FILE_TYPE)
		{
			lightduer_mp3_player_deinit();
		}
	}
	else
	{
		lightduer_app_player_send_queue(ONLINE_PLAYER_MSG_CLOSE);
	}
	g_player_context.player_state = PLAYER_IDLE;

}

static void lightduer_app_player_handle_common_event(void)
{

	player_msg_t msgs;
	uint8_t *share_buf;
	uint32_t share_buf_len;
	int count = 0;
	while (1)
	{
		if (xQueueReceive(g_player_event_queue_handle, &msgs, portMAX_DELAY))
		{
			//	LOG_I(common,"player_handle_common_event:msg_id = %d, count = %d",msgs.msg_id, http_download_ring_buf_data_count());
			switch (msgs.msg_id)
			{
			case ONLINE_PLAYER_MSG_START_DOWNLOAD:
				//	vTaskDelay(500/portTICK_RATE_MS);
				http_start_download(g_player_context.online_play_url);
				lightduer_app_player_send_queue(ONLINE_PLAYER_MSG_WAIT_DATA);
				break;
			case ONLINE_PLAYER_MSG_WAIT_DATA:
				/* prefill data to share  buffer */
				if (http_download_ring_buf_data_count() >= 1 * 1024)
				{
					g_player_context.file_type = MP3_FILE_TYPE; //only debug
					if(g_player_context.player_state == PLAYER_PAUSE)
					{
					    lightduer_app_player_send_queue(ONLINE_PLAYER_MSG_RESUME);
					}
					else
					{
						lightduer_app_player_send_queue(ONLINE_PLAYER_MSG_START_PLAYING);
					}
					break;
				}
				else
				{
					lightduer_app_player_send_queue(ONLINE_PLAYER_MSG_WAIT_DATA);
//						g_player_context.player_state = ONLINE_PLAYER_DOWNLOAD;
//						vTaskDelay(500/portTICK_RATE_MS);
					vTaskDelay(10/portTICK_RATE_MS);

				}
				break;
			case ONLINE_PLAYER_MSG_START_PLAYING:
				if (g_player_context.file_type == MP3_FILE_TYPE)
				{
					g_player_context.file_size = http_download_getfile_size();
					if (g_player_context.file_size < 1024 * 4)
					{
						g_player_context.file_size = 1024 * 4;
					}
					for (; lightduer_app_record_is_recording();)
					{
						vTaskDelay(10/portTICK_RATE_MS);
					}
					LOG_I(common,"start play mp3,data count = %d",http_download_ring_buf_data_count());
#ifdef RECORD_FILE_FOR_DEBUG
					res = f_open(&fp, "SD:/3.mp3", FA_CREATE_ALWAYS | FA_WRITE);
					if(res != FR_OK)
					{
						printf("open fail\n");
					}
					isOpen = true;
#endif

					lightduer_mp3_player_callback(lightduer_app_read_data,g_player_context.file_size,lightduer_app_get_player_state);
					lightduer_mp3_player_play();
				}
				g_player_context.player_state = PLAYER_PLAYING;
				break;

			case ONLINE_PLAYER_MSG_STOP:
				if (g_player_context.player_state != PLAYER_STOP)
				{
					g_player_context.player_state = PLAYER_STOP;
					if (g_player_context.file_type == MP3_FILE_TYPE)
					{
						lightduer_mp3_player_stop();
					}
					g_player_context.player_state = PLAYER_IDLE;
					http_stop_download();
#ifdef RECORD_FILE_FOR_DEBUG
					if (isOpen)
					{
						isOpen = false;
						res = f_close(&fp);
						if(res != FR_OK)
						{
							LOG_I(common,"close file fail\n");
						}
						else
						{
							LOG_I(common,"close file success\n");
						}
					}
#endif

					lightduer_app_idle_send_queue(LIGHTDUER_AUDIO_STOP, NULL,0);
				}
				break;
			case ONLINE_PLAYER_MSG_CLOSE:
				g_player_context.player_state = PLAYER_CLOSE;
				if (g_player_context.file_type == MP3_FILE_TYPE)
				{
					lightduer_mp3_player_stop();

				}
				http_stop_download();
				g_player_context.player_state = PLAYER_IDLE;
				break;
			case ONLINE_PLAYER_MSG_PAUSE:
				if (g_player_context.file_type == MP3_FILE_TYPE)
				{
					lightduer_mp3_player_pause();
				}
				g_player_context.player_state = PLAYER_PAUSE;
				break;
			case ONLINE_PLAYER_MSG_RESUME:
				if (g_player_context.file_type == MP3_FILE_TYPE)
				{
					lightduer_mp3_player_resume();
				}
				g_player_context.player_state = PLAYER_RESUME;

				break;
			default:
				break;
			}
		}

	}
}

static int32_t lightduer_app_change_track(uint8_t type)
{
	int32_t ret = 0;
	uint16_t index = 0;
	uint32_t offset = 0;
	if ((g_player_context.player_state == PLAYER_PLAYING) ||
	        (g_player_context.player_state == PLAYER_PAUSE) ||
	        (g_player_context.player_state == PLAYER_RESUME))
	{
		lightduer_app_player_auto_stop();
	}

	index = g_player_context.file_id;
	if (type == NEXT_TRACK || type == PREVIOUS_TRACK || type == NORMAL_TRACK)
	{
		if (NEXT_TRACK == type)
		{
			if (index == g_player_context.file_total)
			{
				index = AUDIO_PLAYER_DEFAULT_FILE_ID;
			}
			else
			{
				index = index + 1;
			}
		}
		else if (PREVIOUS_TRACK == type)
		{
			if (index == AUDIO_PLAYER_DEFAULT_FILE_ID)
			{
				index = g_player_context.file_total;
			}
			else
			{
				index = index - 1;
			}
		}
		else if (NORMAL_TRACK == type)
		{
			if (index == g_player_context.file_total)
			{
				index = 1;
			}
			else
			{
				index = index + 1;
			}
		}


	}
	else
	{
		LOG_E(common,"[AudPly] change track type error");
		return AUDIO_PLAYER_ERR_FAIL_6TH;
	}
	g_player_context.file_id = index;
	g_player_context.offset = 0;
	ret = audio_file_manager_get_file_path_by_idx(g_player_context.curr_file_path,
	        MAX_FILE_PATH_LEN, 1, g_player_context.file_id);
	if (ret == AUD_STATUS_CODE_SUCCESS)
	{

		lightduer_app_player_play(g_player_context.curr_file_path,LOCAL_PLAY,g_player_context.offset);
	}

	return ret;
}



int32_t lightduer_app_play_local_play(void)
{
	int32_t err = 0;
	if (g_player_context.file_total > 0)
	{
		err = audio_file_manager_get_file_path_by_idx(g_player_context.curr_file_path,
		        MAX_FILE_PATH_LEN, 1, g_player_context.file_id);
		if (err == AUD_STATUS_CODE_SUCCESS)
		{
			lightduer_app_player_play(g_player_context.curr_file_path,LOCAL_PLAY,g_player_context.offset);
		}
	}
	return err;
}


player_action_status_t lightduer_app_play_key_action(lightduer_app_key_value_t key_value,
        lightduer_app_key_action_t key_action)
{
	uint32_t index = 0;
	const player_action_srv_table_t *mapping_table = g_player_action_mapping_table;
	player_action_type_t player_action = AUDIO_PLAYER_ACTION_NONE;
	player_action_status_t result = PLAYER_ACTION_SRV_STATUS_FAIL;
	while (LIGHTDUER_KEY_NONE != mapping_table[index].key_value)
	{
		if ((key_value == mapping_table[index].key_value) &&
		        (key_action == mapping_table[index].key_action))
		{
			player_action = mapping_table[index].player_action;
			result = PLAYER_ACTION_SRV_STATUS_SUCCESS;
			break;
		}
		index++;
	}
	switch(player_action)
	{
	case AUDIO_PLAYER_ACTION_PLAY:
		if (g_player_context.player_state == PLAYER_PLAYING)
		{
			g_player_context.player_state = PLAYER_STOPPING;
			lightduer_app_player_stop();
		}
		else
		{
			if (g_player_context.player_state == PLAYER_STOP)
			{
				g_player_context.player_state = PLAYER_START_PLAY;
				lightduer_app_play_local_play();
			}
		}
		break;
	case AUDIO_PLAYER_ACTION_NEXT_TRACK:
		if (g_player_context.player_state == PLAYER_PLAYING)
		{
			lightduer_app_change_track(NEXT_TRACK);
		}
		break;
	case AUDIO_PLAYER_ACTION_PRE_TRACK:
		if (g_player_context.player_state == PLAYER_PLAYING)
		{
			lightduer_app_change_track(PREVIOUS_TRACK);
		}
		break;
	case AUDIO_PLAYER_ACTION_RECORD:
		if (g_player_context.player_state != PLAYER_STOP)
		{
			lightduer_app_player_stop();
		}
		lightduer_app_idle_send_queue(LIGHTDUER_REC_START,NULL,0);
		break;
	default:
		break;
	}
	return result;
}
void lightduer_app_play_task(void *arg)
{

	local_mp3_breakpoint_data_t bp_data;
	lightduer_mp3_player_init();
	audio_file_manager_init();
	audio_file_manager_get_file_num_by_idx(&g_player_context.file_total, 1);


	if (g_player_context.file_total > 0)
	{
		audio_file_manager_get_breakpoint((void *)&bp_data);
		g_player_context.folder_id = bp_data.folder_id;
		g_player_context.file_id = bp_data.file_id;
		g_player_context.offset = bp_data.offset;
		LOG_I(common, "file_total = %d,folder_id = %d, file_id = %d,offset = %d", g_player_context.file_total,bp_data.folder_id, bp_data.file_id,bp_data.offset);
	}
	g_player_event_semaphore = xSemaphoreCreateBinary();
	g_player_event_queue_handle = xQueueCreate(PLAYER_EVENT_QUEUE_LENGTH, sizeof(player_msg_t));

	while(1)
	{
		lightduer_app_player_handle_common_event();
		vTaskDelay(1000);
	}
}
