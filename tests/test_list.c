#include <stdlib.h>
#include <string.h>

#include "dcds/list.h"
#include "minunit.h"

static int int_cmp(const void *a, const void *b)
{
    return *(const int *)a - *(const int *)b;
}

static void test_push_pop_order(void)
{
    int v[4] = {1, 2, 3, 4};
    dcds_list l;
    dcds_list_init(&l, NULL);
    CHECK(dcds_list_push_back(&l, &v[1]) == DCDS_OK);
    CHECK(dcds_list_push_back(&l, &v[2]) == DCDS_OK);
    CHECK(dcds_list_push_front(&l, &v[0]) == DCDS_OK);
    CHECK(dcds_list_push_back(&l, &v[3]) == DCDS_OK);
    CHECK(l.len == 4);
    void *out = NULL;
    CHECK(dcds_list_pop_front(&l, &out) == DCDS_OK && out == &v[0]);
    CHECK(dcds_list_pop_back(&l, &out) == DCDS_OK && out == &v[3]);
    CHECK(dcds_list_pop_front(&l, &out) == DCDS_OK && out == &v[1]);
    CHECK(dcds_list_pop_back(&l, &out) == DCDS_OK && out == &v[2]);
    CHECK(l.len == 0 && l.head == NULL && l.tail == NULL);
    CHECK(dcds_list_pop_front(&l, &out) == DCDS_ERANGE);
    CHECK(dcds_list_pop_back(&l, &out) == DCDS_ERANGE);
    dcds_list_clear(&l);
}

static void test_find_and_remove_middle(void)
{
    int v[3] = {10, 20, 30};
    dcds_list l;
    dcds_list_init(&l, NULL);
    for (int i = 0; i < 3; i++) {
        CHECK(dcds_list_push_back(&l, &v[i]) == DCDS_OK);
    }
    int key = 20;
    dcds_list_node *n = dcds_list_find(&l, int_cmp, &key);
    CHECK(n != NULL && n->data == &v[1]);
    int missing = 99;
    CHECK(dcds_list_find(&l, int_cmp, &missing) == NULL);
    void *out = NULL;
    dcds_list_remove(&l, n, &out);
    CHECK(out == &v[1] && l.len == 2);
    CHECK(l.head->next == l.tail && l.tail->prev == l.head);
    dcds_list_clear(&l);
}

static void test_reverse(void)
{
    int v[5] = {0, 1, 2, 3, 4};
    dcds_list l;
    dcds_list_init(&l, NULL);
    dcds_list_reverse(&l); /* empty list is fine */
    CHECK(l.head == NULL && l.tail == NULL);
    for (int i = 0; i < 5; i++) {
        CHECK(dcds_list_push_back(&l, &v[i]) == DCDS_OK);
    }
    dcds_list_reverse(&l);
    int expect = 4;
    for (dcds_list_node *n = l.head; n != NULL; n = n->next) {
        CHECK(*(int *)n->data == expect--);
    }
    expect = 0;
    for (dcds_list_node *n = l.tail; n != NULL; n = n->prev) {
        CHECK(*(int *)n->data == expect++);
    }
    CHECK(l.len == 5);
    dcds_list_clear(&l);
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
    dcds_list l;
    dcds_list_init(&l, counting_free);
    freed = 0;
    for (int i = 0; i < 5; i++) {
        CHECK(dcds_list_push_back(&l, boxed(i)) == DCDS_OK);
    }
    CHECK(dcds_list_pop_front(&l, NULL) == DCDS_OK); /* NULL out: released */
    CHECK(freed == 1);
    void *out = NULL;
    CHECK(dcds_list_pop_back(&l, &out) == DCDS_OK); /* out given: ownership moves */
    CHECK(freed == 1);
    free(out);
    dcds_list_clear(&l); /* the three remaining items */
    CHECK(freed == 4);
    CHECK(l.len == 0);
    dcds_list_clear(&l); /* idempotent */
}

static void test_status_str(void)
{
    CHECK(strcmp(dcds_status_str(DCDS_OK), "ok") == 0);
    CHECK(strcmp(dcds_status_str(DCDS_ENOMEM), "out of memory") == 0);
    CHECK(strcmp(dcds_status_str(DCDS_ENOTFOUND), "not found") == 0);
    CHECK(strcmp(dcds_status_str(DCDS_ERANGE), "out of range or empty") == 0);
    volatile int raw = 99; /* volatile: an out-of-range enum value is deliberate here */
    // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
    CHECK(strcmp(dcds_status_str((dcds_status)raw), "unknown status") == 0);
}

int main(void)
{
    RUN(test_push_pop_order);
    RUN(test_find_and_remove_middle);
    RUN(test_reverse);
    RUN(test_ownership);
    RUN(test_status_str);
    return MU_REPORT();
}
