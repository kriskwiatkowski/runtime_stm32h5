// SPDX-FileCopyrightText: 2026 AmongBytes, Ltd.
// SPDX-FileContributor: Kris Kwiatkowski <kris@amongbytes.com>
// SPDX-License-Identifier: LicenseRef-Proprietary

#include <iso646.h>
#include <platform/platform.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "printf.h"
#include "stm32h5xx.h"
#include "stm32h5xx_hal.h"

uint32_t platform_xorshift32_u32(void);
void     platform_usart1_init(void);
void    *platform_get_uart1_handle(void);
void     platform_rng_init(void);

/* MPU Configuration */
static void mpu_config(bool enable) {
    MPU_Region_InitTypeDef     MPU_InitStruct     = { 0 };
    MPU_Attributes_InitTypeDef MPU_AttributesInit = { 0 };

    /* Disables the MPU */
    HAL_MPU_Disable();

    if (enable) {
        // Initializes and configures the Region 0 and the memory to be protected
        MPU_InitStruct.Enable           = MPU_REGION_ENABLE;
        MPU_InitStruct.Number           = MPU_REGION_NUMBER0;
        MPU_InitStruct.BaseAddress      = 0x08FFF000;
        MPU_InitStruct.LimitAddress     = 0x08FFFFFF;
        MPU_InitStruct.AttributesIndex  = MPU_ATTRIBUTES_NUMBER0;
        MPU_InitStruct.AccessPermission = MPU_REGION_ALL_RO;
        MPU_InitStruct.DisableExec      = MPU_INSTRUCTION_ACCESS_DISABLE;
        MPU_InitStruct.IsShareable      = MPU_ACCESS_NOT_SHAREABLE;

        HAL_MPU_ConfigRegion(&MPU_InitStruct);

        // Initializes and configures the Attribute 0 and the memory to be protected
        MPU_AttributesInit.Number     = MPU_ATTRIBUTES_NUMBER0;
        MPU_AttributesInit.Attributes = INNER_OUTER(MPU_NOT_CACHEABLE);
        HAL_MPU_ConfigMemoryAttributes(&MPU_AttributesInit);

        // Enables the MPU
        HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
    }
}

/// ############################
/// Cycle count support
/// ############################

static void cyclecount_reset(void) {
    // This is valid approach according to ARM DDI 0403E.b, section C1.8.8.
    DWT->CYCCNT = 0;  // 0 is a reset value.
}

/* @brief Enables CPU cycle counting.
 *
 * This implementation uses Debug Watchpoint and Trace (DWT) unit's.
 * According to SmartFusion2 data sheet, the Cortex-M3 core implements DWT as
 * per ARMv7-M architecture reference manual (ARM DDI 0403E.b). That means, we
 * can follow the procedure described in ARM DDI 0403E.b, but we must avoid all
 * implementation-defined features, as there is no documentation about them.
 *
 * According to ARM DDI 0403E.b:
 * - To check if DWT is present, we can follow Table C1-2.
 * - To enable CYCCNT, we need to set TRCENA in DEMCR and CYCCNTENA in DWT_CTRL.
 * - To read the cycle count, we can read DWT_CYCCNT.
 * - To reset the cycle count, we can write 0 to DWT_CYCCNT (I assume this is
 * the only allowed reset value).
 *
 * This function enables Trace Enable bit in the Debug Exception and Monitor
 * Control Register. This bit is used to enable several features, including DWT.
 * According to ARM DDI 0403E.b, section C1.6.5, this functionality can only be
 * DISABLED after all ITM and DWT features are disabled. As it would be hard to
 * track that in a generic way, we never disable TRCENA after it is enabled, to
 * make sure results returned by DWT are reliable
 *
 * @retval true if cycle counting was successfully enabled either by the call
 *         to this function or in the past.
 * @retval false if cycle counting could not be enabled (e.g., DWT not present).
 */
static bool cyclecount_init(void) {
    if (CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk) {
        return true;  // Already initialized
    }

    /* Performs a DWT availability check, according to Table C1-2. Note that
     * this check can be done before enabling TRCENA. */
    if (!DWT) {
        return false;  // DWT not present
    }

    /* Enable. It must be done before any DWT registers access (see ARM DDI
     * 0403E.b, section C1.6.5).  */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

    // Check if CYCCNT is present. This must be done after enabling TRCENA.
    if (DWT->CTRL & DWT_CTRL_NOCYCCNT_Msk) {
        return false;  // CYCCNT not present
    }

    // Reset cycle counter
    DWT->CYCCNT = 0;

    /* Start cycle counter. Note that this operation will try to write read-only
     * bits 31:28 (NUMCOMP). Nevertheless, Writing 0 does not affect them; they
     * will still read back as the hardware value. */
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    // Wait until cycle counter is enabled
    while (!(DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk)) {
    }
    // Wait until cycle counter starts counting
    while (DWT->CYCCNT == 0) {
    }

    return true;
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void err_handler(void);

/**
  * @brief System Clock Configuration
  * @param is_32mhz If true, use the original 32 MHz setup.
  *                   If false, switch to 250 MHz using PLL1.
  * Difference between 32 vs 250 Mhz:
  * - Different Flash wait states (LATENCY_1 vs LATENCY_5)
  * - Different Flash programming delay settings (FLASH_PROGRAMMING_DELAY_0 vs FLASH_PROGRAMMING_DELAY_2)
  * - Cache miss penalties and memory/bus behavior
  * - Peripheral timing/ready flags if benchmark touches peripherals
  *
  * For algorithm benchmarking, the 32MhZ is more precise. For real-world performance, the
  * 250 MHz is more relevant.
  *
  * @retval None
  */
void SystemClock_Config(bool is_32mhz) {
    RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
    RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

    if (is_32mhz) {
        // Original setup: HSI/2 -> 32 MHz SYSCLK.
        __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

        while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {
        }

        RCC_OscInitStruct.OscillatorType =
            RCC_OSCILLATORTYPE_HSI48 | RCC_OSCILLATORTYPE_HSI;
        RCC_OscInitStruct.HSIState            = RCC_HSI_ON;
        RCC_OscInitStruct.HSIDiv              = RCC_HSI_DIV2;
        RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
        RCC_OscInitStruct.HSI48State          = RCC_HSI48_ON;
        RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_NONE;
        if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
            err_handler();
        }

        RCC_ClkInitStruct.ClockType =
            RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 |
            RCC_CLOCKTYPE_PCLK2 | RCC_CLOCKTYPE_PCLK3;
        RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_HSI;
        RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
        RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
        RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
        RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;

        if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) !=
            HAL_OK) {
            err_handler();
        }

        __HAL_FLASH_SET_PROGRAM_DELAY(FLASH_PROGRAMMING_DELAY_0);
    } else {

        // High-speed setup: 250 MHz SYSCLK from HSI via PLL1.
        __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

        while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {
        }

        /** Initializes the RCC Oscillators according to the specified parameters
       * in the RCC_OscInitTypeDef structure. */
        RCC_OscInitStruct.OscillatorType =
            RCC_OSCILLATORTYPE_HSI48 | RCC_OSCILLATORTYPE_HSI;
        RCC_OscInitStruct.HSIState            = RCC_HSI_ON;
        RCC_OscInitStruct.HSIDiv              = RCC_HSI_DIV1;
        RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
        RCC_OscInitStruct.HSI48State          = RCC_HSI48_ON;
        RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_ON;
        RCC_OscInitStruct.PLL.PLLSource       = RCC_PLL1_SOURCE_HSI;
        RCC_OscInitStruct.PLL.PLLM            = 7;
        RCC_OscInitStruct.PLL.PLLN            = 54;
        RCC_OscInitStruct.PLL.PLLP            = 2;
        RCC_OscInitStruct.PLL.PLLQ            = 2;
        RCC_OscInitStruct.PLL.PLLR            = 2;
        RCC_OscInitStruct.PLL.PLLRGE          = RCC_PLL1_VCIRANGE_3;
        RCC_OscInitStruct.PLL.PLLVCOSEL       = RCC_PLL1_VCORANGE_WIDE;
        RCC_OscInitStruct.PLL.PLLFRACN        = 5632;
        if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
            err_handler();
        }

        // Initializes the CPU, AHB and APB buses clocks
        RCC_ClkInitStruct.ClockType =
            RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 |
            RCC_CLOCKTYPE_PCLK2 | RCC_CLOCKTYPE_PCLK3;
        RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
        RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
        RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
        RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
        RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;

        if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) !=
            HAL_OK) {
            err_handler();
        }

        // Configure flash programming delay for 168-250 MHz.
        __HAL_FLASH_SET_PROGRAM_DELAY(FLASH_PROGRAMMING_DELAY_2);
    }
}

/**
  * @brief ICACHE Sets up the cache
  *        Enables or disables the instruction cache,
  *        - ICACHE - if enabled it configures it as associative 2-way cache (direct mapped).
  * @param is_enable If true, enable the cache. If false, disable the cache.
  * @retval None
  */
static void setup_cache(bool is_enable) {
    // Disable and invalidate the cache before configuring it.
    if (HAL_ICACHE_Disable() != HAL_OK) {
        err_handler();
    }

    if (HAL_ICACHE_Invalidate() != HAL_OK) {
        err_handler();
    }

    /* DSB + ISB are required after cache invalidation: DSB ensures the
     * invalidation completes before any subsequent memory accesses, and ISB
     * flushes the Cortex-M33 pipeline so the next instruction fetch is a
     * guaranteed cache miss. */
    __DSB();
    __ISB();

    if (is_enable) {
        __HAL_FLASH_PREFETCH_BUFFER_ENABLE();

        // Enable instruction cache in 2-way (direct mapped cache)
        if (HAL_ICACHE_ConfigAssociativityMode(ICACHE_2WAYS) != HAL_OK) {
            err_handler();
        }
        if (HAL_ICACHE_Enable() != HAL_OK) {
            err_handler();
        }
    }
}

char platform_getchar(void) {
    uint8_t d;
    void   *huart = platform_get_uart1_handle();
    while (HAL_UART_Receive(huart, &d, 1, 5000 /*5 seconds*/) != HAL_OK)
        ;
    return d;
}

void platform_putchar(char c) {
    uint8_t d     = c;
    void   *huart = platform_get_uart1_handle();
    HAL_UART_Transmit(huart, &d, 1, 5000 /*5 seconds*/);
}

// Used by printf
void _putchar(char character) { platform_putchar(character); }

static void gpio_init(void) {
    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
}

int platform_init(platform_op_mode_t a) {

    /* Configure MPU to protect the memory region where the secret is stored. This is not strictly
     * necessary, but it adds an extra layer of security against bugs in the code that could potentially
     * leak the secret. It also allows to test that the MPU is working correctly. */
    mpu_config(false);
    // Initialize the HAL library; it must be the first function to be executed before the call of any HAL function.
    HAL_Init();
    // Configure the system clock: false => 250 MHz, true => original 32 MHz.
    SystemClock_Config(a == PLATFORM_CLOCK_USERSPACE);

    // Initialize peripherals
    gpio_init();        // GPIO is needed for USART1 and RNG.
    setup_cache(true);  // Improve execution speed by enabling I-cache.
    platform_rng_init();  // Initialize the hardware RNG, which is used to seed the software RNG.
    platform_usart1_init(); /* Initialize USART1, this is connected to CN10 (ST-Link Virtual COM Port)
                               * and is used for debugging and communication with the host.
                               * It is also possibel to use USAERT3, that is connected to D0/D1 (Arduino ports). */
    cyclecount_init();  // Initialize the cycle counter, which is used for timing measurements.
    return 0;
}

bool platform_set_attr(const struct platform_attr_t *a) {
    size_t i;

    for (i = 0; i < a->n; i++) {
        switch (a->attr[i]) {
            case PLATFORM_CLOCK_MAX:
            case PLATFORM_CLOCK_USERSPACE:
                SystemClock_Config(a->attr[i] == PLATFORM_CLOCK_USERSPACE);
                break;
            case PLATFORM_CACHE_ENABLE:
            case PLATFORM_CACHE_DISABLE:
                setup_cache(a->attr[i] == PLATFORM_CACHE_ENABLE);
                break;
            default:
                return false;
        }
    }

    return true;
}

uint64_t platform_cpu_cyclecount(void) {
    /* Note that CYCCNT is a 32-bit counter that wraps around. Here we are
     * casting to uint64_t to match the function signature, but the value is still
     * 32-bit and will wrap around every ~4295 million cycles.
     * Should better precision be needed, one can adapt SysTick. */
    return (uint64_t)DWT->CYCCNT;
}

void platform_sync(void) { cyclecount_reset(); }
