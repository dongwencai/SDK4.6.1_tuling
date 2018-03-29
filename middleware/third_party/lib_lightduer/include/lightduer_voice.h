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
 * File: lightduer_voice.h
 * Auth: Su Hao (suhao@baidu.com)
 * Desc: Light duer voice APIS.
 */

#include "baidu_json.h"

#ifndef BAIDU_DUER_LIGHTDUER_INCLUDE_LIGHTDUER_VOICE_H
#define BAIDU_DUER_LIGHTDUER_INCLUDE_LIGHTDUER_VOICE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _duer_voice_mode_enum {
    DUER_VOICE_MODE_DEFAULT,
    DUER_VOICE_MODE_CHINESE_TO_ENGLISH,
    DUER_VOICE_MODE_ENGLISH_TO_CHINESE
} duer_voice_mode;

typedef void (*duer_voice_result_func)(baidu_json *result);

void duer_voice_set_result_callback(duer_voice_result_func callback);

void duer_voice_set_mode(duer_voice_mode mode);

int duer_voice_start(int samplerate);

int duer_voice_send(const void *data, size_t size);

int duer_voice_stop(void);

/*
 * topic_id + 1
 *
 * * Return: Other topic_id value
 *           0   Failed
 */
size_t duer_increase_topic_id(void);

#ifdef __cplusplus
}
#endif

#endif/*BAIDU_DUER_LIGHTDUER_INCLUDE_LIGHTDUER_VOICE_H*/
