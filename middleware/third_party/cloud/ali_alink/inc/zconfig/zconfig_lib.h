#ifndef __ZCONFIG_LIB_H
#define __ZCONFIG_LIB_H

#ifndef u8
#define u8              unsigned char
#define u16             unsigned short
#define u32             unsigned int
#endif

#ifndef ETH_ALEN
#define ETH_ALEN        6
#endif

#define ZC_MAX_SSID_LEN         (32 + 1)/* ssid: 32 octets at most, include the NULL-terminated */
#define ZC_MAX_PASSWD_LEN       (63 + 1)/* 8-63 ascii */

//used by tpsk
#define TPSK_LEN            (44)
#define ALINK_IE_HDR        (7)

enum _LOGLEVEL_ {
    LOGLEVEL_NONE,
    LOGLEVEL_ERROR,
    LOGLEVEL_WARN,
    LOGLEVEL_INFO,
    LOGLEVEL_DEBUG
};

enum ZC_AUTH_TYPE_ {
    ZC_AUTH_TYPE_OPEN,
    ZC_AUTH_TYPE_SHARED,
    ZC_AUTH_TYPE_WPAPSK,
    ZC_AUTH_TYPE_WPA8021X,
    ZC_AUTH_TYPE_WPA2PSK,
    ZC_AUTH_TYPE_WPA28021X,
    ZC_AUTH_TYPE_WPAPSKWPA2PSK,
    ZC_AUTH_TYPE_MAX = ZC_AUTH_TYPE_WPAPSKWPA2PSK,
    ZC_AUTH_TYPE_INVALID = 0xff,
};

enum _ENC_TYPE_ {
    ZC_ENC_TYPE_NONE,
    ZC_ENC_TYPE_WEP,
    ZC_ENC_TYPE_TKIP,
    ZC_ENC_TYPE_AES,
    ZC_ENC_TYPE_TKIPAES,
    ZC_ENC_TYPE_MAX = ZC_ENC_TYPE_TKIPAES,
    ZC_ENC_TYPE_INVALID = 0xff,
};

struct zconfig_constructor {
    // get system time, in ms.
    unsigned int (*zc_get_time)(void);

    //malloc & free
    void *(*zc_malloc)(int size);
    void (*zc_free)(void *ptr);

    //tpsk, released by alibaba
    /* exmaple
        char *vendor_get_tpsk(void)
        {
            return "16UQNY5bOiKG4qtjbCMnTeQXziJ9E6yq9zLMZzbnnrY=";	//test tpsk
        }
    */
    char *(*zc_get_tpsk)(void);

    /* log func, log level see enum _LOGLEVEL_, see the following exmaple
        include <stdarg.h>
        void vendor_printf(int log_level, const char* log_tag,
    const char* file, const char* fun, int line, const char* fmt, ...)
    {
        char msg[256];
        va_list args;
        
        if (log_level > LOGLEVEL_INFO)
        return;
     
        va_start(args, fmt);
        vsnprintf(msg, sizeof(msg), fmt, args);
        va_end(args);

        printf("%s", msg);
    }
    */
    void (*zc_printf)(int log_level, const char* log_tag, const char* file,
            const char* fun, int line, const char* fmt, ...);

    /*
    �¼�֪ͨ����Ҫ���������¼�
    a) �����ŵ��¼�
    b) ���յ�ssid��passwd�¼�

    �¼��ɻ���callbackʵ�ֻ�ͨ��ͨ��zconfig_recv_callback()�ķ���ֵ���ж��¼�
    */
    void (*zc_cb_channel_locked)(u8 channel);
    /*
    callback, ���ڷ����������
    ����ssid, passwdһ���᷵��, bssid, auth, encry����ΪNULL, ZC_AUTH_TYPE_INVALID, ZC_ENC_TYPE_INVALID
    */
    void (*zc_cb_got_ssid_passwd)(u8 *ssid, u8 *passwd, u8 *bssid, u8 auth, u8 encry, u8 channel);
};

enum _PKG_TYPE_ {
    PKG_INVALID,        //invalid pkg, --��Ч��
    PKG_BC_FRAME,       //broadcast frame, --�ŵ�ɨ��׶Σ��յ��յ��÷���ֵ�����ӳ��ڵ�ǰ�ŵ�ͣ��ʱ�䣬�����ӳ�T1
    PKG_START_FRAME,    //start frame, --�ŵ�ɨ��׶Σ��յ��÷���ֵ���������ŵ�
    PKG_DATA_FRAME,     //data frame, --���ݰ��������ŵ���ʱ��T2�ղ������ݰ��������½���ɨ��׶�
    PKG_ALINK_ROUTER,   //alink router
    PKG_GROUP_FRAME,    //group frame
    PKG_END             //--���������¼������õ�ssid��passwd��ͨ���ص�����ȥ��ȡssid��passwd
    /*
    �ο�ֵ�
    T1:             400ms >= T2 >= 100ms
    T2:             3s
    */
};


//����monitorģʽǰ����øú���
void zconfig_init(struct zconfig_constructor *con);
//�����ɹ��󣬵��øú������ͷ��ڴ���Դ
void zconfig_destroy(void);
/*
    ����monitor/snifferģʽ�󣬽��յ��İ������ú������д���
    ������monitorʱ���а��������ã����¼��ְ����ܹ��ˣ�
    1) ���ݰ���Ŀ�ĵ�ַΪ�㲥��ַ
    2) ����>40�Ĺ���֡
    input:
    pkt_data: 80211 wireless raw package, include data frame & management frame
    pkt_length:radio_hdr + 80211 hdr + payload, without fcs(4B)
    return:
    ��enum _PKG_TYPE_�ṹ��˵��
*/
int zconfig_recv_callback(void *pkt_data, u32 pkt_length, u8 channel);

/*
 * save apinfo
 * 0 -- success, otherwise, failed.
 */
int zconfig_set_apinfo(u8 *ssid, u8* bssid, u8 channel, u8 auth, u8 encry);

/* helper function, auth/encry type to string */
const char *zconfig_auth_str(u8 auth);
const char *zconfig_encry_str(u8 encry);
#endif
