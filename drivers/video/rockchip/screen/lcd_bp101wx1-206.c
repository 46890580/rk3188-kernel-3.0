#ifndef __LCD_Q72_800x1280__
#define __LCD_Q72_800X1280__

#if  defined(CONFIG_RK616_LVDS)
#define SCREEN_TYPE	    	SCREEN_LVDS
#else
#define SCREEN_TYPE	    	SCREEN_RGB
#endif
#define LVDS_FORMAT      	LVDS_8BIT_1
#define OUT_FACE	    	OUT_P888


#define DCLK	          	71100000 // 75000000
#define LCDC_ACLK         	300000000           //29 lcdc axi DMA ÆµÂÊ

/* Timing */
#define H_PW			10
#define H_BP			160
#define H_VD			1280
#define H_FP			16

#define V_PW			3
#define V_BP			23
#define V_VD			800
#define V_FP			12

#define LCD_WIDTH          	216
#define LCD_HEIGHT         	135

/* Other */
#define DCLK_POL	1
#define DEN_POL		0
#define VSYNC_POL	0
#define HSYNC_POL	0

#define SWAP_RB		0
#define SWAP_RG		0
#define SWAP_GB		0

//#define S_DCLK_POL       1
#endif


