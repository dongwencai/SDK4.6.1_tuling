#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "task.h"
#include "wifi_api.h"
#include "wifi_lwip_helper.h"
#include "httpclient.h"
#include "hal_log.h"
#include "ff.h"
#include "semphr.h"
#include "http_download.h"
#include "http_download_ringbuffer.h"
#include "semphr.h"
#include "ff.h"

//#define  DOWNLOAD_DATA      1
//#define USE_TF_CRAD_FILE_FOR_PLAY 1

#if defined(DOWNLOAD_DATA) || defined(USE_TF_CRAD_FILE_FOR_PLAY)
static FATFS fatfs;
static FIL fp;
static UINT length_write;
static FRESULT res;
#endif

#define DOWNLOAD_SIZE 	1 * 1024
QueueHandle_t 	download_event_queue_handle;
#define 		DOWNLOAD_QUEUE_LENGTH 20
static char *g_http_url;
//char *g_file_size;
static int g_file_size = 0;
static int g_download_data_length = 0;

static httpclient_t g_client = {0};
static bool g_client_connecting = false;
static bool g_client_disconnecting = false;
static uint8_t g_http_download_state = 0;
static uint8_t g_download_buf[DOWNLOAD_SIZE];
static uint8_t g_connect_server_times = 0;


int http_download_getfile_size(){
	return g_file_size;
}
void http_download_send_queue(http_download_msg_type_t msg)
{
    http_download_msg_t message;
	message.msg_id	 = msg;       
    xQueueSend(download_event_queue_handle, (void*)&message, 0);
}

void http_download_send_message(http_download_msg_type_t msg)
{
    http_download_msg_t msgs;
    BaseType_t xHigherPriorityTaskWoken;
    /* We have not woken a task at the start of the ISR*/
    xHigherPriorityTaskWoken = pdFALSE;
    msgs.msg_id    = msg;    
    while (xQueueSendFromISR(download_event_queue_handle, &msgs, &xHigherPriorityTaskWoken) != pdTRUE);

    /* Now the buffer is empty we can switch context if necessary.*/
    if (xHigherPriorityTaskWoken) {
        /*Actual macro used here is port specific.*/
        portYIELD_FROM_ISR(pdTRUE);
    }
}
void http_start_download(char *url){
	g_http_url = url;
	g_file_size = 0;
	http_download_send_queue(START_TO_CONNECT_HTTP_SERVER);
}
void http_stop_download(void){
	if (g_client_connecting){
		g_client_disconnecting = true;
	}
//	g_http_url = NULL;
//	http_download_send_queue(DISCONNECT_HTTP_SERVER);	
}
int http_is_connect(void){
	return g_client_connecting;	
}

static void http_download_handle_common_event(void){
    HTTPCLIENT_RESULT ret; 	
	http_download_msg_t	msgs;
	httpclient_data_t client_data = {0};
	int count = 0;
	while (1) {
        if (xQueueReceive(download_event_queue_handle, &msgs, portMAX_DELAY)){
			switch (msgs.msg_id) {
				case START_TO_CONNECT_HTTP_SERVER:
					if (g_client_disconnecting){						
						g_client_disconnecting = false;
						http_download_send_queue(DISCONNECT_HTTP_SERVER);
						break;
					}					
    				// Connect to server
    				if (!g_client_connecting){
	    				ret = httpclient_connect(&g_client, g_http_url);
                        printf("httpclient_connect ret = %d\r\n",ret);
						g_client_connecting = true;
                        printf("start httpclient_send_request\n");
						ret = httpclient_send_request(&g_client, g_http_url, HTTPCLIENT_GET, &client_data);
						printf("\n\rPlay Url:%s\n",g_http_url);
						if (ret == HTTPCLIENT_OK) {
							g_connect_server_times = 0;
							LOG_I(common,"connect http server success\n\r");
                            printf("HTTPCLIENT_OK!\r\n");
						    #ifdef DOWNLOAD_DATA
						    res = f_open(&fp, "SD:/2.mp3", FA_CREATE_ALWAYS | FA_WRITE);
						    if(res != FR_OK){
						        printf("open fail\n");
						    }
						    #endif	
							#ifdef USE_TF_CRAD_FILE_FOR_PLAY
						    res = f_open(&fp, "SD:/2.mp3", FA_OPEN_EXISTING | FA_READ);
						    if(res != FR_OK){
						        printf("open fail\n");
						    }							
							#endif
							http_download_send_queue(HTTP_DOWNLOAD_DATA);
						}else{
							LOG_I(common,"connect http server failure\n\r");
                            printf("no HTTPCLIENT_OK!\r\n");
							g_connect_server_times ++;
							if (g_connect_server_times > 5){
								http_download_send_queue(DISCONNECT_HTTP_SERVER);
							}else{
								http_download_send_queue(RETRY_CONNECT_HTTP_SERVER);
							}
						}
    				}						
					break;
				case HTTP_DOWNLOAD_DATA:{
					do{
						if (!g_client_connecting){
							break;
						}
						if (g_client_disconnecting){						
							g_client_disconnecting = false;
							http_download_send_queue(DISCONNECT_HTTP_SERVER);
							break;
						}						
						if (http_download_ring_buf_free_space() > DOWNLOAD_SIZE){
					        client_data.response_buf = g_download_buf;
					        client_data.response_buf_len = DOWNLOAD_SIZE;	
							ret = httpclient_recv_response(&g_client, &client_data);
							if (g_file_size == 0){
								g_file_size = client_data.response_content_len;
								g_download_data_length =  0;
                                LOG_I(common,"g_file_size : %d, client_data.response_buf_len : %d\n", g_file_size, client_data.response_buf_len);                               
							}
							if (ret == HTTPCLIENT_RETRIEVE_MORE_DATA){

								#ifdef DOWNLOAD_DATA
	                            res = f_write(&fp, client_data.response_buf, client_data.response_buf_len - 1, &length_write);
	                            if(res != FR_OK){
	                                LOG_I(common,"write fail\n");
	                            }
								#endif
								#ifdef USE_TF_CRAD_FILE_FOR_PLAY
	                            res = f_read(&fp, client_data.response_buf, client_data.response_buf_len - 1, &length_write);
	                            if(res != FR_OK){
	                                LOG_I(common,"write fail\n");
	                            }

								#endif
								http_download_ring_buf_put_data(client_data.response_buf,client_data.response_buf_len -1);	
                            	g_download_data_length += client_data.response_buf_len;
							}
							if (ret == HTTPCLIENT_OK){

								#ifdef DOWNLOAD_DATA
	                            res = f_write(&fp, client_data.response_buf, client_data.response_buf_len - 1, &length_write);
	                            if(res != FR_OK){
	                                LOG_I(common,"write fail\n");
	                            }
								#endif
								#ifdef USE_TF_CRAD_FILE_FOR_PLAY
	                            res = f_read(&fp, client_data.response_buf, client_data.response_buf_len - 1, &length_write);
	                            if(res != FR_OK){
	                                LOG_I(common,"write fail\n");
	                            }
								#endif								
								http_download_ring_buf_put_data(client_data.response_buf,client_data.response_buf_len - 1);	
                            	g_download_data_length += client_data.response_buf_len;
							}
							
						}else{
							//LOG_I(common,"ring buffer data full");   
							vTaskDelay(50/portTICK_RATE_MS);
						}
                        if(((g_download_data_length *11/10) < g_file_size) && (ret == HTTPCLIENT_OK))
                        {
                            printf("continue_ret_HTTPCLIENT_OK");
                            ret = HTTPCLIENT_RETRIEVE_MORE_DATA;
                        }
					}while (HTTPCLIENT_RETRIEVE_MORE_DATA == ret);
                    printf("httpclient_recv_response ret_t = %d\n",ret);
					if (ret == HTTPCLIENT_OK){

					}
                    #if defined(DOWNLOAD_DATA) || defined(USE_TF_CRAD_FILE_FOR_PLAY)
						res = f_close(&fp);
                       	if(res != FR_OK){
                        	printf("close fail\n");
                        }else{
                            printf("close success\n");
                        }
                    #endif
					if (g_download_data_length < 4 * 1024){
						uint8_t *buffer = pvPortMalloc(4 * 1024 - g_download_data_length);
						memset(buffer,0,4 * 1024 - g_download_data_length);
						http_download_ring_buf_put_data(buffer,4 * 1024 - g_download_data_length);	
						vPortFree(buffer);
						
					}
					LOG_I(common, "data download complete, g_download_data_length = %d, ret = %d",g_download_data_length,ret);
					http_download_send_queue(DISCONNECT_HTTP_SERVER);					
#if 0					
					if (!g_client_connecting){
						break;
					}
					if (g_client_disconnecting){						
						g_client_disconnecting = false;
						http_download_send_queue(DISCONNECT_HTTP_SERVER);
						break;
					}
	
					if (http_download_ring_buf_free_space() > DOWNLOAD_SIZE){
				        client_data.response_buf = g_download_buf;
				        client_data.response_buf_len = DOWNLOAD_SIZE;	
						//ret = httpclient_send_request(&g_client, g_http_url, HTTPCLIENT_GET, &client_data);
						if (g_file_size == 0){
                        	LOG_I(common,"start httpclient_recv_response\n");
						}
                        ret = httpclient_recv_response(&g_client, &client_data);
						if (g_file_size == 0){
							LOG_I(common," httpclient_recv_response,ret : %d\n", ret);
						}
						if(ret == HTTPCLIENT_RETRIEVE_MORE_DATA) {
							if (g_file_size == 0){
								g_file_size = client_data.response_content_len;
								g_download_data_length =  0;
                                LOG_I(common,"g_file_sizie : %d, client_data.response_buf_len : %d\n", g_file_size, client_data.response_buf_len);
                               
							}							
							//http_download_ring_buf_put_data(client_data.response_buf,client_data.response_buf_len-1);
							http_download_ring_buf_put_data(client_data.response_buf,client_data.response_buf_len);							
                            g_download_data_length += client_data.response_buf_len;
							LOG_I(common, "data download complete, g_download_data_length = %d",g_download_data_length);

							#ifdef DOWNLOAD_DATA
                            res = f_write(&fp, client_data.response_buf, client_data.response_buf_len - 1, &length_write);
                            if(res != FR_OK){
                                LOG_I(common,"write fail\n");
                            }
                            #endif
							http_download_send_queue(HTTP_DOWNLOAD_DATA);
						}else if (ret == HTTPCLIENT_OK){
                            if (g_file_size == 0){
								g_file_size = client_data.response_content_len;
								g_download_data_length =  0;
                                LOG_I(common,"g_file_size : %d\n", g_file_size);
                               
							}	
							count = ((client_data.response_buf_len)%(DOWNLOAD_SIZE-1) == 0)?(DOWNLOAD_SIZE-1):((client_data.response_buf_len)%(DOWNLOAD_SIZE-1));
                            http_download_ring_buf_put_data(client_data.response_buf,count);
							LOG_I(common, "data download complete, g_download_data_length = %d, count = %d",g_download_data_length, count);
                            g_download_data_length += count;
                            #ifdef DOWNLOAD_DATA
                            res = f_write(&fp, client_data.response_buf, count, &length_write);
                            if(res != FR_OK){
                                printf("write fail\n");
                            }
                            res = f_close(&fp);
                            if(res != FR_OK){
                                printf("close fail\n");
                            }else{
                                printf("close success\n");
                            }
                            #endif
							LOG_I(common, "data download complete!!!!" );
							http_download_send_queue(DISCONNECT_HTTP_SERVER);

						}else{
							if (g_download_data_length > g_file_size){
								http_download_send_queue(DISCONNECT_HTTP_SERVER);
							}else{
								http_download_send_queue(HTTP_DOWNLOAD_DATA);
							}
							LOG_I(common,"http download error:ret = %d, g_download_data_length = %d\n\r", ret, g_download_data_length);
						}
					}else{
                        //printf("ring buf no space\n");
						http_download_send_queue(WAIT_BUFFER_OK);
					}            
#endif           
					}break;
				case WAIT_BUFFER_OK:
                    
    					if (http_download_ring_buf_free_space() > DOWNLOAD_SIZE){
    						http_download_send_queue(HTTP_DOWNLOAD_DATA);
    					}else{
    						vTaskDelay(100/portTICK_RATE_MS);
    						http_download_send_queue(WAIT_BUFFER_OK);
    					}
                        
					break;
				case RETRY_CONNECT_HTTP_SERVER:
					if (g_client_connecting){
						httpclient_close(&g_client);
						g_client_connecting = false;
						LOG_I(common, "RETRY_CONNECT_HTTP_SERVER");
						http_download_send_queue(START_TO_CONNECT_HTTP_SERVER);
					}					
					break;
				case DISCONNECT_HTTP_SERVER:
					if (g_client_connecting){
						g_client_connecting = false;
						g_file_size = 0;
						httpclient_close(&g_client);
						LOG_I(common, "\n\rhttp client closed!!!");
					}
					break;
				default:
					break;
			}
		}

	}
}

void http_download_task_main(void *arg){

	http_download_ring_buf_init();
    

	download_event_queue_handle = xQueueCreate(DOWNLOAD_QUEUE_LENGTH, sizeof(http_download_msg_t));	   
	while(1){
		http_download_handle_common_event();
		vTaskDelay(1000/portTICK_RATE_MS);
	}
}



