#ifndef __ITE_COMMON_H__
#define __ITE_COMMON_H__
#if defined(__KERNEL__)
#include "ite_hal.h"
#include "ite_osdebug.h"
#include "ite_app_devcap.h"
#else
#include "ite_debug.h"
#endif
#include "ite_platform.h"
typedef enum _ITE6811ResultCodes_t
{
    PLATFORM_SUCCESS      = 0,           
    ITE6811_SUCCESS      = 0,           
    ITE6811_ERR_FAIL,                   
    ITE6811_ERR_INVALID_PARAMETER,      
    ITE6811_ERR_IN_USE,                 
    ITE6811_ERR_NOT_AVAIL,              
} ITE6811ResultCodes_t;
typedef enum
{
    ITE6811SYSTEM_NONE      = 0,
    ITE6811SYSTEM_SINK,
    ITE6811SYSTEM_SWITCH,
    ITE6811SYSTEM_SOURCE,
    ITE6811SYSTEM_REPEATER,
} ITE6811SystemTypes_t;
#define YES                         1
#define NO                          0
#ifdef NEVER
uint8_t ITE6811TimerExpired( uint8_t timer );
long    ITE6811TimerElapsed( uint8_t index );
long    ITE6811TimerTotalElapsed ( void );
void    ITE6811TimerWait( uint16_t m_sec );
void    ITE6811TimerSet( uint8_t index, uint16_t m_sec );
void    ITE6811TimerInit( void );
#endif
#endif  
