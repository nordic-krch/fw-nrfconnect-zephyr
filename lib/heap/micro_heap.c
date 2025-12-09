/* Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/micro_heap.h>

void micro_heap_init(struct micro_heap *heap, struct micro_heap_config *config)
{
	heap->bitarray.num_bits = config->blk_cnt;
	heap->bitarray.num_bundles = DIV_ROUND_UP(config->blk_cnt, 32);
	heap->bitarray.bundles = config->mask;
	heap->ptr = (uintptr_t)config->mem;
	heap->blk_size = config->mem_size / config->blk_cnt;
	heap->tail_mask = config->tail_mask;
}

static void tail_mask_set(atomic_t *tail_mask, size_t mask_size, size_t num_bits, size_t off)
{
	size_t tail_bits = num_bits - 1;
	size_t tail_off = off + 1;

	if (tail_bits == 0) {
		return;
	}

	if (mask_size == 1) {
		atomic_or(tail_mask, BIT_MASK(tail_bits) << tail_off);
		return;
	}

	size_t idx = tail_off / 32;
	atomic_t *t_mask = &tail_mask[idx];

	tail_off = tail_off % 32;
	while (tail_bits > 0) {
		uint32_t bits = MIN(32 - tail_off, tail_bits);
		uint32_t mask = (bits == 32) ? UINT32_MAX : (BIT_MASK(bits) << tail_off);

		atomic_or(t_mask, mask);
		t_mask++;
		tail_off = 0;
		tail_bits -= bits;
	}
}

static uint32_t num_bits_get(atomic_t *tail_mask, size_t mask_size, size_t off)
{
	uint32_t num_bits = 1;
	size_t tail_off = off + 1;
	size_t idx = tail_off / 32;
	atomic_t *t_mask = &tail_mask[idx];

	tail_off = tail_off % 32;
	do {
		uint32_t mask = (uint32_t)*t_mask >> tail_off;

		if (mask == UINT32_MAX) {
			num_bits += 32;
			atomic_set(t_mask, 0);
		} else {
			uint32_t bits = __builtin_ctz(~mask);

			if (bits == 0) {
				break;
			}

			num_bits += bits;
			atomic_and(t_mask, ~(BIT_MASK(bits) << tail_off));

			if (bits + tail_off < 32) {
				break;
			}

			tail_off = 0;
		}

		t_mask++;
	} while ((mask_size > 1) && (t_mask != &tail_mask[mask_size]));

	return num_bits;
}

void *micro_heap_alloc(struct micro_heap *heap, size_t length)
{
	size_t num_bits, off;
	int rv;

	num_bits = DIV_ROUND_UP(length, heap->blk_size);
	rv = sys_bitarray_alloc(&heap->bitarray, num_bits, &off);
	if (rv < 0) {
		return NULL;
	}

	tail_mask_set(heap->tail_mask, heap->bitarray.num_bundles, num_bits, off);

#ifdef CONFIG_DMM_STATS
	k_spinlock_key_t key;

	key = k_spin_lock(&dh->lock);
	dh->curr_use += num_bits;
	dh->max_use = MAX(dh->max_use, dh->curr_use);
	k_spin_unlock(&dh->lock, key);
#endif

	return (void *)(heap->ptr + heap->blk_size * off);
}

void micro_heap_free(struct micro_heap *heap, void *buffer)
{
	size_t off = ((uintptr_t)buffer - heap->ptr) / heap->blk_size;
	size_t num_bits = num_bits_get(heap->tail_mask, heap->bitarray.num_bundles, off);
	int rv;

#ifdef CONFIG_DMM_STATS
	atomic_sub(&dh->curr_use, num_bits);
#endif
	rv = sys_bitarray_free(&heap->bitarray, num_bits, off);
	(void)rv;
	__ASSERT_NO_MSG(rv == 0);
}
