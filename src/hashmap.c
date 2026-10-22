#include "dcds/hashmap.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "internal.h"

/* Address of this object marks a deleted slot; it is never dereferenced. */
static char tombstone_marker;
#define TOMBSTONE (&tombstone_marker)

#define MIN_CAP 8u

/* FNV-1a, 64 bit. https://datatracker.ietf.org/doc/html/draft-eastlake-fnv-17 */
static uint64_t fnv1a(const char *s)
{
    uint64_t h = UINT64_C(14695981039346656037);
    for (const unsigned char *p = (const unsigned char *)s; *p != '\0'; p++) {
        h ^= *p;
        h *= UINT64_C(1099511628211);
    }
    return h;
}

void dcds_hashmap_init(dcds_hashmap *m, dcds_free_fn free_fn)
{
    m->slots = NULL;
    m->cap = 0;
    m->len = 0;
    m->used = 0;
    m->free_fn = free_fn;
}

void dcds_hashmap_clear(dcds_hashmap *m)
{
    for (size_t i = 0; i < m->cap; i++) {
        char *k = m->slots[i].key;
        if (k != NULL && k != TOMBSTONE) {
            if (m->free_fn != NULL) {
                m->free_fn(m->slots[i].value);
            }
            free(k);
        }
    }
    free(m->slots);
    m->slots = NULL;
    m->cap = 0;
    m->len = 0;
    m->used = 0;
}

/* Index of the slot holding key, or SIZE_MAX. */
static size_t find_slot(const dcds_hashmap *m, const char *key)
{
    if (m->cap == 0) {
        return SIZE_MAX;
    }
    size_t mask = m->cap - 1;
    size_t i = (size_t)fnv1a(key) & mask;
    for (size_t probes = 0; probes < m->cap; probes++) {
        const char *k = m->slots[i].key;
        if (k == NULL) {
            return SIZE_MAX;
        }
        if (k != TOMBSTONE && strcmp(k, key) == 0) {
            return i;
        }
        i = (i + 1) & mask;
    }
    return SIZE_MAX;
}

/* Rebuild into new_cap slots (power of two), dropping tombstones. Keys move, they are not copied. */
static dcds_status rehash(dcds_hashmap *m, size_t new_cap)
{
    if (new_cap > SIZE_MAX / sizeof(dcds_hashmap_slot)) {
        return DCDS_ENOMEM;
    }
    dcds_hashmap_slot *slots = calloc(new_cap, sizeof *slots);
    if (slots == NULL) {
        return DCDS_ENOMEM;
    }
    size_t mask = new_cap - 1;
    for (size_t i = 0; i < m->cap; i++) {
        const char *k = m->slots[i].key;
        if (k == NULL || k == TOMBSTONE) {
            continue;
        }
        size_t j = (size_t)fnv1a(k) & mask;
        while (slots[j].key != NULL) {
            j = (j + 1) & mask;
        }
        slots[j] = m->slots[i];
    }
    free(m->slots);
    m->slots = slots;
    m->cap = new_cap;
    m->used = m->len;
    return DCDS_OK;
}

dcds_status dcds_hashmap_put(dcds_hashmap *m, const char *key, void *value)
{
    size_t at = find_slot(m, key);
    if (at != SIZE_MAX) {
        if (m->free_fn != NULL) {
            m->free_fn(m->slots[at].value);
        }
        m->slots[at].value = value;
        return DCDS_OK;
    }

    /* Keep used (live + tombstones) at or below 75 % so probe chains stay short and always end. */
    if ((m->used + 1) > m->cap - m->cap / 4) {
        size_t new_cap = m->cap == 0 ? MIN_CAP : m->cap;
        /* After the rebuild, live entries (plus the new one) must fill at most half the table. */
        while ((m->len + 1) > new_cap / 2) {
            if (new_cap > SIZE_MAX / 2) {
                return DCDS_ENOMEM;
            }
            new_cap *= 2;
        }
        dcds_status s = rehash(m, new_cap);
        if (s != DCDS_OK) {
            return s;
        }
    }

    size_t klen = strlen(key) + 1;
    char *copy = malloc(klen);
    if (copy == NULL) {
        return DCDS_ENOMEM;
    }
    memcpy(copy, key, klen);

    size_t mask = m->cap - 1;
    size_t i = (size_t)fnv1a(key) & mask;
    while (m->slots[i].key != NULL && m->slots[i].key != TOMBSTONE) {
        i = (i + 1) & mask;
    }
    if (m->slots[i].key == NULL) {
        m->used++; /* a reused tombstone is already counted in used */
    }
    m->slots[i].key = copy;
    m->slots[i].value = value;
    m->len++;
    return DCDS_OK;
}

void *dcds_hashmap_get(const dcds_hashmap *m, const char *key)
{
    size_t at = find_slot(m, key);
    return at == SIZE_MAX ? NULL : m->slots[at].value;
}

int dcds_hashmap_contains(const dcds_hashmap *m, const char *key)
{
    return find_slot(m, key) != SIZE_MAX;
}

dcds_status dcds_hashmap_remove(dcds_hashmap *m, const char *key, void **out)
{
    size_t at = find_slot(m, key);
    if (at == SIZE_MAX) {
        return DCDS_ENOTFOUND;
    }
    void *value = m->slots[at].value;
    free(m->slots[at].key);
    m->slots[at].key = TOMBSTONE;
    m->slots[at].value = NULL;
    m->len--;
    dcds_give_or_free(out, value, m->free_fn);
    return DCDS_OK;
}

void dcds_hashmap_foreach(const dcds_hashmap *m, dcds_hashmap_visit_fn fn, void *ctx)
{
    for (size_t i = 0; i < m->cap; i++) {
        char *k = m->slots[i].key;
        if (k != NULL && k != TOMBSTONE) {
            fn(k, m->slots[i].value, ctx);
        }
    }
}
