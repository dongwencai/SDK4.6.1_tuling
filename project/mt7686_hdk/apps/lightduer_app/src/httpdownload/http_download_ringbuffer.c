#include "http_download_ringbuffer.h"
#include "lightduer_app_player.h"
#include <string.h>
#include "syslog.h"

//#define LIUGE_QUEUE_DEBUG

int8_t g_http_download_ring_buff[HTT_DOWNLOAD_MAX_SIZE];

http_download_ring_buf_struct_t g_http_download_ringbuf;

void http_download_ring_buf_init(void)
{
	g_http_download_ringbuf.write_ptr = 0;
	g_http_download_ringbuf.read_ptr = 0;
	g_http_download_ringbuf.buf_size = HTT_DOWNLOAD_MAX_SIZE;
	g_http_download_ringbuf.buffer = g_http_download_ring_buff;
}

#ifndef LIUGE_QUEUE_DEBUG
int32_t http_download_ring_buf_data_count(void)
{
	int32_t count = 0;
	if (g_http_download_ringbuf.write_ptr >= g_http_download_ringbuf.read_ptr)
	{
		count = g_http_download_ringbuf.write_ptr - g_http_download_ringbuf.read_ptr;
	}
	else
	{
		count = HTT_DOWNLOAD_MAX_SIZE  - (g_http_download_ringbuf.read_ptr - g_http_download_ringbuf.write_ptr);
	}
	return (HTT_DOWNLOAD_MAX_SIZE + g_http_download_ringbuf.write_ptr - g_http_download_ringbuf.read_ptr) % HTT_DOWNLOAD_MAX_SIZE;;
}
#else // 表达不同，但是结果相同 
int32_t http_download_ring_buf_data_count(void)
{
	return (HTT_DOWNLOAD_MAX_SIZE + g_http_download_ringbuf.write_ptr - g_http_download_ringbuf.read_ptr) % HTT_DOWNLOAD_MAX_SIZE;;
}
#endif

#ifndef LIUGE_QUEUE_DEBUG
int32_t http_download_ring_buf_free_space(void)
{
	int32_t count = 0;
	if (g_http_download_ringbuf.read_ptr > g_http_download_ringbuf.write_ptr)
	{
		count = g_http_download_ringbuf.read_ptr  - g_http_download_ringbuf.write_ptr;
	}
	else if (g_http_download_ringbuf.read_ptr == g_http_download_ringbuf.write_ptr)
	{
		count = HTT_DOWNLOAD_MAX_SIZE;
	}
	else
	{
		count = HTT_DOWNLOAD_MAX_SIZE - (g_http_download_ringbuf.write_ptr - g_http_download_ringbuf.read_ptr);
	}
	return count;
}
#else // 表达不同，但是结果相同 
int32_t http_download_ring_buf_free_space(void)
{
	return (HTT_DOWNLOAD_MAX_SIZE - http_download_ring_buf_data_count());
}
#endif

int32_t http_download_ring_buf_put_data(int8_t* buf, int32_t buf_len)
{
	int16_t temp_len;
	if (buf_len > http_download_ring_buf_free_space())
	{
		LOG_I(common, "ring_buf overflow!!, free count = %d", http_download_ring_buf_free_space());
		return 0;
	}
	if (buf_len + g_http_download_ringbuf.write_ptr >= HTT_DOWNLOAD_MAX_SIZE-1)
	{
		temp_len = g_http_download_ringbuf.buf_size - g_http_download_ringbuf.write_ptr;
		memcpy(g_http_download_ringbuf.buffer + g_http_download_ringbuf.write_ptr, buf, temp_len);
		memcpy(g_http_download_ringbuf.buffer, buf + temp_len, buf_len - temp_len);
		g_http_download_ringbuf.write_ptr = buf_len - temp_len;
	}
	else
	{
		memcpy(g_http_download_ringbuf.buffer + g_http_download_ringbuf.write_ptr, buf, buf_len);
		g_http_download_ringbuf.write_ptr += buf_len;
	}
	return buf_len;
}

/**
* @brief       This function used to read data from ring buffer
* @param[in]   ring_buf: Pointer to a ring buffer
* @param[in/out]   buf: Pointer to buffer
* @param[in]   buf_len: buffer size
* @return      int32_t: the read data size
*/
extern lightduer_app_player_context_t g_player_context;
int32_t http_download_ring_buf_get_data(int8_t* buf, int32_t buf_len)
{

	int32_t consume_len;
	int32_t end_ind  = g_http_download_ringbuf.write_ptr;

	if (buf_len > http_download_ring_buf_data_count())
	{
		if (http_download_ring_buf_data_count() == 0)
		{
			//LOG_I(common, "ring_buf data is empty!!!!");
			printf("ring_buf data is empty!!!!!!!!");
            #if 0
            if(g_player_context.player_state == PLAYER_PLAYING)
            {
                lightduer_mp3_player_pause(); // liuge 0207
                g_player_context.player_state = PLAYER_PAUSE;
                lightduer_app_player_send_queue(ONLINE_PLAYER_MSG_WAIT_DATA);
            }
            #endif
			return 0;
		}
		else
		{
			buf_len = http_download_ring_buf_data_count();
			printf("buf len : %d\n", buf_len);
		}
	}
	if (g_http_download_ringbuf.read_ptr < end_ind)
	{
		if (end_ind - g_http_download_ringbuf.read_ptr > buf_len)
		{
			consume_len = buf_len;
			memcpy(buf, g_http_download_ringbuf.buffer + g_http_download_ringbuf.read_ptr, buf_len);
			g_http_download_ringbuf.read_ptr += buf_len;
		}
		else
		{
			consume_len = end_ind - g_http_download_ringbuf.read_ptr ;
			memcpy(buf, g_http_download_ringbuf.buffer + g_http_download_ringbuf.read_ptr, consume_len);
			g_http_download_ringbuf.read_ptr = end_ind;
		}
	}
	else
	{
		if (g_http_download_ringbuf.buf_size - g_http_download_ringbuf.read_ptr > buf_len)
		{
			consume_len = buf_len;
			memcpy(buf, g_http_download_ringbuf.buffer + g_http_download_ringbuf.read_ptr, buf_len);
			g_http_download_ringbuf.read_ptr += buf_len;
		}
		else
		{
			int32_t copyed_len = g_http_download_ringbuf.buf_size - g_http_download_ringbuf.read_ptr;
			memcpy(buf, g_http_download_ringbuf.buffer + g_http_download_ringbuf.read_ptr, copyed_len);
			g_http_download_ringbuf.read_ptr = 0;
			if (end_ind > buf_len - copyed_len)
			{
				consume_len = buf_len;
				memcpy(buf + copyed_len, g_http_download_ringbuf.buffer, buf_len - copyed_len);
				g_http_download_ringbuf.read_ptr += buf_len - copyed_len;
			}
			else
			{
				consume_len = copyed_len + end_ind;
				memcpy(buf + copyed_len, g_http_download_ringbuf.buffer, end_ind);
				g_http_download_ringbuf.read_ptr = end_ind;
			}
		}
	}

	return consume_len;
}

/*
HTT_DOWNLOAD_MAX_SIZE: the size should be 2^(addr_mask) byte alignment

*/
void http_download_ring_buf_put_two_bytes(uint8_t byte, uint8_t byte_2)
{
	*(g_http_download_ringbuf.buffer +  g_http_download_ringbuf.write_ptr) = byte;
	*(g_http_download_ringbuf.buffer +  g_http_download_ringbuf.write_ptr + 1) = byte_2;
	g_http_download_ringbuf.write_ptr += 2;
	if (g_http_download_ringbuf.write_ptr >= g_http_download_ringbuf.buf_size)
	{
		g_http_download_ringbuf.write_ptr = 0;
	}
}

#ifndef LIUGE_QUEUE_DEBUG
void http_download_ring_buf_get_read_pointer(uint8_t **buffer, uint32_t *length)
{
	int32_t count = 0;
	if (g_http_download_ringbuf.write_ptr >= g_http_download_ringbuf.read_ptr)
	{
		count = g_http_download_ringbuf.write_ptr - g_http_download_ringbuf.read_ptr;
	}
	else
	{
		count = g_http_download_ringbuf.buf_size - g_http_download_ringbuf.read_ptr;
	}
	*buffer = g_http_download_ringbuf.buffer + g_http_download_ringbuf.read_ptr;
	*length = count;
}
#else
void http_download_ring_buf_get_read_pointer(uint8_t **buffer, uint32_t *length)
{
	*buffer = g_http_download_ringbuf.buffer + g_http_download_ringbuf.read_ptr;
	*length = (g_http_download_ringbuf.buf_size + g_http_download_ringbuf.write_ptr - g_http_download_ringbuf.read_ptr) % g_http_download_ringbuf.buf_size;
}
#endif

//#ifndef LIUGE_QUEUE_DEBUG
#if 0
void http_download_ring_buf_update_read_pointer(uint32_t RequestSize)
{
	g_http_download_ringbuf.read_ptr += RequestSize;
	if (g_http_download_ringbuf.read_ptr >= g_http_download_ringbuf.buf_size)
	{
		g_http_download_ringbuf.read_ptr = 0;
	}
}
#else // liuge
void http_download_ring_buf_update_read_pointer(uint32_t RequestSize)
{
	g_http_download_ringbuf.read_ptr = (g_http_download_ringbuf.read_ptr + RequestSize) % g_http_download_ringbuf.buf_size;
}
#endif

void http_download_ring_buf_lseek(uint32_t pos)
{
	g_http_download_ringbuf.read_ptr = pos;
}


