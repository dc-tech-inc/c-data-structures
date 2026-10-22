/**
 * @file list.h
 * @brief Doubly linked list.
 *
 * Complexity (n = length): push/pop at either end O(1), unlink of a known node
 * O(1), find O(n), reverse O(n), clear O(n). Nodes are heap allocated one by
 * one, so iteration is pointer chasing (see the benchmark notes in the README).
 */
#ifndef DCDS_LIST_H
#define DCDS_LIST_H

#include <stddef.h>

#include "dcds/common.h"

#ifdef __cplusplus
extern "C" {
#endif

/** One list node. Read-only for callers; walk with @c next / @c prev. */
typedef struct dcds_list_node {
    struct dcds_list_node *prev;
    struct dcds_list_node *next;
    void *data;
} dcds_list_node;

/** A list. Initialise with dcds_list_init(), release with dcds_list_clear(). */
typedef struct dcds_list {
    dcds_list_node *head; /**< first node or NULL */
    dcds_list_node *tail; /**< last node or NULL */
    size_t len;           /**< number of nodes */
    dcds_free_fn free_fn; /**< owns items when non-NULL (see common.h) */
} dcds_list;

/** Make an empty list. @p free_fn may be NULL. */
void dcds_list_init(dcds_list *l, dcds_free_fn free_fn);

/** Free every node (and every item when the list owns them). The list is empty afterwards and reusable. */
void dcds_list_clear(dcds_list *l);

/** Insert at the front. @return DCDS_OK or DCDS_ENOMEM (list unchanged). O(1). */
dcds_status dcds_list_push_front(dcds_list *l, void *data);

/** Insert at the back. @return DCDS_OK or DCDS_ENOMEM (list unchanged). O(1). */
dcds_status dcds_list_push_back(dcds_list *l, void *data);

/** Remove the first item. @p out may be NULL (item is then released). @return DCDS_OK or DCDS_ERANGE if empty. O(1). */
dcds_status dcds_list_pop_front(dcds_list *l, void **out);

/** Remove the last item. @p out may be NULL (item is then released). @return DCDS_OK or DCDS_ERANGE if empty. O(1). */
dcds_status dcds_list_pop_back(dcds_list *l, void **out);

/**
 * Unlink and free a node that belongs to @p l. @p out may be NULL (item is then released).
 * Passing a node of another list is undefined behaviour. O(1).
 */
void dcds_list_remove(dcds_list *l, dcds_list_node *node, void **out);

/** First node whose item compares equal to @p key under @p cmp (called as cmp(item, key)), or NULL. O(n). */
dcds_list_node *dcds_list_find(const dcds_list *l, dcds_cmp_fn cmp, const void *key);

/** Reverse the list in place without allocating. O(n). */
void dcds_list_reverse(dcds_list *l);

#ifdef __cplusplus
}
#endif

#endif /* DCDS_LIST_H */
