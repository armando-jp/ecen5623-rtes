/*
 * timer.c
 *
 *  Created on: Mar 21, 2022
 *      Author: arman
 */

#include <stdint.h>
#include <stdbool.h>

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"
#include "src/gpio.h"
#include "utils/uartstdio.h"

extern volatile uint32_t time;
extern volatile uint32_t events;


void timer_en_timer0_peripheral()
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER0))
    {
    }
}


void timer_configure_timer0(uint32_t count)
{
    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);

    // Set clock source to system clock (120 MHz)
    // TimerClockSourceSet(TIMER0_BASE, TIMER_CLOCK_SYSTEM);

    // Set the timer interrupt handler
    // TimerIntRegister(TIMER0_BASE, TIMER_A, timer_IRQ);

    // for 1 second = 120000000
    TimerLoadSet(TIMER0_BASE, TIMER_A, count);

    // Setup the interrupt for the timer timeout.
    IntEnable(INT_TIMER0A);
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
}


void timer_enable_timer0()
{
    TimerEnable(TIMER0_BASE, TIMER_A);
}

void timer_disable_timer0()
{
    TimerIntDisable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    TimerDisable(TIMER0_BASE, TIMER_A);
}
