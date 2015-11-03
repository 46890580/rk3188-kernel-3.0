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
#include <linux/ug95.h>
#include <linux/slab.h>
#include <linux/earlysuspend.h>

MODULE_LICENSE("GPL");

#define DEBUG
#ifdef DEBUG
#define MODEMDBG(x...) printk(x)
#else
#define MODEMDBG(fmt,argss...)
#endif

#define UG95_RESET	RK30_PIN0_PA5
#define UG95_PWRKEY	RK30_PIN0_PB1
#define UG95_PWR_ONOFF	RK30_PIN0_PA1
#define UG95_VBUS_ONOFF	RK30_PIN0_PA0
#define UG95_PWR_DWN	RK30_PIN0_PA4

struct class *modem_class = NULL; 
static int modem_status = 0;

static int modem_poweron_off(int on_off)
{
	if(on_off) /* power on */
	{
		gpio_direction_output(UG95_PWR_ONOFF, GPIO_HIGH);
		gpio_direction_output(UG95_VBUS_ONOFF, GPIO_HIGH);

		msleep(50);

		gpio_set_value(UG95_PWRKEY, GPIO_HIGH);
		msleep(800);
		gpio_set_value(UG95_PWRKEY, GPIO_LOW);

		modem_status = 1;
	}
	else
	{
		modem_status = 0;

		gpio_set_value(UG95_PWRKEY, GPIO_HIGH);
		msleep(800);
		gpio_set_value(UG95_PWRKEY, GPIO_LOW);

		msleep(100);

		gpio_direction_output(UG95_VBUS_ONOFF, GPIO_LOW);
		gpio_direction_output(UG95_PWR_ONOFF, GPIO_LOW);
	}
	return 0;
}

static int ug95_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int ug95_release(struct inode *inode, struct file *file)
{
	return 0;
}

static long ug95_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	return 0;
}

static struct file_operations ug95_fops = {
	.owner = THIS_MODULE,
	.open = ug95_open,
	.release = ug95_release,
	.unlocked_ioctl = ug95_ioctl
};

static struct miscdevice ug95_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = MODEM_NAME,
	.fops = &ug95_fops
};

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37))
static ssize_t modem_status_read(struct class *cls, struct class_attribute *attr, char *_buf)
#else
static ssize_t modem_status_read(struct class *cls, char *_buf)
#endif
{
	return sprintf(_buf, "%d\n", modem_status);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37))
static ssize_t modem_status_write(struct class *cls, struct class_attribute *attr, const char *_buf, size_t _count)
#else
static ssize_t modem_status_write(struct class *cls, const char *_buf, size_t _count)
#endif
{
	int new_state = simple_strtoul(_buf, NULL, 16);
	if(new_state == modem_status) return _count;
	if (new_state == 1){
		printk("%s, c(%d), open modem \n", __FUNCTION__, new_state);
		modem_poweron_off(1);
	}else if(new_state == 0){
		printk("%s, c(%d), close modem \n", __FUNCTION__, new_state);
		modem_poweron_off(0);
	}else{
		printk("%s, invalid parameter \n", __FUNCTION__);
	}
	modem_status = new_state;
	return _count; 
}

static CLASS_ATTR(modem_status, 0664, modem_status_read, modem_status_write);

static void rk29_early_suspend(struct early_suspend *h)
{
}

static void rk29_early_resume(struct early_suspend *h)
{
}

static struct early_suspend ug95_early_suspend = {
	.suspend = rk29_early_suspend,
	.resume = rk29_early_resume,
	.level = EARLY_SUSPEND_LEVEL_DISABLE_FB - 1,
};

static int ug95_probe(struct platform_device *pdev)
{
	int ret ;

	gpio_request(UG95_RESET, "ug95 reset");
	gpio_request(UG95_PWRKEY, "ug95 pwrkey");
	gpio_request(UG95_PWR_ONOFF, "ug95 pwr onoff");
	gpio_request(UG95_VBUS_ONOFF, "ug95 vbus onoff");
	gpio_request(UG95_PWR_DWN, "ug95 pwrdwn");

	gpio_direction_output(UG95_PWR_ONOFF, GPIO_LOW);
	gpio_direction_output(UG95_VBUS_ONOFF, GPIO_LOW);
	gpio_direction_output(UG95_PWR_DWN, GPIO_LOW);
	gpio_direction_output(UG95_PWRKEY, GPIO_LOW);
	gpio_direction_output(UG95_RESET, GPIO_LOW);

	modem_class = class_create(THIS_MODULE, "modem");
	ret =  class_create_file(modem_class, &class_attr_modem_status);
	if (ret)
	{
		printk("Fail to class rk291x_modem.\n");
	}

	modem_poweron_off(1);

	register_early_suspend(&ug95_early_suspend);

	ret = misc_register(&ug95_misc);
	if(ret)
		printk("%s: misc_register err\n", __FUNCTION__);
	else
		printk("%s: misc register success\n", __FUNCTION__);	


	return ret;
}

static int ug95_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}

static int ug95_resume(struct platform_device *pdev)
{
	return 0;
}

static void ug95_shutdown(struct platform_device *pdev)
{
	modem_poweron_off(0);

	gpio_free(UG95_RESET);
	gpio_free(UG95_PWRKEY);
	gpio_free(UG95_PWR_ONOFF);
	gpio_free(UG95_VBUS_ONOFF);
	gpio_free(UG95_PWR_DWN);
}

static struct platform_driver ug95_driver = {
	.probe		= ug95_probe,
	.shutdown	= ug95_shutdown,
	.suspend	= ug95_suspend,
	.resume		= ug95_resume,
	.driver	= {
		.name	= "UG95",
		.owner	= THIS_MODULE,
	},
};

static int __init ug95_init(void)
{
	return platform_driver_register(&ug95_driver);
}

static void __exit ug95_exit(void)
{
	platform_driver_unregister(&ug95_driver);
	class_remove_file(modem_class, &class_attr_modem_status);
}

module_init(ug95_init);
module_exit(ug95_exit);
