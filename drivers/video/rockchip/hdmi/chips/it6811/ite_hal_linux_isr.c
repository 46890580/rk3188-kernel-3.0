#define ITE6811_HAL_LINUX_ISR_C
#include "ite_hal.h"
#include "ite_hal_priv.h"
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/completion.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include "ite_drv_mhl_tx.h"



