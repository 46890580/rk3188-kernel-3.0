#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <asm/atomic.h>
#include <mach/io.h>
#include <mach/gpio.h>
#include <mach/iomux.h>

#include "it6811_mhl.h"
#include "it6811_mhl_hw.h"
#include "ite_drv_mhl_tx.h"
#include "ite_mhl_tx_api.h"

extern struct it6811_mhl_pdata *it6811_mhl;

static bool it6811_pwr_stat = FALSE;

static BYTE i2crd(__u16 i2caddr, BYTE regaddr)
{
	struct i2c_msg msgs[2];
	SYS_STATUS ret = -1;
	BYTE buf[1];

	buf[0] = regaddr;

	/* Write device addr fisrt */
	msgs[0].addr	= i2caddr;
	msgs[0].flags	= !I2C_M_RD;
	msgs[0].len		= 1;
	msgs[0].buf		= &buf[0];
	msgs[0].scl_rate= 90*1000;
	/* Then, begin to read data */
	msgs[1].addr	= i2caddr;
	msgs[1].flags	= I2C_M_RD;
	msgs[1].len		= 1;
	msgs[1].buf		= &buf[0];
	msgs[1].scl_rate= 90*1000;

	ret = i2c_transfer(it6811_mhl->client->adapter, msgs, 2);
	if(ret != 2)
		printk("I2C transfer Error! ret = %d\n", ret);

	//ErrorF("Reg%02xH: 0x%02x\n", RegAddr, buf[0]);
	return buf[0];
}

void i2cwr(__u16 i2caddr, BYTE regaddr, BYTE data)
{
	struct i2c_msg msg;
	SYS_STATUS ret = -1;
	BYTE buf[2];
	BYTE drd;

	buf[0] = regaddr;
	buf[1] = data;

	msg.addr	= i2caddr;
	msg.flags	= !I2C_M_RD;
	msg.len		= 2;
	msg.buf		= buf;		
	msg.scl_rate= 90*1000;

	ret = i2c_transfer(it6811_mhl->client->adapter, &msg, 1);
	if(ret != 1)
		printk("I2C transfer Error i2caddr:0x%02x, regaddr:0x%02x, data:0x%02x!\n", i2caddr, regaddr, data);

	/*drd = i2crd(i2caddr, regaddr);
	if (data != drd)
		printk("I2C wr Error i2caddr:0x%02x, regaddr:0x%02x, data-wr:0x%02x, data-rd:0x%02x!\n", i2caddr, regaddr, data, drd);*/

}

BYTE hdmitxrd(BYTE regaddr)
{
	return i2crd(it6811_mhl->client->addr, regaddr);
}

void hdmitxwr(BYTE regaddr, BYTE data)
{
	i2cwr(it6811_mhl->client->addr, regaddr, data);
}
BYTE mhltxrd(BYTE regaddr)
{
	return i2crd(MHL_ADDR, regaddr);
}

void mhltxwr(BYTE regaddr, BYTE data)
{
	i2cwr(MHL_ADDR, regaddr, data);
}

void hdmitxset( unsigned char offset, unsigned char mask, unsigned char wdata )
{
     unsigned char temp;

     temp = hdmitxrd(offset);
     temp = (temp&((~mask)&0xFF))+(mask&wdata);
     hdmitxwr(offset, temp);
     return ;
}

void mhltxset( unsigned char offset, unsigned char mask, unsigned char wdata )
{
 unsigned char temp;

     temp = mhltxrd(offset);
     temp = (temp&((~mask)&0xFF))+(mask&wdata);
     mhltxwr(offset, temp);
     return; 
}

long int get_current_time_us(void)
{
	int this_cpu = smp_processor_id();
	unsigned long long t = cpu_clock(this_cpu);
	unsigned long nanosec_rem;

	nanosec_rem = do_div(t, 1000);

	return (long int)t;
}


int it6811_detect_device(void)
{
	uint8_t VendorID0, VendorID1, DeviceID0, DeviceID1;

	hdmitxset(0x0f, 1, 0); /* switch bank 0 */
	VendorID0 = hdmitxrd(REG_TX_VENDOR_ID0);
	VendorID1 = hdmitxrd(REG_TX_VENDOR_ID1);
	DeviceID0 = hdmitxrd(REG_TX_DEVICE_ID0);
	DeviceID1 = hdmitxrd(REG_TX_DEVICE_ID1);
	printk("it6811: Reg[0-3] = 0x[%02x].[%02x].[%02x].[%02x] cbusslvadr:%02x\n",
			VendorID0, VendorID1, DeviceID0, DeviceID1, hdmitxrd(0xd8));
	if( (VendorID0 == 0x54) && (VendorID1 == 0x49))
	   //    	&&(DeviceID0 == 0x12) && (DeviceID1 == 0x16) )
		return 1;

	printk("[it6811] Device not found!\n");

	return 0;
}

int it6811_sys_detect_hpd(void)
{
	char HPD= 0;
	BYTE sysstat;


#ifdef SUPPORT_HDCP
	if((it6811_mhl->plug_status != 0) && (it6811_mhl->plug_status != 1))
		it6811_mhl->plug_status = hdmitxrd(REG_TX_SYS_STATUS);
	
        sysstat = it6811_mhl->plug_status;
#else
        sysstat = hdmitxrd(REG_TX_SYS_STATUS);
#endif

	HPD = ((sysstat & B_TX_HPDETECT) == B_TX_HPDETECT)?TRUE:FALSE;
	if(HPD)
		return HDMI_HPD_ACTIVED;
	else
		return HDMI_HPD_REMOVED;
}

extern int iteReadEdidBlock( unsigned char *buff ,int block);
int it6811_sys_read_edid(int block, unsigned char *buff)
{
	hdmi_dbg(hdmi->dev, "[%s]\n", __FUNCTION__);
	return ((iteReadEdidBlock(buff, block) == SUCCESS) ? HDMI_ERROR_SUCESS : HDMI_ERROR_FALSE);
}

int it6811_sys_config_video(struct hdmi_video_para *vpara)
{
	printk("[hdmi_drv]720p\n");
	iteHdmiTx_VideoSel(HDMI_720P60);
	return HDMI_ERROR_SUCESS;
}

int it6811_sys_config_audio(struct hdmi_audio *audio)
{
	return HDMI_ERROR_SUCESS;
}

void it6811_sys_enalbe_output(int enable)
{
	hdmi_drv_video_enable(enable);
	hdmi_drv_audio_enable(enable);
}

int it6811_sys_insert(void)
{
	hdmi_dbg(hdmi->dev, "[%s]\n", __FUNCTION__);

	if(it6811_pwr_stat == FALSE) {
		it6811_pwr_stat = TRUE;
		hdmitx_pwron();
	}

	return 0;
}

int it6811_sys_remove(void)
{
	hdmi_dbg(hdmi->dev, "[%s]\n", __FUNCTION__);

	//HDMITX_DisableVideoOutput();
	if(it6811_pwr_stat == TRUE) {
		hdmitx_pwrdn();
		it6811_pwr_stat = FALSE;
	}
	return 0;
}
