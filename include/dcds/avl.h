/**
 * @file avl.h
 * @brief Ordered map as a self-balancing AVL tree.
 *
 * Keys and values are `void *`; ordering comes from the comparator given at
 * init. The tree never owns keys (they stay valid for as long as they are in
 * the tree); it owns values iff a free function is given (see common.h).
 *
 * Invariant: for every node, the heights of its two subtrees differ by at most
 * one, so the height is at most 1.44 * log2(n + 2). Insert, find and remove
 * are O(log n) worst case; in-order traversal is O(n). Recursion depth equals
 * the tree height, so it stays small (about 44 frames for a billion keys).
 */
#ifndef DCDS_AVL_H
#define DCDS_AVL_H

#include <stddef.h>

#include "dcds/common.h"

#ifdef __cplusplus
extern "C" {
#endif

/** One tree node. Internal layout, exposed so the struct can live on the stack. */
typedef struct dcds_avl_node {
    struct dcds_avl_node *left;
    struct dcds_avl_node *right;
    void *key;
    void *value;
    int height; /**< 1 for a leaf */
} dcds_avl_node;

/** A tree. Initialise with dcds_avl_init(), release with dcds_avl_clear(). */
typedef struct dcds_avl {
    dcds_avl_node *root;
    size_t len;
    dcds_cmp_fn cmp;      /**< ordering of keys */
    dcds_free_fn free_fn; /**< owns values when non-NULL (see common.h) */
} dcds_avl;

/** Callback for dcds_avl_foreach(). Must not modify the tree. */
typedef void (*dcds_avl_visit_fn)(const void *key, void *value, void *ctx);

/** Make an empty tree ordered by @p cmp (required). @p free_fn (for values) may be NULL. */
void dcds_avl_init(dcds_avl *t, dcds_cmp_fn cmp, dcds_free_fn free_fn);

/** Free every node (and every value when owned). The tree is empty afterwards and reusable. */
void dcds_avl_clear(dcds_avl *t);

/**
 * Insert, or overwrite the value of an existing equal key (the stored key pointer is kept,
 * @p key is ignored in that case; the old value is released if owned).
 * @return DCDS_OK or DCDS_ENOMEM (tree unchanged). O(log n).
 */
dcds_status dcds_avl_insert(dcds_avl *t, void *key, void *value);

/**
 * Look up @p key. On success stores the value in @p *value (may be NULL to only test presence).
 * @return DCDS_OK or DCDS_ENOTFOUND. O(log n).
 */
dcds_status dcds_avl_get(const dcds_avl *t, const void *key, void **value);

/** Remove @p key. @p out may be NULL (value is then released if owned). @return DCDS_OK or DCDS_ENOTFOUND. O(log n). */
dcds_status dcds_avl_remove(dcds_avl *t, const void *key, void **out);

/** Smallest key. Writes key/value to the non-NULL outputs. @return DCDS_OK or DCDS_ERANGE if empty. O(log n). */
dcds_status dcds_avl_min(const dcds_avl *t, void **key, void **value);

/** Largest key. Writes key/value to the non-NULL outputs. @return DCDS_OK or DCDS_ERANGE if empty. O(log n). */
dcds_status dcds_avl_max(const dcds_avl *t, void **key, void **value);

/** Visit every entry in ascending key order. O(n). */
void dcds_avl_foreach(const dcds_avl *t, dcds_avl_visit_fn fn, void *ctx);

/** Height of the tree (0 when empty). O(1). */
int dcds_avl_height(const dcds_avl *t);

/**
 * Check the structural invariants: strict key order, stored heights, balance factor in [-1, 1]
 * and the entry count. Meant for tests and debugging. @return 1 if valid, else 0. O(n).
 */
int dcds_avl_validate(const dcds_avl *t);

#ifdef __cplusplus
}
#endif

#endif /* DCDS_AVL_H */
