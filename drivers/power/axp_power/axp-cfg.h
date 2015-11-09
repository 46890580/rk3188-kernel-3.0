#ifndef _AXP_CFG_H_
#define _AXP_CFG_H_

#if defined(CONFIG_MACH_RK3188_Q72)
#include "q72-cfg.h"
#elif defined(CONFIG_MACH_RK3188_M7R)
#include "m7r-cfg.h"
#elif defined(CONFIG_MACH_RK3188_A1013)
#include "a1013-cfg.h"
#elif defined(CONFIG_MACH_RK3188_Q3188M)
#include "q3188m-cfg.h"
#else
#error "Un-configured machine type."
#endif

#endif
