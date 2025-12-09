/* Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_MICRO_HEAP_H_
#define ZEPHYR_INCLUDE_SYS_MICRO_HEAP_H_

#include <zephyr/sys/bitarray.h>
#include <zephyr/sys/atomic.h>

struct micro_heap {
	atomic_t *tail_mask;
	uintptr_t ptr;
	size_t blk_size;
	sys_bitarray_t bitarray;
#ifdef CONFIG_MICRO_HEAP_STATS
	atomic_t curr_use;
	uint32_t max_use;
	struct k_spinlock lock;
#endif
};

struct micro_heap_config {
	uint32_t *mask;
	atomic_t *tail_mask;
	uint32_t blk_cnt;
	size_t mem_size;
	void *mem;
};

#define MICROHEAP_DEFINE(name, buffer, size, blk_cnt) \
	static uint32_t name##_mask[DIV_ROUND_UP(blk_cnt, 32)]; \
	static atomic_t name##_tail_mask[DIV_ROUND_UP(blk_cnt, 32)]; \
	static struct micro_heap name = { \
		.tail_mask = name##_tail_mask, \
		.ptr = (uintptr_t)buffer, \
		.blk_size = size / blk_cnt, \
		.bitarray = { \
			.num_bits = blk_cnt, \
			.num_bundles = DIV_ROUND_UP(blk_cnt, 32), \
			.bundles = name##_mask \
		} \
	}

void micro_heap_init(struct micro_heap *heap, struct micro_heap_config *config);
void *micro_heap_alloc(struct micro_heap *heap, size_t length);
void micro_heap_free(struct micro_heap *heap, void *buffer);

#endif /* ZEPHYR_INCLUDE_SYS_MICRO_HEAP_H_ */
