/*
 * Copyright (C) Kris Kwiatkowski, Among Bytes LTD
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  See <http://www.fsf.org/copyleft/gpl.txt>.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Platform attributes
typedef enum {
    PLATFORM_CLOCK_MAX,
    //!< Sets the clock to max possible value
    PLATFORM_CLOCK_USERSPACE,
    //!< Sets the CPU clock to 24Mhz (used for benchmarking)
    PLATFORM_SCA_TRIGGER_HIGH,
    //!< Sets the trigger pin high
    PLATFORM_SCA_TRIGGER_LOW,
    //!< Sets the trigger pin low
    PLATFORM_CACHE_ENABLE,
    //!< Enable cache
    PLATFORM_CACHE_DISABLE
    //!< Disable cache
} platform_op_mode_t;

// Data logging types. Used in \p platform_log_t to specify the type of data being logged.
enum {
    //!< Log uint32_t data
    PLATFORM_LOG_TYPE_U32 = 0,
    //!< Log string data
    PLATFORM_LOG_TYPE_STRING = 1
};

// Structure for trace log data to the host via ITM
struct platform_log_t {
    // Channel number (0 or 1)
    uint8_t channel;
    // Data type: PLATFORM_LOG_TYPE_*
    uint8_t type;
    // Data to transfer
    union {
        //!< 32-bit value
        uint32_t data;
        //!< String value (zero-terminated)
        const char *str;
    } a;
};

//!< Structure for passing platform attributes
struct platform_attr_t {
    //!< Array of attributes to set, each attribute is a value from \p platform_op_mode_t enum
    uint32_t attr[4];
    //!< Number of attributes in the array (max 4)
    size_t n;
};

/**
 * @brief Initializes STM32F2 platform.
 * @retval Always 0.
 */
int platform_init(platform_op_mode_t a);

// Set platform attribute.
bool platform_set_attr(const struct platform_attr_t *a);

// Returns current number of cycles on a clock.
uint64_t platform_cpu_cyclecount(void);

// Get random number from TRNG
int platform_get_random(void *out, unsigned len);

// Improves benchmark results
void platform_sync(void);

// Get a character from the platform's input
char platform_getchar(void);

// Put a character to the platform's output
void platform_putchar(char c);

/**
 * @brief Debug logging utility.
 *
 * The function uses ITM (Instrumentation Trace Macrocell) to log data to the
 * host. It can log either a 32-bit value or a string, depending on the type
 * specified in the \p platform_log_t structure. It can be configured to log
 * to different ITM channels (up to 64).
 *
 * @pre All strings passed to this function must be zero-terminated.
 * @pre MCU must be configured to run at 32MhZ.
 *
 * @param log Pointer to a \p platform_log_t structure containing the data to
 *            log and its type.
 */
void platform_log(const struct platform_log_t *log);
