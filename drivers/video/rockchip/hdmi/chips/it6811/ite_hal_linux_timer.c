#include <linux/delay.h>

#include "ite_hal.h"
#include "ite_hal_priv.h"
void HalTimerWait(uint16_t m_sec)
{
	unsigned long	time_usec = m_sec;
	msleep(time_usec);
}
