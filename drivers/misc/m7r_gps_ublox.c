#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/input.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/circ_buf.h>
#include <linux/interrupt.h>
#include <linux/miscdevice.h>
#include <mach/iomux.h>
#include <mach/gpio.h>
#include <asm/gpio.h>
#include <linux/delay.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/wakelock.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/earlysuspend.h>

#include <mach/gpio.h>

MODULE_LICENSE("GPL");

#define DEBUG
#ifdef DEBUG
#define GPSDBG(x...) printk(x)
#else
#define GPSDBG(fmt,argss...)
#endif

#define GPS_PWR		RK30_PIN0_PB7
#define GPS_ANTON	RK30_PIN0_PB4
#define GPS_INT		RK30_PIN3_PD3
#define GPS_PULSE	RK30_PIN3_PD4

struct class *gps_class = NULL;
static int gps_status;

static int gps_poweron_off(int on_off)
{
	if(on_off) /* power on */
	{
		gpio_set_value(GPS_PWR, GPIO_HIGH);
	}
	else
	{
		gpio_set_value(GPS_PWR, GPIO_LOW);
	}
	return 0;
}

static int gps_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int gps_release(struct inode *inode, struct file *file)
{
	return 0;
}

static long gps_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	return 0;
}

static struct file_operations gps_fops = {
	.owner = THIS_MODULE,
	.open = gps_open,
	.release = gps_release,
	.unlocked_ioctl = gps_ioctl
};

static struct miscdevice gps_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "ublox",
	.fops = &gps_fops
};

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37))
static ssize_t gps_status_read(struct class *cls, struct class_attribute *attr, char *_buf)
#else
static ssize_t gps_status_read(struct class *cls, char *_buf)
#endif
{

	return sprintf(_buf, "%d\n", gps_status);

}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37))
static ssize_t gps_status_write(struct class *cls, struct class_attribute *attr, const char *_buf, size_t _count)
#else
static ssize_t gps_status_write(struct class *cls, const char *_buf, size_t _count)
#endif
{
	int new_state = simple_strtoul(_buf, NULL, 16);
	if(new_state == gps_status) return _count;
	if (new_state == 1){
		printk("%s, c(%d), open gps \n", __FUNCTION__, new_state);
		gps_poweron_off(1);
	}else if(new_state == 0){
		printk("%s, c(%d), close gps \n", __FUNCTION__, new_state);
		gps_poweron_off(0);
	}else{
		printk("%s, invalid parameter \n", __FUNCTION__);
	}
	gps_status = new_state;
	return _count; 
}

static CLASS_ATTR(gps_status, 0666, gps_status_read, gps_status_write);

static void rk29_early_suspend(struct early_suspend *h)
{
}

static void rk29_early_resume(struct early_suspend *h)
{
}

static struct early_suspend gps_early_suspend = {
	.suspend = rk29_early_suspend,
	.resume = rk29_early_resume,
	.level = EARLY_SUSPEND_LEVEL_DISABLE_FB - 1,
};

static int gps_probe(struct platform_device *pdev)
{
	int ret;	

	printk("%s\n", __FUNCTION__);

	gpio_request(GPS_PWR, "gps pwr");
	gpio_direction_output(GPS_PWR, GPIO_LOW);

	gps_class = class_create(THIS_MODULE, "gps");
	ret = class_create_file(gps_class, &class_attr_gps_status);
	if (ret)
	{
		printk("Fail to class gps.\n");
	}

	register_early_suspend(&gps_early_suspend);

	ret = misc_register(&gps_misc);
	if(ret)
		printk("%s: misc_register err\n", __FUNCTION__);
	else
		printk("%s: misc register success\n", __FUNCTION__);

	return ret;
}

int gps_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}

int gps_resume(struct platform_device *pdev)
{
	return 0;
}

void gps_shutdown(struct platform_device *pdev)
{
	gps_poweron_off(0);
}

static struct platform_driver gps_driver = {
	.probe		= gps_probe,
	.shutdown	= gps_shutdown,
	.suspend	= gps_suspend,
	.resume		= gps_resume,
	.driver	= {
		.name	= "gps_ublox",
		.owner	= THIS_MODULE,
	},
};

static int __init gps_init(void)
{
	return platform_driver_register(&gps_driver);
}

static void __exit gps_exit(void)
{
	platform_driver_unregister(&gps_driver);
	class_remove_file(gps_class, &class_attr_gps_status);
}

module_init(gps_init);

module_exit(gps_exit);
