/*
 * Copyright (C) 2012 ROCKCHIP, Inc.
 *
 * author: hhb@rock-chips.com
 * creat date: 2012-04-19
 * route:drivers/video/display/screen/lcd_hj050na_06a.c
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */


#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/rk_fb.h>
#include <mach/gpio.h>
#include <mach/iomux.h>
#include <mach/board.h>
#include <linux/rk_screen.h>
#include "../transmitter/mipi_dsi.h"

/* Base */
#define SCREEN_TYPE	    SCREEN_RGB
#define OUT_FACE	    OUT_P888
#define LVDS_FORMAT		0

#define DCLK	         124*1000000  //50Hz
#define LCDC_ACLK        300000000   //29 lcdc axi DMA

/* Timing */
#define H_PW			50
#define H_BP			30
#define H_VD			1920
#define H_FP			40

#define V_PW			5
#define V_BP			3
#define V_VD			1200
#define V_FP			4

#define LCD_WIDTH		217    //uint mm the lenth of lcd active area
#define LCD_HEIGHT		135
/* Other */
#define DCLK_POL		1
#define DEN_POL		0
#define HSYNC_POL		0
#define VSYNC_POL		0
#define SWAP_RB			0
#define SWAP_GB			0
#define SWAP_RG			0

#define mipi_dsi_init(data)			dsi_set_regs(data, ARRAY_SIZE(data))
#define mipi_dsi_send_dcs_packet(data)		dsi_send_dcs_packet(data, ARRAY_SIZE(data))
#define mipi_dsi_post_init(data)		dsi_set_regs(data, ARRAY_SIZE(data))
#define data_lane  4



static struct rk29lcd_info *gLcd_info = NULL;
int lcd_init(void);
int lcd_standby(u8 enable);


static unsigned int pre_initialize[] = {
	0x00B10000 | ((V_PW & 0Xff) << 8) | (H_PW & 0Xff),
	0x00B20000 | (((V_BP+V_PW) & 0Xff) << 8) | ((H_BP+H_PW) & 0Xff),
	//0x00B20000 | ((V_BP & 0Xff) << 8) | (H_BP & 0Xff),
	0x00B30000 | ((V_FP & 0Xff) << 8) | (H_FP & 0Xff),
	0x00B40000 | H_VD,
	0x00B50000 | V_VD,
	0x00B60000 | (VPF_24BPP) | (VM_BM << 2),     // burst mode 24bits

	0x00de0000 | (data_lane -1),    //4 lanes

	0x00d60000,
	0x00B90000,
	0x00BA822B,   //pll    //816MHz
	0x00BB0003,
	0x00B90001,	
	0x00B80000,	
	0x00B70300,	
};

static unsigned int post_initialize[] = {
#if 1
	0x00B90000,

	0x00BA825C,
	0x00BB0003,
	0x00B90001,
	0x00B60003,
	0x00D60000,
	0x00B80000,
	0x00B7024B,      //software reset ssd2828
#endif
};
/*
static unsigned char dcs_exit_sleep_mode[] = {0x11};
static unsigned char dcs_set_diaplay_on[] = {0x29};
static unsigned char dcs_enter_sleep_mode[] = {0x10};
static unsigned char dcs_set_diaplay_off[] = {0x28};
*/

int lcd_io_init(void)
{
	int ret = 0;
	if(!gLcd_info)
		return -1;
	ret = gpio_request(gLcd_info->reset_pin, NULL);
	if (ret != 0) {
		gpio_free(gLcd_info->reset_pin);
		printk("%s: request LCD_RST_PIN error\n", __func__);
		return -EIO;
	}

	gpio_direction_output(gLcd_info->reset_pin, !GPIO_LOW);
	return ret;
}

int lcd_io_deinit(void)
{
	int ret = 0;
	if(!gLcd_info)
		return -1;
	gpio_direction_input(gLcd_info->reset_pin);
	gpio_free(gLcd_info->reset_pin);
	return ret;
}


int lcd_reset(void) {
	int ret = 0;
	if(!gLcd_info)
		return -1;
	gpio_set_value(gLcd_info->reset_pin, GPIO_LOW);
	msleep(10);
	gpio_set_value(gLcd_info->reset_pin, !GPIO_LOW);
	msleep(2);
	return ret;
}


int lcd_init(void)
{
	u8 dcs[16] = {0};

	dsi_probe_current_chip();

	lcd_reset();	
	msleep(10);
	mipi_dsi_init(pre_initialize);

	dcs[0] = 0x11;
	dsi_send_dcs_packet(dcs, 1);
	msleep(100);
	dcs[0] = 0x29;
	dsi_send_dcs_packet(dcs, 1);
	msleep(1);
	mipi_dsi_post_init(post_initialize);   


	return 0;

}



int lcd_standby(u8 enable)
{
	u8 dcs[16] = {0};	

	if(enable) {

		printk("lcd_standby...\n");
		dcs[0] = 0x28;
		dsi_send_dcs_packet(dcs, 1);
		msleep(2);
		dcs[0] = 0x10;
		dsi_send_dcs_packet(dcs, 1);
		msleep(100);
		dsi_power_off();

	} else {
		dsi_power_up();
		lcd_init();
	}

	return 0;
}
