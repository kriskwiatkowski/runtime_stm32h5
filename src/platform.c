// SPDX-FileCopyrightText: 2026 AmongBytes, Ltd.
// SPDX-FileContributor: Krzysztof Kwiatkowski <kris (at) amongbytes.com>
// SPDX-License-Identifier: LicenseRef-Proprietary

#include <iso646.h>
#include <platform/platform.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "printf.h"
#include "stm32h5xx.h"
#include "stm32h5xx_hal.h"

// Flags if cyclecount was initialized
static bool kCyclecountInit = false;

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

    if (kCyclecountInit) {
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
     * bits 31:28 (NUMCOMP).
     * It will also enable PC sampling and set sync tap to control packet rate. */
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    // Ensure DWT_CTRL_CYCCNTENA_Msk is written
    __DSB();
    __ISB();

    /* Enable PC sampling in the DWT. Every N clock cycles, the current value of
     * the PC is automatically captured and emitted as an ITM packet over SWO. */
    DWT->CTRL |= DWT_CTRL_PCSAMPLENA_Msk;
    /* Sets which bit of the cycle counter is used to trigger periodic synchronisation
     * packets on the SWO stream. This allow the host-side decoder (orbuculum, pyOCD)
     * to lock onto the byte stream and correctly frame the incoming ITM packets.
     * PC samples have source ID = 2 and carry 32-bit value. This corresponds to
     * the value 0x17 in python script for reading ITM packets.
     * At higher speed the DWT_CTRL_CYCTAP_Pos may be the better choice (avoids flooding
     * SWO). */
    // TODO: This doesn't seem to be supported by the board.
    // It seems like SYNCTAP is RAZ/WI on STM32H573 - the hardware uses a fixed
    //       internal tap at bit 6 (÷64) regardless of SYNCTAP value.
    //DWT->CTRL |= DWT_CTRL_SYNCTAP_Msk;

    /* Controls the PC sampling rate using a two-stage clock divider:
     *
     * Stage 1 (SYNCTAP=0x3): taps bit 14 of CYCCNT, producing a tick every
     *                        2^14 = 16384 cycles.
     * Stage 2 (POSTPRESET=7): counts 8 ticks from stage 1 before capturing
     *                        and emitting a PC sample over SWO.
     *
     * sample rate = CPU_CLK / (64 * (POSTPRESET + 1))
     *             = 32 MHz  / (64 * 8)
     *             ~ 62,500 samples/sec
     *             = one sample every 512 cycles
     *
     * It seems, that on the board the SYNCTAP is fixed to SYNCTAP=0x3, which taps
     * bit 6 of CYCCNT, producing a tick every 64 cycles.
     *
     * For more information see Architecture Reference Manual, section about
     * "DWT_CTRL, Control register"
     */
    DWT->CTRL |= (0x7 << DWT_CTRL_POSTPRESET_Pos);

    // Wait until cycle counter is enabled
    while (!(DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk)) {
    }

    // Wait until cycle counter starts counting
    while (DWT->CYCCNT == 0) {
    }

    kCyclecountInit = true;
    return true;
}

// SWO clock is based on CPU clock, so it needs to be updated when CPU clock changes.
static void update_swo_clock(uint32_t cpu_hz) {
    static const uint32_t target_swo_baud = 4000000;  // 4 MHz is very stable
    // Prescaler = (CPU_Clock / Target_Baud) - 1
    TPI->ACPR = (cpu_hz / target_swo_baud) - 1;
}

// Enables ITM (Instrumentation Trace Macrocell) for printf support in SWO viewer. This is optional.
static void itm_init(void) {
    // Configure the SWO Pin (PB3). For SWO, the PB3 must be Alternate Function (AF0)
    GPIOB->MODER   &= ~GPIO_MODER_MODE3_Msk;
    GPIOB->MODER   |= (0x2 << GPIO_MODER_MODE3_Pos);  // AF mode
    GPIOB->AFR[0]  &= ~GPIO_AFRL_AFSEL3_Msk;  // AF0 is usually Trace/SWO
    GPIOB->OSPEEDR |= (0x3 << GPIO_OSPEEDR_OSPEED3_Pos);  // Very High Speed

    // Enable TRACE_EN in DBGMCU
    DBGMCU->CR |= DBGMCU_CR_TRACE_IOEN;

    // Use UART/NRZ encoding, which is what STLINK expects on the SWO pin
    TPI->SPPR = 2;
    // Update SWO clock based on current CPU clock
    update_swo_clock(HAL_RCC_GetSysClockFreq());

    /* Master Trace Control:
     * Bit 0 - ITMENA: Enable ITM
     * Bit 2 - SYNCENA: Periodic sync packets so the decoder can lock onto the SWO stream
     * Bit 3 - DWTENA: Forwards DWT-generated packets (including PC samples) through ITM to the TPIU */
    ITM->TCR |= ITM_TCR_SYNCENA_Msk | ITM_TCR_DWTENA_Msk | ITM_TCR_ITMENA_Msk;

    // Unprivileged access allowed on ALL ports
    ITM->TPR = 0xFFFFFFFF;

    // Enable all stimulus ports
    ITM->TER = 0xFFFFFFFF;
}

void platform_log(const struct platform_log_t *log) {
    uint8_t c = log->channel;
    if (log->type == PLATFORM_LOG_TYPE_U32) {
        // Log uint32_t data to channel 0
        while (ITM->PORT[c].u32 == 0UL)
            ;
        ITM->PORT[c].u32 = log->a.data;
        while (ITM->PORT[c].u32 == 0UL)
            ;
    } else if (log->type == PLATFORM_LOG_TYPE_STRING) {
        // Log string data to channel 1
        const uint8_t *str = (const uint8_t *)log->a.str;
        while (*str) {
            while (ITM->PORT[c].u8 == 0UL)
                ;
            ITM->PORT[c].u8 = *str++;
        }
    }
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
        update_swo_clock(32000000);  // Update SWO clock for 32 MHz CPU clock
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
        update_swo_clock(250000000);  // Update SWO clock for 250 MHz CPU clock
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
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
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
    itm_init();
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
