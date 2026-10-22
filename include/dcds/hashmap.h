/**
 * @file hashmap.h
 * @brief Hash map from NUL-terminated string keys to `void *` values.
 *
 * Open addressing with linear probing, power-of-two capacity, FNV-1a (64 bit)
 * hashing and tombstones for deletion. Keys are copied; values are not.
 * The table is rebuilt (dropping tombstones) before the number of used slots
 * (live + tombstones) would exceed 75 % of the capacity.
 *
 * Complexity: put/get/remove expected O(1), worst case O(n); a rebuild is O(n)
 * and amortised away. FNV-1a is *not* a keyed hash: an attacker who controls
 * keys can force collisions. Do not expose this map to untrusted keys where
 * hash flooding matters (see README, Limitations).
 */
#ifndef DCDS_HASHMAP_H
#define DCDS_HASHMAP_H

#include <stddef.h>

#include "dcds/common.h"

#ifdef __cplusplus
extern "C" {
#endif

/** One table slot. Internal layout, exposed so the struct can live on the stack. */
typedef struct dcds_hashmap_slot {
    char *key; /**< NULL = never used, a private sentinel = deleted, otherwise an owned copy */
    void *value;
} dcds_hashmap_slot;

/** A map. Initialise with dcds_hashmap_init(), release with dcds_hashmap_clear(). */
typedef struct dcds_hashmap {
    dcds_hashmap_slot *slots;
    size_t cap;           /**< slot count, 0 or a power of two */
    size_t len;           /**< live entries */
    size_t used;          /**< live entries + tombstones */
    dcds_free_fn free_fn; /**< owns values when non-NULL (see common.h) */
} dcds_hashmap;

/** Callback for dcds_hashmap_foreach(). Must not modify the map. */
typedef void (*dcds_hashmap_visit_fn)(const char *key, void *value, void *ctx);

/** Make an empty map; no allocation until the first put. @p free_fn (for values) may be NULL. */
void dcds_hashmap_init(dcds_hashmap *m, dcds_free_fn free_fn);

/** Free every key copy, the table and (when owned) every value. The map is empty afterwards and reusable. */
void dcds_hashmap_clear(dcds_hashmap *m);

/**
 * Insert or overwrite. The key is copied. An overwritten value is released if owned.
 * @return DCDS_OK or DCDS_ENOMEM (map unchanged).
 */
dcds_status dcds_hashmap_put(dcds_hashmap *m, const char *key, void *value);

/** Look up @p key. @return the value, or NULL if absent (use dcds_hashmap_contains() to tell a stored NULL apart). */
void *dcds_hashmap_get(const dcds_hashmap *m, const char *key);

/** @return 1 if @p key is present, else 0. */
int dcds_hashmap_contains(const dcds_hashmap *m, const char *key);

/** Remove @p key. @p out may be NULL (value is then released if owned). @return DCDS_OK or DCDS_ENOTFOUND. */
dcds_status dcds_hashmap_remove(dcds_hashmap *m, const char *key, void **out);

/** Call @p fn once per live entry in unspecified order. */
void dcds_hashmap_foreach(const dcds_hashmap *m, dcds_hashmap_visit_fn fn, void *ctx);

#ifdef __cplusplus
}
#endif

#endif /* DCDS_HASHMAP_H */
