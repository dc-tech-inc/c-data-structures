/**
 * @file common.h
 * @brief Types and conventions shared by every dcds container.
 *
 * Ownership rule (the same in every container):
 *  - A container created with a non-NULL @ref dcds_free_fn *owns* the items it
 *    stores. Clearing the container, overwriting an item or removing an item
 *    with no output pointer releases the item through that function.
 *  - Functions that hand an item back through an `out` pointer (pop, remove)
 *    transfer ownership to the caller: the free function is not called.
 *  - With a NULL free function the container only borrows the pointers.
 *
 * Containers store `void *`. They never dereference or copy an item.
 */
#ifndef DCDS_COMMON_H
#define DCDS_COMMON_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Result of a fallible operation. */
typedef enum dcds_status {
    DCDS_OK = 0,        /**< success */
    DCDS_ENOMEM = 1,    /**< allocation failed or size arithmetic overflowed; container unchanged */
    DCDS_ENOTFOUND = 2, /**< key or node is not in the container */
    DCDS_ERANGE = 3     /**< index out of range, or container is empty */
} dcds_status;

/** Three-way comparison: negative, zero or positive like `strcmp`. */
typedef int (*dcds_cmp_fn)(const void *a, const void *b);

/** Releases one stored item. */
typedef void (*dcds_free_fn)(void *item);

/** @return a constant, human readable name for @p s ("ok", "out of memory", ...). */
const char *dcds_status_str(dcds_status s);

#ifdef __cplusplus
}
#endif

#endif /* DCDS_COMMON_H */
