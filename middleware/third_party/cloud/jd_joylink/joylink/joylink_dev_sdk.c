#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
//#include <sys/ioctl.h>
//#include <sys/socket.h>
//#include <netinet/in.h>
//#include <net/if.h>
//#include <arpa/inet.h>
//#include <pthread.h>
//#include <errno.h>
//#include <ifaddrs.h>
//#include <netdb.h>

#include "joylink.h"
#include "joylink_utils.h"
#include "joylink_packets.h"
#include "joylink_crypt.h"
#include "joylink_json.h"
#include "joylink_extern.h"
#include "joylink_sub_dev.h"
#include "joylink_porting_layer.h"

// mtk54101 modify: slim, RW->ZI. remove to init flow.
/*
JLDevice_t  _g_dev = {
    .server_socket = -1,
    .server_st = 0,
    .hb_lost_count = 0,
    .lan_socket = -1,
    .local_port = 80,      
    .send_p = NULL,
//#ifdef __LINUX_UB2__
    //.jlp.mac= "11:22:33:44:55:66",
    .jlp.mac= "11:22:33:44:55:77",
    .jlp.version = 1,
//#ifdef _TENDA_TEST   
    //.jlp.accesskey = "d8a1c5b6a412fa39753389c3fcb0bc54",
    //.jlp.localkey = "f3c0b095b0c69065f63447361cd8cdc9",
    //.jlp.feedid = "144973097202280280",    
    //.jlp.accesskey = "a749446b7aa2c9e6add5d81f5d9c9fed",
    //.jlp.localkey = "e86d13b77d9190fa0817c4342276a369",   
    //.jlp.feedid = "145345002415573456",
    .jlp.devtype = E_JLDEV_TYPE_NORMAL,
    //.jlp.joylink_server = "live.smart.jd.com",
    .jlp.joylink_server = "111.206.228.17",
    .jlp.server_port = 2002, 
#if 0
    .jlp.accesskey = "9b5ccce967e4a818f1fb3f94f306d851",
    .jlp.feedid = "144739743326280530",
    .jlp.devtype = E_JLDEV_TYPE_GW,
    .jlp.joylink_server = "192.168.192.116",
    .jlp.server_port = 7001, 
#endif
    //.jlp.uuid = "CGWQDA",
    //.jlp.uuid = "DHT4YJ",
    .jlp.uuid = "2GIFFN",
    .jlp.lancon = E_LAN_CTRL_ENABLE,
    .jlp.cmd_tran_type = E_CMD_TYPE_LUA_SCRIPT    
    //.jlp.cmd_tran_type = E_CMD_TYPE_JSON
//#endif
};
*/
JLDevice_t  _g_dev;
JLDevice_t  *_g_pdev = &_g_dev;

extern void joylink_proc_lan();
extern void joylink_proc_server();
extern int joylink_proc_server_st();
extern int joylink_ecc_contex_init(void);

void
joylink_dev_set_ver(short ver)
{
    _g_pdev->jlp.version = ver;
    joylink_dev_set_attr_jlp(&_g_pdev->jlp);
}

short
joylink_dev_get_ver()
{
    return _g_pdev->jlp.version;
}

void 
joylink_main_loop(void)
{
    int ret;
	struct sockaddr_in sin;
    bzero(&sin, sizeof(sin));

	struct timeval  selectTimeOut;
	static uint32_t serverTimer;
	joylink_util_timer_reset(&serverTimer);
	static int interval = 0;

	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_family = AF_INET;
	sin.sin_port = htons(_g_pdev->local_port);
	_g_pdev->lan_socket = socket(AF_INET, SOCK_DGRAM, 0);

	if (_g_pdev->lan_socket < 0){
		printf("socket() failed!\n");
		return;
	}

	int broadcastEnable = 1;
	if (setsockopt(_g_pdev->lan_socket, 
                SOL_SOCKET, SO_BROADCAST, 
                (uint8_t *)&broadcastEnable, 
                sizeof(broadcastEnable)) < 0){
		log_error("SO_BROADCAST ERR");
    }

	if(0 > bind(_g_pdev->lan_socket, (SOCKADDR*)&sin, sizeof(SOCKADDR))){
		perror("Bind lan socket error!");
    }

	while (1){          
        printf("%s : heap free size:%d\n", __FUNCTION__, xPortGetFreeHeapSize());
        
		if (joylink_util_is_time_out(serverTimer, interval)){
			joylink_util_timer_reset(&serverTimer);
            if(joylink_dev_is_net_ok()){
                interval = joylink_proc_server_st();
            }else{
                interval = 1000 * 10;
            }
		}

		fd_set  readfds;
		FD_ZERO(&readfds);
		FD_SET(_g_pdev->lan_socket, &readfds);
        
        if (_g_pdev->server_socket != -1 
               && _g_pdev->server_st > 0){
            FD_SET(_g_pdev->server_socket, &readfds);
        }

        int maxFd = (int)_g_pdev->server_socket > 
            (int)_g_pdev->lan_socket ? 
            _g_pdev->server_socket : _g_pdev->lan_socket;

        selectTimeOut.tv_usec = 0L;
        selectTimeOut.tv_sec = (long)1;

		ret = select(maxFd + 1, &readfds, NULL, NULL, &selectTimeOut);
		if (ret < 0){
			printf("Select ERR: %s!\r\n", strerror(errno));
			continue;
		}else if (ret == 0){
			continue;
		}

		if (FD_ISSET(_g_pdev->lan_socket, &readfds)){
            joylink_proc_lan();
		}else if((_g_pdev->server_socket != -1) && 
            (FD_ISSET(_g_pdev->server_socket, &readfds))){
            joylink_proc_server();
		}
	}
}

static void 
joylink_dev_init()
{
    // mtk54101 modify: slim, RW->ZI. remove to init flow.
    _g_pdev->server_socket = -1;
    _g_pdev->server_st = 0;
    _g_pdev->hb_lost_count = 0;
    _g_pdev->lan_socket = -1;
    _g_pdev->local_port = 80;      
    _g_pdev->send_p = NULL;
    memcpy(_g_pdev->jlp.mac,"11:22:33:44:55:88",strlen("11:22:33:44:55:88"));
    _g_pdev->jlp.version = 1;
    _g_pdev->jlp.devtype = E_JLDEV_TYPE_NORMAL;
    memcpy(_g_pdev->jlp.joylink_server,"111.206.228.17",strlen("111.206.228.17"));
    _g_pdev->jlp.server_port = 2002; 
    memcpy(_g_pdev->jlp.uuid,"2GIFFN",strlen("2GIFFN"));
    _g_pdev->jlp.lancon = E_LAN_CTRL_ENABLE;
    _g_pdev->jlp.cmd_tran_type = E_CMD_TYPE_LUA_SCRIPT;   

// mtk54101 modify: slim, RW->ZI. remove to init flow.
    extern WIFICtrl_t *_g_pwifi;
    _g_pwifi->online_state = 1;
    _g_pwifi->downspeed = 500;
    _g_pwifi->upspeed = 600;
    _g_pwifi->_24g.on = 0;   
    memcpy(_g_pwifi->_24g.ssid,"tesssid",strlen("tesssid"));
    memcpy(_g_pwifi->_24g.encryption,"mixed-psk",strlen("mixed-psk"));
    memcpy(_g_pwifi->_24g.key,"tessskey",strlen("tessskey"));


    joylink_dev_get_jlp_info(&_g_pdev->jlp);
    log_debug("--->feedid:%s", _g_pdev->jlp.feedid);
    log_debug("--->uuid:%s", _g_pdev->jlp.uuid);
    log_debug("--->accesskey:%s", _g_pdev->jlp.accesskey);
    log_debug("--->localkey:%s", _g_pdev->jlp.localkey);
}

int 
joylink_main_start()
{
    joylink_ecc_contex_init();
    joylink_dev_init();
	joylink_main_loop();
	return 0;
}

#ifdef _TEST_
int 
main()
{
    joylink_main_start();
	return 0;
}
#else

#endif
