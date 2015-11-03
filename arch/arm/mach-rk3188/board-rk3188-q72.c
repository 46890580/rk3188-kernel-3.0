/*
 *
 * Copyright (C) 2012 ROCKCHIP, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/skbuff.h>
#include <linux/spi/spi.h>
#include <linux/mmc/host.h>
#include <linux/ion.h>
#include <linux/cpufreq.h>
#include <linux/clk.h>
#include <mach/dvfs.h>

#include <asm/setup.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/flash.h>
#include <asm/hardware/gic.h>

#include <mach/board.h>
#include <mach/hardware.h>
#include <mach/io.h>
#include <mach/gpio.h>
#include <mach/iomux.h>
#include <linux/rk_fb.h>
#include <linux/regulator/machine.h>
#include <linux/rfkill-rk.h>
#include <linux/sensor-dev.h>
#include <linux/mfd/tps65910.h>
#include <linux/regulator/act8846.h>
#include <linux/mfd/rk808.h>
#include <linux/regulator/rk29-pwm-regulator.h>
#include <plat/ddr.h>

#if defined(CONFIG_MFD_RK616)
#include <linux/mfd/rk616.h>
#endif
#if defined(CONFIG_RK_HDMI)
#include "../../../drivers/video/rockchip/hdmi/rk_hdmi.h"
#endif

#if defined(CONFIG_SPIM_RK29)
#include "../../../drivers/spi/rk29_spim.h"
#endif
#if defined(CONFIG_ANDROID_TIMED_GPIO)
#include "../../../drivers/staging/android/timed_gpio.h"
#endif

#if defined(CONFIG_BOARD_ID)
#include <linux/board-id.h>
#endif

#include "board-rk3188-q72-camera.c"

static struct spi_board_info board_spi_devices[] = {
};

/* GSL3670 touchpad */
#if defined (CONFIG_TOUCHSCREEN_GSL3670)

#define TOUCH_RESET_PIN         RK30_PIN0_PB6
#define TOUCH_INT_PIN           RK30_PIN1_PB7

static int gsl3670_init_platform_hw(void)
{
	printk(">>>> gsl3670_init_platform_hw <<<<\n");
	if(gpio_request(TOUCH_RESET_PIN,NULL) != 0){
		gpio_free(TOUCH_RESET_PIN);
		printk("gsl3670_init_platform_hw gpio_request(rst) error\n");
		return -EIO;
	}

	if(gpio_request(TOUCH_INT_PIN,NULL) != 0){
		gpio_free(TOUCH_INT_PIN);
		printk("gsl3670_init_platform_hw gpio_request(int) error\n");
		return -EIO;
	}

	gpio_direction_output(TOUCH_RESET_PIN, 0);
	gpio_set_value(TOUCH_RESET_PIN,GPIO_LOW);
	mdelay(10);
	gpio_direction_input(TOUCH_INT_PIN);
	mdelay(10);
	gpio_set_value(TOUCH_RESET_PIN,GPIO_HIGH);
	msleep(300);

	return 0;
}

static struct ts_hw_data gsl3670_info = {
	.reset_gpio	= TOUCH_RESET_PIN,
	.touch_en_gpio	= TOUCH_INT_PIN,
	.init_platform_hw = gsl3670_init_platform_hw,
};

#endif

/***********************************************************
 *	rk30  backlight
 ************************************************************/
#ifdef CONFIG_BACKLIGHT_RK29_BL
#define PWM_ID            3
#define PWM_MODE          PWM3
#define PWM_EFFECT_VALUE  1

#define LCD_DISP_ON_PIN

#ifdef  LCD_DISP_ON_PIN
#define BL_EN_PIN         RK30_PIN0_PA2
#define BL_EN_VALUE       GPIO_HIGH
#endif
static int rk29_backlight_io_init(void)
{
	int ret = 0;

	iomux_set(PWM_MODE);
#ifdef  LCD_DISP_ON_PIN
	ret = gpio_request(BL_EN_PIN, "bl_en");
	if (ret == 0) {
		gpio_direction_output(BL_EN_PIN, BL_EN_VALUE);
	}
#endif
	return ret;
}

static int rk29_backlight_io_deinit(void)
{
	int ret = 0, pwm_gpio;
#ifdef  LCD_DISP_ON_PIN
	gpio_free(BL_EN_PIN);
#endif
	pwm_gpio = iomux_mode_to_gpio(PWM_MODE);
	gpio_request(pwm_gpio, "bl_pwm");
	gpio_direction_output(pwm_gpio, GPIO_LOW);
	return ret;
}

static int rk29_backlight_pwm_suspend(void)
{
	int ret, pwm_gpio = iomux_mode_to_gpio(PWM_MODE);

	ret = gpio_request(pwm_gpio, "bl_pwm");
	if (ret) {
		printk("func %s, line %d: request gpio fail\n", __FUNCTION__, __LINE__);
		return ret;
	}
	gpio_direction_output(pwm_gpio, GPIO_LOW);
#ifdef  LCD_DISP_ON_PIN
	gpio_direction_output(BL_EN_PIN, !BL_EN_VALUE);
#endif
	return ret;
}

static int rk29_backlight_pwm_resume(void)
{
	int pwm_gpio = iomux_mode_to_gpio(PWM_MODE);

	gpio_free(pwm_gpio);
	iomux_set(PWM_MODE);
#ifdef  LCD_DISP_ON_PIN
	msleep(30);
	gpio_direction_output(BL_EN_PIN, BL_EN_VALUE);
#endif
	return 0;
}

static struct rk29_bl_info rk29_bl_info = {
	.pwm_id = PWM_ID,
	.min_brightness=20,
	.max_brightness=255,
	.brightness_mode =BRIGHTNESS_MODE_CONIC,
	.bl_ref = PWM_EFFECT_VALUE,
	.io_init = rk29_backlight_io_init,
	.io_deinit = rk29_backlight_io_deinit,
	.pwm_suspend = rk29_backlight_pwm_suspend,
	.pwm_resume = rk29_backlight_pwm_resume,
};

static struct platform_device rk29_device_backlight = {
	.name	= "rk29_backlight",
	.id 	= -1,
	.dev	= {
		.platform_data  = &rk29_bl_info,
	}
};

#endif

/*MMA8452 gsensor*/
#if defined (CONFIG_GS_MMA8452)
#define MMA8452_INT_PIN   RK30_PIN0_PB7

static int mma8452_init_platform_hw(void)
{
	return 0;
}

static struct sensor_platform_data mma8452_info = {
	.type = SENSOR_TYPE_ACCEL,
	.irq_enable = 1,
	.poll_delay_ms = 30,
	.init_platform_hw = mma8452_init_platform_hw,
#if defined (CONFIG_ANDROID_KITKAT)       
	.orientation = {0, 1, 0, -1, 0, 0, 0, 0, 1},
#else
	.orientation = {-1, 0, 0, 0, -1, 0, 0, 0, 1},
#endif
};
#endif

#ifdef CONFIG_RK_HDMI
#define RK_HDMI_RST_PIN 			RK30_PIN3_PB2
static int rk_hdmi_power_init(void)
{
	int ret;

	if(RK_HDMI_RST_PIN != INVALID_GPIO)
	{
		if (gpio_request(RK_HDMI_RST_PIN, NULL)) {
			printk("func %s, line %d: request gpio fail\n", __FUNCTION__, __LINE__);
			return -1;
		}
		gpio_direction_output(RK_HDMI_RST_PIN, GPIO_LOW);
		gpio_set_value(RK_HDMI_RST_PIN, GPIO_LOW);
		msleep(100);
		gpio_set_value(RK_HDMI_RST_PIN, GPIO_HIGH);
		msleep(50);
	}
	return 0;
}
static struct rk_hdmi_platform_data rk_hdmi_pdata = {
	.io_init = rk_hdmi_power_init,
};
#endif


#ifdef CONFIG_FB_ROCKCHIP

#define LCD_CS_PIN         INVALID_GPIO
#define LCD_CS_VALUE       GPIO_HIGH

#define LCD_EN_PIN         RK30_PIN0_PB0
#define LCD_EN_VALUE       GPIO_LOW

static int rk_fb_io_init(struct rk29_fb_setting_info *fb_setting)
{
	int ret = 0;

	if(LCD_CS_PIN !=INVALID_GPIO)
	{
		ret = gpio_request(LCD_CS_PIN, NULL);
		if (ret != 0)
		{
			gpio_free(LCD_CS_PIN);
			printk(KERN_ERR "request lcd cs pin fail!\n");
			return -1;
		}
		else
		{
			gpio_direction_output(LCD_CS_PIN, LCD_CS_VALUE);
		}
	}

	if(LCD_EN_PIN !=INVALID_GPIO)
	{
		ret = gpio_request(LCD_EN_PIN, NULL);
		if (ret != 0)
		{
			gpio_free(LCD_EN_PIN);
			printk(KERN_ERR "request lcd en pin fail!\n");
			return -1;
		}
		else
		{
			gpio_direction_output(LCD_EN_PIN, LCD_EN_VALUE);
		}
	}
	return 0;
}
static int rk_fb_io_disable(void)
{
	if(LCD_CS_PIN !=INVALID_GPIO)
	{
		gpio_set_value(LCD_CS_PIN, !LCD_CS_VALUE);
	}
	if(LCD_EN_PIN !=INVALID_GPIO)
	{
		gpio_set_value(LCD_EN_PIN, !LCD_EN_VALUE);
	}
	return 0;
}
static int rk_fb_io_enable(void)
{
	if(LCD_CS_PIN !=INVALID_GPIO)
	{
		gpio_set_value(LCD_CS_PIN, LCD_CS_VALUE);
	}
	if(LCD_EN_PIN !=INVALID_GPIO)
	{
		gpio_set_value(LCD_EN_PIN, LCD_EN_VALUE);
	}
	return 0;
}

#if defined(CONFIG_LCDC0_RK3066B) || defined(CONFIG_LCDC0_RK3188)
struct rk29fb_info lcdc0_screen_info = {
#if defined(CONFIG_RK_HDMI)
	.prop		= EXTEND,       //extend display device
	.lcd_info	= NULL,
	.set_screen_info = hdmi_init_lcdc,
#endif
};
#endif

#if defined(CONFIG_LCDC1_RK3066B) || defined(CONFIG_LCDC1_RK3188)
struct rk29fb_info lcdc1_screen_info = {
	.prop		= PRMRY,                //primary display device
	.io_init	= rk_fb_io_init,
	.io_disable	= rk_fb_io_disable,
	.io_enable	= rk_fb_io_enable,
	.set_screen_info = set_lcd_info,
};
#endif

static struct resource resource_fb[] = {
	[0] = {
		.name  = "fb0 buf",
		.start = 0,
		.end   = 0,//RK30_FB0_MEM_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.name  = "ipp buf",  //for rotate
		.start = 0,
		.end   = 0,//RK30_FB0_MEM_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[2] = {
		.name  = "fb2 buf",
		.start = 0,
		.end   = 0,//RK30_FB0_MEM_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
};

static struct platform_device device_fb = {
	.name		= "rk-fb",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(resource_fb),
	.resource	= resource_fb,
};
#endif
#if defined(CONFIG_ARCH_RK3188)
static struct resource resource_mali[] = {
	[0] = {
		.name  = "ump buf",
		.start = 0,
		.end   = 0,
		.flags = IORESOURCE_MEM,
	},

};

static struct platform_device device_mali= {
	.name		= "mali400_ump",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(resource_mali),
	.resource	= resource_mali,
};
#endif

#if defined(CONFIG_LCDC0_RK3066B) || defined(CONFIG_LCDC0_RK3188)
static struct resource resource_lcdc0[] = {
	[0] = {
		.name  = "lcdc0 reg",
		.start = RK30_LCDC0_PHYS,
		.end   = RK30_LCDC0_PHYS + RK30_LCDC0_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},

	[1] = {
		.name  = "lcdc0 irq",
		.start = IRQ_LCDC0,
		.end   = IRQ_LCDC0,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device device_lcdc0 = {
	.name		  = "rk30-lcdc",
	.id		  = 0,
	.num_resources	  = ARRAY_SIZE(resource_lcdc0),
	.resource	  = resource_lcdc0,
	.dev 		= {
		.platform_data = &lcdc0_screen_info,
	},
};
#endif
#if defined(CONFIG_LCDC1_RK3066B) || defined(CONFIG_LCDC1_RK3188)
static struct resource resource_lcdc1[] = {
	[0] = {
		.name  = "lcdc1 reg",
		.start = RK30_LCDC1_PHYS,
		.end   = RK30_LCDC1_PHYS + RK30_LCDC1_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.name  = "lcdc1 irq",
		.start = IRQ_LCDC1,
		.end   = IRQ_LCDC1,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device device_lcdc1 = {
	.name		  = "rk30-lcdc",
	.id		  = 1,
	.num_resources	  = ARRAY_SIZE(resource_lcdc1),
	.resource	  = resource_lcdc1,
	.dev 		= {
		.platform_data = &lcdc1_screen_info,
	},
};
#endif

#ifdef CONFIG_ANDROID_TIMED_GPIO
static struct timed_gpio timed_gpios[] = {
	{
		.name = "vibrator",
		.gpio = INVALID_GPIO,
		.max_timeout = 1000,
		.active_low = 0,
		.adjust_time =20,      //adjust for diff product
	},
};

static struct timed_gpio_platform_data rk29_vibrator_info = {
	.num_gpios = 1,
	.gpios = timed_gpios,
};

static struct platform_device rk29_device_vibrator = {
	.name = "timed-gpio",
	.id = -1,
	.dev = {
		.platform_data = &rk29_vibrator_info,
	},

};
#endif

#ifdef CONFIG_LEDS_GPIO_PLATFORM
static struct gpio_led rk29_leds[] = {
	{
		.name = "button-backlight",
		.gpio = INVALID_GPIO,
		.default_trigger = "timer",
		.active_low = 0,
		.retain_state_suspended = 0,
		.default_state = LEDS_GPIO_DEFSTATE_OFF,
	},
};

static struct gpio_led_platform_data rk29_leds_pdata = {
	.leds = rk29_leds,
	.num_leds = ARRAY_SIZE(rk29_leds),
};

static struct platform_device rk29_device_gpio_leds = {
	.name	= "leds-gpio",
	.id	= -1,
	.dev	= {
		.platform_data  = &rk29_leds_pdata,
	},
};
#endif

#ifdef CONFIG_ION
#define ION_RESERVE_SIZE        (80 * SZ_1M)
#define ION_RESERVE_SIZE_120M   (120 * SZ_1M)
#define ION_RESERVE_SIZE_220M   (220 * SZ_1M)

static struct ion_platform_data rk30_ion_pdata = {
	.nr = 1,
	.heaps = {
		{
			.type = ION_HEAP_TYPE_CARVEOUT,
			.id = ION_NOR_HEAP_ID,
			.name = "norheap",
			//			.size = ION_RESERVE_SIZE,
		}
	},
};

static struct platform_device device_ion = {
	.name = "ion-rockchip",
	.id = 0,
	.dev = {
		.platform_data = &rk30_ion_pdata,
	},
};
#endif

/**************************************************************************************************
 * SDMMC devices,  include the module of SD,MMC,and sdio.noted by xbw at 2012-03-05
 **************************************************************************************************/
#ifdef CONFIG_SDMMC_RK29
#include "board-rk3188-q72-sdmmc-config.c"
#include "../plat-rk/rk-sdmmc-ops.c"
#include "../plat-rk/rk-sdmmc-wifi.c"
#endif //endif ---#ifdef CONFIG_SDMMC_RK29

#ifdef CONFIG_SDMMC0_RK29
static int rk29_sdmmc0_cfg_gpio(void)
{
#ifdef CONFIG_SDMMC_RK29_OLD
	iomux_set(MMC0_CMD);
	iomux_set(MMC0_CLKOUT);
	iomux_set(MMC0_D0);
	iomux_set(MMC0_D1);
	iomux_set(MMC0_D2);
	iomux_set(MMC0_D3);

	iomux_set_gpio_mode(iomux_mode_to_gpio(MMC0_DETN));

	gpio_request(RK30_PIN3_PA7, "sdmmc-power");
	gpio_direction_output(RK30_PIN3_PA7, GPIO_LOW);

#else
	rk29_sdmmc_set_iomux(0, 0xFFFF);

#if defined(CONFIG_SDMMC0_RK29_SDCARD_DET_FROM_GPIO)
#if SDMMC_USE_NEW_IOMUX_API
	iomux_set_gpio_mode(iomux_gpio_to_mode(RK29SDK_SD_CARD_DETECT_N));
#else
	rk30_mux_api_set(RK29SDK_SD_CARD_DETECT_PIN_NAME, RK29SDK_SD_CARD_DETECT_IOMUX_FGPIO);
#endif
#else
#if SDMMC_USE_NEW_IOMUX_API       
	iomux_set(MMC0_DETN);
#else
	rk30_mux_api_set(RK29SDK_SD_CARD_DETECT_PIN_NAME, RK29SDK_SD_CARD_DETECT_IOMUX_FMUX);
#endif
#endif	

#if defined(CONFIG_SDMMC0_RK29_WRITE_PROTECT)
	gpio_request(SDMMC0_WRITE_PROTECT_PIN, "sdmmc-wp");
	gpio_direction_input(SDMMC0_WRITE_PROTECT_PIN);
#endif

#endif

	return 0;
}

#define CONFIG_SDMMC0_USE_DMA
struct rk29_sdmmc_platform_data default_sdmmc0_data = {
	.host_ocr_avail =
		(MMC_VDD_25_26 | MMC_VDD_26_27 | MMC_VDD_27_28 | MMC_VDD_28_29 |
		 MMC_VDD_29_30 | MMC_VDD_30_31 | MMC_VDD_31_32 | MMC_VDD_32_33 |
		 MMC_VDD_33_34 | MMC_VDD_34_35 | MMC_VDD_35_36),
	.host_caps =
		(MMC_CAP_4_BIT_DATA | MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED),
	.io_init = rk29_sdmmc0_cfg_gpio,

#if !defined(CONFIG_SDMMC_RK29_OLD)
	.set_iomux = rk29_sdmmc_set_iomux,
#endif
#ifdef USE_SDMMC_DATA4_DATA7	
	.emmc_is_selected = NULL,
#endif
	.dma_name = "sd_mmc",
#ifdef CONFIG_SDMMC0_USE_DMA
	.use_dma = 1,
#else
	.use_dma = 0,
#endif

#if defined(CONFIG_WIFI_COMBO_MODULE_CONTROL_FUNC) && defined(CONFIG_USE_SDMMC0_FOR_WIFI_DEVELOP_BOARD)
	.status = rk29sdk_wifi_mmc0_status,
	.register_status_notify = rk29sdk_wifi_mmc0_status_register,
#endif

#if defined(RK29SDK_SD_CARD_PWR_EN) || (INVALID_GPIO != RK29SDK_SD_CARD_PWR_EN)
	.power_en = RK29SDK_SD_CARD_PWR_EN,
	.power_en_level = RK29SDK_SD_CARD_PWR_EN_LEVEL,
#else
	.power_en = INVALID_GPIO,
	.power_en_level = GPIO_LOW,
#endif    
	.enable_sd_wakeup = 0,

#if defined(CONFIG_SDMMC0_RK29_WRITE_PROTECT)
	.write_prt = SDMMC0_WRITE_PROTECT_PIN,
	.write_prt_enalbe_level = SDMMC0_WRITE_PROTECT_ENABLE_VALUE;
#else
	.write_prt = INVALID_GPIO,
#endif

		.det_pin_info = {    
#if defined(RK29SDK_SD_CARD_DETECT_N) || (INVALID_GPIO != RK29SDK_SD_CARD_DETECT_N)  
			.io             = RK29SDK_SD_CARD_DETECT_N, //INVALID_GPIO,
			.enable         = RK29SDK_SD_CARD_INSERT_LEVEL,
#ifdef RK29SDK_SD_CARD_DETECT_PIN_NAME
			.iomux          = {
				.name       = RK29SDK_SD_CARD_DETECT_PIN_NAME,
#ifdef RK29SDK_SD_CARD_DETECT_IOMUX_FGPIO
				.fgpio      = RK29SDK_SD_CARD_DETECT_IOMUX_FGPIO,
#endif
#ifdef RK29SDK_SD_CARD_DETECT_IOMUX_FMUX
				.fmux       = RK29SDK_SD_CARD_DETECT_IOMUX_FMUX,
#endif
			},
#endif
#else
			.io             = INVALID_GPIO,
			.enable         = GPIO_LOW,
#endif    
		}, 

};
#endif // CONFIG_SDMMC0_RK29

#ifdef CONFIG_SDMMC1_RK29
#define CONFIG_SDMMC1_USE_DMA
static int rk29_sdmmc1_cfg_gpio(void)
{
#if defined(CONFIG_SDMMC_RK29_OLD)
	iomux_set(MMC1_CMD);
	iomux_set(MMC1_CLKOUT);
	iomux_set(MMC1_D0);
	iomux_set(MMC1_D1);
	iomux_set(MMC1_D2);
	iomux_set(MMC1_D3);
#else

#if defined(CONFIG_SDMMC1_RK29_WRITE_PROTECT)
	gpio_request(SDMMC1_WRITE_PROTECT_PIN, "sdio-wp");
	gpio_direction_input(SDMMC1_WRITE_PROTECT_PIN);
#endif

#endif

	return 0;
}

struct rk29_sdmmc_platform_data default_sdmmc1_data = {
	.host_ocr_avail =
		(MMC_VDD_25_26 | MMC_VDD_26_27 | MMC_VDD_27_28 | MMC_VDD_28_29 |
		 MMC_VDD_29_30 | MMC_VDD_30_31 | MMC_VDD_31_32 | MMC_VDD_32_33 |
		 MMC_VDD_33_34),

#if !defined(CONFIG_USE_SDMMC1_FOR_WIFI_DEVELOP_BOARD)
	.host_caps = (MMC_CAP_4_BIT_DATA | MMC_CAP_SDIO_IRQ |
			MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED),
#else
	.host_caps =
		(MMC_CAP_4_BIT_DATA | MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED),
#endif

	.io_init = rk29_sdmmc1_cfg_gpio,

#if !defined(CONFIG_SDMMC_RK29_OLD)
	.set_iomux = rk29_sdmmc_set_iomux,
#endif
#ifdef USE_SDMMC_DATA4_DATA7	
	.emmc_is_selected = NULL,
#endif
	.dma_name = "sdio",
#ifdef CONFIG_SDMMC1_USE_DMA
	.use_dma = 1,
#else
	.use_dma = 0,
#endif

#if defined(CONFIG_WIFI_CONTROL_FUNC) || defined(CONFIG_WIFI_COMBO_MODULE_CONTROL_FUNC)
	.status = rk29sdk_wifi_status,
	.register_status_notify = rk29sdk_wifi_status_register,
#endif

#if defined(CONFIG_SDMMC1_RK29_WRITE_PROTECT)
	.write_prt = SDMMC1_WRITE_PROTECT_PIN,
	.write_prt_enalbe_level = SDMMC1_WRITE_PROTECT_ENABLE_VALUE;
#else
	.write_prt = INVALID_GPIO,
#endif

#if defined(CONFIG_RK29_SDIO_IRQ_FROM_GPIO)
		.sdio_INT_gpio = RK29SDK_WIFI_SDIO_CARD_INT,
#endif

		.det_pin_info = {    
#if defined(CONFIG_USE_SDMMC1_FOR_WIFI_DEVELOP_BOARD)
#if defined(RK29SDK_SD_CARD_DETECT_N) || (INVALID_GPIO != RK29SDK_SD_CARD_DETECT_N)  
			.io             = RK29SDK_SD_CARD_DETECT_N,
#else
			.io             = INVALID_GPIO,
#endif   

			.enable         = RK29SDK_SD_CARD_INSERT_LEVEL,
#ifdef RK29SDK_SD_CARD_DETECT_PIN_NAME
			.iomux          = {
				.name       = RK29SDK_SD_CARD_DETECT_PIN_NAME,
#ifdef RK29SDK_SD_CARD_DETECT_IOMUX_FGPIO
				.fgpio      = RK29SDK_SD_CARD_DETECT_IOMUX_FGPIO,
#endif
#ifdef RK29SDK_SD_CARD_DETECT_IOMUX_FMUX
				.fmux       = RK29SDK_SD_CARD_DETECT_IOMUX_FMUX,
#endif
			},
#endif
#else
			.io             = INVALID_GPIO,
			.enable         = GPIO_LOW,
#endif
		},

		.enable_sd_wakeup = 0,
};
#endif //endif--#ifdef CONFIG_SDMMC1_RK29

#ifdef CONFIG_SDMMC2_RK29
static int rk29_sdmmc2_cfg_gpio(void)
{
	;
}

struct rk29_sdmmc_platform_data default_sdmmc2_data = {
	.host_ocr_avail =
		(MMC_VDD_165_195|MMC_VDD_25_26 | MMC_VDD_26_27 | MMC_VDD_27_28 | MMC_VDD_28_29 |
		 MMC_VDD_29_30 | MMC_VDD_30_31 | MMC_VDD_31_32 | MMC_VDD_32_33 | MMC_VDD_33_34),

	.host_caps = (MMC_CAP_4_BIT_DATA | MMC_CAP_8_BIT_DATA| MMC_CAP_NONREMOVABLE  |
			MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED | MMC_CAP_UHS_SDR12 |MMC_CAP_UHS_SDR25 |MMC_CAP_UHS_SDR50),

	.io_init = rk29_sdmmc2_cfg_gpio,
	.set_iomux = rk29_sdmmc_set_iomux,
	.emmc_is_selected = sdmmc_is_selected_emmc,

	//.power_en = INVALID_GPIO,
	// .power_en_level = GPIO_LOW,

	.dma_name = "emmc",
	.use_dma = 1,

};
#endif//endif--#ifdef CONFIG_SDMMC2_RK29

/**************************************************************************************************
 * the end of setting for SDMMC devices
 **************************************************************************************************/

#ifdef CONFIG_BATTERY_RK30_ADC
static struct rk30_adc_battery_platform_data rk30_adc_battery_platdata = {
	.dc_det_pin      = RK30_PIN0_PB2,
	.batt_low_pin    = RK30_PIN0_PB1, 
	.charge_set_pin  = INVALID_GPIO,
	.charge_ok_pin   = RK30_PIN0_PA6,
	.dc_det_level    = GPIO_LOW,
	.charge_ok_level = GPIO_HIGH,
};

static struct platform_device rk30_device_adc_battery = {
	.name   = "rk30-battery",
	.id     = -1,
	.dev = {
		.platform_data = &rk30_adc_battery_platdata,
	},
};
#endif

#ifdef CONFIG_RK29_VMAC
#define PHY_PWR_EN_GPIO	RK30_PIN0_PC0
#define PHY_PWR_EN_VALUE   GPIO_HIGH
#include "../mach-rk30/board-rk31-sdk-vmac.c"

#endif

#ifdef CONFIG_RFKILL_RK
// bluetooth rfkill device, its driver in net/rfkill/rfkill-rk.c
static struct rfkill_rk_platform_data rfkill_rk_platdata = {
	.type               = RFKILL_TYPE_BLUETOOTH,

	.poweron_gpio       = { // BT_REG_ON
		.io             = INVALID_GPIO,
		.enable         = GPIO_HIGH,
		.iomux          = {
			.name       = "bt_poweron",
			.fgpio      = GPIO3_C7,
		},
	},

	.reset_gpio         = { // BT_RST
		.io             = RK30_PIN3_PD1, // set io to INVALID_GPIO for disable it
		.enable         = GPIO_LOW,
		.iomux          = {
			.name       = "bt_reset",
			.fgpio      = GPIO3_D1,
		},
	}, 

	.wake_gpio          = { // BT_WAKE, use to control bt's sleep and wakeup
		.io             = RK30_PIN3_PC6, // set io to INVALID_GPIO for disable it
		.enable         = GPIO_HIGH,
		.iomux          = {
			.name       = "bt_wake",
			.fgpio      = GPIO3_C6,
		},
	},

	.wake_host_irq      = { // BT_HOST_WAKE, for bt wakeup host when it is in deep sleep
		.gpio           = {
			.io         = RK30_PIN0_PA5, // set io to INVALID_GPIO for disable it
			.enable     = GPIO_LOW,      // set GPIO_LOW for falling, set 0 for rising
			.iomux      = {
				.name   = NULL,
			},
		},
	},

	.rts_gpio           = { // UART_RTS, enable or disable BT's data coming
		.io             = RK30_PIN1_PA3, // set io to INVALID_GPIO for disable it
		.enable         = GPIO_LOW,
		.iomux          = {
			.name       = "bt_rts",
			.fgpio      = GPIO1_A3,
			.fmux       = UART0_RTSN,
		},
	},
};

static struct platform_device device_rfkill_rk = {
	.name   = "rfkill_rk",
	.id     = -1,
	.dev    = {
		.platform_data = &rfkill_rk_platdata,
	},
};
#endif

static struct platform_device *devices[] __initdata = {
#ifdef CONFIG_ION
	&device_ion,
#endif
#ifdef CONFIG_ANDROID_TIMED_GPIO
	&rk29_device_vibrator,
#endif
#ifdef CONFIG_LEDS_GPIO_PLATFORM
	&rk29_device_gpio_leds,
#endif
#ifdef CONFIG_RK_IRDA
	&irda_device,
#endif
#if defined(CONFIG_WIFI_CONTROL_FUNC)||defined(CONFIG_WIFI_COMBO_MODULE_CONTROL_FUNC)
	&rk29sdk_wifi_device,
#endif

#ifdef CONFIG_BATTERY_RK30_ADC
	&rk30_device_adc_battery,
#endif
#ifdef CONFIG_RFKILL_RK
	&device_rfkill_rk,
#endif
#if defined(CONFIG_ARCH_RK3188)
	&device_mali,
#endif
};


#if defined(CONFIG_MFD_RK616)
#define RK616_RST_PIN 			RK30_PIN3_PB2
#define RK616_PWREN_PIN			INVALID_GPIO
#define RK616_SCL_RATE			(100*1000)   //i2c scl rate
static int rk616_power_on_init(void)
{
	int ret;

	if(RK616_PWREN_PIN != INVALID_GPIO)
	{
		ret = gpio_request(RK616_PWREN_PIN, "rk616 pwren");
		if (ret == 0)
			gpio_direction_output(RK616_PWREN_PIN,GPIO_HIGH);
	}

	if(RK616_RST_PIN != INVALID_GPIO)
	{
		ret = gpio_request(RK616_RST_PIN, "rk616 reset");
		if (ret == 0)
		{
			gpio_direction_output(RK616_RST_PIN, GPIO_HIGH);
			msleep(2);
			gpio_direction_output(RK616_RST_PIN, GPIO_LOW);
			msleep(10);
			gpio_set_value(RK616_RST_PIN, GPIO_HIGH);
		}
	}

	return 0;
}


static int rk616_power_deinit(void)
{
	if(RK616_PWREN_PIN != INVALID_GPIO)
	{
		gpio_set_value(RK616_PWREN_PIN,GPIO_LOW);
		gpio_free(RK616_PWREN_PIN);
	}

	if(RK616_RST_PIN != INVALID_GPIO)
	{
		gpio_set_value(RK616_RST_PIN,GPIO_LOW);
		gpio_free(RK616_RST_PIN);
	}

	return 0;
}

static struct rk616_platform_data rk616_pdata = {
	.power_init	= rk616_power_on_init,
	.power_deinit	= rk616_power_deinit,
	.scl_rate	= RK616_SCL_RATE,
	.lcd0_func	= INPUT,        //port lcd0 as input
	.lcd1_func	= INPUT,        //port lcd1 as input
	.lvds_ch_nr	= 1,            //the number of used lvds channel
	.hdmi_irq	= INVALID_GPIO,
	.spk_ctl_gpio	= RK30_PIN2_PD7,
};
#endif

static int rk_platform_add_display_devices(void)
{
	struct platform_device *fb = NULL;  //fb
	struct platform_device *lcdc0 = NULL; //lcdc0
	struct platform_device *lcdc1 = NULL; //lcdc1
	struct platform_device *bl = NULL; //backlight
#ifdef CONFIG_FB_ROCKCHIP
	fb = &device_fb;
#endif

#if defined(CONFIG_LCDC0_RK3066B) || defined(CONFIG_LCDC0_RK3188)
	lcdc0 = &device_lcdc0,
#endif

#if defined(CONFIG_LCDC1_RK3066B) || defined(CONFIG_LCDC1_RK3188)
	      lcdc1 = &device_lcdc1,
#endif

#ifdef CONFIG_BACKLIGHT_RK29_BL
	      bl = &rk29_device_backlight,
#endif
	      __rk_platform_add_display_devices(fb,lcdc0,lcdc1,bl);

	return 0;
}

// i2c
#ifdef CONFIG_I2C0_RK30
static struct i2c_board_info __initdata i2c0_info[] = {
#if defined (CONFIG_GS_MMA8452)
	{
		.type	        = "gs_mma8452",
		.addr	        = 0x1d,
		.flags	        = 0,
		.irq	        = MMA8452_INT_PIN,
		.platform_data = &mma8452_info,
	},
#endif
#if defined (CONFIG_SND_SOC_RK1000)
	{
		.type          = "rk1000_i2c_codec",
		.addr          = 0x60,
		.flags         = 0,
	},
	{
		.type          = "rk1000_control",
		.addr          = 0x40,
		.flags         = 0,
	},
#endif

};
#endif

int __sramdata g_pmic_type =  0;
#ifdef CONFIG_I2C1_RK30

static struct i2c_board_info __initdata i2c1_info[] = {

#if defined (CONFIG_RTC_HYM8563)
	{
		.type                   = "rtc_hym8563",
		.addr           = 0x51,
		.flags                  = 0,
		.irq            = RK30_PIN1_PA4,
	},
#endif

};
#endif

void __sramfunc board_pmu_suspend(void)
{      
}

void __sramfunc board_pmu_resume(void)
{      
}

int __sramdata gpio3d6_iomux,gpio3d6_do,gpio3d6_dir,gpio3d6_en;

#define grf_readl(offset)	readl_relaxed(RK30_GRF_BASE + offset)
#define grf_writel(v, offset)	do { writel_relaxed(v, RK30_GRF_BASE + offset); dsb(); } while (0)

void __sramfunc rk30_pwm_logic_suspend_voltage(void)
{
}

void __sramfunc rk30_pwm_logic_resume_voltage(void)
{
}

void  rk30_pwm_suspend_voltage_set(void)
{
}

void  rk30_pwm_resume_voltage_set(void)
{
}

#ifdef CONFIG_I2C2_RK30
static struct i2c_board_info __initdata i2c2_info[] = {
#if defined (CONFIG_TOUCHSCREEN_GSL3670)
	{
		.type		= "gsl3670",
		.addr		= 0x40,
		.flags		= 0,
		.irq		= TOUCH_INT_PIN,
		.platform_data	= &gsl3670_info,
	},
#endif

};

#endif

#ifdef CONFIG_I2C3_RK30
static struct i2c_board_info __initdata i2c3_info[] = {
};
#endif

#ifdef CONFIG_I2C4_RK30
static struct i2c_board_info __initdata i2c4_info[] = {
#if defined (CONFIG_MFD_RK616)
	{
		.type		= "rk616",
		.addr		= 0x50,
		.flags		= 0,
		.platform_data	= &rk616_pdata,
	},
#endif

};
#endif

#ifdef CONFIG_I2C_GPIO_RK30
#define I2C_SDA_PIN     INVALID_GPIO// RK30_PIN2_PD6   //set sda_pin here
#define I2C_SCL_PIN     INVALID_GPIO//RK30_PIN2_PD7   //set scl_pin here
static int rk30_i2c_io_init(void)
{
	//set iomux (gpio) here
	//rk30_mux_api_set(GPIO2D7_I2C1SCL_NAME, GPIO2D_GPIO2D7);
	//rk30_mux_api_set(GPIO2D6_I2C1SDA_NAME, GPIO2D_GPIO2D6);

	return 0;
}
struct i2c_gpio_platform_data default_i2c_gpio_data = {
	.sda_pin = I2C_SDA_PIN,
	.scl_pin = I2C_SCL_PIN,
	.udelay = 5, // clk = 500/udelay = 100Khz
	.timeout = 100,//msecs_to_jiffies(100),
	.bus_num    = 5,
	.io_init = rk30_i2c_io_init,
};
static struct i2c_board_info __initdata i2c_gpio_info[] = {
};
#endif

static void __init rk30_i2c_register_board_info(void)
{
#ifdef CONFIG_I2C0_RK30
	i2c_register_board_info(0, i2c0_info, ARRAY_SIZE(i2c0_info));
#endif
#ifdef CONFIG_I2C1_RK30
	i2c_register_board_info(1, i2c1_info, ARRAY_SIZE(i2c1_info));
#endif
#ifdef CONFIG_I2C2_RK30
	i2c_register_board_info(2, i2c2_info, ARRAY_SIZE(i2c2_info));
#endif
#ifdef CONFIG_I2C3_RK30
	i2c_register_board_info(3, i2c3_info, ARRAY_SIZE(i2c3_info));
#endif
#ifdef CONFIG_I2C4_RK30
	i2c_register_board_info(4, i2c4_info, ARRAY_SIZE(i2c4_info));
#endif
#ifdef CONFIG_I2C_GPIO_RK30
	i2c_register_board_info(5, i2c_gpio_info, ARRAY_SIZE(i2c_gpio_info));
#endif
}
//end of i2c

#include <plat/key.h>

static struct rk29_keys_button key_button[] = {
#if 0   /* KEY_POWER report by PMU driver */
	{
		.desc           = "play",
		.code           = KEY_POWER,
		.gpio           = RK30_PIN0_PA4,
		.active_low     = PRESS_LEV_LOW,
		.wakeup         = 1,
	},
#endif
	{
		.desc           = "vol+",
		.code           = KEY_VOLUMEUP,
		.adc_value      = 1,
		.gpio           = INVALID_GPIO,
		.active_low     = PRESS_LEV_LOW,
	},
	{
		.desc           = "vol-",
		.code           = KEY_VOLUMEDOWN,
		.adc_value      = 512,
		.gpio           = INVALID_GPIO,
		.active_low     = PRESS_LEV_LOW,
	},

};

struct rk29_keys_platform_data rk29_keys_pdata = {
	.buttons	= key_button,
	.nbuttons	= ARRAY_SIZE(key_button),
	.chn		= 1,  //chn: 0-7, if do not use ADC,set 'chn' -1
};



static void __init machine_rk30_board_init(void)
{
	//avs_init();

	pm_power_off = NULL;

	rk30_i2c_register_board_info();
	spi_register_board_info(board_spi_devices, ARRAY_SIZE(board_spi_devices));
	platform_add_devices(devices, ARRAY_SIZE(devices));
	rk_platform_add_display_devices();
	board_usb_detect_init(RK30_PIN0_PA7);

#if defined(CONFIG_WIFI_CONTROL_FUNC)
	rk29sdk_wifi_bt_gpio_control_init();
#elif defined(CONFIG_WIFI_COMBO_MODULE_CONTROL_FUNC)
	rk29sdk_wifi_combo_module_gpio_init();
#endif

}
#define HD_SCREEN_SIZE 1920UL*1200UL*4*3
static void __init rk30_reserve(void)
{
	int size, ion_reserve_size;
#if defined(CONFIG_ARCH_RK3188)
	/*if lcd resolution great than or equal to 1920*1200,reserve the ump memory */
	if(!(get_fb_size() < ALIGN(HD_SCREEN_SIZE,SZ_1M)))
	{
		int ump_mem_phy_size=512UL*1024UL*1024UL; 
		resource_mali[0].start = board_mem_reserve_add("ump buf", ump_mem_phy_size); 
		resource_mali[0].end = resource_mali[0].start + ump_mem_phy_size -1;
	}
#endif
#ifdef CONFIG_ION
	size = ddr_get_cap() >> 20;
	if(size >= 1024) { // DDR >= 1G, set ion to 120M
		rk30_ion_pdata.heaps[0].size = ION_RESERVE_SIZE_220M;
		ion_reserve_size = ION_RESERVE_SIZE_220M;
	}
	else {
		rk30_ion_pdata.heaps[0].size = ION_RESERVE_SIZE;
		ion_reserve_size = ION_RESERVE_SIZE;
	}
	rk30_ion_pdata.heaps[0].size += get_fb_size();
	ion_reserve_size += get_fb_size();
	printk("ddr size = %d M, set ion_reserve_size size to %d\n", size, ion_reserve_size);
	//rk30_ion_pdata.heaps[0].base = board_mem_reserve_add("ion", ION_RESERVE_SIZE);
	rk30_ion_pdata.heaps[0].base = board_mem_reserve_add("ion", ion_reserve_size);
#endif
	/*
	 * get fbbuf from ion
	 */
	//#ifdef CONFIG_FB_ROCKCHIP
#if 0
	resource_fb[0].start = board_mem_reserve_add("fb0 buf", get_fb_size());
	resource_fb[0].end = resource_fb[0].start + get_fb_size()- 1;
#if 0
	resource_fb[1].start = board_mem_reserve_add("ipp buf", RK30_FB0_MEM_SIZE);
	resource_fb[1].end = resource_fb[1].start + RK30_FB0_MEM_SIZE - 1;
#endif

#if defined(CONFIG_FB_ROTATE) || !defined(CONFIG_THREE_FB_BUFFER)
	resource_fb[2].start = board_mem_reserve_add("fb2 buf",get_fb_size());
	resource_fb[2].end = resource_fb[2].start + get_fb_size() - 1;
#endif
#endif

#ifdef CONFIG_VIDEO_RK29
	rk30_camera_request_reserve_mem();
#endif

	board_mem_reserved();
}
/******************************** arm dvfs frequency volt table **********************************/
/**
 * dvfs_cpu_logic_table: table for arm and logic dvfs 
 * @frequency	: arm frequency
 * @cpu_volt	: arm voltage depend on frequency
 */

#if defined(CONFIG_ARCH_RK3188)
//sdk
static struct cpufreq_frequency_table dvfs_arm_table_volt_level0[] = {
	{.frequency = 312 * 1000,       .index = 850 * 1000},
	{.frequency = 504 * 1000,       .index = 900 * 1000},
	{.frequency = 816 * 1000,       .index = 950 * 1000},
	{.frequency = 1008 * 1000,      .index = 1025 * 1000},
	{.frequency = 1200 * 1000,      .index = 1100 * 1000},
	{.frequency = 1416 * 1000,      .index = 1200 * 1000},
	{.frequency = 1608 * 1000,      .index = 1300 * 1000},
	{.frequency = CPUFREQ_TABLE_END},
};
//default
static struct cpufreq_frequency_table dvfs_arm_table_volt_level1[] = {
	{.frequency = 312 * 1000,       .index = 875 * 1000},
	{.frequency = 504 * 1000,       .index = 925 * 1000},
	{.frequency = 816 * 1000,       .index = 975 * 1000},
	{.frequency = 1008 * 1000,      .index = 1075 * 1000},
	{.frequency = 1200 * 1000,      .index = 1150 * 1000},
	{.frequency = 1416 * 1000,      .index = 1250 * 1000},
	{.frequency = 1608 * 1000,      .index = 1350 * 1000},
	{.frequency = CPUFREQ_TABLE_END},
};
// cube 10'
static struct cpufreq_frequency_table dvfs_arm_table_volt_level2[] = {
	{.frequency = 312 * 1000,       .index = 900 * 1000},
	{.frequency = 504 * 1000,       .index = 925 * 1000},
	{.frequency = 816 * 1000,       .index = 1000 * 1000},
	{.frequency = 1008 * 1000,      .index = 1075 * 1000},
	{.frequency = 1200 * 1000,      .index = 1200 * 1000},
	{.frequency = 1416 * 1000,      .index = 1250 * 1000},
	{.frequency = 1608 * 1000,      .index = 1350 * 1000},
	{.frequency = CPUFREQ_TABLE_END},
};

/******************************** gpu dvfs frequency volt table **********************************/
//sdk
static struct cpufreq_frequency_table dvfs_gpu_table_volt_level0[] = {	
	{.frequency = 133 * 1000,       .index = 975 * 1000},//the mininum rate is limited 133M for rk3188
	{.frequency = 200 * 1000,       .index = 975 * 1000},
	{.frequency = 266 * 1000,       .index = 1000 * 1000},
	{.frequency = 300 * 1000,       .index = 1050 * 1000},
	{.frequency = 400 * 1000,       .index = 1100 * 1000},
	{.frequency = 600 * 1000,       .index = 1200 * 1000},
	{.frequency = CPUFREQ_TABLE_END},
};
//cube 10'
static struct cpufreq_frequency_table dvfs_gpu_table_volt_level1[] = {	
	{.frequency = 133 * 1000,       .index = 975 * 1000},//the mininum rate is limited 133M for rk3188
	{.frequency = 200 * 1000,       .index = 1000 * 1000},
	{.frequency = 266 * 1000,       .index = 1025 * 1000},
	{.frequency = 300 * 1000,       .index = 1050 * 1000},
	{.frequency = 400 * 1000,       .index = 1100 * 1000},
	{.frequency = 600 * 1000,       .index = 1250 * 1000},
	{.frequency = CPUFREQ_TABLE_END},
};

/******************************** ddr dvfs frequency volt table **********************************/
static struct cpufreq_frequency_table dvfs_ddr_table_volt_level0[] = {
	{.frequency = 200 * 1000 + DDR_FREQ_SUSPEND,    .index = 950 * 1000},
	{.frequency = 300 * 1000 + DDR_FREQ_VIDEO,      .index = 1000 * 1000},
	{.frequency = 396 * 1000 + DDR_FREQ_NORMAL,     .index = 1100 * 1000},
	//{.frequency = 528 * 1000 + DDR_FREQ_NORMAL,     .index = 1200 * 1000},
	{.frequency = CPUFREQ_TABLE_END},
};

//if you board is good for volt quality,select dvfs_arm_table_volt_level0
#define dvfs_arm_table dvfs_arm_table_volt_level2
#define dvfs_gpu_table dvfs_gpu_table_volt_level1
#define dvfs_ddr_table dvfs_ddr_table_volt_level0

#else
//for RK3168 && RK3066B
static struct cpufreq_frequency_table dvfs_arm_table[] = {
	{.frequency = 312 * 1000,       .index = 950 * 1000},
	{.frequency = 504 * 1000,       .index = 1000 * 1000},
	{.frequency = 816 * 1000,       .index = 1050 * 1000},
	{.frequency = 1008 * 1000,      .index = 1125 * 1000},
	{.frequency = 1200 * 1000,      .index = 1200 * 1000},
	//{.frequency = 1416 * 1000,      .index = 1250 * 1000},
	//{.frequency = 1608 * 1000,      .index = 1300 * 1000},
	{.frequency = CPUFREQ_TABLE_END},
};

static struct cpufreq_frequency_table dvfs_gpu_table[] = {
	{.frequency = 100 * 1000,       .index = 1000 * 1000},
	{.frequency = 200 * 1000,       .index = 1000 * 1000},
	{.frequency = 266 * 1000,       .index = 1050 * 1000},
	//{.frequency = 300 * 1000,       .index = 1050 * 1000},
	{.frequency = 400 * 1000,       .index = 1125 * 1000},
	{.frequency = CPUFREQ_TABLE_END},
};

static struct cpufreq_frequency_table dvfs_ddr_table[] = {
	{.frequency = 200 * 1000 + DDR_FREQ_SUSPEND,    .index = 1000 * 1000},
	{.frequency = 300 * 1000 + DDR_FREQ_VIDEO,      .index = 1050 * 1000},
	{.frequency = 400 * 1000 + DDR_FREQ_NORMAL,     .index = 1100 * 1000},
	{.frequency = CPUFREQ_TABLE_END},
};
#endif
/******************************** arm dvfs frequency volt table end **********************************/
//#define DVFS_CPU_TABLE_SIZE	(ARRAY_SIZE(dvfs_cpu_logic_table))
//static struct cpufreq_frequency_table cpu_dvfs_table[DVFS_CPU_TABLE_SIZE];
//static struct cpufreq_frequency_table dep_cpu2core_table[DVFS_CPU_TABLE_SIZE];

void __init board_clock_init(void)
{
	rk30_clock_data_init(periph_pll_default, codec_pll_default, RK30_CLOCKS_DEFAULT_FLAGS);
	//dvfs_set_arm_logic_volt(dvfs_cpu_logic_table, cpu_dvfs_table, dep_cpu2core_table);	
	dvfs_set_freq_volt_table(clk_get(NULL, "cpu"), dvfs_arm_table);
	dvfs_set_freq_volt_table(clk_get(NULL, "gpu"), dvfs_gpu_table);
	dvfs_set_freq_volt_table(clk_get(NULL, "ddr"), dvfs_ddr_table);
}

MACHINE_START(RK30, "RK30board")
	.boot_params	= PLAT_PHYS_OFFSET + 0x800,
	.fixup		= rk30_fixup,
	.reserve	= &rk30_reserve,
	.map_io		= rk30_map_io,
	.init_irq	= rk30_init_irq,
	.timer		= &rk30_timer,
	.init_machine	= machine_rk30_board_init,
MACHINE_END
