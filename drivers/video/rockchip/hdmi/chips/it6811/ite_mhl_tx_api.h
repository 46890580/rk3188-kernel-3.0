#ifndef __MHL_TX_API_H__
#define __MHL_TX_API_H__
#include "ite_platform.h"


void 	ITE6811MhlTxInitialize( uint8_t pollIntervalMs );
#define	MHL_TX_EVENT_NONE				0x00	
#define	MHL_TX_EVENT_DISCONNECTION		0x01	
#define	MHL_TX_EVENT_CONNECTION			0x02	
#define	MHL_TX_EVENT_RCP_READY			0x03	
#define	MHL_TX_EVENT_RCP_RECEIVED		0x04	
#define	MHL_TX_EVENT_RCPK_RECEIVED		0x05	
#define	MHL_TX_EVENT_RCPE_RECEIVED		0x06	
#define	MHL_TX_EVENT_DCAP_CHG			0x07	
#define	MHL_TX_EVENT_DSCR_CHG			0x08	
#define	MHL_TX_EVENT_POW_BIT_CHG		0x09	
#define	MHL_TX_EVENT_RGND_MHL			0x0A	
typedef enum
{
	MHL_TX_EVENT_STATUS_HANDLED = 0
	,MHL_TX_EVENT_STATUS_PASSTHROUGH
}MhlTxNotifyEventsStatus_e;
bool_t ITE6811MhlTxRcpSend( uint8_t rcpKeyCode );
bool_t ITE6811MhlTxRcpkSend( uint8_t rcpKeyCode );
bool_t ITE6811MhlTxRcpeSend( uint8_t rcpeErrorCode );
bool_t ITE6811MhlTxSetPathEn(void);
bool_t ITE6811MhlTxClrPathEn(void);
extern	void	AppMhlTxDisableInterrupts( void );
extern	void	AppMhlTxRestoreInterrupts( void );
extern	void	AppVbusControl( bool_t powerOn );
void  AppNotifyMhlEnabledStatusChange(bool_t enabled);
void  AppNotifyMhlDownStreamHPDStatusChange(bool_t connected);
MhlTxNotifyEventsStatus_e AppNotifyMhlEvent(uint8_t eventCode, uint8_t eventParam);
void ITE6811MhlTxDeviceIsr(void);
void hdmi_drv_power_on(void);
int hdmi_drv_video_enable(bool enable);
int hdmi_drv_audio_enable(bool enable);
void iteHdmiTx_VideoSel (int vmode);
void iteHdmiTx_AudioSel (int AduioMode);



typedef enum
{
	SCRATCHPAD_FAIL= -4
	,SCRATCHPAD_BAD_PARAM = -3
	,SCRATCHPAD_NOT_SUPPORTED = -2
	,SCRATCHPAD_BUSY = -1
	,SCRATCHPAD_SUCCESS = 0
}ScratchPadStatus_e;

typedef enum{
    HDMI_STATE_NO_DEVICE,
    HDMI_STATE_ACTIVE,
    HDMI_STATE_DPI_ENABLE
}HDMI_STATE;

ScratchPadStatus_e ITE6811MhlTxRequestWriteBurst(uint8_t startReg, uint8_t length, uint8_t *pData);
bool_t MhlTxCBusBusy(void);
void MhlTxProcessEvents(void);
uint8_t    ITE6811TxReadConnectionStatus(void);
uint8_t ITE6811MhlTxSetPreferredPixelFormat(uint8_t clkMode);
uint8_t ITE6811TxGetPeerDevCapEntry(uint8_t index,uint8_t *pData);
ScratchPadStatus_e ITE6811GetScratchPadVector(uint8_t offset,uint8_t length, uint8_t *pData);
int hdmi_drv_init(void);


#if defined(__KERNEL__)
#include <linux/kernel.h>
#define	PRINT	printk
#else
#include "ite_osdebug.h"
#define	PRINT	printf
#endif

#ifndef DEBUG
#define DEBUG
#endif

// xuecheng, disable EDID print to see whether plug in detect will be faster
#define ENABLE_TX_EDID_PRINT 0
#ifdef ENABLE_TX_EDID_PRINT
    #define TX_EDID_PRINT printk
#else
    #define TX_EDID_PRINT(x) do {} while (0)
#endif
#endif  
