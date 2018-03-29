/*
    2017-11-22 17:55:47
    修改最大保存条目只需修改WIFI_MAX_SAVE_NUM与wifi_nvdm_item_name_t
*/
#ifndef __WIFI_AUTO_CONNECT_H__
#include "wifi_api.h"
#define __WIFI_AUTO_CONNECT_H__
#define WIFI_SSID_MAXLEN 32
#define WIFI_PSK_MAXLEN 64
#define WIFI_SCAN_MAX_COUNT 40
#define WIFI_MAX_SAVE_NUM 5
#define NVDM_WIFI_GROUP_NAME "wifi_ssid_group"
#define NVDM_WIFI_CFG_ITEM_NAME "cfg"
#define NVDM_DEFAULT_SSID "MTK_SOFT_AP"
#define NVDM_DEFAULT_PSK "12345678"
#define WIFI_TASK_QUEUE_LENGTH 20
#define MAX_SCAN_LOOP 3//The bigger the number is, the more SSID the search is

typedef enum
{
    WIFI_SCAN_LIST_COMPLETE = 1,
    WIFI_DISCONNECT = 2,
    WIFI_AP_NON_EXISTENT = 3,
    WIFI_SCAN_COMPLETE=4,
	WIFI_SMART_LINK_START=5,
} wifi_task_even_t;


typedef enum
{
    AP1=1,
    AP2=2,
    AP3=3,
    AP4=3,
    AP5=5,
    AP_MAX=WIFI_MAX_SAVE_NUM,

} wifi_nvdm_item_name_t;
typedef struct
{
    wifi_nvdm_item_name_t nvdm_item_name;//Each hotspot corresponds to the number
    uint8_t ssid[WIFI_SSID_MAXLEN];
    uint8_t ssid_len;
    uint8_t psk[WIFI_PSK_MAXLEN];
    uint8_t psk_len;
    int8_t rssi;
} wifi_ap_info_t;
typedef struct
{
    wifi_nvdm_item_name_t current_wifi_ap_item;//current connect ap
    wifi_ap_info_t current_ap_info;
    wifi_nvdm_item_name_t last_saved_ap;//use to save ap fun
} nvdm_wifi_conf_t; //All the parameters that need to be saved by power down


uint8_t wifi_auto_connect_init(void);



void wifi_disconnect( void);
wifi_nvdm_item_name_t wifi_save_ap(uint8_t *ssid,uint8_t ssid_len,uint8_t *psk,uint8_t psk_len);

void network_dect_task(void *p);
//void wifi_scan(void);


#endif

