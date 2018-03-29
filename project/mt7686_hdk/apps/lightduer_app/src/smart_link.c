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
#include "wifi_auto_connect.h"
#include "lightduer_app_idle.h"
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
#include "smart_link.h"

extern unsigned char airkiss_random;
uint8_t lan_buf[200];
uint16_t lan_buf_len;
static int airkiss_net_ready_rsp(void);

const airkiss_config_t akconf =
{
    (airkiss_memset_fn)&memset,
    (airkiss_memcpy_fn)&memcpy,
    (airkiss_memcmp_fn)&memcmp,
    0
};



#if defined(MTK_JOYLINK_ENABLE)   || defined(MTK_ALINK_ENABLE) || defined(MTK_AIRKISS_ENABLE)
static int32_t cloud_start(cloud_platform_e mode)
{

#ifdef MTK_JOYLINK_ENABLE
    extern int joylink_demo();
    if (mode & CLOUD_SMART_PLATFORM_JOYLINK)
    {
        joylink_demo();
    }
#endif

#ifdef MTK_ALINK_ENABLE
    extern int alink_demo();
    if (mode & CLOUD_SMART_PLATFORM_ALINK)
    {
        alink_demo();
    }
#endif

#ifdef MTK_AIRKISS_ENABLE
    extern void airkiss_demo(void);
    if (mode & CLOUD_SMART_PLATFORM_AIRKISS)
    {
        airkiss_demo();
    }
#endif
    return 0;
}
#endif

#if defined(MTK_JOYLINK_ENABLE)   || defined(MTK_ALINK_ENABLE) || defined(MTK_AIRKISS_ENABLE)
static void _smart_connection_cb_handle(smart_connection_event_t event, void *data)
{
    switch(event)
    {
        case WIFI_SMART_CONNECTION_EVENT_CHANNEL_LOCKED:
            break;
        case WIFI_SMART_CONNECTION_EVENT_INFO_COLLECTED:
        {
            smt_connt_result_t *result = (smt_connt_result_t *)data;
            printf("******************************ssid is %s,ssid len is %d***************************\n",result->ssid,result->ssid_len);
            printf("******************************pwd is %s, pwd  len is %d***************************\n",result->pwd,result->pwd_len);
//save ssid in nvdm
//connect this ssid
			wifi_nvdm_item_name_t ret=0;

			ret=wifi_save_ap(result->ssid,result->ssid_len,result->pwd,result->pwd_len);
			wifi_connect_use_item(ret);
#if 0
            wifi_config_set_ssid(WIFI_PORT_STA, result->ssid, result->ssid_len);

            if (result->pwd_len != 0)
            {
                wifi_config_set_wpa_psk_key(WIFI_PORT_STA, result->pwd, result->pwd_len);
                if(result->pwd_len == 10 || result->pwd_len == 26 || result->pwd_len == 5 || result->pwd_len == 13)
                {
                    wifi_wep_key_t wep_key;

                    if (result->pwd_len == 10 || result->pwd_len == 26)
                    {
                        wep_key.wep_key_length[0] = result->pwd_len / 2;
                        AtoH((char *)result->pwd, (char *)&wep_key.wep_key[0], (int)wep_key.wep_key_length[0]);
                    }
                    else if (result->pwd_len == 5 || result->pwd_len == 13)
                    {
                        wep_key.wep_key_length[0] = result->pwd_len;
                        memcpy(wep_key.wep_key[0], result->pwd, result->pwd_len);
                    }

                    wep_key.wep_tx_key_index = 0;
                    wifi_config_set_wep_key(WIFI_PORT_STA, &wep_key);
                }

            }

            wifi_config_reload_setting();
#endif
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
    uint8_t i=0;
    uint8_t link_status=0;

    cloud_platform_e mode;
    int flag;
    smnt_type_e smnt_type;
    msc_device_info_t device_info;
    msc_config_set_smnt_autostart(1);
    msc_config_set_smnt_type(SUPPORT_AIRKISS);
    msc_config_set_cloud_platform(CLOUD_SMART_PLATFORM_AIRKISS);
    if(0 != msc_config_get_smnt_autostart(&flag))
    {
        //lxg add
        printf("msc_config_get_smnt_autostart fail\n");

        goto fail;
    }

    if(1 == flag)
    {
        msc_config_set_smnt_autostart(0);

        if (0 != msc_config_get_smnt_type(&smnt_type))
        {
            goto fail;
        }

        switch(smnt_type)
        {
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
        if (MSC_OK != wifi_multi_smart_connection_start(device_info, smnt_type,_smart_connection_cb_handle))
        {
            goto fail;
        }

    }
    if (0 != msc_config_get_cloud_platform(&mode))
    {
        goto fail;
    }


    if (0 != cloud_start(mode))
    {
        goto fail;
    }

    while(i<30)
    {
        i++;
        wifi_connection_get_link_status(&link_status);
        if(link_status==WIFI_STATUS_LINK_CONNECTED)
        {
        	
            airkiss_net_ready_rsp();
            break;
        }
		printf("wait for connect...\n");
		vTaskDelay(1000);
    }
fail:
#endif
	//lightduer_app_set_system_state(LIGHTDUER_APP_IDLE_STATE);
    vTaskDelete(NULL);
}


//mt7686 notice gongzhonghao , device connect wifi
#if 1
int airkiss_net_ready_rsp(void)
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
    if (s < 0)
    {
        printf("lwip_socket_example, UDP client create failed\n");
        goto idle;
    }
    int optval = 1;//Õâ¸öÖµÒ»¶¨ÒªÉèÖÃ£¬·ñÔò¿ÉÄÜµ¼ÖÂsendto()Ê§°Ü
    lwip_setsockopt(s, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(int));
    printf("lwip_socket_example, UDP client create success\n");
    /* Connect */
    ret = lwip_connect(s, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0)
    {
        lwip_close(s);
        printf("lwip_socket_example,UDP client connect failed\n");
        goto idle;
    }

    printf("lwip_socket_example,UDP client connect success\n");
    while (count < 10)
    {
        /* Write something */
        printf("send_buf : %d\n", airkiss_random);
        ret = lwip_write(s, buf, 1);
        printf("lwip_write\n");

        count++;
        vTaskDelay(1000);
    }

    /* Close */
    ret = lwip_close(s);
	lightduer_app_set_system_state(LIGHTDUER_APP_IDLE_STATE);
    return ret;
idle:
    return -1;
}
#endif
//search device
#if 0
static int airkiss_net_ready_rsp(void)
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
    addr.sin_port = lwip_htons(10000);//12476
    addr.sin_addr.s_addr = inet_addr("255.255.255.255");
    //addr.sin_addr.s_addr = INADDR_ANY;
    /* Create the socket */
    s = lwip_socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0)
    {
        printf("lwip_socket_example, UDP client create failed\n");
        goto idle;
    }
    int optval = 1;//Õâ¸öÖµÒ»¶¨ÒªÉèÖÃ£¬·ñÔò¿ÉÄÜµ¼ÖÂsendto()Ê§°Ü
    lwip_setsockopt(s, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(int));
    printf("lwip_socket_example, UDP client create success\n");
    /* Connect */
    ret = lwip_connect(s, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0)
    {
        lwip_close(s);
        printf("lwip_socket_example,UDP client connect failed\n");
        goto idle;
    }

    printf("lwip_socket_example,UDP client connect success\n");
    while (count < 59)
    {
        /* Write something */
        memset(lan_buf,0,200);
        lan_buf_len = sizeof(lan_buf);
        ret = airkiss_lan_pack(AIRKISS_LAN_SSDP_NOTIFY_CMD, DEVICE_TYPE, DEVICE_ID, 0, 0, lan_buf, &lan_buf_len, &akconf);
        if (ret != AIRKISS_LAN_PAKE_READY)
        {
            printf("Pack lan packet error!\n");
        }

        printf("lan_buf : %d\n",lan_buf);
        ret = lwip_write(s, lan_buf, lan_buf_len);
        printf("lwip_write count %d\n",count);

        count++;
        vTaskDelay(1000);
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
    //udp_client_test();
    while(1)
    {
        vTaskDelay(1000);
    }
}

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
  
void airkiss_start(void)
{
    xTaskCreate(cloud_test, APP_TASK_NAME, APP_TASK_STACKSIZE/sizeof(portSTACK_TYPE), NULL, APP_TASK_PRIO, NULL);

//TODO  airkiss�ɹ�֮��  ����ssidȥ���ӡ����ӳɹ��󷵻����ݰ���΢�š�
    //xTaskCreate(airkiss_notify, "airkiss_notify", 1024, NULL, 4, NULL);


}

