/*
 * Copyright (C) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <hal/nrf_vpr_csr.h>

static int vpr_init(void)
{
	/* RT peripherals for VPR all share one enable.
	 * To prevent redundant calls, do it here once.
	 */
	nrf_vpr_csr_rtperiph_enable_set(true);

	/* Set deep sleep as the default sleep method. */
	csr_write(VPRCSR_NORDIC_VPRNORDICSLEEPCTRL,
		  VPRCSR_NORDIC_VPRNORDICSLEEPCTRL_SLEEPSTATE_DEEPSLEEP);
	nrf_barrier_w();

	return 0;
}

SYS_INIT(vpr_init, PRE_KERNEL_1, 0);
