#ifndef _AWS_LIB_H_
#define _AWS_LIB_H_

/* auth type */
enum AWS_AUTH_TYPE {
	AWS_AUTH_TYPE_OPEN,
	AWS_AUTH_TYPE_SHARED,
	AWS_AUTH_TYPE_WPAPSK,
	AWS_AUTH_TYPE_WPA8021X,
	AWS_AUTH_TYPE_WPA2PSK,
	AWS_AUTH_TYPE_WPA28021X,
	AWS_AUTH_TYPE_WPAPSKWPA2PSK,
	AWS_AUTH_TYPE_MAX = AWS_AUTH_TYPE_WPAPSKWPA2PSK,
	AWS_AUTH_TYPE_INVALID = 0xff,
};

/* encry type */
enum AWS_ENC_TYPE {
	AWS_ENC_TYPE_NONE,
	AWS_ENC_TYPE_WEP,
	AWS_ENC_TYPE_TKIP,
	AWS_ENC_TYPE_AES,
	AWS_ENC_TYPE_TKIPAES,
	AWS_ENC_TYPE_MAX = AWS_ENC_TYPE_TKIPAES,
	AWS_ENC_TYPE_INVALID = 0xff,
};

/* link type */
enum AWS_LINK_TYPE {
	AWS_LINK_TYPE_NONE,
	AWS_LINK_TYPE_PRISM,
	AWS_LINK_TYPE_80211_RADIO,
	AWS_LINK_TYPE_80211_RADIO_AVS
};

//���ڴ�ӡaws������汾
const char *aws_version(void);

//��monitorģʽ��ץ���İ�����ú������д���
//������
//	buf: frame buffer
//	length: frame len
//	link_type: see enum AWS_LINK_TYPE
//	with_fcs: frame include 80211 fcs field, the tailing 4bytes
//
//˵����
//	����ǰִ����������, ���link_type��with_fcs����
//	a) iwconfig wlan0 mode monitor	#����monitorģʽ
//	b) iwconfig wlan0 channel 6	#�л����ŵ�6(��·�����ŵ�Ϊ׼)
//	c) tcpdump -i wlan0 -s0 -w file.pacp	#ץ�������ļ�
//	d) ��wireshark����omnipeek�򿪣�����ͷ��ʽ������β�Ƿ����FCS 4�ֽ�
//
//	�����İ�ͷ����Ϊ��
//	�޶���İ�ͷ��AWS_LINK_TYPE_NONE
//	radio header: hdr_len = *(unsigned short *)(buf + 2)
//	avs header: hdr_len = *(unsigned long *)(buf + 4)
//	prism header: hdr_len = 144
//
void aws_80211_frame_handler(char *buf, int length,
		enum AWS_LINK_TYPE link_type, int with_fcs);

//����һ����������, �ú�����block��ֱ�������ɹ����߳�ʱ�˳�,
//	��ʱʱ����aws_timeout_period_ms����
//������
//	model: ��Ʒmodel, ��
//	secret: ��Ʒsecret, ��
//	mac: ��Ʒmac��ַ����11:22:33:44:55:66
//	sn: ��Ʒsn���룬ͨ����NULL
void aws_start(char *model, char *secret, char *mac, char *sn);
//{�ú���������������:
//	init();
//	vendor_monitor_open();
//	aws_main_thread_func();
//	vendor_monitor_close();
//	destroy();
//}

//aws_start���غ󣬵��øú�������ȡssid��passwd����Ϣ
//aws�ɹ�ʱ��ssid & passwdһ���᷵�ط�NULL�ַ���, ��bssid��auth, encry, channel
//	�п��ܻ᷵��NULL����INVALIDֵ(ȡ�����Ƿ�����wifi�б�����������)
//awsʧ�ܳ�ʱ�󣬸ú����᷵��0, �����в���ΪNULL��INVALID VALUE
//
//auth defined by enum AWS_AUTH_TYPE
//encry defined by enum AWS_ENC_TYPE
//
//����ֵ��1--�ɹ���0--ʧ��
int aws_get_ssid_passwd(char *ssid, char *passwd, char *bssid,
	char *auth, char *encry, char *channel);

//���͹㲥֪ͨAPP�������ɹ���
//Ĭ�ϻ�㲥2min, �㲥�������յ�APPӦ�����ǰ��ֹ
void aws_notify_app(void);

//�����������ɹ���ʧ�ܣ��󣬵��øú������ͷ�������ռ�õ���Դ
void aws_destroy(void);
#endif
