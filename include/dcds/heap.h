/**
 * @file heap.h
 * @brief Binary min-heap (priority queue) of `void *`.
 *
 * The item that compares *smallest* under the comparator is popped first; pass
 * a reversed comparator for a max-heap. Stored in an array: parent of i is
 * (i-1)/2.
 *
 * Complexity (n = length): push O(log n) (amortised, includes growth), pop
 * O(log n), peek O(1). The heap is not stable: equal items pop in unspecified order.
 */
#ifndef DCDS_HEAP_H
#define DCDS_HEAP_H

#include <stddef.h>

#include "dcds/common.h"

#ifdef __cplusplus
extern "C" {
#endif

/** A heap. Initialise with dcds_heap_init(), release with dcds_heap_clear(). */
typedef struct dcds_heap {
    void **items;
    size_t len;
    size_t cap;
    dcds_cmp_fn cmp;      /**< ordering; must be a consistent total preorder */
    dcds_free_fn free_fn; /**< owns items when non-NULL (see common.h) */
} dcds_heap;

/** Make an empty heap ordered by @p cmp (required). @p free_fn may be NULL. */
void dcds_heap_init(dcds_heap *h, dcds_cmp_fn cmp, dcds_free_fn free_fn);

/** Free the storage (and every item when owned). The heap is empty afterwards and reusable. */
void dcds_heap_clear(dcds_heap *h);

/** Insert. @return DCDS_OK or DCDS_ENOMEM (heap unchanged). O(log n). */
dcds_status dcds_heap_push(dcds_heap *h, void *item);

/** Remove the smallest item. @p out may be NULL (item is then released). @return DCDS_OK or DCDS_ERANGE if empty. O(log n). */
dcds_status dcds_heap_pop(dcds_heap *h, void **out);

/** @return the smallest item without removing it, or NULL if empty. O(1). */
void *dcds_heap_peek(const dcds_heap *h);

#ifdef __cplusplus
}
#endif

#endif /* DCDS_HEAP_H */
