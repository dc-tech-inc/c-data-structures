/* Minimal unit-test helper (see docs/adr/0002-minimal-test-framework.md). */
#ifndef MINUNIT_H
#define MINUNIT_H

#include <stdio.h>

static int mu_checks;
static int mu_failures;

/* Records a failure and keeps going, so one run reports every broken check. */
#define CHECK(cond)                                                              \
    do {                                                                         \
        mu_checks++;                                                             \
        if (!(cond)) {                                                           \
            mu_failures++;                                                       \
            fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #cond); \
        }                                                                        \
    } while (0)

#define RUN(test)                                         \
    do {                                                  \
        int before = mu_failures;                         \
        test();                                           \
        printf("%-4s %s\n", mu_failures == before ? "ok" : "FAIL", #test); \
    } while (0)

/* Call last in main(): prints the summary and yields the exit status. */
#define MU_REPORT()                                                              \
    (printf("%d checks, %d failed\n", mu_checks, mu_failures), mu_failures != 0)

/* Deterministic xorshift64* so randomised tests are reproducible. */
static unsigned long long mu_rng_state = 88172645463325252ULL;
static inline unsigned long long mu_rand(void)
{
    mu_rng_state ^= mu_rng_state >> 12;
    mu_rng_state ^= mu_rng_state << 25;
    mu_rng_state ^= mu_rng_state >> 27;
    return mu_rng_state * 2685821657736338717ULL;
}

#endif /* MINUNIT_H */
