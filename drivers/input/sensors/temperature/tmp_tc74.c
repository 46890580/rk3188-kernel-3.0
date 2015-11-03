/* drivers/input/sensors/temperature/tmp_tc74.c
 *
 * Copyright (C) 2012-2015 ROCKCHIP.
 * Author: luowei <lw@rock-chips.com>
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
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <linux/gpio.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/freezer.h>
#include <mach/gpio.h>
#include <mach/board.h> 
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/sensor-dev.h>


#define ADDR_CONFIG   	0x01  
#define ADDR_TEMP 	0x00
#define CMD_NORMAL	0x00	
#define CMD_STDBY	0x80

static int g_tc74_temp = 0;
static int g_tc74_pr_status = SENSOR_OFF;



/****************operate according to sensor chip:start************/

static int sensor_active(struct i2c_client *client, int enable, int rate)
{
	int result = 0;
	if((enable)&&(g_tc74_pr_status == SENSOR_OFF))
	{
		result = sensor_write_reg(client, ADDR_CONFIG,CMD_NORMAL);
		if(result)		
			printk("%s:line=%d,error\n",__func__,__LINE__);
		else
			g_tc74_pr_status = SENSOR_ON;
	}else if((!enable)&&(g_tc74_pr_status == SENSOR_ON))
	{
		result = sensor_write_reg(client, ADDR_CONFIG,CMD_STDBY);
		if(result)		
			printk("%s:line=%d,error\n",__func__,__LINE__);
		else
			g_tc74_pr_status = SENSOR_OFF;	
	}	
	return result;
}



static int sensor_init(struct i2c_client *client)
{	
	struct sensor_private_data *sensor =
		(struct sensor_private_data *) i2c_get_clientdata(client);	
	int result = 0;

	result = sensor->ops->active(client,0,0);
	if(result)
	{
		printk("%s:line=%d,error\n",__func__,__LINE__);
		return result;
	}
	sensor->status_cur = SENSOR_OFF;

	return result;
}



static int temperature_report_value(struct input_dev *input, int data)
{
	//get temperature, high and temperature from register data

	input_report_abs(input, ABS_THROTTLE, data);
	input_sync(input);

	return 0;
}


static int sensor_report_value(struct i2c_client *client)
{
	struct sensor_private_data *sensor =
		(struct sensor_private_data *) i2c_get_clientdata(client);	
	g_tc74_temp = sensor_read_reg(client, ADDR_TEMP);
	temperature_report_value(sensor->input_dev, g_tc74_temp);
	printk("==============temperature = %d \n",g_tc74_temp);
	return  0;
}


struct sensor_operate temperature_tc74_ops = {
	.name				= "tmp_tc74",
	.type				= SENSOR_TYPE_TEMPERATURE,	//sensor type and it should be correct
	.id_i2c				= TEMPERATURE_ID_TC74,	//i2c id number
	.read_reg			= SENSOR_UNKNOW_DATA,	//read data
	.read_len			= 3,			//data length
	.id_reg				= SENSOR_UNKNOW_DATA,	//read device id from this register
	.id_data 			= SENSOR_UNKNOW_DATA,	//device id
	.precision			= 24,			//8 bits
	.ctrl_reg 			= SENSOR_UNKNOW_DATA,	//enable or disable 
	.int_status_reg 		= SENSOR_UNKNOW_DATA,	//intterupt status register
	.range				= {0,100},		//range
	.trig				= SENSOR_UNKNOW_DATA,		
	.active				= sensor_active,	
	.init				= sensor_init,
	.report				= sensor_report_value,
};

/****************operate according to sensor chip:end************/

//function name should not be changed
static struct sensor_operate *temperature_get_ops(void)
{
	return &temperature_tc74_ops;
}


static int __init temperature_tc74_init(void)
{
	struct sensor_operate *ops = temperature_get_ops();
	int result = 0;
	int type = ops->type;
	result = sensor_register_slave(type, NULL, NULL, temperature_get_ops);
	return result;
}

static void __exit temperature_tc74_exit(void)
{
	struct sensor_operate *ops = temperature_get_ops();
	int type = ops->type;
	sensor_unregister_slave(type, NULL, NULL, temperature_get_ops);
}


module_init(temperature_tc74_init);
module_exit(temperature_tc74_exit);

