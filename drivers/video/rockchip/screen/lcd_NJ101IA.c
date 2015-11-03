#ifndef _LCD_NJ101IA__
#define _LCD_NJ101IA__


/* Base */
#define SCREEN_TYPE		SCREEN_LVDS
#define LVDS_FORMAT		LVDS_8BIT_1
#define OUT_FACE		OUT_P888
#define DCLK			71100000
#define LCDC_ACLK        	300000000           //29 lcdc axi DMA ÆµÂÊ

/* Timing */
#define H_PW			10
#define H_BP			50
#define H_VD			1280
#define H_FP			100

#define V_PW			2
#define V_BP			2
#define V_VD			800
#define V_FP			19

#define LCD_WIDTH       	217
#define LCD_HEIGHT      	136

/* Other */
#define DCLK_POL                1
#define DEN_POL			0
#define VSYNC_POL		0
#define HSYNC_POL		0

#define SWAP_RB			0
#define SWAP_RG			0
#define SWAP_GB			0 

#endif
