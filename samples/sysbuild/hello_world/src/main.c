/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/printk.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/counter.h>
#include <hal/nrf_gpio.h>

void pin_test(void)
{
	uint32_t t1,t2;

	printk("test start\n");

	t1 = k_cycle_get_32();
	k_busy_wait(1000);
	t1 = k_cycle_get_32() - t1;

	k_msleep(10);

	t2 = k_cycle_get_32();
	k_busy_wait(1000);
	t2 = k_cycle_get_32() - t2;

	printk("t1:%d t2:%d\n", t1, t2);
};

int main(void)
{
	pin_test();

	printk("Hello world from %s\n", CONFIG_BOARD_TARGET);

	return 0;
}
