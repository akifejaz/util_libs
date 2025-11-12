/*
 * Copyright 2025, 10xEngineers 
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once
#include <autoconf.h>

/* The SpacemiT K1 SoC contains one PXA compatible UART. */

enum chardev_id {
    UART0,
    PS_SERIAL_DEFAULT = UART0
};

#define UART0_PADDR 0xd4017000
#define UART0_IRQ 42

#define DEFAULT_SERIAL_PADDR UART0_PADDR
#define DEFAULT_SERIAL_INTERRUPT UART0_IRQ
