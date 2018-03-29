#include <stdio.h> /* printf, sprintf */
//#include <dirent.h>
#include <stdlib.h> /* exit, atoi, malloc, free */
#include <unistd.h> /* read, write, close */
#include <string.h> /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
//#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h> /* struct hostent, gethostbyname */
#include <sys/time.h>
#include <ctype.h>
#include <time.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "ff.h"
#include "cJSON.h"
#include "aes.h"
#include "tuling_app_speech.h"
#include "lightduer_app_i2s_record.h"
#include "lightduer_app_player.h"
#include "lightduer_app_idle.h"


QueueHandle_t g_tuling_speech_queue_handle;
//static char urlname[1024] = {0};
extern char g_url[];
static int sockfd;

static TulingUser_t *tulingUser = NULL;

static char upload_head[] =
    "POST /speech/chat HTTP/1.1\r\n"
    "Host: %s\r\n"
    "Connection: keep-alive\r\n"
    "Content-Length: %d\r\n"
    "Cache-Control: no-cache\r\n"
    "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.36\r\n"
    "Content-Type: multipart/form-data; boundary=%s\r\n"
    "Accept: */*\r\n"
    "Accept-Encoding: gzip, deflate\r\n"
    "Accept-Language: en-US,en;q=0.8,zh-CN;q=0.6,zh;q=0.4,zh-TW;q=0.2,es;q=0.2\r\n"
    "\r\n";

static char upload_parameters[] =
    "Content-Disposition: form-data; name=\"parameters\"\r\n\r\n%s";

static char upload_speech[] =
    "Content-Disposition: form-data; name=\"speech\"; filename=\"speech.wav\"\r\n"
    "Content-Type: application/octet-stream\r\n\r\n";

void error(const char *msg)
{
	perror(msg);
	exit(0);
}

/*
 * 获取当前时间
 */
long getCurrentTime()
{
	struct timeval tv;
	gettimeofday(&tv,NULL);
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

/*
 * 获取随机字符串
 */
void get_rand_str(char s[], int number)
{
	char str[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
	int i,lstr;
	char ss[2] = {0};
	lstr = strlen(str);//计算字符串长度
	srand((unsigned int)time((time_t *)NULL));//使用系统时间来初始化随机数发生器
	for(i = 1; i <= number; i++) //按指定大小返回相应的字符串
	{
		sprintf(ss,"%c",str[(rand()%lstr)]);//rand()%lstr 可随机返回0-71之间的整数, str[0-71]可随机得到其中的字符
		strcat(s,ss);//将随机生成的字符串连接到指定数组后面
	}
}

struct resp_header
{
	int status_code;//HTTP/1.1 '200' OK
	char content_type[128];//Content-Type: application/gzip
	long content_length;//Content-Length: 11683079
};

static void get_resp_header(const char *response,struct resp_header *resp)
{
	/*获取响应头的信息*/
	char *pos = strstr(response, "HTTP/");
	if (pos)
		sscanf(pos, "%*s %d", &resp->status_code);//返回状态码
	pos = strstr(response, "Content-Type:");//返回内容类型
	if (pos)
		sscanf(pos, "%*s %s", resp->content_type);
	pos = strstr(response, "Content-Length:");//内容的长度(字节)
	if (pos)
		sscanf(pos, "%*s %ld", &resp->content_length);
}

/*
 * 构造请求，并发送请求
 */
void buildRequest(int socket_fd, char *file_data, int len, int asr_type, int realtime, int index, char *identify, char *host)
{
	char *boundary_header = "------AiWiFiBoundary";
	char* end = "\r\n"; 			//结尾换行
	char* twoHyphens = "--";		//两个连字符
	char s[20] = {0};
	get_rand_str(s,19);
	char *boundary = malloc(strlen(boundary_header)+strlen(s) +1);
	//char boundary[strlen(boundary_header)+strlen(s) +1];
	memset(boundary, 0, strlen(boundary_header)+strlen(s) +1);
	strcat(boundary, boundary_header);
	strcat(boundary, s);
	printf("boundary is : %s\n", boundary);
	char firstBoundary[128]= {0};
	char secondBoundary[128]= {0};
	char endBoundary[128]= {0};
	sprintf(firstBoundary, "%s%s%s", twoHyphens, boundary, end);
	sprintf(secondBoundary, "%s%s%s%s", end, twoHyphens, boundary, end);
	sprintf(endBoundary, "%s%s%s%s%s", end, twoHyphens, boundary, twoHyphens, end);
	cJSON *root = cJSON_CreateObject();
    printf("yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy\n");
    printf("tulingUser->api_key = %s\n",tulingUser->api_key);
	cJSON_AddStringToObject(root,"ak",tulingUser->api_key);    
    printf("wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww\n");
	cJSON_AddNumberToObject(root,"asr", asr_type);
	cJSON_AddNumberToObject(root,"tts", 4);    
    printf("rrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrr\n");
	cJSON_AddNumberToObject(root,"flag", 3);
    printf("eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee\n");
	if(identify)
	{
		cJSON_AddStringToObject(root,"identify",identify);
	}
	if(realtime)
	{
		cJSON_AddNumberToObject(root,"realTime", 1);
	}
	if(index)
	{
		cJSON_AddNumberToObject(root,"index", index);
	}
	uint8_t in[17];
	uint8_t out[64] = {'0'};
	uint8_t aes_key_1[17];
	uint8_t iv[17]= {0};
	memcpy(in, tulingUser->user_id, strlen(tulingUser->user_id));
	memcpy(aes_key_1, tulingUser->aes_key, strlen(tulingUser->aes_key));
	memcpy(iv, tulingUser->api_key, 16);
	AES128_CBC_encrypt_buffer(out, in, 16, aes_key_1, iv);
	uint8_t outStr[64] = {'0'};
	int i,aseLen=0;
	for(i=0; i < 16; i++)
	{
		aseLen+=snprintf(outStr+aseLen,64,"%.2x",out[i]);
	}
	cJSON_AddStringToObject(root,"uid", outStr);
	cJSON_AddStringToObject(root,"token", tulingUser->token);
	char* str_js = cJSON_Print(root);
	cJSON_Delete(root);
	printf("parameters is : %s\n", str_js);
	char *parameter_data = malloc(strlen(str_js)+ strlen(upload_parameters) + strlen(boundary) + strlen(end)*2 + strlen(twoHyphens) +1);
	sprintf(parameter_data, upload_parameters, str_js);
	strcat(parameter_data, secondBoundary);

	int content_length = len+ strlen(boundary)*2 + strlen(parameter_data) + strlen(upload_speech) + strlen(end)*3 + strlen(twoHyphens)*3;
	//printf("content length is %d \n", content_length);

	char header_data[4096] = {0};
	int ret = snprintf(header_data,4096, upload_head, host, content_length, boundary);

	//printf("Request header is : %s\n", header_data);

	//header_data,boundary,parameter_data,boundary,upload_speech,fileData,end,boundary,boundary_end
	send(socket_fd, header_data, ret,0);
	send(socket_fd, firstBoundary, strlen(firstBoundary),0);
	send(socket_fd, parameter_data, strlen(parameter_data),0);
	send(socket_fd, upload_speech, strlen(upload_speech),0);

	int w_size=0,pos=0,all_Size=0;
	while(1)
	{
		//printf("ret = %d\n",ret);
		pos =send(socket_fd,file_data+w_size,len-w_size,0);
		w_size +=pos;
		all_Size +=len;
		if( w_size== len)
		{
			w_size=0;
			break;
		}
	}

	send(socket_fd, endBoundary, strlen(endBoundary),0);

	free(boundary);
	free(parameter_data);
	free(str_js);
}

/*
 * 获取响应结果
 */
void getResponse(int socket_fd, char **text)
{
	/* receive the response */
	char response[4096];
	memset(response, 0, sizeof(response));
	int length = 0,mem_size=4096;
	/*
	while( length = recv(socket_fd, response, 1024, 0))
	{
	    if(length < 0)
	    {
	        printf("Recieve Data From Server Failed!\n");
	        break;
	    }

	    printf("RESPONSE:\n %s\n", response);

	    memset(response, 0, sizeof(response));
	}
	*/

	struct resp_header resp;
	int ret=0;
	while (1)
	{
		ret = recv(socket_fd, response+length, 1,0);
		if(ret<=0)
			break;
		//找到响应头的头部信息, 两个"\r\n"为分割点
		int flag = 0;
		int i;
		for (i = strlen(response) - 1; response[i] == '\n' || response[i] == '\r'; i--, flag++);
		if (flag == 4)
			break;
		length += ret;
		if(length>=mem_size-1)
		{
			break;
		}
	}
	get_resp_header(response,&resp);
	printf("resp.content_length = %ld status_code = %d\n",resp.content_length,resp.status_code);
	if(resp.status_code!=200||resp.content_length==0)
	{
		return;
	}
	char *code = (char *)calloc(1,resp.content_length+1);
	if(code==NULL)
	{
		return;
	}
	ret=0;
	length=0;
	while(1)
	{
		ret = recv(socket_fd, code+length, resp.content_length-length,0);
		if(ret<=0)
		{
			free(code);
			break;
		}
		length+=ret;
		//printf("result = %s len=%d\n",code,len);
		if(length==resp.content_length)
			break;
	}
	printf("response is %s\n",code);

	*text = code;
    code = NULL; // liuge 
}

/*
 * 获取socket文件描述符
 */
int get_socket_fd(char *host)
{
	int portno = 80;
	int sockfd;
	struct hostent *server;
	struct sockaddr_in serv_addr;

	/* create the socket */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) error("ERROR opening socket");

	/* lookup the ip address */
	server = gethostbyname(host);
	if (server == NULL) error("ERROR, no such host");

	/* fill in the structure */
	memset(&serv_addr,0,sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	memcpy(&serv_addr.sin_addr.s_addr,server->h_addr,server->h_length);

	/* connect the socket */
	if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
		error("ERROR connecting5");
	return sockfd;

}

/*
 * 更新token
 */
void updateTokenValue(const char *token)
{
	int size = sizeof(tulingUser->token);
	memset(tulingUser->token,0,size);
	snprintf(tulingUser->token,size,"%s",token);
}

cJSON * cJSON_HasObjectItem(const cJSON *object, const char *string)
{
	return cJSON_GetObjectItem(object, string) ? 1 : 0;
}


/*******************************************
@函数功能:	json解析服务器数据
@参数:	pMsg	服务器数据
***********************************************/
static void parseJson_string(const char * pMsg)
{
	if(NULL == pMsg)
	{
		return;
	}
	cJSON * pJson = cJSON_Parse(pMsg);
	if(NULL == pJson)
	{
		return;
	}
	cJSON *pSub = cJSON_GetObjectItem(pJson, "token");//获取token的值，用于下一次请求时上传的校验值
	if(pSub!=NULL)
	{
		printf("TokenValue = %s\n",pSub->valuestring);
		updateTokenValue(pSub->valuestring);
	}
	pSub = cJSON_GetObjectItem(pJson, "code");
	if(NULL == pSub)
	{
		goto exit;
	}
	switch(pSub->valueint)
	{
	case 40001:
	case 40002:
	case 40003:
	case 40004:
	case 40005:
	case 40006:
	case 40007:
	case 305000:
	case 302000:
	case 200000:
		goto exit;
	}
	pSub = cJSON_GetObjectItem(pJson, "asr");
	if(NULL == pSub)
	{
		goto exit;
	}
	printf("info: %s \n",pSub->valuestring);
	pSub = cJSON_GetObjectItem(pJson, "tts");
	if(NULL != pSub)
	{
		printf("tts: %s \n",pSub->valuestring);
	}

	if (cJSON_HasObjectItem(pJson, "nlp"))
	{
		cJSON *item = cJSON_GetObjectItem(pJson, "nlp");
		cJSON *subitem = cJSON_GetArrayItem(item, 0);
		printf("nlp url is   %s\n", subitem->valuestring);
		memset(g_url, 0, 1024);
        memcpy(g_url, subitem->valuestring, strlen(subitem->valuestring) + 1);
		//sprintf(g_url, "play %s", subitem->valuestring);
		//system(urlname);

	}
	if (cJSON_HasObjectItem(pJson, "func"))
	{
		cJSON *func = cJSON_GetObjectItem(pJson, "func");
		cJSON *url = cJSON_GetObjectItem(func, "url");
		printf("func url is   %s\n", url->valuestring);
		memset(g_url, 0, 1024);
		memcpy(g_url, url->valuestring, strlen(url->valuestring) + 1);
		//sprintf(g_url, "play %s", url->valuestring);
		close(sockfd);
		lightduer_app_player_play(g_url,NET_PLAY,0);
		vTaskDelay(100 / portTICK_RATE_MS);
		//system(urlname);
	}
exit:
	if (pJson)
	{
		cJSON_Delete(pJson);
		pJson = NULL;
	}
}

/*
 * 发送请求，读取响应
 */
void requestAndResponse(int socket_fd, char *file_data, int len, int asr_type, int realtime, int index, char *identify, char *host)
{
	char *text = NULL;
	buildRequest(socket_fd,file_data,len,asr_type,realtime,index,identify,host);
	long start_time = getCurrentTime();
	printf("Send finish at %ld.\r\n", start_time);
	getResponse(socket_fd, &text);
	long end_time = getCurrentTime();
	printf("Response finish at %ld.\r\n", end_time);
	printf("耗时：%ld\n", end_time-start_time);
	if(text)
	{
		parseJson_string((const char * )text);
		free(text);
	}
	else
	{
	    printf("%s\r\n",text);
		printf("req failed\n");
	}
}

int InitTuling(const char *userId,const char *aes_key,const char *api_key)
{
	tulingUser = (TulingUser_t *)calloc(1,sizeof(TulingUser_t));
	if(tulingUser==NULL)
	{
		return -1;
	}
	memcpy(tulingUser->user_id,userId,strlen(userId));
	memcpy(tulingUser->aes_key,aes_key,strlen(aes_key));
	memcpy(tulingUser->api_key,api_key,strlen(api_key));
	//memcpy(tulingUser->token,token,strlen(token));
	memset(tulingUser->token,0,64);
	return 0;
}
void DestoryTuling(void)
{
	if(tulingUser)
	{
		vPortFree(tulingUser);
		tulingUser = NULL;
	}
}

void tuling_app_speech_send_queue(EN_TULING_MASSAGE_t message)
{
	xQueueSend(g_tuling_speech_queue_handle, (void*)&message, 0);
}


void tuling_app_speech_handle_common_event(void)
{
	EN_TULING_MASSAGE_t message;
    const char *user_id  = "ai00487d2ece3545";
	const char *aes_key = "b71G08969t6Xv9s3"; // Secret
	const char *api_key = "aac4d48affe84232b349dce004f715bd"; // Apikey
	//asr_type 0:pcm_16K_16bit; 1:pcm_8K_16bit; 2:amr_8K_16bit; 3:amr_16K_16bit; 4:opus
	//int asr_type = atoi(argv[2]);
	const int asr_type = 0;
	const char *host = "smartdevice.ai.tuling123.com";
	const int size = 1024 * 10;
	int req_length = 0;
	int piece;
	char identify[20] = {0};
	int i = 0;
    char *ring_buf = NULL;   
    InitTuling(user_id,aes_key,api_key);
	while(1)
	{
		if (xQueueReceive(g_tuling_speech_queue_handle, &message, portMAX_DELAY))
		{
		    printf("g_tuling_speech_queue_handle recieve message!\n");
			switch(message)
			{
			case TULING_MASSAGE_RECORD_START:
				http_download_ring_buf_init();
				lightduer_app_rec_send_queue(I2S_REC_QUEUE_EVENT_START_RECORD,NULL);

				break;
			case TULING_MASSAGE_REQUEST_AND_RESPONSE:
				sockfd = get_socket_fd(host);
				http_download_ring_buf_get_read_pointer(&ring_buf,&req_length);
				http_download_ring_buf_update_read_pointer(req_length);
				printf("----------req_length = %d\r\n",req_length);
				if(req_length > size)
				{
					//流式识别
					piece = (req_length % size != 0) ? (req_length / size + 1) : (req_length / size);

					get_rand_str(identify,19);
                    printf("tuling_app_speech_handle_common_event enter for\n");
					for(i = 0; i < piece; i++)
					{
						requestAndResponse(sockfd,ring_buf+size*i,(i == (piece - 1)) ? (req_length - size * i) : size,asr_type,1,(i==(piece - 1))?(-i-1):(i+1),identify,host);
					}
				}
				else
				{
					//非流式识别
					requestAndResponse(sockfd,ring_buf,req_length,asr_type,0,0,NULL,host);
					printf("---------------------fei liu shi shi bie----------\r\n");
				}
				//close(sockfd);
				break;

			default:
				break;

			}
		}
	}
}

void tuling_app_speech_task(void* arg)
{
	int32_t ret=0;
	uint8_t link_status=0;
	
	g_tuling_speech_queue_handle = xQueueCreate(TULING_EVENT_QUEUE_LENGTH, sizeof(EN_TULING_MASSAGE_t));
	while(1)
	{
		ret=wifi_connection_get_link_status(&link_status);
		if(link_status==WIFI_STATUS_LINK_CONNECTED)
		{tuling_app_speech_handle_common_event();}
		vTaskDelay(50);
	}
}
