#include "dcds/heap.h"

#include <stdlib.h>

#include "internal.h"

void dcds_heap_init(dcds_heap *h, dcds_cmp_fn cmp, dcds_free_fn free_fn)
{
    h->items = NULL;
    h->len = 0;
    h->cap = 0;
    h->cmp = cmp;
    h->free_fn = free_fn;
}

void dcds_heap_clear(dcds_heap *h)
{
    if (h->free_fn != NULL) {
        for (size_t i = 0; i < h->len; i++) {
            h->free_fn(h->items[i]);
        }
    }
    free(h->items);
    h->items = NULL;
    h->len = 0;
    h->cap = 0;
}

static void sift_up(dcds_heap *h, size_t i)
{
    void *item = h->items[i];
    while (i > 0) {
        size_t parent = (i - 1) / 2;
        if (h->cmp(item, h->items[parent]) >= 0) {
            break;
        }
        h->items[i] = h->items[parent];
        i = parent;
    }
    h->items[i] = item;
}

static void sift_down(dcds_heap *h, size_t i)
{
    void *item = h->items[i];
    for (;;) {
        size_t child = 2 * i + 1;
        if (child >= h->len) {
            break;
        }
        if (child + 1 < h->len && h->cmp(h->items[child + 1], h->items[child]) < 0) {
            child++;
        }
        if (h->cmp(h->items[child], item) >= 0) {
            break;
        }
        h->items[i] = h->items[child];
        i = child;
    }
    h->items[i] = item;
}

dcds_status dcds_heap_push(dcds_heap *h, void *item)
{
    if (h->len == h->cap) {
        size_t cap = dcds_next_cap(h->cap);
        if (cap == 0) {
            return DCDS_ENOMEM;
        }
        void **items = realloc(h->items, cap * sizeof *items);
        if (items == NULL) {
            return DCDS_ENOMEM;
        }
        h->items = items;
        h->cap = cap;
    }
    h->items[h->len] = item;
    sift_up(h, h->len);
    h->len++;
    return DCDS_OK;
}

dcds_status dcds_heap_pop(dcds_heap *h, void **out)
{
    if (h->len == 0) {
        return DCDS_ERANGE;
    }
    void *top = h->items[0];
    h->len--;
    if (h->len > 0) {
        h->items[0] = h->items[h->len];
        sift_down(h, 0);
    }
    dcds_give_or_free(out, top, h->free_fn);
    return DCDS_OK;
}

void *dcds_heap_peek(const dcds_heap *h)
{
    return h->len == 0 ? NULL : h->items[0];
}
