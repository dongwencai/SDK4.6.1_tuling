/* --------------------------------------------------
 * @brief: 
 *
 * @version: 1.0
 *
 * @date: 10/08/2015 09:28:27 AM
 *
 * @author: yangzhongxuan , yangzhongxuan@gmail.com
 * --------------------------------------------------
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"

#include "joylink_json.h"
#include "joylink_log.h"
#include "joylink_extern.h"
#include "joylink_porting_layer.h"

int 
joylink_parse_scan(DevScan_t *scan, const char * pMsg)
{
    int ret = E_RET_ERROR;
    if(NULL == pMsg || NULL == scan){
        printf("--->:ERROR: pMsg is NULL\n");
        return ret;
    }

    cJSON * pJson = cJSON_Parse(pMsg);

    if(NULL == pJson){
        printf("--->:cJSON_Parse is error:%s\n", pMsg);
        return ret;
    }

    cJSON * pSub = cJSON_GetObjectItem(pJson, "uuid");
    if(NULL != pSub){
        strcpy(scan->uuid, pSub->valuestring);
    }
    pSub = cJSON_GetObjectItem(pJson, "status");
    if(NULL != pSub){
        scan->type = pSub->valueint;
    }
    ret = E_RET_OK;

    // mtk54101 modify: memory leak
    cJSON_Delete(pJson);

    return ret;
}

char * 
joylink_package_scan(const char *retMsg, const int retCode,
        int scan_type, JLDevice_t *dv)
{
    if(NULL == retMsg ){
        return NULL;
    }
    cJSON *root, *arrary;
    char *out  = NULL; 

    root = cJSON_CreateObject();
    if(NULL == root){
        goto RET;
    }

// mtk54101 modify: memory leak
#if 0 
    arrary = cJSON_CreateArray();
    if(NULL == arrary){
        cJSON_Delete(root);
        goto RET;
    }
#endif

    cJSON_AddNumberToObject(root, "code", retCode);
    cJSON_AddStringToObject(root, "msg", retMsg); 

    cJSON_AddStringToObject(root, "mac", dv->jlp.mac); 
    cJSON_AddStringToObject(root, "productuuid", dv->jlp.uuid); 
    cJSON_AddStringToObject(root, "feedid", dv->jlp.feedid); 
    cJSON_AddStringToObject(root, "devkey", dv->jlp.pubkeyS); 
    cJSON_AddNumberToObject(root, "lancon", dv->jlp.lancon); 
    cJSON_AddNumberToObject(root, "trantype", dv->jlp.cmd_tran_type); 
    cJSON_AddNumberToObject(root, "devtype", dv->jlp.devtype); 

    
    JLDevInfo_t *sdev = NULL;
    int count = 0;
    char *jsdev = NULL;

#ifdef JL_SUPPORT_SUB_FEATURE
    if(dv->jlp.devtype == E_JLDEV_TYPE_GW){
        sdev = joylink_dev_sub_devs_get(&count, scan_type);
        if(NULL != sdev){
            jsdev = joylink_package_subdev(sdev, count);

            cJSON *jsp_dev = cJSON_Parse(jsdev);
            if(jsp_dev){
                cJSON_AddItemToObject(root,"subdev", jsp_dev);
            }
        }

        if(sdev != NULL){
            joylink_free(sdev);
        }

        if(jsdev != NULL){
            joylink_free(jsdev);
        }
    }
#endif /* JL_SUPPORT_SUB_FEATURE */

    out=cJSON_Print(root);  
    cJSON_Delete(root); 
RET:
    return out;
}

int 
joylink_parse_lan_write_key(DevEnable_t *de, const char * pMsg)
{
    int ret = -1;
    if(NULL == pMsg || NULL == de){
        printf("--->:ERROR: pMsg is NULL\n");
        return ret;
    }
    cJSON * pJson = cJSON_Parse(pMsg);

    if(NULL == pJson){
        printf("--->:cJSON_Parse is error:%s\n", pMsg);
      return ret;
    }

    cJSON * pData = cJSON_GetObjectItem(pJson, "data");
    if(NULL != pData){
        cJSON * pSub = cJSON_GetObjectItem(pData, "feedid");
        if(NULL != pSub){
            strcpy(de->feedid, pSub->valuestring);
        }

        pSub = cJSON_GetObjectItem(pData, "localkey");
        if(NULL != pSub){
            strcpy(de->localkey, pSub->valuestring);
        }

        pSub = cJSON_GetObjectItem(pData, "accesskey");
        if(NULL != pSub){
            strcpy(de->accesskey, pSub->valuestring);
        }

        pSub = cJSON_GetObjectItem(pData, "lancon");
        if(NULL != pSub){
            de->lancon = pSub->valueint;
        }                                                                                                         
        pSub = cJSON_GetObjectItem(pData, "trantype");
        if(NULL != pSub){
            de->cmd_tran_type = pSub->valueint;
        }                                                                                                         
        pSub = cJSON_GetObjectItem(pData, "joylink_server");
        if(NULL != pSub){
            if(cJSON_GetArraySize(pSub) > 0){
                cJSON *pv;
                pv = cJSON_GetArrayItem(pSub, 0);
                if(NULL != pv){
                    strcpy(de->joylink_server, pv->valuestring);
                }
            }
        }                                                                                                        
        cJSON *pOpt = cJSON_GetObjectItem(pData, "opt");
        char *out = NULL;
        if(NULL != pOpt){
            out = cJSON_Print(pOpt);
            if(NULL != out){
                strcpy(de->opt, out);
                joylink_free(out);
            }
        }
    }

    cJSON_Delete(pJson);
    ret = 0;
    return ret;
}

int 
joylink_parse_json_ctrl(char *feedid, const char * pMsg)
{
    int ret = E_RET_ERROR;
    if(NULL == pMsg || NULL == feedid){
        printf("--->:ERROR: pMsg is NULL\n");
        return ret;
    }

    cJSON * pJson = cJSON_Parse(pMsg);

    if(NULL == pJson){
        printf("--->:cJSON_Parse is error:%s\n", pMsg);
      return ret;
    }

    cJSON * pSub = cJSON_GetObjectItem(pJson, "feedid");
    if(NULL != pSub){
        strcpy(feedid, pSub->valuestring);
    }
    cJSON_Delete(pJson);
    ret = E_RET_OK;

    return ret;
}

int 
joylink_parse_server_ota_order_req(JLOtaOrder_t* otaOrder, const char * pMsg)
{
    int ret = -1;
    if(NULL == pMsg || NULL == otaOrder){
        log_error("--->:ERROR: pMsg is NULL\n");
        return ret;
    }
    cJSON * pJson = cJSON_Parse(pMsg);

    if(NULL == pJson){
        log_error("--->:cJSON_Parse is error:%s\n", pMsg);
      return ret;
    }

    cJSON * pData = cJSON_GetObjectItem(pJson, "serial");
    if(NULL != pData){
        otaOrder->serial = pData->valueint;
    }

    pData = cJSON_GetObjectItem(pJson, "data");
    if(NULL != pData){
        cJSON * pSub = cJSON_GetObjectItem(pData, "feedid");
        if(NULL != pSub){
            strcpy(otaOrder->feedid, pSub->valuestring);
        }

        pSub = cJSON_GetObjectItem(pData, "productuuid");
        if(NULL != pSub){
            strcpy(otaOrder->productuuid, pSub->valuestring);
        }

        pSub = cJSON_GetObjectItem(pData, "version");
        if(NULL != pSub){
            otaOrder->version = pSub->valueint;
        }                                                                                                         
        
        pSub = cJSON_GetObjectItem(pData, "versionname");
        if(NULL != pSub){
            strcpy(otaOrder->versionname, pSub->valuestring);
        }

        pSub = cJSON_GetObjectItem(pData, "crc32");
        if(NULL != pSub){
            otaOrder->crc32 = pSub->valueint;
        }

        pSub = cJSON_GetObjectItem(pData, "url");
        if(NULL != pSub){
            strcpy(otaOrder->url, pSub->valuestring);
        }
    }

    cJSON_Delete(pJson);
    ret = 0;
    return ret;
}

int
joylink_parse_server_ota_upload_req(JLOtaUploadRsp_t* otaUpload, const char* pMsg)
{
    int ret = -1;
    if(NULL == pMsg || NULL == otaUpload){
        log_error("--->:ERROR: pMsg is NULL\n");
        return ret;
    }

    cJSON * pJson = cJSON_Parse(pMsg);

    if(NULL == pJson){
        log_error("--->:cJSON_Parse is error:%s\n", pMsg);
      return ret;
    }

    cJSON * pSub = cJSON_GetObjectItem(pJson, "code");
    if(NULL != pSub){
        otaUpload->code = pSub->valueint;
    }

    pSub = cJSON_GetObjectItem(pJson, "msg");
    if(NULL != pSub){
        strcpy(otaUpload->msg, pSub->valuestring);
    }

    cJSON_Delete(pJson);
    ret = 0;
    return ret;
}

char *
joylink_package_ota_upload(JLOtaUpload_t *otaUpload)
{
    if(NULL == otaUpload){
        return NULL;
    }
    cJSON *root, *data;
    char *out = NULL;
    root = cJSON_CreateObject();
    if(NULL == root){
        goto RET;
    }
    data = cJSON_CreateObject();
    if(NULL == data){
        cJSON_Delete(root);
        goto RET;
    }

    cJSON_AddStringToObject(root, "cmd", "otastat");

    cJSON_AddStringToObject(data, "feedid", otaUpload->feedid);
    cJSON_AddStringToObject(data, "productuuid", otaUpload->productuuid);
    cJSON_AddNumberToObject(data, "status", otaUpload->status);
    cJSON_AddStringToObject(data, "status_desc", otaUpload->status_desc);
    cJSON_AddNumberToObject(data, "progress", otaUpload->progress);

    cJSON_AddItemToObject(root,"data", data);

    out=cJSON_Print(root);
    cJSON_Delete(root);

RET:
    return out;
}
