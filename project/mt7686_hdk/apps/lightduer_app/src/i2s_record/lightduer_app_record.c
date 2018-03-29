/* Copyright Statement:
 *
 * (C) 2005-2016  MediaTek Inc. All rights reserved.
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. ("MediaTek") and/or its licensors.
 * Without the prior written permission of MediaTek and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 * You may only use, reproduce, modify, or distribute (as applicable) MediaTek Software
 * if you have agreed to and been bound by the applicable license agreement with
 * MediaTek ("License Agreement") and been granted explicit permission to do so within
 * the License Agreement ("Permitted User").  If you are not a Permitted User,
 * please cease any access or use of MediaTek Software immediately.
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT MEDIATEK SOFTWARE RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES
 * ARE PROVIDED TO RECEIVER ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 */



#include "lightduer_app_i2s_record.h"
#include "task_def.h"
#include "hal_i2s.h"
#include "nau8810.h"
#include "hal_i2c_master.h"
#include "hal_audio_internal_service.h"
#include "memory_attribute.h"
#include "wifi_api.h"
#include "ff.h"
#include "http_download_ringbuffer.h"
#include "lightduer_app_idle.h"

#define I2S_TX_VFIFO_LENGTH   512//1024
#define I2S_RX_VFIFO_LENGTH   512//1024

//uint16_t write_buffer[1024 * 3] = {0};
FIL 	fp;
UINT	write_lenth;

#define RECORD_TASK_RECORDING 	1
#define RECORD_TASK_REC_STOP	2
static uint8_t g_record_state = RECORD_TASK_REC_STOP;

ATTR_ZIDATA_IN_NONCACHED_RAM_4BYTE_ALIGN static uint32_t I2S_TX_VFIFO[I2S_TX_VFIFO_LENGTH];
ATTR_ZIDATA_IN_NONCACHED_RAM_4BYTE_ALIGN static uint32_t I2S_RX_VFIFO[I2S_RX_VFIFO_LENGTH];

static QueueHandle_t lightduer_app_rec_queue_handle = NULL;

static hal_i2s_port_t i2s_port = HAL_I2S_1;
static lightduer_app_record_context_t g_lightduer_app_record_cnt;

#if defined I2S_REC_LOG_ENABLE
log_create_module(i2s_record, PRINT_LEVEL_INFO);
#define LOGE(fmt,arg...)   LOG_E(i2s_record, "[I2S REC]: "fmt,##arg)
#define LOGW(fmt,arg...)   LOG_W(i2s_record, "[I2S REC]: "fmt,##arg)
#define LOGI(fmt,arg...)   LOG_I(i2s_record, "[I2S REC]: "fmt,##arg)
#else
#define LOGE(fmt,arg...)
#define LOGW(fmt,arg...)
#define LOGI(fmt,arg...)
#endif
//#define ENABLE_DATA_TRANSFER
#ifdef ENABLE_DATA_TRANSFER
static const uint8_t test_file_array[] = {
	#include "PowerOn.wav.txt"
};
static uint8_t *g_txdata = test_file_array;
static uint32_t g_data_length = 0;
#endif
static uint8_t g_tx_buffer[1024 * 3] = {0};

static void lightduer_app_event_register_callback(i2s_rec_queue_event_id_t reg_id, i2s_rec_callback_t callback)
{
    uint8_t id_idx;
	for (id_idx = 0; id_idx < MAX_I2S_REC_FUNCTIONS; id_idx++){
		if (g_lightduer_app_record_cnt.handler_array[id_idx].in_use_flag == 0){
			g_lightduer_app_record_cnt.handler_array[id_idx].event_id = reg_id;
			g_lightduer_app_record_cnt.handler_array[id_idx].callback_func = callback;
			g_lightduer_app_record_cnt.handler_array[id_idx].in_use_flag = 1;
			g_lightduer_app_record_cnt.i2s_rec_queue_reg_num++;	
			break;
		}
	}
}

static void lightduer_app_event_deregister_callback(i2s_rec_queue_event_id_t dereg_id)
{
    uint32_t id_idx;
    for (id_idx = 0; id_idx < MAX_I2S_REC_FUNCTIONS; id_idx++) {
        if (g_lightduer_app_record_cnt.handler_array[id_idx].event_id == dereg_id) {
            g_lightduer_app_record_cnt.handler_array[id_idx].in_use_flag = 0;
            g_lightduer_app_record_cnt.i2s_rec_queue_reg_num--;
            break;
        }
    }
}

void lightduer_app_rec_send_queue(i2s_rec_queue_event_id_t id, void *parameter)
{
    i2s_rec_queue_event_t event;
    event.id        = id;
    event.parameter = parameter;

    xQueueSend(lightduer_app_rec_queue_handle, &event, 0);
}

static void lightduer_app_rec_event_send_from_isr(i2s_rec_queue_event_id_t id, void *parameter)
{
    i2s_rec_queue_event_t event;
    event.id        = id;
    event.parameter = parameter;
    if (xQueueSendFromISR(lightduer_app_rec_queue_handle, &event, 0) != pdPASS) {
        LOGE("queue not pass %d\r\n", id);
    }
}
static int8_t lightduer_app_i2s_configure(void)
{
    LOGI("[CTRL]i2s_configure \n");
    hal_i2s_config_t i2s_config;
    hal_i2s_status_t result = HAL_I2S_STATUS_OK;

    result = hal_i2s_init_ex(i2s_port, HAL_I2S_TYPE_EXTERNAL_MODE);
    if (HAL_I2S_STATUS_OK != result) {
        LOGE("hal_i2s_init failed\n");
        return -1;
    }

    /* Configure I2S  */
    i2s_config.clock_mode = HAL_I2S_MASTER;
    i2s_config.rx_down_rate = HAL_I2S_RX_DOWN_RATE_DISABLE;
    i2s_config.tx_mode = HAL_I2S_TX_MONO_DUPLICATE_DISABLE;

    //i2s_config.i2s_out.channel_number = HAL_I2S_STEREO;
    //i2s_config.i2s_in.channel_number = HAL_I2S_STEREO;

    i2s_config.i2s_out.channel_number = HAL_I2S_MONO;
    i2s_config.i2s_in.channel_number = HAL_I2S_MONO;

    //i2s_config.i2s_out.sample_rate = HAL_I2S_SAMPLE_RATE_8K;
    //i2s_config.i2s_in.sample_rate = HAL_I2S_SAMPLE_RATE_8K;

    i2s_config.i2s_out.sample_rate = HAL_I2S_SAMPLE_RATE_16K;
    i2s_config.i2s_in.sample_rate = HAL_I2S_SAMPLE_RATE_16K;
    
    i2s_config.i2s_in.msb_offset = 0;
    i2s_config.i2s_out.msb_offset = 0;
    i2s_config.i2s_in.word_select_inverse = HAL_I2S_WORD_SELECT_INVERSE_DISABLE;
    i2s_config.i2s_out.word_select_inverse = HAL_I2S_WORD_SELECT_INVERSE_DISABLE;
    i2s_config.i2s_in.lr_swap = HAL_I2S_LR_SWAP_DISABLE;
    i2s_config.i2s_out.lr_swap = HAL_I2S_LR_SWAP_DISABLE;

    result = hal_i2s_set_config_ex(i2s_port, &i2s_config);
    if (HAL_I2S_STATUS_OK != result) {
        LOGE("hal_i2s_set_config failed\n");
        return -1;
    }

    result = hal_i2s_setup_tx_vfifo_ex(i2s_port, I2S_TX_VFIFO, I2S_TX_VFIFO_LENGTH / 2, I2S_TX_VFIFO_LENGTH);
    if (HAL_I2S_STATUS_OK != result) {
        LOGE("hal_i2s_setup_tx_vfifo failed\n");
        return -1;
    }

    result = hal_i2s_setup_rx_vfifo_ex(i2s_port,I2S_RX_VFIFO, I2S_RX_VFIFO_LENGTH / 2, I2S_RX_VFIFO_LENGTH);
    if (HAL_I2S_STATUS_OK != result) {
        LOGE("hal_i2s_setup_rx_vfifo failed\n");
        return -1;
    }

    //memset(I2S_TxBuf, 0, sizeof I2S_TxBuf);
    //memset(I2S_RxBuf, 0, sizeof I2S_RxBuf);
    return 1;
}


static int8_t lightduer_app_nau8810_configure(void)
{
    LOGI("[CTRL]nau8810_configure \n");
    /*configure NAU8810*/
    AUCODEC_STATUS_e codec_status;
    hal_i2c_port_t i2c_port;
    hal_i2c_frequency_t frequency;

    codec_status = AUCODEC_STATUS_OK;
    i2c_port = HAL_I2C_MASTER_0;
    frequency = HAL_I2C_FREQUENCY_50K;

    codec_status = aucodec_i2c_init(i2c_port, frequency); //init codec
    if (codec_status != AUCODEC_STATUS_OK) {
        LOGE("aucodec_i2c_init failed\n");
    }

    aucodec_softreset();//soft reset

    codec_status = aucodec_init();
    if (codec_status != AUCODEC_STATUS_OK) {
        LOGE("aucodec_init failed\n");
    }

    aucodec_set_dai_fmt(eI2S, e16Bit, eBCLK_NO_INV);//set DAI format

    //codec_status = aucodec_set_dai_sysclk(eSR8KHz, eSLAVE, e32xFS, 24576000, ePLLEnable);//24576000
    
    codec_status = aucodec_set_dai_sysclk(eSR16KHz, eSLAVE, e32xFS, 24576000, ePLLEnable);//24576000
    if (codec_status != AUCODEC_STATUS_OK) {
        LOGE("aucodec_init failed\n");
    }

    aucodec_set_input(eMicIn);//Input: MIC, Output:  ADCOUT
    aucodec_set_output(eSpkOut);//Input: DACIN, Output:  speaker out
    aucodec_set_output(eLineOut);//Input: DACIN, Output:  aux out

    if (codec_status == AUCODEC_STATUS_OK) {
        return 1;
    } else {
        return -1;
    }

}



static void lightduer_app_i2s_open(void)
{
    LOGI("[CTRL]lightduer_app_i2s_open \n");
    hal_i2s_enable_tx_ex(i2s_port);
    hal_i2s_enable_rx_ex(i2s_port);
    hal_i2s_enable_audio_top_ex(i2s_port);
}


static void lightduer_app_hisr_get_data(void *parameter)
{
    //LOGI("[CTRL]lightduer_app_hisr_get_data \n");
    FRESULT ret;
	static uint32_t g_cnt = 0;
    uint32_t tx_vfifo_free_count = 0;
    uint32_t rx_vfifo_data_count = 0;
    uint32_t consume_count = 0;
    uint32_t i = 0;
    uint32_t read_temp = 0;
   
    hal_i2s_get_tx_sample_count_ex(i2s_port, &tx_vfifo_free_count);
    hal_i2s_get_rx_sample_count_ex(i2s_port, &rx_vfifo_data_count);

    while (rx_vfifo_data_count != 0){
		consume_count = MINIMUM(tx_vfifo_free_count, rx_vfifo_data_count);
		consume_count &= 0xFFFFFFFE;
		if (consume_count < 200)
			break;
		if (http_download_ring_buf_free_space() < (consume_count * 2)){
			LOG_I(common,"buffer is full and skip the data\n");
			break;
		}
		for (i = 0; i < consume_count; i++) {
            hal_i2s_rx_read_ex(i2s_port, &read_temp);            
      //      hal_i2s_tx_write_ex(i2s_port, read_temp);
//			read_temp &= 0xFFFF;
			#ifdef ENABLE_DATA_TRANSFER
			http_download_ring_buf_put_two_bytes(test_file_array[g_data_length], test_file_array[g_data_length + 1]);
			g_data_length += 2;
			#else
			http_download_ring_buf_put_two_bytes(read_temp & 0xFF,(read_temp & 0xFF00) >> 8);	
			g_tx_buffer[i * 2] = read_temp & 0xFF;
			g_tx_buffer[i*2 + 1] = (read_temp & 0xFF00) >> 8;	
			#endif
		}
		rx_vfifo_data_count -= consume_count;
        hal_i2s_get_tx_sample_count_ex(i2s_port, &tx_vfifo_free_count);
		#ifdef ENABLE_DATA_TRANSFER
		
		if (g_data_length >= (sizeof(test_file_array) - 2048)){
			lightduer_app_idle_isr_send_message(LIGHTDUER_REC_STOP);
		}else
		#else
        /*
		if (lightduer_app_idle_get_duer_record_state() == DUER_RECORD_RECORDING){
		//	LOG_I(common,"data[0x%x,0x%x,0x%x,0x%x,0x%x,0x%x]", g_tx_buffer[0],g_tx_buffer[1], g_tx_buffer[2],g_tx_buffer[3],g_tx_buffer[4],g_tx_buffer[5]);
			lightduer_app_idle_isr_send_message(LIGHTDUER_REC_DATA);
		}else{
			//skip recording data
			http_download_ring_buf_init();
		}
		*/
		#endif
		g_cnt ++;
		if (g_cnt > 50){
	//		LOGI("write consume_count = %d, write_lenth = %d\n", consume_count, write_lenth);
			g_cnt = 0;
		#ifdef ENABLE_DATA_TRANSFER
			LOGI("write g_data_length = %d, file size = %d\n", g_data_length, sizeof(test_file_array));
		#endif


		}
	}
    hal_i2s_enable_rx_dma_interrupt_ex(i2s_port);

}
void lightduer_app_record_close(void)
{
    LOGI("[CTRL]lightduer_app_record_close enter \n");
    //lightduer_app_event_deregister_callback(I2S_REC_QUEUE_EVENT_GET_DATA_PROCESS);
	hal_i2s_disable_rx_dma_interrupt_ex(i2s_port);
	hal_i2s_disable_tx_dma_interrupt_ex(i2s_port);
	hal_i2s_disable_tx_ex(i2s_port);
    hal_i2s_disable_rx_ex(i2s_port);
    hal_i2s_disable_audio_top_ex(i2s_port);
    hal_i2s_deinit_ex(i2s_port);
    hal_i2s_stop_tx_vfifo_ex(i2s_port);
    hal_i2s_stop_rx_vfifo_ex(i2s_port);
	aucodec_softreset();//soft reset
	aucodec_i2c_deinit();	
    printf("\n\r lightduer_app_record_close exit\n");	
	g_record_state = RECORD_TASK_REC_STOP;	
}

static void lightduer_app_i2s_rx_callback(hal_i2s_event_t event, void *user_data)
{

    switch (event) {
        case HAL_I2S_EVENT_DATA_REQUEST:
			LOGI("[CTRL]i2s_rx_callback HAL_I2S_EVENT_DATA_REQUEST out\n");
            break;
        case HAL_I2S_EVENT_DATA_NOTIFICATION: {
            hal_i2s_disable_rx_dma_interrupt_ex(i2s_port);
            lightduer_app_rec_event_send_from_isr(I2S_REC_QUEUE_EVENT_GET_DATA_PROCESS, user_data);
        }
        break;
    };
}
bool lightduer_app_record_is_recording(){
	
	return (g_record_state == RECORD_TASK_RECORDING);

}
void lightduer_app_record_init(){
	int8_t result = 0;	
	result = lightduer_app_nau8810_configure();
	if (result == -1) {
		LOGE("da7212_configure failed---\n");
	}
	aucodec_set_mic_gain(0);
	
	result = lightduer_app_i2s_configure();
	if (result == -1) {
		LOGE("lightduer_app_i2s_configure failed---\n");
	}

}
void lightduer_app_start_record(){
	http_download_ring_buf_init();
	lightduer_app_event_register_callback(I2S_REC_QUEUE_EVENT_GET_DATA_PROCESS, lightduer_app_hisr_get_data);	
	if (g_lightduer_app_record_cnt.i2s_rec_queue_reg_num > 0){

	    hal_i2s_register_rx_vfifo_callback_ex(i2s_port, lightduer_app_i2s_rx_callback, 
				&g_lightduer_app_record_cnt.handler_array[g_lightduer_app_record_cnt.i2s_rec_queue_reg_num-1]);
		
	    /* enable dma interrupt */
	    hal_i2s_enable_rx_dma_interrupt_ex(i2s_port);
	    lightduer_app_i2s_open();
	}
	g_record_state = RECORD_TASK_RECORDING;
}

void lightduer_app_record_task(void *arg)
{
    LOGI("[CTRL]lightduer_app_record_task \n");
    FATFS fs;
#if 0
    FRESULT res;
    res = f_mount(&fs, "0:", 1);
    if(res == FR_OK){
        printf("mount ok\n");
    }else{
        printf("mount fail\n");
    }
    res = f_open(&fp, "SD:/1.pcm", FA_CREATE_ALWAYS | FA_WRITE | FA_READ);
    if(res == FR_OK){
        printf("open ok\n");
    }else{
        printf("open fail\n");
    }
#endif	
	uint8_t id_idx;
	for (id_idx = 0; id_idx < MAX_I2S_REC_FUNCTIONS; id_idx++) {
		g_lightduer_app_record_cnt.handler_array[id_idx].in_use_flag = 0;
	}

	i2s_rec_queue_event_t event;
	lightduer_app_rec_queue_handle = xQueueCreate(I2S_REC_QUEUE_SIZE, sizeof(i2s_rec_queue_event_t));
	/* Initialize queue registration */

	while (1) {
		if (xQueueReceive(lightduer_app_rec_queue_handle, &event, portMAX_DELAY)) {
			//LOGI("[CTRL]i2s_rec_task_main recieve queue once, rece_id=%d...\n", event.id);
			
			i2s_rec_queue_event_id_t rece_id = event.id;
			i2s_rec_handle_t *hdl = event.parameter;				
			
			switch(event.id){
				case I2S_REC_QUEUE_EVENT_GET_DATA_PROCESS:{
					uint8_t id_idx;
					for (id_idx = 0; id_idx < MAX_I2S_REC_FUNCTIONS; id_idx++) {					
						if ((hdl->handler_array[id_idx].in_use_flag)&&(hdl->handler_array[id_idx].event_id == rece_id)) {
							//LOGI("[CTRL]i2s_rec_task_main handler called\n");
							hdl->handler_array[id_idx].callback_func(NULL);
							break;
						}
					}
				}break;
				case I2S_REC_QUEUE_EVENT_START_RECORD:
					LOGI("I2S_REC_QUEUE_EVENT_START_RECORD!!! \n");
					vTaskDelay(500/portTICK_RATE_MS);
					lightduer_app_record_init();
					lightduer_app_start_record();						
					break;
				case I2S_REC_QUEUE_EVENT_STOP_RECORD:
					lightduer_app_record_close();
					break;
				default:
					break;
			}
		

		}
	}
}




