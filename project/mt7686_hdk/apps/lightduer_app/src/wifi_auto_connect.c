/*
    2017-12-22 12:08:04
    update to sdk4.6.1

ref:at_command_wifi.c
AT+WIFISTA=scanmode

AT+WIFISTA=disconnect->wifi_connection_disconnect_ap()

example:
1,call wifi_auto_connect_init();
2,creat task :network_dect_task();

use wifi_save_ap() to save new wifi ap
*/
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wifi_auto_connect.h"
#include "wifi_api.h"
#include "nvdm.h"
#include "wifi_nvdm_config.h"
#include "task.h"
#include "smart_link.h"
#include "lightduer_app_key.h"
#include "lightduer_app_idle.h"
//#include "lightduer_app_idle.h"

extern lightduer_app_idle_context_t g_duer_idle_cntx;

wifi_config_t config = {0};
wifi_config_ext_t config_ext = {0};
wifi_cfg_t wifi_config = {0};
nvdm_wifi_conf_t nvdm_wifi_conf;//power down save val
static wifi_scan_list_item_t ap_list[WIFI_SCAN_MAX_COUNT];
static uint8_t scan_count=0;
QueueHandle_t wifi_task_queue=NULL;
wifi_task_even_t wifi_task_even;
uint8_t wifi_list_complete=0;
//extern lightduer_app_idle_context_t g_duer_idle_cntx;
static nvdm_status_t nvdm_save_wifi_info(wifi_nvdm_item_name_t item,wifi_ap_info_t *ap);

static nvdm_status_t nvdm_get_wifi_info(wifi_nvdm_item_name_t wifi_item,wifi_ap_info_t *ap);
static nvdm_status_t nvdm_get_wifi_cfg(void);
static nvdm_status_t nvdm_save_wifi_cfg(void);
static int32_t wifi_init_done_handler(wifi_event_t event,
                                      uint8_t *payload,
                                      uint32_t length);
int32_t wifi_scan_complete_handler(wifi_event_t event, uint8_t *payload, uint32_t length);
int32_t wifi_disconnect_handler(wifi_event_t event, uint8_t *payload, uint32_t length);
static void scan_task(void *p);
static void sort_ap_list(wifi_scan_list_item_t *list,uint8_t len);
void print_ap_list(wifi_scan_list_item_t *list,uint8_t len);
static void wifi_auto_connect(void);
static void sort_nvdm_list(wifi_ap_info_t *list,uint8_t len);


#if 0
#ifdef MTK_USER_FAST_TX_ENABLE
#include "type_def.h"

#define DemoPktLen 64
extern UINT_8 DemoPkt[];

extern uint32_t g_FastTx_Channel;
extern PUINT_8 g_PktForSend;
extern UINT_32 g_PktLen;
static void fastTx_init(uint32_t channel, PUINT_8 pPktContent, UINT_32 PktLen)
{
    g_FastTx_Channel = channel;
    g_PktForSend = pPktContent;
    g_PktLen = PktLen;
}
#endif

#endif
extern int32_t lightduer_app_wifi_connected(wifi_event_t event,uint8_t *payload,uint32_t length);
#if 0
uint8_t wifi_auto_connect_init()
{
    /* User initial the parameters for wifi initial process,  system will determin which wifi operation mode
     * will be started , and adopt which settings for the specific mode while wifi initial process is running*/
    wifi_cfg_t wifi_config = {0};
    if (0 != wifi_config_init(&wifi_config))
    {
        LOG_E(common, "wifi config init fail");
        return -1;
    }

    wifi_config_t config = {0};
    wifi_config_ext_t config_ext = {0};

    config.opmode = wifi_config.opmode;

    memcpy(config.sta_config.ssid, wifi_config.sta_ssid, 32);
    config.sta_config.ssid_length = wifi_config.sta_ssid_len;
    config.sta_config.bssid_present = 0;
    memcpy(config.sta_config.password, wifi_config.sta_wpa_psk, 64);
    config.sta_config.password_length = wifi_config.sta_wpa_psk_len;
    config_ext.sta_wep_key_index_present = 1;
    config_ext.sta_wep_key_index = wifi_config.sta_default_key_id;
    config_ext.sta_auto_connect_present = 1;
    config_ext.sta_auto_connect = 1;

    memcpy(config.ap_config.ssid, wifi_config.ap_ssid, 32);
    config.ap_config.ssid_length = wifi_config.ap_ssid_len;
    memcpy(config.ap_config.password, wifi_config.ap_wpa_psk, 64);
    config.ap_config.password_length = wifi_config.ap_wpa_psk_len;
    config.ap_config.auth_mode = (wifi_auth_mode_t)wifi_config.ap_auth_mode;
    config.ap_config.encrypt_type = (wifi_encrypt_type_t)wifi_config.ap_encryp_type;
    config.ap_config.channel = wifi_config.ap_channel;
    config.ap_config.bandwidth = wifi_config.ap_bw;
    config.ap_config.bandwidth_ext = WIFI_BANDWIDTH_EXT_40MHZ_UP;
    config_ext.ap_wep_key_index_present = 1;
    config_ext.ap_wep_key_index = wifi_config.ap_default_key_id;
    config_ext.ap_hidden_ssid_enable_present = 1;
    config_ext.ap_hidden_ssid_enable = wifi_config.ap_hide_ssid;
    config_ext.sta_power_save_mode = wifi_config.sta_power_save_mode;
    wifi_init(&config, &config_ext);
    wifi_connection_register_event_handler(WIFI_EVENT_IOT_INIT_COMPLETE, wifi_init_done_handler);
    wifi_connection_register_event_handler(WIFI_EVENT_IOT_CONNECTED, lightduer_app_wifi_connected);
    //Castro-
    /* Tcpip stack and net interface initialization,  dhcp client, dhcp server process initialization*/
    lwip_network_init(config.opmode);
    lwip_net_start(config.opmode);

    return 0;
}
#endif
#if 1
uint8_t wifi_auto_connect_init(void)
{
    nvdm_status_t ret=0;

    ret=nvdm_get_wifi_cfg();
#if 0
#ifdef MTK_USER_FAST_TX_ENABLE
    /* Customize Packet Content and Length */
    fastTx_init(11, DemoPkt, DemoPktLen);
#endif
#endif
    if (0 != wifi_config_init(&wifi_config))
    {
        LOG_E(common, "wifi config init fail");
        return -1;
    }

    config.opmode = wifi_config.opmode;

    //read wifi cfg from nvdm
    if(ret==NVDM_STATUS_ITEM_NOT_FOUND)//read powe down saved val from nvdm
    {
        //first startup
        nvdm_wifi_conf.current_wifi_ap_item=AP1;
        nvdm_wifi_conf.last_saved_ap=AP1;
        nvdm_wifi_conf.current_ap_info.nvdm_item_name=AP1;
        memcpy(nvdm_wifi_conf.current_ap_info.ssid, NVDM_DEFAULT_SSID, strlen(NVDM_DEFAULT_SSID));
        memcpy(nvdm_wifi_conf.current_ap_info.psk, NVDM_DEFAULT_PSK, strlen(NVDM_DEFAULT_PSK));
        nvdm_wifi_conf.current_ap_info.ssid_len=strlen(NVDM_DEFAULT_SSID);
        nvdm_wifi_conf.current_ap_info.psk_len=strlen(NVDM_DEFAULT_PSK);
        //save wifi info
        wifi_save_ap(NVDM_DEFAULT_SSID, strlen(NVDM_DEFAULT_SSID), NVDM_DEFAULT_PSK,strlen(NVDM_DEFAULT_PSK));
        //nvdm_write_data_item(NVDM_WIFI_GROUP_NAME, AP1, NVDM_DATA_ITEM_TYPE_RAW_DATA ,&nvdm_wifi_conf, sizeof(nvdm_wifi_conf));
        nvdm_save_wifi_cfg();

    }
    memcpy(config.sta_config.ssid,nvdm_wifi_conf.current_ap_info.ssid, 32);//default ssid


    config.sta_config.ssid_length = wifi_config.sta_ssid_len;
    config.sta_config.bssid_present = 0;

    memcpy(config.sta_config.password,nvdm_wifi_conf.current_ap_info.psk, 64);//default psk


    config.sta_config.password_length = wifi_config.sta_wpa_psk_len;
    config_ext.sta_wep_key_index_present = 1;
    config_ext.sta_wep_key_index = wifi_config.sta_default_key_id;
    config_ext.sta_auto_connect_present = 1;//?
    config_ext.sta_auto_connect = 0;
    //to be ap
    memcpy(config.ap_config.ssid, wifi_config.ap_ssid, 32);
    config.ap_config.ssid_length = wifi_config.ap_ssid_len;
    memcpy(config.ap_config.password, wifi_config.ap_wpa_psk, 64);

    config.ap_config.password_length = wifi_config.ap_wpa_psk_len;
    config.ap_config.auth_mode = (wifi_auth_mode_t)wifi_config.ap_auth_mode;
    config.ap_config.encrypt_type = (wifi_encrypt_type_t)wifi_config.ap_encryp_type;
    config.ap_config.channel = wifi_config.ap_channel;
    config.ap_config.bandwidth = wifi_config.ap_bw;
    config.ap_config.bandwidth_ext = WIFI_BANDWIDTH_EXT_40MHZ_UP;
    config_ext.ap_wep_key_index_present = 1;
    config_ext.ap_wep_key_index = wifi_config.ap_default_key_id;
    config_ext.ap_hidden_ssid_enable_present = 1;
    config_ext.ap_hidden_ssid_enable = wifi_config.ap_hide_ssid;
    config_ext.sta_power_save_mode = wifi_config.sta_power_save_mode;
    //to be ap end
    /* Initialize wifi stack and register wifi init complete event handler,
    * notes:  the wifi initial process will be implemented and finished while system task scheduler is running,
    *             when it is done , the WIFI_EVENT_IOT_INIT_COMPLETE event will be triggered */
    wifi_init(&config, &config_ext);
    wifi_connection_register_event_handler(WIFI_EVENT_IOT_INIT_COMPLETE, wifi_init_done_handler);
    wifi_connection_register_event_handler(WIFI_EVENT_IOT_CONNECTED, lightduer_app_wifi_connected);
    wifi_connection_register_event_handler(WIFI_EVENT_IOT_SCAN_COMPLETE, wifi_scan_complete_handler);
    wifi_connection_register_event_handler(WIFI_EVENT_IOT_DISCONNECTED, wifi_disconnect_handler);



    lwip_network_init(config.opmode);

    lwip_net_start(config.opmode);
    printf("wifi init done\r\n");
    return 0;

}
#endif
//wifi_manual_connect
void wifi_change_ssid_psk(uint8_t * ssid,uint8_t ssid_len,uint8_t *psk,uint8_t psk_len)
{
    wifi_config_set_ssid(WIFI_PORT_STA,ssid,ssid_len);
    wifi_config_set_wpa_psk_key(WIFI_PORT_STA,psk,psk_len);
    wifi_config_reload_setting();

}
void wifi_connect_use_item(wifi_nvdm_item_name_t item_name)
{
    wifi_ap_info_t ap;
    nvdm_get_wifi_info(item_name,&ap);
    if(ap.ssid_len!=0)
    {
        wifi_change_ssid_psk(ap.ssid,ap.ssid_len,ap.psk,ap.psk_len);
    }


}
void wifi_disconnect(void)
{
    //lwip_net_stop(config.opmode);
    wifi_connection_disconnect_ap();

}
void wifi_delet_ap()
{


}
wifi_nvdm_item_name_t wifi_save_ap(uint8_t *ssid,uint8_t ssid_len,uint8_t *psk,uint8_t psk_len) //todo
{
    //This return value is optional  ,You can know where AP is saved by returning the value
    wifi_ap_info_t ap;
    nvdm_status_t ret=0;
    //Select item name to save
    for(uint8_t i=1; i<WIFI_MAX_SAVE_NUM; i++) //find free space
    {
        printf("for i = %d\r\n",i);
        ret=nvdm_get_wifi_info(i,&ap);
        if(ret==0)
        {

            if(ap.ssid_len==ssid_len && strncmp(ap.ssid,ssid,ssid_len)==0)//判断NVDM里是否已经有此ssid
            {
                //update psk  do not update last_saved_ap
                memset(ap.psk,0,sizeof(ap.psk));
                memcpy(ap.psk,psk,psk_len);
                ap.psk_len=psk_len;
                nvdm_save_wifi_info(i,&ap);
                return i;
            }
        }
        if(NVDM_STATUS_ITEM_NOT_FOUND==ret)//if empty
        {
            memset(&ap,0,sizeof(ap));//clear the read date
            memcpy(ap.ssid,ssid,ssid_len);
            ap.ssid_len=ssid_len;
            memcpy(ap.psk,psk,psk_len);
            ap.psk_len=psk_len;
            ap.nvdm_item_name=i;//save ap item with this ssid & psk
            nvdm_save_wifi_info(i,&ap);
            nvdm_wifi_conf.last_saved_ap=i;
            nvdm_save_wifi_cfg();
            //printf("in wifi save ap,ssid= %s ,psk=%s,item=%d \r\n",ap.ssid,ap.psk,i);

            return i;
        }

    }
    //if no free space
    if(nvdm_wifi_conf.last_saved_ap<WIFI_MAX_SAVE_NUM)
    {
        nvdm_wifi_conf.last_saved_ap++;
        ap.nvdm_item_name=nvdm_wifi_conf.last_saved_ap;//save ap item with this ssid & psk

        nvdm_save_wifi_cfg();
        nvdm_save_wifi_info(nvdm_wifi_conf.last_saved_ap,&ap);
        return ap.nvdm_item_name;
    }
    else
    {
        nvdm_wifi_conf.last_saved_ap=AP1;
        ap.nvdm_item_name=nvdm_wifi_conf.last_saved_ap;//save ap item with this ssid & psk

        nvdm_save_wifi_cfg();
        nvdm_save_wifi_info(nvdm_wifi_conf.last_saved_ap,&ap);
        return ap.nvdm_item_name;
    }
    return -1;//not empty flash space
}
#if 0
void read_write_test()
{

    wifi_ap_info_t ap;

    nvdm_status_t ret=0;
    char item[4];
    sprintf(item,"%d",AP1);
    uint8_t buf[sizeof(wifi_ap_info_t)]= {0};

    memcpy(ap.ssid,"pro6",sizeof("pro6"));
    memcpy(ap.psk,"00000000",sizeof("00000000"));

    memcpy(buf,&ap,sizeof(buf));
    ret= nvdm_write_data_item(NVDM_WIFI_GROUP_NAME,item,NVDM_DATA_ITEM_TYPE_RAW_DATA,buf,sizeof(buf));
    if(ret!=0)printf("nvdm save failed\r\n");
    else printf("save succeed\r\n");

    memset(buf,0,sizeof(buf));
    uint32_t size=sizeof(buf);
    ret=nvdm_read_data_item(NVDM_WIFI_GROUP_NAME,item,buf,&size);
    if(ret==0)
    {
        memcpy(&ap,buf,sizeof(wifi_ap_info_t));
        printf("item:%d,ssid:%s,psk:%s\r\n",ap.nvdm_item_name,ap.ssid,ap.psk);
    }
    else
    {
        printf("nvdm get wifi info fialed ret=%d\r\n",ret);
    }
    return ret;

}
void ap_save_test()
{
    wifi_nvdm_item_name_t ret;
    wifi_ap_info_t ap;
    printf("-------------\r\n");
    ret=wifi_save_ap("pro6",sizeof("pro6"),"00000000",sizeof("00000000"));
    printf("ap%d \r\n",ret);

    printf("-------------\r\n");
    ret=wifi_save_ap("myhome",sizeof("myhome"),"12345678",sizeof("12345678"));
    printf("ap%d \r\n\r\n",ret);
    printf("-------------\r\n");

    ret=wifi_save_ap("MTK_IOT_AP",sizeof("MTK_IOT_AP"),"12345678",sizeof("12345678"));
    printf("ap%d \r\n\r\n",ret);
    printf("-------------\r\n");
////////////////read//////////////////

    memset(&ap,0,sizeof(ap));
    nvdm_get_wifi_info(AP1,&ap);
    printf("ap1 ssid = %s ,psk = %s \r\n",ap.ssid,ap.psk);
    printf("-------------\r\n");

    memset(&ap,0,sizeof(ap));
    nvdm_get_wifi_info(AP2,&ap);
    printf("ap2 ssid = %s ,psk = %s \r\n",ap.ssid,ap.psk);
    printf("-------------\r\n");
    memset(&ap,0,sizeof(ap));
    nvdm_get_wifi_info(AP3,&ap);
    printf("ap3 ssid = %s ,psk = %s \r\n",ap.ssid,ap.psk);
    printf("-------------\r\n");

    //////////////////update ap//////////////

    printf("-------update ap------\r\n");
    ret=wifi_save_ap("pro6",sizeof("pro6"),"77777777",sizeof("77777777"));
    printf("ap%d \r\n",ret);
    printf("-------------\r\n");
    /////////////////////read////////////////////

    memset(&ap,0,sizeof(ap));
    nvdm_get_wifi_info(AP1,&ap);
    printf("ap1 ssid = %s ,psk = %s \r\n",ap.ssid,ap.psk);
    printf("-------------\r\n");

    memset(&ap,0,sizeof(ap));
    nvdm_get_wifi_info(AP2,&ap);
    printf("ap2 ssid = %s ,psk = %s \r\n",ap.ssid,ap.psk);
    printf("-------------\r\n");
    memset(&ap,0,sizeof(ap));
    nvdm_get_wifi_info(AP3,&ap);
    printf("ap3 ssid = %s ,psk = %s \r\n",ap.ssid,ap.psk);
    printf("-------------\r\n");

}
void connect_test(void *p)
{
    wifi_auto_connect_init();
    ap_save_test();
    wifi_connect_use_item(AP1);
    vTaskDelay(10000);
    while(1)
    {
        wifi_config_set_ssid(WIFI_PORT_STA,"pro6",strlen("pro6"));
        wifi_config_set_wpa_psk_key(WIFI_PORT_STA,"00000000",strlen("00000000"));
        wifi_config_reload_setting();
        vTaskDelay(10000);
        wifi_config_set_ssid(WIFI_PORT_STA,NVDM_DEFAULT_SSID,strlen(NVDM_DEFAULT_SSID));
//  wifi_config_set_security_mode(WIFI_PORT_AP,WIFI_AUTH_MODE_WPA_PSK_WPA2_PSK,WIFI_ENCRYPT_TYPE_TKIP_AES_MIX );
        wifi_config_set_wpa_psk_key(WIFI_PORT_STA,NVDM_DEFAULT_PSK,strlen(NVDM_DEFAULT_PSK));

        //wifi_config_set_security_mode(WIFI_PORT_AP,WIFI_AUTH_MODE_WPA_PSK_WPA2_PSK,  WIFI_ENCRYPT_TYPE_TKIP_AES_MIX);


        printf("-----------------use psk :12345678\r\n");
        wifi_config_reload_setting();
        vTaskDelay(10000);
    }
}
#endif
static nvdm_status_t nvdm_save_wifi_info(wifi_nvdm_item_name_t item,wifi_ap_info_t *ap)//todo
{
    nvdm_status_t ret=0;
    char temp[4];
    sprintf(temp,"%d",item);
    printf("nvdm_save_wifi_info  temp:%s\r\n",temp);
    uint8_t buf[sizeof(wifi_ap_info_t)]= {0};
    memcpy(buf,ap,sizeof(buf));
    ret= nvdm_write_data_item(NVDM_WIFI_GROUP_NAME,temp,NVDM_DATA_ITEM_TYPE_RAW_DATA,buf,sizeof(buf));
    if(ret!=0)printf("nvdm save failed\r\n");

    return ret;
}
static nvdm_status_t nvdm_get_wifi_info(wifi_nvdm_item_name_t wifi_item,wifi_ap_info_t *ap)
{
    //return nvdm_read_data_item(NVDM_WIFI_GROUP_NAME,&wifi_item,ap,sizeof(wifi_ap_info_t));

    nvdm_status_t ret = 0;
    char temp[4];
    sprintf(temp,"%d",wifi_item);

    uint32_t size=sizeof(wifi_ap_info_t);
    uint8_t buf[sizeof(wifi_ap_info_t)];
    printf("nvdm get wifi info:item %s \r\n",temp);
    ret=nvdm_read_data_item(NVDM_WIFI_GROUP_NAME,temp,buf,&size);
    if(ret==0)
    {
        memcpy(ap,buf,sizeof(wifi_ap_info_t));
        printf("item:%d,ssid:%s,psk:%s\r\n",ap->nvdm_item_name,ap->ssid,ap->psk);
    }
    else
    {
        printf("nvdm get wifi info fialed ret=%d\r\n",ret);
    }
    return ret;
}
static nvdm_status_t nvdm_save_wifi_cfg(void)
{
    nvdm_status_t ret=0;

    uint8_t buf[sizeof(nvdm_wifi_conf)]= {0};
    memcpy(buf,&nvdm_wifi_conf,sizeof(buf));
    ret= nvdm_write_data_item(NVDM_WIFI_GROUP_NAME,NVDM_WIFI_CFG_ITEM_NAME,NVDM_DATA_ITEM_TYPE_RAW_DATA,buf,sizeof(buf));
    if(ret!=0)printf("nvdm save failed\r\n");
    return ret;
}
//uint8_t buff[sizeof(nvdm_wifi_conf)];
static nvdm_status_t nvdm_get_wifi_cfg(void)
{

    nvdm_status_t ret=0;
    uint32_t size=sizeof(nvdm_wifi_conf);
    uint8_t buf[sizeof(nvdm_wifi_conf)]= {0};
    ret=nvdm_read_data_item(NVDM_WIFI_GROUP_NAME,NVDM_WIFI_CFG_ITEM_NAME,buf,&size);
    printf("nvdm_get_wifi_cfg  ret = %d \r\n",ret);
    if(ret==NVDM_STATUS_OK)
    {
        printf("+ \r\n");
        memcpy(&nvdm_wifi_conf,buf,sizeof(buf));
        printf("- \r\n");
        printf("get wifi cfg,current_wifi_ap_item :%d \r\n",nvdm_wifi_conf.current_wifi_ap_item);
    }
    else
    {
        printf("nvdm get wifi cfg failed \r\n");
    }
    return ret;

}

static void wifi_auto_connect(void)
{
    int32_t ret=0;

    nvdm_status_t nvdm_ret=0;

#if 1
    /* you can scan and select ap here
        if(wifi_connection_scan_init(ap_list,WIFI_SCAN_MAX_COUNT)>=0)
        {

        }
        else
        {
            printf("wifi scan fail\r\n");
            return WIFI_OPERATION_FAILED;

        }
        */

#if 0 //use scan task
    if(wifi_list_complete==0)
    {

        printf("creat scan task\r\n");
        xTaskCreate(scan_task,"scan_task",1024*40,NULL,TASK_PRIORITY_NORMAL,NULL);
        if(!wifi_connection_scan_init(ap_list,WIFI_SCAN_MAX_COUNT)>=0)
        {
            printf(" scan init fail\r\n");
        }
        wifi_connection_start_scan(NULL, 0, NULL, 0, 0);
    }
    while(wifi_list_complete!=3)//waitting for list complete
    {
        static uint8_t timeout_count=0;
        timeout_count++;
        if(timeout_count==7)
        {
            timeout_count=0;
            //break;
        }
        vTaskDelay(1000);

    }
#endif

    while(wifi_list_complete<MAX_SCAN_LOOP)
    {

        printf("creat scan task\r\n");
        // xTaskCreate(scan_task,"scan_task",1024*40,NULL,TASK_PRIORITY_NORMAL,NULL);
        if(!wifi_connection_scan_init(ap_list,WIFI_SCAN_MAX_COUNT)>=0)
        {
            printf(" scan init fail\r\n");
        }
        wifi_connection_start_scan(NULL, 0, NULL, 0, 0);
        vTaskDelay(600);//wait for scan
    }
    wifi_list_complete=0;
    printf("scan list complete\r\n");
    print_ap_list(&ap_list, WIFI_SCAN_MAX_COUNT);
    wifi_ap_info_t nvdm_list[WIFI_MAX_SAVE_NUM]= {0};
    for(uint8_t i=0; i<WIFI_MAX_SAVE_NUM; i++)
    {
        nvdm_get_wifi_info(i+1,&nvdm_list[i]);
    }

    for(uint8_t i=0; i<WIFI_MAX_SAVE_NUM; i++)//copy rssi to saved ap
    {
        for(uint8_t j=0; j<WIFI_SCAN_MAX_COUNT; j++)
        {

            if(nvdm_list[i].ssid_len!=ap_list[j].ssid_length)continue;
            if(strcmp(nvdm_list[i].ssid,ap_list[j].ssid)!=0)continue;
            nvdm_list[i].rssi=ap_list[j].rssi;

        }
    }


    sort_nvdm_list(nvdm_list,WIFI_MAX_SAVE_NUM);
//   for(uint8_t i=0; i<WIFI_MAX_SAVE_NUM; i++)
//   {
//       printf("nvdm-----ssid:%-33s  len:%d    rssi:%d\r\n",nvdm_list[i].ssid,nvdm_list[i].ssid_len,nvdm_list[i].rssi);
//   }
    static uint8_t k=0;
    wifi_connect_use_item(nvdm_list[k].nvdm_item_name);//connect ap
    nvdm_wifi_conf.current_wifi_ap_item=k;


    if(k<WIFI_MAX_SAVE_NUM-1)k++;
    else k=0;
#else //old code

    wifi_ap_info_t tempap;
    uint8_t i=0;
    nvdm_status_t rett=0;

    for(i=nvdm_wifi_conf.current_wifi_ap_item+1; i<=WIFI_MAX_SAVE_NUM; i++)
    {
        printf("i = %d\r\n",i);
        if( nvdm_wifi_conf.current_wifi_ap_item==WIFI_MAX_SAVE_NUM)
        {
            i=1;
            nvdm_wifi_conf.current_wifi_ap_item=1;
        }

        rett=nvdm_get_wifi_info(i,&tempap);
        if(rett==0)
        {
            nvdm_wifi_conf.current_wifi_ap_item=i;
            printf("break i = %d\r\n",i);
            break;
        }
        else if(rett=-4)
        {
            nvdm_wifi_conf.current_wifi_ap_item=AP1;
        }
    }

    wifi_connect_use_item(nvdm_wifi_conf.current_wifi_ap_item);

#endif


    return 0;
}

int32_t wifi_disconnect_handler(wifi_event_t event, uint8_t *payload, uint32_t length)
{
    //send msg to queue
    return 1;
}
int32_t wifi_scan_complete_handler(wifi_event_t event, uint8_t *payload, uint32_t length)
{
    int handled = 0;
    int i = 0;
    int scan_ap_size=WIFI_SCAN_MAX_COUNT;

    switch (event)
    {

        case WIFI_EVENT_IOT_SCAN_COMPLETE:
            handled = 1;
#if 0
            wifi_scan_list_item_t temp;

            for(int i=0; i<WIFI_SCAN_MAX_COUNT-1; i++)
            {
                for(int j=i+1; j<WIFI_SCAN_MAX_COUNT; j++)
                {
                    if(ap_list[i].rssi>ap_list[j].rssi)
                    {
                        memcpy(&temp,&ap_list[i],sizeof(temp));
                        memcpy(&ap_list[i],&ap_list[j],sizeof(temp));
                        memcpy(&ap_list[i],&temp,sizeof(temp));
                    }
                }
            }

            for (i = 0; i < scan_ap_size; i++)
            {
                printf("%d: ssid: %-33s,  rssi:  %d,  ch:  %d\r\n",i,ap_list[i].ssid,ap_list[i].rssi,ap_list[i].channel);
            }

#endif
            //  sort_ap_list(ap_list,WIFI_SCAN_MAX_COUNT);
            //  print_ap_list(ap_list,WIFI_SCAN_MAX_COUNT);

            wifi_task_even_t ev=WIFI_SCAN_COMPLETE;
            //  xQueueSend(wifi_task_queue,&ev,0);//xQueueSendFromISR
            wifi_list_complete++;
            printf("scan complete handle wifi_list_complete = %d \r\n",wifi_list_complete);
            break;

        default:
            handled = 0;
            printf("[MTK Event Callback Sample]: Unknown event(%d)\n", event);
            break;
    }
    return handled;

}

static int32_t wifi_init_done_handler(wifi_event_t event,
                                      uint8_t *payload,
                                      uint32_t length)
{
    printf( "WiFi Init Done: port = %d\r\n", payload[6]);
    return 1;
}
void print_ap_list(wifi_scan_list_item_t *list,uint8_t len)
{
    for (uint8_t i = 0; i < len; i++)
    {
        printf("%d: ssid: %-33s,  rssi:  %d,  ch:  %d\r\n",i,list[i].ssid,list[i].rssi,list[i].channel);
    }

}
static void sort_ap_list(wifi_scan_list_item_t *list,uint8_t len)
{
    wifi_scan_list_item_t temp;

    for(int i=0; i<len-1; i++)
    {
        for(int j=i+1; j<len; j++)
        {
            if((list+i)->rssi>(list+j)->rssi)
            {
                memcpy(&temp,(list+i),sizeof(temp));
                memcpy((list+i),(list+j),sizeof(temp));
                memcpy((list+j),&temp,sizeof(temp));
            }
        }
    }
}
static void sort_nvdm_list(wifi_ap_info_t *list,uint8_t len)
{
    wifi_ap_info_t temp;

    for(int i=0; i<len-1; i++)
    {
        for(int j=i+1; j<len; j++)
        {
            printf("rssi=%d   rssi=%d    \r\n",(list+i)->rssi,(list+j)->rssi);
            if((list+i)->rssi<(list+j)->rssi && (list+i)->rssi!=0 && (list+j)->rssi!=0)
            {

                memcpy(&temp,(list+i),sizeof(temp));
                memcpy((list+i),(list+j),sizeof(temp));
                memcpy((list+j),&temp,sizeof(temp));
            }

            if((list+i)->rssi==0)
            {
                memcpy(&temp,(list+i),sizeof(temp));
                memcpy((list+i),(list+j),sizeof(temp));
                memcpy((list+j),&temp,sizeof(temp));
            }
        }
    }


}

#if 0
static uint8_t compare_ap(wifi_scan_list_item_t a,wifi_scan_list_item_t b)//same return 0,
{

    if(a.ssid_length!=b.ssid_length)return 1;
    for(uint8_t i=0; i<a.ssid_length; i++)
    {
        if(a.ssid[i]!=b.ssid[i])return 1;

    }
    //compare bssid?
    return 0;
}
//to do
static uint8_t merge_ap_list(wifi_scan_list_item_t *list1,uint8_t list1_len,wifi_scan_list_item_t *list2,uint8_t list2_len)//Merge lists 1 and 2 to list_res
{
    uint8_t same=0;
    uint8_t insert_p=0;
    uint8_t list2_p=0;
    printf("list 1\r\n;");
    print_ap_list(list1,list1_len);
    printf("list 2\r\n;");
    print_ap_list(list2,list2_len);

    for(uint8_t i=0; i<list1_len; i++)
    {
        if(((list1+i)->ssid_length)==0)
        {
            printf("fine empty p=%d\r\n",i);
            insert_p=i;
            break;
        }
    }

    for(uint8_t i=0; i<list2_len; i++)
    {
        for(uint8_t j=0; j<insert_p; j++)
        {
            if(!compare_ap(*(list1+i),*(list2+j)))
            {
                same=1;
                break;
            }
            else
            {
                same=0;
                list2_p=j;
            }
            if(same==0)
            {
                //insert to p
                printf("memcpy ssid = %-33s\r\n",(list2+list2_p)->ssid);
                memcpy(list1+insert_p,list2+list2_p,sizeof(wifi_scan_list_item_t));
                if(insert_p<list1_len)insert_p++;
            }
        }
    }
    printf("max list \r\n");
    //sort_ap_list(max_list,max_list_len);
    print_ap_list(list1,list1_len);
    return 0;

}
static void scan_task(void *p)
{
    wifi_task_even_t even=0;
    static uint8_t scan_count=0;
    static wifi_scan_list_item_t temp[WIFI_SCAN_MAX_COUNT];
    memset(temp,0,sizeof(wifi_scan_list_item_t)*WIFI_SCAN_MAX_COUNT*2);
    printf("in scan task\r\n");
//    if(temp==NULL)
//    {
//        temp=pvPortMalloc(sizeof(wifi_scan_list_item_t)*WIFI_SCAN_MAX_COUNT);
//        memset(temp,0,sizeof(wifi_scan_list_item_t)*WIFI_SCAN_MAX_COUNT);
//    }
    if(wifi_connection_scan_init(ap_list,WIFI_SCAN_MAX_COUNT)<0)
    {
        printf(" scan init fail\r\n");
    }
    wifi_connection_start_scan(NULL, 0, NULL, 0, 0);
    while(1)vTaskDelay(2000);
    while(1)
    {
        if(xQueueReceive(wifi_task_queue,&even,portMAX_DELAY))
        {
            scan_count++;
#if 1
            if(even==WIFI_SCAN_COMPLETE && scan_count<MAX_SCAN_LOOP)
            {
                if(temp!=NULL)
                {
                    //memcpy(temp,ap_list,sizeof(wifi_scan_list_item_t)*WIFI_SCAN_MAX_COUNT);
                    printf("scan list 1\n");
                    //merge_ap_list(temp,WIFI_SCAN_MAX_COUNT,ap_list,WIFI_SCAN_MAX_COUNT);
                    if(!wifi_connection_scan_init(ap_list,WIFI_SCAN_MAX_COUNT)>=0)
                    {
                        printf(" scan init fail\r\n");
                    }
                    wifi_connection_start_scan(NULL, 0, NULL, 0, 0);
                }


            }
#endif

        }

        if(scan_count==3)
        {

            //build list
            scan_count=0;
            printf("scan finish\r\n");
            wifi_list_complete=1;
            // sort_ap_list(ap_list,WIFI_SCAN_MAX_COUNT);
            print_ap_list(ap_list,WIFI_SCAN_MAX_COUNT);

            break;
        }
    }
    vTaskDelete(NULL);
}
#endif


void network_dect_task(void *p)
{

    uint8_t link_status=0;
    int32_t ret=0;
    static uint8_t saved=0;
    wifi_task_even_t even=0;
    wifi_task_queue=xQueueCreate(WIFI_TASK_QUEUE_LENGTH,sizeof(uint8_t));
    if(wifi_task_queue==NULL)printf("wifi_task_queue = null");
    wifi_auto_connect_init();
    // wifi_connect_use_item(nvdm_wifi_conf.current_wifi_ap_item);
    vTaskDelay(1000);//wait for connect

    wifi_save_ap("pro6",strlen("pro6"),"00000000",strlen("00000000"));
//   wifi_save_ap("myhome",strlen("myhome"),"12345678",strlen("12345678"));
    printf("save ap\r\n");

#if 1
    while(1)
    {
        ret=wifi_connection_get_link_status(&link_status);

        if(ret<0)
        {
            printf("get link status fail \r\n");
            continue;
        }
        if(link_status==WIFI_STATUS_LINK_DISCONNECTED) //&& g_duer_idle_cntx.system_state !=LIGHTDUER_APP_SMART_LINK_STATE)//if not network and not in air kiss
        {
            printf("auto connect ....\r\n");
            wifi_auto_connect();
            saved=0;
        }
        else //connect
        {
            if(saved==0)
            {
                nvdm_save_wifi_cfg();
                saved=1;
            }
        }

        vTaskDelay(5000);
    }
#else if
    while(1)
    {
        if (xQueueReceive(wifi_task_queue, &wifi_task_even, portMAX_DELAY))
        {
            switch(wifi_task_even)
            {
                case WIFI_SCAN_LIST_COMPLETE:
                    break;
                case WIFI_DISCONNECT:
                    //start scan
                    break;
                case WIFI_AP_NON_EXISTENT:
                    break;


            }
        }
    }
#endif
}

