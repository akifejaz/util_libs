/*
 * Copyright 2025, 10xEngineers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <platsupport/mach/timer.h>
#include <stdint.h>
#include <utils/util.h>

#define MAX_TIMEOUT_NS (SPACEMIT_TIMER_MAX_TICKS * (NS_IN_S / SPACEMIT_TIMER_TICKS_PER_SECOND))

static void spacemit_timer_enable_clocks(spacemit_timer_t *timer)
{
    volatile uint32_t *base = (volatile uint32_t *)((uint8_t *)timer->vaddr + 0x1000); // APB_CLK_BASE
	uint8_t fnclksel = K1_TCCR_CS0_VALUE;
    uint32_t offset  = APBC_TIMERx_CLK_RST_OFFSET;
    uint32_t read_value = 0x0;

    read_value |= (1 << 0);
	read_value |= (1 << 1);
	read_value &= ~(1 << 2);

    uint32_t clk_select_shift = 0x4;
    read_value &= ~(0x7u << clk_select_shift);    // clear bits
    read_value |= ((fnclksel & 0x7u) << clk_select_shift);

	*(base + offset/4) = read_value;
}

void spacemit_timer_enable(spacemit_timer_t *timer)
{
    assert(timer);
    assert(timer->regs);
    assert(timer->timer_n < SPACEMIT_NUM_TIMERS);

    uint8_t n = timer->timer_n;
    timer->regs->tmr_cnt_en  |= (1u << timer->timer_n);
}


void spacemit_timer_disable(spacemit_timer_t *timer)
{
    assert(timer);
	assert(timer->regs);

	timer->regs->tmr_cnt_en  &= ~(1u << timer->timer_n);
}


void spacemit_timer_handle_irq(spacemit_timer_t *timer)
{
    assert(timer);
	assert(timer->regs);

	uint8_t n = timer->timer_n;
	timer->value_h += 1;

	/* Clear the interrupt */
	uint32_t *int_clear = (uint32_t *)((uint8_t *)timer->vaddr + K1_TICLR_OFS_CAL(n));
	*(int_clear) |= (1u << n);    // using match register as timer number (E.g. Timer0 uses Match0, Timer1 uses Match1, etc)

}

uint64_t spacemit_timer_get_time(spacemit_timer_t *timer)
{
    assert(timer);
	assert(timer->regs);

	uint8_t n = timer->timer_n;
	/* the timer value counts down from the load value */
	uint64_t value_l = (uint64_t)(SPACEMIT_TIMER_MAX_TICKS - (*(uint32_t *)((uint8_t *)timer->vaddr + K1_TCNT_OFS_CAL(n))));
	uint64_t value_h = (uint64_t)timer->value_h;

	/* Include unhandled interrupt in value_h */
	uint32_t *status = (uint32_t *)((uint8_t *)timer->vaddr + K1_TSR_OFS_CAL(n));
	if ((*status) & (1u << n)) {
		value_h += 1;
	}

	uint64_t value_ticks = (value_h << 32) | value_l;

	/* convert from ticks to nanoseconds */
	uint64_t value_whole_seconds = value_ticks / SPACEMIT_TIMER_TICKS_PER_SECOND;
	uint64_t value_subsecond_ticks = value_ticks % SPACEMIT_TIMER_TICKS_PER_SECOND;
	uint64_t value_subsecond_ns =
		(value_subsecond_ticks * NS_IN_S) / SPACEMIT_TIMER_TICKS_PER_SECOND;
	uint64_t value_ns = value_whole_seconds * NS_IN_S + value_subsecond_ns;

	return value_ns;
}

void spacemit_timer_reset(spacemit_timer_t *timer)
{
    assert(timer);
	assert(timer->regs);

	spacemit_timer_disable(timer);
	timer->value_h = 0;
	uint8_t n      = timer->timer_n;

	/* Reset match register */
	uint32_t *timer_match = (uint32_t *)((uint8_t *)timer->vaddr + K1_TMR_OFS_CAL(n, n));
	*(timer_match ) = (uint32_t)(SPACEMIT_TIMER_MAX_TICKS);
}

int spacemit_timer_set_timeout(spacemit_timer_t *timer, uint64_t ns, bool is_periodic)
{
    spacemit_timer_disable(timer);
	timer->value_h = 0;
	uint8_t n = timer->timer_n;

	if (is_periodic) {
		timer->regs->tmr_mode &= ~(1u << n);      /* Periodic mode */
	} else {
		timer->regs->tmr_mode |= (1u << n);       /* Free-run mode */
	}

	/* convert from nanoseconds to ticks */
	uint64_t ticks_whole_seconds = (ns / NS_IN_S) * SPACEMIT_TIMER_TICKS_PER_SECOND;
	uint64_t ticks_remainder = (ns % NS_IN_S) * SPACEMIT_TIMER_TICKS_PER_SECOND / NS_IN_S;
	uint64_t num_ticks = ticks_whole_seconds + ticks_remainder;

	if (num_ticks > SPACEMIT_TIMER_MAX_TICKS) {
		ZF_LOGE("Requested timeout of %"PRIu64" ns exceeds hardware limit of %"PRIu64" ns",
				ns,
				MAX_TIMEOUT_NS);
		return -EINVAL;
	}

	/* Set the match register */
	uint32_t *timer_match = (uint32_t *)((uint8_t *)timer->vaddr + K1_TMR_OFS_CAL(n, n));
	*(timer_match )       = (uint32_t)(num_ticks);

	/* Enable interrupt */
	uint32_t *int_enable = (uint32_t *)((uint8_t *)timer->vaddr + K1_TIER_OFS_CAL(n));
	*(int_enable)        |= (1u << n);

	spacemit_timer_enable(timer);

	return 0;
}


void spacemit_timer_disable_all(void *vaddr)
{
    assert(vaddr);
	volatile spacemit_timer_regs_t *regs = vaddr;
	regs->tmr_cnt_en     &= ~(0x7u);

}

void spacemit_timer_init(spacemit_timer_t *timer, void *vaddr, size_t n)
{
    assert(timer);
    assert(vaddr);
    assert(n < SPACEMIT_NUM_TIMERS);

	timer->vaddr = (uint32_t *)vaddr;

    spacemit_timer_enable_clocks(timer);
    timer->regs      = vaddr;  /* all timers share same base addr */
    timer->timer_n   = n;
    timer->value_h   = 0;
    timer->regs->tmr_cnt_en     &= ~(1u << n);
    timer->regs->tmr_mode       |= (1u << n);       /* Free-run mode */

    uint32_t clock_ctrl          = timer->regs->tmr_clock_ctrl;
    clock_ctrl                   &= ~(3U << (n * 2));      /* Clear existing bits */
    clock_ctrl                   |= (0x0 << (n * 2));      /* Set to fast clock (12.8 MHz) */
    timer->regs->tmr_clock_ctrl  = clock_ctrl;

	/* Set the match register */
	uint32_t *timer_match = (uint32_t *)((uint8_t *)timer->vaddr + K1_TMR_OFS_CAL(n, n));
	*(timer_match )       = (uint32_t)(SPACEMIT_TIMER_MAX_TICKS);

	/* Enable interrupt */
	uint32_t *int_enable = (uint32_t *)((uint8_t *)timer->vaddr + K1_TIER_OFS_CAL(n));
	*(int_enable)        |= (1u << n);
}
