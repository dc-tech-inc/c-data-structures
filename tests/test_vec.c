#include <stdint.h>
#include <stdlib.h>

#include "dcds/vec.h"
#include "minunit.h"

#define N 1000

static void test_push_get_set(void)
{
    static int v[N];
    dcds_vec vec;
    dcds_vec_init(&vec, NULL);
    CHECK(dcds_vec_get(&vec, 0) == NULL);
    for (int i = 0; i < N; i++) {
        v[i] = i;
        CHECK(dcds_vec_push(&vec, &v[i]) == DCDS_OK);
    }
    CHECK(vec.len == N && vec.cap >= N);
    CHECK(dcds_vec_get(&vec, 0) == &v[0]);
    CHECK(dcds_vec_get(&vec, N - 1) == &v[N - 1]);
    CHECK(dcds_vec_get(&vec, N) == NULL);
    CHECK(dcds_vec_set(&vec, 3, &v[7]) == DCDS_OK && dcds_vec_get(&vec, 3) == &v[7]);
    CHECK(dcds_vec_set(&vec, N, &v[0]) == DCDS_ERANGE);
    dcds_vec_clear(&vec);
    CHECK(vec.len == 0 && vec.items == NULL && vec.cap == 0);
}

static void test_insert_remove(void)
{
    int v[4] = {0, 1, 2, 3};
    dcds_vec vec;
    dcds_vec_init(&vec, NULL);
    CHECK(dcds_vec_insert(&vec, 1, &v[0]) == DCDS_ERANGE); /* beyond len */
    CHECK(dcds_vec_insert(&vec, 0, &v[1]) == DCDS_OK);     /* [1] */
    CHECK(dcds_vec_insert(&vec, 0, &v[0]) == DCDS_OK);     /* [0 1] */
    CHECK(dcds_vec_insert(&vec, 2, &v[3]) == DCDS_OK);     /* [0 1 3] */
    CHECK(dcds_vec_insert(&vec, 2, &v[2]) == DCDS_OK);     /* [0 1 2 3] */
    for (size_t i = 0; i < 4; i++) {
        CHECK(dcds_vec_get(&vec, i) == &v[i]);
    }
    void *out = NULL;
    CHECK(dcds_vec_remove(&vec, 1, &out) == DCDS_OK && out == &v[1]);
    CHECK(vec.len == 3 && dcds_vec_get(&vec, 1) == &v[2] && dcds_vec_get(&vec, 2) == &v[3]);
    CHECK(dcds_vec_remove(&vec, 3, &out) == DCDS_ERANGE);
    CHECK(dcds_vec_remove(&vec, 2, &out) == DCDS_OK && out == &v[3]); /* last element */
    CHECK(dcds_vec_pop(&vec, &out) == DCDS_OK && out == &v[2]);
    CHECK(dcds_vec_pop(&vec, &out) == DCDS_OK && out == &v[0]);
    CHECK(dcds_vec_pop(&vec, &out) == DCDS_ERANGE);
    dcds_vec_clear(&vec);
}

static void test_reserve(void)
{
    dcds_vec vec;
    dcds_vec_init(&vec, NULL);
    CHECK(dcds_vec_reserve(&vec, 100) == DCDS_OK && vec.cap >= 100 && vec.len == 0);
    size_t cap = vec.cap;
    CHECK(dcds_vec_reserve(&vec, 10) == DCDS_OK && vec.cap == cap); /* never shrinks */
    CHECK(dcds_vec_reserve(&vec, SIZE_MAX) == DCDS_ENOMEM);         /* overflow is reported, not wrapped */
    CHECK(vec.cap == cap);
    dcds_vec_clear(&vec);
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
    dcds_vec vec;
    dcds_vec_init(&vec, counting_free);
    freed = 0;
    for (int i = 0; i < 6; i++) {
        CHECK(dcds_vec_push(&vec, boxed(i)) == DCDS_OK);
    }
    CHECK(dcds_vec_set(&vec, 0, boxed(100)) == DCDS_OK); /* old item released */
    CHECK(freed == 1);
    CHECK(dcds_vec_remove(&vec, 0, NULL) == DCDS_OK); /* released */
    CHECK(freed == 2);
    CHECK(dcds_vec_pop(&vec, NULL) == DCDS_OK);
    CHECK(freed == 3);
    void *out = NULL;
    CHECK(dcds_vec_pop(&vec, &out) == DCDS_OK && freed == 3); /* handed back, not freed */
    free(out);
    dcds_vec_clear(&vec); /* remaining 3 */
    CHECK(freed == 6);
}

/* Random operations checked against a plain array model. */
static void test_random_against_model(void)
{
    enum { CAP = 4096 };
    static void *model[CAP];
    static int store[CAP];
    size_t n = 0;
    int next = 0;
    dcds_vec vec;
    dcds_vec_init(&vec, NULL);
    for (int step = 0; step < 20000; step++) {
        unsigned long long r = mu_rand();
        size_t pos = n == 0 ? 0 : (size_t)(r >> 8) % (n + 1);
        if (n < CAP - 1 && (r % 3 != 0 || n == 0)) {
            void *item = &store[next++ % CAP];
            CHECK(dcds_vec_insert(&vec, pos, item) == DCDS_OK);
            for (size_t i = n; i > pos; i--) {
                model[i] = model[i - 1];
            }
            model[pos] = item;
            n++;
        } else {
            pos = (size_t)(r >> 8) % n;
            void *out = NULL;
            CHECK(dcds_vec_remove(&vec, pos, &out) == DCDS_OK && out == model[pos]);
            for (size_t i = pos; i + 1 < n; i++) {
                model[i] = model[i + 1];
            }
            n--;
        }
        CHECK(vec.len == n);
    }
    for (size_t i = 0; i < n; i++) {
        CHECK(dcds_vec_get(&vec, i) == model[i]);
    }
    dcds_vec_clear(&vec);
}

int main(void)
{
    RUN(test_push_get_set);
    RUN(test_insert_remove);
    RUN(test_reserve);
    RUN(test_ownership);
    RUN(test_random_against_model);
    return MU_REPORT();
}
