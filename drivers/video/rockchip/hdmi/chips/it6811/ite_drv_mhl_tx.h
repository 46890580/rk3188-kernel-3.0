#ifndef _ITE_DRV_MHL_TX_H_
#define _ITE_DRV_MHL_TX_H_
#include "ite_drv_mhl_tx_edid.h"
//#define _SUPPORT_RAP_
//#define _SUPPORT_RCP_
//#define _SUPPORT_UCP_
//#define _SUPPORT_UCP_MOUSE_

//#define _SUPPORT_HDCP_			  
//#define _SUPPORT_HDCP_REPEATER_	

//#define DISABLE_HDMITX_CSC

///////////////////////////////////////////////////////////////
// IT6811 debug define
//
///////////////////////////////////////////////////////////////

#define debug_6811	printk

#define HDMI_ADDR 0x4C
#define MHL_ADDR 0x64

#define IT6811_DEBUG_INT_PRINTF debug_6811
#define IT6811_DEBUG_CAP_INFO debug_6811
#define MHL_MSC_DEBUG_PRINTF debug_6811

#define HDMITX_DEBUG_PRINTF debug_6811
#define HDMITX_MHL_DEBUG_PRINTF debug_6811
#define HDMITX_DEBUG_HDCP_PRINTF debug_6811
#define HDMITX_DEBUG_HDCP_INT_PRINTF debug_6811


//#define _SHOW_VID_INFO_	    
//#define _SHOW_HDCP_INFO_	

/////////////////////////////////////////////////////////////////////
// Packet and Info Frame definition and datastructure.
/////////////////////////////////////////////////////////////////////

#define VENDORSPEC_INFOFRAME_TYPE 0x81

#define SPD_INFOFRAME_TYPE 0x83
#define AUDIO_INFOFRAME_TYPE 0x84
#define MPEG_INFOFRAME_TYPE 0x85

#define VENDORSPEC_INFOFRAME_VER 0x01

#define SPD_INFOFRAME_VER 0x01
#define AUDIO_INFOFRAME_VER 0x01
#define MPEG_INFOFRAME_VER 0x01

#define VENDORSPEC_INFOFRAME_LEN 8

#define SPD_INFOFRAME_LEN 25
#define AUDIO_INFOFRAME_LEN 10
#define MPEG_INFOFRAME_LEN 10

#define ACP_PKT_LEN 9
#define ISRC1_PKT_LEN 16
#define ISRC2_PKT_LEN 16
/////////////////////////////////////////////////
// IT6811 function
//
//
//
/////////////////////////////////////////////////

#define TRUE 1
#define FALSE 0

#define SUCCESS	        1
#define RCVABORT        2
#define RCVNACK         3
#define ARBLOSE         4
#define FWTXFAIL        5
#define FWRXPKT         6
#define FAIL		   -1
#define ABORT          -2

#define AFE_SPEED_HIGH            1
#define AFE_SPEED_LOW             0

#define HDMI            0
#define DVI             1
#define RSVD            2

#define F_MODE_RGB24  0
#define F_MODE_RGB444 0
#define F_MODE_YUV422 1
#define F_MODE_YUV444 2
#define F_MODE_CLRMOD_MASK 3

#define VID8BIT         0
#define VID10BIT        1
#define VID12BIT        2
#define VID16BIT        3

#define RGB444          0
#define YCbCr422        1
#define YCbCr444        2

#define DynVESA         0
#define DynCEA          1

#define ITU601          0
#define ITU709          1

#define TDM2CH          0x0
#define TDM4CH          0x1
#define TDM6CH          0x2
#define TDM8CH          0x3

#define AUD32K          0x3
#define AUD48K          0x2
#define AUD96K          0xA
#define AUD192K         0xE
#define AUD44K          0x0
#define AUD88K          0x8
#define AUD176K         0xC
#define AUD768K         0x9

#define I2S             0
#define SPDIF           1

#define LPCM            0
#define NLPCM           1
#define HBR             2
#define DSD             3

#define NOCSC           0
#define RGB2YUV         2
#define YUV2RGB         3

#define _FrmPkt          0
#define _SbSFull         3
#define _TopBtm          6
#define _SbSHalf         8



#define AUD16BIT        0x2
#define AUD18BIT        0x4
#define AUD20BIT        0x3
#define AUD24BIT        0xB

#define AUDCAL1         0x4
#define AUDCAL2         0x0
#define AUDCAL3         0x8

#define PICAR_NO        0
#define PICAR4_3        1
#define PICAR16_9       2

#define ACTAR_PIC       8
#define ACTAR4_3        9
#define ACTAR16_9       10
#define ACTAR14_9       11

#define MHLInt00B       0x20
#define DCAP_CHG        0x01
#define DSCR_CHG        0x02
#define REQ_WRT         0x04
#define GRT_WRT         0x08

#define MHLInt01B       0x21
#define EDID_CHG        0x01

#define MHLSts00B       0x30
#define DCAP_RDY        0x01

#define MHLSts01B       0x31
#define NORM_MODE       0x03
#define PACK_MODE       0x02
#define PATH_EN         0x08
#define MUTED           0x10

#define MSG_MSGE		0x02
#define MSG_RCP 		0x10
#define MSG_RCPK		0x11
#define MSG_RCPE		0x12
#define MSG_RAP 		0x20
#define MSG_RAPK		0x21
#define MSG_UCP			0x30
#define MSG_UCPK		0x31
#define MSG_UCPE		0x32
#define MSG_MOUSE	    0x40
#define MSG_MOUSEK		0x41
#define MSG_MOUSEE		0x42


///////////////////////////////////////////////////////////
// Time counter for state machine
// Timer Value is based on loop or task time,
//
// i.e. TIME 10
// 		Thread 100ms
//		real  TIME is 100X100ms = 1s
///
///////////////////////////////////////////////////////////
//
//#define INTTIMEOUT      1
//#define SYSTIMEOUT      2
//#define VIDTIMEOUT      5
//#define AUDTIMEOUT      2
//#define RXSENOFFTIME    5

/////////////////////////////////////////
//Cbus command fire wait time
//Maxmun time for determin CBUS fail 
//	CBUSWAITTIME(ms) x CBUSWAITNUM
/////////////////////////////////////////
#define CBUSWAITTIME    10
#define CBUSWAITNUM     10

/////////////////////////////////////////
// CBUS discover error Counter
//
// determine Cbus connection by interrupt count
//
////////////////////////////////////////
#define CBUSDETMAX      100
#define DISVFAILMAX     20
#define CBUSFAILMAX     20

//Cbus retry is disable fo SW porting issue.
//when CBUS fail just assume it will always fail.
//#define CBUSRTYMAX      20

//#define RAPBUSYNUM      50
//#define RCPBUSYNUM      50


//initial define options
//////////////////////////////////////////
// CBUS Input Option
//
// CBUS Discovery and Disconnect Option
///
/////////////////////////////////////////

#define _EnCBusReDisv     FALSE

#define _EnCBusU3IDDQ     FALSE

#define _EnVBUSOut        FALSE	   //Some dongle need to force this

#define _AutoSwBack       TRUE

//////////////////////////////////////////
// MHL POWDN GRCLK 
// MHL can power-down GRCLK only when REnCBUS=0
//
/////////////////////////////////////////
//#define  _EnGRCLKPD  FALSE  

//////////////////////////////////////////
// MSC Option
//
// 
///
/////////////////////////////////////////

#define _MaskMSCDoneInt  	FALSE 	// MSC Rpd Done Mask and MSC Req Done Mask 
#define _MSCBurstWrID  		FALSE   // TRUE: Adopter ID from MHL5E/MHL5F
//#define _EnPktFIFOBurst  	TRUE 	// TRUE for MSC test
//#define _MSCBurstWrOpt  	FALSE  	// TRUE: Not write Adopter ID into ScratchPad
//#define _EnMSCBurstWr 	TRUE
//#define _EnMSCHwRty  		FALSE
//#define _MSCRxUCP2Nack  	TRUE
#define _EnHWPathEn  		FALSE   // FALSE: FW PATH_EN

//////////////////////////////////////////
// Define  System EDID option
// 
//
/////////////////////////////////////////
// SEGRD: Segment Read, COMBRD: Combine Read
#define _SEGRD           0
#define _COMBRD          1

#define _EnEDIDRead   TRUE
#define _EnEDIDParse  FALSE
#define _EDIDRdByte_   16
#define _EDIDRdType_   _SEGRD 		
	  
//////////////////////////////////////////
// MHL Output mode config
// 
//
/////////////////////////////////////////
//R/B or Cr/Cb swap after data packing
#define  _PackSwap 	  FALSE
//Packet pixel mode enable
#define  _EnPackPix   FALSE
//packet pixel mode band swap
#define  _EnPPGBSwap  TRUE
//Packet pixel mode HDCP
#define  _PPHDCPOpt	  TRUE

//////////////////////////////////////////
// video Input Option
//
//
//////////////////////////////////////////

#define _SyncEmbedd 	FALSE
// TRUE: VSYNC change in SAV, FALSE: VSYNC change in EAV
#define _EnSavVSync 	FALSE 
// TRUE: 8/10/12-bit 656 format, FALSE: 16/20/24-bit 656 format		
#define _EnCCIR8  		FALSE    	 
//Input DDR mode
#define _EnInDDR  		FALSE
//Input DE olny, need sync gen
#define _EnDEOnly  		FALSE
//YCbCr422 BTA-T1004 format
#define _EnBTAFmt  		FALSE 
//clip R/G/B/Y to 16~235, Cb/Cr to 16~240
#define _EnColorClip   	TRUE

#define _RegPCLKDiv2   	FALSE

#define _Reg2x656Clk   	FALSE


#define _LMSwap 		FALSE
#define _YCSwap 		FALSE
#define _RBSwap 		FALSE
//////////////////////////////////////////
// Audio Input Option
//
//
//////////////////////////////////////////
#define _SUPPORT_AUDIO_ TRUE
#define _AUDIO_I2S      TRUE

#define INPUT_SAMPLE_FREQ_HZ AUD44K//Tranmin for MTK setting

#define SUPPORT_AUDI_AudSWL 0x00 //0x00 = 16bit, 0x40 = 18bit, 0x80 = 20bit, 0xc0 = 24bit. //Tranmin 

#if(SUPPORT_AUDI_AudSWL==0x00) //16
    #define CHTSTS_SWCODE 0x02
#elif(SUPPORT_AUDI_AudSWL==0x40) //18
    #define CHTSTS_SWCODE 0x04
#elif(SUPPORT_AUDI_AudSWL==0x80) //20
    #define CHTSTS_SWCODE 0x03
#else //24
    #define CHTSTS_SWCODE 0x0B
#endif

//#define I2S_FORMAT 0x04 // 24bit I2S audio，BIT0 = 0 for 24BIT I2S MODE, BIT1 = 0 FOR LEFT JUSTIFIED MODE, BIT2 =1 for cancel the delay, bit 5 for polarity of SCLK of I2S(0->rising).
//#define I2S_FORMAT 0x06 // 24bit I2S audio, right justify //

#define I2S_FORMAT 0x00 // 24bit I2S audio
//#define I2S_FORMAT 0x01 // 32bit I2S audio, left justify with 1T delay
//#define I2S_FORMAT 0x05 // 32bit I2S audio, left justify with 0T delay//Tranmin add
// #define I2S_FORMAT 0x02 // 24bit I2S audio, right justify
// #define I2S_FORMAT 0x03 // 32bit I2S audio, right justify

//////////////////////////////////////////
// HDCP setting 
// 
//
/////////////////////////////////////////
// HDCP Option
//count about HDCP fail retry 
#define _HDCPFireMax  	100
#define _ChkKSVListMax  500

//////////////////////////////////////////
// Video color space convert
//
//
//////////////////////////////////////////
#define SUPPORT_INPUTRGB
#define SUPPORT_INPUTYUV444
#define SUPPORT_INPUTYUV422

#define B_HDMITX_CSC_BYPASS    0
#define B_HDMITX_CSC_RGB2YUV   2
#define B_HDMITX_CSC_YUV2RGB   3


#define F_VIDMODE_ITU709  (1<<4)
#define F_VIDMODE_ITU601  0

#define F_VIDMODE_0_255   0
#define F_VIDMODE_16_235  (1<<5)



/////////////////////////////////////////////////////
struct IT6811_REG_INI {
    unsigned char ucAddr;
    unsigned char andmask;
    unsigned char ucValue;
}  ;


typedef enum  {
    MHL_USB_PWRDN = 0,
	MHL_USB,
	MHL_Cbusdet,
    MHL_1KDetect,
    MHL_CBUSDiscover,
	MHL_CBUSDisDone,
    MHL_Unknown
}MHLState_Type;

typedef enum  {
    HDMI_Video_REST= 0,
	HDMI_Video_WAIT,
	HDMI_Video_ON,
    HDMI_Video_Unknown
}HDMI_Video_state;


typedef enum  {
    HDCP_Off = 0,
	HDCP_CPStart,
	HDCP_CPGoing,
	HDCP_CPDone,
	HDCP_CPFail,
    HDCP_CPUnknown
} HDCPSts_Type ;

/////////////////////////////////////////
//
//NOTE: all the info struct in infoframe
//      is aligned from LSB. 
//
////////////////////////////////////////
#define AVI_INFOFRAME_TYPE 0x82
#define AVI_INFOFRAME_VER 0x02
#define AVI_INFOFRAME_LEN 13
//  |Type 
//  |Ver 
//	|Len 
//	|Scan:2 | BarInfo:2 | ActiveFmtInfoPresent:1 | ColorMode:2 | FU1:1|
//  |AspectRatio:4 | ictureAspectRatio:2 | Colorimetry:2 
//	|Scaling:2 | FU2:6 
//  |VIC:7 | FU3:1
//	|PixelRepetition:4 | FU4:4 
//	|Ln_End_Top 
//	|Ln_Start_Bottom 
//	|Pix_End_Left 
//	|Pix_Start_Right 

struct AVI_InfoFrame{
 
	unsigned char AVI_HB[3] ;
    unsigned char AVI_DB[AVI_INFOFRAME_LEN] ;
 
};

struct it6811_dev_data {  

    unsigned char ver;
    unsigned long RCLK;

    //MHL Sink CAPS
    //unsigned char RxMHLVer;
	//power control
	//unsigned char GRCLKPD;
	unsigned int RxSen;
   
 
    HDMI_Video_state Hdmi_video_state;
    // video format
    unsigned int VidFmt;
    //unsigned int Vidchg;
    //unsigned char ColorDepth;
    unsigned char InColorMode;
    unsigned char OutColorMode;
    unsigned char DynRange ;
    unsigned char YCbCrCoef ;
    unsigned char PixRpt;
	struct AVI_InfoFrame *Aviinfo;
    // 3D Option
    //unsigned char En3D;
    //unsigned char Sel3DFmt;      //FrmPkt, TopBtm, SbSHalf, SbSFull
    unsigned char EnPackPix;

    // Audio Option
    //unsigned char AudEn ;
    //unsigned char AudSel ; // I2S or SPDIF
    unsigned char AudFmt ; //audio sampling freq
    unsigned char AudCh ;
    unsigned char AudType ; // LPCM, NLPCM, HBR, DSD


#ifdef _SUPPORT_HDCP_
   //HDCP
   unsigned char HDCPEnable;
   HDCPSts_Type Hdcp_state;
   unsigned int HDCPFireCnt ;
   //unsigned int HDCPRiChkCnt ;
   //unsigned int ksvchkcnt ;
   //unsigned int syncdetfailcnt;
#endif
 

    //CBUS MSC 
	MHLState_Type Mhl_state;
    unsigned char CBusDetCnt ;
    unsigned char Det1KFailCnt ;
    unsigned char DisvFailCnt ;  //Discover fail count determine when to switch to USB mode
    unsigned int  devcapRdy;
	unsigned char Mhl_devcap[16];

	unsigned char txmsgdata[2];
	unsigned char rxmsgdata[2];
	//unsigned char txscrpad[16];
	//unsigned char rxscrpad[16];
	
	
};
extern void ITE6811MhlTxDeviceIsr( void );


// Video Mode Constants
//====================================================
#define VMD_ASPECT_RATIO_4x3			0x01
#define VMD_ASPECT_RATIO_16x9			0x02

//====================================================
// Video mode define ( = VIC code, please see CEA-861 spec)
#define HDMI_640X480P		1
#define HDMI_480I60_4X3	6
#define HDMI_480I60_16X9	7
#define HDMI_576I50_4X3	21
#define HDMI_576I50_16X9	22
#define HDMI_480P60_4X3	2
#define HDMI_480P60_16X9	3
#define HDMI_576P50_4X3	17
#define HDMI_576P50_16X9	18
#define HDMI_720P60			4
#define HDMI_720P50			19
#define HDMI_1080I60		5
#define HDMI_1080I50		20
#define HDMI_1080P24		32
#define HDMI_1080P25		33
#define HDMI_1080P30		34
//#define HDMI_1080P60		16 //MHL doesn't supported
//#define HDMI_1080P50		31 //MHL doesn't supported

typedef struct
{
    uint8_t inputColorSpace;
    uint8_t outputColorSpace;
    uint8_t inputVideoCode;
    uint8_t outputVideoCode;
    uint8_t inputcolorimetryAspectRatio;
    uint8_t outputcolorimetryAspectRatio;
    uint8_t input_AR;   
    uint8_t output_AR;  
} video_data_t;
enum
{
    ACR_N_value_192k    = 24576,
    ACR_N_value_96k     = 12288,
    ACR_N_value_48k     = 6144,
    ACR_N_value_176k    = 25088,
    ACR_N_value_88k     = 12544,
    ACR_N_value_44k     = 6272,
    ACR_N_value_32k     = 4096,
    ACR_N_value_default = 6144,
};
#define COLOR_SPACE_RGB 0x00
#define COLOR_SPACE_YCBCR422 0x01
#define COLOR_SPACE_YCBCR444 0x02
typedef enum
{
    VM_VGA = 0,     
    VM_480P,        
    VM_576P,        
    VM_720P60,      
    VM_720P50,      
    VM_INVALID
} inVideoTypes_t;
typedef enum
{
    I2S_192 = 0,
    I2S_96,
    I2S_48,
    I2S_176,
    I2S_88,
    I2S_44,
    I2S_32,
    TDM_192,
    TDM_96,
    TDM_48,
    TDM_176,
    TDM_88,
    TDM_44,
    TDM_32,
    TDM_192_8ch,
    AUD_SPDIF,
    AUD_TYP_NUM,
    AUD_INVALID
} inAudioTypes_t;
typedef struct
{
    uint8_t regAUD_mode;    
    uint8_t regAUD_ctrl;    
    uint8_t regAUD_freq;    
    uint8_t regAUD_src;     
    uint8_t regAUD_tdm_ctrl;
    uint8_t regAUD_path;    
} audioConfig_t;
typedef struct tagAVModeChange
{
    bool_t video_change;
    bool_t audio_change;
}AVModeChange_t;
typedef struct tagAVMode
{
    inVideoTypes_t video_mode;
    inAudioTypes_t audio_mode;
}AVMode_t;
//bool_t ITE6811VideoInputIsValid(void);
extern void AVModeDetect(AVModeChange_t * pAVModeChange, AVMode_t * pAVMode);

void ITE6811MhlTxTmdsEnable(void);
void hdmitx_pwron(void);
void hdmitx_pwrdn(void);

#define	MHL_LOGICAL_DEVICE_MAP		( MHL_DEV_LD_VIDEO | MHL_DEV_LD_GUI )
#endif
