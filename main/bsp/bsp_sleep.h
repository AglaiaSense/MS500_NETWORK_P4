#ifndef BSP_SLEEP_H
#define BSP_SLEEP_H

#include "vc_config.h"


void enter_light_sleep_time(uint64_t sleep_time_us) ;
void enter_deep_sleep_time(uint64_t sleep_time_us) ;

void enter_light_sleep_gpio() ;
void enter_deep_sleep_gpio() ;


#endif /* BSP_SLEEP_H */
