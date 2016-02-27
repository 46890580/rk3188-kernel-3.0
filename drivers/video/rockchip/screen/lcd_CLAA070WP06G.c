#ifndef __LCD_LD089WU1__
#define __LCD_LD089WU1__

#if defined(CONFIG_MIPI_DSI)
#include "../transmitter/mipi_dsi.h"
#endif
#include "linux/delay.h"

#if  defined(CONFIG_RK610_LVDS) || defined(CONFIG_RK616_LVDS)
#define SCREEN_TYPE	    	SCREEN_LVDS
#else
#define SCREEN_TYPE	    	SCREEN_RGB
#endif
#define LVDS_FORMAT         0     //mipi lcd don't need it, so 0 would be ok.
#define OUT_FACE	    	OUT_P888


#define DCLK	          	64000000
#define LCDC_ACLK         	300000000           //29 lcdc axi DMA ÆµÂÊ

#define H_VD			800
#define V_VD			1280

/* Timing */
#if 0
#define H_PW			8
#define H_BP			0
#define H_FP			0

#define V_PW			8
#define V_BP			0
#define V_FP			0
#else
#define H_PW			8
#define H_BP			0
#define H_FP			0

#define V_PW			8
#define V_BP			0
#define V_FP			0
#endif

#define ssdH_PW			(H_PW + 0)
#define ssdH_BP			(H_BP + 0)
#define ssdH_FP			(H_FP + 0)

#define ssdV_PW			(V_PW + 0)
#define ssdV_BP			(V_BP + 0)
#define ssdV_FP			(V_FP + 0)


#define LCD_WIDTH          	204
#define LCD_HEIGHT         	136
/* Other */
#if defined(CONFIG_RK610_LVDS) || defined(CONFIG_RK616_LVDS) || defined(CONFIG_MIPI_DSI)
#define DCLK_POL	0
#else
#define DCLK_POL	0
#endif
#define DEN_POL		1
#define VSYNC_POL	0
#define HSYNC_POL	0

#define SWAP_RB		0
#define SWAP_RG		0
#define SWAP_GB		0

#define RK_SCREEN_INIT 	1

/* about mipi */
#define MIPI_DSI_LANE 4
#define MIPI_DSI_HS_CLK 850*1000000

#if defined(RK_SCREEN_INIT)
static struct rk29lcd_info *gLcd_info = NULL;


/*********************** wangluheng ***********************/
static unsigned int regs_after[] = {
	0x00b80000,
	0x00b90000,
	0x00b70050,
	0x00ba8010,
	0x00bb0006,
	0x00b90001,
	50,

	0x00c92302,
	0x00ca2301,
	0x00cb0510,
	0x00cc1005,

	0x00d00000,

	(0x00b10000 | (ssdV_PW << 8) | ssdH_PW),
	(0x00b20000 | (ssdV_BP << 8) | ssdH_BP),
	(0x00b30000 | (ssdH_FP << 8) | ssdH_FP),
	(0x00b40000 | H_VD),
	(0x00b50000 | V_VD),
	(0x00b60000 | (DCLK_POL << 13) | (HSYNC_POL << 14) | (VSYNC_POL << 15) | 3),

	0x00de0003,
	0x00d60005,
	0x00b90000,
	0x00bac455,
	//0x00ba8664,
	0x00b90001,
	50,
	0x00b7025b,
	0x00c00100,
	100,
};
static unsigned int regs_before[] = {
	0x00de0003,
	0x00de0003,
	0x00b90000,
	0x00b70350,
	0x00d60000,
	0x00b80000,
	0x00ba8010,
	0x00bb0003,
	0x00b90001,
	0x00de0000,

	0x00c00100,
	100,
	//0x00c92302,
};
/*********************** wangluheng ***********************/

int rk_lcd_init(void) {
	int ret;
	u8 dcs[16] = {0};

	printk("wangluheng: entering %s\n", __FUNCTION__);
	if(dsi_is_active() != 1) {
		ret = dsi_probe_current_chip();
		if(ret) {
			printk("mipi dsi probe fail\n");
			return -ENODEV;
		}	
		//return -1;
	}

	printk("%s", __FUNCTION__);

	msleep(100);
	/*below is changeable*/
	dsi_enable_hs_clk(1);
	dsi_enable_video_mode(0);
	dsi_enable_command_mode(1);

	dsi_set_regs(regs_before, sizeof(regs_before)/sizeof(int));

	dcs[0] = dcs_exit_sleep_mode; 
	dsi_send_dcs_packet(dcs, 1);
	msleep(100);
	dcs[0] = dcs_set_display_on;
	dsi_send_dcs_packet(dcs, 1);

	dsi_set_regs(regs_after, sizeof(regs_after)/sizeof(int));

	msleep(10);
	dsi_enable_command_mode(0);
	dsi_enable_video_mode(1);
	//printk("++++++++++++++++%s:%d\n", __func__, __LINE__);

	return 0;
};



int rk_lcd_standby(u8 enable) {

	u8 dcs[16] = {0};
	//if(dsi_is_active() != 1)
	//	return -1;
		
	if(enable) {
		dsi_enable_video_mode(0);
		dsi_enable_command_mode(1);
		/*below is changeable*/
		dcs[0] = dcs_set_display_off; 
		dsi_send_dcs_packet(dcs, 1);
		msleep(1);
		dcs[0] = dcs_enter_sleep_mode; 
		dsi_send_dcs_packet(dcs, 1);
		msleep(1);
		//printk("++++++++++++++++%s:%d\n", __func__, __LINE__);
		dsi_power_off();
	
	} else {
		/*below is changeable*/
		//dsi_power_up();
		rk_lcd_init();
		//printk("++++++++++++++++%s:%d\n", __func__, __LINE__);
	
	}

	return 0;
};
#endif

#endif  
