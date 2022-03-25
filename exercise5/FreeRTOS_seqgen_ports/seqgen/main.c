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
#include "src/uart.h"
#include "src/gpio.h"

/******************************************************************************
 * Function prototypes
 *****************************************************************************/
void vApplicationTickHook( void );


/******************************************************************************
 * The error routine that is called if the driver library encounters an error.
 *****************************************************************************/
#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{
}
#endif

/******************************************************************************
 * Declare semaphore variables & abort variables
 *****************************************************************************/
xSemaphoreHandle t1_sem = NULL;
xSemaphoreHandle t2_sem = NULL;
xSemaphoreHandle t3_sem = NULL;
xSemaphoreHandle t4_sem = NULL;
xSemaphoreHandle t5_sem = NULL;
xSemaphoreHandle t6_sem = NULL;
xSemaphoreHandle t7_sem = NULL;
xSemaphoreHandle print_sem = NULL;

int abortTest   = 0;
int abortS1     = 0;
int abortS2     = 0;
int abortS3     = 0;
int abortS4     = 0;
int abortS5     = 0;
int abortS6     = 0;
int abortS7     = 0;

uint32_t sequencer_delay_time = pdMS_TO_TICKS(33);
uint32_t sequence_periods = 900;

////////////////////////////////////////////////////////////////////////////////
// Sequencer definitions
//      * Initially print/log its start time.
//      * DO/WHILE (!abortTest && (sequence count < total sequences)):
//      *       DO/WHILE (there is still sleep time remaining):
//      *           SLEEP FOR 33.33 msec (30 Hz)
//      *           IF (pause interrupted by signal):
//      *               save and print remaining time
//      *               increment delay counter
//      *           IF (pause return < 0):
//      *               error. terminate thread.
//      *       Increment sequence count
//      *       Log current time
//      *       IF (looping delay > 0), Log
//      *       IF (sequence count a ratio of task period): release the sem.
//
//      This sequencer is running in an ISR. Therefore, we can't hang out in an
//      big ole while loop. Instead, we will keep track of ticks and compare
//      those to our delay time variable (which is in ticks). This function is
//      on every system tick.
////////////////////////////////////////////////////////////////////////////////
void vApplicationTickHook(void)
{
    // Here lies the sequencer!
    static int printed_start_msg = 0;
    static uint32_t seqCnt = 0;
    static uint32_t current_time_val = 0;

    // Print the sequencer start time.
    if(!printed_start_msg)
    {
        if(xSemaphoreTakeFromISR(print_sem, NULL) == pdTRUE)
        {
            current_time_val = pdTICKS_TO_MS(xTaskGetTickCount());
            UARTprintf("sequencer start @ sec = %d, msec = %d!\r\n", current_time_val/1000.0, current_time_val%1000);
            xSemaphoreGiveFromISR(print_sem, NULL);
        }
        printed_start_msg = 1;
    }

    // Check if sequence period has been met
    if(seqCnt < sequence_periods)
    {
        // check if it a sequencer period has elapsed (33ms)
        if((xTaskGetTickCount()%sequencer_delay_time) == 0)
        {
            seqCnt++;
            if(xSemaphoreTakeFromISR(print_sem, NULL) == pdTRUE)
            {
                current_time_val = pdTICKS_TO_MS(xTaskGetTickCount());
                UARTprintf("sequencer cycle %d @ sec = %d, msec = %d!\r\n", seqCnt, current_time_val/1000, current_time_val%1000);
                xSemaphoreGiveFromISR(print_sem, NULL);
            }

            // Servcie_1 = RT_MAX-1 @ 3 Hz
             if((seqCnt % 10) == 0) xSemaphoreGiveFromISR(t1_sem, NULL);

            // Service_2 = RT_MAX-2 @ 1 Hz
            if((seqCnt % 30) == 0) xSemaphoreGiveFromISR(t2_sem, NULL);

            // Service_3 = RT_MAX-3 @ 0.5 Hz
            if((seqCnt % 60) == 0) xSemaphoreGiveFromISR(t3_sem, NULL);

            // Service_4 = RT_MAX-2 @ 1 Hz
            if((seqCnt % 30) == 0) xSemaphoreGiveFromISR(t4_sem, NULL);

            // Service_5 = RT_MAX-3 @ 0.5 Hz
            if((seqCnt % 60) == 0) xSemaphoreGiveFromISR(t5_sem, NULL);

            // Service_6 = RT_MAX-2 @ 1 Hz
            if((seqCnt % 30) == 0) xSemaphoreGiveFromISR(t6_sem, NULL);

            // Service_7 = RT_MIN   0.1 Hz
            if((seqCnt % 300) == 0) xSemaphoreGiveFromISR(t7_sem, NULL);

        }
    }
    else
    {
        // release all semaphores
        xSemaphoreGiveFromISR(t1_sem, NULL);
        xSemaphoreGiveFromISR(t2_sem, NULL);
        xSemaphoreGiveFromISR(t3_sem, NULL);
        xSemaphoreGiveFromISR(t4_sem, NULL);
        xSemaphoreGiveFromISR(t5_sem, NULL);
        xSemaphoreGiveFromISR(t6_sem, NULL);
        xSemaphoreGiveFromISR(t7_sem, NULL);
        // set abort flag to true
        abortS1     = 1;
        abortS2     = 1;
        abortS3     = 1;
        abortS4     = 1;
        abortS5     = 1;
        abortS6     = 1;
        abortS7     = 1;
    }


//    tick_counter++;
//    if(pdTICKS_TO_MS(tick_counter) >= 1000)
//    {
//        gpio_toggle_led1();
//        tick_counter = 0;
//    }
}

/******************************************************************************
 * Service definitions
 * All services will:
 *      * Initially print/log their start time.
 *      * WHILE !abortS#
 *      *       SEM_TAKE(semS#)
 *      *       S#Cnt++
 *      *       GET TIME
 *      *       LOG RELEASE TIME
 *****************************************************************************/
TaskHandle_t myTask1Handle = NULL;
void service_1(void *p)
{
    static uint32_t current_time_val = 0;
    unsigned long S1Cnt=0;

    // Print the service
    if(xSemaphoreTake(print_sem, portMAX_DELAY) == pdTRUE)
    {
        current_time_val = pdTICKS_TO_MS(xTaskGetTickCount());
        UARTprintf("service_1 start @ sec = %d, msec = %d!\r\n", current_time_val/1000, current_time_val%1000);
        xSemaphoreGive(print_sem);
    }

    while(!abortS1)
    {
        if(xSemaphoreTake(t1_sem, portMAX_DELAY) == pdTRUE)
        {
            // Increment counter variable
            S1Cnt++;
            // Print the current time
            if(xSemaphoreTake(print_sem, portMAX_DELAY) == pdTRUE)
            {
                current_time_val = pdTICKS_TO_MS(xTaskGetTickCount());
                UARTprintf("service_1 release @ sec = %d, msec = %d!\r\n", current_time_val/1000, current_time_val%1000);
                xSemaphoreGive(print_sem);
            }
        }
    }
}

TaskHandle_t myTask2Handle = NULL;
void service_2(void *p)
{
    static uint32_t current_time_val = 0;
    unsigned long S2Cnt=0;

    // Print the service
    if(xSemaphoreTake(print_sem, portMAX_DELAY) == pdTRUE)
    {
        current_time_val = pdTICKS_TO_MS(xTaskGetTickCount());
        UARTprintf("service_2 start @ sec = %d, msec = %d!\r\n", current_time_val/1000, current_time_val%1000);
        xSemaphoreGive(print_sem);
    }

    while(!abortS2)
    {
        if(xSemaphoreTake(t2_sem, portMAX_DELAY) == pdTRUE)
        {
            // Increment counter variable
            S2Cnt++;
            // Print the current time
            if(xSemaphoreTake(print_sem, portMAX_DELAY) == pdTRUE)
            {
                current_time_val = pdTICKS_TO_MS(xTaskGetTickCount());
                UARTprintf("service_2 release @ sec = %d, msec = %d!\r\n", current_time_val/1000, current_time_val%1000);
                xSemaphoreGive(print_sem);
            }
        }
    }
}

TaskHandle_t myTask3Handle = NULL;
void service_3(void *p)
{
    static uint32_t current_time_val = 0;
    unsigned long S3Cnt=0;

    // Print the service
    if(xSemaphoreTake(print_sem, portMAX_DELAY) == pdTRUE)
    {
        current_time_val = pdTICKS_TO_MS(xTaskGetTickCount());
        UARTprintf("service_3 start @ sec = %d, msec = %d!\r\n", current_time_val/1000, current_time_val%1000);
        xSemaphoreGive(print_sem);
    }

    while(!abortS3)
    {
        if(xSemaphoreTake(t3_sem, portMAX_DELAY) == pdTRUE)
        {
            // Increment counter variable
            S3Cnt++;
            // Print the current time
            if(xSemaphoreTake(print_sem, portMAX_DELAY) == pdTRUE)
            {
                current_time_val = pdTICKS_TO_MS(xTaskGetTickCount());
                UARTprintf("service_3 release @ sec = %d, msec = %d!\r\n", current_time_val/1000, current_time_val%1000);
                xSemaphoreGive(print_sem);
            }
        }
    }
}

TaskHandle_t myTask4Handle = NULL;
void service_4(void *p)
{
    static uint32_t current_time_val = 0;
    unsigned long S4Cnt=0;

    // Print the service
    if(xSemaphoreTake(print_sem, portMAX_DELAY) == pdTRUE)
    {
        current_time_val = pdTICKS_TO_MS(xTaskGetTickCount());
        UARTprintf("service_4 start @ sec = %d, msec = %d!\r\n", current_time_val/1000, current_time_val%1000);
        xSemaphoreGive(print_sem);
    }

    while(!abortS4)
    {
        if(xSemaphoreTake(t4_sem, portMAX_DELAY) == pdTRUE)
        {
            // Increment counter variable
            S4Cnt++;
            // Print the current time
            if(xSemaphoreTake(print_sem, portMAX_DELAY) == pdTRUE)
            {
                current_time_val = pdTICKS_TO_MS(xTaskGetTickCount());
                UARTprintf("service_4 release @ sec = %d, msec = %d!\r\n", current_time_val/1000, current_time_val%1000);
                xSemaphoreGive(print_sem);
            }
        }
    }
}

TaskHandle_t myTask5Handle = NULL;
void service_5(void *p)
{
    static uint32_t current_time_val = 0;
    unsigned long S5Cnt=0;

    // Print the service
    if(xSemaphoreTake(print_sem, portMAX_DELAY) == pdTRUE)
    {
        current_time_val = pdTICKS_TO_MS(xTaskGetTickCount());
        UARTprintf("service_5 start @ sec = %d, msec = %d!\r\n", current_time_val/1000, current_time_val%1000);
        xSemaphoreGive(print_sem);
    }

    while(!abortS5)
    {
        if(xSemaphoreTake(t5_sem, portMAX_DELAY) == pdTRUE)
        {
            // Increment counter variable
            S5Cnt++;
            // Print the current time
            if(xSemaphoreTake(print_sem, portMAX_DELAY) == pdTRUE)
            {
                current_time_val = pdTICKS_TO_MS(xTaskGetTickCount());
                UARTprintf("service_5 release @ sec = %d, msec = %d!\r\n", current_time_val/1000, current_time_val%1000);
                xSemaphoreGive(print_sem);
            }
        }
    }
}

TaskHandle_t myTask6Handle = NULL;
void service_6(void *p)
{
    static uint32_t current_time_val = 0;
    unsigned long S6Cnt=0;

    // Print the service
    if(xSemaphoreTake(print_sem, portMAX_DELAY) == pdTRUE)
    {
        current_time_val = pdTICKS_TO_MS(xTaskGetTickCount());
        UARTprintf("service_6 start @ sec = %d, msec = %d!\r\n", current_time_val/1000, current_time_val%1000);
        xSemaphoreGive(print_sem);
    }

    while(!abortS6)
    {
        if(xSemaphoreTake(t6_sem, portMAX_DELAY) == pdTRUE)
        {
            // Increment counter variable
            S6Cnt++;
            // Print the current time
            if(xSemaphoreTake(print_sem, portMAX_DELAY) == pdTRUE)
            {
                current_time_val = pdTICKS_TO_MS(xTaskGetTickCount());
                UARTprintf("service_6 release @ sec = %d, msec = %d!\r\n", current_time_val/1000, current_time_val%1000);
                xSemaphoreGive(print_sem);
            }
        }
    }
}

TaskHandle_t myTask7Handle = NULL;
void service_7(void *p)
{
    static uint32_t current_time_val = 0;
    unsigned long S7Cnt=0;

    // Print the service
    if(xSemaphoreTake(print_sem, portMAX_DELAY) == pdTRUE)
    {
        current_time_val = pdTICKS_TO_MS(xTaskGetTickCount());
        UARTprintf("service_7 start @ sec = %d, msec = %d!\r\n", current_time_val/1000, current_time_val%1000);
        xSemaphoreGive(print_sem);
    }

    while(!abortS7)
    {
        if(xSemaphoreTake(t7_sem, portMAX_DELAY) == pdTRUE)
        {
            // Increment counter variable
            S7Cnt++;
            // Print the current time
            if(xSemaphoreTake(print_sem, portMAX_DELAY) == pdTRUE)
            {
                current_time_val = pdTICKS_TO_MS(xTaskGetTickCount());
                UARTprintf("service_7 release @ sec = %d, msec = %d!\r\n", current_time_val/1000, current_time_val%1000);
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

    // Configure semaphores
    t1_sem = xSemaphoreCreateBinary();
    t2_sem = xSemaphoreCreateBinary();
    t3_sem = xSemaphoreCreateBinary();
    t4_sem = xSemaphoreCreateBinary();
    t5_sem = xSemaphoreCreateBinary();
    t6_sem = xSemaphoreCreateBinary();
    t7_sem = xSemaphoreCreateBinary();
    print_sem = xSemaphoreCreateBinary();

    // Create tasks here
    // UARTprintf("Creating Tasks\r\n");
    xTaskCreate(
            service_1,                      // task function
            "s_1",                          // task name
            configMINIMAL_STACK_SIZE,   // task stack size
            (void*) 0,                      // variable for args
            configMAX_PRIORITIES-1,         // task priority
            &myTask1Handle                  // task handler variable
    );

    xTaskCreate(
            service_2,                      // task function
            "s_2",                          // task name
            configMINIMAL_STACK_SIZE,   // task stack size
            (void*) 0,                      // variable for args
            configMAX_PRIORITIES-2,         // task priority
            &myTask2Handle                  // task handler variable
    );

    xTaskCreate(
            service_3,                      // task function
            "s_3",                          // task name
            configMINIMAL_STACK_SIZE,   // task stack size
            (void*) 0,                      // variable for args
            configMAX_PRIORITIES-3,         // task priority
            &myTask3Handle                  // task handler variable
    );

    xTaskCreate(
            service_4,                      // task function
            "s_4",                          // task name
            configMINIMAL_STACK_SIZE,   // task stack size
            (void*) 0,                      // variable for args
            configMAX_PRIORITIES-2,         // task priority
            &myTask4Handle                  // task handler variable
    );

    xTaskCreate(
            service_5,                      // task function
            "s_5",                          // task name
            configMINIMAL_STACK_SIZE,   // task stack size
            (void*) 0,                      // variable for args
            configMAX_PRIORITIES-3,         // task priority
            &myTask5Handle                  // task handler variable
    );

    xTaskCreate(
            service_6,                      // task function
            "s_6",                          // task name
            configMINIMAL_STACK_SIZE,   // task stack size
            (void*) 0,                      // variable for args
            configMAX_PRIORITIES-2,         // task priority
            &myTask6Handle                  // task handler variable
    );

    xTaskCreate(
            service_7,                      // task function
            "s_7",                          // task name
            configMINIMAL_STACK_SIZE,   // task stack size
            (void*) 0,                      // variable for args
            configMAX_PRIORITIES-5,         // task priority
            &myTask7Handle                  // task handler variable
    );

    // start scheduler
    xSemaphoreGive(print_sem);
    vTaskStartScheduler();

    // we should never get here
    while(1)
    {
        UARTprintf("SHOULD NOT BE HERE\r\n");
    }
}

