/*
 * gpio.c
 *
 *  Created on: Mar 21, 2022
 *      Author: arman
 */

#include <stdint.h>
#include <stdbool.h>

#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "src/gpio.h"

#define USER_LED1  GPIO_PIN_0
#define USER_LED2  GPIO_PIN_1

void gpio_enable_peripheral()
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPION))
    {
    }
}

void gpio_configure_leds()
{
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, (USER_LED1|USER_LED2));
}


void gpio_toggle_led1()
{
    uint32_t pin_val;
    pin_val = GPIOPinRead(GPIO_PORTN_BASE, USER_LED1);
    GPIOPinWrite(GPIO_PORTN_BASE, (USER_LED1), pin_val^1);
}

void gpio_on_led1()
{
    GPIOPinWrite(GPIO_PORTN_BASE, USER_LED1, 1);
}
