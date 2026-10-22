/* Private helpers shared by the implementation files. Not installed. */
#ifndef DCDS_INTERNAL_H
#define DCDS_INTERNAL_H

#include <stdint.h>

#include "dcds/common.h"

/* Hand an item back to the caller (out != NULL) or release it (out == NULL). */
static inline void dcds_give_or_free(void **out, void *item, dcds_free_fn free_fn)
{
    if (out != NULL) {
        *out = item;
    } else if (free_fn != NULL) {
        free_fn(item);
    }
}

/* Next capacity for a pointer array: doubles, starting at 8. Returns 0 on overflow. */
static inline size_t dcds_next_cap(size_t cap)
{
    size_t next = (cap == 0) ? 8 : cap * 2;
    if (cap != 0 && (cap > SIZE_MAX / 2 || next > SIZE_MAX / sizeof(void *))) {
        return 0;
    }
    return next;
}

#endif /* DCDS_INTERNAL_H */
