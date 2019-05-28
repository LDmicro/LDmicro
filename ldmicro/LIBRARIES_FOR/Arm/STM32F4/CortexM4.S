/************************************************************************************
 * File:    startup.S
 * Purpose: Startup file for Cortex-M4 devices.
 *          Should use with GCC for ARM Embedded Processors
 * Version: V1.4
 * Date:    09 July 2012
 * Notice:  Changed for use with emIDE project wizard
 * Date:    05 July 2013
 *
 * Copyright (c) 2011, 2012, ARM Limited
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of the ARM Limited nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL ARM LIMITED BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ************************************************************************************/

    .syntax unified
    .arch armv7-m

/************************************************************************************
 *
 *  The minimal vector table for Cortex-M4.
 *  Device specific external interrupts can be added below.
 ************************************************************************************/
    .section .isr_vector
    .align 2
    .globl __isr_vector
__isr_vector:
    .long    __stack_end__         /* Top of Stack */
    .long    Reset_Handler         /* Reset Handler */
    .long    NMI_Handler           /* NMI Handler */
    .long    HardFault_Handler     /* Hard Fault Handler */
    .long    MemManage_Handler     /* MPU Fault Handler */
    .long    BusFault_Handler      /* Bus Fault Handler */
    .long    UsageFault_Handler    /* Usage Fault Handler */
    .long    0                     /* Reserved */
    .long    0                     /* Reserved */
    .long    0                     /* Reserved */
    .long    0                     /* Reserved */
    .long    SVC_Handler           /* SVCall Handler */
    .long    DebugMon_Handler      /* Debug Monitor Handler */
    .long    0                     /* Reserved */
    .long    PendSV_Handler        /* PendSV Handler */
    .long    SysTick_Handler       /* SysTick Handler */

    /* External interrupts */

/*
#ifdef WWDG_HANDLER
    .long    WWDG_Handler		        	// WATCHDOG=0
    #warning Using Interrupt 0
#else
    .long    Default_Handler
#endif
*/

    .weak    WWDG_Handler
    .long    WWDG_Handler		        	// WATCHDOG=0

    .weak    PVD_Handler
    .long    PVD_Handler		        	// PVD=1

    .weak    TAMP_STAMP_Handler
    .long    TAMP_STAMP_Handler	    	    // TAMPER=2

    .weak    RTC_WKUP_Handler
    .long    RTC_WKUP_Handler		        // RTC=3

    .weak    FLASH_Handler
    .long    FLASH_Handler		    	    // FLASH=4

    .weak    RCC_Handler
    .long    RCC_Handler			        // RCC=5

    .weak    EXTI0_Handler
    .long    EXTI0_Handler		       	    // EXTI=6

    .weak    EXTI1_Handler
    .long    EXTI1_Handler		    	    // EXTI=7

    .weak    EXTI2_Handler
    .long    EXTI2_Handler			        // EXTI=8

    .weak    EXTI3_Handler
    .long    EXTI3_Handler			        // EXTI=9

    .weak    EXTI4_Handler
    .long    EXTI4_Handler			        // EXTI=10

    .weak    DMA1_S0_Handler
    .long    DMA1_S0_Handler	            // DMA1=11

    .weak    DMA1_S1_Handler
    .long    DMA1_S1_Handler     	        // DMA1=12

    .weak    DMA1_S2_Handler
    .long    DMA1_S2_Handler     	        // DMA1=13

    .weak    DMA1_S3_Handler
    .long    DMA1_S3_Handler     	        // DMA1=14

    .weak    DMA1_S4_Handler
    .long    DMA1_S4_Handler     	        // DMA1=15

    .weak    DMA1_S5_Handler
    .long    DMA1_S5_Handler     	        // DMA1=16

    .weak    DMA1_S6_Handler
    .long    DMA1_S6_Handler    	        // DMA1= 17

    .weak    ADC_Handler
    .long    ADC_Handler		        	// ADC1-2-3=18

    .weak    CAN1_TX_Handler
    .long    CAN1_TX_Handler		        // CAN1=19

    .weak    CAN1_RX0_Handler
    .long    CAN1_RX0_Handler	        	// CAN1=20

    .weak    CAN1_RX1_Handler
    .long    CAN1_RX1_Handler	        	// CAN1=21

    .weak    CAN1_SCE_Handler
    .long    CAN1_SCE_Handler	        	// CAN1=22

    .weak    EXTI9_5_Handler
    .long    EXTI9_5_Handler	        	// EXTI=23

    .weak    TIM1_BRK_TIM9_Handler
    .long    TIM1_BRK_TIM9_Handler	        // TIM1=24

    .weak    TIM1_UP_TIM10_Handler
    .long    TIM1_UP_TIM10_Handler	        // TIM1=25

    .weak    TIM1_TRG_COM_TIM11_Handler
    .long    TIM1_TRG_COM_TIM11_Handler	    // TIM1=26

    .weak    TIM1_CC_Handler
    .long    TIM1_CC_Handler		        // TIM1=27

    .weak    TIM2_Handler
    .long    TIM2_Handler   		        // TIM2=28

    .weak    TIM3_Handler
    .long    TIM3_Handler			        // TIM3=29

    .weak    TIM4_Handler
    .long    TIM4_Handler			        // TIM4=30

    .weak    I2C1_EV_Handler
    .long    I2C1_EV_Handler	        	// I2C1=31

    .weak    I2C1_ER_Handler
    .long    I2C1_ER_Handler	        	// I2C1=32

    .weak    I2C2_EV_Handler
    .long    I2C2_EV_Handler	        	// I2C2=33

    .weak    I2C2_ER_Handler
    .long    I2C2_ER_Handler	        	// I2C2=34

    .weak    SPI1_Handler
    .long    SPI1_Handler		        	// SPI1=35

    .weak    SPI2_Handler
    .long    SPI2_Handler		        	// SPI2=36

    .weak    USART1_Handler
    .long    USART1_Handler	        	    // USART1=37

    .weak    USART2_Handler
    .long    USART2_Handler	                // USART2=38

    .weak    USART3_Handler
    .long    USART3_Handler	                // USART3=39

    .weak    EXTI15_10_Handler
    .long    EXTI15_10_Handler              // EXTI=40

    .weak    RTC_ALARM_Handler
    .long    RTC_ALARM_Handler	            // RTC=41

    .weak    OTG_FS_WKUP_Handler
    .long    OTG_FS_WKUP_Handler	        // USB=42

    .weak    TIM8_BRK_TIM12_Handler
    .long    TIM8_BRK_TIM12_Handler         // TIM8=43

    .weak    TIM8_UP_TIM13_Handler
    .long    TIM8_UP_TIM13_Handler	        // TIM8=44

    .weak    TIM8_TRG_COM_TIM14_Handler
    .long    TIM8_TRG_COM_TIM14_Handler	    // TIM8=45

    .weak    TIM8_CC_Handler
    .long    TIM8_CC_Handler		        // TIM8=46

    .weak    DMA1_S7_Handler
    .long    DMA1_S7_Handler	            // DMA1=47

    .weak    FSMC_Handler
    .long    FSMC_Handler			        // FSMC=48

    .weak    SDIO_Handler
    .long    SDIO_Handler			        // SDIO=49

    .weak    TIM5_Handler
    .long    TIM5_Handler			        // TIM5=50

    .weak    SPI3_Handler
    .long    SPI3_Handler			        // SPI3=51

    .weak    UART4_Handler
    .long    UART4_Handler			        // UART4=52

    .weak    UART5_Handler
    .long    UART5_Handler			        // UART5=53

    .weak    TIM6_DAC_Handler
    .long    TIM6_DAC_Handler		        // TIM6-DAC1=54

    .weak    TIM7_Handler
    .long    TIM7_Handler			        // TIM7=55

    .weak    DMA2_S0_Handler
    .long    DMA2_S0_Handler	            // DMA2=56

    .weak    DMA2_S1_Handler
    .long    DMA2_S1_Handler	            // DMA2=57

    .weak    DMA2_S2_Handler
    .long    DMA2_S2_Handler	            // DMA2=58

    .weak    DMA2_S3_Handler
    .long    DMA2_S3_Handler	            // DMA2=59

    .weak    DMA2_S4_Handler
    .long    DMA2_S4_Handler	            // DMA2=60

    .weak    ETH_Handler
    .long    ETH_Handler			        // Ethernet=61

    .weak    ETH_WKUP_Handler
    .long    ETH_WKUP_Handler		        // Ethernet=62

    .weak    CAN2_TX_Handler
    .long    CAN2_TX_Handler		        // CAN2=63

    .weak    CAN2_RX0_Handler
    .long    CAN2_RX0_Handler		        // CAN2=64

    .weak    CAN2_RX1_Handler
    .long    CAN2_RX1_Handler		        // CAN2=65

    .weak    CAN2_SCE_Handler
    .long    CAN2_SCE_Handler		        // CAN2=66

    .weak    OTG_FS_Handler
    .long    OTG_FS_Handler		            // USB=67

    .weak    DMA2_S5
    .long    DMA2_S5	                    // DMA2=68

    .weak    DMA2_S6
    .long    DMA2_S6	                    // DMA2=69

    .weak    DMA2_S7
    .long    DMA2_S7	                    // DMA2=70

    .weak    USART6_Handler
    .long    USART6_Handler		            // USART6=71

    .weak    I2C3_EV_Handler
    .long    I2C3_EV_Handler		        // I2C3=72

    .weak    I2C3_ER_Handler
    .long    I2C3_ER_Handler		        // I2C3=73

    .weak    OTG_HS_EP1_OUT_Handler
    .long    OTG_HS_EP1_OUT_Handler	        // USB=74

    .weak    OTG_HS_EP1_IN_Handler
    .long    OTG_HS_EP1_IN_Handler	        // USB=75

    .weak    OTG_HS_WKUP_Handler
    .long    OTG_HS_WKUP_Handler		    // USB=76

    .weak    OTG_HS_Handler
    .long    OTG_HS_Handler		            // USB=77

    .weak    DCMI_Handler
    .long    DCMI_Handler			        // DCMI=78

    .weak    CRYP_Handler
    .long    CRYP_Handler			        // CRYP=79

    .weak    HASH_RNG_Handler
    .long    HASH_RNG_Handler		        // Hash=80

    .weak    FPU_Handler
    .long    FPU_Handler			        // FPU=81


    .size    __isr_vector, . - __isr_vector

/************************************************************************************
 *
 *  Reset_Handler()
 *  This function gets called at start of execution after a reset event.
 *  Copies data from ROM to RAM, clears BSS if defined,
 *  calls SystemInit() if defined, finally calls main()
 ************************************************************************************/
    .text
    .thumb
    .global __stack_end__
    .thumb_func
    .align 2
    .globl    Reset_Handler
    .type    Reset_Handler, %function
Reset_Handler:

/*  Setup stack pointer. Helpful for targets running in RAM without script file */
    ldr   r1, =__stack_end__
    msr   msp, r1

/*  Loop to copy data from read only memory to RAM. The ranges
 *   of copy from/to are specified by following symbols evaluated in
 *   linker script.
 *   __etext: End of code section, i.e., begin of data sections to copy from.
 *   __data_start__/__data_end__: RAM address range that data should be
 *   copied to. Both must be aligned to 4 bytes boundary.  */

    ldr    r1, =__etext
    ldr    r2, =__data_start__
    ldr    r3, =__data_end__

#if 1
/*  Here are two copies of loop implemenations. First one favors code size
 *  and the second one favors performance. Default uses the first one.
 *  Change to "#if 0" to use the second one */
.LC0:
    cmp     r2, r3
    ittt    lt
    ldrlt   r0, [r1], #4
    strlt   r0, [r2], #4
    blt    .LC0
#else
    subs    r3, r2
    ble    .LC1
.LC0:
    subs    r3, #4
    ldr    r0, [r1, r3]
    str    r0, [r2, r3]
    bgt    .LC0
.LC1:
#endif

/*  Loop to zero out BSS section, which uses following symbols
 *  in linker script:
 *   __bss_start__: start of BSS section. Must align to 4
 *   __bss_end__: end of BSS section. Must align to 4
 */
    ldr r1, =__bss_start__
    ldr r2, =__bss_end__

    movs    r0, 0
.LC2:
    cmp     r1, r2
    itt    lt
    strlt   r0, [r1], #4
    blt    .LC2

#ifdef __SYSTEM_INIT
    bl    SystemInit
#endif

    bl    main
    .pool
    .size Reset_Handler, . - Reset_Handler

/************************************************************************************
 *
 *  Weak definition for exceptions.
 *  Any function with the same name will override the weak definition.
 ************************************************************************************/
/*  Macro to define default handlers. Default handler
 *  will be weak symbol and just dead loops. They can be
 *  overwritten by other handlers */
    .macro    def_default_handler    handler_name
    .align 1
    .thumb_func
    .weak    \handler_name
    .type    \handler_name, %function
\handler_name :
    b    .
    .size    \handler_name, . - \handler_name
    .endm

    def_default_handler    NMI_Handler
    def_default_handler    HardFault_Handler
    def_default_handler    MemManage_Handler
    def_default_handler    BusFault_Handler
    def_default_handler    UsageFault_Handler
    def_default_handler    SVC_Handler
    def_default_handler    DebugMon_Handler
    def_default_handler    PendSV_Handler
    def_default_handler    SysTick_Handler
    def_default_handler    Default_Handler

    .weak    DEF_IRQHandler
    .set    DEF_IRQHandler, Default_Handler

    .end
