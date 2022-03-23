//*****************************************************************************
//
// project0.c - Example to demonstrate minimal TivaWare setup
//
// Copyright (c) 2012-2020 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
// 
// Texas Instruments (TI) is supplying this software for use solely and
// exclusively on TI's microcontroller products. The software is owned by
// TI and/or its suppliers, and is protected under applicable copyright
// laws. You may not combine this software with "viral" open-source
// software in order to form a larger program.
// 
// THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
// NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
// NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
// CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
// DAMAGES, FOR ANY REASON WHATSOEVER.
// 
// This is part of revision 2.2.0.295 of the EK-TM4C1294XL Firmware Package.
//
//*****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"
#include "utils/uartstdio.h"


// FreeRTOS includes
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

// Custom files
#include "src/gpio.h"
#include "src/uart.h"
#include "src/timer.h"

// Function prototypes
void vApplicationTickHook( void );
void Timer0IntHandler(void);


//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{
}
#endif

// Declare semaphore
xSemaphoreHandle t1_sem = NULL;
xSemaphoreHandle t2_sem = NULL;
xSemaphoreHandle print_sem = NULL;

// Other vars
volatile uint32_t t1_misses = 0;
volatile uint32_t t2_misses = 0;

// Task definitions
void vApplicationTickHook(void)
{
    static uint16_t tick_counter = 0;
    tick_counter++;
    if(pdTICKS_TO_MS(tick_counter) >= 1000)
    {
        gpio_toggle_led1();
        tick_counter = 0;
    }
}
TaskHandle_t myTask1Handle = NULL;
void task_1(void *p)
{
    static uint32_t time_start_ms = 0;
    static uint32_t time_end_ms = 0;

    while(1)
    {
        if(xSemaphoreTake(t1_sem, portMAX_DELAY) == pdTRUE)
        {
            time_start_ms = pdTICKS_TO_MS(xTaskGetTickCount());
//            if(xSemaphoreTake(print_sem, portMAX_DELAY) == pdTRUE)
//            {
//                UARTprintf("%d: task1 start!\r\n", time_start_ms);
//                xSemaphoreGive(print_sem);
//            }

            volatile unsigned long long i;
            for (i = 0; i < 35000ULL; ++i);

            time_end_ms = pdTICKS_TO_MS(xTaskGetTickCount());
            if(xSemaphoreTake(print_sem, portMAX_DELAY) == pdTRUE)
            {
                UARTprintf("%d: task1\r\n\tstart=%d\r\n\tend=%d\r\n\tcomputation time=%d ms\r\n\tTotal misses=%d\r\n", time_end_ms, time_start_ms, time_end_ms, (time_end_ms - time_start_ms), t2_misses);
                xSemaphoreGive(print_sem);
            }
        }
    }
}

TaskHandle_t myTask2Handle = NULL;
void task_2(void *p)
{
    static uint32_t time_start_ms = 0;
    static uint32_t time_end_ms = 0;

    while(1)
    {
        if(xSemaphoreTake(t2_sem, portMAX_DELAY) == pdTRUE)
        {
            time_start_ms = pdTICKS_TO_MS(xTaskGetTickCount());
//            if(xSemaphoreTake(print_sem, portMAX_DELAY) == pdTRUE)
//            {
//                UARTprintf("%d: task2 start!\r\n", time_start_ms);
//                xSemaphoreGive(print_sem);
//            }

            volatile unsigned long long i;
            for (i = 0; i < 180000ULL; ++i);

            time_end_ms = pdTICKS_TO_MS(xTaskGetTickCount());
            if(xSemaphoreTake(print_sem, portMAX_DELAY) == pdTRUE)
            {
                UARTprintf("%d: task2\r\n\tstart=%d\r\n\tend=%d\r\n\tcomputation time=%d ms\r\n\tTotal misses=%d\r\n", time_end_ms, time_start_ms, time_end_ms, (time_end_ms - time_start_ms), t2_misses);
                xSemaphoreGive(print_sem);
            }
        }
    }
}

//*****************************************************************************
//
// Main 'C' Language entry point.  Toggle an LED using TivaWare.
//
//*****************************************************************************
int main(void)
{
    uint32_t ui32SysClock;

    //
    // Run from the PLL at 120 MHz.
    // Note: SYSCTL_CFG_VCO_240 is a new setting provided in TivaWare 2.2.x and
    // later to better reflect the actual VCO speed due to SYSCTL#22.
    //
    ui32SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                       SYSCTL_OSC_MAIN |
                                       SYSCTL_USE_PLL |
                                       SYSCTL_CFG_VCO_240), 120000000);

    // config UART
    uart_config(115200, ui32SysClock);

    // configure GPIO for blinking LEDs
    gpio_enable_peripheral();
    gpio_configure_leds();

    // enable timer0 module for interrupts
    timer_en_timer0_peripheral();
    // Enable processor interrupts.
    IntMasterEnable();
    timer_configure_timer0(120000); // timer IRQ every 1ms
    timer_enable_timer0();

    // Configure semaphores
    t1_sem = xSemaphoreCreateBinary();
    t2_sem = xSemaphoreCreateBinary();
    print_sem = xSemaphoreCreateBinary();

    // Create tasks here
    UARTprintf("Creating Tasks\r\n");
//    xTaskCreate(
//            task_heart_beat,                // task function
//            "heart_beat",                   // task name
//            configMINIMAL_STACK_SIZE,   // task stack size
//            (void*) 0,                      // variable for args
//            tskIDLE_PRIORITY,               // task priority
//            &myTask1Handle                  // task handler variable
//    );

    xTaskCreate(
            task_1,                // task function
            "t_1",                   // task name
            configMINIMAL_STACK_SIZE * 3,   // task stack size
            (void*) 0,                      // variable for args
            tskIDLE_PRIORITY,               // task priority
            &myTask1Handle                  // task handler variable
    );

    xTaskCreate(
            task_2,                // task function
            "t_2",                   // task name
            configMINIMAL_STACK_SIZE * 3,   // task stack size
            (void*) 0,                      // variable for args
            tskIDLE_PRIORITY,               // task priority
            &myTask2Handle                  // task handler variable
    );

    // start scheduler
    UARTprintf("STARTING TASK SCHEDULER\r\n");
    xSemaphoreGive(print_sem);
    vTaskStartScheduler();

    // we should never get here
    while(1)
    {
        UARTprintf("SHOULD NOT BE HERE\r\n");
    }
}

void Timer0IntHandler(void)
{
    static uint32_t t1_iterations = 0;
    static uint32_t t2_iterations = 0;
    static uint32_t global_time = 0;
    UBaseType_t saved_interrupt_status;

    saved_interrupt_status = taskENTER_CRITICAL_FROM_ISR();
    global_time = pdTICKS_TO_MS(xTaskGetTickCountFromISR());

    // UARTprintf("timer0; Inside ISR!\r\nGlobaltime=%d\r\n", global_time);

    // 1. Clear the interrupt src
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    // 2. Process the interrupt
    if(global_time/30 > t1_iterations)
    {
        if(uxSemaphoreGetCount(t1_sem) == 0)
        {
            xSemaphoreGiveFromISR(t1_sem, NULL);
        }
        else
        {
            t1_misses++;
        }
        t1_iterations++;
    }

    if(global_time/80 > t2_iterations)
    {
        if(uxSemaphoreGetCount(t2_sem) == 0)
        {
            xSemaphoreGiveFromISR(t2_sem, NULL);
        }
        else
        {
            t2_misses++;
        }
        t2_iterations++;
    }
    taskEXIT_CRITICAL_FROM_ISR(saved_interrupt_status);
}
