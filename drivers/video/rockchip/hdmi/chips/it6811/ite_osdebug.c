
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/kthread.h>


#include "ite_common.h"
#include "ite_osdebug.h"
#include <linux/slab.h>
#include <linux/string.h>
#define ChannelIndex(channel) (channel >> 3)
#define ChannelMask(channel) (1 << (channel & 7))
extern unsigned char DebugChannelMasks[];
extern ushort DebugFormat;
#if defined(DEBUG)
unsigned char DebugChannelMasks[ITE6811_OSAL_DEBUG_NUM_CHANNELS/8+1]={0xFF,0xFF,0xFF,0xFF};
module_param_array(DebugChannelMasks, byte, NULL, S_IRUGO | S_IWUSR);
ushort DebugFormat = ITE6811_OS_DEBUG_FORMAT_FILEINFO;
module_param(DebugFormat, ushort, S_IRUGO | S_IWUSR);
#endif

void ITE6811OsDebugChannelEnable(ITE6811OsalDebugChannels_e channel)
{
#if defined(DEBUG)
	uint8_t index = ChannelIndex(channel);
	uint8_t mask  = ChannelMask(channel) ;
    DebugChannelMasks[index] |= mask;
#endif
}
void ITE6811OsDebugChannelDisable(ITE6811OsalDebugChannels_e channel)
{
#if defined(DEBUG)
	uint8_t index =ChannelIndex(channel);
	uint8_t mask  =ChannelMask(channel) ;
    DebugChannelMasks[index] &= ~mask;
#endif
}
bool_t ITE6811OsDebugChannelIsEnabled(ITE6811OsalDebugChannels_e channel)
{
#if defined(DEBUG)
	uint8_t index = ChannelIndex(channel);
	uint8_t mask  = ChannelMask(channel) ;
    return (DebugChannelMasks[index] & mask)?true:false;
#else
    return false;
#endif
}
void ITE6811OsDebugSetConfig(uint16_t flags)
{
#if defined(DEBUG)
    DebugFormat = flags;
#endif
}
uint16_t ITE6811OsDebugGetConfig(void)
{
#if defined(DEBUG)
    return DebugFormat;
#else
    return 0;
#endif
}
void ITE6811OsDebugPrintSimple(ITE6811OsalDebugChannels_e channel, char *pszFormat,...)
{
#if 1
//#if defined(DEBUG)
    if (ITE6811OsDebugChannelIsEnabled( channel ))
    {
    	va_list ap;
    	va_start(ap,pszFormat);
    	printk(pszFormat, ap);
    	va_end(ap);
    }
#endif
}
void ITE6811OsDebugPrintShort(ITE6811OsalDebugChannels_e channel, char *pszFormat,...)
{
//#if defined(DEBUG)
#if 1
    if (ITE6811OsDebugChannelIsEnabled( channel ))
    {
	    va_list ap;
	    va_start(ap,pszFormat);
	    ITE6811OsDebugPrint(NULL, 0, channel, pszFormat, ap);
	    va_end(ap);
	}
#endif
}
#define MAX_DEBUG_MSG_SIZE	512
void ITE6811OsDebugPrint(const char *pszFileName, uint32_t iLineNum, uint32_t channel, const char *pszFormat, ...)
{
#if defined(DEBUG)
	uint8_t		*pBuf = NULL;
	uint8_t		*pBufOffset;
	int			remainingBufLen = MAX_DEBUG_MSG_SIZE;
	int			len;
	va_list		ap;
    if (ITE6811OsDebugChannelIsEnabled( channel ))
    {
	    pBuf = kmalloc(remainingBufLen, GFP_KERNEL);
	    if(pBuf == NULL)
	    	return;
	    pBufOffset = pBuf;
        if(pszFileName != NULL && (ITE6811_OS_DEBUG_FORMAT_FILEINFO & DebugFormat))
        {
        	const char *pc;
            for(pc = &pszFileName[strlen(pszFileName)];pc  >= pszFileName;--pc)
            {
                if ('\\' == *pc)
                {
                    ++pc;
                    break;
                }
                if ('/' ==*pc)
                {
                    ++pc;
                    break;
                }
            }
            len = scnprintf(pBufOffset, remainingBufLen, "%s:%d ",pc,(int)iLineNum);
            if(len < 0) {
            	kfree(pBuf);
            	return;
            }
            remainingBufLen -= len;
            pBufOffset += len;
        }
        if (ITE6811_OS_DEBUG_FORMAT_CHANNEL & DebugFormat)
        {
        	len = scnprintf(pBufOffset, remainingBufLen, "Chan:%d ", channel);
        	if(len < 0) {
        		kfree(pBuf);
        		return;
        	}
            remainingBufLen -= len;
            pBufOffset += len;
        }
        va_start(ap,pszFormat);
        vsnprintf(pBufOffset, remainingBufLen, pszFormat, ap);
        va_end(ap);
    	printk(pBuf);
		kfree(pBuf);
    }
#endif 
}
