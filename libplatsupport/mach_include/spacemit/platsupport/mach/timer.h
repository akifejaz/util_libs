/*
 * Copyright 2025, 10xEngineers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define SPACEMIT_TIMER_BASE      0xD4014000
#define SPACEMIT_TIMER_SIZE      0x2000u
#define SPACEMIT_TIMER0_IRQ      0x17u
#define SPACEMIT_TIMER1_IRQ      0x1Au
#define SLOW_CLOCK false

#if SLOW_CLOCK
#define SPACEMIT_TIMER_TICKS_PER_SECOND  32768u
#define K1_TCCR_CS0_VALUE                0x1u   /* slow clock */
#else
#define SPACEMIT_TIMER_TICKS_PER_SECOND  12800000u
#define K1_TCCR_CS0_VALUE                0x0u   /* fast clock */
#endif

#define SPACEMIT_NUM_TIMERS 3
#define SPACEMIT_TIMER_MAX_TICKS UINT32_MAX

#define APB_CLK_BASE                0xD4015000
#define APBC_TIMERx_CLK_RST_OFFSET  0x34u

#define SPACEMIT_TIMERS_TCER_OFFSET          0x0   /* Timer Count Enable Register (TCER)   */
#define SPACEMIT_TIMERS_TCMR_OFFSET          0x4   /* Timer Count Mode Register (TCMR)     */
#define SPACEMIT_TIMERS_TCRR_OFFSET          0x8   /* Timer Count Restart Register (TCRR)  */
#define SPACEMIT_TIMERS_TCCR_OFFSET          0xC   /* Timer Clock Control Register (TCCR)  */

/* Timer Match Register */
#define SPACEMIT_TIMERS_TMR_OFFSET  0x10u
#define K1_TMR_OFS_CAL(n, m)  (SPACEMIT_TIMERS_TMR_OFFSET + ((uint32_t)(n) * 0x10u) + ((uint32_t)(m) * 0x4u))

/* Timer Preload Value Register */
#define SPACEMIT_TIMERS_TPLVR_OFFSET       0x40u
#define K1_TPLVR_OFS_CAL(n)  (SPACEMIT_TIMERS_TPLVR_OFFSET + ((uint32_t)(n) * 0x4u))

/* Timer Preload Control Register */
#define SPACEMIT_TIMERS_TPLCR_OFFSET       0x50u
#define K1_TPLCR_OFS_CAL(n)  (SPACEMIT_TIMERS_TPLCR_OFFSET + ((uint32_t)(n) * 0x4u))
#define K1_TPLCR_ENCODE(mcs, crpd) \
    ( ((uint32_t)(mcs) & 0x3u) | ((uint32_t)(crpd) << 2) )

/* Timer Interrupt Enable Register */
#define SPACEMIT_TIMERS_IER_OFFSET        0x60u
#define K1_TIER_OFS_CAL(n)  (SPACEMIT_TIMERS_IER_OFFSET + ((uint32_t)(n) * 0x4u))

/* Timer Interrupt Clear Register */
#define SPACEMIT_TIMERS_ICR_OFFSET        0x70u
#define K1_TICLR_OFS_CAL(n)  (SPACEMIT_TIMERS_ICR_OFFSET + ((uint32_t)(n) * 0x4u))

/* Timer Status Register */
#define SPACEMIT_TIMERS_ISR_OFFSET        0x80u
#define K1_TSR_OFS_CAL(n)  (SPACEMIT_TIMERS_ISR_OFFSET + ((uint32_t)(n) * 0x4u))

/* Timer Count Register */
#define SPACEMIT_TIMERS_CNT_OFFSET        0x90u
#define K1_TCNT_OFS_CAL(n)  (SPACEMIT_TIMERS_CNT_OFFSET + ((uint32_t)(n) * 0x4u))

typedef struct {
    uint32_t tmr_cnt_en;
    uint32_t tmr_mode;
    uint32_t tmr_restart;
    uint32_t tmr_clock_ctrl;
} spacemit_timer_regs_t;

typedef struct {
    volatile spacemit_timer_regs_t *regs;
    uint8_t  timer_n;
    uint32_t value_h;
    uint32_t *vaddr;
} spacemit_timer_t;

void spacemit_timer_enable(spacemit_timer_t *timer);
void spacemit_timer_disable(spacemit_timer_t *timer);
void spacemit_timer_handle_irq(spacemit_timer_t *timer);
uint64_t spacemit_timer_get_time(spacemit_timer_t *timer);
void spacemit_timer_reset(spacemit_timer_t *timer);
int spacemit_timer_set_timeout(spacemit_timer_t *timer, uint64_t ns, bool is_periodic);
void spacemit_timer_disable_all(void *vaddr);
void spacemit_timer_init(spacemit_timer_t *timer, void *vaddr, uint64_t channel);
