#ifndef _OS_TYPES_H
#define _OS_TYPES_H

#if !defined(__KERNEL__)
#include <sys/time.h>
#include <sys/uio.h>
#include "ite_types.h"
#endif

#include "ite_ids.h"
#include "ite_inline.h"

#define OS_NO_WAIT          0
#define OS_INFINITE_WAIT    -1
#define OS_MAX_TIMEOUT      (1000*60*60*24) 
typedef struct timeval ITE6811OsTimeVal_t;
typedef struct iovec ITE6811OsIoVec_t;
typedef uint32_t ITE6811OsIpAddr_t;
typedef enum
{
    ITE6811_OS_STATUS_SUCCESS                = ITE6811_STATUS_SET_GROUP(ITE6811_GROUP_OSAL, ITE6811_STATUS_SUCCESS),             
    ITE6811_OS_STATUS_WARN_PENDING           = ITE6811_STATUS_SET_GROUP(ITE6811_GROUP_OSAL, ITE6811_STATUS_WARN_PENDING),        
    ITE6811_OS_STATUS_WARN_BREAK             = ITE6811_STATUS_SET_GROUP(ITE6811_GROUP_OSAL, ITE6811_STATUS_WARN_BREAK),          
    ITE6811_OS_STATUS_WARN_INCOMPLETE        = ITE6811_STATUS_SET_GROUP(ITE6811_GROUP_OSAL, ITE6811_STATUS_WARN_INCOMPLETE),     
    ITE6811_OS_STATUS_ERR_INVALID_HANDLE     = ITE6811_STATUS_SET_GROUP(ITE6811_GROUP_OSAL, ITE6811_STATUS_ERR_INVALID_HANDLE),  
    ITE6811_OS_STATUS_ERR_INVALID_PARAM      = ITE6811_STATUS_SET_GROUP(ITE6811_GROUP_OSAL, ITE6811_STATUS_ERR_INVALID_PARAM),   
    ITE6811_OS_STATUS_ERR_INVALID_OP         = ITE6811_STATUS_SET_GROUP(ITE6811_GROUP_OSAL, ITE6811_STATUS_ERR_INVALID_OP),      
    ITE6811_OS_STATUS_ERR_NOT_AVAIL          = ITE6811_STATUS_SET_GROUP(ITE6811_GROUP_OSAL, ITE6811_STATUS_ERR_NOT_AVAIL),       
    ITE6811_OS_STATUS_ERR_IN_USE             = ITE6811_STATUS_SET_GROUP(ITE6811_GROUP_OSAL, ITE6811_STATUS_ERR_IN_USE),          
    ITE6811_OS_STATUS_ERR_TIMEOUT            = ITE6811_STATUS_SET_GROUP(ITE6811_GROUP_OSAL, ITE6811_STATUS_ERR_TIMEOUT),         
    ITE6811_OS_STATUS_ERR_FAILED             = ITE6811_STATUS_SET_GROUP(ITE6811_GROUP_OSAL, ITE6811_STATUS_ERR_FAILED),          
    ITE6811_OS_STATUS_ERR_NOT_IMPLEMENTED    = ITE6811_STATUS_SET_GROUP(ITE6811_GROUP_OSAL, ITE6811_STATUS_ERR_NOT_IMPLEMENTED), 
    ITE6811_OS_STATUS_ERR_SEM_COUNT_EXCEEDED = ITE6811_STATUS_SET_GROUP(ITE6811_GROUP_OSAL, ITE6811_STATUS_ERR_CUSTOM1),         
    ITE6811_OS_STATUS_ERR_QUEUE_EMPTY        = ITE6811_STATUS_SET_GROUP(ITE6811_GROUP_OSAL, ITE6811_STATUS_ERR_CUSTOM2),         
    ITE6811_OS_STATUS_ERR_QUEUE_FULL         = ITE6811_STATUS_SET_GROUP(ITE6811_GROUP_OSAL, ITE6811_STATUS_ERR_CUSTOM3),         
    ITE6811_OS_STATUS_LAST
} ITE6811OsStatus_t;
static ITE6811_INLINE const char * ITE6811OsStatusString(const ITE6811OsStatus_t status)
{
    switch (status)
    {
        case ITE6811_OS_STATUS_SUCCESS:
            return "Success";
        case ITE6811_OS_STATUS_WARN_PENDING:
            return "Warning-Operation Pending";
        case ITE6811_OS_STATUS_WARN_BREAK:
            return "Warning-Break";
        case ITE6811_OS_STATUS_WARN_INCOMPLETE:
            return "Warning-Operation Incomplete";
        case ITE6811_OS_STATUS_ERR_INVALID_HANDLE:
            return "Error-Invalid Handle";
        case ITE6811_OS_STATUS_ERR_INVALID_PARAM:
            return "Error-Invalid Parameter";
        case ITE6811_OS_STATUS_ERR_INVALID_OP:
            return "Error-Invalid Operation";
        case ITE6811_OS_STATUS_ERR_NOT_AVAIL:
            return "Error-Resource Not Available";
        case ITE6811_OS_STATUS_ERR_IN_USE:
            return "Error-Resource In Use";
        case ITE6811_OS_STATUS_ERR_TIMEOUT:
            return "Error-Timeout Expired";
        case ITE6811_OS_STATUS_ERR_FAILED:
            return "Error-General Failure";
        case ITE6811_OS_STATUS_ERR_NOT_IMPLEMENTED:
            return "Error-Not Implemented";
        case ITE6811_OS_STATUS_ERR_SEM_COUNT_EXCEEDED:
            return "Error-Semaphore Count Exceeded";
        case ITE6811_OS_STATUS_ERR_QUEUE_EMPTY:
            return "Error-Queue Empty";
		case ITE6811_OS_STATUS_ERR_QUEUE_FULL:
			return "Error-Queue Full";	
        default:
            return "UNKNOWN";
    }
}
#endif 
