#ifndef _AWS_PLATFORM_H_
#define _AWS_PLATFORM_H_
/*
 * platform porting API
 */

//һ�����ó�ʱʱ��, ���鳬ʱʱ��1-3min, APP��һ������1min��ʱ
extern int aws_timeout_period_ms;

//һ������ÿ���ŵ�ͣ��ʱ��, ����200ms-400ms
extern int aws_chn_scanning_period_ms;

//ϵͳ��boot�������ʱ��, �����ж��հ�֮��ļ��ʱ��
unsigned int vendor_get_time_ms(void);

//aws�����øú��������ŵ��л�֮���sleep
void vendor_msleep(int ms);

//ϵͳmalloc/free����
void *vendor_malloc(int size);
void vendor_free(void *ptr);

//ϵͳ��ӡ����, �ɲ�ʵ��
void vendor_printf(int log_level, const char* log_tag, const char* file,
	const char* fun, int line, const char* fmt, ...);

char *vendor_get_model(void);
char *vendor_get_secret(void);
char *vendor_get_mac(void);
char *vendor_get_sn(void);
int vendor_alink_version(void);

//aws����øú���������80211���߰�
//��ƽ̨��ͨ��ע��ص�����aws_recv_80211_frame()���հ�ʱ��
//���ú�����Ϊvendor_msleep(100)
int vendor_recv_80211_frame(void);

//����monitorģʽ, ������һЩ׼����������
//����wifi������Ĭ���ŵ�6
//����linuxƽ̨����ʼ��socket�������������׼���հ�
//����rtos��ƽ̨��ע���հ��ص�����aws_recv_80211_frame()��ϵͳ�ӿ�
void vendor_monitor_open(void);

//�˳�monitorģʽ���ص�stationģʽ, ������Դ����
void vendor_monitor_close(void);

//wifi�ŵ��л����ŵ�1-13
void vendor_channel_switch(char primary_channel,char secondary_channel,
		char bssid[6]);

//ͨ�����º������������ɹ�֪ͨ��APP, �˿ڶ�������
#define UDP_TX_PORT			(65123)
#define UDP_RX_PORT			(65126)
int vendor_broadcast_notification(char *msg, int msg_num);

#endif
