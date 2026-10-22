/*
 * Reproducible micro-benchmarks. Prints a Markdown table of the median of
 * REPEATS runs, in nanoseconds per operation, plus the machine and compiler.
 * Numbers are only meaningful for the machine printed in the header; the
 * README quotes the CI run, not a laptop.
 *
 *   make bench            (optimised build, no sanitizers)
 */
#define _POSIX_C_SOURCE 200809L

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>
#include <time.h>

#include "dcds/avl.h"
#include "dcds/hashmap.h"
#include "dcds/heap.h"
#include "dcds/list.h"
#include "dcds/vec.h"

#define N 200000u
#define REPEATS 5

static volatile uint64_t sink;

static uint64_t now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * UINT64_C(1000000000) + (uint64_t)ts.tv_nsec;
}

static int cmp_u64(const void *a, const void *b)
{
    uint64_t x = *(const uint64_t *)a;
    uint64_t y = *(const uint64_t *)b;
    return (x > y) - (x < y);
}

static uint64_t rng_state = UINT64_C(88172645463325252);
static uint64_t rng(void)
{
    rng_state ^= rng_state >> 12;
    rng_state ^= rng_state << 25;
    rng_state ^= rng_state >> 27;
    return rng_state * UINT64_C(2685821657736338717);
}

/* Shared input, generated once: random 64-bit keys and the same numbers as strings. */
static uint64_t nums[N];
static char (*strs)[32];

static void die(const char *what)
{
    fprintf(stderr, "bench: %s failed\n", what);
    exit(1);
}

/* ---- each bench returns elapsed ns for the timed section ---- */

static uint64_t b_vec_push(void)
{
    dcds_vec v;
    dcds_vec_init(&v, NULL);
    uint64_t t0 = now_ns();
    for (size_t i = 0; i < N; i++) {
        if (dcds_vec_push(&v, &nums[i]) != DCDS_OK) {
            die("vec push");
        }
    }
    uint64_t dt = now_ns() - t0;
    sink += v.len;
    dcds_vec_clear(&v);
    return dt;
}

static uint64_t b_vec_scan(void)
{
    dcds_vec v;
    dcds_vec_init(&v, NULL);
    for (size_t i = 0; i < N; i++) {
        if (dcds_vec_push(&v, &nums[i]) != DCDS_OK) {
            die("vec push");
        }
    }
    uint64_t sum = 0;
    uint64_t t0 = now_ns();
    for (size_t i = 0; i < N; i++) {
        sum += *(uint64_t *)dcds_vec_get(&v, i);
    }
    uint64_t dt = now_ns() - t0;
    sink += sum;
    dcds_vec_clear(&v);
    return dt;
}

static uint64_t b_list_push(void)
{
    dcds_list l;
    dcds_list_init(&l, NULL);
    uint64_t t0 = now_ns();
    for (size_t i = 0; i < N; i++) {
        if (dcds_list_push_back(&l, &nums[i]) != DCDS_OK) {
            die("list push");
        }
    }
    uint64_t dt = now_ns() - t0;
    sink += l.len;
    dcds_list_clear(&l);
    return dt;
}

static uint64_t b_list_scan(void)
{
    dcds_list l;
    dcds_list_init(&l, NULL);
    for (size_t i = 0; i < N; i++) {
        if (dcds_list_push_back(&l, &nums[i]) != DCDS_OK) {
            die("list push");
        }
    }
    uint64_t sum = 0;
    uint64_t t0 = now_ns();
    for (const dcds_list_node *n = l.head; n != NULL; n = n->next) {
        sum += *(uint64_t *)n->data;
    }
    uint64_t dt = now_ns() - t0;
    sink += sum;
    dcds_list_clear(&l);
    return dt;
}

static uint64_t b_map_put(void)
{
    dcds_hashmap m;
    dcds_hashmap_init(&m, NULL);
    uint64_t t0 = now_ns();
    for (size_t i = 0; i < N; i++) {
        if (dcds_hashmap_put(&m, strs[i], &nums[i]) != DCDS_OK) {
            die("map put");
        }
    }
    uint64_t dt = now_ns() - t0;
    sink += m.len;
    dcds_hashmap_clear(&m);
    return dt;
}

static uint64_t b_map_get(void)
{
    dcds_hashmap m;
    dcds_hashmap_init(&m, NULL);
    for (size_t i = 0; i < N; i++) {
        if (dcds_hashmap_put(&m, strs[i], &nums[i]) != DCDS_OK) {
            die("map put");
        }
    }
    uint64_t sum = 0;
    uint64_t t0 = now_ns();
    for (size_t i = 0; i < N; i++) {
        sum += *(uint64_t *)dcds_hashmap_get(&m, strs[i]);
    }
    uint64_t dt = now_ns() - t0;
    sink += sum;
    dcds_hashmap_clear(&m);
    return dt;
}

static uint64_t b_heap_push_pop(void)
{
    dcds_heap h;
    dcds_heap_init(&h, cmp_u64, NULL);
    uint64_t t0 = now_ns();
    for (size_t i = 0; i < N; i++) {
        if (dcds_heap_push(&h, &nums[i]) != DCDS_OK) {
            die("heap push");
        }
    }
    void *out = NULL;
    uint64_t sum = 0;
    while (dcds_heap_pop(&h, &out) == DCDS_OK) {
        sum += *(uint64_t *)out;
    }
    uint64_t dt = now_ns() - t0;
    sink += sum;
    dcds_heap_clear(&h);
    return dt;
}

static uint64_t b_avl_insert(void)
{
    dcds_avl t;
    dcds_avl_init(&t, cmp_u64, NULL);
    uint64_t t0 = now_ns();
    for (size_t i = 0; i < N; i++) {
        if (dcds_avl_insert(&t, &nums[i], &nums[i]) != DCDS_OK) {
            die("avl insert");
        }
    }
    uint64_t dt = now_ns() - t0;
    sink += t.len;
    dcds_avl_clear(&t);
    return dt;
}

static uint64_t b_avl_get(void)
{
    dcds_avl t;
    dcds_avl_init(&t, cmp_u64, NULL);
    for (size_t i = 0; i < N; i++) {
        if (dcds_avl_insert(&t, &nums[i], &nums[i]) != DCDS_OK) {
            die("avl insert");
        }
    }
    uint64_t sum = 0;
    uint64_t t0 = now_ns();
    for (size_t i = 0; i < N; i++) {
        void *v = NULL;
        if (dcds_avl_get(&t, &nums[i], &v) != DCDS_OK) {
            die("avl get");
        }
        sum += *(uint64_t *)v;
    }
    uint64_t dt = now_ns() - t0;
    sink += sum;
    dcds_avl_clear(&t);
    return dt;
}

static uint64_t b_qsort(void)
{
    uint64_t *copy = malloc(N * sizeof *copy);
    if (copy == NULL) {
        die("malloc");
    }
    memcpy(copy, nums, N * sizeof *copy);
    uint64_t t0 = now_ns();
    qsort(copy, N, sizeof *copy, cmp_u64);
    uint64_t dt = now_ns() - t0;
    sink += copy[0];
    free(copy);
    return dt;
}

typedef struct bench {
    const char *name;
    const char *what;
    uint64_t (*fn)(void);
} bench;

static int cmp_u64_plain(const void *a, const void *b)
{
    return cmp_u64(a, b);
}

static uint64_t median_ns(uint64_t (*fn)(void))
{
    uint64_t runs[REPEATS];
    (void)fn();         /* warm-up, discarded */
    for (int i = 0; i < REPEATS; i++) {
        runs[i] = fn();
    }
    qsort(runs, REPEATS, sizeof runs[0], cmp_u64_plain);
    return runs[REPEATS / 2];
}

static void print_environment(void)
{
    struct utsname u;
    char cpu[128] = "unknown";
    FILE *f = fopen("/proc/cpuinfo", "r");
    if (f != NULL) {
        char line[256];
        while (fgets(line, sizeof line, f) != NULL) {
            if (strncmp(line, "model name", 10) == 0) {
                const char *colon = strchr(line, ':');
                if (colon != NULL) {
                    snprintf(cpu, sizeof cpu, "%.*s", (int)strcspn(colon + 2, "\n"), colon + 2);
                }
                break;
            }
        }
        fclose(f);
    }
    printf("- Compiler: `%s`\n", __VERSION__);
#ifdef __OPTIMIZE__
    printf("- Optimised build: yes (`__OPTIMIZE__` defined)\n");
#else
    printf("- Optimised build: NO - numbers below are not representative\n");
#endif
#if defined(__SANITIZE_ADDRESS__)
    printf("- Sanitizers: ON - numbers below are not representative\n");
#endif
    if (uname(&u) == 0) {
        printf("- OS: %s %s (%s)\n", u.sysname, u.release, u.machine);
    }
    printf("- CPU: %s\n", cpu);
    printf("- Inputs: %u random 64-bit keys (fixed seed), %d timed runs after 1 warm-up, median reported\n", N, REPEATS);
}

int main(void)
{
    strs = malloc(N * sizeof *strs);
    if (strs == NULL) {
        die("malloc");
    }
    for (size_t i = 0; i < N; i++) {
        nums[i] = rng();
        /* The index makes every key distinct, so maps and trees hold exactly N entries. */
        snprintf(strs[i], sizeof strs[i], "key-%zu-%llu", i, (unsigned long long)(nums[i] & 0xffff));
    }

    static const bench benches[] = {
        {"vec push", "append N items, includes growth", b_vec_push},
        {"vec scan", "read N items by index", b_vec_scan},
        {"list push_back", "N node allocations", b_list_push},
        {"list scan", "walk N nodes", b_list_scan},
        {"hashmap put", "N string keys, includes rehash and key copies", b_map_put},
        {"hashmap get", "N hits", b_map_get},
        {"heap push+pop", "N pushes then N pops (heapsort)", b_heap_push_pop},
        {"avl insert", "N random keys", b_avl_insert},
        {"avl get", "N hits", b_avl_get},
        {"libc qsort", "reference: sort N keys once", b_qsort},
    };

    printf("### dcds benchmark\n\n");
    print_environment();
    printf("\n| Benchmark | Notes | ns / op (median) |\n|---|---|---:|\n");
    for (size_t i = 0; i < sizeof benches / sizeof benches[0]; i++) {
        uint64_t ns = median_ns(benches[i].fn);
        printf("| %s | %s | %.1f |\n", benches[i].name, benches[i].what, (double)ns / (double)N);
    }
    printf("\nns/op = total time of the timed section divided by N. Single thread, no pinning.\n");
    free(strs);
    return sink == UINT64_MAX; /* never true; keeps the optimiser from dropping the work */
}
