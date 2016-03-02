#ifndef __ITE_OSDEBUG_H__
#define __ITE_OSDEBUG_H__
typedef enum
{
	 ITE6811_OS_DEBUG_FORMAT_SIMPLE   	= 0x0000u
	,ITE6811_OS_DEBUG_FORMAT_FILEINFO 	= 0x0001u
	,ITE6811_OS_DEBUG_FORMAT_CHANNEL  	= 0x0002u
	,ITE6811_OS_DEBUG_FORMAT_TIMESTAMP	= 0x0004u
}ITE6811OsDebugFormat_e;

#define MODULE_SET(name) \
    ITE6811_OSAL_DEBUG_##name, \
    ITE6811_OSAL_DEBUG_##name##_DBG =ITE6811_OSAL_DEBUG_##name, \
    ITE6811_OSAL_DEBUG_##name##_ERR, \
    ITE6811_OSAL_DEBUG_##name##_TRACE, \
    ITE6811_OSAL_DEBUG_##name##_ALWAYS, \
    ITE6811_OSAL_DEBUG_##name##_USER1, \
    ITE6811_OSAL_DEBUG_##name##_USER2, \
    ITE6811_OSAL_DEBUG_##name##_USER3, \
    ITE6811_OSAL_DEBUG_##name##_USER4, \
    ITE6811_OSAL_DEBUG_##name##_MASK = ITE6811_OSAL_DEBUG_##name##_USER4,

typedef enum t_ITE6811OsalDebugChannels_e
{
	MODULE_SET(APP)
	MODULE_SET(TRACE)
    MODULE_SET(POWER_MAN)
    MODULE_SET(TX)
    MODULE_SET(EDID)
    MODULE_SET(HDCP)
    MODULE_SET(AV_CONFIG)
    MODULE_SET(ENTRY_EXIT)
    MODULE_SET(CBUS)
    MODULE_SET(SCRATCHPAD)
    MODULE_SET(SCHEDULER)
    MODULE_SET(CRA)
    ITE6811_OSAL_DEBUG_NUM_CHANNELS
}ITE6811OsalDebugChannels_e;
#ifndef ITE6811_DEBUG_CONFIG_RESOURCE_CONSTRAINED 
typedef void ITE6811OsDebugChannel_t;
uint32_t ITE6811OsDebugChannelAdd(uint32_t numChannels, ITE6811OsDebugChannel_t *paChannelList);
#endif 
void ITE6811OsDebugChannelEnable(ITE6811OsalDebugChannels_e channel);
void ITE6811OsDebugChannelDisable(ITE6811OsalDebugChannels_e channel);
#define ITE_OS_DISABLE_DEBUG_CHANNEL(channel) ITE6811OsDebugChannelDisable(channel)
bool_t ITE6811OsDebugChannelIsEnabled(ITE6811OsalDebugChannels_e channel);
void ITE6811OsDebugSetConfig(uint16_t flags);
#define ITE6811OsDebugConfig(flags) ITE6811OsDebugSetConfig(flags)
uint16_t ITE6811OsDebugGetConfig(void);
void ITE6811OsDebugPrintSimple(ITE6811OsalDebugChannels_e channel, char *pszFormat,...);
void ITE6811OsDebugPrintShort(ITE6811OsalDebugChannels_e channel, char *pszFormat,...);
void ITE6811OsDebugPrint(const char *pFileName, uint32_t iLineNum, ITE6811OsalDebugChannels_e channel, const char *pszFormat, ...);

#define ITE6811_DEBUG_PRINT printk

#define ITE6811_DEBUG(channel,x) if (ITE6811OsDebugChannelIsEnabled(channel) {x}
#define ITE6811_PRINT_FULL(channel,...) ITE6811OsDebugPrint(__FILE__,__LINE__,channel,__VA_ARGS__)
#define ITE6811_PRINT(channel,...) ITE6811OsDebugPrintShort(channel,__VA_ARGS__)
#define ITE6811_PRINT_PLAIN(channel,...) ITE6811OsDebugPrintSimple(channel,__VA_ARGS__)
#endif 
