/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <hal/nrf_timer.h>
#include <zephyr/drivers/misc/ppi/nrfx_dppi.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app);

static uint32_t dppi_test(NRF_TIMER_Type *tmr0, NRF_TIMER_Type *tmr1, bool free_handle)
{
	uint32_t eep = (uint32_t)&tmr0->EVENTS_COMPARE[0];
	uint32_t tep = (uint32_t)&tmr1->TASKS_START;
	int err;
	uint32_t handle;

	/* Allocate DPPI connection. */
	err = nrf_dppi_conn_alloc(eep, tep, &handle);
	if (err < 0) {
		LOG_ERR("Failed to allocate DPPI connection.");
		return 0;
	}

	nrf_dppi_conn_ctrl(handle, true);

	tmr0->CC[0] = 100;
	tmr1->TASKS_CAPTURE[0] = 1;
	uint32_t cc0 = tmr1->CC[0];
	tmr0->TASKS_START=1;
	k_busy_wait(1000);

	tmr1->TASKS_CAPTURE[0] = 1;
	uint32_t cc1 = tmr1->CC[0];

	tmr0->TASKS_STOP=1;
	tmr0->TASKS_CLEAR=1;
	tmr1->TASKS_STOP=1;
	tmr1->TASKS_CLEAR=1;

	LOG_INF("PPI Test between timer %p and %p", tmr0, tmr1);
	if (cc1 != 0) {
		LOG_INF("PPI started second timer! CC before:%d, after:%d", cc0, cc1);
	} else {
		LOG_ERR("PPI connection did not work.");
	}

	nrf_dppi_conn_ctrl(handle, false);
	if (free_handle) {
		nrf_dppi_conn_free(eep, tep, handle);
	} else {
		nrf_dppi_ep_clear(eep);
		nrf_dppi_ep_clear(tep);
	}

	return handle;
}

int main(void)
{
	(void)dppi_test(NRF_TIMER130, NRF_TIMER131, true);

	(void)dppi_test(NRF_TIMER131, NRF_TIMER130, true);

	uint32_t handle = dppi_test(NRF_TIMER131, NRF_TIMER130, false);

	dppi_test(NRF_TIMER131, NRF_TIMER133, true);

	dppi_test(NRF_TIMER130, NRF_TIMER120, true);

	nrf_dppi_domain_conn_free(handle);

	printf("Hello World! %s\n", CONFIG_BOARD_TARGET);

	return 0;
}
