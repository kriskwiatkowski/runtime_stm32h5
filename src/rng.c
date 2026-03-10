// SPDX-FileCopyrightText: 2026 AmongBytes, Ltd.
// SPDX-FileContributor: Kris Kwiatkowski <kris@amongbytes.com>
// SPDX-License-Identifier: LicenseRef-Proprietary

#include <stdint.h>

#include "stm32h5xx_hal.h"

/* xorshift32 state must be non-zero */
static uint32_t xs32_state = 0xA634716A;
// RNG handle.
static RNG_HandleTypeDef hrng;

void err_handler(void);

void setup_rng(uint32_t seed) { xs32_state = seed; }

uint32_t xorshift32_u32(void) {
    uint32_t x;

    x          = xs32_state;
    x          = x ^ (x << 13);
    x          = x ^ (x >> 17);
    x          = x ^ (x << 5);
    xs32_state = x;
    return x;
}

void platform_rng_init(void) {
    hrng.Instance                 = RNG;
    hrng.Init.ClockErrorDetection = RNG_CED_ENABLE;
    if (HAL_RNG_Init(&hrng) != HAL_OK) {
        err_handler();
    }
}

int platform_get_random(void *out, unsigned len) {

    union {
        unsigned char aschar[4];
        uint32_t      asint;
    } random;

    // Generate random seed.
    if (HAL_RNG_GenerateRandomNumber(&hrng, &random.asint) != HAL_OK) {
        return -1;  // RNG error
    }

    setup_rng(random.asint);

    uint8_t *obuf = (uint8_t *)out;
    while (len > 4) {
        random.asint = xorshift32_u32();
        *obuf++      = random.aschar[0];
        *obuf++      = random.aschar[1];
        *obuf++      = random.aschar[2];
        *obuf++      = random.aschar[3];
        len          -= 4;
    }

    if (len > 0) {
        for (random.asint = xorshift32_u32(); len > 0; --len) {
            *obuf++ = random.aschar[len - 1];
        }
    }

    return 0;
}
