/*
 * FreeRTOS V1.4.7
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://aws.amazon.com/freertos
 * http://www.FreeRTOS.org
 */


/*
 * Debug setup instructions:
 * 1) Open the debug configuration dialog.
 * 2) Go to the Debugger tab.
 * 3) If the 'Mode Setup' options are not visible, click the 'Show Generator' button.
 * 4) In the Mode Setup|Reset Mode drop down ensure that
 *    'Software System Reset' is selected.
 */

#include "main.h"
#include "stdint.h"
#include "stdarg.h"

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"

/* Demo includes */
#include "aws_demo.h"
#include "iot_system_init.h"
#include "iot_logging_task.h"
#include "aws_clientcredential.h"
#include "aws_dev_mode_key_provisioning.h"

/* Application version info. */
#include "aws_application_version.h"

/* Declare the firmware version structure for all to see. */
const AppVersion32_t xAppFirmwareVersion =
{
    .u.x.ucMajor = APP_VERSION_MAJOR,
    .u.x.ucMinor = APP_VERSION_MINOR,
    .u.x.usBuild = APP_VERSION_BUILD,
};

/* The SPI driver polls at a high priority. The logging task's priority must also
 * be high to be not be starved of CPU time. */
#define mainLOGGING_TASK_PRIORITY                         ( configMAX_PRIORITIES - 1 )
#define mainLOGGING_TASK_STACK_SIZE                       ( configMINIMAL_STACK_SIZE * 5 )
#define mainLOGGING_MESSAGE_QUEUE_LENGTH                  ( 15 )

/*-----------------------------------------------------------*/

void vApplicationDaemonTaskStartupHook( void );

/**********************
* Global Variables
**********************/
RTC_HandleTypeDef xHrtc;
RNG_HandleTypeDef xHrng;
UART_HandleTypeDef xConsoleUart;

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static void SystemClock_Config( void );
static void Console_UART_Init( void );
static void RTC_Init( void );

/**
 * @brief Initializes the STM32L475 IoT node board.
 *
 * Initialization of clock, LEDs, RNG, RTC, and WIFI module.
 */
static void prvMiscInitialization( void );

/**
 * @brief Initializes the FreeRTOS heap.
 *
 * Heap_5 is being used because the RAM is not contiguous, therefore the heap
 * needs to be initialized.  See http://www.freertos.org/a00111.html
 */
static void prvInitializeHeap( void );


static void prvWaterMeterTask( void * pArgument );

/**
 * @brief Application runtime entry point.
 */
int main( void )
{
    /* Perform any hardware initialization that does not require the RTOS to be
     * running.  */
    prvMiscInitialization();

    /* Create tasks that are not dependent on the WiFi being initialized. */
    xLoggingTaskInitialize( mainLOGGING_TASK_STACK_SIZE,
                            mainLOGGING_TASK_PRIORITY,
                            mainLOGGING_MESSAGE_QUEUE_LENGTH );

    /* Start the scheduler.  Initialization that requires the OS to be running,
     * including the BG96 initialization, is performed in the RTOS daemon task
     * startup hook. */
    vTaskStartScheduler();

    return 0;
}
/*-----------------------------------------------------------*/

void vApplicationDaemonTaskStartupHook( void )
{

    /* A simple example to demonstrate key and certificate provisioning in
     * micro-controller flash using PKCS#11 interface. This should be replaced
     * by production ready key provisioning mechanism. */
    vDevModeKeyProvisioning();

    if( SYSTEM_Init() == pdPASS )
    {
    	/* Static initialization of the BG96 modem */
    	BG96_Modem_Init();

    	/* Starts the BG96 modem tasks before running the demos */
        BG96_Modem_Start();

        /* Start demos. */
    	DEMO_RUNNER_RunDemos();

		xTaskCreate(prvWaterMeterTask, "WATER METER", configMINIMAL_STACK_SIZE*8, NULL, tskIDLE_PRIORITY+1, NULL);
    }

}

/*-----------------------------------------------------------*/

/* configUSE_STATIC_ALLOCATION is set to 1, so the application must provide an
 * implementation of vApplicationGetIdleTaskMemory() to provide the memory that is
 * used by the Idle task. */
void vApplicationGetIdleTaskMemory( StaticTask_t ** ppxIdleTaskTCBBuffer,
                                    StackType_t ** ppxIdleTaskStackBuffer,
                                    uint32_t * pulIdleTaskStackSize )
{
/* If the buffers to be provided to the Idle task are declared inside this
 * function then they must be declared static - otherwise they will be allocated on
 * the stack and so not exists after this function exits. */
    static StaticTask_t xIdleTaskTCB;
    static StackType_t uxIdleTaskStack[ configMINIMAL_STACK_SIZE ];

    /* Pass out a pointer to the StaticTask_t structure in which the Idle
     * task's state will be stored. */
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

    /* Pass out the array that will be used as the Idle task's stack. */
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;

    /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
     * Note that, as the array is necessarily of type StackType_t,
     * configMINIMAL_STACK_SIZE is specified in words, not bytes. */
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}
/*-----------------------------------------------------------*/

/* configUSE_STATIC_ALLOCATION is set to 1, so the application must provide an
 * implementation of vApplicationGetTimerTaskMemory() to provide the memory that is
 * used by the RTOS daemon/time task. */
void vApplicationGetTimerTaskMemory( StaticTask_t ** ppxTimerTaskTCBBuffer,
                                     StackType_t ** ppxTimerTaskStackBuffer,
                                     uint32_t * pulTimerTaskStackSize )
{
/* If the buffers to be provided to the Timer task are declared inside this
 * function then they must be declared static - otherwise they will be allocated on
 * the stack and so not exists after this function exits. */
    static StaticTask_t xTimerTaskTCB;
    static StackType_t uxTimerTaskStack[ configTIMER_TASK_STACK_DEPTH ];

    /* Pass out a pointer to the StaticTask_t structure in which the Idle
     * task's state will be stored. */
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;

    /* Pass out the array that will be used as the Timer task's stack. */
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;

    /* Pass out the size of the array pointed to by *ppxTimerTaskStackBuffer.
     * Note that, as the array is necessarily of type StackType_t,
     * configMINIMAL_STACK_SIZE is specified in words, not bytes. */
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}
/*-----------------------------------------------------------*/

/**
 * @brief Publishes a character to the STM32L475 UART
 *
 * This is used to implement the tinyprintf created by Spare Time Labs
 * http://www.sparetimelabs.com/tinyprintf/tinyprintf.php
 *
 * @param pv    unused void pointer for compliance with tinyprintf
 * @param ch    character to be printed
 */
void vSTM32L475putc( void * pv,
                     char ch )
{
    while( HAL_OK != HAL_UART_Transmit( &xConsoleUart, ( uint8_t * ) &ch, 1, 30000 ) )
    {
    }
}
/*-----------------------------------------------------------*/

/**
 * @brief Initializes the board.
 */
static void prvMiscInitialization( void )
{
    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    /* Configure the system clock. */
    SystemClock_Config();

    /* Heap_5 is being used because the RAM is not contiguous in memory, so the
     * heap must be initialized. */
    prvInitializeHeap();

    BSP_LED_Init( LED_GREEN );
    BSP_JOY_Init( JOY_MODE_EXTI );

    /* RNG init function. */
    xHrng.Instance = RNG;

    if( HAL_RNG_Init( &xHrng ) != HAL_OK )
    {
        Error_Handler();
    }

    /* RTC init. */
    RTC_Init();

    /* UART console init. */
    Console_UART_Init();

    /* BG96 Modem Hardware Initialization */
    BG96_HAL_Init();
}
/*-----------------------------------------------------------*/

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    /** Initializes the CPU, AHB and APB busses clocks
    */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_MSI;
    RCC_OscInitStruct.LSIState = RCC_LSI_ON;
    RCC_OscInitStruct.MSIState = RCC_MSI_ON;
    RCC_OscInitStruct.MSICalibrationValue = 0;
    RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
    RCC_OscInitStruct.PLL.PLLM = 1;
    RCC_OscInitStruct.PLL.PLLN = 40;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
    RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }
    /** Initializes the CPU, AHB and APB busses clocks
    */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
    {
        Error_Handler();
    }
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC|RCC_PERIPHCLK_USART1
                                |RCC_PERIPHCLK_USART2|RCC_PERIPHCLK_LPUART1
                                |RCC_PERIPHCLK_I2C1|RCC_PERIPHCLK_RNG;

    PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
    PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
    PeriphClkInit.Lpuart1ClockSelection = RCC_LPUART1CLKSOURCE_PCLK1;
    PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_PCLK1;
    PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
    PeriphClkInit.RngClockSelection = RCC_RNGCLKSOURCE_PLLSAI1;
    PeriphClkInit.PLLSAI1.PLLSAI1Source = RCC_PLLSOURCE_MSI;
    PeriphClkInit.PLLSAI1.PLLSAI1M = 1;
    PeriphClkInit.PLLSAI1.PLLSAI1N = 20;
    PeriphClkInit.PLLSAI1.PLLSAI1P = RCC_PLLP_DIV2;
    PeriphClkInit.PLLSAI1.PLLSAI1Q = RCC_PLLQ_DIV2;
    PeriphClkInit.PLLSAI1.PLLSAI1R = RCC_PLLR_DIV2;
    PeriphClkInit.PLLSAI1.PLLSAI1ClockOut = RCC_PLLSAI1_48M2CLK;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
        Error_Handler();
    }
    /** Configure the main internal regulator output voltage
    */
    if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
    {
        Error_Handler();
    }

    /**Configure the Systick interrupt time
    */
    HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

    /**Configure the Systick
    */
    HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

    /* SysTick_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(SysTick_IRQn, 15, 0);
}
/*-----------------------------------------------------------*/

/**
 * @brief UART console initialization function.
 */
static void Console_UART_Init( void )
{
    xConsoleUart.Instance = USART2;
    xConsoleUart.Init.BaudRate = 115200;
    xConsoleUart.Init.WordLength = UART_WORDLENGTH_8B;
    xConsoleUart.Init.StopBits = UART_STOPBITS_1;
    xConsoleUart.Init.Parity = UART_PARITY_NONE;
    xConsoleUart.Init.Mode = UART_MODE_TX_RX;
    xConsoleUart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    xConsoleUart.Init.OverSampling = UART_OVERSAMPLING_16;
    xConsoleUart.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    xConsoleUart.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    BSP_COM_Init( COM1, &xConsoleUart );
}
/*-----------------------------------------------------------*/

/**
 * @brief RTC init function.
 */
static void RTC_Init( void )
{
    RTC_TimeTypeDef xsTime;
    RTC_DateTypeDef xsDate;

    /* Initialize RTC Only. */
    xHrtc.Instance = RTC;
    xHrtc.Init.HourFormat = RTC_HOURFORMAT_24;
    xHrtc.Init.AsynchPrediv = 127;
    xHrtc.Init.SynchPrediv = 255;
    xHrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
    xHrtc.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
    xHrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    xHrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;

    if( HAL_RTC_Init( &xHrtc ) != HAL_OK )
    {
        Error_Handler();
    }

    /* Initialize RTC and set the Time and Date. */
    xsTime.Hours = 0x12;
    xsTime.Minutes = 0x0;
    xsTime.Seconds = 0x0;
    xsTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    xsTime.StoreOperation = RTC_STOREOPERATION_RESET;

    if( HAL_RTC_SetTime( &xHrtc, &xsTime, RTC_FORMAT_BCD ) != HAL_OK )
    {
        Error_Handler();
    }

    xsDate.WeekDay = RTC_WEEKDAY_FRIDAY;
    xsDate.Month = RTC_MONTH_JANUARY;
    xsDate.Date = 0x24;
    xsDate.Year = 0x17;

    if( HAL_RTC_SetDate( &xHrtc, &xsDate, RTC_FORMAT_BCD ) != HAL_OK )
    {
        Error_Handler();
    }
}
/*-----------------------------------------------------------*/

/**
 * @brief  This function is executed in case of error occurrence.
 */
void Error_Handler( void )
{
    while( 1 )
    {
        BSP_LED_Toggle( LED_GREEN );
        HAL_Delay( 200 );
    }
}
/*-----------------------------------------------------------*/

/**
 * @brief Warn user if pvPortMalloc fails.
 *
 * Called if a call to pvPortMalloc() fails because there is insufficient
 * free memory available in the FreeRTOS heap.  pvPortMalloc() is called
 * internally by FreeRTOS API functions that create tasks, queues, software
 * timers, and semaphores.  The size of the FreeRTOS heap is set by the
 * configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h.
 *
 */
void vApplicationMallocFailedHook()
{
    taskDISABLE_INTERRUPTS();

    for( ; ; )
    {
    }
}
/*-----------------------------------------------------------*/

/**
 * @brief Loop forever if stack overflow is detected.
 *
 * If configCHECK_FOR_STACK_OVERFLOW is set to 1,
 * this hook provides a location for applications to
 * define a response to a stack overflow.
 *
 * Use this hook to help identify that a stack overflow
 * has occurred.
 *
 */
void vApplicationStackOverflowHook( TaskHandle_t xTask,
                                    char * pcTaskName )
{
    portDISABLE_INTERRUPTS();

    /* Loop forever */
    for( ; ; )
    {
    }
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook( void )
{
}
/*-----------------------------------------------------------*/

void * malloc( size_t xSize )
{
    configASSERT( xSize == ~0 );

    return NULL;
}
/*-----------------------------------------------------------*/


void vOutputChar( const char cChar,
                  const TickType_t xTicksToWait )
{
    ( void ) cChar;
    ( void ) xTicksToWait;
}
/*-----------------------------------------------------------*/

void vMainUARTPrintString( char * pcString )
{
    const uint32_t ulTimeout = 3000UL;

    HAL_UART_Transmit( &xConsoleUart,
                       ( uint8_t * ) pcString,
                       strlen( pcString ),
                       ulTimeout );
}
/*-----------------------------------------------------------*/

void prvGetRegistersFromStack( uint32_t * pulFaultStackAddress )
{
/* These are volatile to try and prevent the compiler/linker optimising them
 * away as the variables never actually get used.  If the debugger won't show the
 * values of the variables, make them global my moving their declaration outside
 * of this function. */
    volatile uint32_t r0;
    volatile uint32_t r1;
    volatile uint32_t r2;
    volatile uint32_t r3;
    volatile uint32_t r12;
    volatile uint32_t lr;  /* Link register. */
    volatile uint32_t pc;  /* Program counter. */
    volatile uint32_t psr; /* Program status register. */

    r0 = pulFaultStackAddress[ 0 ];
    r1 = pulFaultStackAddress[ 1 ];
    r2 = pulFaultStackAddress[ 2 ];
    r3 = pulFaultStackAddress[ 3 ];

    r12 = pulFaultStackAddress[ 4 ];
    lr = pulFaultStackAddress[ 5 ];
    pc = pulFaultStackAddress[ 6 ];
    psr = pulFaultStackAddress[ 7 ];

    /* Remove compiler warnings about the variables not being used. */
    ( void ) r0;
    ( void ) r1;
    ( void ) r2;
    ( void ) r3;
    ( void ) r12;
    ( void ) lr;  /* Link register. */
    ( void ) pc;  /* Program counter. */
    ( void ) psr; /* Program status register. */

    /* When the following line is hit, the variables contain the register values. */
    for( ; ; )
    {
    }
}
/*-----------------------------------------------------------*/

/* The fault handler implementation calls a function called
 * prvGetRegistersFromStack(). */
void HardFault_Handler( void )
{
    __asm volatile
    (
        " tst lr, #4                                                \n"
        " ite eq                                                    \n"
        " mrseq r0, msp                                             \n"
        " mrsne r0, psp                                             \n"
        " ldr r1, [r0, #24]                                         \n"
        " ldr r2, handler2_address_const                            \n"
        " bx r2                                                     \n"
        " handler2_address_const: .word prvGetRegistersFromStack    \n"
    );
}
/*-----------------------------------------------------------*/

/* Psuedo random number generator.  Just used by demos so does not need to be
 * secure.  Do not use the standard C library rand() function as it can cause
 * unexpected behaviour, such as calls to malloc(). */
int iMainRand32( void )
{
    static UBaseType_t uxlNextRand; /*_RB_ Not seeded. */
    const uint32_t ulMultiplier = 0x015a4e35UL, ulIncrement = 1UL;

    /* Utility function to generate a pseudo random number. */

    uxlNextRand = ( ulMultiplier * uxlNextRand ) + ulIncrement;

    return( ( int ) ( uxlNextRand >> 16UL ) & 0x7fffUL );
}
/*-----------------------------------------------------------*/

static void prvInitializeHeap( void )
{
    static uint8_t ucHeap1[ configTOTAL_HEAP_SIZE ];
    static uint8_t ucHeap2[ 100 * 1024 ] __attribute__( ( section( ".freertos_heap2" ) ) );

    HeapRegion_t xHeapRegions[] =
    {
        { ( unsigned char * ) ucHeap2, sizeof( ucHeap2 ) },
        { ( unsigned char * ) ucHeap1, sizeof( ucHeap1 ) },
        { NULL,                        0                 }
    };

    vPortDefineHeapRegions( xHeapRegions );
}
/*-----------------------------------------------------------*/

/*-----------------------------------------------------------*/

/**
 * @brief  EXTI line detection callback.
 *
 * @param  GPIO_Pin: Specifies the port pin connected to corresponding EXTI line.
 */
void HAL_GPIO_EXTI_Callback( uint16_t GPIO_Pin )
{
    switch( GPIO_Pin )
    {
        default:
            break;
    }
}
/*-----------------------------------------------------------*/

/**
 * @brief SPI Interrupt Handler.
 *
 * @note Inventek module is configured to use SPI3.
 */
void SPI3_IRQHandler( void )
{
}
/*-----------------------------------------------------------*/

/**
 * @brief Period elapsed callback in non blocking mode
 *
 * @note This function is called  when TIM1 interrupt took place, inside
 * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
 * a global variable "uwTick" used as application time base.
 *
 * @param  htim : TIM handle
 * @retval None
 */
void HAL_TIM_PeriodElapsedCallback( TIM_HandleTypeDef * htim )
{
    if( htim->Instance == TIM6 )
    {
        HAL_IncTick();
    }
}
/*-----------------------------------------------------------*/

#define BIT_DELAY (828)

void sw_uart_putc(uint8_t b) {
    GPIOG->BRR = (uint32_t) GPIO_PIN_7;
    GPIOG->BSRR = (uint32_t) GPIO_PIN_8;
    for (int d=0; d<BIT_DELAY; d++) asm("nop");

    for (int i=0; i<8; i++) {
        if (b & 0x01) {
            GPIOG->BSRR = (uint32_t) GPIO_PIN_7;
            GPIOG->BRR = (uint32_t) GPIO_PIN_8;
        } else {
            GPIOG->BRR = (uint32_t) GPIO_PIN_7;
            GPIOG->BSRR = (uint32_t) GPIO_PIN_8;
        }
        for (int d=0; d<BIT_DELAY; d++) asm("nop");
        b >>= 1;
    }

    GPIOG->BSRR = (uint32_t) GPIO_PIN_7;
    GPIOG->BRR = (uint32_t) GPIO_PIN_8;
    for (int d=0; d<BIT_DELAY; d++) asm("nop");
}

void sw_uart_send(uint8_t* buf, uint32_t length) {
    for (int i=0; i<length; i++) sw_uart_putc(buf[i]);
}

uint8_t sw_uart_getc() {
    volatile uint8_t c = 0;
    volatile uint16_t b[10];
    memset(b, 0, 20);

    while((GPIOG->IDR & GPIO_PIN_7) != 0); // wait start bit

    for (int i=0; i<310; i++) b[0] += ((GPIOG->IDR & GPIO_PIN_7)) == 0 ? 0 : 1;
    for (int i=0; i<324; i++) b[1] += ((GPIOG->IDR & GPIO_PIN_7)) == 0 ? 0 : 1;
    for (int i=0; i<324; i++) b[2] += ((GPIOG->IDR & GPIO_PIN_7)) == 0 ? 0 : 1;
    for (int i=0; i<324; i++) b[3] += ((GPIOG->IDR & GPIO_PIN_7)) == 0 ? 0 : 1;
    for (int i=0; i<324; i++) b[4] += ((GPIOG->IDR & GPIO_PIN_7)) == 0 ? 0 : 1;
    for (int i=0; i<324; i++) b[5] += ((GPIOG->IDR & GPIO_PIN_7)) == 0 ? 0 : 1;
    for (int i=0; i<324; i++) b[6] += ((GPIOG->IDR & GPIO_PIN_7)) == 0 ? 0 : 1;
    for (int i=0; i<324; i++) b[7] += ((GPIOG->IDR & GPIO_PIN_7)) == 0 ? 0 : 1;
    for (int i=0; i<324; i++) b[8] += ((GPIOG->IDR & GPIO_PIN_7)) == 0 ? 0 : 1;

    c = 0;
    for (int i=0; i<8; i++) {
        if (b[i+1] > 160) c |= 1<<i;
    }

    while((GPIOG->IDR & GPIO_PIN_7) == 0); // wait stop bit

    return c;
}

int sw_uart_recv(uint8_t *buf, uint32_t len)
{
    volatile uint32_t m = GPIOG->MODER;
    GPIOG->MODER &= ~(GPIO_MODER_MODER7 | GPIO_MODER_MODER8);

    for (int i=0; i<len; i++)
    {
        buf[i] = sw_uart_getc();
    }

    GPIOG->MODER = m;

    return len;
}

#define CRC_LENGTH (2)
#define RESPONSE_LENGTH (45)

static const int TABLE[] = {
    0x0000, 0xc0c1, 0xc181, 0x0140, 0xc301, 0x03c0, 0x0280, 0xc241,
    0xc601, 0x06c0, 0x0780, 0xc741, 0x0500, 0xc5c1, 0xc481, 0x0440,
    0xcc01, 0x0cc0, 0x0d80, 0xcd41, 0x0f00, 0xcfc1, 0xce81, 0x0e40,
    0x0a00, 0xcac1, 0xcb81, 0x0b40, 0xc901, 0x09c0, 0x0880, 0xc841,
    0xd801, 0x18c0, 0x1980, 0xd941, 0x1b00, 0xdbc1, 0xda81, 0x1a40,
    0x1e00, 0xdec1, 0xdf81, 0x1f40, 0xdd01, 0x1dc0, 0x1c80, 0xdc41,
    0x1400, 0xd4c1, 0xd581, 0x1540, 0xd701, 0x17c0, 0x1680, 0xd641,
    0xd201, 0x12c0, 0x1380, 0xd341, 0x1100, 0xd1c1, 0xd081, 0x1040,
    0xf001, 0x30c0, 0x3180, 0xf141, 0x3300, 0xf3c1, 0xf281, 0x3240,
    0x3600, 0xf6c1, 0xf781, 0x3740, 0xf501, 0x35c0, 0x3480, 0xf441,
    0x3c00, 0xfcc1, 0xfd81, 0x3d40, 0xff01, 0x3fc0, 0x3e80, 0xfe41,
    0xfa01, 0x3ac0, 0x3b80, 0xfb41, 0x3900, 0xf9c1, 0xf881, 0x3840,
    0x2800, 0xe8c1, 0xe981, 0x2940, 0xeb01, 0x2bc0, 0x2a80, 0xea41,
    0xee01, 0x2ec0, 0x2f80, 0xef41, 0x2d00, 0xedc1, 0xec81, 0x2c40,
    0xe401, 0x24c0, 0x2580, 0xe541, 0x2700, 0xe7c1, 0xe681, 0x2640,
    0x2200, 0xe2c1, 0xe381, 0x2340, 0xe101, 0x21c0, 0x2080, 0xe041,
    0xa001, 0x60c0, 0x6180, 0xa141, 0x6300, 0xa3c1, 0xa281, 0x6240,
    0x6600, 0xa6c1, 0xa781, 0x6740, 0xa501, 0x65c0, 0x6480, 0xa441,
    0x6c00, 0xacc1, 0xad81, 0x6d40, 0xaf01, 0x6fc0, 0x6e80, 0xae41,
    0xaa01, 0x6ac0, 0x6b80, 0xab41, 0x6900, 0xa9c1, 0xa881, 0x6840,
    0x7800, 0xb8c1, 0xb981, 0x7940, 0xbb01, 0x7bc0, 0x7a80, 0xba41,
    0xbe01, 0x7ec0, 0x7f80, 0xbf41, 0x7d00, 0xbdc1, 0xbc81, 0x7c40,
    0xb401, 0x74c0, 0x7580, 0xb541, 0x7700, 0xb7c1, 0xb681, 0x7640,
    0x7200, 0xb2c1, 0xb381, 0x7340, 0xb101, 0x71c0, 0x7080, 0xb041,
    0x5000, 0x90c1, 0x9181, 0x5140, 0x9301, 0x53c0, 0x5280, 0x9241,
    0x9601, 0x56c0, 0x5780, 0x9741, 0x5500, 0x95c1, 0x9481, 0x5440,
    0x9c01, 0x5cc0, 0x5d80, 0x9d41, 0x5f00, 0x9fc1, 0x9e81, 0x5e40,
    0x5a00, 0x9ac1, 0x9b81, 0x5b40, 0x9901, 0x59c0, 0x5880, 0x9841,
    0x8801, 0x48c0, 0x4980, 0x8941, 0x4b00, 0x8bc1, 0x8a81, 0x4a40,
    0x4e00, 0x8ec1, 0x8f81, 0x4f40, 0x8d01, 0x4dc0, 0x4c80, 0x8c41,
    0x4400, 0x84c1, 0x8581, 0x4540, 0x8701, 0x47c0, 0x4680, 0x8641,
    0x8201, 0x42c0, 0x4380, 0x8341, 0x4100, 0x81c1, 0x8081, 0x4040
};

void updateCrc(uint8_t* bytes, int offset, int length, int crcIndex)
{
    if ((NULL == bytes) || (length == 0) || (crcIndex < 0)) return;
    int checksum = 0xFFFF;
    for (int i = offset; i < offset + length; i++) {
        checksum = (checksum >> 8) ^ TABLE[((checksum) ^ ((int) bytes[i] & 0xff)) & 0xff];
    }
    long value = checksum;
    bytes[crcIndex] = (uint8_t) ((value & 0x000000ff));
    bytes[crcIndex + 1] = (uint8_t) ((value & 0x0000ff00) >> 8);
}

void compose(uint8_t* request, size_t request_len, uint8_t stationNo)
{
    request[0] = stationNo;
    request[1] = 0x03;
    request[2] = 0x05;
    request[3] = 0x04;
    request[4] = 0x00;
    request[5] = 0x14;
    request[6] = 0x00;
    request[7] = 0x00;
    updateCrc(request, 0, request_len - CRC_LENGTH, request_len - CRC_LENGTH);
}

uint8_t water_meter_station_id = 0;
uint64_t water_meter_meter_id = 0;
uint64_t water_meter_amount = 0;
uint8_t water_meter_amount_place = 0;
uint64_t water_meter_reverse_amount = 0;
uint8_t water_meter_reverse_amount_place = 0;
uint16_t water_meter_lday = 0;
uint16_t water_meter_nday = 0;
uint16_t water_meter_oday = 0;
uint16_t water_meter_uday = 0;
uint16_t water_meter_hday = 0;
uint16_t water_meter_bday = 0;
uint16_t water_meter_op = 0;

uint8_t water_meter_rawdata[45] = {0};

#include <semphr.h>

SemaphoreHandle_t sema_water_meter = NULL;

void water_meter_read()
{
    xSemaphoreGive(sema_water_meter);
    vTaskDelay(300);
}

static void prvWaterMeterTask( void * pArgument )
{
#if 0
req:
    01 03 05 04 00 14 04 C8
rsp:
    00 01 03 28 F0 01 05 00
    00 00 00 01 06 00 FF E0
    00 00 00 00 00 00 00 68
    08 FF F8 59 47 4D 34 00
    FE 00 FE 00 FF 00 FF 00
    00 00 00 00 00
#endif
    uint8_t req[8] = { 0x01, 0x03, 0x05, 0x04, 0x00, 0x14, 0x04, 0xC8 };
    uint8_t rsp[45];

    HAL_GPIO_WritePin(GPIOG, GPIO_PIN_7, GPIO_PIN_SET);

    vSemaphoreCreateBinary( sema_water_meter );


    vTaskDelay(100);
    while(1) {
        xSemaphoreTake(sema_water_meter, portMAX_DELAY);

        compose(req, sizeof(req), 1);

        if (0) {
            configPRINTF(("req: %02X %02X %02X %02X %02X %02X %02X %02X\r\n", req[0], req[1], req[2], req[3], req[4], req[5], req[6], req[7]));
        }

        taskENTER_CRITICAL();
        sw_uart_send(req, 8);
        sw_uart_recv(rsp, 45);
        taskEXIT_CRITICAL();

        if (0)
        {
            for (int i=0; i<45; i++) {
                configPRINTF(("rsp[%d]: %02X\r\n", i, rsp[i]));
            }
        }
        memcpy(water_meter_rawdata, rsp, 45);

        water_meter_station_id = rsp[0];

        water_meter_amount = rsp[8];
        water_meter_amount = (water_meter_amount << 8) | (rsp[7]);
        water_meter_amount = (water_meter_amount << 8) | (rsp[6]);
        water_meter_amount = (water_meter_amount << 8) | (rsp[5]);
        water_meter_amount = (water_meter_amount << 8) | (rsp[4]);
        water_meter_amount = (water_meter_amount << 8) | (rsp[3]);

        water_meter_amount_place = rsp[10];

        water_meter_reverse_amount = rsp[16];
        water_meter_reverse_amount = (water_meter_reverse_amount << 8) | (rsp[15]);
        water_meter_reverse_amount = (water_meter_reverse_amount << 8) | (rsp[14]);
        water_meter_reverse_amount = (water_meter_reverse_amount << 8) | (rsp[13]);
        water_meter_reverse_amount = (water_meter_reverse_amount << 8) | (rsp[12]);
        water_meter_reverse_amount = (water_meter_reverse_amount << 8) | (rsp[11]);

        water_meter_reverse_amount_place = rsp[18];
        water_meter_lday = (rsp[20] << 8) | (rsp[19]);
        water_meter_nday = (rsp[22] << 8) | (rsp[21]);
        water_meter_oday = (rsp[24] << 8) | (rsp[23]);
        water_meter_uday = (rsp[26] << 8) | (rsp[25]);
        water_meter_hday = (rsp[28] << 8) | (rsp[27]);
        water_meter_bday = (rsp[30] << 8) | (rsp[29]);
        water_meter_op = (rsp[33] << 8) | (rsp[31]);

        water_meter_meter_id = rsp[42];
        water_meter_meter_id = (water_meter_meter_id << 8) | (rsp[41]);
        water_meter_meter_id = (water_meter_meter_id << 8) | (rsp[40]);
        water_meter_meter_id = (water_meter_meter_id << 8) | (rsp[39]);
        water_meter_meter_id = (water_meter_meter_id << 8) | (rsp[38]);
        water_meter_meter_id = (water_meter_meter_id << 8) | (rsp[37]);
    }

    while(1) vTaskDelay(1000);
}
