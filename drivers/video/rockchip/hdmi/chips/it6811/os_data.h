#ifndef _OS_DATA_H
#define _OS_DATA_H
#ifdef PLATFORM3
#error This file is not designed for use on the specified platform.
#endif
#if !defined(__KERNEL__)
#include <sys/time.h>
#include <bits/local_lim.h>
#include <pthread.h>
#else
#include <linux/time.h>
#endif
#define OS_API_NAME_SIZE    16                      
#define OS_PRIV_NAME_SIZE   (OS_API_NAME_SIZE + 16) 
#define OS_NAME_SIZE        (OS_PRIV_NAME_SIZE + 16) 
#define ITE6811_OS_STACK_SIZE_MIN    PTHREAD_STACK_MIN   
struct _ITE6811OsTaskInfo_t;
#define OS_CURRENT_TASK    ((struct _ITE6811OsTaskInfo_t *) 0)
typedef struct _ITE6811OsSemInfo_t * ITE6811OsSemaphore_t;
typedef struct _ITE6811OsQueueInfo_t * ITE6811OsQueue_t;
typedef struct _ITE6811OsTaskInfo_t * ITE6811OsTask_t;
typedef struct
{
    struct timeval theTime;
} ITE6811OsTime_t;
typedef struct _ITE6811OsTimerInfo_t * ITE6811OsTimer_t;
#endif 
