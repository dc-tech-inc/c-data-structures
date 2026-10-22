#include <stdlib.h>

#include "dcds/avl.h"
#include "minunit.h"

static int int_cmp(const void *a, const void *b)
{
    int x = *(const int *)a;
    int y = *(const int *)b;
    return (x > y) - (x < y);
}

#define N 2048
static int keys[N];

static void init_keys(void)
{
    for (int i = 0; i < N; i++) {
        keys[i] = i;
    }
}

static void test_empty(void)
{
    dcds_avl t;
    dcds_avl_init(&t, int_cmp, NULL);
    int k = 1;
    void *out = NULL;
    CHECK(dcds_avl_validate(&t) && dcds_avl_height(&t) == 0);
    CHECK(dcds_avl_get(&t, &k, &out) == DCDS_ENOTFOUND);
    CHECK(dcds_avl_remove(&t, &k, NULL) == DCDS_ENOTFOUND);
    CHECK(dcds_avl_min(&t, &out, NULL) == DCDS_ERANGE);
    CHECK(dcds_avl_max(&t, &out, NULL) == DCDS_ERANGE);
    dcds_avl_clear(&t);
}

/* Sorted insertion is the classic degenerate case for an unbalanced BST. */
static void test_sorted_insert_stays_balanced(void)
{
    init_keys();
    dcds_avl t;
    dcds_avl_init(&t, int_cmp, NULL);
    for (int i = 0; i < N; i++) {
        CHECK(dcds_avl_insert(&t, &keys[i], &keys[i]) == DCDS_OK);
    }
    CHECK(t.len == N && dcds_avl_validate(&t));
    CHECK(dcds_avl_height(&t) <= 12); /* 1.44 * log2(2050) ~ 15.9 worst case; sorted input gives 12 */
    void *k = NULL, *v = NULL;
    CHECK(dcds_avl_min(&t, &k, &v) == DCDS_OK && *(int *)k == 0 && v == &keys[0]);
    CHECK(dcds_avl_max(&t, &k, NULL) == DCDS_OK && *(int *)k == N - 1);
    for (int i = N - 1; i >= 0; i--) { /* and descending removal */
        CHECK(dcds_avl_remove(&t, &keys[i], NULL) == DCDS_OK);
    }
    CHECK(t.len == 0 && t.root == NULL && dcds_avl_validate(&t));
}

typedef struct walk {
    int prev;
    size_t n;
    int ordered;
} walk;

// cppcheck-suppress constParameterCallback ; the signature is fixed by dcds_avl_visit_fn
static void walk_cb(const void *key, void *value, void *ctx)
{
    walk *w = ctx;
    int k = *(const int *)key;
    if (w->n > 0 && k <= w->prev) {
        w->ordered = 0;
    }
    if (value != key) {
        w->ordered = 0;
    }
    w->prev = k;
    w->n++;
}

static void test_foreach_in_order_and_overwrite(void)
{
    init_keys();
    dcds_avl t;
    dcds_avl_init(&t, int_cmp, NULL);
    for (int i = 0; i < 100; i++) {
        int k = (i * 37) % 100; /* a permutation of 0..99 */
        CHECK(dcds_avl_insert(&t, &keys[k], &keys[k]) == DCDS_OK);
    }
    walk w = {0, 0, 1};
    dcds_avl_foreach(&t, walk_cb, &w);
    CHECK(w.n == 100 && w.ordered);
    /* overwrite: same key, new value, no growth */
    CHECK(dcds_avl_insert(&t, &keys[5], &keys[6]) == DCDS_OK);
    CHECK(t.len == 100);
    void *v = NULL;
    CHECK(dcds_avl_get(&t, &keys[5], &v) == DCDS_OK && v == &keys[6]);
    CHECK(dcds_avl_get(&t, &keys[5], NULL) == DCDS_OK); /* presence test */
    dcds_avl_clear(&t);
}

static int freed;
static void counting_free(void *p)
{
    freed++;
    free(p);
}

static void *boxed(int x)
{
    int *p = malloc(sizeof *p);
    if (p != NULL) {
        *p = x;
    }
    return p;
}

static void test_ownership(void)
{
    init_keys();
    dcds_avl t;
    dcds_avl_init(&t, int_cmp, counting_free);
    freed = 0;
    for (int i = 0; i < 6; i++) {
        CHECK(dcds_avl_insert(&t, &keys[i], boxed(i)) == DCDS_OK);
    }
    CHECK(dcds_avl_insert(&t, &keys[0], boxed(100)) == DCDS_OK); /* overwritten value released */
    CHECK(freed == 1);
    CHECK(dcds_avl_remove(&t, &keys[1], NULL) == DCDS_OK && freed == 2);
    void *out = NULL;
    CHECK(dcds_avl_remove(&t, &keys[2], &out) == DCDS_OK && freed == 2);
    free(out);
    dcds_avl_clear(&t); /* 4 left */
    CHECK(freed == 6);
}

/* Random insert/remove against a presence model; the invariants are checked every 50 steps and at the end. */
static void test_random_against_model(void)
{
    init_keys();
    static int present[N];
    size_t live = 0;
    dcds_avl t;
    dcds_avl_init(&t, int_cmp, NULL);
    for (int step = 0; step < 60000; step++) {
        int k = (int)(mu_rand() % N);
        if (mu_rand() % 5 < 3) {
            CHECK(dcds_avl_insert(&t, &keys[k], &keys[k]) == DCDS_OK);
            live += present[k] ? 0 : 1;
            present[k] = 1;
        } else {
            dcds_status s = dcds_avl_remove(&t, &keys[k], NULL);
            CHECK(s == (present[k] ? DCDS_OK : DCDS_ENOTFOUND));
            live -= present[k] ? 1 : 0;
            present[k] = 0;
        }
        CHECK(t.len == live);
        if (step % 50 == 0) {
            CHECK(dcds_avl_validate(&t));
        }
    }
    CHECK(dcds_avl_validate(&t));
    for (int k = 0; k < N; k++) {
        CHECK((dcds_avl_get(&t, &keys[k], NULL) == DCDS_OK) == present[k]);
    }
    dcds_avl_clear(&t);
}

/* Deleting nodes with two children exercises the successor-replacement path. */
static void test_remove_root_repeatedly(void)
{
    init_keys();
    dcds_avl t;
    dcds_avl_init(&t, int_cmp, NULL);
    for (int i = 0; i < 255; i++) {
        CHECK(dcds_avl_insert(&t, &keys[i], &keys[i]) == DCDS_OK);
    }
    while (t.root != NULL) {
        int rk = *(int *)t.root->key;
        CHECK(dcds_avl_remove(&t, &rk, NULL) == DCDS_OK);
        CHECK(dcds_avl_validate(&t));
    }
    CHECK(t.len == 0);
}

int main(void)
{
    RUN(test_empty);
    RUN(test_sorted_insert_stays_balanced);
    RUN(test_foreach_in_order_and_overwrite);
    RUN(test_ownership);
    RUN(test_random_against_model);
    RUN(test_remove_root_repeatedly);
    return MU_REPORT();
}
