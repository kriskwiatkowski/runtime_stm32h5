/**
  ******************************************************************************
  * @file    system_stm32h5xx.c
  * @author  MCD Application Team
  * @brief   CMSIS Cortex-M33 Device Peripheral Access Layer System Source File
  *
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  *   This file provides two functions and one global variable to be called from
  *   user application:
  *      - SystemInit(): This function is called at startup just after reset and
  *                      before branch to main program. This call is made inside
  *                      the "startup_stm32h5xx.s" file.
  *
  *      - SystemCoreClock variable: Contains the core clock (HCLK), it can be used
  *                                  by the user application to setup the SysTick
  *                                  timer or configure other parameters.
  *
  *      - SystemCoreClockUpdate(): Updates the variable SystemCoreClock and must
  *                                 be called whenever the core clock is changed
  *                                 during program execution.
  *
  *   After each device reset the HSI (64 MHz) is used as system clock source.
  *   Then SystemInit() function is called, in "startup_stm32h5xx.s" file, to
  *   configure the system clock before to branch to main program.
  *
  *   This file configures the system clock as follows:
  *=============================================================================
  *-----------------------------------------------------------------------------
  *        System Clock source                     | HSI
  *-----------------------------------------------------------------------------
  *        SYSCLK(Hz)                              | 64000000
  *-----------------------------------------------------------------------------
  *        HCLK(Hz)                                | 64000000
  *-----------------------------------------------------------------------------
  *        AHB Prescaler                           | 1
  *-----------------------------------------------------------------------------
  *        APB1 Prescaler                          | 1
  *-----------------------------------------------------------------------------
  *        APB2 Prescaler                          | 1
  *-----------------------------------------------------------------------------
  *        APB3 Prescaler                          | 1
  *-----------------------------------------------------------------------------
  *        HSI Division factor                     | 1
  *-----------------------------------------------------------------------------
  *        PLL1_SRC                                | No clock
  *-----------------------------------------------------------------------------
  *        PLL1_M                                  | Prescaler disabled
  *-----------------------------------------------------------------------------
  *        PLL1_N                                  | 129
  *-----------------------------------------------------------------------------
  *        PLL1_P                                  | 2
  *-----------------------------------------------------------------------------
  *        PLL1_Q                                  | 2
  *-----------------------------------------------------------------------------
  *        PLL1_R                                  | 2
  *-----------------------------------------------------------------------------
  *        PLL1_FRACN                              | 0
  *-----------------------------------------------------------------------------
  *        PLL2_SRC                                | No clock
  *-----------------------------------------------------------------------------
  *        PLL2_M                                  | Prescaler disabled
  *-----------------------------------------------------------------------------
  *        PLL2_N                                  | 129
  *-----------------------------------------------------------------------------
  *        PLL2_P                                  | 2
  *-----------------------------------------------------------------------------
  *        PLL2_Q                                  | 2
  *-----------------------------------------------------------------------------
  *        PLL2_R                                  | 2
  *-----------------------------------------------------------------------------
  *        PLL2_FRACN                              | 0
  *-----------------------------------------------------------------------------
  *        PLL3_SRC                                | No clock
  *-----------------------------------------------------------------------------
  *        PLL3_M                                  | Prescaler disabled
  *-----------------------------------------------------------------------------
  *        PLL3_N                                  | 129
  *-----------------------------------------------------------------------------
  *        PLL3_P                                  | 2
  *-----------------------------------------------------------------------------
  *        PLL3_Q                                  | 2
  *-----------------------------------------------------------------------------
  *        PLL3_R                                  | 2
  *-----------------------------------------------------------------------------
  *        PLL3_FRACN                              | 0
  *-----------------------------------------------------------------------------
  *=============================================================================
  */

/** @addtogroup CMSIS
  * @{
  */

/** @addtogroup STM32H5xx_system
  * @{
  */

/** @addtogroup STM32H5xx_System_Private_Includes
  * @{
  */

#include "stm32h5xx.h"

/**
  * @}
  */

/** @addtogroup STM32H5xx_System_Private_TypesDefinitions
  * @{
  */

/**
  * @}
  */

/** @addtogroup STM32H5xx_System_Private_Defines
  * @{
  */

#if !defined  (HSE_VALUE)
#define HSE_VALUE    (25000000UL) /*!< Value of the External oscillator in Hz */
#endif /* HSE_VALUE */

#if !defined  (CSI_VALUE)
#define CSI_VALUE    (4000000UL)  /*!< Value of the Internal oscillator in Hz*/
#endif /* CSI_VALUE */

#if !defined  (HSI_VALUE)
#define HSI_VALUE    (64000000UL) /*!< Value of the Internal oscillator in Hz */
#endif /* HSI_VALUE */

#define SYSTEM_CLOCK_INIT    (64000000UL)

/************************* Miscellaneous Configuration ************************/
/*!< Uncomment the following line if you need to relocate your vector Table in
     Internal SRAM. */
/* #define VECT_TAB_SRAM */
#define VECT_TAB_OFFSET  0x00U /*!< Vector Table base offset field.
                                   This value must be a multiple of 0x200. */
/******************************************************************************/

/**
  * @}
  */

/** @addtogroup STM32H5xx_System_Private_Macros
  * @{
  */

/**
  * @}
  */

/** @addtogroup STM32H5xx_System_Private_Variables
  * @{
  */
/* The SystemCoreClock variable is updated in three ways:
    1) by calling CMSIS function SystemCoreClockUpdate()
    2) by calling HAL API function HAL_RCC_GetHCLKFreq()
    3) each time HAL_RCC_ClockConfig() is called to configure the system clock frequency
       Note: If you use this function to configure the system clock; then there
             is no need to call the 2 first functions listed above, since SystemCoreClock
             variable is updated automatically.
*/
uint32_t SystemCoreClock = SYSTEM_CLOCK_INIT;

const uint8_t  AHBPrescTable[16] = {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 1U, 2U, 3U, 4U, 6U, 7U, 8U, 9U};
const uint8_t  APBPrescTable[8] =  {0U, 0U, 0U, 0U, 1U, 2U, 3U, 4U};
/**
  * @}
  */

/** @addtogroup STM32H5xx_System_Private_FunctionPrototypes
  * @{
  */

/**
  * @}
  */

/** @addtogroup STM32H5xx_System_Private_Functions
  * @{
  */

/**
  * @brief  Setup the microcontroller system.
  * @param  None
  * @retval None
  */

void SystemInit(void)
{
  /* Nothing done in non-secure */
}

/**
  * @brief  Update SystemCoreClock variable according to Clock Register Values.
  *         The SystemCoreClock variable contains the core clock (HCLK), it can
  *         be used by the user application to setup the SysTick timer or configure
  *         other parameters.
  *
  * @note   Each time the core clock (HCLK) changes, this function must be called
  *         to update SystemCoreClock variable value. Otherwise, any configuration
  *         based on this variable will be incorrect.
  *
  * @note   - The system frequency computed by this function is not the real
  *           frequency in the chip. It is calculated based on the predefined
  *           constant and the selected clock source:
  *
  *           - If SYSCLK source is CSI, SystemCoreClock will contain the CSI_VALUE(*)
  *
  *           - If SYSCLK source is HSI, SystemCoreClock will contain the HSI_VALUE(**)
  *
  *           - If SYSCLK source is HSE, SystemCoreClock will contain the HSE_VALUE(***)
  *
  *           - If SYSCLK source is PLL, SystemCoreClock will contain the HSE_VALUE(***)
  *             or HSI_VALUE(**) or CSI_VALUE(*) multiplied/divided by the PLL factors.
  *
  *         (*) CSI_VALUE is a constant defined in stm32h5xx_hal.h file (default value
  *             4 MHz) but the real value may vary depending on the variations
  *             in voltage and temperature.
  *
  *         (**) HSI_VALUE is a constant defined in stm32h5xx_hal.h file (default value
  *              64 MHz) but the real value may vary depending on the variations
  *              in voltage and temperature.
  *
  *         (***) HSE_VALUE is a constant defined in stm32h5xx_hal.h file (default value
  *              25 MHz), user has to ensure that HSE_VALUE is same as the real
  *              frequency of the crystal used. Otherwise, this function may
  *              have wrong result.
  *
  *         - The result of this function could be not correct when using fractional
  *           value for HSE crystal.
  *
  * @param  None
  * @retval None
  */
void SystemCoreClockUpdate(void)
{
  uint32_t pllp;
  uint32_t pllsource;
  uint32_t pllm;
  uint32_t pllfracen;
  uint32_t hsivalue;
  uint32_t tmp;
  float_t fracn1;
  float_t pllvco;

  /* Get SYSCLK source -------------------------------------------------------*/
  switch (RCC->CFGR1 & RCC_CFGR1_SWS)
  {
    case 0x00UL:  /* HSI used as system clock source */
      SystemCoreClock = (uint32_t)(HSI_VALUE >> ((RCC->CR & RCC_CR_HSIDIV) >> 3));
      break;

    case 0x08UL:  /* CSI used as system clock  source */
      SystemCoreClock = CSI_VALUE;
      break;

    case 0x10UL:  /* HSE used as system clock  source */
      SystemCoreClock = HSE_VALUE;
      break;

    case 0x18UL:  /* PLL1 used as system clock source */
      /* PLL_VCO = (HSE_VALUE or HSI_VALUE or CSI_VALUE/ PLLM) * PLLN
      SYSCLK = PLL_VCO / PLLR
      */
      pllsource = (RCC->PLL1CFGR & RCC_PLL1CFGR_PLL1SRC);
      pllm = ((RCC->PLL1CFGR & RCC_PLL1CFGR_PLL1M) >> RCC_PLL1CFGR_PLL1M_Pos);
      pllfracen = ((RCC->PLL1CFGR & RCC_PLL1CFGR_PLL1FRACEN) >> RCC_PLL1CFGR_PLL1FRACEN_Pos);
      fracn1 = (float_t)(uint32_t)(pllfracen * ((RCC->PLL1FRACR & RCC_PLL1FRACR_PLL1FRACN) >>
                                                RCC_PLL1FRACR_PLL1FRACN_Pos));

      switch (pllsource)
      {
        case 0x01UL:  /* HSI used as PLL clock source */
          hsivalue = (HSI_VALUE >> ((RCC->CR & RCC_CR_HSIDIV) >> 3)) ;
          pllvco = ((float_t)hsivalue / (float_t)pllm) * ((float_t)(uint32_t)(RCC->PLL1DIVR & RCC_PLL1DIVR_PLL1N) + \
                                                          (fracn1 / (float_t)0x2000) + (float_t)1);
          break;

        case 0x02UL:  /* CSI used as PLL clock source */
          pllvco = ((float_t)CSI_VALUE / (float_t)pllm) * ((float_t)(uint32_t)(RCC->PLL1DIVR & RCC_PLL1DIVR_PLL1N) + \
                                                           (fracn1 / (float_t)0x2000) + (float_t)1);
          break;

        case 0x03UL:  /* HSE used as PLL clock source */
          pllvco = ((float_t)HSE_VALUE / (float_t)pllm) * ((float_t)(uint32_t)(RCC->PLL1DIVR & RCC_PLL1DIVR_PLL1N) + \
                                                           (fracn1 / (float_t)0x2000) + (float_t)1);
          break;

        default:  /* No clock sent to PLL*/
          pllvco = (float_t) 0U;
          break;
      }

      pllp = (((RCC->PLL1DIVR & RCC_PLL1DIVR_PLL1P) >> RCC_PLL1DIVR_PLL1P_Pos) + 1U) ;
      SystemCoreClock = (uint32_t)(float_t)(pllvco / (float_t)pllp);

      break;

    default:
      SystemCoreClock = HSI_VALUE;
      break;
  }
  /* Compute HCLK clock frequency --------------------------------------------*/
  /* Get HCLK prescaler */
  tmp = AHBPrescTable[((RCC->CFGR2 & RCC_CFGR2_HPRE) >> RCC_CFGR2_HPRE_Pos)];
  /* HCLK clock frequency */
  SystemCoreClock >>= tmp;
}


/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */
