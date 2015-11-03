#ifndef _AXP_CFG_H_
#define _AXP_CFG_H_

#if defined(CONFIG_MACH_RK3188_Q72)
#include "q72-cfg.h"
#elif defined(CONFIG_MACH_RK3188_M7R)
#include "m7r-cfg.h"
#else
#error "Un-configured machine type."
#endif

#endif
