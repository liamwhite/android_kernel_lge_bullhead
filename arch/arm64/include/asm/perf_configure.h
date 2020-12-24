/*
 * PMU configuration routines
 *
 * Copyright (C) 2020 Liam White
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _ARM64_PERF_CONFIGURE_H
#define _ARM64_PERF_CONFIGURE_H

#include <linux/sched.h>

#define ARMV8_PMU_COUNTER_MASK			31
#define ARMV8_PMU_MAX_COUNTERS			31
#define ARMV8_PMU_MAX_USER_COUNTERS 		30

#define ARMV8_PMU_ENABLE_EXPORT			(1 << 4)
#define ARMV8_PMU_ENABLE_DIVIDER		(1 << 3)
#define ARMV8_PMU_RESET_CYCLE_COUNTER		(1 << 2)
#define ARMV8_PMU_RESET_EVENT_COUNTERS		(1 << 1)
#define ARMV8_PMU_ENABLE_EVENT_COUNTERS		(1 << 0)

#define ARMV8_PMU_CYCLE_COUNTER			(1 << 31)
#define ARMV8_PMU_ALL_COUNTERS			((1LU << 31) - 1)

#define ARMV8_PMU_EVENT_COUNTER_READ_EL0	(1 << 3)
#define ARMV8_PMU_CYCLE_COUNTER_READ_EL0	(1 << 2)
#define ARMV8_PMU_NO_TRAP_PMSWINC_EL0		(1 << 1)
#define ARMV8_PMU_NO_TRAP_ALL_EL0		(1 << 0)

static inline u64 armv8pmu_read_counter(u64 index)
{
	u64 ret;

	index &= ARMV8_PMU_COUNTER_MASK;

	// 1. Selects the current event counter in the PMU selection register
	// 2. Writes the selected event type register to the local variable

	asm volatile (
		"msr pmselr_el0, %1\n\t"
		"mrs %0, pmxevcntr_el0\n\t"
		: "=r"(ret)
		: "r"(index)
	);

	return ret;
}

static inline u64 armv8pmu_read_cycles(void)
{
	u64 ret;

	// 1. Writes the cycle counter register to the local variable

	asm volatile (
		"mrs %0, pmccntr_el0\n\t"
		: "=r"(ret)
	);

	return ret;
}

static inline void armv8pmu_enable_counter(u64 index, u64 type)
{
	index &= ARMV8_PMU_COUNTER_MASK;

	// 1. Selects the current event counter in the PMU selection register
	// 2. Writes the selected event type register to the PMU

	asm volatile (
		"msr pmselr_el0, %0\n\t"
		"msr pmxevtyper_el0, %1\n\t"
		:: "r"(index), "r"(type)
	);
}

static inline void armv8pmu_init_counters(struct task_struct *tsk)
{
	int idx;

	// 1. Enable access to performance counters
	// 2. Disable all counter overflow interrupts
	// 3. Reset all counters to 0, enable event exporting (? why)
	// 4. Enable all counters
	// 5. Clear all overflows

	asm volatile (
		"msr pmuserenr_el0, %0\n\t"
		"msr pmintenclr_el1, %2\n\t"
		"msr pmcr_el0, %1\n\t"
		"msr pmcntenset_el0, %2\n\t"
		"msr pmovsclr_el0, %2\n\t"
		::
		"r"(
			ARMV8_PMU_EVENT_COUNTER_READ_EL0 |
			ARMV8_PMU_CYCLE_COUNTER_READ_EL0 |
			ARMV8_PMU_NO_TRAP_PMSWINC_EL0 |
			ARMV8_PMU_NO_TRAP_ALL_EL0
		),
		"r"(
			ARMV8_PMU_ENABLE_EXPORT |
			ARMV8_PMU_RESET_CYCLE_COUNTER |
			ARMV8_PMU_RESET_EVENT_COUNTERS |
			ARMV8_PMU_ENABLE_EVENT_COUNTERS
		),
		"r"(
			ARMV8_PMU_CYCLE_COUNTER |
			ARMV8_PMU_ALL_COUNTERS
		)
	);

	// Enable counters for each tracked event type
	for (idx = 0; idx < ARMV8_PMU_MAX_USER_COUNTERS; ++idx) {
		if (tsk->armv8pmu_counterset[idx] > 0) {
			armv8pmu_enable_counter(idx, tsk->armv8pmu_counterset[idx]);
		}
	}
}

#endif
