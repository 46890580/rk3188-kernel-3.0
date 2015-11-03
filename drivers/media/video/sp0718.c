#include "generic_sensor.h"
/*
 *      Driver Version Note
 *v0.0.1: this driver is compatible with generic_sensor
 *v0.1.1:
 *        add sensor_focus_af_const_pause_usr_cb;
 */
static int version = KERNEL_VERSION(0,1,1);
module_param(version, int, S_IRUGO);



static int debug;
module_param(debug, int, S_IRUGO|S_IWUSR);

#define dprintk(level, fmt, arg...) do {			\
	if (debug >= level) 					\
	printk(KERN_WARNING fmt , ## arg); } while (0)

/* Sensor Driver Configuration Begin */
#define SENSOR_NAME RK29_CAM_SENSOR_SP0718
#define SENSOR_V4L2_IDENT V4L2_IDENT_SP0718
#define SENSOR_ID 0x71
#define SENSOR_BUS_PARAM		(SOCAM_MASTER |\
		SOCAM_PCLK_SAMPLE_RISING|SOCAM_HSYNC_ACTIVE_HIGH| SOCAM_VSYNC_ACTIVE_HIGH|\
		SOCAM_DATA_ACTIVE_HIGH | SOCAM_DATAWIDTH_8  |SOCAM_MCLK_24MHZ)

#define SENSOR_PREVIEW_W					 640
#define SENSOR_PREVIEW_H					 480
#define SENSOR_PREVIEW_FPS					 15000	   // 15fps 
#define SENSOR_FULLRES_L_FPS				 7500	   // 7.5fps
#define SENSOR_FULLRES_H_FPS				 7500	   // 7.5fps
#define SENSOR_720P_FPS 					 0
#define SENSOR_1080P_FPS					 0

#define SENSOR_REGISTER_LEN 				 1		   // sensor register address bytes
#define SENSOR_VALUE_LEN					 1		   // sensor register value bytes

static unsigned int SensorConfiguration = (CFG_WhiteBalance|CFG_Effect|CFG_Scene);
static unsigned int SensorChipID[] = {SENSOR_ID};
/* Sensor Driver Configuration End */


#define SENSOR_NAME_STRING(a) STR(CONS(SENSOR_NAME, a))
#define SENSOR_NAME_VARFUN(a) CONS(SENSOR_NAME, a)

#define SensorRegVal(a,b) CONS4(SensorReg,SENSOR_REGISTER_LEN,Val,SENSOR_VALUE_LEN)(a,b)
#define sensor_write(client,reg,v) CONS4(sensor_write_reg,SENSOR_REGISTER_LEN,val,SENSOR_VALUE_LEN)(client,(reg),(v))
#define sensor_read(client,reg,v) CONS4(sensor_read_reg,SENSOR_REGISTER_LEN,val,SENSOR_VALUE_LEN)(client,(reg),(v))
#define sensor_write_array generic_sensor_write_array

struct sensor_parameter
{
	unsigned int PreviewDummyPixels;
	unsigned int CaptureDummyPixels;
	unsigned int preview_exposure;
	unsigned short int preview_line_width;
	unsigned short int preview_gain;

	unsigned short int PreviewPclk;
	unsigned short int CapturePclk;
	char awb[6];
};

struct specific_sensor{
	struct generic_sensor common_sensor;
	//define user data below
	struct sensor_parameter parameter;

};


#define SP0718_NORMAL_Y0ffset  0x20
#define SP0718_LOWLIGHT_Y0ffset  0x25
//AE target
#define  SP0718_P1_0xeb  0x78
#define  SP0718_P1_0xec  0x6c
//HEQ
#define  SP0718_P1_0x10  0x00//outdoor
#define  SP0718_P1_0x14  0x20
#define  SP0718_P1_0x11  0x00//nr
#define  SP0718_P1_0x15  0x18
#define  SP0718_P1_0x12  0x00//dummy
#define  SP0718_P1_0x16  0x10
#define  SP0718_P1_0x13  0x00//low
#define  SP0718_P1_0x17  0x00

#define FLICK_50HZ 1



/*
 *  The follow setting need been filled.
 *  
 *  Must Filled:
 *  sensor_init_data :				Sensor initial setting;
 *  sensor_fullres_lowfps_data : 	Sensor full resolution setting with best auality, recommand for video;
 *  sensor_preview_data :			Sensor preview resolution setting, recommand it is vga or svga;
 *  sensor_softreset_data :			Sensor software reset register;
 *  sensor_check_id_data :			Sensir chip id register;
 *
 *  Optional filled:
 *  sensor_fullres_highfps_data: 	Sensor full resolution setting with high framerate, recommand for video;
 *  sensor_720p: 					Sensor 720p setting, it is for video;
 *  sensor_1080p:					Sensor 1080p setting, it is for video;
 *
 *  :::::WARNING:::::
 *  The SensorEnd which is the setting end flag must be filled int the last of each setting;
 */

/* Sensor initial setting */
static struct rk_sensor_reg sensor_init_data[] =
{	
	//saturday param test
	{0xfd,0x00},
	{0x1C,0x00},
	{0x31,0x10},
	{0x27,0xb3},//0xb3	//2x gain
	{0x1b,0x17},
	{0x26,0xaa},
	{0x37,0x02},
	{0x28,0x8f},
	{0x1a,0x73},
	{0x1e,0x1b},
	{0x21,0x06},//blackout voltage
	{0x22,0x2a},//colbias
	{0x0f,0x3f},
	{0x10,0x3e},
	{0x11,0x00},
	{0x12,0x01},
	{0x13,0x3f},
	{0x14,0x04},
	{0x15,0x30},
	{0x16,0x31},
	{0x17,0x01},
	{0x69,0x31},
	{0x6a,0x2a},
	{0x6b,0x33},
	{0x6c,0x1a},
	{0x6d,0x32},
	{0x6e,0x28},
	{0x6f,0x29},
	{0x70,0x34},
	{0x71,0x18},
	{0x36,0x00},//02 delete badframe
	{0xfd,0x01},
	{0x5d,0x51},//position
	{0xf2,0x19},

	//Blacklevel
	{0x1f,0x10},
	{0x20,0x1f},
	//pregain 
	{0xfd,0x02},
	{0x00,0x88},
	{0x01,0x88},
	//SI15_SP0718 24M 50Hz 15-8fps 
	//ae setting
	{0xfd,0x00},
	{0x03,0x01},
	{0x04,0xce},
	{0x06,0x00},
	{0x09,0x02},
	{0x0a,0xc4},
	{0xfd,0x01},
	{0xef,0x4d},
	{0xf0,0x00},
	{0x02,0x0c},
	{0x03,0x01},
	{0x06,0x47},
	{0x07,0x00},
	{0x08,0x01},
	{0x09,0x00},
	//Status   
	{0xfd,0x02},
	{0xbe,0x9c},
	{0xbf,0x03},
	{0xd0,0x9c},
	{0xd1,0x03},
	{0xfd,0x01},
	{0x5b,0x03},
	{0x5c,0x9c},

	//rpc
	{0xfd,0x01},
	{0xe0,0x40},//24//4c//48//4c//44//4c//3e//3c//3a//38//rpc_1base_max
	{0xe1,0x30},//24//3c//38//3c//36//3c//30//2e//2c//2a//rpc_2base_max
	{0xe2,0x2e},//24//34//30//34//2e//34//2a//28//26//26//rpc_3base_max
	{0xe3,0x2a},//24//2a//2e//2c//2e//2a//2e//26//24//22//rpc_4base_max
	{0xe4,0x2a},//24//2a//2e//2c//2e//2a//2e//26//24//22//rpc_5base_max
	{0xe5,0x28},//24//2c//2a//2c//28//2c//24//22//20//rpc_6base_max
	{0xe6,0x28},//24//2c//2a//2c//28//2c//24//22//20//rpc_7base_max
	{0xe7,0x26},//24//2a//28//2a//26//2a//22//20//20//1e//rpc_8base_max
	{0xe8,0x26},//24//2a//28//2a//26//2a//22//20//20//1e//rpc_9base_max
	{0xe9,0x26},//24//2a//28//2a//26//2a//22//20//20//1e//rpc_10base_max
	{0xea,0x26},//24//28//26//28//24//28//20//1f//1e//1d//rpc_11base_max
	{0xf3,0x26},//24//28//26//28//24//28//20//1f//1e//1d//rpc_12base_max
	{0xf4,0x26},//24//28//26//28//24//28//20//1f//1e//1d//rpc_13base_max
	//ae gain &status
	{0xfd,0x01},
	{0x04,0xe0},//rpc_max_indr
	{0x05,0x26},//1e//rpc_min_indr 
	{0x0a,0xa0},//rpc_max_outdr
	{0x0b,0x26},//rpc_min_outdr
	{0x5a,0x40},//dp rpc   
	{0xfd,0x02}, 
	{0xbc,0xa0},//rpc_heq_low
	{0xbd,0x80},//rpc_heq_dummy
	{0xb8,0x80},//mean_normal_dummy
	{0xb9,0x90},//mean_dummy_normal

	//ae target
	{0xfd,0x01}, 
	{0xeb,SP0718_P1_0xeb},//78 
	{0xec,SP0718_P1_0xec},//78
	{0xed,0x0a},	
	{0xee,0x10},

	//lsc       
	{0xfd,0x01},
	{0x26,0x30},
	{0x27,0x2c},
	{0x28,0x07},
	{0x29,0x08},
	{0x2a,0x40},
	{0x2b,0x03},
	{0x2c,0x00},
	{0x2d,0x00},

	{0xa1,0x24},
	{0xa2,0x27},
	{0xa3,0x27},
	{0xa4,0x2b},
	{0xa5,0x1c},
	{0xa6,0x1a},
	{0xa7,0x1a},
	{0xa8,0x1a},
	{0xa9,0x18},
	{0xaa,0x1c},
	{0xab,0x17},
	{0xac,0x17},
	{0xad,0x08},
	{0xae,0x08},
	{0xaf,0x08},
	{0xb0,0x00},
	{0xb1,0x00},
	{0xb2,0x00},
	{0xb3,0x00},
	{0xb4,0x00},
	{0xb5,0x02},
	{0xb6,0x06},
	{0xb7,0x00},
	{0xb8,0x00},


	//DP       
	{0xfd,0x01},
	{0x48,0x09},
	{0x49,0x99},

	//awb       
	{0xfd,0x01},
	{0x32,0x05},
	{0xfd,0x00},
	{0xe7,0x03},
	{0xfd,0x02},
	{0x26,0xc8},
	{0x27,0xb6},
	{0xfd,0x00},
	{0xe7,0x00},
	{0xfd,0x02},
	{0x1b,0x80},
	{0x1a,0x80},
	{0x18,0x26},
	{0x19,0x28},
	{0xfd,0x02},
	{0x2a,0x00},
	{0x2b,0x08},
	{0x28,0xef},//0xa0//f8
	{0x29,0x20},

	//d65 90  e2 93
	{0x66,0x42},//0x59//0x60//0x58//4e//0x48
	{0x67,0x62},//0x74//0x70//0x78//6b//0x69
	{0x68,0xee},//0xd6//0xe3//0xd5//cb//0xaa
	{0x69,0x18},//0xf4//0xf3//0xf8//ed
	{0x6a,0xa6},//0xa5
	//indoor 91
	{0x7c,0x3b},//0x45//30//41//0x2f//0x44
	{0x7d,0x5b},//0x70//60//55//0x4b//0x6f
	{0x7e,0x15},//0a//0xed
	{0x7f,0x39},//23//0x28
	{0x80,0xaa},//0xa6
	//cwf   92 
	{0x70,0x3e},//0x38//41//0x3b
	{0x71,0x59},//0x5b//5f//0x55
	{0x72,0x31},//0x30//22//0x28
	{0x73,0x4f},//0x54//44//0x45
	{0x74,0xaa},
	//tl84  93 
	{0x6b,0x1b},//0x18//11
	{0x6c,0x3a},//0x3c//25//0x2f
	{0x6d,0x3e},//0x3a//35
	{0x6e,0x59},//0x5c//46//0x52
	{0x6f,0xaa},
	//f    94
	{0x61,0xea},//0x03//0x00//f4//0xed
	{0x62,0x03},//0x1a//0x25//0f//0f
	{0x63,0x6a},//0x62//0x60//52//0x5d
	{0x64,0x8a},//0x7d//0x85//70//0x75//0x8f
	{0x65,0x6a},//0xaa//6a

	{0x75,0x80},
	{0x76,0x20},
	{0x77,0x00},
	{0x24,0x25},

	//针对室内调偏不过灯箱测试使用//针对人脸调偏
	{0x20,0xd8},
	{0x21,0xa3},//82//a8偏暗照度还有调偏
	{0x22,0xd0},//e3//bc
	{0x23,0x86},

	//outdoor r\b range
	{0x78,0xc3},//d8
	{0x79,0xba},//82
	{0x7a,0xa6},//e3
	{0x7b,0x99},//86


	//skin 
	{0x08,0x15},//
	{0x09,0x04},//
	{0x0a,0x20},//
	{0x0b,0x12},//
	{0x0c,0x27},//
	{0x0d,0x06},//
	{0x0e,0x63},//

	//wt th
	{0x3b,0x10},
	//gw
	{0x31,0x60},
	{0x32,0x60},
	{0x33,0xc0},
	{0x35,0x6f},

	// sharp
	{0xfd,0x02},
	{0xde,0x0f},
	{0xd2,0x02},//6//控制黑白边；0-边粗，f-变细
	{0xd3,0x06},
	{0xd4,0x06},
	{0xd5,0x06},
	{0xd7,0x20},//10//2x根据增益判断轮廓阈值
	{0xd8,0x30},//24//1A//4x
	{0xd9,0x38},//28//8x
	{0xda,0x38},//16x
	{0xdb,0x08},//
	{0xe8,0x58},//48//轮廓强度
	{0xe9,0x48},
	{0xea,0x30},
	{0xeb,0x20},
	{0xec,0x48},//60//80
	{0xed,0x48},//50//60
	{0xee,0x30},
	{0xef,0x20},
	//平坦区域锐化力度
	{0xf3,0x50},
	{0xf4,0x10},
	{0xf5,0x10},
	{0xf6,0x10},
	//dns       
	{0xfd,0x01},
	{0x64,0x44},//沿方向边缘平滑力度  //0-最强，8-最弱
	{0x65,0x22},
	{0x6d,0x04},//8//强平滑（平坦）区域平滑阈值
	{0x6e,0x06},//8
	{0x6f,0x10},
	{0x70,0x10},
	{0x71,0x08},//0d//弱平滑（非平坦）区域平滑阈值	
	{0x72,0x12},//1b
	{0x73,0x1c},//20
	{0x74,0x24},
	{0x75,0x44},//[7:4]平坦区域强度，[3:0]非平坦区域强度；0-最强，8-最弱；
	{0x76,0x02},//46
	{0x77,0x02},//33
	{0x78,0x02},
	{0x81,0x10},//18//2x//根据增益判定区域阈值，低于这个做强平滑、大于这个做弱平滑；
	{0x82,0x20},//30//4x
	{0x83,0x30},//40//8x
	{0x84,0x48},//50//16x
	{0x85,0x0c},//12/8+reg0x81 第二阈值，在平坦和非平坦区域做连接
	{0xfd,0x02},
	{0xdc,0x0f},

	//gamma    
	{0xfd,0x01},
	{0x8b,0x00},//00//00     
	{0x8c,0x0a},//0c//09     
	{0x8d,0x16},//19//17     
	{0x8e,0x1f},//25//24     
	{0x8f,0x2a},//30//33     
	{0x90,0x3c},//44//47     
	{0x91,0x4e},//54//58     
	{0x92,0x5f},//61//64     
	{0x93,0x6c},//6d//70     
	{0x94,0x82},//80//81     
	{0x95,0x94},//92//8f     
	{0x96,0xa6},//a1//9b     
	{0x97,0xb2},//ad//a5     
	{0x98,0xbf},//ba//b0     
	{0x99,0xc9},//c4//ba     
	{0x9a,0xd1},//cf//c4     
	{0x9b,0xd8},//d7//ce     
	{0x9c,0xe0},//e0//d7     
	{0x9d,0xe8},//e8//e1     
	{0x9e,0xef},//ef//ea     
	{0x9f,0xf8},//f7//f5     
	{0xa0,0xff},//ff//ff     
	//CCM      
	{0xfd,0x02},
	{0x15,0xd0},//b>th
	{0x16,0x95},//r<th
	//gc镜头照人脸偏黄
	//!F        
	{0xa0,0x80},//80
	{0xa1,0x00},//00
	{0xa2,0x00},//00
	{0xa3,0x00},//06
	{0xa4,0x8c},//8c
	{0xa5,0xf4},//ed
	{0xa6,0x0c},//0c
	{0xa7,0xf4},//f4
	{0xa8,0x80},//80
	{0xa9,0x00},//00
	{0xaa,0x30},//30
	{0xab,0x0c},//0c 
	//F        
	{0xac,0x8c},
	{0xad,0xf4},
	{0xae,0x00},
	{0xaf,0xed},
	{0xb0,0x8c},
	{0xb1,0x06},
	{0xb2,0xf4},
	{0xb3,0xf4},
	{0xb4,0x99},
	{0xb5,0x0c},
	{0xb6,0x03},
	{0xb7,0x0f},

	//sat u     
	{0xfd,0x01},
	{0xd3,0x9c},//0x88//50
	{0xd4,0x98},//0x88//50
	{0xd5,0x8c},//50
	{0xd6,0x84},//50
	//sat v   
	{0xd7,0x9c},//0x88//50
	{0xd8,0x98},//0x88//50
	{0xd9,0x8c},//50
	{0xda,0x84},//50
	//auto_sat  
	{0xdd,0x30},
	{0xde,0x10},
	{0xd2,0x01},//autosa_en
	{0xdf,0xff},//a0//y_mean_th

	//uv_th     
	{0xfd,0x01},
	{0xc2,0xaa},
	{0xc3,0xaa},
	{0xc4,0x66},
	{0xc5,0x66}, 

	//heq
	{0xfd,0x01},
	{0x0f,0xff},
	{0x10,SP0718_P1_0x10}, //out
	{0x14,SP0718_P1_0x14}, 
	{0x11,SP0718_P1_0x11}, //nr
	{0x15,SP0718_P1_0x15},  
	{0x12,SP0718_P1_0x12}, //dummy
	{0x16,SP0718_P1_0x16}, 
	{0x13,SP0718_P1_0x13}, //low 	
	{0x17,SP0718_P1_0x17},   	

	{0xfd,0x01},
	{0xcd,0x20},
	{0xce,0x1f},
	{0xcf,0x20},
	{0xd0,0x55},  
	//auto 
	{0xfd,0x01},
	{0xfb,0x33},
	{0x32,0x15},
	{0x33,0xff},
	{0x34,0xe7},
	{0x35,0x40},	  	

	SensorEnd            
};
/* Senor full resolution setting: recommand for capture */
static struct rk_sensor_reg sensor_fullres_lowfps_data[] ={
	SensorEnd
};

/* Senor full resolution setting: recommand for video */
static struct rk_sensor_reg sensor_fullres_highfps_data[] ={
	SensorEnd
};
/* Preview resolution setting*/
static struct rk_sensor_reg sensor_preview_data[] =
{
	SensorEnd
};
/* 1280x720 */
static struct rk_sensor_reg sensor_720p[]={
	SensorEnd
};

/* 1920x1080 */
static struct rk_sensor_reg sensor_1080p[]={
	SensorEnd
};


static struct rk_sensor_reg sensor_softreset_data[]={
	SensorEnd
};

static struct rk_sensor_reg sensor_check_id_data[]={
	SensorRegVal(0x02,0),
	SensorEnd
};
/*
 *  The following setting must been filled, if the function is turn on by CONFIG_SENSOR_xxxx
 */
static struct rk_sensor_reg sensor_WhiteB_Auto[]=
{
	{0xfd,0x02},                      
	{0x26,0xc8},		                  
	{0x27,0xb6},                      
	{0xfd,0x01}, 		
	{0x32,0x15},   //awb & ae  opened
	{0xfd,0x00},  
	SensorEnd 
};
/* Cloudy Colour Temperature : 6500K - 8000K  */
static	struct rk_sensor_reg sensor_WhiteB_Cloudy[]=
{
	{0xfd,0x01}, 
	{0x32,0x05},          
	{0xfd,0x02},          
	{0x26,0xc8},	        
	{0x27,0x89},	        
	{0xfd,0x00},		
	SensorEnd 
};
/* ClearDay Colour Temperature : 5000K - 6500K	*/
static	struct rk_sensor_reg sensor_WhiteB_ClearDay[]=
{
	{0xfd,0x01}, 
	{0x32,0x05},           
	{0xfd,0x02},           
	{0x26,0xaa},	         
	{0x27,0xce},	         
	{0xfd,0x00}, 		
	SensorEnd 
};
/* Office Colour Temperature : 3500K - 5000K  */
static	struct rk_sensor_reg sensor_WhiteB_TungstenLamp1[]=
{
	{0xfd,0x01},  
	{0x32,0x05},                  
	{0xfd,0x02},                  
	{0x26,0x91},		              
	{0x27,0xc8},		              
	{0xfd,0x00},		
	SensorEnd 
};
/* Home Colour Temperature : 2500K - 3500K	*/
static	struct rk_sensor_reg sensor_WhiteB_TungstenLamp2[]=
{
	{0xfd,0x01}, 
	{0x32,0x05},                 
	{0xfd,0x02},                 
	{0x26,0x75},		             
	{0x27,0xe2},		             
	{0xfd,0x00}, 
	SensorEnd 
};
static struct rk_sensor_reg *sensor_WhiteBalanceSeqe[] = {sensor_WhiteB_Auto, sensor_WhiteB_TungstenLamp1,sensor_WhiteB_TungstenLamp2,
	sensor_WhiteB_ClearDay, sensor_WhiteB_Cloudy,NULL,
};

static	struct rk_sensor_reg sensor_Brightness0[]=
{
	// Brightness -2
	{0xfd,0x01},
	{0xdb,0xe0},//level -2
	SensorEnd
};

static	struct rk_sensor_reg sensor_Brightness1[]=
{
	// Brightness -1
	{0xfd,0x01},
	{0xdb,0xf0},//level -1
	SensorEnd

};

static	struct rk_sensor_reg sensor_Brightness2[]=
{
	//  Brightness 0
	{0xfd,0x01},
	{0xdb,0x00},//level 0
	SensorEnd
};

static	struct rk_sensor_reg sensor_Brightness3[]=
{
	// Brightness +1
	{0xfd,0x01},
	{0xdb,0x10},//level +1
	SensorEnd
};

static	struct rk_sensor_reg sensor_Brightness4[]=
{
	//  Brightness +2
	{0xfd,0x01},
	{0xdb,0x20},//level +2
	SensorEnd
};

static	struct rk_sensor_reg sensor_Brightness5[]=
{
	//  Brightness +3
	{0xfd,0x01},
	{0xdb,0x30},//level +3
	SensorEnd

};
static struct rk_sensor_reg *sensor_BrightnessSeqe[] = {sensor_Brightness0, sensor_Brightness1, sensor_Brightness2, sensor_Brightness3,
	sensor_Brightness4, sensor_Brightness5,NULL,
};

static	struct rk_sensor_reg sensor_Effect_Normal[] =
{
	{0xfd,0x01},
	{0x66,0x00},
	{0x67,0x80},
	{0x68,0x80},
	SensorEnd
};

static	struct rk_sensor_reg sensor_Effect_WandB[] =
{
	{0xfd,0x01},
	{0x66,0x20},
	{0x67,0x80},
	{0x68,0x80},
	SensorEnd
};

static	struct rk_sensor_reg sensor_Effect_Sepia[] =
{
	{0xfd,0x01},
	{0x66,0x10},
	{0x67,0xc0},
	{0x68,0x20},
	SensorEnd
};

static	struct rk_sensor_reg sensor_Effect_Negative[] =
{
	//Negative
	{0xfd,0x01},
	{0x66,0x04},
	{0x67,0x80},
	{0x68,0x80},
	SensorEnd
};
static	struct rk_sensor_reg sensor_Effect_Bluish[] =
{
	// Bluish
	{0xfd,0x01},
	{0x66,0x10},
	{0x67,0x20},
	{0x68,0xf0},
	SensorEnd
};

static	struct rk_sensor_reg sensor_Effect_Green[] =
{
	//  Greenish
	{0xfd,0x01},
	{0x66,0x10},
	{0x67,0x20},
	{0x68,0x20},
	SensorEnd
};
static struct rk_sensor_reg *sensor_EffectSeqe[] = {sensor_Effect_Normal, sensor_Effect_WandB, sensor_Effect_Negative,sensor_Effect_Sepia,
	sensor_Effect_Bluish, sensor_Effect_Green,NULL,
};

static	struct rk_sensor_reg sensor_Exposure0[]=
{
	//level -3   
	{0xfd,0x01},  
	{0xeb,SP0718_P1_0xeb-0x18},
	{0xec,SP0718_P1_0xec-0x18},	
	{0xfd,0x00},
	SensorEnd
};

static	struct rk_sensor_reg sensor_Exposure1[]=
{
	//level -2   
	{0xfd,0x01},   
	{0xeb,SP0718_P1_0xeb-0x10},
	{0xec,SP0718_P1_0xec-0x10},	
	{0xfd, 0x00},
	SensorEnd
};

static	struct rk_sensor_reg sensor_Exposure2[]=
{
	//level -1   
	{0xfd,0x01},   
	{0xeb,SP0718_P1_0xeb-0x08},
	{0xec,SP0718_P1_0xec-0x08},	
	{0xfd, 0x00},
	SensorEnd
};

static	struct rk_sensor_reg sensor_Exposure3[]=
{
	//level 0   
	{0xfd,0x01},   
	{0xeb,SP0718_P1_0xeb},
	{0xec,SP0718_P1_0xec},	
	{0xfd, 0x00},
	SensorEnd
};

static	struct rk_sensor_reg sensor_Exposure4[]=
{
	//level +1   
	{0xfd,0x01},   
	{0xeb,SP0718_P1_0xeb+0x08},
	{0xec,SP0718_P1_0xec+0x08},	
	{0xfd, 0x00},
	SensorEnd
};

static	struct rk_sensor_reg sensor_Exposure5[]=
{
	//level +2   
	{0xfd,0x01},   
	{0xeb,SP0718_P1_0xeb+0x10},
	{0xec,SP0718_P1_0xec+0x10},	
	{0xfd, 0x00},
	SensorEnd
};

static	struct rk_sensor_reg sensor_Exposure6[]=
{
	//level +3   
	{0xfd,0x01},   
	{0xeb,SP0718_P1_0xeb+0x18},
	{0xec,SP0718_P1_0xec+0x18},	
	{0xfd, 0x00},
	SensorEnd
};

static struct rk_sensor_reg *sensor_ExposureSeqe[] = {sensor_Exposure0, sensor_Exposure1, sensor_Exposure2, sensor_Exposure3,
	sensor_Exposure4, sensor_Exposure5,sensor_Exposure6,NULL,
};

static	struct rk_sensor_reg sensor_Saturation0[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Saturation1[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Saturation2[]=
{
	SensorEnd
};
static struct rk_sensor_reg *sensor_SaturationSeqe[] = {sensor_Saturation0, sensor_Saturation1, sensor_Saturation2, NULL,};

static	struct rk_sensor_reg sensor_Contrast0[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Contrast1[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Contrast2[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Contrast3[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Contrast4[]=
{
	SensorEnd
};


static	struct rk_sensor_reg sensor_Contrast5[]=
{
	SensorEnd
};

static	struct rk_sensor_reg sensor_Contrast6[]=
{
	SensorEnd
};
static struct rk_sensor_reg *sensor_ContrastSeqe[] = {sensor_Contrast0, sensor_Contrast1, sensor_Contrast2, sensor_Contrast3,
	sensor_Contrast4, sensor_Contrast5, sensor_Contrast6, NULL,
};
static	struct rk_sensor_reg sensor_SceneAuto[] =
{
	{0xfd,0x01},
	{0xcd,SP0718_NORMAL_Y0ffset},
	{0xce,0x1f},
#if 1
#if FLICK_50HZ
	//SI15_SP0718 24M 50Hz 15-8fps
	//ae setting
	{0xfd,0x00},
	{0x03,0x01},
	{0x04,0xce},
	{0x06,0x00},
	{0x09,0x02},
	{0x0a,0xc4},
	{0xfd,0x01},
	{0xef,0x4d},
	{0xf0,0x00},
	{0x02,0x0c},
	{0x03,0x01},
	{0x06,0x47},
	{0x07,0x00},
	{0x08,0x01},
	{0x09,0x00},
	//Status   
	{0xfd,0x02},
	{0xbe,0x9c},
	{0xbf,0x03},
	{0xd0,0x9c},
	{0xd1,0x03},
	{0xfd,0x01},
	{0x5b,0x03},
	{0x5c,0x9c},
	//{0xfd , 0x01},
	//{0x32 , 0x15},
#else
	//SI15_SP0718 24M 60Hz 15-8fps
	//ae setting
	{0xfd,0x00},
	{0x03,0x01},
	{0x04,0x80},
	{0x06,0x00},
	{0x09,0x02},
	{0x0a,0xc9},
	{0xfd,0x01},
	{0xef,0x40},
	{0xf0,0x00},
	{0x02,0x0f},
	{0x03,0x01},
	{0x06,0x3a},
	{0x07,0x00},
	{0x08,0x01},
	{0x09,0x00},
	//Statu    
	{0xfd,0x02},
	{0xbe,0xc0},
	{0xbf,0x03},
	{0xd0,0xc0},
	{0xd1,0x03},
	{0xfd,0x01},
	{0x5b,0x03},
	{0x5c,0xc0},
	//{0xfd , 0x01},
	//{0x32 , 0x15},
#endif
#endif
	SensorEnd
};

static	struct rk_sensor_reg sensor_SceneNight[] =
{
	{0xfd,0x01},
	{0xcd,SP0718_LOWLIGHT_Y0ffset},
	{0xce,0x1f},
#if 1
#if FLICK_50HZ
	//SI15_SP0718 24M 50Hz 6-15fps
	//ae setting
	{0xfd,0x00},
	{0x03,0x01},
	{0x04,0xce},
	{0x06,0x00},
	{0x09,0x02},
	{0x0a,0xc4},
	{0xfd,0x01},
	{0xef,0x4d},
	{0xf0,0x00},
	{0x02,0x10},
	{0x03,0x01},
	{0x06,0x47},
	{0x07,0x00},
	{0x08,0x01},
	{0x09,0x00},
	//Statu    
	{0xfd,0x02},
	{0xbe,0xd0},
	{0xbf,0x04},
	{0xd0,0xd0},
	{0xd1,0x04},
	{0xfd,0x01},
	{0x5b,0x04},
	{0x5c,0xd0}, 
	//{0xfd , 0x01},
    //{0x32 , 0x15},

#else
	//SI15_SP0718 24M 60Hz 6-15fps
	//ae setting
	{0xfd,0x00},
	{0x03,0x01},
	{0x04,0x80},
	{0x06,0x00},
	{0x09,0x02},
	{0x0a,0xc9},
	{0xfd,0x01},
	{0xef,0x40},
	{0xf0,0x00},
	{0x02,0x14},
	{0x03,0x01},
	{0x06,0x3a},
	{0x07,0x00},
	{0x08,0x01},
	{0x09,0x00},
	//Statu    
	{0xfd,0x02},
	{0xbe,0x00},
	{0xbf,0x05},
	{0xd0,0x00},
	{0xd1,0x05},
	{0xfd,0x01},
	{0x5b,0x05},
	{0x5c,0x00},
    //{0xfd , 0x01},
    //{0x32 , 0x15},
#endif
#endif
	SensorEnd
};
static struct rk_sensor_reg *sensor_SceneSeqe[] = {sensor_SceneAuto, sensor_SceneNight,NULL,};

static struct rk_sensor_reg sensor_Zoom0[] =
{
	SensorEnd
};

static struct rk_sensor_reg sensor_Zoom1[] =
{
	SensorEnd
};

static struct rk_sensor_reg sensor_Zoom2[] =
{
	SensorEnd
};


static struct rk_sensor_reg sensor_Zoom3[] =
{
	SensorEnd
};
static struct rk_sensor_reg *sensor_ZoomSeqe[] = {sensor_Zoom0, sensor_Zoom1, sensor_Zoom2, sensor_Zoom3, NULL,};

/*
 * User could be add v4l2_querymenu in sensor_controls by new_usr_v4l2menu
 */
static struct v4l2_querymenu sensor_menus[] =
{
};
/*
 * User could be add v4l2_queryctrl in sensor_controls by new_user_v4l2ctrl
 */
static struct sensor_v4l2ctrl_usr_s sensor_controls[] =
{
};

//MUST define the current used format as the first item   
static struct rk_sensor_datafmt sensor_colour_fmts[] = {
	{V4L2_MBUS_FMT_YUYV8_2X8, V4L2_COLORSPACE_JPEG} 
};
static struct soc_camera_ops sensor_ops;


/*
 **********************************************************
 * Following is local code:
 * 
 * Please codeing your program here 
 **********************************************************
 */
/*
 **********************************************************
 * Following is callback
 * If necessary, you could coding these callback
 **********************************************************
 */
/*
 * the function is called in open sensor  
 */
static int sensor_activate_cb(struct i2c_client *client)
{	
	return 0;
}
/*
 * the function is called in close sensor
 */
static int sensor_deactivate_cb(struct i2c_client *client)
{	
	return 0;
}
/*
 * the function is called before sensor register setting in VIDIOC_S_FMT  
 */
static int sensor_s_fmt_cb_th(struct i2c_client *client,struct v4l2_mbus_framefmt *mf, bool capture)
{
	return 0;
}
/*
 * the function is called after sensor register setting finished in VIDIOC_S_FMT  
 */
static int sensor_s_fmt_cb_bh (struct i2c_client *client,struct v4l2_mbus_framefmt *mf, bool capture)
{
	return 0;
}
static int sensor_try_fmt_cb_th(struct i2c_client *client,struct v4l2_mbus_framefmt *mf)
{
	return 0;
}

static int sensor_softrest_usr_cb(struct i2c_client *client,struct rk_sensor_reg *series)
{

	return 0;
}
static int sensor_check_id_usr_cb(struct i2c_client *client,struct rk_sensor_reg *series)
{
	return 0;
}
static int sensor_suspend(struct soc_camera_device *icd, pm_message_t pm_msg)
{
	//struct i2c_client *client = to_i2c_client(to_soc_camera_control(icd));

	if (pm_msg.event == PM_EVENT_SUSPEND) {
		SENSOR_DG("Suspend");

	} else {
		SENSOR_TR("pm_msg.event(0x%x) != PM_EVENT_SUSPEND\n",pm_msg.event);
		return -EINVAL;
	}
	return 0;
}

static int sensor_resume(struct soc_camera_device *icd)
{

	SENSOR_DG("Resume");

	return 0;

}
static int sensor_mirror_cb (struct i2c_client *client, int mirror)
{
	char val;
	int err = 0;

	SENSOR_DG("mirror: %d",mirror);
	if (mirror) {

		sensor_write(client, 0xfd, 0x00);
		err = sensor_read(client, 0x31, &val);
		if (err == 0) {
			if((val & 0x20) == 0)
				err = sensor_write(client, 0x31, (val |0x20));
			else 
				err = sensor_write(client, 0x31, (val & 0xdf));
		}
	} else {
		//do nothing
	}

	return err;    
}
/*
 * the function is v4l2 control V4L2_CID_HFLIP callback	
 */
static int sensor_v4l2ctrl_mirror_cb(struct soc_camera_device *icd, struct sensor_v4l2ctrl_info_s *ctrl_info, 
		struct v4l2_ext_control *ext_ctrl)
{
	struct i2c_client *client = to_i2c_client(to_soc_camera_control(icd));

	if (sensor_mirror_cb(client,ext_ctrl->value) != 0)
		SENSOR_TR("sensor_mirror failed, value:0x%x",ext_ctrl->value);

	SENSOR_DG("sensor_mirror success, value:0x%x",ext_ctrl->value);
	return 0;
}

static int sensor_flip_cb(struct i2c_client *client, int flip)
{
	char val;
	int err = 0;	

	SENSOR_DG("flip: %d",flip);
	if (flip) {		
		sensor_write(client, 0xfd, 0x00);
		err = sensor_read(client, 0x31, &val);
		if (err == 0) {
			if((val & 0x40) == 0)
				err = sensor_write(client, 0x31, (val |0x40));
			else 
				err = sensor_write(client, 0x31, (val & 0xbf));
		}
	} else {
		//do nothing
	}

	return err;    
}
/*
 * the function is v4l2 control V4L2_CID_VFLIP callback	
 */
static int sensor_v4l2ctrl_flip_cb(struct soc_camera_device *icd, struct sensor_v4l2ctrl_info_s *ctrl_info, 
		struct v4l2_ext_control *ext_ctrl)
{
	struct i2c_client *client = to_i2c_client(to_soc_camera_control(icd));

	if (sensor_flip_cb(client,ext_ctrl->value) != 0)
		SENSOR_TR("sensor_flip failed, value:0x%x",ext_ctrl->value);

	SENSOR_DG("sensor_flip success, value:0x%x",ext_ctrl->value);
	return 0;
}
/*
 * the functions are focus callbacks
 */
static int sensor_focus_init_usr_cb(struct i2c_client *client){
	return 0;
}

static int sensor_focus_af_single_usr_cb(struct i2c_client *client){
	return 0;
}

static int sensor_focus_af_near_usr_cb(struct i2c_client *client){
	return 0;
}

static int sensor_focus_af_far_usr_cb(struct i2c_client *client){
	return 0;
}

static int sensor_focus_af_specialpos_usr_cb(struct i2c_client *client,int pos){
	return 0;
}

static int sensor_focus_af_const_usr_cb(struct i2c_client *client){
	return 0;
}
static int sensor_focus_af_const_pause_usr_cb(struct i2c_client *client)
{
	return 0;
}
static int sensor_focus_af_close_usr_cb(struct i2c_client *client){
	return 0;
}

static int sensor_focus_af_zoneupdate_usr_cb(struct i2c_client *client){
	return 0;
}

/*
   face defect call back
   */
static int	sensor_face_detect_usr_cb(struct i2c_client *client,int on){
	return 0;
}

/*
 *	The function can been run in sensor_init_parametres which run in sensor_probe, so user can do some
 * initialization in the function. 
 */
static void sensor_init_parameters_user(struct specific_sensor* spsensor,struct soc_camera_device *icd)
{
	return;
}

/*
 * :::::WARNING:::::
 * It is not allowed to modify the following code
 */

sensor_init_parameters_default_code();

sensor_v4l2_struct_initialization();

sensor_probe_default_code();

sensor_remove_default_code();

sensor_driver_default_module_code();
