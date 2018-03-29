#ifndef __HTTP_DOWNLOAD_H__
#define __HTTP_DOWNLOAD_H__


typedef enum {
	START_TO_CONNECT_HTTP_SERVER	= 0x01,
	HTTP_DOWNLOAD_DATA				= 0x02,
	WAIT_BUFFER_OK					= 0x03,
	DISCONNECT_HTTP_SERVER			= 0x04,
	RETRY_CONNECT_HTTP_SERVER		= 0x05,
	
} http_download_msg_type_t;

typedef struct {
    char *src_mod;
    http_download_msg_type_t msg_id;

} http_download_msg_t;

void http_download_task_main(void *arg);
void http_start_download(char *url);
void http_stop_download(void);
int http_download_getfile_size();
int http_is_connect(void);



#endif
