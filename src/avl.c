#include "dcds/avl.h"

#include <stdlib.h>

#include "internal.h"

void dcds_avl_init(dcds_avl *t, dcds_cmp_fn cmp, dcds_free_fn free_fn)
{
    t->root = NULL;
    t->len = 0;
    t->cmp = cmp;
    t->free_fn = free_fn;
}

static void free_subtree(dcds_avl_node *n, dcds_free_fn free_fn)
{
    if (n == NULL) {
        return;
    }
    free_subtree(n->left, free_fn);
    free_subtree(n->right, free_fn);
    if (free_fn != NULL) {
        free_fn(n->value);
    }
    free(n);
}

void dcds_avl_clear(dcds_avl *t)
{
    free_subtree(t->root, t->free_fn);
    t->root = NULL;
    t->len = 0;
}

static int height_of(const dcds_avl_node *n)
{
    return n == NULL ? 0 : n->height;
}

static void update_height(dcds_avl_node *n)
{
    int l = height_of(n->left);
    int r = height_of(n->right);
    n->height = (l > r ? l : r) + 1;
}

static dcds_avl_node *rotate_right(dcds_avl_node *n)
{
    dcds_avl_node *l = n->left;
    n->left = l->right;
    l->right = n;
    update_height(n);
    update_height(l);
    return l;
}

/* Callers (rebalance) only rotate left when the right subtree is at least two taller, so n->right is non-NULL. */
static dcds_avl_node *rotate_left(dcds_avl_node *n)
{
    dcds_avl_node *r = n->right;
    // NOLINTNEXTLINE(clang-analyzer-core.NullDereference)
    n->right = r->left;
    r->left = n;
    update_height(n);
    update_height(r);
    return r;
}

/* Restore the AVL invariant at n, assuming both subtrees already satisfy it. Returns the new subtree root. */
static dcds_avl_node *rebalance(dcds_avl_node *n)
{
    update_height(n);
    int balance = height_of(n->left) - height_of(n->right);
    if (balance > 1) {
        if (height_of(n->left->left) < height_of(n->left->right)) {
            n->left = rotate_left(n->left); /* left-right case */
        }
        return rotate_right(n);
    }
    if (balance < -1) {
        if (height_of(n->right->right) < height_of(n->right->left)) {
            n->right = rotate_right(n->right); /* right-left case */
        }
        return rotate_left(n);
    }
    return n;
}

typedef struct insert_ctx {
    dcds_avl *t;
    void *key;
    void *value;
    dcds_status status;
    int added;
} insert_ctx;

static dcds_avl_node *insert_rec(dcds_avl_node *n, insert_ctx *c)
{
    if (n == NULL) {
        dcds_avl_node *leaf = malloc(sizeof *leaf);
        if (leaf == NULL) {
            c->status = DCDS_ENOMEM;
            return NULL;
        }
        leaf->left = NULL;
        leaf->right = NULL;
        leaf->key = c->key;
        leaf->value = c->value;
        leaf->height = 1;
        c->added = 1;
        return leaf;
    }
    int cmp = c->t->cmp(c->key, n->key);
    if (cmp == 0) {
        if (c->t->free_fn != NULL) {
            c->t->free_fn(n->value);
        }
        n->value = c->value;
        return n;
    }
    if (cmp < 0) {
        n->left = insert_rec(n->left, c);
    } else {
        n->right = insert_rec(n->right, c);
    }
    /* On ENOMEM nothing below changed, so the shape (and balance) is untouched. */
    return c->added ? rebalance(n) : n;
}

dcds_status dcds_avl_insert(dcds_avl *t, void *key, void *value)
{
    insert_ctx c = {t, key, value, DCDS_OK, 0};
    t->root = insert_rec(t->root, &c);
    if (c.added) {
        t->len++;
    }
    return c.status;
}

static const dcds_avl_node *find_node(const dcds_avl *t, const void *key)
{
    const dcds_avl_node *n = t->root;
    while (n != NULL) {
        int cmp = t->cmp(key, n->key);
        if (cmp == 0) {
            return n;
        }
        n = cmp < 0 ? n->left : n->right;
    }
    return NULL;
}

dcds_status dcds_avl_get(const dcds_avl *t, const void *key, void **value)
{
    const dcds_avl_node *n = find_node(t, key);
    if (n == NULL) {
        return DCDS_ENOTFOUND;
    }
    if (value != NULL) {
        *value = n->value;
    }
    return DCDS_OK;
}

/* Detach the smallest node of n's subtree (stored in *min) and return the rebalanced remainder. */
static dcds_avl_node *detach_min(dcds_avl_node *n, dcds_avl_node **min)
{
    if (n->left == NULL) {
        *min = n;
        return n->right;
    }
    n->left = detach_min(n->left, min);
    return rebalance(n);
}

typedef struct remove_ctx {
    dcds_avl *t;
    const void *key;
    void *value;
    int found;
} remove_ctx;

static dcds_avl_node *remove_rec(dcds_avl_node *n, remove_ctx *c)
{
    if (n == NULL) {
        return NULL;
    }
    int cmp = c->t->cmp(c->key, n->key);
    if (cmp < 0) {
        n->left = remove_rec(n->left, c);
    } else if (cmp > 0) {
        n->right = remove_rec(n->right, c);
    } else {
        c->found = 1;
        c->value = n->value;
        if (n->left == NULL || n->right == NULL) {
            dcds_avl_node *child = n->left != NULL ? n->left : n->right;
            free(n);
            return child;
        }
        /* Two children: the in-order successor takes this node's place. */
        dcds_avl_node *succ = NULL;
        dcds_avl_node *right = detach_min(n->right, &succ);
        succ->left = n->left;
        succ->right = right;
        free(n);
        n = succ;
    }
    return c->found ? rebalance(n) : n;
}

dcds_status dcds_avl_remove(dcds_avl *t, const void *key, void **out)
{
    remove_ctx c = {t, key, NULL, 0};
    t->root = remove_rec(t->root, &c);
    if (!c.found) {
        return DCDS_ENOTFOUND;
    }
    t->len--;
    dcds_give_or_free(out, c.value, t->free_fn);
    return DCDS_OK;
}

dcds_status dcds_avl_min(const dcds_avl *t, void **key, void **value)
{
    const dcds_avl_node *n = t->root;
    if (n == NULL) {
        return DCDS_ERANGE;
    }
    while (n->left != NULL) {
        n = n->left;
    }
    if (key != NULL) {
        *key = n->key;
    }
    if (value != NULL) {
        *value = n->value;
    }
    return DCDS_OK;
}

dcds_status dcds_avl_max(const dcds_avl *t, void **key, void **value)
{
    const dcds_avl_node *n = t->root;
    if (n == NULL) {
        return DCDS_ERANGE;
    }
    while (n->right != NULL) {
        n = n->right;
    }
    if (key != NULL) {
        *key = n->key;
    }
    if (value != NULL) {
        *value = n->value;
    }
    return DCDS_OK;
}

static void foreach_rec(const dcds_avl_node *n, dcds_avl_visit_fn fn, void *ctx)
{
    if (n == NULL) {
        return;
    }
    foreach_rec(n->left, fn, ctx);
    fn(n->key, n->value, ctx);
    foreach_rec(n->right, fn, ctx);
}

void dcds_avl_foreach(const dcds_avl *t, dcds_avl_visit_fn fn, void *ctx)
{
    foreach_rec(t->root, fn, ctx);
}

int dcds_avl_height(const dcds_avl *t)
{
    return height_of(t->root);
}

/* Returns the verified height of the subtree, or -1 if any invariant fails. lo/hi are exclusive bounds (NULL = none). */
static int validate_rec(const dcds_avl *t, const dcds_avl_node *n, const void *lo, const void *hi, size_t *count)
{
    if (n == NULL) {
        return 0;
    }
    if ((lo != NULL && t->cmp(n->key, lo) <= 0) || (hi != NULL && t->cmp(n->key, hi) >= 0)) {
        return -1;
    }
    int l = validate_rec(t, n->left, lo, n->key, count);
    int r = validate_rec(t, n->right, n->key, hi, count);
    if (l < 0 || r < 0 || l - r > 1 || r - l > 1) {
        return -1;
    }
    int h = (l > r ? l : r) + 1;
    if (n->height != h) {
        return -1;
    }
    (*count)++;
    return h;
}

int dcds_avl_validate(const dcds_avl *t)
{
    size_t count = 0;
    return validate_rec(t, t->root, NULL, NULL, &count) >= 0 && count == t->len;
}
