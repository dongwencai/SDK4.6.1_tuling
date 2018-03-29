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



#include "i2s_rec.h"
#include "task_def.h"
#include "hal_i2s.h"
#include "nau8810.h"
#include "hal_i2c_master.h"
#include "hal_audio_internal_service.h"
#include "memory_attribute.h"
#include "wifi_api.h"
#include "ff.h"
/* for I2S record demo */
#define I2S_REC_DEMO_TASK_NAME       "i2s_rec_demo"
#define I2S_REC_DEMO_TASK_STACKSIZE  1024
#define I2S_REC_DEMO_TASK_PRIO       TASK_PRIORITY_NORMAL


#define I2S_TX_VFIFO_LENGTH   1024
#define I2S_RX_VFIFO_LENGTH   1024

uint16_t write_buffer[1024 * 3] = {0};
FIL 	fp;
UINT	write_lenth;




ATTR_ZIDATA_IN_NONCACHED_RAM_4BYTE_ALIGN static uint32_t I2S_TX_VFIFO[I2S_TX_VFIFO_LENGTH];
ATTR_ZIDATA_IN_NONCACHED_RAM_4BYTE_ALIGN static uint32_t I2S_RX_VFIFO[I2S_RX_VFIFO_LENGTH];


#if 0 //for test
static uint16_t audio_Tone2k_16kSR[32] = {
    0xffff, 0xcd1a, 0xb805, 0xcd1b, 0x0000, 0x32e6, 0x47fb, 0x32e5,
    0x0000, 0xcd1a, 0xb805, 0xcd1b, 0x0000, 0x32e5, 0x47fb, 0x32e5,
    0x0000, 0xcd1b, 0xb805, 0xcd1b, 0x0000, 0x32e5, 0x47fb, 0x32e5,
    0x0000, 0xcd1b, 0xb806, 0xcd1a, 0xffff, 0x32e5, 0x47f9, 0x32e6
};
#endif

# define testmem 1


static QueueHandle_t i2s_rec_queue_handle = NULL;
static uint8_t i2s_rec_queue_reg_num = 0;
static i2s_rec_queue_event_id_t i2s_rec_queue_id_array[MAX_I2S_REC_FUNCTIONS];
static i2s_rec_callback_t i2s_rec_queue_handler[MAX_I2S_REC_FUNCTIONS];

static hal_i2s_port_t i2s_port = HAL_I2S_1;

//typedef struct i2s_rec_handle_s i2s_rec_handle_t;

struct i2s_rec_s {  
	i2s_rec_queue_event_id_t event_id;
	uint8_t in_use_flag;
    void (*callback_func)(void *parameter);     
};

typedef struct  {                                       
    struct i2s_rec_s handler_array[MAX_I2S_REC_FUNCTIONS];
}i2s_rec_handle_t;




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

static void i2s_rec_event_register_callback(i2s_rec_queue_event_id_t reg_id, i2s_rec_callback_t callback)
{
    uint8_t id_idx;
    for (id_idx = 0; id_idx < MAX_I2S_REC_FUNCTIONS; id_idx++) {
        if (i2s_rec_queue_id_array[id_idx] == I2S_REC_QUEUE_EVENT_NONE) {
            i2s_rec_queue_id_array[id_idx] = reg_id;
            i2s_rec_queue_handler[id_idx] = callback;
            LOGI("i2s_rec_queue_id_array[%u]=%u\n", id_idx, i2s_rec_queue_id_array[id_idx]);
            i2s_rec_queue_reg_num++;
            break;
        }
    }
}

#if 1 //example for deregistering  callback, mark as below to avoid build warning
static void i2s_rec_event_deregister_callback(i2s_rec_queue_event_id_t dereg_id)
{
    uint32_t id_idx;
    for (id_idx = 0; id_idx < MAX_I2S_REC_FUNCTIONS; id_idx++) {
        if (i2s_rec_queue_id_array[id_idx] == dereg_id) {
            i2s_rec_queue_id_array[id_idx] = I2S_REC_QUEUE_EVENT_NONE;
            i2s_rec_queue_reg_num--;
            break;
        }
    }
}
#endif

static void i2s_rec_event_send_from_isr(i2s_rec_queue_event_id_t id, void *parameter)
{
    i2s_rec_queue_event_t event;
    event.id        = id;
    event.parameter = parameter;
    if (xQueueSendFromISR(i2s_rec_queue_handle, &event, 0) != pdPASS) {
        LOGE("queue not pass %d\r\n", id);
    }
}


static void i2s_rec_task_main(void *arg)
{
    LOGI("[CTRL]i2s_rec_task_main \n");

    i2s_rec_queue_event_t event;

    i2s_rec_queue_handle = xQueueCreate(I2S_REC_QUEUE_SIZE, sizeof(i2s_rec_queue_event_t));
    /* Initialize queue registration */
    uint8_t id_idx;
    for (id_idx = 0; id_idx < MAX_I2S_REC_FUNCTIONS; id_idx++) {
        i2s_rec_queue_id_array[id_idx] = I2S_REC_QUEUE_EVENT_NONE;
    }

#ifndef testmem
    while (1) {
        if (xQueueReceive(i2s_rec_queue_handle, &event, portMAX_DELAY)) {
			LOGI("[CTRL]i2s_rec_task_main recieve queue once, rece_id=%d...\n", event.id);
            i2s_rec_queue_event_id_t rece_id = event.id;
            uint8_t id_idx;
            for (id_idx = 0; id_idx < MAX_I2S_REC_FUNCTIONS; id_idx++) {
				LOGI("[CTRL]i2s_rec_task_main check once, id_idx=%d, rece_id=%d...\n", i2s_rec_queue_id_array[id_idx], rece_id);
                if (i2s_rec_queue_id_array[id_idx] == rece_id) {
					LOGI("[CTRL]i2s_rec_task_main handler called\n");
                    i2s_rec_queue_handler[id_idx](event.parameter);
                    break;
                }
            }
        }
    }
#else
	while (1) {
		if (xQueueReceive(i2s_rec_queue_handle, &event, portMAX_DELAY)) {
			//LOGI("[CTRL]i2s_rec_task_main recieve queue once, rece_id=%d...\n", event.id);
			i2s_rec_queue_event_id_t rece_id = event.id;
			i2s_rec_handle_t *hdl = event.parameter;
			
			uint8_t id_idx;
			for (id_idx = 0; id_idx < MAX_I2S_REC_FUNCTIONS; id_idx++) {
				//LOGI("[CTRL]i2s_rec_task_main check once, id_idx=%d, rece_id=%d...\n", i2s_rec_queue_id_array[id_idx], rece_id);
				if ((hdl->handler_array[id_idx].in_use_flag)&&(hdl->handler_array[id_idx].event_id == rece_id)) {
					//LOGI("[CTRL]i2s_rec_task_main handler called\n");
					hdl->handler_array[id_idx].callback_func(NULL);
					break;
				}
			}
		}
	}

#endif

}


void i2s_rec_task_create(void)
{
    LOGI("[CTRL]i2s_rec_task_create \n");
    xTaskCreate(i2s_rec_task_main, "i2s_task", 1024 / sizeof(portSTACK_TYPE), NULL, TASK_PRIORITY_SOFT_REALTIME, NULL);
}


static int8_t i2s_configure(void)
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


static int8_t nau8810_configure(void)
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



static void i2s_open(void)
{
    LOGI("[CTRL]i2s_open \n");
    hal_i2s_enable_tx_ex(i2s_port);
    hal_i2s_enable_rx_ex(i2s_port);
    hal_i2s_enable_audio_top_ex(i2s_port);
}


static void hisr_get_data(void *parameter)
{
    //LOGI("[CTRL]hisr_get_data \n");
    FRESULT ret;

    uint32_t tx_vfifo_free_count = 0;
    uint32_t rx_vfifo_data_count = 0;
    uint32_t consume_count = 0;
    uint32_t i = 0;
    uint32_t read_temp = 0;
    
	uint8_t *RecBuf;

    hal_i2s_get_tx_sample_count_ex(i2s_port, &tx_vfifo_free_count);
    hal_i2s_get_rx_sample_count_ex(i2s_port, &rx_vfifo_data_count);

    while (rx_vfifo_data_count != 0){
		consume_count = MINIMUM(tx_vfifo_free_count, rx_vfifo_data_count);
		memset(write_buffer, 0, sizeof(write_buffer));
		RecBuf = (uint8_t *)write_buffer;
		consume_count &= 0xFFFFFFFE;
		if (consume_count < 200)
			break;
		for (i = 0; i < consume_count; i++) {
            hal_i2s_rx_read_ex(i2s_port, &read_temp);
            
            hal_i2s_tx_write_ex(i2s_port, read_temp);
			read_temp &= 0xFFFF;
			//hal_i2s_tx_write(read_temp);

			RecBuf[i * 2] = read_temp & 0xFF;
			RecBuf[i * 2 + 1] = (read_temp & 0xFF00) >> 8;	

		}

		rx_vfifo_data_count -= consume_count;
        hal_i2s_get_tx_sample_count_ex(i2s_port, &tx_vfifo_free_count);

		
		ret = f_write(&fp,RecBuf,consume_count * 2,&write_lenth);
		if((ret != FR_OK)||(consume_count * 2 != write_lenth))
		{
			LOGI("write fail\n");
		}


	}
    #if 0
    while (rx_vfifo_data_count != 0) {
        consume_count = MINIMUM(tx_vfifo_free_count, rx_vfifo_data_count);
        for (i = 0; i < consume_count; i++) {
            hal_i2s_rx_read_ex(i2s_port, &read_temp);
            hal_i2s_tx_write_ex(i2s_port, read_temp);
        }
        rx_vfifo_data_count -= consume_count;
        hal_i2s_get_tx_sample_count_ex(i2s_port, &tx_vfifo_free_count);
    }
    #endif
    //hal_i2s_enable_tx_dma_interrupt();
    hal_i2s_enable_rx_dma_interrupt_ex(i2s_port);
	//LOGI("[CTRL]hisr_get_data enable irq and out\n");
}


#if 1 //example for disabling i2s function, mark as below to avoid build warning
static void i2s_close(void)
{
    LOGI("[CTRL]i2s_close \n");

    i2s_rec_event_deregister_callback(I2S_REC_QUEUE_EVENT_GET_DATA_PROCESS);

    hal_i2s_disable_tx();
    hal_i2s_disable_rx();
    hal_i2s_disable_audio_top();
    hal_i2s_deinit();
    hal_i2s_stop_tx_vfifo();
    hal_i2s_stop_rx_vfifo();
}
#endif

static void i2s_rx_callback(hal_i2s_event_t event, void *user_data)
{
    //LOGI("[CTRL]i2s_rx_callback \n");

    switch (event) {
        case HAL_I2S_EVENT_DATA_REQUEST:
			LOGI("[CTRL]i2s_rx_callback HAL_I2S_EVENT_DATA_REQUEST out\n");
            break;
        case HAL_I2S_EVENT_DATA_NOTIFICATION: {
            hal_i2s_disable_rx_dma_interrupt_ex(i2s_port);
            i2s_rec_event_send_from_isr(I2S_REC_QUEUE_EVENT_GET_DATA_PROCESS, user_data);
        }
		//LOGI("[CTRL]i2s_rx_callback HAL_I2S_EVENT_DATA_NOTIFICATION out\n");
        break;
    };
}

i2s_rec_handle_t *i2s_rec_hdl_init(i2s_rec_queue_event_id_t reg_id, i2s_rec_callback_t isr)
{
	uint8_t id_idx;

	i2s_rec_handle_t *handle;
	handle = (i2s_rec_handle_t *)pvPortMalloc(sizeof(i2s_rec_handle_t));
    memset(handle, 0, sizeof(i2s_rec_handle_t));
	
    for (id_idx = 0; id_idx < MAX_I2S_REC_FUNCTIONS; id_idx++) {
        if (handle->handler_array[id_idx].in_use_flag == 0) {
            handle->handler_array[id_idx].event_id = reg_id;
            handle->handler_array[id_idx].callback_func = isr;
            LOGI("init handler_array[%u] to event id:%u \n", id_idx, reg_id);
            handle->handler_array[id_idx].in_use_flag = 1;
            break;
        }
    }
	return handle;
}

void i2s_rec_demo(void *arg)
{
    LOGI("[CTRL]i2s_rec_demo \n");
    FATFS fs;

    //lwip_net_ready();

    FRESULT res;
    res = f_mount(&fs, "0:", 1);
    if(res == FR_OK){
        printf("mount ok\n");
    }else{
        printf("mount fail\n");
    }
    res = f_open(&fp, "SD:/1.pcm", FA_CREATE_ALWAYS | FA_WRITE);
    if(res == FR_OK){
        printf("open ok\n");
    }else{
        printf("open fail\n");
    }
    int8_t result = 0;

    result = nau8810_configure();
    if (result == -1) {
        LOGE("da7212_configure failed---\n");
    }

    result = i2s_configure();
    if (result == -1) {
        LOGE("i2s_configure failed---\n");
    }
	
	i2s_rec_handle_t *hdl = NULL;

    i2s_rec_task_create();
    //i2s_rec_event_register_callback(I2S_REC_QUEUE_EVENT_GET_DATA_PROCESS, hisr_get_data);
    hdl = i2s_rec_hdl_init(I2S_REC_QUEUE_EVENT_GET_DATA_PROCESS, hisr_get_data);
	
    hal_i2s_register_rx_vfifo_callback_ex(i2s_port, i2s_rx_callback, hdl);
    /* enable dma interrupt */
    hal_i2s_enable_rx_dma_interrupt_ex(i2s_port);
    i2s_open();

    vTaskDelay(10000);
    i2s_close();
    res = f_close(&fp);
    if(res == FR_OK){
        printf("close ok\n");
    }else{
        printf("close fail\n");
    }

    while (1) {
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
}



