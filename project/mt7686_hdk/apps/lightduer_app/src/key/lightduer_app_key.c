#include "stdint.h"
#include "stdio.h"
#include "FreeRTOS.h"
#include "task.h"
#include "lightduer_app_key.h"
#include "hal_adc.h"
#include "hal_gpt.h"
#include "hal_gpio.h"
#include "syslog.h"
#include "lightduer_app_idle.h"
extern void wifi_smart_link_key_action(lightduer_app_key_value_t key_value,lightduer_app_key_action_t key_action);
#ifdef MTK_CUSTOM_BAIDU_PCBA	
#define LIGHTDUER_APP_KEY_ADC_GPIO  HAL_GPIO_18
#else
#define LIGHTDUER_APP_KEY_ADC_GPIO  HAL_GPIO_17
#endif
static lightduer_app_key_context_t key_cntx;
#if 1
static const uint8_t lightduer_app_keyy_mapping[] = {
    LIGHTDUER_KEY_NONE,
    LIGHTDUER_KEY_RECORD,
    LIGHTDUER_KEY_NEXT,
    LIGHTDUER_KEY_PREV,
    LIGHTDUER_KEY_PLAY,
};

#else
static const uint8_t lightduer_app_keyy_mapping[] = {
    LIGHTDUER_KEY_NONE,
    LIGHTDUER_KEY_PLAY,
    LIGHTDUER_KEY_NEXT,
    LIGHTDUER_KEY_PREV,
    LIGHTDUER_KEY_RECORD,
};
#endif
#if 0
const adc_vale_t adc_def_value[] = { 
#if 0
	{1000, 1300},
    {1400, 1700},
	{1700, 2100},
    {2100, 2400},
#else
    {0,    500},
    {500,  900},
    {900,  1150},
    {1150, 1500},
	{1500, 1900},
    //{2100, 2400},

#endif
};
#endif
#ifdef MTK_CUSTOM_BAIDU_PCBA	
static const uint16_t adc_def_value[] = {
    0,300,600,1000,1400, 1800
};

#else

static const uint16_t adc_def_value[] = {
    0,500,900,1150,1500, 1900
};
#endif

/**
*@brief  In this function we get the corresponding voltage to the raw data from ADC.
*@param[in] adc_data: the raw data from ADC.
*@return This example returns the value of corresponding voltage to adc_data.
*@note If "adc_data" represents the raw data from ADC, the corresponding voltage is: (reference voltage/ ((2^resolution)-1)))*adc_data.
The reference voltage of MT7686 is 2.5V and resolution of MT7686 is 12bit.
*/
static uint16_t lightduer_app_key_adc_raw_to_voltage(uint16_t adc_data)
{
    /* According to the formulation described above, the corresponding voltage of the raw data "adc_data" is
    2500/(2^12-1)*adc_data, and the uint of the voltage is mV */
    uint16_t voltage = (adc_data * 2500) / 4095;
    return voltage;
}

static uint32_t lightduer_app_key_get_adc_value(void)
{
    uint32_t adc_data;
    uint32_t adc_voltage;
    hal_adc_init();
	hal_gpt_delay_ms(1);
#ifdef MTK_CUSTOM_BAIDU_PCBA	
    hal_adc_get_data_polling(HAL_ADC_CHANNEL_1, &adc_data);
#else
	hal_adc_get_data_polling(HAL_ADC_CHANNEL_0, &adc_data);
#endif
    adc_voltage = lightduer_app_key_adc_raw_to_voltage(adc_data);
 //   LOG_I(common,"adc channel: %7d, adc raw data: 0x%04x, voltage: %d\r\n", HAL_ADC_CHANNEL_0, (unsigned int)adc_data, (int)adc_voltage);
    hal_adc_deinit();
	return adc_voltage;
}
void bt_sink_app_key_action_handler(lightduer_app_key_value_t key_value, lightduer_app_key_action_t key_action)
{
//	lightduer_app_play_key_action(key_value,key_action);
}

static lightduer_app_key_action_t lightduer_app_key_get_key(uint16_t adc_voltage)
{
	uint8_t i;
	lightduer_app_key_value_t key_value = LIGHTDUER_KEY_NONE;
	lightduer_app_key_action_t key_action = LIGHTDUER_KEY_ACT_NONE;
	//if (adc_voltage  < 300){
        if (adc_voltage  <= adc_def_value[1]){
		//key released
		if (key_cntx.bKeyPressed){
			if (key_cntx.key_count < LIGHTDUER_KEY_LONG_PRESS_TIME){
				key_action = LIGHTDUER_KEY_ACT_PRESS_UP;
			}else /*if (key_cntx.key_count > LIGHTDUER_KEY_LONG_PRESS_TIME)*/{
				key_action = LIGHTDUER_KEY_ACT_LONG_PRESS_UP;
			}
			key_cntx.bKeyPressed = false;
		}
	}else{
		//key pressed
		if (!key_cntx.bKeyPressed){
			key_cntx.key_count = 0;
		}
		for (i = 0; i < sizeof(adc_def_value)/sizeof(adc_def_value[0]) - 1; i ++){
			if ((adc_voltage > adc_def_value[i]) && 	
				//(adc_voltage > adc_def_value[i].min_adc_value)){
				(adc_voltage <= adc_def_value[i + 1])){
				key_value = lightduer_app_keyy_mapping[i];
				if (!key_cntx.bKeyPressed){
					key_cntx.key_value = key_value;
				}
				break;
			}
		}
		if (key_cntx.key_value != key_value){
			key_cntx.key_count = 0;
			key_cntx.key_value = key_value;
		}else{
			key_cntx.key_count ++;
			if (key_cntx.key_count == LIGHTDUER_KEY_LONG_PRESS_TIME){
				key_action = LIGHTDUER_KEY_ACT_LONG_PRESS_DOWN;
			}else
			{
				if (key_cntx.key_count == LIGHTDUER_KEY_DEBOUNCE_TIME){
					key_action = LIGHTDUER_KEY_ACT_PRESS_DOWN;
				}else if (key_cntx.key_count > LIGHTDUER_KEY_LONG_PRESS_TIME){
					key_cntx.key_count = LIGHTDUER_KEY_LONG_PRESS_TIME + 1;
				}
			}			
		}
		key_cntx.bKeyPressed = true;
	}
	return key_action;
}

void lightduer_app_key_task(void *arg){
	uint32_t adc_value;
	lightduer_app_key_action_t key_action;
	key_cntx.bKeyPressed = false;
    hal_gpio_init(LIGHTDUER_APP_KEY_ADC_GPIO);
    hal_pinmux_set_function(LIGHTDUER_APP_KEY_ADC_GPIO, 6);
	printf("enter lightduer_app_key_task! \r\n");
	while(1){

		adc_value = lightduer_app_key_get_adc_value();
		key_action = lightduer_app_key_get_key(adc_value);
		if (key_action != LIGHTDUER_KEY_ACT_NONE){            
            printf("adc_value=%d\r\n",(int)adc_value);
            printf("key_action=%d\r\n",key_action);
            printf("key_cntx.key_value=%d\r\n",key_cntx.key_value);
			lightduer_app_common_key_action(key_cntx.key_value,key_action);
			//wifi_smart_link_key_action(key_cntx.key_value,key_action);
		}
		vTaskDelay(50/portTICK_RATE_MS);
	}
	
}
