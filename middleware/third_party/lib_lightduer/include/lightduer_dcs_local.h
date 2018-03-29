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
 * File: lightduer_dcs_local.h
 * Auth: Gang Chen (chengang12@baidu.com)
 * Desc: Provide some functions for dcs module locally.
 */
#ifndef BAIDU_DUER_LIGHTDUER_DCS_LOCAL_H
#define BAIDU_DUER_LIGHTDUER_DCS_LOCAL_H

#include <stdlib.h>
#include <stdbool.h>
#include "baidu_json.h"
#include "lightduer_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * DESC:
 * Get current dialog request id.
 *
 * PARAM: none
 *
 * @RETURN: current dialog request id
 */
int duer_get_request_id_internal(void);

/**
 * DESC:
 * Create a new dialog request id for each conversation.
 *
 * PARAM: none
 *
 * @RETURN: the new dialog request id
 */
int duer_create_request_id_internal(void);

/**
 * DESC:
 * Get speaker controler's state.
 *
 * PARAM: none
 *
 * @RETURN: NULL if failed, pointer point to the state if success.
 */
baidu_json *duer_get_speak_control_state_internal(void);

/**
 * DESC:
 * Get the playback state.
 *
 * PARAM: none
 *
 * @RETURN: NULL if failed, pointer point to the state if success.
 */
baidu_json *duer_get_playback_state_internal(void);

/**
 * DESC:
 * Get the speech state.
 *
 * PARAM: none
 *
 * @RETURN: NULL if failed, pointer point to the state if success.
 */
baidu_json *duer_get_speech_state_internal(void);

/**
 * DESC:
 * Get the alert state.
 *
 * PARAM: none
 *
 * @RETURN: NULL if failed, pointer point to the state if success.
 */
baidu_json *duer_get_alert_state_internal(void);

/**
 * DESC:
 * Get the client context.
 *
 * PARAM: none
 *
 * @RETURN: NULL if failed, pointer point to the client context if success.
 */
baidu_json *duer_get_client_context_internal(void);

/**
 * DESC:
 * Check whether there is a speech waiting to play.
 *
 * PARAM: none
 *
 * @RETURN: true if a speech is waiting to play, false if not.
 */
bool duer_speech_need_play_internal(void);

/**
 * DESC:
 * Pause the audio player.
 *
 * PARAM: none
 *
 * @RETURN: none.
 */
void duer_pause_audio_internal(void);

/**
 * DESC:
 * Resume the audio player.
 *
 * PARAM: none
 *
 * @RETURN: none.
 */
void duer_resume_audio_internal(void);

/**
 * DESC:
 * Notify that speech is stopped.
 *
 * PARAM: none
 *
 * @RETURN: none.
 */
void duer_speech_on_stop_internal(void);

/**
 * DESC:
 * Report exception.
 *
 * @PARAM[in] directive: which directive cause to the exception
 * @PARAM[in] type: exception type
 * @PARAM[in] msg: excetpion content
 *
 * @RETURN: 0 if success, negative if failed.
 */
int duer_report_exception_internal(const char* directive, const char* type, const char* msg);

/**
 * DESC:
 * Declare the system interface.
 *
 * @PARAM: none
 *
 * @RETURN: none.
 */
void duer_declare_sys_interface_internal(void);

/**
 * DESC:
 * Used to reset user activety time.
 *
 * @PARAM: none
 *
 * @RETURN: none.
 */
void duer_user_activity_internal(void);

/**
 * DESC:
 * Returns a pointer to a new string which is a duplicate of the string 'str'.
 *
 * @PARAM[in] str: the string need to duplicated.
 *
 * @RETURN: a pointer to the duplicated string, or NULL if insufficient memory was available.
 */
char *duer_strdup_internal(const char *str);

/**
 * DESC:
 * Used to check whether there is a multiple rounds dialogue.
 *
 * @PARAM: none
 *
 * @RETURN: true if it is multiple round dialogue.
 */
bool duer_is_multiple_round_dialogue(void);

/**
 * DESC:
 * Used to create dcs event.
 *
 * @PARAM[in] namespace: the namespace of the event need to report.
 * @PARAM[in] name: the name the event need to report.
 * @PARAM[in] msg_id: the message_id of the event,
 *                    it could be NULL if the event don't have message_id.
 *
 * @RETURN: pinter of the created dcs event if success, or NULL if failed.
 */
baidu_json *duer_create_dcs_event(const char *namespace, const char *name, const char *msg_id);

#ifdef __cplusplus
}
#endif

#endif/*BAIDU_DUER_LIGHTDUER_DCS_LOCAL_H*/
