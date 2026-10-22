#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dcds/hashmap.h"
#include "minunit.h"

static void test_basic(void)
{
    int a = 1, b = 2;
    dcds_hashmap m;
    dcds_hashmap_init(&m, NULL);
    CHECK(dcds_hashmap_get(&m, "x") == NULL); /* empty map, no table yet */
    CHECK(!dcds_hashmap_contains(&m, "x"));
    CHECK(dcds_hashmap_remove(&m, "x", NULL) == DCDS_ENOTFOUND);
    CHECK(dcds_hashmap_put(&m, "alpha", &a) == DCDS_OK);
    CHECK(dcds_hashmap_put(&m, "beta", &b) == DCDS_OK);
    CHECK(m.len == 2);
    CHECK(dcds_hashmap_get(&m, "alpha") == &a && dcds_hashmap_get(&m, "beta") == &b);
    CHECK(dcds_hashmap_get(&m, "gamma") == NULL);
    CHECK(dcds_hashmap_put(&m, "alpha", &b) == DCDS_OK); /* overwrite */
    CHECK(m.len == 2 && dcds_hashmap_get(&m, "alpha") == &b);
    char key[] = "mutable";
    CHECK(dcds_hashmap_put(&m, key, &a) == DCDS_OK);
    key[0] = 'X'; /* the map holds its own copy */
    CHECK(dcds_hashmap_get(&m, "mutable") == &a && !dcds_hashmap_contains(&m, key));
    CHECK(dcds_hashmap_put(&m, "", &a) == DCDS_OK); /* empty string is a valid key */
    CHECK(dcds_hashmap_get(&m, "") == &a);
    CHECK(dcds_hashmap_put(&m, "nullval", NULL) == DCDS_OK);
    CHECK(dcds_hashmap_contains(&m, "nullval") && dcds_hashmap_get(&m, "nullval") == NULL);
    void *out = NULL;
    CHECK(dcds_hashmap_remove(&m, "alpha", &out) == DCDS_OK && out == &b);
    CHECK(!dcds_hashmap_contains(&m, "alpha") && m.len == 4);
    CHECK(dcds_hashmap_remove(&m, "alpha", NULL) == DCDS_ENOTFOUND);
    dcds_hashmap_clear(&m);
    CHECK(m.len == 0 && m.cap == 0 && m.slots == NULL);
}

static void sum_cb(const char *key, void *value, void *ctx)
{
    (void)key;
    *(long *)ctx += *(int *)value;
}

static void test_growth_and_foreach(void)
{
    enum { N = 5000 };
    static int vals[N];
    char key[32];
    dcds_hashmap m;
    dcds_hashmap_init(&m, NULL);
    long expect = 0;
    for (int i = 0; i < N; i++) {
        vals[i] = i;
        expect += i;
        snprintf(key, sizeof key, "key-%d", i);
        CHECK(dcds_hashmap_put(&m, key, &vals[i]) == DCDS_OK);
    }
    CHECK(m.len == N);
    CHECK(m.used <= m.cap - m.cap / 4); /* load factor stays bounded */
    for (int i = 0; i < N; i++) {
        snprintf(key, sizeof key, "key-%d", i);
        CHECK(dcds_hashmap_get(&m, key) == &vals[i]);
    }
    long sum = 0;
    dcds_hashmap_foreach(&m, sum_cb, &sum);
    CHECK(sum == expect);
    dcds_hashmap_clear(&m);
}

/* Heavy insert/delete churn must reuse tombstones and never loop forever on a full-of-tombstones table. */
static void test_tombstone_churn(void)
{
    enum { ROUNDS = 20000 };
    static int one = 1;
    char key[32];
    dcds_hashmap m;
    dcds_hashmap_init(&m, NULL);
    for (int i = 0; i < ROUNDS; i++) {
        snprintf(key, sizeof key, "k%d", i);
        CHECK(dcds_hashmap_put(&m, key, &one) == DCDS_OK);
        if (i >= 3) {
            snprintf(key, sizeof key, "k%d", i - 3);
            CHECK(dcds_hashmap_remove(&m, key, NULL) == DCDS_OK);
        }
        CHECK(m.used <= m.cap - m.cap / 4);
    }
    CHECK(m.len == 3);
    CHECK(m.cap <= 64); /* the table did not balloon on churn */
    CHECK(!dcds_hashmap_contains(&m, "k0"));
    CHECK(dcds_hashmap_contains(&m, "k19999"));
    dcds_hashmap_clear(&m);
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
    dcds_hashmap m;
    dcds_hashmap_init(&m, counting_free);
    freed = 0;
    CHECK(dcds_hashmap_put(&m, "a", boxed(1)) == DCDS_OK);
    CHECK(dcds_hashmap_put(&m, "b", boxed(2)) == DCDS_OK);
    CHECK(dcds_hashmap_put(&m, "c", boxed(3)) == DCDS_OK);
    CHECK(dcds_hashmap_put(&m, "a", boxed(10)) == DCDS_OK); /* old value released */
    CHECK(freed == 1);
    CHECK(dcds_hashmap_remove(&m, "b", NULL) == DCDS_OK);
    CHECK(freed == 2);
    void *out = NULL;
    CHECK(dcds_hashmap_remove(&m, "c", &out) == DCDS_OK && freed == 2);
    free(out);
    dcds_hashmap_clear(&m); /* "a" */
    CHECK(freed == 3);
}

/* Random put/remove checked against a presence model. */
static void test_random_against_model(void)
{
    enum { KEYS = 300 };
    static int present[KEYS];
    static int vals[KEYS];
    char key[16];
    size_t live = 0;
    dcds_hashmap m;
    dcds_hashmap_init(&m, NULL);
    for (int step = 0; step < 30000; step++) {
        int k = (int)(mu_rand() % KEYS);
        snprintf(key, sizeof key, "%d", k);
        if (mu_rand() % 2 == 0) {
            vals[k] = step;
            CHECK(dcds_hashmap_put(&m, key, &vals[k]) == DCDS_OK);
            live += present[k] ? 0 : 1;
            present[k] = 1;
        } else {
            dcds_status s = dcds_hashmap_remove(&m, key, NULL);
            CHECK(s == (present[k] ? DCDS_OK : DCDS_ENOTFOUND));
            live -= present[k] ? 1 : 0;
            present[k] = 0;
        }
        CHECK(m.len == live);
    }
    for (int k = 0; k < KEYS; k++) {
        snprintf(key, sizeof key, "%d", k);
        CHECK(dcds_hashmap_contains(&m, key) == present[k]);
    }
    dcds_hashmap_clear(&m);
}

int main(void)
{
    RUN(test_basic);
    RUN(test_growth_and_foreach);
    RUN(test_tombstone_churn);
    RUN(test_ownership);
    RUN(test_random_against_model);
    return MU_REPORT();
}
