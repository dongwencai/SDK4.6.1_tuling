/**
 * Copyright (2017) Baidu Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/**
 * File: lightduer_dcs_dummy.c
 * Auth: Gang Chen (chengang12@baidu.com)
 * Desc: Define some weak symbols for DCS module.
 *       Developers need to implement these APIs according to their requirements.
 */
#include "stdint.h"
#include "stdio.h"
#include "FreeRTOS.h"
#include "task.h"
#include "lightduer_dcs.h"
//#include "lightduer_log.h"
#include "lightduer_dcs_alert.h"
#include "lightduer_ds_log_dcs.h"
#include "lightduer_app_idle.h"
extern int32_t lightduer_app_player_get_progress();

__attribute__((weak)) void duer_dcs_speak_handler(const char *url)
{
 //   DUER_DS_LOG_REPORT_DCS_HANDLER_UNREALIZE();
 //   DUER_LOGW("Please implement this interface to play speech");
	lightduer_app_idle_send_queue(LIGHTDUER_SPEAK_HANDLER_IND, url,0);
}
__attribute__((weak)) void duer_dcs_audio_play_handler(const char *url, const int offset)
{
  //  DUER_DS_LOG_REPORT_DCS_HANDLER_UNREALIZE();
  //  DUER_LOGW("Please implement this interface to play audio");
  	lightduer_app_idle_send_queue(LIGHTDUER_PLAY_HANDLER_IND, url, offset);
}

__attribute__((weak)) void duer_dcs_audio_stop_handler(void)
{
  //  DUER_DS_LOG_REPORT_DCS_HANDLER_UNREALIZE();
  //  DUER_LOGW("Please implement this interface to stop audio player");
  	lightduer_app_idle_send_queue(LIGHTDUER_AUDIO_STOP_IND, NULL, 0);
}

__attribute__((weak)) void duer_dcs_audio_resume_handler(const char *url, const int offset)
{
  //  DUER_DS_LOG_REPORT_DCS_HANDLER_UNREALIZE();
  	lightduer_app_idle_send_queue(LIGHTDUER_AUDIO_RESUME_IND, NULL, 0);
}

__attribute__((weak)) void duer_dcs_audio_pause_handler(void)
{
   // DUER_DS_LOG_REPORT_DCS_HANDLER_UNREALIZE();
   lightduer_app_idle_send_queue(LIGHTDUER_AUDIO_PAUSE_IND, NULL, 0);
}

__attribute__((weak)) int duer_dcs_audio_get_play_progress(void)
{
   // DUER_DS_LOG_REPORT_DCS_HANDLER_UNREALIZE();
  	printf("Please implement this interface, it's used to get the audio play progress\n\r");
    return lightduer_app_player_get_progress();
}

__attribute__((weak)) void duer_dcs_get_speaker_state(int *volume, bool *is_mute)
{
  //  DUER_DS_LOG_REPORT_DCS_HANDLER_UNREALIZE();
  //  DUER_LOGW("Please implement this interface to get speaker state");
}

__attribute__((weak)) void duer_dcs_volume_set_handler(int volume)
{
  //  DUER_DS_LOG_REPORT_DCS_HANDLER_UNREALIZE();
 //   DUER_LOGW("Please implement this interface to set volume");
}

__attribute__((weak)) void duer_dcs_volume_adjust_handler(int volume)
{
  //  DUER_DS_LOG_REPORT_DCS_HANDLER_UNREALIZE();
   // DUER_LOGW("Please implement this interface to adjust volume");
}

__attribute__((weak)) void duer_dcs_mute_handler(bool is_mute)
{
 //   DUER_DS_LOG_REPORT_DCS_HANDLER_UNREALIZE();
  //  DUER_LOGW("Please implement this interface to set mute state");
}

__attribute__((weak)) void duer_dcs_listen_handler(void)
{
  //  DUER_DS_LOG_REPORT_DCS_HANDLER_UNREALIZE();
  //  DUER_LOGW("Please implement this interface to start recording");
     lightduer_app_idle_send_queue(LIGHTDUER_AUDIO_START_LISTEN_IND, NULL, 0);
}

__attribute__((weak)) void duer_dcs_stop_listen_handler(void)
{
  //  DUER_DS_LOG_REPORT_DCS_HANDLER_UNREALIZE();
  //  DUER_LOGW("Please implement this interface to stop recording");
     lightduer_app_idle_send_queue(LIGHTDUER_AUDIO_STOP_LISTEN_IND, NULL, 0);    
}

__attribute__((weak)) void duer_dcs_alert_set_handler(const char *token,
                                                      const char *time,
                                                      const char *type)
{
  //  DUER_DS_LOG_REPORT_DCS_HANDLER_UNREALIZE();
  //  DUER_LOGW("Please implement this interface to set alert");
}

__attribute__((weak)) void duer_dcs_alert_delete_handler(const char *token)
{
 //   DUER_DS_LOG_REPORT_DCS_HANDLER_UNREALIZE();
 //   DUER_LOGW("Please implement this interface to delete alert");
}

__attribute__((weak)) void duer_dcs_get_all_alert(baidu_json *alert_array)
{
 //   DUER_DS_LOG_REPORT_DCS_HANDLER_UNREALIZE();
 //   DUER_LOGW("Please implement this interface to report all alert info");
}

