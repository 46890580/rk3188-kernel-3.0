#ifndef __IT6811_MLH_H__
#define __IT6811_MLH_H__
#include "../../rk_hdmi.h"

#define DEBUG_IT6811

#ifdef DEBUG_IT6811
#define IT6811_DEBUG_PRINTF printk
#else
#define IT6811_DEBUG_PRINTF(x,...) do {} while(0)
#endif

#define TX_DEBUG_PRINT          IT6811_DEBUG_PRINTF
//#define IT6811_DEBUG_INT_PRINTF IT6811_DEBUG_PRINTF

typedef  unsigned char    cBYTE;


typedef char CHAR,*PCHAR ;
typedef unsigned char uchar,*puchar ;
typedef unsigned char UCHAR,*PUCHAR ;
typedef unsigned char byte,*pbyte ;
typedef unsigned char BYTE,*PBYTE ;

typedef short SHORT,*PSHORT ;
typedef unsigned short *pushort ;
typedef unsigned short USHORT,*PUSHORT ;
typedef unsigned short word,*pword ;
typedef unsigned short WORD,*PWORD ;
typedef unsigned int UINT,*PUINT ;

typedef long LONG,*PLONG ;
typedef unsigned long *pulong ;
typedef unsigned long ULONG,*PULONG ;
typedef unsigned long dword,*pdword ;
typedef unsigned long DWORD,*PDWORD ;


typedef enum _SYS_STATUS {
    ER_SUCCESS = 0,
    ER_FAIL,
    ER_RESERVED
} SYS_STATUS ;


#if defined(CONFIG_HDMI_SOURCE_LCDC1)
#define HDMI_SOURCE_DEFAULT HDMI_SOURCE_LCDC1
#else
#define HDMI_SOURCE_DEFAULT HDMI_SOURCE_LCDC0
#endif

#define REG_TX_VENDOR_ID0   0x00
#define REG_TX_VENDOR_ID1   0x01
#define REG_TX_DEVICE_ID0   0x02
#define REG_TX_DEVICE_ID1   0x03

#define REG_TX_SYS_STATUS     0x0E
    // readonly
    #define B_TX_INT_ACTIVE    (1<<7)
    #define B_TX_HPDETECT      (1<<6)
    #define B_TX_RXSENDETECT   (1<<5)
    #define B_TXVIDSTABLE   (1<<4)
    // read/write
    #define O_TX_CTSINTSTEP    2
    #define M_TX_CTSINTSTEP    (3<<2)
    #define B_TX_CLR_AUD_CTS     (1<<1)
    #define B_TX_INTACTDONE    (1<<0)

struct it6811_mhl_pdata {
	int gpio;
	struct i2c_client *client;
	struct delayed_work delay_work;
	struct workqueue_struct *workqueue;
        int plug_status;
};

extern struct cat66121_hdmi_pdata *cat66121_hdmi;

extern int cat66121_detect_device(void);
extern int cat66121_hdmi_sys_init(void);
extern void cat66121_hdmi_interrupt(void);
extern int cat66121_hdmi_sys_detect_hpd(void);
extern int cat66121_hdmi_sys_insert(void);
extern int cat66121_hdmi_sys_remove(void);
extern int cat66121_hdmi_sys_read_edid(int block, unsigned char *buff);
extern int cat66121_hdmi_sys_config_video(struct hdmi_video_para *vpara);
extern int cat66121_hdmi_sys_config_audio(struct hdmi_audio *audio);
extern void cat66121_hdmi_sys_enalbe_output(int enable);
extern int cat66121_hdmi_register_hdcp_callbacks(void (*hdcp_cb)(void),
					 void (*hdcp_irq_cb)(int status),
					 int (*hdcp_power_on_cb)(void),
					 void (*hdcp_power_off_cb)(void));
#endif
