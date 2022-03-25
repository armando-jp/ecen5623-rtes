/*
 * timer.h
 *
 *  Created on: Mar 21, 2022
 *      Author: arman
 */

#ifndef SRC_TIMER_H_
#define SRC_TIMER_H_

#include <stdint.h>

void Timer0IntHandler();
void timer_en_timer0_peripheral();
void timer_configure_timer0(uint32_t count);
void timer_enable_timer0();
void timer_disable_timer0();


#endif /* SRC_TIMER_H_ */
