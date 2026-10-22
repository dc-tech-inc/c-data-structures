#include "dcds/vec.h"

#include <stdlib.h>
#include <string.h>

#include "internal.h"

void dcds_vec_init(dcds_vec *v, dcds_free_fn free_fn)
{
    v->items = NULL;
    v->len = 0;
    v->cap = 0;
    v->free_fn = free_fn;
}

void dcds_vec_clear(dcds_vec *v)
{
    if (v->free_fn != NULL) {
        for (size_t i = 0; i < v->len; i++) {
            v->free_fn(v->items[i]);
        }
    }
    free(v->items);
    v->items = NULL;
    v->len = 0;
    v->cap = 0;
}

dcds_status dcds_vec_reserve(dcds_vec *v, size_t min_cap)
{
    if (min_cap <= v->cap) {
        return DCDS_OK;
    }
    size_t cap = v->cap;
    while (cap < min_cap) {
        cap = dcds_next_cap(cap);
        if (cap == 0) {
            return DCDS_ENOMEM;
        }
    }
    void **items = realloc(v->items, cap * sizeof *items);
    if (items == NULL) {
        return DCDS_ENOMEM;
    }
    v->items = items;
    v->cap = cap;
    return DCDS_OK;
}

dcds_status dcds_vec_push(dcds_vec *v, void *item)
{
    if (v->len == SIZE_MAX) {
        return DCDS_ENOMEM;
    }
    dcds_status s = dcds_vec_reserve(v, v->len + 1);
    if (s != DCDS_OK) {
        return s;
    }
    v->items[v->len++] = item;
    return DCDS_OK;
}

dcds_status dcds_vec_pop(dcds_vec *v, void **out)
{
    if (v->len == 0) {
        return DCDS_ERANGE;
    }
    v->len--;
    dcds_give_or_free(out, v->items[v->len], v->free_fn);
    return DCDS_OK;
}

void *dcds_vec_get(const dcds_vec *v, size_t index)
{
    return index < v->len ? v->items[index] : NULL;
}

dcds_status dcds_vec_set(dcds_vec *v, size_t index, void *item)
{
    if (index >= v->len) {
        return DCDS_ERANGE;
    }
    if (v->free_fn != NULL) {
        v->free_fn(v->items[index]);
    }
    v->items[index] = item;
    return DCDS_OK;
}

dcds_status dcds_vec_insert(dcds_vec *v, size_t index, void *item)
{
    if (index > v->len) {
        return DCDS_ERANGE;
    }
    if (v->len == SIZE_MAX) {
        return DCDS_ENOMEM;
    }
    dcds_status s = dcds_vec_reserve(v, v->len + 1);
    if (s != DCDS_OK) {
        return s;
    }
    memmove(&v->items[index + 1], &v->items[index], (v->len - index) * sizeof *v->items);
    v->items[index] = item;
    v->len++;
    return DCDS_OK;
}

dcds_status dcds_vec_remove(dcds_vec *v, size_t index, void **out)
{
    if (index >= v->len) {
        return DCDS_ERANGE;
    }
    void *item = v->items[index];
    memmove(&v->items[index], &v->items[index + 1], (v->len - index - 1) * sizeof *v->items);
    v->len--;
    dcds_give_or_free(out, item, v->free_fn);
    return DCDS_OK;
}
