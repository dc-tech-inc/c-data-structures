#include <stdlib.h>

#include "dcds/heap.h"
#include "minunit.h"

static int int_cmp(const void *a, const void *b)
{
    int x = *(const int *)a;
    int y = *(const int *)b;
    return (x > y) - (x < y);
}

static int rev_cmp(const void *a, const void *b)
{
    return int_cmp(b, a);
}

static void test_pop_order(void)
{
    int v[] = {5, 3, 8, 1, 9, 2, 7, 3};
    dcds_heap h;
    dcds_heap_init(&h, int_cmp, NULL);
    CHECK(dcds_heap_peek(&h) == NULL);
    void *out = NULL;
    CHECK(dcds_heap_pop(&h, &out) == DCDS_ERANGE);
    for (size_t i = 0; i < sizeof v / sizeof v[0]; i++) {
        CHECK(dcds_heap_push(&h, &v[i]) == DCDS_OK);
    }
    CHECK(*(int *)dcds_heap_peek(&h) == 1);
    int prev = -1;
    size_t popped = 0;
    while (dcds_heap_pop(&h, &out) == DCDS_OK) {
        int x = *(int *)out;
        CHECK(x >= prev);
        prev = x;
        popped++;
    }
    CHECK(popped == sizeof v / sizeof v[0]);
    CHECK(h.len == 0);
    dcds_heap_clear(&h);
}

static void test_max_heap_via_comparator(void)
{
    int v[] = {4, 10, 6};
    dcds_heap h;
    dcds_heap_init(&h, rev_cmp, NULL);
    for (size_t i = 0; i < 3; i++) {
        CHECK(dcds_heap_push(&h, &v[i]) == DCDS_OK);
    }
    void *out = NULL;
    CHECK(dcds_heap_pop(&h, &out) == DCDS_OK && *(int *)out == 10);
    CHECK(dcds_heap_pop(&h, &out) == DCDS_OK && *(int *)out == 6);
    CHECK(dcds_heap_pop(&h, &out) == DCDS_OK && *(int *)out == 4);
    dcds_heap_clear(&h);
}

static void test_single_element(void)
{
    int x = 42;
    dcds_heap h;
    dcds_heap_init(&h, int_cmp, NULL);
    CHECK(dcds_heap_push(&h, &x) == DCDS_OK);
    void *out = NULL;
    CHECK(dcds_heap_pop(&h, &out) == DCDS_OK && out == &x && h.len == 0);
    CHECK(dcds_heap_pop(&h, &out) == DCDS_ERANGE);
    dcds_heap_clear(&h);
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
    dcds_heap h;
    dcds_heap_init(&h, int_cmp, counting_free);
    freed = 0;
    for (int i = 5; i > 0; i--) {
        CHECK(dcds_heap_push(&h, boxed(i)) == DCDS_OK);
    }
    CHECK(dcds_heap_pop(&h, NULL) == DCDS_OK && freed == 1);
    void *out = NULL;
    CHECK(dcds_heap_pop(&h, &out) == DCDS_OK && freed == 1);
    CHECK(*(int *)out == 2);
    free(out);
    dcds_heap_clear(&h); /* 3 left */
    CHECK(freed == 4);
}

/* Interleaved push/pop; every pop must equal the minimum of a reference multiset kept as counts. */
static void test_random_against_model(void)
{
    enum { RANGE = 200 };
    static int vals[RANGE];
    static int count[RANGE];
    size_t n = 0;
    dcds_heap h;
    dcds_heap_init(&h, int_cmp, NULL);
    for (int i = 0; i < RANGE; i++) {
        vals[i] = i;
    }
    for (int step = 0; step < 40000; step++) {
        if (n == 0 || mu_rand() % 5 < 3) {
            int k = (int)(mu_rand() % RANGE);
            CHECK(dcds_heap_push(&h, &vals[k]) == DCDS_OK);
            count[k]++;
            n++;
        } else {
            int min = 0;
            while (count[min] == 0) {
                min++;
            }
            void *out = NULL;
            CHECK(dcds_heap_pop(&h, &out) == DCDS_OK && *(int *)out == min);
            count[min]--;
            n--;
        }
        CHECK(h.len == n);
    }
    dcds_heap_clear(&h);
}

int main(void)
{
    RUN(test_pop_order);
    RUN(test_max_heap_via_comparator);
    RUN(test_single_element);
    RUN(test_ownership);
    RUN(test_random_against_model);
    return MU_REPORT();
}
