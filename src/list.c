#include "dcds/list.h"

#include <stdlib.h>

#include "internal.h"

void dcds_list_init(dcds_list *l, dcds_free_fn free_fn)
{
    l->head = NULL;
    l->tail = NULL;
    l->len = 0;
    l->free_fn = free_fn;
}

void dcds_list_clear(dcds_list *l)
{
    dcds_list_node *n = l->head;
    while (n != NULL) {
        dcds_list_node *next = n->next;
        if (l->free_fn != NULL) {
            l->free_fn(n->data);
        }
        free(n);
        n = next;
    }
    l->head = NULL;
    l->tail = NULL;
    l->len = 0;
}

static dcds_list_node *new_node(void *data)
{
    dcds_list_node *n = malloc(sizeof *n);
    if (n != NULL) {
        n->prev = NULL;
        n->next = NULL;
        n->data = data;
    }
    return n;
}

dcds_status dcds_list_push_front(dcds_list *l, void *data)
{
    dcds_list_node *n = new_node(data);
    if (n == NULL) {
        return DCDS_ENOMEM;
    }
    n->next = l->head;
    if (l->head != NULL) {
        l->head->prev = n;
    } else {
        l->tail = n;
    }
    l->head = n;
    l->len++;
    return DCDS_OK;
}

dcds_status dcds_list_push_back(dcds_list *l, void *data)
{
    dcds_list_node *n = new_node(data);
    if (n == NULL) {
        return DCDS_ENOMEM;
    }
    n->prev = l->tail;
    if (l->tail != NULL) {
        l->tail->next = n;
    } else {
        l->head = n;
    }
    l->tail = n;
    l->len++;
    return DCDS_OK;
}

void dcds_list_remove(dcds_list *l, dcds_list_node *node, void **out)
{
    if (node->prev != NULL) {
        node->prev->next = node->next;
    } else {
        l->head = node->next;
    }
    if (node->next != NULL) {
        node->next->prev = node->prev;
    } else {
        l->tail = node->prev;
    }
    l->len--;
    dcds_give_or_free(out, node->data, l->free_fn);
    free(node);
}

dcds_status dcds_list_pop_front(dcds_list *l, void **out)
{
    if (l->head == NULL) {
        return DCDS_ERANGE;
    }
    dcds_list_remove(l, l->head, out);
    return DCDS_OK;
}

dcds_status dcds_list_pop_back(dcds_list *l, void **out)
{
    if (l->tail == NULL) {
        return DCDS_ERANGE;
    }
    dcds_list_remove(l, l->tail, out);
    return DCDS_OK;
}

dcds_list_node *dcds_list_find(const dcds_list *l, dcds_cmp_fn cmp, const void *key)
{
    for (dcds_list_node *n = l->head; n != NULL; n = n->next) {
        if (cmp(n->data, key) == 0) {
            return n;
        }
    }
    return NULL;
}

void dcds_list_reverse(dcds_list *l)
{
    dcds_list_node *n = l->head;
    while (n != NULL) {
        dcds_list_node *next = n->next;
        n->next = n->prev;
        n->prev = next;
        n = next;
    }
    dcds_list_node *old_head = l->head;
    l->head = l->tail;
    l->tail = old_head;
}
