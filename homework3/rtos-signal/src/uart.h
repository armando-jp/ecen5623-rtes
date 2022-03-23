/*
 * uart.h
 *
 *  Created on: Mar 21, 2022
 *      Author: arman
 */

#ifndef SRC_UART_H_
#define SRC_UART_H_

#include <stdint.h>

void uart_config(uint32_t baud, uint32_t sys_clock);

#endif /* SRC_UART_H_ */
