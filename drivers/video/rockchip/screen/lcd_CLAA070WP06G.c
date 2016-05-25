#ifndef __LCD_LD089WU1__
#define __LCD_LD089WU1__

#if defined(CONFIG_MIPI_DSI)
#include "../transmitter/mipi_dsi.h"
#endif
#include "linux/delay.h"

#if  defined(CONFIG_RK610_LVDS) || defined(CONFIG_RK616_LVDS)
#define SCREEN_TYPE	    	SCREEN_LVDS
#else
#define SCREEN_TYPE	    	SCREEN_MIPI
#endif
#define LVDS_FORMAT         0     //mipi lcd don't need it, so 0 would be ok.
#define OUT_FACE	    	OUT_P888 //OUT_P888


#define DCLK	          	67*1000000
#define LCDC_ACLK         	300000000           //29 lcdc axi DMA ÆµÂÊ

#define H_VD			800
#define V_VD			1280

#define H_PW			 64 //16
#define H_BP			64 //40
#define H_FP			64 //24

#define V_PW			8//50 //6
#define V_BP			8 // 26
#define V_FP			8 //4

#define LCD_WIDTH          	204
#define LCD_HEIGHT         	136
/* Other */
#if defined(CONFIG_RK610_LVDS) || defined(CONFIG_RK616_LVDS) || defined(CONFIG_MIPI_DSI)
#define DCLK_POL	1
#else
#define DCLK_POL	0
#endif
#define DEN_POL		0
#define VSYNC_POL	0
#define HSYNC_POL	0

#define SWAP_RB		0
#define SWAP_RG		0
#define SWAP_GB		0

#define RK_SCREEN_INIT 	1

/* about mipi */
#define MIPI_DSI_LANE 4
//#define MIPI_DSI_HS_CLK (6 * DCLK)
#define MIPI_DSI_HS_CLK 510*1000000

#if defined(RK_SCREEN_INIT)
static struct rk29lcd_info *gLcd_info = NULL;

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
	msleep(50);

	printk("%s", __FUNCTION__);

	/*below is changeable*/

#if 0
	dsi_enable_hs_clk(1);

	dcs[0] = LPDT;
	dcs[1] = dcs_exit_sleep_mode; 
	dsi_send_dcs_packet(dcs, 2);
	msleep(10);
	dcs[0] = LPDT;
	dcs[1] = dcs_set_display_on;
	dsi_send_dcs_packet(dcs, 2);
	msleep(10);
	dsi_enable_video_mode(1);
#else
	dsi_enable_hs_clk(1);
	dsi_enable_video_mode(0);
	dsi_enable_command_mode(1);
	dcs[0] = dcs_exit_sleep_mode; 
	dsi_send_dcs_packet(dcs, 1);
	msleep(10);
	dcs[0] = dcs_set_display_on;
	dsi_send_dcs_packet(dcs, 1);
	msleep(10);
	dsi_enable_command_mode(0);
	dsi_enable_video_mode(1);
#endif

	//printk("++++++++++++++++%s:%d\n", __func__, __LINE__);

	return 0;
};


int giFlag = 1;
int rk_lcd_standby(u8 enable) {

	u8 dcs[16] = {0};
	if(dsi_is_active() != 1)
		return -1;
		
	if(enable) {

		giFlag = giFlag - 1;
		if( giFlag < 0 )
			return -1;

#if 1
		dcs[0] = LPDT;
		dcs[1] = dcs_set_display_off; 
		dsi_send_dcs_packet(dcs, 2);

		msleep(1);

		dcs[0] = LPDT;
		dcs[1] = dcs_enter_sleep_mode; 
		dsi_send_dcs_packet(dcs, 2);
		msleep(1);
#else
		dsi_enable_video_mode(0);
		dsi_enable_command_mode(1);
		/*below is changeable*/
		dcs[0] = dcs_set_display_off; 
		dsi_send_dcs_packet(dcs, 1);
		msleep(1);
		dcs[0] = dcs_enter_sleep_mode; 
		dsi_send_dcs_packet(dcs, 1);
		msleep(1);
#endif
		printk("++++++++++++++++%s:%d\n", __func__, __LINE__);
	
	} else {
		/*below is changeable*/
		giFlag = giFlag + 1 ;
		rk_lcd_init();
		printk("++++++++++++++++%s:%d\n", __func__, __LINE__);
	
	}

	return 0;
};
#endif

#endif  
