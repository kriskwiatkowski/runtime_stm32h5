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
 */

#include <platform/platform.h>
#include <platform/printf.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

extern uint32_t SystemCoreClock;

void run_bench(void) {
    size_t i = 0;

    for (i = 0; i < 10; i++) {
        uint32_t t1 = (uint32_t)platform_cpu_cyclecount();
        for (size_t _ = 0; _ < 1000000; _++) {
            __asm volatile("nop");
        }
        uint32_t t2 = (uint32_t)platform_cpu_cyclecount();
        printf("Counters: %lu %lu, diff: %lu\n", t1, t2, t2 - t1);

        /*
        * | Instruction         | Cycles     |
        * |---------------------|------------|
        * | nop	                | 1          |
        * | increment _	        | 1          |
        * | cmp _ < 1_000_000	| 1          |
        * | branch (back to top)| 1          |
        * |   Total	            | 4          |
        * |   Expected diff     | ~4_000_000 |
        */
    }
}

void send_debug_trace(void) {
    char                 *string  = "The number is: ";
    struct platform_log_t log_str = { .channel = 1,
                                      .type    = PLATFORM_LOG_TYPE_STRING };
    log_str.a.str                 = string;

    struct platform_log_t log_u32 = {
        .a.data = 43, .channel = 0, .type = PLATFORM_LOG_TYPE_U32
    };

    platform_log(&log_str);
    platform_log(&log_u32);
}

int main(void) {
    size_t  i       = 0;
    uint8_t buf[32] = { 0 };

    platform_init(PLATFORM_CLOCK_USERSPACE);
    printf("Board initialized...\n");
    printf("CPU Clock: %u MHz.\n", SystemCoreClock / 1000000);

    // Check if cycle counting works
    uint32_t t = (uint32_t)platform_cpu_cyclecount();
    i++;
    t = platform_cpu_cyclecount() - t;
    printf("Cycle count: %lu.\n", t);

    // Check getting a random number
    platform_get_random(buf, sizeof(buf));
    printf("RNG: ");
    for (i = 0; i < sizeof(buf); i++) {
        printf("%02x", buf[i]);
    }
    printf("\n");

    printf("Running benchmark with cache enabled...\n");
    run_bench();

    // Disable cache and try do same again, to see the difference in cycle counts and performance.
    printf("Running benchmark with cache disabled...\n");
    struct platform_attr_t a = { .n = 1, .attr = { PLATFORM_CACHE_DISABLE } };
    platform_set_attr(&a);
    run_bench();
    send_debug_trace();

    printf("\n");
    return 0;
}
