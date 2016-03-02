

#include "ite_drv_mhl_tx.h"

#ifndef DEBUG
#define DEBUG
#endif

#ifdef DEBUG
#define TRACE_INT_TIME
#endif

#ifdef TRACE_INT_TIME
#include <linux/jiffies.h>
#endif

#include <linux/string.h>
#include <linux/delay.h>

#include "ite_hal.h"

#include "ite_mhl_defs.h"
#include "ite_mhl_tx_api.h"
#include "ite_mhl_tx_base_drv_api.h"  

#include "ite_drv_mhl_tx.h"
#include "ite_platform.h"

#include <linux/time.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/hrtimer.h>
#include <linux/time.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/interrupt.h>
#include <linux/sched.h>

#include "ite_drv_mhl_tx.h"
#include "it6811_mhl.h"
#include "it6811_mhl_hw.h"

#define IT6811_MHL_ADOPTER_ID    0x0245
#define POWER_STATE_D3              3
#define POWER_STATE_D0_NO_MHL       2
#define POWER_STATE_D0_MHL          0
#define POWER_STATE_FIRST_INIT      0xFF
#define TX_HW_RESET_PERIOD      10  
#define TX_HW_RESET_DELAY           100
#define IT6811_MHL_ADDR	0xC8

extern long int get_current_time_us(void);

static uint8_t fwPowerState = POWER_STATE_FIRST_INIT;

static  bool_t      mscCmdInProgress;   

bool_t      mscAbortFlag = false;   


#define I2C_INACCESSIBLE -1
#define I2C_ACCESSIBLE 1
#define SIZE_AVI_INFOFRAME              17

void    SwitchToD3( void );

static  void    SwitchToD0( void );
static  void    ForceUsbIdSwitchOpen ( void );
static  void    MhlTxDrvProcessConnection ( void );
static  void    MhlTxDrvProcessDisconnection ( void );

static bool_t tmdsPowRdy;
video_data_t video_data;
static AVMode_t AVmode = {VM_INVALID, AUD_INVALID};

static void AudioVideoIsr(bool_t force_update);
static void SetAudioMode(inAudioTypes_t audiomode);

extern uint8_t VIDEO_CAPABILITY_D_BLOCK_found;
static int hdmitx_ini(void);
void HDMITX_change_audio(unsigned char AudType,unsigned char AudFs,unsigned char AudCh);
void HDMITX_Set_ColorType(unsigned char inputColorMode,unsigned char outputColorMode);
void HDMITX_SET_SignalType(unsigned char DynRange,unsigned char colorcoef,unsigned char pixrep);
void HDMITX_SetVideoOutput(int mode);

struct it6811_dev_data it6811data;

#define delay1ms(ms) msleep(ms)

static void hdimtx_write_init(struct IT6811_REG_INI const *tdata);
static void mhltx_write_init(struct IT6811_REG_INI const *tdata);
static void hdmirx_Var_init(struct it6811_dev_data *it6811);
static void chgbank( unsigned char bankno );
static void hdmitx_irq( struct it6811_dev_data *it6811 );
static int mscfire( int offset, int wdata );
static int mscwait( void );
static void hdmitx_rst( struct it6811_dev_data *it6811 );
static void fire_afe(unsigned char on);
static unsigned long cal_pclk( void /*struct it6811_dev_data *it6811*/ );
static int ddcfire( int offset, int wdata );
static int ddcwait( void );

#ifdef _SUPPORT_HDCP_
static int Hdmi_HDCP_state(struct it6811_dev_data *it6811 , HDCPSts_Type state);
static int hdcprd( int offset, int bytenum );
static int hdmtx_enhdcp( struct it6811_dev_data *it6811);
static int Hdmi_HDCP_state(struct it6811_dev_data *it6811 , HDCPSts_Type state);
static int Hdmi_HDCP_handler(struct it6811_dev_data *it6811 );
static void hdmitx_int_HDCP_AuthFail(struct it6811_dev_data *it6811);
#ifdef _SHOW_HDCP_INFO_
static void hdcpsts( void );
#endif
#endif
static void mhl_read_mscmsg( struct it6811_dev_data *it6811 );
#ifdef _SUPPORT_RCP_

static void rcp_report_event( unsigned char key);
#endif
#ifdef _SUPPORT_UCP_
static void ucp_report_event( unsigned char key);
#endif
#ifdef _SUPPORT_UCP_MOUSE_
static void ucp_mouse_report_event( unsigned char key,int x,int y);
#endif
static struct it6811_dev_data* get_it6811_dev_data(void);
static int get_it6811_Aviinfo(struct it6811_dev_data *it6811);
void HDMITX_SET_PacketPixelmode(unsigned char mode);

int IteIrqCheck(void);
int IteMhlDeviceTxIsr(void);
void IteMhlOpenUSBswitch(int on);
void IteMhlEnterIDDQmode(void);


bool_t ITE6811MhlTxChipInitialize( void )
{
    tmdsPowRdy = false;
    mscCmdInProgress = false;   

    fwPowerState = POWER_STATE_D0_NO_MHL;
    ITE_OS_DISABLE_DEBUG_CHANNEL(ITE6811_OSAL_DEBUG_SCHEDULER);
    //memset(&video_data, 0x00, sizeof(video_data));    
    video_data.inputColorSpace = COLOR_SPACE_RGB;
    video_data.outputColorSpace = video_data.inputColorSpace ;//COLOR_SPACE_RGB;// change by spl
    video_data.outputVideoCode = 2;
    video_data.inputcolorimetryAspectRatio = 0x18;
    video_data.outputcolorimetryAspectRatio = 0x18;
    video_data.output_AR = 0;

    hdmitx_ini();

    SwitchToD3();

    return true;
}

void ITE6811MhlTxDeviceIsr(void)
{
    uint8_t intMStatus;
    int i;
#ifdef TRACE_INT_TIME
    unsigned long K1;
    unsigned long K2;
    printk("-------------------ITE6811MhlTxDeviceIsr start -----------------\n");
    K1 = get_jiffies_64();
#endif
    i = 0;
    do {
        IteMhlDeviceTxIsr();

        if (POWER_STATE_D3 != fwPowerState) {
            MhlTxProcessEvents();
        }

        intMStatus= hdmitxrd(0xF0); 
        if (0x00 == intMStatus) {
            TX_DEBUG_PRINT("Drv: EXITING ISR DUE TO intMStatus - 0x00 loop = [%02X] intMStatus = [%02X] \n\n", (int) i, (int)intMStatus);
        }
        i++;
        
        if (i > 60) {
            TX_DEBUG_PRINT("force exit ITE6811MhlTxDeviceIsr \n");
            break;
        } else if (i > 50) {
            TX_DEBUG_PRINT("something error in ITE6811MhlTxDeviceIsr \n");
        }
    } while (intMStatus);

#ifdef TRACE_INT_TIME
    K2 = get_jiffies_64();
    printk("-------------------ITE6811MhlTxDeviceIsr last %d ms----------------\n",(int)(K2 - K1));
#endif
}

void ITE6811ExtDeviceIsr(void)
{
    if (fwPowerState <= POWER_STATE_D0_NO_MHL) {

#ifdef TRACE_INT_TIME
        unsigned long K1;
        unsigned long K2;
        K1 = get_jiffies_64();
#endif

        AudioVideoIsr(false);

#ifdef TRACE_INT_TIME
        K2 = get_jiffies_64();
#endif
    } else {
        TX_DEBUG_PRINT("in D3 mode , ITE6811ExtDeviceIsr not handled\n");
    }
}

void ITE6811MhlTxDrvTmdsControl (bool_t enable)
{
	tmdsPowRdy = enable;
	fire_afe(enable);
	TX_DEBUG_PRINT("TMDS Output %s\n", enable ? "Enabled" : "Disabled");
}

void    ITE6811MhlTxDrvNotifyEdidChange ( void )
{
    TX_DEBUG_PRINT("ITE6811MhlTxDrvNotifyEdidChange\n");
}

bool_t ITE6811MhlTxDrvSendCbusCommand ( cbus_req_t *pReq  )
{
    bool_t  success = true;
    uint8_t i, startbit;
    if( (POWER_STATE_D0_MHL != fwPowerState ) || (mscCmdInProgress)) {
        TX_DEBUG_PRINT("Error: fwPowerState: %02X, or CBUS(0x1C):%02X mscCmdInProgress = %d\n", (int) fwPowerState, (int) mhltxrd(0x1C), (int) mscCmdInProgress);
        return false;
    }
    mscCmdInProgress    = true;

    TX_DEBUG_PRINT("Sending MSC command %02X, %02X, %02X\n", pReq->command, (MHL_MSC_MSG == pReq->command) ? pReq->payload_u.msgData[0] : pReq->offsetData, (MHL_MSC_MSG == pReq->command) ? pReq->payload_u.msgData[1] : pReq->payload_u.msgData[0]);

    mhltxwr(0x54, pReq->offsetData);   
    mhltxwr(0x55, pReq->payload_u.msgData[0]);
    
    startbit = 0x00;
    switch ( pReq->command )
    {
        case MHL_SET_INT:   
            mhltxwr(0x50,0x80);
            break;

        case MHL_WRITE_STAT:    
            mhltxwr(0x50,0x80);
            break;

        case MHL_READ_DEVCAP:   
            mhltxwr(0x50,0x40);
            break;

        case MHL_GET_STATE:   
            mhltxwr(0x50,0x01);
            break;      

        case MHL_GET_VENDOR_ID:   
            mhltxwr(0x50,0x02);
            break;  

        case MHL_SET_HPD:   
            mhltxwr(0x50,0x04);
            break;        

        case MHL_CLR_HPD:   
            mhltxwr(0x50,0x08);
            break;        

        case MHL_GET_DDC_ERRORCODE:  
            mhltxwr(0x50,0x10);
            break;    

        case MHL_GET_MSC_ERRORCODE: 
            mhltxwr(0x50,0x20);
            break; 

        case MHL_GET_SC1_ERRORCODE:         
        case MHL_GET_SC3_ERRORCODE:     
            mhltxwr(0x54, pReq->command );
            mhltxwr(0x51,0x02);
            break;

        case MHL_MSC_MSG:
            mhltxwr(0x54, pReq->command );
            mhltxwr(0x55, pReq->payload_u.msgData[1] );
            mhltxwr(0x51,0x02);
            break;

        case MHL_WRITE_BURST:
            if (NULL == pReq->payload_u.pdatabytes) {
                TX_DEBUG_PRINT("\nPut pointer to WRITE_BURST data in req.pdatabytes!!!\n\n");
                success = false;
            } else {
                uint8_t *pData = pReq->payload_u.pdatabytes;
                TX_DEBUG_PRINT("\nWriting data into scratchpad\n\n");

                mhltxset(0x5C, 0x01, 0x01); // TxPktFIFO clear
                mhltxset(0x00, 0x80, 0x00);

                for (i = 0; i < pReq->length; i++) {
                    mhltxwr(0x59, *pData++ );      //Write Burst use TXpktFIFO
                }
                mhltxwr(0x51,0x01);
            }
            break;

        default:
            success = false;
            break;
    }

    if (!success)
        TX_DEBUG_PRINT("\nITE6811MhlTxDrvSendCbusCommand failed\n\n");

    return (success);
}

bool_t ITE6811MhlTxDrvCBusBusy(void)
{
    return mscCmdInProgress ? true :false;
}

static void ForceUsbIdSwitchOpen( void )
{
    IteMhlOpenUSBswitch(0);
}

void SwitchToD0( void )
{
    TX_DEBUG_PRINT("Switch to D0\n");
    fwPowerState = POWER_STATE_D0_NO_MHL;
    AudioVideoIsr(true);
}

extern void CBusQueueReset(void);

void SwitchToD3( void )
{
    if(POWER_STATE_D3 != fwPowerState) {
        TX_DEBUG_PRINT("Switch To D3\n");

        ForceUsbIdSwitchOpen();  //change bus to usb mode 
        IteMhlEnterIDDQmode();
        CBusQueueReset();
        
        fwPowerState = POWER_STATE_D3;
    }
}

void ForceSwitchToD3(void)
{
    IteMhlOpenUSBswitch(0);
    IteMhlEnterIDDQmode();
    CBusQueueReset();
    fwPowerState = POWER_STATE_D3;
}

static void MhlTxDrvProcessConnection ( void )
{
    TX_DEBUG_PRINT("MHL Cable Connected. CBUS:0x1C = %02X\n", (int) mhltxrd(0x1C));

    if (POWER_STATE_D0_MHL != fwPowerState) {
        fwPowerState = POWER_STATE_D0_MHL;
        ITE6811MhlTxNotifyConnection(true);
    }
}

static void MhlTxDrvProcessDisconnection( void )
{
    TX_DEBUG_PRINT("MhlTxDrvProcessDisconnection\n");
    
    ITE6811MhlTxNotifyDsHpdChange(0);
    
    if (POWER_STATE_D0_MHL == fwPowerState ) {
        ITE6811MhlTxNotifyConnection(false);
    }
    
    SwitchToD3();
}

static uint8_t CBusProcessErrors( uint8_t intStatus )
{
    uint8_t result       = 0;
    uint8_t abortReason  = 0;
 

    if (intStatus & 0x02) {
        abortReason = mhltxrd(0x18);
        mhltxwr(0x18, abortReason);
        if (abortReason & 0x01) {
            IT6811_DEBUG_INT_PRINTF("IT6811-ERROR: Incomplete Packet !!!\n");
        }
        if (abortReason & 0x02) {
            IT6811_DEBUG_INT_PRINTF("IT6811-ERROR: 100ms TimeOut !!!\n");
        }
        if (abortReason & 0x04) {
            IT6811_DEBUG_INT_PRINTF("IT6811-ERROR: Protocol Error !!!\n");
        }
        if (abortReason & 0x08) {
            IT6811_DEBUG_INT_PRINTF("IT6811-ERROR: Retry > 32 times !!!\n");
        }
        if (abortReason & 0x10) {
            IT6811_DEBUG_INT_PRINTF("IT6811-ERROR: Receive ABORT Packet !!!\n");
        }
        if (abortReason & 0x20) {
            IT6811_DEBUG_INT_PRINTF("IT6811-MSC_MSG Requester Receive NACK Packet !!! \n");
        }
        if (abortReason & 0x40) {
            IT6811_DEBUG_INT_PRINTF("IT6811-Disable HW Retry and MSC Requester Arbitration Lose at 1st Packet !!!\n");
        }
        if (abortReason & 0x80) {
            IT6811_DEBUG_INT_PRINTF("IT6811-Disable HW Retry and MSC Requester Arbitration Lose before 1st Packet !!!\n");
        
        }

        abortReason = mhltxrd(0x19);
        mhltxwr(0x19, abortReason);
        if( abortReason&0x01 ) {
            IT6811_DEBUG_INT_PRINTF("IT6811-ERROR: TX FW Fail in the middle of the command sequence !!!\n");
        }
        if( abortReason&0x02 ) {
            IT6811_DEBUG_INT_PRINTF("IT6811-ERROR: TX Fail because FW mode RxPktFIFO not empty !!!\n");
        }
    }

    if (intStatus & 0x08) {
         abortReason = mhltxrd(0x1A);
         mhltxwr(0x1A, abortReason);
       
        if (abortReason & 0x01) {
            IT6811_DEBUG_INT_PRINTF("IT6811-ERROR: Initial Bad Offset !!!\n");
        }
        if (abortReason & 0x02) {
            IT6811_DEBUG_INT_PRINTF("IT6811-ERROR: Incremental Bad Offset !!!\n");
        }
        if (abortReason & 0x04) {
            IT6811_DEBUG_INT_PRINTF("IT6811-ERROR: Invalid Command !!!\n");
        }
        if (abortReason & 0x08) {
             IT6811_DEBUG_INT_PRINTF("IT6811-ERROR: Receive dPacket in Responder Idle State !!!\n");
        }
        if (abortReason & 0x10) {
            IT6811_DEBUG_INT_PRINTF("IT6811-ERROR: Incomplete Packet !!!\n");
        }
        if (abortReason & 0x20) {
            IT6811_DEBUG_INT_PRINTF("IT6811-ERROR: 100ms TimeOut !!!\n");
        }
        if (abortReason & 0x40) {
            IT6811_DEBUG_INT_PRINTF("IT6811-MSC_MSG Responder Busy ==> Return NACK Packet !!!\n");
        }
        if (abortReason & 0x80) {
			IT6811_DEBUG_INT_PRINTF("IT6811-ERROR: Protocol Error !!!\n");
		}

        abortReason = mhltxrd(0x1B);
        mhltxwr(0x1B, abortReason);
        if (abortReason & 0x01) {
			IT6811_DEBUG_INT_PRINTF("IT6811-ERROR: Retry > 32 times !!!\n");
		}
        if (abortReason & 0x02) {
			IT6811_DEBUG_INT_PRINTF("IT6811-ERROR: Receive ABORT Packet !!!\n");
		}
     
     }
        
    if(intStatus&0x20) {
        abortReason = mhltxrd(0x16);
        mhltxwr(0x16, abortReason);
       
        if( abortReason&0x01 ) {
            IT6811_DEBUG_INT_PRINTF("IT6811-ERROR: Retry > 32 times !!!\n");
        }
        if( abortReason&0x02 ) {
            IT6811_DEBUG_INT_PRINTF("IT6811-ERROR: DDC TimeOut !!!\n");
        }
        if( abortReason&0x04 ) {
            IT6811_DEBUG_INT_PRINTF("IT6811-ERROR: Receive Wrong Type Packet !!!\n");
        }
        if( abortReason&0x08 ) {
            IT6811_DEBUG_INT_PRINTF("IT6811-ERROR: Receive Unsupported Packet !!!\n");
        }
        if( abortReason&0x10 ) {
            IT6811_DEBUG_INT_PRINTF("IT6811-ERROR: Receive Incomplete Packet !!!\n");
        }
        if( abortReason&0x20 ) {
            IT6811_DEBUG_INT_PRINTF("IT6811-ERROR: Receive ABORT in Idle State !!!\n");
        }
        if( abortReason&0x40 ) {
            IT6811_DEBUG_INT_PRINTF("IT6811-ERROR: Receive Unexpected Packet!!!\n");
        }
        if( abortReason&0x80 ) {
            IT6811_DEBUG_INT_PRINTF("IT6811-ERROR: Receive ABORT in non-Idle State !!!\n");
        }
    }

    return( result );
}

void ITE6811MhlTxDrvGetScratchPad(uint8_t startReg,uint8_t *pData,uint8_t length)
{
	int i;
    
    for (i = 0; i < length; ++i,++startReg) {
        *pData++ = mhltxrd(0xC0 + startReg);
    }
}


void ITE6811MhlTxDrvPowBitChange (bool_t enable)
{
    if (enable) {
        mhltxset(0x0F, 0x04, 0x04);     // For dongle device, some dongle does not report RxPOW correctly 
        TX_DEBUG_PRINT("POW bit 0->1, set DISC_CTRL8[2] = 1\n");
    }
}

void ITEMhlTxDrvSetClkMode(uint8_t clkMode)
{
    TX_DEBUG_PRINT("ITEMhlTxDrvSetClkMode:0x%02x\n",(int)clkMode);

    if (MHL_STATUS_CLK_MODE_NORMAL == clkMode) {
        HDMITX_SET_PacketPixelmode(0); //set normal pixel mode
        TX_DEBUG_PRINT("ITEMhlTxDrvSetClkMode: MHL_STATUS_CLK_MODE_NORMAL \n");
    } else {
        HDMITX_SET_PacketPixelmode(2); //set force packet pixel mode
        TX_DEBUG_PRINT("ITEMhlTxDrvSetClkMode: MHL_STATUS_CLK_MODE_PACKED_PIXEL \n");
    }
}

PLACE_IN_CODE_SEG const audioConfig_t audioData[AUD_TYP_NUM] =
{
    {0x11, 0x40, 0x0E, 0x03, 0x00, 0x05},
    {0x11, 0x40, 0x0A, 0x01, 0x00, 0x05},
    {0x11, 0x40, 0x02, 0x00, 0x00, 0x05},
    {0x11, 0x40, 0x0C, 0x03, 0x00, 0x05},
    {0x11, 0x40, 0x08, 0x01, 0x00, 0x05},
    {0x11, 0x40, 0x00, 0x00, 0x00, 0x05},
    {0x11, 0x40, 0x03, 0x00, 0x00, 0x05},
    {0x11, 0x40, 0x0E, 0x03, 0x03, 0x05},
    {0x11, 0x40, 0x0A, 0x01, 0x03, 0x05},
    {0x11, 0x40, 0x02, 0x00, 0x03, 0x05},
    {0x11, 0x40, 0x0C, 0x03, 0x03, 0x05},
    {0x11, 0x40, 0x08, 0x01, 0x03, 0x05},
    {0x11, 0x40, 0x00, 0x00, 0x03, 0x05},
    {0x11, 0x40, 0x03, 0x00, 0x03, 0x05},
    {0xF1, 0x40, 0x0E, 0x00, 0x03, 0x07},
    {0x03, 0x00, 0x00, 0x00, 0x00, 0x05}
};

static void SetAudioMode(inAudioTypes_t audiomode)
{
    if (audiomode >= AUD_TYP_NUM)
        audiomode = I2S_48;
}

static void AudioVideoIsr(bool_t force_update)
{
    AVModeChange_t mode_change = {false, false};
    unsigned char pixelrep = 0;

    if (force_update) {
		TX_DEBUG_PRINT("ITE6811VideoInputIsValid,audio_changed,video_changed\n");
        mode_change.audio_change = true;
        mode_change.video_change = true;
    } else {
    	TX_DEBUG_PRINT("force_update=false...............\n");
    }

    if (mode_change.audio_change) {
        TX_DEBUG_PRINT("SetAudioMode & SetACRNValue\n");
        HDMITX_change_audio((unsigned char) LPCM, (unsigned char) audioData[AVmode.audio_mode].regAUD_freq, (unsigned char) 2);
    }

    if(mode_change.video_change) {
        TX_DEBUG_PRINT("mode_change.video_changed =true\n ");

        video_data.outputColorSpace = video_data.inputColorSpace;
        video_data.outputcolorimetryAspectRatio = video_data.inputcolorimetryAspectRatio;

		HDMITX_Set_ColorType((unsigned char)video_data.inputColorSpace,(unsigned char) video_data.outputColorSpace);

        if(cal_pclk()<25000L) {
            pixelrep =1;
        }

        if (VIDEO_CAPABILITY_D_BLOCK_found) {
            HDMITX_SET_SignalType(DynCEA,ITU709,pixelrep);
            HDMITX_DEBUG_PRINTF("VIDEO_CAPABILITY_D_BLOCK_found = true, limited range\n");
        } else {
            HDMITX_SET_SignalType(DynVESA,ITU709,pixelrep);
            HDMITX_DEBUG_PRINTF("VIDEO_CAPABILITY_D_BLOCK_found= false. defult range\n");
        }

        HDMITX_SetVideoOutput((int) video_data.inputVideoCode);
    }

    if ((mode_change.video_change || mode_change.audio_change) && tmdsPowRdy) {
            TX_DEBUG_PRINT("((mode_change.video_change || mode_change.audio_change) && tmdsPowRdy) \n");
    }
}


#if defined(USE_PROC)&&defined(__KERNEL__)
void drv_mhl_seq_show(struct seq_file *s)
{
    int gpio_value;
    switch (fwPowerState) {
        case POWER_STATE_D3:
             seq_printf(s, "MHL POWER STATE          [D3]\n");
             break;
        case POWER_STATE_D0_NO_MHL:
             seq_printf(s, "MHL POWER STATE          [D0_NO_MHL]\n");
             break;
        case POWER_STATE_D0_MHL:
             seq_printf(s, "MHL POWER STATE          [D0_MHL]\n");
             break;
        case POWER_STATE_FIRST_INIT:
             seq_printf(s, "MHL POWER STATE          [FIRST_INIT]\n");
             break;
        default:
            break;
    }

    if (tmdsPowRdy)  
        seq_printf(s, "TMDS                     [ON]\n");
    else
        seq_printf(s, "TMDS                     [OFF]\n");

}
#endif

//------------------------------------------------------------------------------
// Function Name: iteHdmiTx_VideoSel()
// Function Description: Select output video mode
//
// Accepts: Video mode
// Returns: none
// Globals: none
//------------------------------------------------------------------------------
void iteHdmiTx_VideoSel(int vmode)
{
    int AspectRatio = 0;
    video_data.inputColorSpace  = COLOR_SPACE_RGB;
    video_data.outputColorSpace = COLOR_SPACE_RGB;
    video_data.inputVideoCode   = vmode;

    switch (vmode) {
        case HDMI_480I60_4X3:
        case HDMI_576I50_4X3:
            AspectRatio = VMD_ASPECT_RATIO_4x3;
            break;
            
        case HDMI_480I60_16X9:
        case HDMI_576I50_16X9:
            AspectRatio     = VMD_ASPECT_RATIO_16x9;
            break;
            
        case HDMI_480P60_4X3:
        case HDMI_576P50_4X3:
        case HDMI_640X480P:
            AspectRatio     = VMD_ASPECT_RATIO_4x3;
            break;

        case HDMI_480P60_16X9:
        case HDMI_576P50_16X9:
            AspectRatio     = VMD_ASPECT_RATIO_16x9;
            break;
            
        case HDMI_720P60:
        case HDMI_720P50:
        case HDMI_1080I60:
        case HDMI_1080I50:
        case HDMI_1080P24:
        case HDMI_1080P25:
        case HDMI_1080P30:              
            AspectRatio     = VMD_ASPECT_RATIO_16x9;
            break;
                
        default:
            break;
    }

    if (AspectRatio == VMD_ASPECT_RATIO_4x3)
        video_data.inputcolorimetryAspectRatio = 0x18;
    else
		video_data.inputcolorimetryAspectRatio = 0x28;
    video_data.input_AR = AspectRatio;
}

void iteHdmiTx_AudioSel (int AduioMode)
{
    AVmode.audio_mode = AduioMode;
}

#define T_RES_CHANGE_DELAY      128         // delay between turning TMDS bus off and changing output resolution

#ifdef _SUPPORT_RAP_                                
static int const SuppRAPCode[32] = {
                    //  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, A, B, C, D, E, F
                        1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0
                        1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};// 1
#endif

#ifdef _SUPPORT_RCP_   
static int const SuppRCPCode[128]= { 
                        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, // 0
                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 1
                        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, // 2
                        1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, // 3
                        0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, // 4
                        1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 5
                        1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, // 6
                        0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0};// 7

#endif

static  unsigned char const AVIDB_Table[][AVI_INFOFRAME_LEN] =
{
    //Pclk(k)    DB[0],DB[1],DB[2],DB[3],DB[4],DB[5],DB[6],DB[7],DB[8],DB[9],DB[A],DB[B],DB[C]
    {0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00},// mode 0 //VESA mode
    {0x00 ,0x28 ,0x00 ,0x04 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00},// mode 1280X720P@60 16:9 CEA
};

#ifndef DISABLE_HDMITX_CSC
#define SIZEOF_CSCMTX 21
static unsigned char const bCSCMtx_RGB2YUV_ITU601_16_235[SIZEOF_CSCMTX]=
{
    0x00,0x80,0x10,
    0xB2,0x04,0x65,0x02,0xE9,0x00,
    0x93,0x3C,0x18,0x04,0x55,0x3F,
    0x49,0x3D,0x9F,0x3E,0x18,0x04
};

static unsigned char const bCSCMtx_RGB2YUV_ITU601_0_255[SIZEOF_CSCMTX]=
{
    0x10,0x80,0x10,
    0x09,0x04,0x0E,0x02,0xC9,0x00,
    0x0F,0x3D,0x84,0x03,0x6D,0x3F,
    0xAB,0x3D,0xD1,0x3E,0x84,0x03
};

static unsigned char const bCSCMtx_RGB2YUV_ITU709_16_235[SIZEOF_CSCMTX]=
{
    0x00,0x80,0x10,
    0xB8,0x05,0xB4,0x01,0x94,0x00,
    0x4A,0x3C,0x17,0x04,0x9F,0x3F,
    0xD9,0x3C,0x10,0x3F,0x17,0x04
};

static unsigned char const bCSCMtx_RGB2YUV_ITU709_0_255[SIZEOF_CSCMTX]=
{
    0x10,0x80,0x10,
    0xEA,0x04,0x77,0x01,0x7F,0x00,
    0xD0,0x3C,0x83,0x03,0xAD,0x3F,
    0x4B,0x3D,0x32,0x3F,0x83,0x03   
};

static unsigned char const bCSCMtx_YUV2RGB_ITU601_16_235[SIZEOF_CSCMTX] =
{
    0x00,0x00,0x00,
    0x00,0x08,0x6B,0x3A,0x50,0x3D,
    0x00,0x08,0xF5,0x0A,0x02,0x00,
    0x00,0x08,0xFD,0x3F,0xDA,0x0D   
};

static unsigned char const  bCSCMtx_YUV2RGB_ITU601_0_255[SIZEOF_CSCMTX] =
{
    0x04,0x00,0xA7,
    0x4F,0x09,0x81,0x39,0xDD,0x3C,
    0x4F,0x09,0xC4,0x0C,0x01,0x00,
    0x4F,0x09,0xFD,0x3F,0x1F,0x10   
 };
static unsigned char const  bCSCMtx_YUV2RGB_ITU709_16_235[SIZEOF_CSCMTX] =
{
    0x00,0x00,0x00,
    0x00,0x08,0x55,0x3C,0x88,0x3E,
    0x00,0x08,0x51,0x0C,0x00,0x00,
    0x00,0x08,0x00,0x00,0x84,0x0E   
};

static unsigned char const bCSCMtx_YUV2RGB_ITU709_0_255[SIZEOF_CSCMTX] =
{
    0x04,0x00,0xA7,
    0x4F,0x09,0xBA,0x3B,0x4B,0x3E,
    0x4F,0x09,0x57,0x0E,0x02,0x00,
    0x4F,0x09,0xFE,0x3F,0xE8,0x10
};
#endif

static  struct IT6811_REG_INI const tIt6811init_hdmi[] = {

    {0x0F, 0x40, 0x00},   // Enable GRCLK
    // add for HDCP issue
    {0x04, 0x1D, 0x1D},   // ACLK/VCLK/HDCP Reset
    // PLL Reset
    {0x62, 0x08, 0x00},   // XP_RESETB
    {0x64, 0x04, 0x00},   // IP_RESETB 
    {0x04, 0x20, 0x20},   // RCLK Reset
    {0x04, 0x1D, 0x1D},   // ACLK/VCLK/HDCP Reset
    // MHL Slave Address
    {0x8D, 0xFF, IT6811_MHL_ADDR|0x01}, //CBUS Slave address enable
    {0x62, 0x90, 0x10},
    {0x64, 0x89, 0x09},
    {0x68, 0x10, 0x10},    // EnExtRST =0
    {0xE8, 0x02, 0x02},    //EnCBusSMT ==1
    // Initial Value]
    {0xF8, 0xFF, 0xC3},
    {0xF8, 0xFF, 0xA5},
    {0x5D, 0x08, 0x00},     //  EnExtOSC = FALSE;
    {0x05, 0x1E, 0x00},     //ForceRxOn =FALSE; RCLKPDSel =0;  PwrDnRCLK = FALSE;
    {0xF4, 0x0C, 0x00},  //DDC75K(0),DDC125K(1),DDC312K(2)
    {0xF3, 0x32, 0x30},  //ForceVOut = 0; CBUSDrv =1
    {0xF8, 0xFF, 0xFF},
    {0x5A, 0x0C, 0x0C},
    {0xD1, 0x0A, 0x02},     // ForceTxCLKStb =0; High Sensitivity
    // RINGOSC 
    {0x65, 0x03, 0x00},   // _RINGOSC_NORM :0,  _RINGOSC_FAST:1  ,_RINGOSC_SLOW:2
    {0x5D, 0x04, 0x00},  // RCLK_FREQ_20M =0
    // HDCP TimeBase
    // TimeLoMax    0x24A00
    //  TimeBaseSel 00
    {0x47, 0xFF, 0x00},
    {0x48, 0xFF, 0x4A},
    {0x49, 0xC3, 0x02}, //(TimeBaseSel<<6)+((_TimeLoMax&0x30000)>>16)},
    {0x71, 0xF8, 0x18},   //(_XPStableTime<<6)+(_EnXPLockChk<<5)+(_EnPLLBufRst<<4)+(_EnFFAutoRst<<3)},
    {0xd1, 0x02, 0x00},   //_EnCBusDeGlitch =0
    {0x59, 0x08, 0x08},   // PCLKINV =1
    {0x72, 0x40, _EnBTAFmt<<6},    ///YCbCr422 BTA-T1004
    {0x70, 0xE7, (_RegPCLKDiv2<<5)|(_EnInDDR<<2)},//|0x03},               // _PCLKDLY = 0, Input Video Format
    {0xBF, 0x0F, (_PackSwap<<3)|(_LMSwap<<2)|(_YCSwap<<1)|_RBSwap},
    {0x88, 0xF0, 0x00},  //CSC color ghcoef table inital, this register is random value when power on.
    {0x59, 0x10,0x00},  // NO ManuallPR
    // Inverse Audio Latch Edge of IACLK
    {0xE1, 0x20, 0x00},  //InvAudCLK =0
    {0x05, 0xC0, 0x40},     //_IntPol_LOW 0,  _IntIOMode_IO_OD 1
    // Define SCK/WS/SPDIF Pin Mux
    {0xE9, 0x07, 0x01},   //(_UseExtVBusDet<<2)+(_EnSCKWSMux<<1)+_EnSPDIFMux}, 
    //set_aud_opt
    {0xC5, 0x20, 0x00},  //   Auto count CTS
    {0x59, 0x04, 0x00},  //  Disable audio auto mute
    {0x5C, 0x40, 0x40},  //  Enhance SPDIF mode
    {0xE1, 0x40, 0x00},  //  Disable audio full PKT mode 
    // Clear all Interrupt
    {0x06, 0xFF, 0xFF},
    {0x07, 0xFF, 0xFF},
    {0x08, 0xFF, 0xFF},
    {0xEE, 0xFF, 0xFF},
    {0x09, 0x03, 0x00},      // Enable HPD and RxSen Interrupt

    {0xFF, 0xFF, 0xFF}, /* ending mark */

};

static struct IT6811_REG_INI const tIt6811init_mhl[] = {
    // MHLTX Reset
    {0x0F, 0x10, 0x10},    // Disable CBUS
    //cal_oclk();
    {0x04, 0xFF, 0xFF},    // 
    {0x05, 0xFF, 0xFF},    // 
    {0x06, 0xFF, 0xFF},    // 
    {0x0A, 0xFF, 0x00},    // Enable Disconnect and Discovery Interrupt
    {0x08, 0xFF, 0x7F},    // Enable CBusNotDet Interrupt
    {0x01, 0x0C, 0x00},  //_RINGOSC_NORM
    {0x52, 0xFF, 0x00},    // READ_DEVCAP firmware mode
    {0x53, 0xFF, 0x80},    //MSCHwMask = 0x8000;     
    {0x00, 0x80, 0x00}, //CBUS debug control
    {0x01, 0x80, 0x80},    //EnCBusDeGlitch =1
    {0x0C, 0xF1,0xE0},      //(_PPHDCPOpt<<7)+(CBusDbgSel<<4)+(_EnHWPathEn)},
    {0x36, 0xFC, 0xB4},   //AckHigh = 0x0B AckLow=1 ;(AckHigh<<4)+(AckLow<<2))
    {0x38, 0x01,0x01},     //EnDDCWaitAbort
    {0x5C, 0xBC, 0x94},   //(EnPktFIFOBurst<<7)+(MSCBurstWrOpt<<5)+(EnMSCBurstWr<<4)+(EnMSCHwRty<<3)+(MSCRxUCP2Nack<<2)
    {0x32, 0xFF, 0x0C},
    // Device Capability Initialization
#if( _EnVBUSOut)
    {0x82, 0x10, 0x10},
#else
    {0x82, 0x10, 0x00},
#endif
    {0x83, 0xFF, 0x02},   // ADOPTER_ID_H
    {0x84, 0xFF, 0x45},   // ADOPTER_ID_L
    {0x8B, 0xFF, 0x68},   // DEVICE_ID_H
    {0x8C, 0xFF, 0x11},   // DEVICE_ID_L
     // Enable MHL
#if( _EnVBUSOut)
    {0x0F, 0x3B, 0x22}, // Enable MHL, clear bit 4 //(_EnPPGBSwap<<5)+(_EnPackPix<<3)+(_EnVBUSOut<<1)
#else
    {0x0F, 0x3B, 0x20}, // Enable MHL, clear bit 4 //(_EnPPGBSwap<<5)+(_EnPackPix<<3)+(_EnVBUSOut<<1)   
#endif
    
    {0xFF, 0xFF, 0xFF}, /* ending mark */
};

void hdimtx_write_init(struct IT6811_REG_INI const *tdata)
{
    int cnt = 0;
    while(tdata[cnt].ucAddr != 0xFF) {
        hdmitxset(tdata[cnt].ucAddr,tdata[cnt].andmask,tdata[cnt].ucValue);
        cnt++;
    }
}

void mhltx_write_init(struct IT6811_REG_INI const *tdata)
{
    int cnt = 0;
    while(tdata[cnt].ucAddr != 0xFF) {
        mhltxset(tdata[cnt].ucAddr,tdata[cnt].andmask,tdata[cnt].ucValue);
        cnt++;
    }
}

static void hdmirx_Var_init(struct it6811_dev_data *it6811)
{
    it6811->RCLK =0;
    it6811->RxSen = FALSE;
    it6811->devcapRdy = FALSE;
    it6811->VidFmt = 0;
    it6811->PixRpt = 1;
    it6811->EnPackPix = 0;     // TRUE for MHL PackedPixel Mode , FALSE for 24-bit Mode
    it6811->InColorMode  = RGB444;        //YCbCr422 RGB444 YCbCr444
    it6811->OutColorMode = YCbCr422;
    it6811->DynRange = DynCEA;        //DynCEA, DynVESA
    it6811->YCbCrCoef = ITU709;       //ITU709, ITU601
    it6811->Aviinfo = NULL;
    it6811->AudFmt = INPUT_SAMPLE_FREQ_HZ;//AUD48K;
    it6811->AudCh = 2;
    it6811->AudType = LPCM; // LPCM, NLPCM,

#ifdef _SUPPORT_HDCP_
    it6811->HDCPEnable = TRUE;
#endif


}

static void chgbank( unsigned char bankno )
{
    hdmitxset(0x0f, 0x03, bankno&0x03);
}

static unsigned long cal_pclk( void /*struct it6811_dev_data *it6811*/ )
{
    unsigned long clk;    
    switch (video_data.inputVideoCode)
    {
        case HDMI_480I60_4X3:
        case HDMI_576I50_4X3:            
        case HDMI_480I60_16X9:
        case HDMI_576I50_16X9:
            clk =  27000L/2;            
            break;
            
        case HDMI_640X480P:    
            clk = 25000L;
            break;
        case HDMI_480P60_4X3:
        case HDMI_576P50_4X3:
        case HDMI_480P60_16X9:
        case HDMI_576P50_16X9:
            clk =  27000L;
            break;
            
        case HDMI_720P60:
        case HDMI_720P50:
        case HDMI_1080I60:
        case HDMI_1080I50: 
        case HDMI_1080P24:
        case HDMI_1080P25:
        case HDMI_1080P30:
            clk = 74250L;
            break;
                
        default:
            clk = 74250L;
            break;
        }

    HDMITX_DEBUG_PRINTF("IT6811-cal_pclk() Count TxCLK=%lu MHz\n", (unsigned long)clk/1000);
    return clk;
}

static void setup_mhltxafe( struct it6811_dev_data *it6811, unsigned long HCLK )
{
    HDMITX_DEBUG_PRINTF("IT6811- setup_mhltxafe(%lu MHz)\n", (unsigned long) HCLK/1000);

    if( HCLK > 250000 ) {
        hdmitxset(0x63, 0x3F, 0x2F);
        hdmitxset(0x66, 0x80, 0x80);
        hdmitxset(0x6B, 0x0F, 0x00);
    } else  {
         hdmitxset(0x63, 0x3F, 0x23);
         hdmitxset(0x66, 0x80, 0x00);
         hdmitxset(0x6B, 0x0F, 0x03);
    }

    if( HCLK > 100000 ) {
        hdmitxset(0x62, 0x90, 0x80);
        hdmitxset(0x64, 0x89, 0x80);
        hdmitxset(0x68, 0x40, 0x00);

        HDMITX_DEBUG_PRINTF("IT6811-\nChange TX AFE setting to High Speed mode ...\n\n");

    } else {
        hdmitxset(0x62, 0x90, 0x10);
        hdmitxset(0x64, 0x89, 0x09);
        hdmitxset(0x68, 0x40, 0x10);

        HDMITX_DEBUG_PRINTF("IT6811-\nChange TX AFE setting to Low Speed mode ...\n\n");
    }
}

static void fire_afe(unsigned char on)
{
    if (on)
		hdmitxset(0x61, 0x30, 0x00);   // Enable AFE output
    else
    	hdmitxset(0x61, 0x30, 0x30);   // PowerDown AFE output

}

static void cal_oclk( struct it6811_dev_data *it6811 )
{
    int i;
    unsigned long rddata;
    unsigned long  sum, OSCCLK;
    int oscdiv, t10usint, t10usflt;
    long int sys_time1 = 0,sys_time2 = 0; //US
    unsigned long  delay_count = 0; //MS

    mhltxset(0x0F, 0x10, 0x10);    // Disable CBUS

    if(it6811->RCLK == 0){
        sum = 0;

        for(i=0; i<4; i++) {
            mhltxwr(0x01, 0x41);
            sys_time1 = get_current_time_us();
      
            mdelay(20);//100
      
            sys_time2 = get_current_time_us();      
			IT6811_DEBUG_PRINTF("cal_clock ,sys_time = %d us\n", (int)(sys_time2 - sys_time1));
			delay_count += (unsigned long)((sys_time2 - sys_time1)/1000);
       
            mhltxwr(0x01, 0x40); 
            rddata = (unsigned long)mhltxrd(0x12);
            rddata += (unsigned long)mhltxrd(0x13)<<8;
            rddata += (unsigned long)mhltxrd(0x14)<<16;

            sum += rddata;

            IT6811_DEBUG_PRINTF("IT6811-loop=%d(100ms), rddata=%lu  sum =%lu\n", i, rddata,sum);
        }

        OSCCLK = sum / delay_count;
        
        IT6811_DEBUG_PRINTF("IT6811-OSCCLK=%luKHz\n", OSCCLK);
        
        oscdiv = OSCCLK / 10000;

        if((OSCCLK % 10000) > 5000) 
            oscdiv++;
             
        IT6811_DEBUG_PRINTF("IT6811-oscdiv=%d \n", oscdiv);

        IT6811_DEBUG_PRINTF("IT6811-OCLK=%lukHz\n", OSCCLK / oscdiv);

        mhltxset(0x01, 0x70, oscdiv << 4);//0x50
        
        OSCCLK >>= 2;
        
        it6811->RCLK = (unsigned long)OSCCLK;
    }
    OSCCLK = it6811->RCLK;
    t10usint = OSCCLK/100;
    t10usflt = OSCCLK%100; 
    
    IT6811_DEBUG_PRINTF("IT6811-T10usInt=0x%X, T10usFlt=0x%X\n", (int)t10usint, (int)t10usflt);

    mhltxwr(0x02,(unsigned char) (t10usint&0xFF)); //0x7e
    mhltxwr(0x03,(unsigned char) (((t10usint&0x100)>>1)+t10usflt));//0x02
    IT6811_DEBUG_PRINTF("IT6811-MHL reg 0x02 = %X , reg 0x03 = %X\n", (int )mhltxrd(0x02), (int)mhltxrd(0x03));
}


static void Mhl_state(struct it6811_dev_data *it6811 , MHLState_Type state)
{
   if (it6811->Mhl_state == state){
        switch (state) {
            case MHL_Cbusdet:
                it6811->CBusDetCnt++;
	            break;

            case MHL_1KDetect:
                it6811->Det1KFailCnt++;
                break;

            case MHL_CBUSDiscover:
                it6811->DisvFailCnt++;  
                break;

			case MHL_USB_PWRDN:
			case MHL_USB:
            case MHL_CBUSDisDone:
            default :
                break;
        } 
   } else {
	    it6811->Mhl_state = state;
	    switch (state) {
	        case MHL_USB_PWRDN:
	             HDMITX_MHL_DEBUG_PRINTF("IT6811dev[DevNum].Mhl_state => MHL_USB_PWRDN \n");
	        case MHL_USB:
	            HDMITX_MHL_DEBUG_PRINTF("it6811->Mhl_state => MHL_USB \n");
				mhltxset(0x0A, 0x02, 0x02);
	            mhltxset(0x0F, 0x01, 0x01);    //Switch to USB, keep Cbusmhltxset(0x0A, 0x02, 0x02);
	            break;

	        case MHL_Cbusdet:
	            mhltxwr(0x08, 0xFF);   
	            mhltxwr(0x09, 0xFF);   
	            mhltxset(0x0F, 0x11, 0x11);    //  reset Cbus fsm
	            mhltxset(0x0F, 0x11, 0x00);    //   
	            it6811->CBusDetCnt = 0;
	            it6811->Det1KFailCnt = 0;
	            it6811->DisvFailCnt = 0;    
	            break;

	        case MHL_1KDetect:
	            HDMITX_MHL_DEBUG_PRINTF("it6811->Mhl_state => MHL_1KDetect \n");
				mhltxset(0x0A, 0x02, 0x00);
	            mhltxset(0x0F, 0x10, 0x10);    //  reset Cbus fsm
	            mhltxset(0x0F, 0x10, 0x00);    //           
	            it6811->CBusDetCnt = 0;
	            it6811->Det1KFailCnt = 0;
	            it6811->CBusDetCnt = 0;             
	            break;

	        case MHL_CBUSDiscover:
	            // Enable MHL CBUS Interrupt
	            mhltxwr(0x08, 0x0F);    //(_MaskRxPktDoneInt<<2)+_MaskTxPktDoneInt);
	            mhltxwr(0x09, 0x50);   //(_MaskDDCDoneInt<<6)+(_MaskDDCDoneInt<<4)+(_MaskMSCDoneInt<<2)+_MaskMSCDoneInt);

	            hdmitxset(0x09, 0x20, 0x00);    // Enable DDC NACK Interrupt
	            mhltxset(0x0F, 0x00, 0x00);
	            hdmitxset(0x04, 0x1D, 0x01);       

	            mhltxset(0x0F, 0x11, 0x00);    // Switch back to MHL and enable FSM
	            
	            it6811->RxSen = FALSE;
	            it6811->devcapRdy = FALSE;
	            
	            HDMITX_MHL_DEBUG_PRINTF("it6811->Mhl_state => MHL_CBUSDiscover \n");
	            break;

	        case MHL_CBUSDisDone:
	            HDMITX_MHL_DEBUG_PRINTF("it6811->Mhl_state => MHL_CBUSDisDone \n");
	            break;

	        default :
	            break;
	    }
   	}
}

static void set_aud_fmt( struct it6811_dev_data *it6811 )
{
    int i, audsrc, infoca = 0;
    unsigned int audio_n_value = 0;
    unsigned char chksum;

    hdmitxset(0xE1, 0x1F, 0x01); // Config I2C Mode

#if(_AUDIO_I2S)
        hdmitxwr(0xE2, 0xE4);
#else
        hdmitxwr(0xE2, 0x00);
        hdmitxset(0x58, 0x80, 0x00);
#endif

    hdmitxset(0xE3, 0x10,0x10);      // user define channel status

    switch (it6811->AudType) {
		case HBR:
			hdmitxwr(0xE5, 0x08);
			break;

		case DSD:
			hdmitxwr(0xE5, 0x02);
			break;

		default:
			hdmitxwr(0xE5, 0x00);
			break;
    }

    chgbank(1);

    switch (it6811->AudFmt) {
        case AUD32K :   // 4096 = 0x1000
             audio_n_value = 0x1000;
             break;

        case AUD44K :   // 6272 = 0x1880
             audio_n_value = 0x1880;
             break;

        case AUD48K :   // 6144 = 0x1800
             audio_n_value = 0x1800;
             break;

        case AUD88K :   // 12544 = 0x3100
             audio_n_value = 0x3100;
             break;

        case AUD96K :   // 12288 = 0x3000
             audio_n_value = 0x3000;
             break;

        case AUD176K :   // 25088 = 0x6200
             audio_n_value = 0x6200;
             break;

        case AUD192K :   // 24576 = 0x6000
             audio_n_value = 0x6000;
             break;

        case AUD768K :   // 24576 = 0x6000
             audio_n_value = 0x6000;
             break;

    	default :
            HDMITX_DEBUG_PRINTF("IT6811-ERROR: AudFmt Error !!!\n");
            break;
    }

    HDMITX_DEBUG_PRINTF("IT6811- AUDIO N Value = %2.2X\n",audio_n_value);

    hdmitxwr(0x33, (unsigned char)(audio_n_value&0xFF));
    audio_n_value >>= 8;
    hdmitxwr(0x34, (unsigned char)(audio_n_value&0xFF));
    audio_n_value >>= 8;
    hdmitxwr(0x35, (unsigned char)(audio_n_value&0xFF));


    // Channel Status
    if( it6811->AudType==LPCM )
        hdmitxwr(0x91, 0x00);
    else
        hdmitxwr(0x91, 0x04);

    hdmitxwr(0x92, 0x00);
    hdmitxwr(0x93, 0x00);
    hdmitxwr(0x94, 0x00);
    hdmitxwr(0x98, it6811->AudFmt);
    hdmitxwr(0x99, ((~(it6811->AudFmt<<4))&0xF0)  | CHTSTS_SWCODE);

    // Audio InfoFrame
    switch( it6811->AudCh ) 
    {
    case 0 : infoca = 0xFF; break;  // no audio
    case 2 : infoca = 0x00; break;
    //case 3 : infoca = 0x01; break;  // 0x01,0x02,0x04
    //case 4 : infoca = 0x03; break;  // 0x03,0x05,0x06,0x08,0x14
    //case 5 : infoca = 0x07; break;  // 0x07,0x09,0x0A,0x0C,0x15,0x16,0x18
    //case 6 : infoca = 0x0B; break;  // 0x0B,0x0D,0x0E,0x10,0x17,0x19,0x1A,0x1C
    //case 7 : infoca = 0x0F; break;  // 0x0F,0x11,0x12,0x1B,0x1D,0x1E
    //case 8 : infoca = 0x1F; break;  // 0x13,0x1F
    default : HDMITX_DEBUG_PRINTF("IT6811-ERROR: Audio Channel Number Error !!!\n");
    }

    hdmitxwr(0x68, it6811->AudCh-1);

    hdmitxwr(0x69, 0x00);

    hdmitxwr(0x6A, 0x00);
    hdmitxwr(0x6B, infoca);
    hdmitxwr(0x6C, 0x00);

    chksum = 0x84;
    chksum += 0x01;
    chksum += 0x0A;
    for(i=0x68; i<0x6D; i++)
        chksum += hdmitxrd(i);

    hdmitxwr(0x6D, 0x100-chksum);


    chgbank(0);

    switch( infoca ) 
    {
    case 0x00 : audsrc = 0x01; break;

    default   : audsrc = 0x00; break; // no audio
    }

    //NO HBR support
    //if( IT6811dev[DevNum].AudType==HBR && IT6811dev[DevNum].AudSel==I2S )
    //    hdmitxset(0xE0, 0x1F, 0x0F);
    //else
    #if(_AUDIO_I2S)
    hdmitxset(0xE0, 0x1F, 0x01); //enable I2S 0
    #else
    hdmitxset(0xE0, 0x1F, 0x11);
    #endif

	   hdmitxset(0xE0, 0xc0, SUPPORT_AUDI_AudSWL); //Tranmin for set the input I2S Sample length
	   hdmitxset(0xE1, 0x07, I2S_FORMAT); // // Tranmin Config I2S Mode again as user define,the I2S format & Justified & delay settings
}

static void aud_chg( struct it6811_dev_data *it6811 , int audio_on )
{
    if((audio_on == 1)&&(it6811->Hdmi_video_state == HDMI_Video_ON)) {
        HDMITX_DEBUG_PRINTF("IT6811-AudState change to ON state\n");
        hdmitxset(0x04, 0x14, 0x14);  // Audio Clock Domain Reset
        set_aud_fmt(it6811);
        hdmitxset(0x0F, 0x20, 0x00);  // Enable IACLK
        hdmitxset(0x04, 0x14, 0x04);  // Release Audio Clock Domain Reset
        hdmitxset(0x04, 0x04, 0x00);  // Release Synchronous Audio Reset
        hdmitxwr(0xEE, 0x03);         // Clear Audio Decode Error and No Audio Interrupt
		hdmitxset(0x09, 0x80, 0x80);  // Disable Audio FIFO OverFlow Interrupt
        hdmitxset(0x0B, 0x20, 0x20);  // Disable Audio CTS Error Interrupt
        hdmitxset(0xEC, 0x03, 0x03);  // Disable Audio Decode Error and No Audio Interrupt
        hdmitxwr(0xCE, 0x03);         // Enable Audio InfoFrame
    } else{
        HDMITX_DEBUG_PRINTF("IT6811-AudState change to Reset state\n");
        hdmitxset(0x04, 0x14, 0x14);  // Audio Clock Domain Reset
        hdmitxwr(0xCE, 0x00);         // Disable Audio InfoFrame
        hdmitxwr(0xEE, 0x03);         // Clear Audio Decode Error and No Audio Interrupt
		hdmitxset(0xEC, 0x03, 0x03);  // Disable Audio Decode Error and No Audio Interrupt
		hdmitxset(0x09, 0x80, 0x80);  // Disable Audio FIFO OverFlow Interrupt
        hdmitxset(0x0B, 0x20, 0x20);  // Disable Audio CTS Error Interrupt
    }
}

void hdmitx_SetCSCScale(struct it6811_dev_data *it6811)
{
    unsigned char csc = B_HDMITX_CSC_BYPASS ;
    
    #ifndef DISABLE_HDMITX_CSC
    unsigned char i, colorflag ;
    const unsigned char* ptCsc = bCSCMtx_RGB2YUV_ITU709_0_255;
    #endif

    HDMITX_DEBUG_PRINTF("IT6811-hdmitx_SetCSCScale()\n");

    switch(it6811->InColorMode)
    {
#ifdef SUPPORT_INPUTYUV444
    case F_MODE_YUV444:
        HDMITX_DEBUG_PRINTF("IT6811-Input mode is YUV444 ");
        switch(it6811->OutColorMode)
        {
            case F_MODE_YUV444:
                HDMITX_DEBUG_PRINTF("IT6811-Output mode is YUV444\n");
                csc = B_HDMITX_CSC_BYPASS ;
            	break ;

            case F_MODE_YUV422:
                HDMITX_DEBUG_PRINTF("IT6811-Output mode is YUV422\n");
                csc = B_HDMITX_CSC_BYPASS ;
            	break ;

            case F_MODE_RGB444:
                HDMITX_DEBUG_PRINTF("IT6811-Output mode is RGB24\n");
                csc = B_HDMITX_CSC_YUV2RGB ;
            	break ;
        }
        break ;
#endif

#ifdef SUPPORT_INPUTYUV422
    case F_MODE_YUV422:
        HDMITX_DEBUG_PRINTF("IT6811-Input mode is YUV422\n");
        switch(it6811->OutColorMode)
        {
            case F_MODE_YUV444:
                HDMITX_DEBUG_PRINTF("IT6811-Output mode is YUV444\n");
                csc = B_HDMITX_CSC_BYPASS ;
            break ;
            case F_MODE_YUV422:
                HDMITX_DEBUG_PRINTF("IT6811-Output mode is YUV422\n");
                csc = B_HDMITX_CSC_BYPASS ;
            break ;

            case F_MODE_RGB444:

                HDMITX_DEBUG_PRINTF("IT6811-Output mode is RGB24\n");

                csc = B_HDMITX_CSC_YUV2RGB ;

            break ;
        }
    	break ;
#endif

#ifdef SUPPORT_INPUTRGB
    case F_MODE_RGB444:
    	HDMITX_DEBUG_PRINTF("IT6811-Input mode is RGB24\n");
        switch(it6811->OutColorMode) {
            case F_MODE_YUV444:
                HDMITX_DEBUG_PRINTF("IT6811-Output mode is YUV444\n");
                csc = B_HDMITX_CSC_RGB2YUV ;
            	break ;

            case F_MODE_YUV422:
                HDMITX_DEBUG_PRINTF("IT6811-Output mode is YUV422\n");
                csc = B_HDMITX_CSC_RGB2YUV ;
            	break ;

            case F_MODE_RGB444:
                HDMITX_DEBUG_PRINTF("IT6811-Output mode is RGB24\n");
                csc = B_HDMITX_CSC_BYPASS ;
            	break ;
        }
		break ;
#endif
    }

#ifndef DISABLE_HDMITX_CSC
    colorflag = 0;
    if(it6811->DynRange == DynCEA) {
        colorflag |= F_VIDMODE_16_235;
    }

    if(it6811->YCbCrCoef == ITU709) {
        colorflag |= F_VIDMODE_ITU709;
    }

    switch(csc) {
#ifdef SUPPORT_INPUTRGB
        case B_HDMITX_CSC_RGB2YUV:
            HDMITX_DEBUG_PRINTF("IT6811-CSC = RGB2YUV %x ",csc);
            switch(colorflag&(F_VIDMODE_ITU709|F_VIDMODE_16_235))
            {
                case F_VIDMODE_ITU709|F_VIDMODE_16_235:
                    HDMITX_DEBUG_PRINTF("IT6811-ITU709 16-235 ");
                    ptCsc = bCSCMtx_RGB2YUV_ITU709_16_235;
                	break ;
                case F_VIDMODE_ITU709|F_VIDMODE_0_255:
                    HDMITX_DEBUG_PRINTF("IT6811-ITU709 0-255 ");
                    ptCsc = bCSCMtx_RGB2YUV_ITU709_0_255;
                	break ;
                case F_VIDMODE_ITU601|F_VIDMODE_16_235:
                    HDMITX_DEBUG_PRINTF("IT6811-ITU601 16-235 ");
                    ptCsc = bCSCMtx_RGB2YUV_ITU601_16_235;
                	break ;
                case F_VIDMODE_ITU601|F_VIDMODE_0_255:
                default:
                    HDMITX_DEBUG_PRINTF("IT6811-ITU709 0-255 ");
                    ptCsc = bCSCMtx_RGB2YUV_ITU709_0_255;
					break ;
            }
#endif

#ifdef SUPPORT_INPUTYUV
        case B_HDMITX_CSC_YUV2RGB:
            HDMITX_DEBUG_PRINTF("IT6811-CSC = YUV2RGB %x ",csc);
            switch(bInputMode&(F_VIDMODE_ITU709|F_VIDMODE_16_235)) {
                case F_VIDMODE_ITU709|F_VIDMODE_16_235:
                    HDMITX_DEBUG_PRINTF("IT6811-ITU709 16-235 ");
                    ptCsc = bCSCMtx_YUV2RGB_ITU709_16_235;
                	break;
                case F_VIDMODE_ITU709|F_VIDMODE_0_255:
                    HDMITX_DEBUG_PRINTF("it6811-ITU709 0-255 ");
                    ptCsc = bCSCMtx_YUV2RGB_ITU709_0_255;
                	break;
                case F_VIDMODE_ITU601|F_VIDMODE_16_235:
                    HDMITX_DEBUG_PRINTF("IT6811-ITU601 16-235 ");
                    ptCsc = bCSCMtx_YUV2RGB_ITU601_16_235;
                	break;
                case F_VIDMODE_ITU601|F_VIDMODE_0_255:
                default:
                    HDMITX_DEBUG_PRINTF("IT6811-ITU709 16-235 ");
                    ptCsc = bCSCMtx_YUV2RGB_ITU709_16_235;
                	break;
            }
        	break;
#endif

        default :
            //csc = B_HDMITX_CSC_BYPASS;
        	break;
    }
    
    HDMITX_DEBUG_PRINTF("csc = %d \n",csc);
    if(csc != B_HDMITX_CSC_BYPASS) {
        for( i = 0 ; i < SIZEOF_CSCMTX ; i++ ){
            hdmitxwr(0x73+i, ptCsc[i]) ; 
            //HDMITX_DEBUG_PRINTF("IT6811-reg%02X <- %02X\n",(int)(i+0x73),(int)bCSCMtx_YUV2RGB_ITU601_0_255[i]);
        }
    }
    
#else// DISABLE_HDMITX_CSC
    csc = B_HDMITX_CSC_BYPASS ;
#endif// DISABLE_HDMITX_CSC

    if( csc == B_HDMITX_CSC_BYPASS ) {
        hdmitxset(0xF, 0x10, 0x10);
    } else {
        hdmitxset(0xF, 0x10, 0x00);
    }

#if(_EnColorClip)       
    if( it6811->DynRange == DynCEA)  //DynCEA Enable EnColorClip
    csc |= 0x08;
#endif

    hdmitxset(0x72, 0x0F, csc);
}
static void hdmitx_set_avi_infoframe(struct it6811_dev_data *it6811)
{
    int i,checksum;
    
    struct AVI_InfoFrame avi_data;
    
    memset(&avi_data, 0, sizeof(struct AVI_InfoFrame));
    avi_data.AVI_HB[0] = AVI_INFOFRAME_TYPE;
    avi_data.AVI_HB[1] = AVI_INFOFRAME_VER;
    avi_data.AVI_HB[2] = AVI_INFOFRAME_LEN; 
    avi_data.AVI_DB[0] &= 0x9F;  //clear color
    avi_data.AVI_DB[0] |=   (it6811->OutColorMode&0x03)<<5;
    avi_data.AVI_DB[1] = video_data.outputcolorimetryAspectRatio;
    avi_data.AVI_DB[3] = video_data.outputVideoCode;

    if( it6811->DynRange==DynVESA )
        avi_data.AVI_DB[4] =  0x40|(it6811->PixRpt-1);
    else
        avi_data.AVI_DB[4] =  it6811->PixRpt-1;
    
    chgbank(1);

    hdmitxwr(0x58,avi_data.AVI_DB[0]);
    hdmitxwr(0x59,avi_data.AVI_DB[1]);
    hdmitxwr(0x5A,avi_data.AVI_DB[2]);
    hdmitxwr(0x5B,avi_data.AVI_DB[3]);
    hdmitxwr(0x5C,avi_data.AVI_DB[4]);
    hdmitxwr(0x5E,avi_data.AVI_DB[5]);
    hdmitxwr(0x5F,avi_data.AVI_DB[6]);
    hdmitxwr(0x60,avi_data.AVI_DB[7]);
    hdmitxwr(0x61,avi_data.AVI_DB[8]);
    hdmitxwr(0x62,avi_data.AVI_DB[9]);
    hdmitxwr(0x63,avi_data.AVI_DB[10]);
    hdmitxwr(0x64,avi_data.AVI_DB[11]);
    hdmitxwr(0x65,avi_data.AVI_DB[12]);

    for(i = 0,checksum = 0; i < 13 ; i++) {
        checksum -= avi_data.AVI_DB[i] ;
    }
    checksum -= AVI_INFOFRAME_VER+AVI_INFOFRAME_TYPE+AVI_INFOFRAME_LEN ;
    
    hdmitxwr(0x5D,checksum);
    chgbank(0);
    hdmitxwr(0xCD, 0x03);           // Enable AVI InfoFrame 
}

void set_vid_fmt(struct it6811_dev_data *it6811 )
{
    int packet_mode ;
    unsigned long Pclk;
    long VCLK;
    long HCLK;

    //
    //NOTE: Packet pixel mode only YUV422
    //
    Pclk = cal_pclk();
	HDMITX_DEBUG_PRINTF("IT6811- !!!! PACKET PIXEL MODE .............Pclk is %d,it6811->PixRpt %d\n",(int)Pclk, (int)(it6811->PixRpt));
    //VCLK = Pclk*it6811->PixRpt*1;
    VCLK = Pclk; //*it6811->PixRpt*1;//Tranmin 20130427
    packet_mode = FALSE;
    if (it6811->EnPackPix == 2) {   //FORCE packet pixel mode
        it6811->OutColorMode = YCbCr422;
        packet_mode = TRUE;
    } else {
        if (it6811->EnPackPix == 1) { //AUTO packet pixel mode
            if (Pclk > 80000L) {
                it6811->OutColorMode = YCbCr422;
                packet_mode = TRUE;         
            }
        }   
    }
        
    hdmitxset(0x70, 0xC0, (it6811->InColorMode<<6));  // Input Video Format
    //hdmitxset(0xC0, 0x01, it6811->EnHDMI);
    hdmitxset(0xC0, 0x01, 0x01);//force hdmi mode
    hdmitxset(0xc1, 0x70, 0);  //IT6811 24bit color mode only
    hdmitx_SetCSCScale(it6811);
    hdmitx_set_avi_infoframe(it6811);  //use default infoframe table.
    
    if (packet_mode == TRUE) {
        HCLK = VCLK*2;
        mhltxset(0x0F, 0x08,0x08);
        mhltxset(0x0C, 0x06, 0x02);  //Trigger Link mode packet
        HDMITX_DEBUG_PRINTF("IT6811- !!!! PACKET PIXEL MODE .............\n");
    } else {
        HCLK = VCLK*3;
        mhltxset(0x0F, 0x08,0x00);
        mhltxset(0x0C, 0x06, 0x02);  //Trigger Link mode packet
        HDMITX_DEBUG_PRINTF("IT6811- !!!! NORMAL PIXEL MODE .............\n");
    }
    
    setup_mhltxafe(it6811,HCLK);    
}

static int Hdmi_Video_state(struct it6811_dev_data *it6811 , HDMI_Video_state state)
{
    if(it6811->Hdmi_video_state == state) {
		return -1;
    } else {
        if ((hdmitxrd(0x0E) & 0x60) != 0x60) {
            state = HDMI_Video_REST;
        }
        
        it6811->Hdmi_video_state = state;

        if (state != HDMI_Video_ON) {
#ifdef _SUPPORT_HDCP_
            Hdmi_HDCP_state(it6811, HDCP_Off);
#endif
            aud_chg(it6811,0);
        }

        switch(state) {
	        case HDMI_Video_REST:
	            HDMITX_DEBUG_PRINTF("Hdmi_Video_state ==> HDMI_Video_REST");
	            hdmitxset(0x04, 0x08, 0x08);  // Video Clock Domain Reset
	            hdmitxset(0x0A, 0x40, 0x40);  // Enable Video UnStable Interrupt
	            hdmitxset(0x0B, 0x08, 0x08);  // Enable Video Stable Interrupt
	            hdmitxset(0xEC, 0x04, 0x04);  // Enable Video Input FIFO auto-reset Interrupt
	            hdmitxset(0xEC, 0x84, 0x84);  // Enable Video In/Out FIFO Interrupt
	            hdmitxset(0xC1, 0x01, 0x01);  // Set AVMute
	            break;

	        case HDMI_Video_WAIT:
	            HDMITX_DEBUG_PRINTF("Hdmi_Video_state ==> HDMI_Video_WAIT");
	            hdmitxset(0x04, 0x08, 0x08);  // Video Clock Domain Reset
	            hdmitxset(0x0A, 0x40, 0x00);  // Enable Video UnStable Interrupt
	            hdmitxset(0x0B, 0x08, 0x00);  // Enable Video Stable Interrupt
	            hdmitxset(0xEC, 0x04, 0x00);  // Enable Video Input FIFO auto-reset Interrupt
	            hdmitxset(0xEC, 0x84, 0x84);  // Enable Video In/Out FIFO Interrupt
	            hdmitxset(0xC1, 0x01, 0x01);  // Set AVMute
	            hdmitxset(0x04, 0x08, 0x00);  // Release Video Clock Domain Reset   
	            break;

	        case HDMI_Video_ON:
	            HDMITX_DEBUG_PRINTF("Hdmi_Video_state ==> HDMI_Video_ON");
	            set_vid_fmt(it6811);
	            //setup_mhltxafe(it6811,cal_pclk());  // setup TX AFE here if PCLK is unknown 
	            //setup_mhltxafe(74250L);         //720P 74.25M 
	            hdmitxwr(0xEE, 0x84);           // Clear Video In/Out FIFO Interrupt
	            hdmitxset(0xEC, 0x84, 0x00);    // Enable Video In/Out FIFO Interrupt
	            hdmitxset(0xC6, 0x03, 0x03);    // Enable General Control Packet

	            fire_afe(TRUE);
	            
#if _SUPPORT_AUDIO_
	            aud_chg(it6811,1);
#endif
	            
#ifdef _SUPPORT_HDCP_
	            Hdmi_HDCP_state(it6811,HDCP_CPStart);
#else
	            hdmitxset(0xC1, 0x01, 0x00);   // clear AVMute
#endif
	            break;

	        default:
	            break;
        }
        return 0;
    }
}

static int ddcwait( void )
{
    int cbuswaitcnt;
    unsigned char Reg06;
    unsigned char MHL05;
	unsigned char reg16;

     cbuswaitcnt = 0;
     do {
        cbuswaitcnt++;   
        delay1ms(CBUSWAITTIME);
		reg16 = mhltxrd(0x16);
     } while (((mhltxrd(0x1C) & 0x01) == 0x01) && (cbuswaitcnt < CBUSWAITNUM) && (((reg16 & 0x80) == 0) || ((reg16 & 0x38) != 0)));

     Reg06 = hdmitxrd(0x06);
     MHL05 = mhltxrd(0x05);

    if (cbuswaitcnt == CBUSWAITNUM || (Reg06 & 0x20) || (MHL05 & 0x20) || (reg16 & 0x38) )
        return FAIL;
    else
        return SUCCESS;
}

static int ddcfire( int offset, int wdata )
{
    int ddcreqsts, retrycnt;

    retrycnt = 0;
    hdmitxwr((unsigned char )offset, (unsigned char)wdata);

    ddcreqsts = ddcwait();

    if (ddcreqsts != SUCCESS) {
        HDMITX_MHL_DEBUG_PRINTF("IT6811-ERROR: DDC Request Maximum Retry FAil !!!\n");
        return FAIL;
    } else {
        return SUCCESS;
    }
}

#ifdef _SUPPORT_HDCP_
static int hdcprd( int offset, int bytenum )
{
    int err =0;
    
    hdmitxset(0x10, 0x01, 0x01);    // Enable PC DDC Mode
    hdmitxwr(0x15, 0x09);           // DDC FIFO Clear
    hdmitxwr(0x11, 0x74);           // HDCP Address
    hdmitxwr(0x12, (unsigned char)offset);         // HDCP Offset
    hdmitxwr(0x13, (unsigned char)bytenum);        // Read ByteNum

    if (ddcfire(0x15, 0x00) == FAIL) {
        HDMITX_DEBUG_HDCP_PRINTF("IT6811-ERROR: DDC HDCP Read Fail !!!\n");
        err = FAIL;
    }
    hdmitxset(0x10, 0x01, 0x00);    // Disable PC DDC Mode
    return err;
}


static int hdmtx_enhdcp( struct it6811_dev_data *it6811)
{
    int RxHDMIMode;
    int WaitCnt;
    unsigned char BKSV[5];
    int ret;

    // Reset HDCP Module
    hdmitxset(0x04, 0x01, 0x01);
    hdmitxset(0x20, 0x01, 0x00);    // Disable CP_Desired
    delay1ms(1);
    hdmitxset(0x04, 0x01, 0x00);
    // set HDCP Option
    hdmitxwr(0xF8, 0xC3);
    hdmitxwr(0xF8, 0xA5);
    hdmitxset(0x20, 0x80, 0x80);    // RegEnSiPROM='1'
    hdmitxset(0x37, 0x01, 0x00);    // PWR_DOWN='0'
    hdmitxset(0x20, 0x80, 0x00);    // RegEnSiPROM='0'
    hdmitxset(0xC4, 0x01, 0x00);    // EncDis =0;
    hdmitxset(0x4B, 0x30, 0x30);    //(EnRiCombRead<<5)+(EnR0CombRead<<4));
    hdmitxwr(0xF8, 0xFF);
    hdmitxset(0x20, 0x06, 0x00);    //(EnSyncDetChk<<2)+(EnHDCP1p1<<1));
    hdmitxset(0x4F, 0x0F, 0x08);  //(_EnHDCPAutoMute<<3)+(_EnSyncDet2FailInt<<2)+(_EnRiChk2DoneInt<<1)+_EnAutoReAuth);
    hdmitxwr(0x07, 0x03);         // also clear previous Authentication Done Interrupt
    hdmitxset(0x0A, 0x07, 0x00);  // Enable Authentication Fail/Done/KSVListChk Interrupt
    hdmitxwr(0xEE, 0x30);          // Clear Ri/Pj Done Interrupt
    hdmitxset(0xEC, 0x30, 0x30);   //EnRiPjInt ,Enable Ri/Pj Done Interrupt
    hdmitxset(0xED, 0x01, 0x01);   //EnSyncDetChk Disable Sync Detect Fail Interrupt
    hdmitxset(0x1F, 0x01, 0x01);  // Enable An Generator
    delay1ms(1);
    hdmitxset(0x1F, 0x01, 0x00);  // Stop An Generator
    hdmitxwr(0x28, hdmitxrd(0x30));
    hdmitxwr(0x29, hdmitxrd(0x31));
    hdmitxwr(0x2A, hdmitxrd(0x32));
    hdmitxwr(0x2B, hdmitxrd(0x33));
    hdmitxwr(0x2C, hdmitxrd(0x34));
    hdmitxwr(0x2D, hdmitxrd(0x35));
    hdmitxwr(0x2E, hdmitxrd(0x36));
    hdmitxwr(0x2F, hdmitxrd(0x37));
    
    hdmitxset(0x20, 0x01, 0x01);  // Enable CP_Desired

    if(it6811->ver == 0) {
        hdmitxset(0x1B, 0x80, 0x80);
    
        ret = hdcprd(0x42, 1);
        if(ret<0) 
            goto hdcp_ena_err_return;
    
        RxHDMIMode = (hdmitxrd(0x17)&0x10)>>4;
        hdmitxset(0x1B, 0x80, 0x00);
    } else {
	    ret = hdcprd(0x41, 2);    // BStatus;
	    if(ret<0) 
	        goto hdcp_ena_err_return;
	    
	    RxHDMIMode = (hdmitxrd(0x45)&0x10)>>4;
    }

    HDMITX_DEBUG_HDCP_PRINTF("IT6811-Enable HDCP Fire %d => Current RX HDMI MODE=%d \n",it6811->HDCPFireCnt, RxHDMIMode);

    WaitCnt = 0;

    while ((RxHDMIMode != 0x01)) {
        delay1ms(10);

        if(it6811->ver ==0) {
            hdmitxset(0x1B, 0x80, 0x80);
        
            ret = hdcprd(0x42, 1);
            if(ret<0) 
                goto hdcp_ena_err_return;
        
            RxHDMIMode = (hdmitxrd(0x17)&0x10)>>4;
        
            hdmitxset(0x1B, 0x80, 0x00);
        } else {
            ret = hdcprd(0x41, 2);    // BStatus;
            if(ret<0) 
                goto hdcp_ena_err_return;
            RxHDMIMode = (hdmitxrd(0x45)&0x10)>>4;
        }

        HDMITX_DEBUG_HDCP_PRINTF("IT6811-Waiting for RX HDMI_MODE change to 1 ...\n");

        if (WaitCnt++ == 9) {
            HDMITX_DEBUG_HDCP_PRINTF("IT6811-\nERROR: RX HDMI_MODE keeps in 0 Time-Out !!!\n\n");
            //IT6811dev[DevNum].Hdcp_state = HDCP_CPRetry;
            goto hdcp_ena_err_return;
            return -1;
        }
    }

    // add for HDCP ATC Test
    hdmitxset(0x1B, 0x80, 0x80);
    ret = hdcprd(0x00, 5);
    if(ret<0) goto hdcp_ena_err_return;
    
    //hdmitxbrd(0x17, 5, &BKSV[0]);
    BKSV[0] = hdmitxrd(0x17);
    BKSV[1] = hdmitxrd(0x17);
    BKSV[2] = hdmitxrd(0x17);
    BKSV[3] = hdmitxrd(0x17);
    BKSV[4] = hdmitxrd(0x17);

    hdmitxset(0x1B, 0x80, 0x00);

    HDMITX_DEBUG_HDCP_PRINTF("IT6811-BKSV = 0x%X%X%X%X%X%X%X%X%X%X \n", (int)(BKSV[4]>>4), (int)(BKSV[4]&0x0F),
                                               (int)(BKSV[3]>>4), (int)(BKSV[3]&0x0F),
                                               (int)(BKSV[2]>>4), (int)(BKSV[2]&0x0F),
                                               (int)(BKSV[1]>>4), (int)(BKSV[1]&0x0F),
                                               (int)(BKSV[0]>>4), (int)(BKSV[0]&0x0F));

    if (BKSV[0]==0x23 && BKSV[1]==0xDE && BKSV[2]==0x5C && BKSV[3]==0x43 && BKSV[4]==0x93) {
        HDMITX_DEBUG_HDCP_PRINTF("IT6811-The BKSV is in revocation list for ATC test !!!\n");
        HDMITX_DEBUG_HDCP_PRINTF("IT6811-Abort HDCP Authentication !!!\n");
        goto hdcp_ena_err_return;
    }

    // Clear Previous HDCP Done/Fail Interrupt when EnRiChk2DoneInt=TRUE
    hdmitxwr(0x07, 0x03);
    hdmitxset(0x21, 0x01, 0x01);  // HDCP Fire

    return 0;
    
hdcp_ena_err_return:
    return -1;
    
}

static int Hdmi_HDCP_state(struct it6811_dev_data *it6811 , HDCPSts_Type state)
{
    if(it6811->HDCPEnable == FALSE) {
        state =  HDCP_Off;
    }
    
    switch(state) {  
        case HDCP_CPStart:
            it6811->HDCPFireCnt++;
            break;

		case HDCP_Off:				
        case HDCP_CPGoing:      
        case HDCP_CPDone:
        case HDCP_CPFail:
        default:
            break;
    }

    if (it6811->Hdcp_state != state) {
        it6811->Hdcp_state = state;
        switch (state) {
            case HDCP_Off:  
                HDMITX_DEBUG_HDCP_PRINTF( "it6811->Hdcp_state -->HDCP_Off \n");
                hdmitxset(0x20, 0x01, 0x00);   // Disable CP_Desired
                hdmitxset(0x04, 0x01, 0x01);
                hdmitxset(0x0A, 0x07, 0x07);   // disable Authentication Fail/Done/KSVListChk Interrupt
                hdmitxwr(0xEE, 0x30);          // Clear Ri/Pj Done Interrupt    
                hdmitxwr(0x07, 0x03);          // also clear previous Authentication Done Interrupt             
                hdmitxset(0xEC, 0x30, 0x30);   //EnRiPjInt ,Enable Ri/Pj Done Interrupt
                hdmitxset(0xED, 0x01, 0x01);   //EnSyncDetChk Disable Sync Detect Fail Interrupt
                it6811->HDCPFireCnt = 0;
                break;
                
            case HDCP_CPStart:
                HDMITX_DEBUG_HDCP_PRINTF("it6811->Hdcp_state -->HDCP_CPStart \n");
                break;

            case HDCP_CPGoing:
                HDMITX_DEBUG_HDCP_PRINTF("it6811->Hdcp_state -->HDCP_CPGoing \n");
                break;

            case HDCP_CPDone:
                hdmitxset(0xC1, 0x01, 0x00);   // clear AVMute
                HDMITX_DEBUG_HDCP_PRINTF("it6811->Hdcp_state -->HDCP_CPDone \n");
                it6811->HDCPFireCnt = 0;
                break;

            case HDCP_CPFail:               
                hdmitxset(0x20, 0x01, 0x00);   // Disable CPDesired
                hdmitxset(0xC2, 0x40, 0x40);   // Black Screen
                break;

            default:
                break;
        }
    }
    return 0;
}


static int Hdmi_HDCP_handler(struct it6811_dev_data *it6811 )
{
    int ret = 0;
    switch(it6811->Hdcp_state){
        case HDCP_CPStart:
            ret = hdmtx_enhdcp(it6811);
            if(ret <0)
                Hdmi_HDCP_state(it6811,HDCP_CPStart);   
            else
                Hdmi_HDCP_state(it6811,HDCP_CPGoing);
            break;

        case HDCP_CPGoing:
		case HDCP_Off:	
        case HDCP_CPDone:
        case HDCP_CPFail:
        default:
            break;
    }
    return ret;
    
}

#ifdef _SHOW_HDCP_INFO_
static void hdcpsts( void ) 
{
    unsigned char An1, An2, An3, An4, An5, An6, An7, An8;
    unsigned char AKSV1, AKSV2, AKSV3, AKSV4, AKSV5;
    unsigned char BKSV1, BKSV2, BKSV3, BKSV4, BKSV5;
    unsigned char ARi1, ARi2, BRi1, BRi2;
    unsigned char BCaps, BStatus1, BStatus2;
    unsigned char AuthStatus;

    BKSV1      = hdmitxrd(0x3B);
    BKSV2      = hdmitxrd(0x3C);
    BKSV3      = hdmitxrd(0x3D);
    BKSV4      = hdmitxrd(0x3E);
    BKSV5      = hdmitxrd(0x3F);
    BRi1       = hdmitxrd(0x40);
    BRi2       = hdmitxrd(0x41);
    AKSV1      = hdmitxrd(0x23);
    AKSV2      = hdmitxrd(0x24);
    AKSV3      = hdmitxrd(0x25);
    AKSV4      = hdmitxrd(0x26);
    AKSV5      = hdmitxrd(0x27);
    An1        = hdmitxrd(0x28);
    An2        = hdmitxrd(0x29);
    An3        = hdmitxrd(0x2A);
    An4        = hdmitxrd(0x2B);
    An5        = hdmitxrd(0x2C);
    An6        = hdmitxrd(0x2D);
    An7        = hdmitxrd(0x2E);
    An8        = hdmitxrd(0x2F);
    ARi1       = hdmitxrd(0x38);
    ARi2       = hdmitxrd(0x39);
    AuthStatus = hdmitxrd(0x46);
    BCaps      = hdmitxrd(0x43);
    BStatus1   = hdmitxrd(0x44);
    BStatus2   = hdmitxrd(0x45);

    HDMITX_DEBUG_HDCP_PRINTF("IT6811-An   = 0x%X%X%X%X%X%X%X%X%X%X%X%X%X%X%X%X \n",(int)An8>>4, (int)An8&0x0F,
                                                           (int)An7>>4, (int)An7&0x0F,
                                                           (int)An6>>4, (int)An6&0x0F,
                                                           (int)An5>>4, (int)An5&0x0F,
                                                           (int)An4>>4, (int)An4&0x0F,
                                                           (int)An3>>4, (int)An3&0x0F,
                                                           (int)An2>>4, (int)An2&0x0F,
                                                           (int)An1>>4, (int)An1&0x0F);

    HDMITX_DEBUG_HDCP_PRINTF("IT6811-AKSV = 0x%X%X%X%X%X%X%X%X%X%X \n", (int)AKSV5>>4, (int)AKSV5&0x0F,
                                               (int)AKSV4>>4, (int)AKSV4&0x0F,
                                               (int)AKSV3>>4, (int)AKSV3&0x0F,
                                               (int)AKSV2>>4, (int)AKSV2&0x0F,
                                               (int)AKSV1>>4, (int)AKSV1&0x0F);
    HDMITX_DEBUG_HDCP_PRINTF("IT6811-BKSV = 0x%X%X%X%X%X%X%X%X%X%X \n", (int)BKSV5>>4, (int)BKSV5&0x0F,
                                               (int)BKSV4>>4, (int)BKSV4&0x0F,
                                               (int)BKSV3>>4, (int)BKSV3&0x0F,
                                               (int)BKSV2>>4, (int)BKSV2&0x0F,
                                               (int)BKSV1>>4, (int)BKSV1&0x0F);

    HDMITX_DEBUG_HDCP_PRINTF("IT6811-AR0 = 0x%X%X%X%X \n", (int)ARi2>>4, (int)ARi2&0x0F, (int)ARi1>>4, (int)ARi1&0x0F);
    HDMITX_DEBUG_HDCP_PRINTF("IT6811-BR0 = 0x%X%X%X%X \n", (int)BRi2>>4, (int)BRi2&0x0F, (int)BRi1>>4, (int)BRi1&0x0));
    HDMITX_DEBUG_HDCP_PRINTF("IT6811-Rx HDCP Fast Reauthentication = %d \n", BCaps&0x01);
    HDMITX_DEBUG_HDCP_PRINTF("IT6811-Rx HDCP 1.1 Features = %d ", (int)(BCaps&0x02)>>1);

    if ( (BCaps&0x02) /*&& EnHDCP1p1*/ )
        HDMITX_DEBUG_HDCP_PRINTF("IT6811-Enabled\n");
    else
        HDMITX_DEBUG_HDCP_PRINTF("IT6811-Disabled\n");

    if( BCaps&0x10 )
        HDMITX_DEBUG_HDCP_PRINTF("IT6811-Rx HDCP Maximum DDC Speed = 400KHz\n");
    else
        HDMITX_DEBUG_HDCP_PRINTF("IT6811-Rx HDCP Maximum DDC Speed = 100KHz\n");

    HDMITX_DEBUG_HDCP_PRINTF("IT6811-Rx HDCP Repeater = %d \n", (int)(BCaps&0x40)>>6);
    HDMITX_DEBUG_HDCP_PRINTF("IT6811-Tx Authentication Status = 0x%X \n", (int)AuthStatus);

    if( (AuthStatus&0x80)!=0x80 ) {
        if( AuthStatus&0x01 )
            HDMITX_DEBUG_HDCP_PRINTF("IT6811-Auth Fail: DDC NAck too may times !!!\n");
        if( AuthStatus&0x02 )
            HDMITX_DEBUG_HDCP_PRINTF("IT6811-Auth Fail: BKSV Invalid !!!\n");
        if( AuthStatus&0x04 )
            HDMITX_DEBUG_HDCP_PRINTF("IT6811-Auth Fail: Link Check Fail (AR0/=BR0) !!!\n");
        if( AuthStatus&0x08 )
            HDMITX_DEBUG_HDCP_PRINTF("IT6811-Auth Fail: Link Integrity Ri Check Fail !!!\n");
        if( AuthStatus&0x10 )
            HDMITX_DEBUG_HDCP_PRINTF("IT6811-Auth Fail: Link Integrity Pj Check Fail !!!\n");
        if( AuthStatus&0x20 )
            HDMITX_DEBUG_HDCP_PRINTF("IT6811-Auth Fail: Link Integrity Sync Detect Fail !!!\n");
        if( AuthStatus&0x40 )
            HDMITX_DEBUG_HDCP_PRINTF("IT6811-Auth Fail: DDC Bus Hang TimeOut !!!\n");
    }

    HDMITX_DEBUG_HDCP_PRINTF("\n\n");

}

#endif

static void hdmitx_int_HDCP_AuthFail(struct it6811_dev_data *it6811)
{
}
#endif

static void hdmitx_rst( struct it6811_dev_data *it6811 )
{

	hdimtx_write_init(tIt6811init_hdmi);
	IT6811_DEBUG_PRINTF("it6811: tIt6811init_hdmi loaded\n");
	cal_oclk(it6811);
	mhltx_write_init(tIt6811init_mhl);
	IT6811_DEBUG_PRINTF("it6811: tIt6811init_mhl loaded\n");
	Mhl_state(it6811,MHL_USB);
	Hdmi_Video_state(it6811,HDMI_Video_REST);
}

static int hdmitx_ini(void)
{
	unsigned char VenID[2]={0xFF,0xFF}, DevID[2]={0xFF,0xFF};
	struct it6811_dev_data  *it6811 = get_it6811_dev_data();

	VenID[0] = hdmitxrd(0x00);
	VenID[1] = hdmitxrd(0x01);
	DevID[0] = hdmitxrd(0x02);
	DevID[1] = hdmitxrd(0x03);

	IT6811_DEBUG_PRINTF("IT6811-Current DevID=%X%X\n", (int)DevID[1], (int)DevID[0]);
	IT6811_DEBUG_PRINTF("IT6811-Current VenID=%X%X\n", (int)VenID[1], (int)VenID[0]);

	if ((VenID[0] == 0x54) && (VenID[1] == 0x49) && (DevID[0] == 0x11)) {
		if((DevID[1] & 0x0F) == 0x08) {
			IT6811_DEBUG_PRINTF("IT6811-###############################################\n");
			IT6811_DEBUG_PRINTF("IT6811-#            MHLTX Initialization             #\n");
			IT6811_DEBUG_PRINTF("IT6811-###############################################\n");

			hdmirx_Var_init(it6811);
			it6811->ver = (DevID[1]&0xF0)>>4;
			IT6811_DEBUG_PRINTF("IT6811- Ver = %d \n",(int)it6811->ver);
			hdmitx_rst(it6811);

			return 0;
		}
	} else {
		IT6811_DEBUG_PRINTF("IT6811- ERROR: Can not find IT6811A0 MHLTX Device !!!\n");
	}
     
    return -1;
}

static int mscwait( void )
{
    int cbuswaitcnt; 
    unsigned char MHL05;

     cbuswaitcnt = 0;
     do {
         cbuswaitcnt++;
         delay1ms(CBUSWAITTIME);

     } while ((mhltxrd(0x1C) & 0x02) == 0x02 && cbuswaitcnt < CBUSWAITNUM   );


     MHL05 = mhltxrd(0x05);
     if( (cbuswaitcnt==CBUSWAITNUM) || MHL05&0x02 )
     	return FAIL;
     else
         return SUCCESS;
}

static int mscfire( int offset, int wdata )
{
    int mscreqsts;

    mhltxwr((unsigned char)offset, (unsigned char)wdata);
    mscreqsts = mscwait();

    return (mscreqsts == SUCCESS) ? SUCCESS : FAIL;       
}

static void hdmitx_irq( struct it6811_dev_data *it6811 )
{
    unsigned char Reg06, Reg07, Reg08, RegEE, RegEF, RegF0;
    unsigned char MHL04, MHL05, MHL06;
    unsigned char MHLA0, MHLA1, MHLA2, MHLA3;
    unsigned char rddata;
    unsigned int temp;
    
    Reg06 = 0x00;
    Reg07 = 0x00;
    Reg08 = 0x00;
    RegEE = 0x00;   
    RegEF = 0x00;
    MHL04 = 0x00;
    MHL05 = 0x00;
    MHL06 = 0x00;
    MHLA0 = 0x00;
    MHLA1 = 0x00;
    MHLA2 = 0x00;
    MHLA3 = 0x00;       

    RegF0 = hdmitxrd(0xF0);
    if (RegF0 & 0x10) {
        IT6811_DEBUG_INT_PRINTF("IT6811-Detect U3 Wakeup Interrupt ...\n");
        hdmitxset(0xE8, 0x60, 0x00);
        Mhl_state(it6811, MHL_Cbusdet);
    }

    if ((RegF0 & 0x0F) == 0x00) {
        goto exit_hdmitx_irq;
    }

    if (RegF0 & 0x01){
        //W1C all int staus
        Reg06 = hdmitxrd(0x06);
        Reg07 = hdmitxrd(0x07);
        Reg08 = hdmitxrd(0x08);
        hdmitxwr(0x06, Reg06);
        hdmitxwr(0x07, Reg07);
        hdmitxwr(0x08, Reg08);
    }

    if (RegF0 & 0x02) {
        RegEE = hdmitxrd(0xEE);
        RegEF = hdmitxrd(0xEF);
        hdmitxwr(0xEE, RegEE);
        hdmitxwr(0xEF, RegEF);
    }

    if (RegF0 & 0x04) {
        MHL04 = mhltxrd(0x04);
        MHL05 = mhltxrd(0x05);
        MHL06 = mhltxrd(0x06);
        mhltxwr(0x04, MHL04);
        mhltxwr(0x05, MHL05);
        mhltxwr(0x06, MHL06);
    }
    
    if (RegF0 & 0x08) {
        MHLA0 = mhltxrd(0xA0);
        MHLA1 = mhltxrd(0xA1);
        MHLA2 = mhltxrd(0xA2);
        MHLA3 = mhltxrd(0xA3);
        mhltxwr(0xA0, MHLA0);
        mhltxwr(0xA1, MHLA1);
        mhltxwr(0xA2, MHLA2);
        mhltxwr(0xA3, MHLA3);
    }
    
	IT6811_DEBUG_INT_PRINTF("IT6811-hdmitx_irq debug:\n");
    IT6811_DEBUG_INT_PRINTF("reg06=%2.2X ,reg07=%2.2X ,reg08=%2.2X\n",(int)Reg06,(int)Reg07,(int)Reg08);
    IT6811_DEBUG_INT_PRINTF("regEE=%2.2X ,regEF=%2.2X, regF0 = %2.2X \n ",(int)RegEE,(int)RegEF,(int)RegF0);
    IT6811_DEBUG_INT_PRINTF("MHL04=%2.2X ,MHL05=%2.2X ,MHL06=%2.2X\n ",(int)MHL04,(int)MHL05,(int)MHL06);
    IT6811_DEBUG_INT_PRINTF("MHLA0=%2.2X ,MHLA1=%2.2X ,MHLA2=%2.2X ,MHLA3=%2.2X\n ",(int)MHLA0,(int)MHLA1,(int)MHLA2,(int)MHLA3);
    IT6811_DEBUG_INT_PRINTF("reg04=%2.2X ,reg05=%2.2X ,reg0B=%2.2X\n ",(int)hdmitxrd(0x04),(int)hdmitxrd(0x05),(int)hdmitxrd(0x0B));
    IT6811_DEBUG_INT_PRINTF("reg0E=%2.2X ,reg0F=%2.2X , \n ",(int)hdmitxrd(0x0E),(int)hdmitxrd(0x0F));
    IT6811_DEBUG_INT_PRINTF("reg70=%2.2X ,reg71=%2.2X ,reg72=%2.2X\n ",(int)hdmitxrd(0x70),(int)hdmitxrd(0x70),(int)hdmitxrd(0x72));

    if (Reg06 & 0x01) {
        IT6811_DEBUG_INT_PRINTF("IT6811-HPD Change Interrupt \n");
        
        Hdmi_Video_state(it6811, HDMI_Video_WAIT);
        
        rddata = hdmitxrd(0x0E);
        it6811->RxSen = ((unsigned int)rddata & 0x60) >> 5;
        IT6811_DEBUG_INT_PRINTF("IT6811-HPD Change Interrupt HPD/RXsen(it6811->RxSen) ===> %d/%d(%d)!! ", (int)((rddata & 0x40) >>  6), (int)((rddata & 0x20) >> 5), it6811->RxSen);

        ITE6811MhlTxNotifyDsHpdChange(rddata & 0x40);
    }

    if (Reg06 & 0x02) {
        Hdmi_Video_state(it6811, HDMI_Video_WAIT);
        it6811->RxSen = (int) (hdmitxrd(0x0E) & 0x60) >> 5;
        IT6811_DEBUG_INT_PRINTF("IT6811-RxSen Change Interrupt => RxSen = %d " ,it6811->RxSen);
    }

    if (Reg06 & 0x10) {
        IT6811_DEBUG_INT_PRINTF("IT6811-DDC FIFO Error Interrupt ...\n");
        hdmitxset(0x10, 0x01, 0x01);
        hdmitxwr(0x15, 0x09);
        hdmitxset(0x10, 0x01, 0x00);
    }

    if (Reg06 & 0x20) {
        IT6811_DEBUG_INT_PRINTF("IT6811-DDC NACK Interrupt ...\n");
        #ifdef _SUPPORT_HDCP_
        Hdmi_HDCP_state(it6811,HDCP_Off);
        #endif
    }

    if (Reg06 & 0x80) {
        IT6811_DEBUG_INT_PRINTF("IT6811-Audio FIFO Error Interrupt ...\n");
        hdmitxset(0x04, 0x04, 0x04);
        delay1ms(10);
        hdmitxset(0x04, 0x04, 0x00);
    }

 // Interrupt Reg07
    if (Reg07 & 0x01) {
        IT6811_DEBUG_INT_PRINTF("IT6811-HDCP Authentication Fail Interrupt ...\n\n");
         
        #ifdef _SUPPORT_HDCP_
        hdmitx_int_HDCP_AuthFail(it6811);
        // software work around for HDCP issue , It will remove after next version IC release  
        if ((it6811->HDCPFireCnt) && ((it6811->HDCPFireCnt % 3) == 0)) { 
            hdmitx_rst(it6811);
            Hdmi_Video_state(it6811,HDMI_Video_WAIT);
        }

        if (it6811->HDCPFireCnt > _HDCPFireMax) {

            Hdmi_HDCP_state(it6811,HDCP_CPFail);
            HDMITX_DEBUG_HDCP_INT_PRINTF(("IT6811-ERROR: Set Black Screen because of HDCPFireCnt>%d !!!\n", _HDCPFireMax));
        } else {
            Hdmi_HDCP_state(it6811,HDCP_CPStart);
        }
        #endif

    }

    if (Reg07 & 0x02) { 
        IT6811_DEBUG_INT_PRINTF("IT6811-HDCP Authentication Done Interrupt ...\n\n");
         
        #ifdef _SUPPORT_HDCP_
        Hdmi_HDCP_state(it6811,HDCP_CPDone);
        #ifdef _SHOW_HDCP_INFO_
        hdcpsts();
        #endif
        #endif
      }

    if (Reg07 & 0x04) {
        #ifdef _SUPPORT_HDCP_REPEATER_
        IT6811_DEBUG_INT_PRINTF("IT6811-HDCP KSV List Check Interrupt ... %d\n", (int)it6811->ksvchkcnt);
        hdmitx_int_HDCP_KSVLIST();
        #endif
    }
    
    if (Reg07 & 0x08) {
        IT6811_DEBUG_INT_PRINTF("IT6811-General Control Packet Done Interrupt ...\n\n");
    }

    if (Reg07 & 0x10) {
        IT6811_DEBUG_INT_PRINTF("IT6811-Null Packet Done Interrupt ...\n\n");
    }

    if (Reg07 & 0x20) {
        IT6811_DEBUG_INT_PRINTF("IT6811-ACP Packet Done Interrupt ...\n\n");
    }

    if (Reg07 & 0x80) {
        IT6811_DEBUG_INT_PRINTF("IT6811-3D InfoFrame Packet Done Interrupt ...\n\n");
    }

 // Interrupt Reg08
    if (Reg08 & 0x01) {
        IT6811_DEBUG_INT_PRINTF("IT6811-AVI InfoFrame Packet Done Interrupt ...\n\n");
    }

    if (Reg08 & 0x02) {
        IT6811_DEBUG_INT_PRINTF("IT6811-Audio InfoFrame Packet Done Interrupt ...\n\n");
    }

    if (Reg08 & 0x04) {
        IT6811_DEBUG_INT_PRINTF("IT6811-Gamut Data Packet Done Interrupt ...\n\n");
    }

    if (Reg08 & 0x08) {
        IT6811_DEBUG_INT_PRINTF("IT6811-MPEG InfoFrame Packet Done Interrupt ...\n\n");
    }

    if ((Reg07 & 0x40) || (Reg08 & 0x10)) {
        IT6811_DEBUG_INT_PRINTF("IT6811-Reg07=0x%02X, Reg08=0x%02X\n", (int)Reg07, (int)Reg08);

        if ((hdmitxrd(0x0E) & 0x10) == 0x10) {
            IT6811_DEBUG_INT_PRINTF("IT6811-Video Stable On Interrupt ...\n");
            Hdmi_Video_state(it6811,HDMI_Video_ON);
        } else {
            Hdmi_Video_state(it6811,HDMI_Video_WAIT);
            IT6811_DEBUG_INT_PRINTF("IT6811-Video Stable Off Interrupt ...\n");
        }
    }

    if (Reg08 & 0x20){
        IT6811_DEBUG_INT_PRINTF("IT6811-VSYNC Trigger Interrupt ...\n\n");
    }

    if (Reg08 & 0x40) {
        chgbank(1);
        temp = ((unsigned int)(hdmitxrd(0x35)&0xF0)>>4)+(unsigned int)(hdmitxrd(0x36)<<4)+(unsigned int)(hdmitxrd(0x37)<<12);
        IT6811_DEBUG_INT_PRINTF("IT6811-AudCTSCnt = 0x%X\n",temp);
        chgbank(0);

        IT6811_DEBUG_INT_PRINTF("IT6811-Audio CTS Error Interrupt ...\n\n");
    }

    if (RegEE & 0x01) {
        IT6811_DEBUG_INT_PRINTF("IT6811-Audio Decode Error Interrupt ...\n");

        #if(!_AUDIO_I2S)
            IT6811_DEBUG_INT_PRINTF("IT6811-Current OSFreqNum[13:6] = 0x%X\n", hdmitxrd(0x5B));
            IT6811_DEBUG_INT_PRINTF("IT6811-Current OSFreqNum[5:0] = 0x%X\n", (hdmitxrd(0x5C)&0x3F));
            IT6811_DEBUG_INT_PRINTF("IT6811-\n");
        #endif
    }

    if (RegEE & 0x02) {
        IT6811_DEBUG_INT_PRINTF("IT6811-No Audio Interrupt ...\n");
    }

    if (RegEE & 0x04) {
        IT6811_DEBUG_INT_PRINTF("IT6811-Video FIFO Error Interrupt ...\n");
    }

    if (RegEE & 0x10) {
        #ifdef _SUPPORT_HDCP_
        IT6811_DEBUG_INT_PRINTF("IT6811-HDCP Ri Check Done Interrupt ...\n");
        //hdmitx_int_HDCP_RiCheckDone();
        #ifdef _SHOW_HDCP_INFO_
        hdcpsts();
        #endif
        #endif
    }

    if (RegEE & 0x20) {
        IT6811_DEBUG_INT_PRINTF("IT6811-HDCP Pj Check Done Interrupt ... \n");
         
        #ifdef _SUPPORT_HDCP_
        #ifdef _SHOW_HDCP_INFO_
        hdcpsts();
        #endif
        #endif
    }

    if (RegEE & 0x40) {
        IT6811_DEBUG_INT_PRINTF("IT6811-Video Parameter Change Interrupt ...\n");
        IT6811_DEBUG_INT_PRINTF("IT6811-Take Care of the following Readback. Video Stable = %X ...\n", (int)((hdmitxrd(0x0e)&0x10)>>4));
        IT6811_DEBUG_INT_PRINTF("IT6811-Video Parameter Change Status reg1d[7:4]= 0x%01X => ", (int)((hdmitxrd(0xd1)&0xF0)>>4));
    }

    if (RegEE & 0x80) {
        Hdmi_Video_state(it6811,HDMI_Video_WAIT);
        IT6811_DEBUG_INT_PRINTF("IT6811-V2H FIFO Error Interrupt ...\n");
    }

    if (RegEF & 0x01) {
        IT6811_DEBUG_INT_PRINTF("IT6811-HDCP 1.2 Sync Detect Fail Interrupt ...\n");
    }

    if (MHL04 & 0x01) {
        IT6811_DEBUG_INT_PRINTF("IT6811-CBUS Link Layer TX Packet Done Interrupt ...\n");
    }

    if (MHL04 & 0x02) {
        IT6811_DEBUG_INT_PRINTF("IT6811-ERROR: CBUS Link Layer TX Packet Fail Interrupt ... \n");
        IT6811_DEBUG_INT_PRINTF("IT6811- TX Packet error Status reg15=%X\n", (int)mhltxrd(0x15));
         
        rddata = mhltxrd(0x15);
        mhltxwr(0x15, rddata&0xF0);

        #if(_EnCBusReDisv)
        if( it6811->PKTFailCnt > CBUSFAILMAX ) {
            IT6811_DEBUG_INT_PRINTF("IT6811-ERROR: CBUS Link Layer Error ==> Retry CBUS Discovery Process !!!\n");
            Mhl_state(it6811,MHL_Cbusdet);
         }
        #endif
    }

    if (MHL04 & 0x04) {
        IT6811_DEBUG_INT_PRINTF("IT6811-CBUS Link Layer RX Packet Done Interrupt ...\n");
    }

    if (MHL04 & 0x08) {
         IT6811_DEBUG_INT_PRINTF("IT6811-ERROR: CBUS Link Layer RX Packet Fail Interrupt ... \n");
         IT6811_DEBUG_INT_PRINTF("IT6811- TX Packet error Status reg15=%X\n", (int)mhltxrd(0x15));
         rddata = mhltxrd(0x15);
         mhltxwr(0x15, (rddata & 0x0F));

        #if( _EnCBusReDisv)
        if ( it6811->PKTFailCnt > CBUSFAILMAX) {
            IT6811_DEBUG_INT_PRINTF("IT6811-ERROR: CBUS Link Layer Error ==> Retry CBUS Discovery Process !!!\n");
    
            Mhl_state(it6811, MHL_Cbusdet);
        }
        #endif
    }

    if (MHL04 & 0x10) {
        IT6811_DEBUG_INT_PRINTF("IT6811-MSC RX MSC_MSG Interrupt ...\n");
        mhl_read_mscmsg(it6811 );
    }

    if (MHL04 & 0x20) {
         IT6811_DEBUG_INT_PRINTF("IT6811-MSC RX WRITE_STAT Interrupt ...\n");
    }
    if (MHL04 & 0x40) {
        IT6811_DEBUG_INT_PRINTF("IT6811-MSC RX WRITE_BURST Interrupt  ...\n");
        ITE6811MhlTxMscWriteBurstDone(MHL04&0x40);
    }

    if (MHL04 & 0x80) {
        IT6811_DEBUG_INT_PRINTF("IT6811-CBUS Low and VBus5V not Detect Interrupt ... ==> %dth Fail\n", (int)it6811->CBusDetCnt);
        
        if (it6811->CBusDetCnt > CBUSDETMAX) {
            IT6811_DEBUG_INT_PRINTF("IT6811-ERROR: CBUS Low and VBus5V Detect Error ==> Switch to USB Mode !!!\n");
            Mhl_state(it6811, MHL_USB);
            MhlTxDrvProcessDisconnection();
        
        } else {
            Mhl_state(it6811, MHL_Cbusdet); 
		}
    }

    if (MHL05 & 0x01) {
        IT6811_DEBUG_INT_PRINTF("IT6811-MSC Req Done Interrupt ...\n");
        mscCmdInProgress = false;
        ITE6811MhlTxMscCommandDone(mhltxrd(0x56));
    }

    if (MHL05 & 0x02) {
        //IT6811_DEBUG_INT_PRINTF("IT6811-MSC Req Fail Interrupt (Unexpected) ... ==> %dth Fail\n", it6811->MSCFailCnt);
        IT6811_DEBUG_INT_PRINTF("IT6811-MSC Req Fail Interrupt (Unexpected) ...\n");
        IT6811_DEBUG_INT_PRINTF("IT6811-MSC Req Fail reg18= %02X \n",(int)mhltxrd(0x18));

        CBusProcessErrors(MHL05 & 0x02);
        mscCmdInProgress = false;

        #if( _EnCBusReDisv)
        if (it6811->MSCFailCnt > CBUSFAILMAX) {
            IT6811_DEBUG_INT_PRINTF("IT6811-ERROR: CBUS MSC Channel Error ==> Retry CBUS Discovery Process !!!\n");
            Mhl_state(it6811, MHL_Cbusdet);
        }
        #endif
    }

    if (MHL05 & 0x04) {
     IT6811_DEBUG_INT_PRINTF("IT6811-MSC Rpd Done Interrupt ...\n");
    }

    if (MHL05 & 0x08) {
        IT6811_DEBUG_INT_PRINTF("IT6811-MSC Rpd Fail Interrupt ...\n");
        IT6811_DEBUG_INT_PRINTF("IT6811-MSC Rpd Fail status reg1A=%X reg1B=%X\n", (int)mhltxrd(0x1A),(int)mhltxrd(0x1B)); 
        
        CBusProcessErrors(MHL05 & 0x08);
        mscCmdInProgress = false;

         #if(_EnCBusReDisv)
         if (it6811->MSCFailCnt > CBUSFAILMAX) {
            IT6811_DEBUG_INT_PRINTF("IT6811-ERROR: CBUS MSC Channel Error ==> Retry CBUS Discovery Process !!!\n");
            Mhl_state(it6811, MHL_Cbusdet);
         }
         #endif
    }

    if (MHL05 & 0x10) {
        IT6811_DEBUG_INT_PRINTF("IT6811-DDC Req Done Interrupt ...\n");
    }

    if (MHL05 & 0x20) {
        IT6811_DEBUG_INT_PRINTF("IT6811-DDC Req Fail Interrupt (Hardware) ... \n");
        IT6811_DEBUG_INT_PRINTF("IT6811-DDC Req Fail reg16=%X\n",(int)mhltxrd(0x16));
         
        CBusProcessErrors(MHL05 & 0x20);
        mscCmdInProgress = false;

        #if(_EnCBusReDisv)
        if (it6811->DDCFailCnt > CBUSFAILMAX) {
            IT6811_DEBUG_INT_PRINTF("IT6811-ERROR: CBUS DDC Channel Error ==> Retry CBUS Discovery Process !!!\n");
            //mhl_cbus_rediscover();
            Mhl_state(it6811, MHL_Cbusdet);
        }
        #endif

    }

    if (MHL05 & 0x40) {
        IT6811_DEBUG_INT_PRINTF("IT6811-DDC Rpd Done Interrupt ...\n");
    }

    if (MHL05 & 0x80) {
        IT6811_DEBUG_INT_PRINTF("IT6811-DDC Rpd Fail Interrupt ...\n");
    }

    if (MHL06 & 0x01) {
        IT6811_DEBUG_INT_PRINTF("IT6811-CBUS 1K Detect Done Interrupt ...\n");
        Mhl_state(it6811, MHL_CBUSDiscover);
    }

    if (MHL06 & 0x02) {
        IT6811_DEBUG_INT_PRINTF("IT6811-CBUS 1K Detect Fail Interrupt ... ==> %dth Fail\n", (int)it6811->Det1KFailCnt);

         if (it6811->Det1KFailCnt > DISVFAILMAX) {
            IT6811_DEBUG_INT_PRINTF("it6811-ERROR: CBUS 1K Detect Error ==> Switch to USB Mode !!!\n");
                
            Mhl_state(it6811, MHL_USB);
            MhlTxDrvProcessDisconnection();
         } else {
            Mhl_state(it6811, MHL_1KDetect);
            SwitchToD0();
         }
    }

    if (MHL06 & 0x04) {
        IT6811_DEBUG_INT_PRINTF("it6811-CBUS Discovery Done Interrupt ...\n");

        Mhl_state(it6811, MHL_CBUSDisDone);
		MhlTxDrvProcessConnection();//add by spl
    }

    if (MHL06 & 0x08) {
        IT6811_DEBUG_INT_PRINTF("it6811-CBUS Discovery Fail Interrupt ... ==> %dth Fail\n",(int) it6811->DisvFailCnt);

        if (it6811->DisvFailCnt > DISVFAILMAX) {
            IT6811_DEBUG_INT_PRINTF("it6811-ERROR: CBUS Discovery Error ==> Switch to USB Mode !!!\n");

            Mhl_state(it6811, MHL_USB);
            MhlTxDrvProcessDisconnection();
        } else {
            Mhl_state(it6811, MHL_CBUSDiscover);
        }
    }

    if (MHL06 & 0x20) {
        IT6811_DEBUG_INT_PRINTF("it6811-CBUS MUTE Change Interrupt ...\n");
        IT6811_DEBUG_INT_PRINTF("it6811-Current CBUS MUTE Status = %X \n", (int)(mhltxrd(0xB1)&0x10)>>4);
    }

    if ((MHL06 & 0x10) || (MHL06 & 0x40)) {
        uint8_t status_0=0; 
        uint8_t status_1=0;

        if (MHL06 & 0x10) {
			IT6811_DEBUG_INT_PRINTF("IT6811-CBUS PATH_EN Change Interrupt ...\n");

			rddata = (mhltxrd(0xB1) & 0x08) >> 3;
			IT6811_DEBUG_INT_PRINTF("it6811-Current RX PATH_EN status = %d \n", (int)rddata);

			if (rddata) {
				status_1 = MHL_STATUS_PATH_ENABLED;
			}
        }

        if (MHL06 & 0x40) {
            IT6811_DEBUG_INT_PRINTF("IT6811-[MHL06!!]CBUS DCapRdy Change Interrupt ...\n");
            
            if (mhltxrd(0xB0) & 0x01) {
                it6811->devcapRdy = TRUE;
                status_0 = MHL_STATUS_DCAP_RDY;
            } else {
                it6811->devcapRdy = FALSE;
                IT6811_DEBUG_INT_PRINTF("IT6811-DCapRdy Change from '1' to '0'\n");
            }
        }

        ITE6811MhlTxGotMhlStatus(status_0, status_1);

    }

    if (MHL06 & 0x80) {
        IT6811_DEBUG_INT_PRINTF("IT6811-VBUS Status Change Interrupt ...\n");
        IT6811_DEBUG_INT_PRINTF("IT6811-Current VBUS Status = %X\n",(int) (mhltxrd(0x10)&0x08)>>3);

        #if(_AutoSwBack)
        if ((mhltxrd(0x10) & 0x08)) {
            if (mhltxrd(0x0F) & 0x01) {  
                Mhl_state(it6811, MHL_Cbusdet);
            }
        }
        #endif
    }

    if (MHLA0 & 0x01) {
        IT6811_DEBUG_INT_PRINTF("IT6811-[MHLA0!!!] MHL Device Capability Change Interrupt ...\n");
        ITE6811MhlTxGotMhlIntr(MHL_INT_DCAP_CHG, 0);
    }

    if (MHLA0 & 0x02) {
        IT6811_DEBUG_INT_PRINTF("IT6811-MHL DSCR_CHG Interrupt ......\n");
        ITE6811MhlTxGotMhlIntr(MHL_INT_DSCR_CHG, 0); 
    }

    if (MHLA0 & 0x04) {
        IT6811_DEBUG_INT_PRINTF("IT6811-MHL REQ_WRT Interrupt  ...\n");
        ITE6811MhlTxGotMhlIntr(MHL_INT_REQ_WRT, 0);   
    }

    if (MHLA0 & 0x08) {
        IT6811_DEBUG_INT_PRINTF("IT6811-[**]MHL GNT_WRT Interrupt  ...\n");
        ITE6811MhlTxGotMhlIntr(MHL_INT_GRT_WRT, 0);
    }

    if (MHLA1 & 0x02) {
        IT6811_DEBUG_INT_PRINTF("IT6811-MHL EDID Change Interrupt ...\n");
        ITE6811MhlTxGotMhlIntr(0, MHL_INT_EDID_CHG);
    }

exit_hdmitx_irq:
    return;

}

void hdmitx_pwron( void )
{
    // MHLTX PwrOn
    hdmitxset(0x0F, 0x78, 0x38);   // PwrOn GRCLK
    hdmitxset(0x05, 0x01, 0x00);   // PwrOn PCLK

    // PLL PwrOn
    hdmitxset(0x61, 0x20, 0x00);   // PwrOn DRV
    hdmitxset(0x62, 0x44, 0x00);   // PwrOn XPLL
    hdmitxset(0x64, 0x40, 0x00);   // PwrOn IPLL

    // PLL Reset OFF
    hdmitxset(0x61, 0x10, 0x00);   // DRV_RST
    hdmitxset(0x62, 0x08, 0x08);   // XP_RESETB
    hdmitxset(0x64, 0x04, 0x04);   // IP_RESETB
    
    IT6811_DEBUG_PRINTF("IT6811-Power On MHLTX \n");
}

void hdmitx_pwrdn( void )
{
    hdmitxset(0x04, 0x1D, 0x1D);   // ACLK/VCLK/HDCP Reset

    // Clear all Interrupt
    hdmitxwr(0x06, 0xFF);
    hdmitxwr(0x07, 0xFF);
    hdmitxwr(0x08, 0xFF);
    hdmitxwr(0xEE, 0xFF);

    mhltxwr(0x04, 0xFF);
    mhltxwr(0x05, 0xFF);
    mhltxwr(0x06, 0xFF); 
   
    //Enable GRCLK

    hdmitxset(0x0F, 0x40, 0x00);
    // PLL Reset
    hdmitxset(0x61, 0x10, 0x10);   // DRV_RST
    hdmitxset(0x62, 0x08, 0x00);   // XP_RESETB
    hdmitxset(0x64, 0x04, 0x00);   // IP_RESETB
    //idle(100);
    delay1ms(1);

    // PLL PwrDn
    hdmitxset(0x61, 0x20, 0x20);   // PwrDn DRV
    hdmitxset(0x62, 0x44, 0x44);   // PwrDn XPLL
    hdmitxset(0x64, 0x40, 0x40);   // PwrDn IPLL

    //hdmitxwr(0x70, 0x00);        // Select TXCLK power-down path
     hdmitxset(0x70, 0x20, 0x00);

    //MHLTX PwrDn
    hdmitxset(0x05, 0x01, 0x01);   // PwrDn PCLK
    //hdmitxset(0x0F, 0x08, 0x08);   // PwrDn CRCLK
    hdmitxwr(0xE0, 0xC0);          // PwrDn GIACLK, IACLK, ACLK and SCLK
    hdmitxset(0x72, 0x03, 0x00);   // PwrDn GTxCLK(QCLK)

}

void HDMITX_SetVideoOutput(int mode)
{
    struct it6811_dev_data *it6811 = get_it6811_dev_data();

    HDMITX_DEBUG_PRINTF("IT6811-HDMITX_EnableVideoOutput(%X)\n",(int)mode);
    it6811->VidFmt = mode;
    Hdmi_Video_state(it6811,HDMI_Video_WAIT);
}

void HDMITX_SET_SignalType(unsigned char DynRange,unsigned char colorcoef,unsigned char pixrep)
{
    struct it6811_dev_data *it6811 = get_it6811_dev_data();

    it6811->DynRange   = DynRange;         //DynCEA, DynVESA
    it6811->YCbCrCoef  = colorcoef;       //ITU709, ITU601
    it6811->PixRpt     =  pixrep;
}
void HDMITX_SET_PacketPixelmode(unsigned char mode)
{
    struct it6811_dev_data *it6811 = get_it6811_dev_data();
    it6811->EnPackPix   = mode;
}

void HDMITX_SET_HDCP(unsigned char mode)
{
#ifdef _SUPPORT_HDCP_
    //struct it6811_data *it6811data = dev_get_drvdata(it6811dev);
    struct it6811_dev_data *it6811 = get_it6811_dev_data();
    
    mutex_lock(&it6811data->lock);
    it6811->HDCPEnable = mode;
    mutex_unlock(&it6811data->lock);
#endif
}


void HDMITX_Set_ColorType(unsigned char inputColorMode,unsigned char outputColorMode)
{
    struct it6811_dev_data *it6811 = get_it6811_dev_data();

    it6811->InColorMode  = inputColorMode;        //YCbCr422 RGB444 YCbCr444
    it6811->OutColorMode = outputColorMode;
}
int HDMITX_SetAVIInfoFrame(void *p)
{
    int ret;
    int i;
    struct AVI_InfoFrame *avi;
    struct it6811_dev_data *it6811 = get_it6811_dev_data();
 
    ret = get_it6811_Aviinfo(it6811); 
    
    avi =  (struct AVI_InfoFrame*)p;

    if(ret<0){
        HDMITX_DEBUG_PRINTF("IT6811-get_it6811_Aviinfo() ......FAIL !! \n");
        goto set_avi_error;
    }

    it6811->Aviinfo->AVI_HB[0] = avi->AVI_HB[0];
    it6811->Aviinfo->AVI_HB[1] = avi->AVI_HB[1];
    it6811->Aviinfo->AVI_HB[2] = avi->AVI_HB[2]; 
    
    for (i = 0; i < AVI_INFOFRAME_LEN; i++) {
        it6811->Aviinfo->AVI_DB[i]  = avi->AVI_DB[i];
    }       
    
    return 0;
    
set_avi_error:  
    return -1;
}


void HDMITX_change_audio(unsigned char AudType,unsigned char AudFs,unsigned char AudCh)
{
    struct it6811_dev_data *it6811 = get_it6811_dev_data();

    it6811->AudFmt   = AudFs;
    it6811->AudType  = AudType;
    it6811->AudCh    = AudCh;
    
    aud_chg(it6811,TRUE);
}


void cbus_send_mscmsg( struct it6811_dev_data *it6811 )
{
    mhltxwr(0x54, it6811->txmsgdata[0]);
    mhltxwr(0x55, it6811->txmsgdata[1]);
    mscfire(0x51, 0x02);
}

#ifdef _SUPPORT_RCP_

static void mhl_parse_RCPkey(struct it6811_dev_data *it6811)
{
    if (SuppRCPCode[it6811->rxmsgdata[1]]) {
        it6811->txmsgdata[0] = MSG_RCPK;
        it6811->txmsgdata[1] = it6811->rxmsgdata[1];
        MHL_MSC_DEBUG_PRINTF(("Send a RCPK with action code = 0x%02X\n", it6811->txmsgdata[1]));
        
        rcp_report_event(it6811->rxmsgdata[1]);
    } else {
        it6811->txmsgdata[0] = MSG_RCPE;
        it6811->txmsgdata[1] = 0x01;
        
        MHL_MSC_DEBUG_PRINTF(("Send a RCPE with status code = 0x%02X\n", it6811->txmsgdata[1]));
    }

    cbus_send_mscmsg(it6811);
}
#endif

#ifdef  _SUPPORT_RAP_
static void mhl_parse_RAPkey(struct it6811_dev_data *it6811)
{
    it6811->txmsgdata[0] = MSG_RAPK;    
    
    if( SuppRAPCode[it6811->rxmsgdata[1]]) {
        it6811->txmsgdata[1] = 0x00;
    }
    else{
        it6811->txmsgdata[1] = 0x02;
    }       
        
    switch( it6811->rxmsgdata[1] ) {
        case 0x00: 
            MHL_MSC_DEBUG_PRINTF(("Poll\n")); 
            break;
        case 0x10: 
            MHL_MSC_DEBUG_PRINTF(("Change to CONTENT_ON state\n"));
            hdmitxset(0x61, 0x10, 0x00);
            break;
        case 0x11: 
            MHL_MSC_DEBUG_PRINTF(("Change to CONTENT_OFF state\n"));
            hdmitxset(0x61, 0x10, 0x10);
            break;
        default  : 

        it6811->txmsgdata[1] = 0x01;
        MHL_MSC_DEBUG_PRINTF(("ERROR: Unknown RAP action code 0x%02X !!!\n", it6811->rxmsgdata[1]));
        MHL_MSC_DEBUG_PRINTF(("Send a RAPK with status code = 0x%02X\n", it6811->txmsgdata[1]));
     }

    cbus_send_mscmsg(it6811);
}
#endif
#ifdef _SUPPORT_UCP_
static void mhl_parse_UCPkey(struct it6811_dev_data *it6811)
{
    if ((it6811->rxmsgdata[1]&0x80)==0x00) {
        it6811->txmsgdata[0] = MSG_UCPK;
        it6811->txmsgdata[1] = it6811->rxmsgdata[1];
        ucp_report_event(it6811->rxmsgdata[1]);
    } else {
        it6811->txmsgdata[0] = MSG_UCPE;
        it6811->txmsgdata[1] = 0x01;
    }

    cbus_send_mscmsg(it6811);
}
#endif
#ifdef _SUPPORT_UCP_MOUSE_
static void mhl_parse_MOUSEkey(struct it6811_dev_data *it6811)
{
    unsigned char opt,sig;
    unsigned char key =0;
    int x =0,y=0;
    
    opt = (it6811->rxmsgdata[1]&0xC0)>>6;
    sig = (it6811->rxmsgdata[1]&0x20)>>5;

    if(opt <3){
        it6811->txmsgdata[0] = MSG_MOUSEK;
        it6811->txmsgdata[1] = it6811->rxmsgdata[1];
        
        if (opt==0) {
            key = it6811->rxmsgdata[1]&0x3F;
        } else if (opt == 1) {
            x = (int)(it6811->rxmsgdata[1]&0x1F);
            x = x*2;  //scalling move
            if(sig) x = (-1)*x;
        } else {
            y = (int)(it6811->rxmsgdata[1]&0x1F);
            y = y*2;  //scalling move
            if(sig) y = (-1)*y;
        }
        
        ucp_mouse_report_event(key,x,y);
    } else {
        it6811->txmsgdata[0] = MSG_MOUSEE;
        it6811->txmsgdata[1] = 0x01;
    }

    cbus_send_mscmsg(it6811);
}
#endif

static void mhl_read_mscmsg( struct it6811_dev_data *it6811 )
{
    it6811->rxmsgdata[0] = mhltxrd(0x60);
    it6811->rxmsgdata[1] = mhltxrd(0x61);
    ITE6811MhlTxGotMhlMscMsg( it6811->rxmsgdata[0], it6811->rxmsgdata[1] );
}

static struct it6811_dev_data* get_it6811_dev_data(void)
{
        return &it6811data;
}

static int get_it6811_Aviinfo(struct it6811_dev_data *it6811)
{
    return 0;
}

void IteMhlEnterIDDQmode(void)
{
}

void IteMhlOpenUSBswitch(int on)
{
    struct it6811_dev_data *it6811 = get_it6811_dev_data();

    if (on) {
        Mhl_state(it6811,MHL_USB);
    } else {
        Mhl_state(it6811,MHL_Cbusdet);
    }
}

int IteMhlDeviceTxIsr(void)
{
    struct it6811_dev_data *it6811 = get_it6811_dev_data();
    hdmitx_irq(it6811);
	return 0;
}

int IteIrqCheck(void)
{
    uint8_t int4Status;

    int4Status = hdmitxrd(0xF0); 
    if(0xFF == int4Status) {
        return I2C_INACCESSIBLE;
    } else {
    	return I2C_ACCESSIBLE;
    }
}


int iteDDccheck(void)
{
    unsigned char cbusstat;
    cbusstat = mhltxrd(0x1C);
    HDMITX_DEBUG_PRINTF("it6811 Cbus 0x1C = %2.2X \n",cbusstat);
    
    if(cbusstat&0x01)
        return FALSE;

    hdmitxwr(0x15, 0x09);           // DDC FIFO Clear
    hdmitxwr(0x11, 0xA0);           // EDID Address
    hdmitxwr(0x12, 0);              // EDID Offset
    hdmitxwr(0x13, 2);              // Read ByteNum 
    hdmitxwr(0x14, 0);              // EDID Segment
     
    if( _EDIDRdType_== _COMBRD ) {
        hdmitxset(0x1B, 0x80, 0x80);
     
        if( ddcfire(0x15, 0x00)==FAIL ) {
            HDMITX_DEBUG_PRINTF("ERROR: DDC EDID Read Fail !!!\n");
            return FALSE;
        }
    } else {
        if( ddcfire(0x15, 0x03)==FAIL ) {
            HDMITX_DEBUG_PRINTF("ERROR: DDC EDID Read Fail !!!\n");
            return FALSE;
        }
    }
    return TRUE;
}    


int iteReadEdidBlock( unsigned char *buff ,int block)
{
    int i;
	int offset = 0;

	for(i=0;i<32;i++) {
		buff[offset+i] = hdmitxrd(0x17);
		HDMITX_DEBUG_PRINTF("EDID[%2.2X] is [%2.2X]-------\n",i,hdmitxrd(0x17));
	}	 

	for(offset = 0; offset < 128; offset += _EDIDRdByte_) {
		hdmitxwr(0x15, 0x09);                       // DDC FIFO Clear
		hdmitxwr(0x11, 0xA0);                       // EDID Address
		hdmitxwr(0x12, offset);                     // EDID Offset
		hdmitxwr(0x13, _EDIDRdByte_);               // Read ByteNum 
		hdmitxwr(0x14, (unsigned char)(block/2));    // EDID Segment

		HDMITX_DEBUG_PRINTF("EDID read :offset = %d  !!!\n",offset);
		HDMITX_DEBUG_PRINTF("reg0x11=%2.2X  reg0x12=%2.2X reg0x13=%2.2X ", hdmitxrd(0x11), hdmitxrd(0x12), hdmitxrd(0x13));
		HDMITX_DEBUG_PRINTF("reg0x14=%2.2X  reg0x15=%2.2X reg0x16=%2.2X \n", hdmitxrd(0x14), hdmitxrd(0x15), hdmitxrd(0x16));

		if( _EDIDRdType_== _COMBRD ) {
			hdmitxset(0x1B, 0x80, 0x80);

			if( ddcfire(0x15, 0x00)==FAIL ) {
				HDMITX_DEBUG_PRINTF("ERROR: DDC EDID Read Fail !!!\n");
				return FAIL;
			}
		} else if( ddcfire(0x15, 0x03)==FAIL ) {
			HDMITX_DEBUG_PRINTF("ERROR: DDC EDID Read Fail !!!\n");
			return FAIL;
		}

		for(i=0;i<_EDIDRdByte_;i++) {
			buff[offset+i] = hdmitxrd(0x17);
			HDMITX_DEBUG_PRINTF("buff[%2.2X] is [%2.2X]-------\n",offset+i,buff[offset+i]);
		}    
	}

     hdmitxwr(0x15, 0x09);          // DDC FIFO Clear
     hdmitxset(0x10, 0x01, 0x00);   // Disable PC DDC Mode

     return SUCCESS;
}
