
#ifndef __LCD_CLAA101FP08_MIPI__
#define __LCD_CLAA101FP08_MIPI__

#if defined(CONFIG_MIPI_DSI)
#include "../transmitter/mipi_dsi.h"
#endif
#include "linux/delay.h"

/* Base */
#if  defined(CONFIG_RK616_LVDS)
#define SCREEN_TYPE	    	SCREEN_LVDS
#else
#define SCREEN_TYPE	    	SCREEN_RGB
#endif
#define LVDS_FORMAT      	LVDS_8BIT_1

#define OUT_FACE	    OUT_P888

/*
 * ssd2828: pll >= 6 x DCLK
 *          pll = (24M / (MS + 1)) * NS
 *          NS / (MS + 1) = DCLK / 4
 *          62.5 < pll < 125,  FR = b00
 *          126  < pll < 250,  FR = b01
 *          251  < pll < 500,  FR = b10
 *          501  < pll < 1000, FR = b11
 */
#if 0
/*
 * DCLK: 152 M
 * pll:  152 x 6 = 912 M
 * MS:   0
 * NS:   912 / 24 = 38 M, 0x26
 * FR:   b11, 0x3
 * LPD:  912 / 8 / 10 = 11.4
 */
#define DCLK	          150000000
#define PLCR_MS 0
#define PLCR_NS 0x26
#define PLCR_FR 0x3
#define CCR_LPD 12
#elif 1
/*
 * DCLK: 124 M
 * pll:  124 x 6 = 744 M
 * MS:   0
 * NS:   462 / 24 = 31 M, 0x1f
 * FR:   b11, 0x3
 * LPD:  528 / 8 / 6 = 11
 */
#define DCLK	          124000000
#define PLCR_MS 0
#define PLCR_NS 0x1f
#define PLCR_FR 0x3
#define CCR_LPD 9
#else
#define DCLK	          	64000000
#define PLCK_MS 0
#define PLCK_NS 0x10
#define PLCK_FR 0x2
#endif
#define LCDC_ACLK         300000000           //29 lcdc axi DMA ÆµÂÊ



#define H_VD			1920
#define V_VD			1200
#if 0
/* Horizontal blank time typical 120 */
#define H_PW			1
#define H_BP			118
#define H_FP			1

/* Vertical blank time typical 12 */
#define V_PW			1
#define V_BP			10
#define V_FP			1
#else
/* Horizontal blank time typical 120 */
#define H_PW			4
#define H_BP			0
#define H_FP			116

/* Vertical blank time typical 12 */
#define V_PW			5
#define V_BP			3
#define V_FP			4
#endif

#define ssdH_PW			(H_PW + 0)
#define ssdH_BP			(H_BP + 0)
#define ssdH_FP			(H_FP + 0)

#define ssdV_PW			(V_PW + 0)
#define ssdV_BP			(V_BP + 0)
#define ssdV_FP			(V_FP + 0)

#define LCD_WIDTH          	217
#define LCD_HEIGHT         	136
/* Other */
#if defined(CONFIG_RK616_LVDS)
#define DCLK_POL	1
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
/*
 * when use 24M tx_clk, 24 bit RGB, mipi_clk >= pclk
 * mipi_clk = (fin / MS) * NS
 * e.g. pclk = 148.35M, then mipi_clk > 890.1M
 */
static unsigned int regs_after[] = {
	0x00b80000, /* b7~6 VCM, b5~4 VCE, b3~2 VC2, b1~0 VC1 */
	0x00b90000, /* disable pll. b15~14 sys_clk divider(0: devided by 1), b13 sys_clk disable(0:enable), b0 pll enable(0: pwr down) */
	0x00b70050, /* b5(0:clk source is tx_clk:24M external crystal) */
	0x00ba0000 | ((PLCR_FR & 0x03) << 14) | ((PLCR_MS & 0x1f) << 8) | (PLCR_NS & 0xff),
	0x00bb0000 | (CCR_LPD & 0x3f),
	0x00b90001, /* enable pll */
	50,

	0x00c92302, /* hs zero delay, hs prepare delay */
	0x00ca2301, /* clk zero delay, clk prepare delay */
	0x00cb0510, /* clk pre delay, clk prepare delay */
	0x00cc1005, /* clk trail delay, hs trail delay */

	0x00d00000,

	(0x00b10000 | (ssdV_PW << 8) | ssdH_PW),
	(0x00b20000 | (ssdV_BP << 8) | ssdH_BP),
	(0x00b30000 | (ssdH_FP << 8) | ssdH_FP),
	(0x00b40000 | H_VD),
	(0x00b50000 | V_VD),
	(0x00b60000 | (DCLK_POL << 13) | (HSYNC_POL << 14) | (VSYNC_POL << 15) | 3), /* non-burst mode with sync pulse */

	0x00de0003, /* 4 lanes, b11 = 4 lanes */
	0x00d60005,
	0x00b90000, /* disable pll */
	0x00ba0000 | ((PLCR_FR & 0x03) << 14) | ((4) << 8) | (165 & 0xff),
	0x00ba0000 | ((PLCR_FR & 0x03) << 14) | ((PLCR_MS & 0x1f) << 8) | (PLCR_NS & 0xff),
	//0x00ba8664,
	0x00b90001, /* enable pll */
	50,
	0x00b7025b, /* clr-ref external, video is enabled, enter hs mode */
	0x00c00100, /* sw reset */
	100,
};
static unsigned int regs_before[] = {
	0x00de0003,
	0x00b90000, /* disable pll */
	0x00b70350, /* b5(0:clk source is tx_clk:24M external crystal) */
	0x00d60000,
	0x00b80000,
	0x00ba8014, /* set pll 500M */
	0x00bb0005,
	0x00b90001, /* enable pll */
	0x00de0000, /* 1 lane */

	0x00c00100, /* sw reset */
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
