#if defined(__KERNEL__)
#include <linux/kernel.h>

#define DEBUG 1
#if defined(DEBUG)
extern bool gMhlDbgPrintEnable;
#endif
#endif
#define ITE_DEVICE_ID			0x6811
#define TX_HW_RESET_PERIOD		10
#define RCP_ENABLE 	1
