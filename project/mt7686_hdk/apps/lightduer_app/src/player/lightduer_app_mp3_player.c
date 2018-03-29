#include "stdint.h"
#include "stdio.h"
#include "FreeRTOS.h"
#include "task.h"
#include "lightduer_app_mp3_player.h"
#include "mp3_codec.h"

static lightduer_mp3_player_context_t g_mp3_context;

static void lightduer_mp3_codec_callback(mp3_codec_media_handle_t *hdl, mp3_codec_event_t event)
{
    uint8_t *share_buf;
    uint32_t share_buf_len;
    switch (event) {
        case MP3_CODEC_MEDIA_JUMP_FILE_TO:
			lightduer_app_file_lseek((DWORD)hdl->jump_file_to_specified_position);
            break;
		case MP3_CODEC_MEDIA_BITSTREAM_END:{
			printf("MP3_CODEC_MEDIA_BITSTREAM_END\n\r");
			g_mp3_context.callbackstate(MP3_PLAY_NEXT_TRACK);
		}break;
			
        case MP3_CODEC_MEDIA_REQUEST:
        case MP3_CODEC_MEDIA_UNDERFLOW:{
            uint32_t loop_idx;
            uint32_t loop_cnt = 2;
			int32_t length_read;
			if (lightduer_app_file_over()){
				//lightduer_mp3_player_stop();
				hdl->flush(hdl, 1);
				g_mp3_context.callbackstate(MP3_PLAY_NEXT_TRACK);
				break;
			}
            for (loop_idx = 0; loop_idx < loop_cnt; loop_idx++) {
                hdl->get_write_buffer(hdl, &share_buf, &share_buf_len);
				g_mp3_context.callbackfordata(share_buf, share_buf_len, &length_read);			
                hdl->write_data_done(hdl, share_buf_len);
            }
            hdl->finish_write_data(hdl);
       	}break;
    }
}
void lightduer_mp3_player_init(void){
	if (g_mp3_context.mp3_player_hdl == NULL){
		g_mp3_context.mp3_player_hdl = mp3_codec_open(lightduer_mp3_codec_callback);
	}
	if (g_mp3_context.mp3_player_hdl != NULL){	
		mp3_codec_set_memory2();
	}

}

void lightduer_mp3_player_callback(mp3_player_getdata_callback_t callback,uint32_t file_size,mp3_player_state_callback_t player_state){
	printf("[MP3 file_size = %d\r\n", file_size);
	g_mp3_context.callbackfordata = callback;
	g_mp3_context.callbackstate = player_state;
	g_mp3_context.retmain_file_size = file_size;	
}

bool lightduer_mp3_player_play(){

	bool ret = false;
    uint8_t *share_buf;
    uint32_t share_buf_len;	
	int32_t length_read;

	if ((g_mp3_context.callbackfordata == NULL) && (g_mp3_context.callbackstate == NULL)){
		return false;
	}

	if (g_mp3_context.mp3_player_hdl != NULL){
 		
    	g_mp3_context.mp3_player_hdl->get_write_buffer(g_mp3_context.mp3_player_hdl, &share_buf, &share_buf_len);
		g_mp3_context.callbackfordata(share_buf,share_buf_len, &length_read);		
		g_mp3_context.mp3_player_hdl->write_data_done(g_mp3_context.mp3_player_hdl, share_buf_len);
		g_mp3_context.mp3_player_hdl->finish_write_data(g_mp3_context.mp3_player_hdl);			
    	g_mp3_context.mp3_player_hdl->skip_id3v2_and_reach_next_frame(g_mp3_context.mp3_player_hdl, g_mp3_context.retmain_file_size);
    	printf("[MP3 Codec Demo] play +\r\n");
   	 	g_mp3_context.mp3_player_hdl->play(g_mp3_context.mp3_player_hdl);
		g_mp3_context.callbackstate(MP3_PLAY_PLAYING);
		ret = true;
	}	

	return ret;
}




void lightduer_mp3_player_pause(){
	if (g_mp3_context.mp3_player_hdl != NULL){	
		g_mp3_context.mp3_player_hdl->pause(g_mp3_context.mp3_player_hdl);
		g_mp3_context.callbackstate(MP3_PLAY_PAUSE);
	}
}


void lightduer_mp3_player_resume(){
	if (g_mp3_context.mp3_player_hdl != NULL){	
		g_mp3_context.mp3_player_hdl->resume(g_mp3_context.mp3_player_hdl);
		g_mp3_context.callbackstate(MP3_PLAY_PLAYING);
	}
}


void lightduer_mp3_player_stop(){
	if (g_mp3_context.mp3_player_hdl != NULL){
	    g_mp3_context.mp3_player_hdl->stop(g_mp3_context.mp3_player_hdl);
	}	
}
void lightduer_mp3_player_deinit(){
	if (g_mp3_context.mp3_player_hdl != NULL){
	    g_mp3_context.mp3_player_hdl->stop(g_mp3_context.mp3_player_hdl);
	    g_mp3_context.mp3_player_hdl->close_codec(g_mp3_context.mp3_player_hdl);
		g_mp3_context.mp3_player_hdl = NULL;
		g_mp3_context.callbackfordata = NULL;
	
	}	
}



