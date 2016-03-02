#ifndef _OSAL_H
#define _OSAL_H


typedef bool bool_t;
#include "os_types.h"
#include "os_data.h"

#if 0
ITE6811OsStatus_t ITE6811OsSemaphoreCreate
(
    const char *pName,
    uint32_t maxCount,
    uint32_t initialValue,
    ITE6811OsSemaphore_t *pRetSemId
);

#ifndef PLATFORM3
#if defined(OS_CONFIG_DEBUG)
void ITE6811OsSemaphoreDumpList(uint32_t channel);
#define ITE6811_OS_SEMAPHORE_DUMP_LIST(channel) ITE6811OsSemaphoreDumpList(channel)
#else
#define ITE6811_OS_SEMAPHORE_DUMP_LIST(channel) 
#endif 
#endif 
ITE6811OsStatus_t ITE6811OsQueueCreate
(
    const char *pName,
    size_t   elementSize,
    uint32_t maxElements,
    ITE6811OsQueue_t *pRetQueueId
);
ITE6811OsStatus_t ITE6811OsQueueDelete(ITE6811OsQueue_t queueId);
ITE6811OsStatus_t ITE6811OsQueueSend
(
    ITE6811OsQueue_t queueId,
    const void *pBuffer,
    size_t size
);
ITE6811OsStatus_t ITE6811OsQueueReceive
(
    ITE6811OsQueue_t queueId,
    void *pBuffer,
    int32_t timeMsec,
    size_t *pSize
);
#ifndef PLATFORM3
#if defined(OS_CONFIG_DEBUG)
void ITE6811OsQueueDumpList(uint32_t channel);
#define ITE6811_OS_QUEUE_DUMP_LIST(channel) ITE6811OsQueueDumpList(channel)
#else
#define ITE6811_OS_QUEUE_DUMP_LIST(channel) 
#endif 
#endif 
ITE6811OsStatus_t ITE6811OsTaskCreate
(
    const char *pName,
    void (*pTaskFunction)(void *pArg),
    void *pTaskArg,
    uint32_t priority,
    size_t stackSize,
    ITE6811OsTask_t *pRetTaskId
);
void ITE6811OsTaskSelfDelete(void);
ITE6811OsStatus_t ITE6811OsTaskSleepUsec(uint64_t timeUsec);
#ifndef PLATFORM3
#if defined(OS_CONFIG_DEBUG)
void ITE6811OsTaskDumpList(uint32_t channel);
#define ITE6811_OS_TASK_DUMP_LIST(channel) ITE6811OsTaskDumpList(channel)
#else
#define ITE6811_OS_TASK_DUMP_LIST(channel) 
#endif 
#endif 


void ITE6811OsGetTimeCurrent(ITE6811OsTime_t *pRetTime);
int64_t ITE6811OsGetTimeDifferenceMs
(
    const ITE6811OsTime_t *pTime1,
    const ITE6811OsTime_t *pTime2
);
#ifndef PLATFORM3
#if defined(OS_CONFIG_DEBUG)
void ITE6811OsTimerDumpList(uint32_t channel);
#define ITE6811_OS_TIMER_DUMP_LIST(channel) ITE6811OsTimerDumpList(channel)
#else
#define ITE6811_OS_TIMER_DUMP_LIST(channel) 
#endif 
#endif 
void * ITE6811OsAlloc
(
    const char *pName,
    size_t size,
    uint32_t flags
);
void * ITE6811OsCalloc
(
    const char *pName,
    size_t size,
    uint32_t flags
);
void ITE6811OsFree(void *pAddr);

#endif

#endif 
