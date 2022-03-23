/*
 * task.c
 *
 *  Created on: Mar 22, 2022
 *      Author: arman
 */

#include <stdint.h>
#include <stdbool.h>

#include "driverlib/interrupt.h"
#include "utils/uartstdio.h"

#include "src/gpio.h"
#include "src/task.h"

extern volatile uint32_t time;
extern volatile uint32_t events;

void task1()
{
    IntMasterDisable();
    if(events & 0x01)
    {
        // time is printed in seconds
        UARTprintf("%d: Task1 was signaled by timer ISR.\r\n", time);
        gpio_toggle_led1();
        events = events & ~(0x01);
    }
    IntMasterEnable();
}
