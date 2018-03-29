#include "stdint.h"
#include "stdio.h"
#include "FreeRTOS.h"
#include "task.h"
#include "lightduer_app_led.h"

void lightduer_app_led_task(void *arg){

	while(1){
		//printf("audio download done\n");
		vTaskDelay(1000);
	}
	
}