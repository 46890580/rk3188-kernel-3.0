#if !defined(ITE6811_HAL_PRIV_H)
#define ITE6811_HAL_PRIV_H
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/semaphore.h>
#include <linux/kthread.h>
#include <linux/spinlock.h>
#ifdef __cplusplus 
extern "C" { 
#endif  

typedef struct  {
	struct	i2c_driver	driver;
	struct	i2c_client	*pI2cClient;
	fwIrqHandler_t		irqHandler;
    unsigned int        MonRequestIRQ;
	spinlock_t          MonRequestIRQ_Lock;
    unsigned int        MonControlReleased;
    fnCheckDevice       CheckDevice;
#ifdef RGB_BOARD
    unsigned int        ExtDeviceIRQ;
	fwIrqHandler_t		ExtDeviceirqHandler;
#endif
} mhlDeviceContext_t, *pMhlDeviceContext;
extern bool gHalInitedFlag;
extern struct i2c_device_id gMhlI2cIdTable[2];
extern mhlDeviceContext_t gMhlDevice;
halReturn_t HalInitCheck(void);
#ifdef __cplusplus
}
#endif  
#endif 
