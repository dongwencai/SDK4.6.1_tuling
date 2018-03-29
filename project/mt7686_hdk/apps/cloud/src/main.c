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

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "sys_init.h"
#include "wifi_nvdm_config.h"
#include "wifi_lwip_helper.h"
#if defined(MTK_MINICLI_ENABLE)
#include "cli_def.h"
#endif
#include "hal_feature_config.h"
#include "hal_wdt.h"
#include "bsp_gpio_ept_config.h"
#include "hal_sleep_manager.h"
#include "hal.h"

#if defined(MTK_JOYLINK_ENABLE)   || defined(MTK_ALINK_ENABLE) || defined(MTK_AIRKISS_ENABLE)
#include "msc_api.h"
#include "msc_internal.h"
#endif
#include "connsys_profile.h"
#include "wifi_api.h"

#include "task_def.h"

#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "lwip/sockets.h"
#include "ethernetif.h"
#include "lwip/sockets.h"
#include "netif/etharp.h"
#include "portmacro.h"
#include "airkiss.h"

extern unsigned char airkiss_random;
uint8_t lan_buf[200];
uint16_t lan_buf_len;

#define DEVICE_TYPE   "gh_822649924e9c"
#define DEVICE_ID     "gh_822649924e9c_aca0300c095a4510"
#define DEFAULT_LAN_PORT    12476

const airkiss_config_t akconf =
{
	(airkiss_memset_fn)&memset,
	(airkiss_memcpy_fn)&memcpy,
	(airkiss_memcmp_fn)&memcmp,
	0
};


/**
  * @brief      Watch dog timeout handle.
  * @param      wdt_reset_status, watch dog reset reason.
  * @return     None
  */
#ifdef HAL_WDT_DEBUG_MODE_ENABLED
void wdt_timeout_handle(hal_wdt_reset_status_t wdt_reset_status)
{
    printf("%s: stattus:%u\r\n", __FUNCTION__,(unsigned int)wdt_reset_status);
    /* assert 0 to trigger exception hanling flow */
    configASSERT(0);
}
#endif

/**
  * @brief      Initial watch dog.
  * @param      None
  * @return     None
  */
void wdt_init(void)
{
    hal_wdt_config_t wdt_config;
    /* timeout if more than HAL_WDT_TIMEOUT_VALUE seconds not to kick watch dog */
    wdt_config.seconds = HAL_WDT_TIMEOUT_VALUE;

#ifdef HAL_WDT_DEBUG_MODE_ENABLED
    wdt_config.mode = HAL_WDT_MODE_INTERRUPT;
#else
    wdt_config.mode = HAL_WDT_MODE_RESET;
#endif

    hal_wdt_init(&wdt_config);

#ifdef HAL_WDT_DEBUG_MODE_ENABLED
    hal_wdt_register_callback(wdt_timeout_handle);
#endif

    hal_wdt_enable(HAL_WDT_ENABLE_MAGIC);
}

/**
  * @brief      Feed watchdog in systick handler.
  * @param      None
  * @return     None
  */
#ifdef configUSE_TICK_HOOK
void vApplicationTickHook( void )
{
    /*Feed watchdog in systick handler, so can check ISR run too long, but can not check task level exception */
    hal_wdt_feed(HAL_WDT_FEED_MAGIC);
}
#endif

#if defined(MTK_JOYLINK_ENABLE)   || defined(MTK_ALINK_ENABLE) || defined(MTK_AIRKISS_ENABLE)
static int32_t cloud_start(cloud_platform_e mode)
{

#ifdef MTK_JOYLINK_ENABLE
    extern int joylink_demo();
    if (mode & CLOUD_SMART_PLATFORM_JOYLINK) {
        joylink_demo();
    }
#endif

#ifdef MTK_ALINK_ENABLE
    extern int alink_demo();
    if (mode & CLOUD_SMART_PLATFORM_ALINK) {
        alink_demo();
    }
#endif

#ifdef MTK_AIRKISS_ENABLE
    extern void airkiss_demo(void);
    if (mode & CLOUD_SMART_PLATFORM_AIRKISS) {
        airkiss_demo();
    }
#endif
    return 0;
}
#endif

#if defined(MTK_JOYLINK_ENABLE)   || defined(MTK_ALINK_ENABLE) || defined(MTK_AIRKISS_ENABLE)
static void _smart_connection_cb_handle(smart_connection_event_t event, void *data)
{
    switch(event) {
        case WIFI_SMART_CONNECTION_EVENT_CHANNEL_LOCKED:
            break;
        case WIFI_SMART_CONNECTION_EVENT_INFO_COLLECTED: {
            smt_connt_result_t *result = (smt_connt_result_t *)data;
            printf("******************************ssid is %s,ssid len is %d***************************\n",result->ssid,result->ssid_len);
            printf("******************************pwd is %s, pwd  len is %d***************************\n",result->pwd,result->pwd_len);
            
            wifi_config_set_ssid(WIFI_PORT_STA, result->ssid, result->ssid_len);

            if (result->pwd_len != 0) {
                wifi_config_set_wpa_psk_key(WIFI_PORT_STA, result->pwd, result->pwd_len);
                if(result->pwd_len == 10 || result->pwd_len == 26 || result->pwd_len == 5 || result->pwd_len == 13) {
                    wifi_wep_key_t wep_key;

                    if (result->pwd_len == 10 || result->pwd_len == 26) {
                        wep_key.wep_key_length[0] = result->pwd_len / 2;
                        AtoH((char *)result->pwd, (char *)&wep_key.wep_key[0], (int)wep_key.wep_key_length[0]);
                    } else if (result->pwd_len == 5 || result->pwd_len == 13) {
                        wep_key.wep_key_length[0] = result->pwd_len;
                        memcpy(wep_key.wep_key[0], result->pwd, result->pwd_len);
                    }

                    wep_key.wep_tx_key_index = 0;
                    wifi_config_set_wep_key(WIFI_PORT_STA, &wep_key);
                }

            }

            wifi_config_reload_setting();
        }
        break;
        default:
            break;
    }
}
#endif

/**
  * @brief      Create a task for cloud test
  * @param[in]  void *args:Not used
  * @return     None
  */
void cloud_test(void *args)
{

    printf("enter cloud test\n");
#if defined(MTK_JOYLINK_ENABLE)   || defined(MTK_ALINK_ENABLE) || defined(MTK_AIRKISS_ENABLE)

    cloud_platform_e mode;
    int flag;
    smnt_type_e smnt_type;
    msc_device_info_t device_info;
    msc_config_set_smnt_autostart(1);
    msc_config_set_smnt_type(2);
    msc_config_set_cloud_platform(CLOUD_SMART_PLATFORM_AIRKISS);
    if(0 != msc_config_get_smnt_autostart(&flag)) {
        //lxg add 
        printf("msc_config_get_smnt_autostart fail\n");
        
        goto fail;
    }

    if(1 == flag) {
        msc_config_set_smnt_autostart(0);

        if (0 != msc_config_get_smnt_type(&smnt_type)) {
            goto fail;
        }

        switch(smnt_type) {
            case SUPPORT_JOYLINK:
                // TODO: fill   joylink_device_info, include cid , puid,smnt_key
                memset(&device_info.joylink_device_info,0,sizeof(joylink_info_t));
                break;
            case SUPPORT_AIRKISS | SUPPORT_MTK_SMNT:
                printf("airkiss\n");
                // TODO:  fill  am_device_info , for mtk smart connection and airkiss
                memset(&device_info.am_device_info,0,sizeof(am_info_t));
                break;
            default:
                break;
        }
        if (MSC_OK != wifi_multi_smart_connection_start(device_info, smnt_type,_smart_connection_cb_handle)) {
            goto fail;
        }

    }
    if (0 != msc_config_get_cloud_platform(&mode)) {
        goto fail;
    }

    if (0 != cloud_start(mode)) {
        goto fail;
    }
fail:
#endif
    vTaskDelete(NULL);
}

int32_t wifi_init_done_handler(wifi_event_t event,
                                      uint8_t *payload,
                                      uint32_t length)
{
    LOG_I(common, "WiFi Init Done: port = %d", payload[6]);
    return 1;
}
//mt7686 notice gongzhonghao , device connect wifi
#if 0
int udp_client_test(void)
{
    int s;
    int ret;
    int rlen;
    struct sockaddr_in addr;
    int count = 0;
    printf("lwip_socket_example, udp_client_test starts");
    char buf[10] = {0};
    memset(&addr, 0, sizeof(addr));
    addr.sin_len = sizeof(addr);
    addr.sin_family = AF_INET;
    addr.sin_port = lwip_htons(10000);
    addr.sin_addr.s_addr = inet_addr("255.255.255.255");  
    //addr.sin_addr.s_addr = INADDR_ANY; 
    buf[0] = airkiss_random;
    /* Create the socket */
    s = lwip_socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) {
        printf("lwip_socket_example, UDP client create failed\n");
        goto idle;
    }
    int optval = 1;//Õâ¸öÖµÒ»¶¨ÒªÉèÖÃ£¬·ñÔò¿ÉÄÜµ¼ÖÂsendto()Ê§°Ü  
    lwip_setsockopt(s, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(int));  
    printf("lwip_socket_example, UDP client create success\n");
    /* Connect */
    ret = lwip_connect(s, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0) {
        lwip_close(s);
        printf("lwip_socket_example,UDP client connect failed\n");
        goto idle;
    }
    
    printf("lwip_socket_example,UDP client connect success\n");
    while (count < 10) {
        /* Write something */
        printf("send_buf : %d\n", airkiss_random);
        ret = lwip_write(s, buf, 1);
        printf("lwip_write\n");

        count++;
        vTaskDelay(2000);
    }

    /* Close */
    ret = lwip_close(s);
    return ret;
idle:
    return -1;
}
#endif
//search device
#if 1
int udp_client_test(void)
{
    int s;
    int ret;
    int rlen;
    struct sockaddr_in addr;
    int count = 0;

    printf("lwip_socket_example, udp_client_test starts");
    memset(&addr, 0, sizeof(addr));
    addr.sin_len = sizeof(addr);
    addr.sin_family = AF_INET;
    addr.sin_port = lwip_htons(12476);
    addr.sin_addr.s_addr = inet_addr("255.255.255.255");  
    //addr.sin_addr.s_addr = INADDR_ANY; 
    /* Create the socket */
    s = lwip_socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) {
        printf("lwip_socket_example, UDP client create failed\n");
        goto idle;
    }
    int optval = 1;//Õâ¸öÖµÒ»¶¨ÒªÉèÖÃ£¬·ñÔò¿ÉÄÜµ¼ÖÂsendto()Ê§°Ü  
    lwip_setsockopt(s, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(int));  
    printf("lwip_socket_example, UDP client create success\n");
    /* Connect */
    ret = lwip_connect(s, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0) {
        lwip_close(s);
        printf("lwip_socket_example,UDP client connect failed\n");
        goto idle;
    }
    
    printf("lwip_socket_example,UDP client connect success\n");
    while (count < 59) {
        /* Write something */
        memset(lan_buf,0,200);        
        lan_buf_len = sizeof(lan_buf);
        ret = airkiss_lan_pack(AIRKISS_LAN_SSDP_NOTIFY_CMD, DEVICE_TYPE, DEVICE_ID, 0, 0, lan_buf, &lan_buf_len, &akconf);
        if (ret != AIRKISS_LAN_PAKE_READY) {
            printf("Pack lan packet error!\n");
        }
        
        printf("lan_buf : %d\n",lan_buf);
        ret = lwip_write(s, lan_buf, lan_buf_len);
        printf("lwip_write\n");

        count++;
        vTaskDelay(2000);
    }

    /* Close */
    ret = lwip_close(s);
    return ret;
idle:
    return -1;
}
#endif

void airkiss_notify()
{
    lwip_net_ready();
    printf("lwip_net_ready\n");
    udp_client_test();
    while(1){
        vTaskDelay(1000);
    }
}

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{
    /* Do system initialization, eg: hardware, nvdm, logging and random seed. */
    system_init();

    /* bsp_ept_gpio_setting_init() under driver/board/mt76x7_hdk/ept will initialize the GPIO settings
     * generated by easy pinmux tool (ept). ept_*.c and ept*.h are the ept files and will be used by
     * bsp_ept_gpio_setting_init() for GPIO pinumux setup.
     */
    bsp_ept_gpio_setting_init();

    /* User initial the parameters for wifi initial process,  system will determin which wifi operation mode
     * will be started , and adopt which settings for the specific mode while wifi initial process is running*/
     #if 1
    wifi_cfg_t wifi_config = {0};
    if (0 != wifi_config_init(&wifi_config)) {
        LOG_E(common, "wifi config init fail");
        return -1;
    }

    wifi_config_t config = {0};
    wifi_config_ext_t config_ext = {0};

    config.opmode = wifi_config.opmode;

    //memcpy(config.sta_config.ssid, wifi_config.sta_ssid, 32);
    //config.sta_config.ssid_length = wifi_config.sta_ssid_len;

    memcpy(config.sta_config.ssid, "myhome", 32);
    config.sta_config.ssid_length = 6;
    config.sta_config.bssid_present = 0;
    //memcpy(config.sta_config.password, wifi_config.sta_wpa_psk, 64);
    //config.sta_config.password_length = wifi_config.sta_wpa_psk_len;

    memcpy(config.sta_config.password, "12345678", 64);
    config.sta_config.password_length = 8;
    config_ext.sta_wep_key_index_present = 1;
    config_ext.sta_wep_key_index = wifi_config.sta_default_key_id;
    config_ext.sta_auto_connect_present = 1;
    config_ext.sta_auto_connect = 1;

#ifdef MTK_ALINK_ENABLE
    uint8_t buff[2] = "0";
    uint32_t buf_size = sizeof(buff);
    nvdm_read_data_item(WIFI_PROFILE_BUFFER_STA,
                        "SmartConfig",
                        buff,
                        &buf_size);
    printf("SmartConfig read buff[size:%u]:%s\n", (unsigned int)buf_size, buff);
    if(buff[0] == '1') {
        config_ext.sta_auto_connect = 0;
        printf("Disable Wi-Fi STA mode auto connection\n");
    } else {
        config_ext.sta_auto_connect = 1;
        printf("Enable Wi-Fi STA mode auto connection\n");
    }
#endif

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

    /* Initialize wifi stack and register wifi init complete event handler,
     * notes:  the wifi initial process will be implemented and finished while system task scheduler is running,
     *            when it is done , the WIFI_EVENT_IOT_INIT_COMPLETE event will be triggered */
    wifi_init(&config, &config_ext);

    wifi_connection_register_event_handler(WIFI_EVENT_IOT_INIT_COMPLETE, wifi_init_done_handler);


    /* Tcpip stack and net interface initialization,  dhcp client, dhcp server process initialization*/
    lwip_network_init(config.opmode);
    lwip_net_start(config.opmode);
    #endif

#if defined(MTK_MINICLI_ENABLE)
    /* Initialize cli task to enable user input cli command from uart port.*/
    cli_def_create();
    cli_task_create();
#endif

    // Create a task for cloud test
    xTaskCreate(cloud_test, APP_TASK_NAME, APP_TASK_STACKSIZE/sizeof(portSTACK_TYPE), NULL, APP_TASK_PRIO, NULL);

    
    xTaskCreate(airkiss_notify, "airkiss_notify", 1024, NULL, 4, NULL);
    /* Call this function to indicate the system initialize done. */
    SysInitStatus_Set();


    vTaskStartScheduler();

    /* If all is well, the scheduler will now be running, and the following line
    will never be reached.  If the following line does execute, then there was
    insufficient FreeRTOS heap memory available for the idle and/or timer tasks
    to be created.  See the memory management section on the FreeRTOS web site
    for more details. */
    for ( ;; );
}

