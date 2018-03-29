
#ifndef __HTTP_DOWNLOAD_RING_BUF_H__
#define __HTTP_DOWNLOAD_RING_BUF_H__
#include <stdint.h>

//#define HTT_DOWNLOAD_MAX_SIZE 1024 * 1024
#define HTT_DOWNLOAD_MAX_SIZE (400 * 1024)


typedef struct http_download_ring_buf_sturct{
    int8_t *buffer;
    int32_t buf_size;
    int32_t write_ptr;
    int32_t read_ptr;
} http_download_ring_buf_struct_t;

void http_download_ring_buf_init(void);
int32_t http_download_ring_buf_free_space(void);
int32_t http_download_ring_buf_data_count(void);
int32_t http_download_ring_buf_put_data(int8_t* buf, int32_t buf_len);
int32_t http_download_ring_buf_get_data(int8_t* buf, int32_t buf_len);
void http_download_ring_buf_get_read_pointer(uint8_t **buffer, uint32_t *length);
void http_download_ring_buf_update_read_pointer(uint32_t RequestSize);
void http_download_ring_buf_put_two_bytes(uint8_t byte, uint8_t byte_2);
void http_download_ring_buf_lseek(uint32_t pos);

#endif


