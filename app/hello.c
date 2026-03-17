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

int main(void) {
    size_t  i       = 0;
    uint8_t buf[32] = { 0 };

    platform_init(PLATFORM_CLOCK_USERSPACE);
    printf("Board initialized...\n");
    printf("CPU Clock: %u MHz.\n", SystemCoreClock / 1000000);

    // Check if cycle counting works
    uint64_t t = platform_cpu_cyclecount();
    i++;
    t = platform_cpu_cyclecount() - t;
    printf("Cycle count: %llu.\n", t);

    // Check getting a random number
    platform_get_random(buf, sizeof(buf));
    printf("RNG: ");
    for (i = 0; i < sizeof(buf); i++) {
        printf("%02x", buf[i]);
    }
    printf("\n");
    return 0;
}
