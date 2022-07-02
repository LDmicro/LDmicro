/**
  ******************************************************************************
  * @file    Project/stm32f4xx_StdPeriph_Template/system_stm32f4xx.c
  * @author  MCD Application Team
  * @version V0.0.3
  * @date    10/15/2010
  * @brief   CMSIS Cortex-M3 Device Peripheral Access Layer System Source File.
  ******************************************************************************
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2010 STMicroelectronics</center></h2>
  ******************************************************************************
  */

#include "stm32f4xx.h"

/** @addtogroup stm32f4xx_System_Private_Defines
  * @{
  */

/*!< Uncomment the line corresponding to the desired System clock (SYSCLK)
   frequency (after reset the HSI is used as SYSCLK source)

   IMPORTANT NOTE:
   ==============
   1. After each device reset the HSI is used as System clock source.

   2. Please make sure that the selected System clock doesn't exceed your device's
      maximum frequency.

    */

/* PLLVCO = (HSE_VALUE / PLL_M) * PLL_N */
//#define PLL_M (HSE_VALUE / 1000000) /* Possible value 0 and 63 */
uint32_t PLL_M = 8; 

/// #define PLL_N   336                     /* Possible value 192 and 432 */
//#define PLL_N 200 /* Possible value 192 and 432 */
uint32_t PLL_N = 64;

/* SYSCLK = PLLVCO / PLL_P !!!! DO NOT EXCEED 120MHz */
#define PLL_P 2 /* Possible value 2, 4, 6, or 8 */

/* OTGFS, SDIO and RNG Clock =  PLLVCO / PLL_Q */
/// #define PLL_Q   7  /* Possible value between 4 and 15 */
#define PLL_Q 4 /* Possible value between 4 and 15 */

/* I2SCLK =  PLLVCO / PLL_R */
#define PLL_R 2 /* Possible value between 2 and 7 */

/* ex. to have SYSCLK @  120 MHz
       SYSCLK = PLLVCO / PLL_P
              = ((HSE_VALUE / PLL_M) * PLL_N) / PLL_P
              = ((25 MHz / 25) * 240 ) / 2 = 120 MHz
   */

/*!< Uncomment the following line if you need to relocate your vector Table in
     Internal SRAM. */
/* #define VECT_TAB_SRAM */
#define VECT_TAB_OFFSET 0x0
/*!< Vector Table base offset field.
                                  This value must be a multiple of 0x100. */

/*******************************************************************************
*  Clock Definitions
*******************************************************************************/
//uint32_t SystemCoreClock = ((HSE_VALUE / PLL_M) * PLL_N) / PLL_P; /*!< System Clock Frequency (Core Clock) */
uint32_t SystemCoreClock = 0; /*!< System Clock Frequency (Core Clock) */

/**
  * @}
  */

/** @addtogroup stm32f4xx_System_Private_FunctionPrototypes
  * @{
  */

static void SetSysClock(void);

/** @addtogroup stm32f4xx_System_Private_Functions
  * @{
  */

/**
  * @brief  Setup the microcontroller system
  *         Initialize the Embedded Flash Interface, the PLL and update the SystemFrequency variable.
  * @note   This function should be used only after reset.
  * @param  None
  * @retval None
  */
void SystemInit(void)
{
    /* Reset the RCC clock configuration to the default reset state(for debug purpose) */
    /* Set HSION bit */
    RCC->CR |= (uint32_t)0x00000001;
    /* Reset CFGR register */
    RCC->CFGR = 0x00000000;
    /* Reset HSEON, CSSON and PLLON bits */
    RCC->CR &= (uint32_t)0xFEF6FFFF;
    /* Reset PLLCFGR register */
    RCC->PLLCFGR = 0x00000000;
    /* Reset HSEBYP bit */
    RCC->CR &= (uint32_t)0xFFFBFFFF;
    /* Disable all interrupts */
    RCC->CIR = 0x00000000;

    /* Configure the System clock frequency, HCLK, PCLK2 and PCLK1 prescalers */
    /* Configure the Flash Latency cycles and enable prefetch buffer */
    SetSysClock();

#ifdef VECT_TAB_SRAM
    SCB->VTOR = SRAM_BASE | VECT_TAB_OFFSET; /* Vector Table Relocation in Internal SRAM. */
#else
    SCB->VTOR = FLASH_BASE | VECT_TAB_OFFSET; /* Vector Table Relocation in Internal FLASH. */
#endif
}

/**
  * @brief  Configures the System clock frequency, HCLK, PCLK2 and PCLK1 prescalers.
  * @param  None
  * @retval None
  */
static void SetSysClock(void)
{
    uint32_t AHB_Prescaler, APB1_Prescaler, APB2_Prescaler;
    // recalculate PLL_X constants and SystemCoreClock
    if((SYSCLK_VALUE == 100000000) && (HSE_VALUE == 25000000)) {
        PLL_M = 25;
        PLL_N = 200;
        AHB_Prescaler = RCC_CFGR_HPRE_DIV1;
        APB1_Prescaler = RCC_CFGR_PPRE1_DIV4;
        APB2_Prescaler = RCC_CFGR_PPRE2_DIV2;
    } else if((SYSCLK_VALUE == 84000000) && (HSE_VALUE == 25000000)) {
        PLL_M = 25;
        PLL_N = 168;
        AHB_Prescaler = RCC_CFGR_HPRE_DIV1;
        APB1_Prescaler = RCC_CFGR_PPRE1_DIV2;
        APB2_Prescaler = RCC_CFGR_PPRE2_DIV1;
    } else if((SYSCLK_VALUE == 16000000) && (HSE_VALUE == 16000000)) {
        PLL_M = 8;
        PLL_N = 64;
        AHB_Prescaler = RCC_CFGR_HPRE_DIV4;
        APB1_Prescaler = RCC_CFGR_PPRE1_DIV1;
        APB2_Prescaler = RCC_CFGR_PPRE2_DIV1;
    } else {
        // #error Unsupported SYSCLK_VALUE or HSE_VALUE
    }
    SystemCoreClock = ((HSE_VALUE / PLL_M) * PLL_N) / PLL_P;

    __IO uint32_t StartUpCounter = 0, HSEStatus = 0;

    /* Enable HSE */
    RCC->CR |= ((uint32_t)RCC_CR_HSEON);

    /* Wait till HSE is ready and if Time out is reached exit */
    do {
        HSEStatus = RCC->CR & RCC_CR_HSERDY;
        StartUpCounter++;
    } while((HSEStatus == 0) && (StartUpCounter != HSE_STARTUP_TIMEOUT));

    if((RCC->CR & RCC_CR_HSERDY) != RESET) {
        HSEStatus = (uint32_t)0x01;
    } else {
        HSEStatus = (uint32_t)0x00;
    }

    if(HSEStatus == (uint32_t)0x01) {
        /* HCLK = SYSCLK */
        RCC->CFGR |= AHB_Prescaler;

        /* PCLK2 = HCLK / 2  */
        RCC->CFGR |= APB2_Prescaler;

        /* PCLK1 = HCLK / 4 */
        RCC->CFGR |= APB1_Prescaler;

        /* Select HSE as PLL source */
        RCC->PLLCFGR = PLL_M | (PLL_N << 6) | (((PLL_P >> 1) - 1) << 16) | (RCC_PLLCFGR_PLLSRC_HSE) | (PLL_Q << 24) | (PLL_R << 28);

        /* Enable PLL */
        RCC->CR |= ((uint32_t)RCC_CR_PLLON);

        /* Wait till PLL is ready */
        while((RCC->CR & RCC_CR_PLLRDY) == 0) {
        }

        /*  Enable Flash prefetch, Instruction cache and Data cache for max performance
        and set Flash 3 wait state */
        FLASH->ACR = FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN | 0x03;

        /* Select PLL as system clock source */
        RCC->CFGR &= (uint32_t)((uint32_t) ~(RCC_CFGR_SW));
        RCC->CFGR |= ((uint32_t)RCC_CFGR_SW_PLL);

        /* Wait till PLL is used as system clock source */
        while((RCC->CFGR & (uint32_t)RCC_CFGR_SWS) != (uint32_t)RCC_CFGR_SWS_PLL) {
        }
    } else { /* If HSE fails to start-up, the application will have wrong clock
         configuration. User can add here some code to deal with this error */
    }
}

/**
  * @brief  Update SystemCoreClock according to Clock Register Values
  * @note   None
  * @param  None
  * @retval None
  */
void SystemCoreClockUpdate(void)
{
    /* To be implemented in next version */
}

/**
  * @brief  Setup the external memory controller. Called in startup_stm32f4xx.s
  *          before jump to __main
  * @param  None
  * @retval None
  */
#ifdef DATA_IN_ExtSRAM
/**
  * @brief  Setup the external memory controller.
  *         Called in startup_stm32f4xx.s/.c before jump to main.
  *         This function configures the external PSRAM mounted on STM3220F_EVAL board
  *         This PSRAM will be used as program data memory (including heap and stack).
  * @param  None
  * @retval None
  */
void SystemInit_ExtMemCtl(void)
{
    /* To be implemented in next version */
}
#endif /* DATA_IN_ExtSRAM */

/******************* (C) COPYRIGHT 2010 STMicroelectronics *****END OF FILE****/
