/**
 * @file vec.h
 * @brief Dynamic array of `void *` with geometric growth.
 *
 * Complexity (n = length): get/set O(1), push amortised O(1), pop O(1),
 * insert/remove at index O(n), reserve O(n) when it reallocates.
 * Capacity doubles; pointers returned by dcds_vec_data() are invalidated by any
 * call that can grow the vector.
 */
#ifndef DCDS_VEC_H
#define DCDS_VEC_H

#include <stddef.h>

#include "dcds/common.h"

#ifdef __cplusplus
extern "C" {
#endif

/** A vector. Initialise with dcds_vec_init(), release with dcds_vec_clear(). */
typedef struct dcds_vec {
    void **items;         /**< contiguous storage, @c len valid entries */
    size_t len;           /**< number of stored items */
    size_t cap;           /**< allocated entries */
    dcds_free_fn free_fn; /**< owns items when non-NULL (see common.h) */
} dcds_vec;

/** Make an empty vector. No allocation happens until the first push/reserve. @p free_fn may be NULL. */
void dcds_vec_init(dcds_vec *v, dcds_free_fn free_fn);

/** Free the storage (and every item when owned). The vector is empty afterwards and reusable. */
void dcds_vec_clear(dcds_vec *v);

/** Ensure room for at least @p min_cap items. @return DCDS_OK or DCDS_ENOMEM (vector unchanged). */
dcds_status dcds_vec_reserve(dcds_vec *v, size_t min_cap);

/** Append. @return DCDS_OK or DCDS_ENOMEM (vector unchanged). Amortised O(1). */
dcds_status dcds_vec_push(dcds_vec *v, void *item);

/** Remove the last item. @p out may be NULL (item is then released). @return DCDS_OK or DCDS_ERANGE if empty. O(1). */
dcds_status dcds_vec_pop(dcds_vec *v, void **out);

/** @return the item at @p index, or NULL when out of range (a stored NULL is indistinguishable; check len first). O(1). */
void *dcds_vec_get(const dcds_vec *v, size_t index);

/** Replace the item at @p index; the previous item is released if owned. @return DCDS_OK or DCDS_ERANGE. O(1). */
dcds_status dcds_vec_set(dcds_vec *v, size_t index, void *item);

/** Insert before @p index (@p index == len appends). @return DCDS_OK, DCDS_ERANGE or DCDS_ENOMEM. O(n). */
dcds_status dcds_vec_insert(dcds_vec *v, size_t index, void *item);

/** Remove the item at @p index, shifting the tail left. @p out may be NULL (item is then released). @return DCDS_OK or DCDS_ERANGE. O(n). */
dcds_status dcds_vec_remove(dcds_vec *v, size_t index, void **out);

#ifdef __cplusplus
}
#endif

#endif /* DCDS_VEC_H */
