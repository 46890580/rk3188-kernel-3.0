#if !defined(MHL_DRIVER_H)
#define MHL_DRIVER_H
#include "ite_hal.h"
#ifdef __cplusplus 
extern "C" { 
#endif  
#if 1//defined(MAKE_8338_DRIVER)
#define MHL_PART_NAME   "ITE_MHL"
#define MHL_DRIVER_NAME "ite6811drv"
#define MHL_DRIVER_DESC "ITE6811 Tx Driver"
#define MHL_DEVICE_NAME "ITE6811"
#else   
#error "Need to add name and description strings for new drivers here!"
#endif
#define MHL_DRIVER_MINOR_MAX   1
#define EVENT_POLL_INTERVAL_MS	50
#define MHL_STATE_FLAG_CONNECTED	0x01	
#define MHL_STATE_FLAG_RCP_READY	0x02	
#define MHL_STATE_FLAG_RCP_SENT		0x04	
#define MHL_STATE_FLAG_RCP_RECEIVED	0x08	
#define MHL_STATE_FLAG_RCP_ACK		0x10	
#define MHL_STATE_FLAG_RCP_NAK		0x20	
typedef struct {
	struct device		*pDevice;			
	uint8_t				flags;				
	uint8_t				keyCode;			
	uint8_t				errCode;			
	uint8_t				devCapOffset;		
} MHL_DRIVER_CONTEXT_T, *PMHL_DRIVER_CONTEXT_T;

MHL_DRIVER_CONTEXT_T gDriverContext;
//int32_t ITE6811MhlOpen(struct inode *pInode, struct file *pFile);
//int32_t ITE6811MhlRelease(struct inode *pInode, struct file *pFile);
//long ITE6811MhlIoctl(struct file *pFile, unsigned int ioctlCode,unsigned long ioctlParam);
#ifdef __cplusplus
}
#endif  
#endif 
