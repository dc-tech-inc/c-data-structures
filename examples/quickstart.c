/* Word frequencies with hash map + AVL + heap: count words, list them alphabetically, print the top two. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dcds/avl.h"
#include "dcds/hashmap.h"
#include "dcds/heap.h"

typedef struct entry {
    const char *word; /* key owned by the hash map; valid while the map lives */
    int count;
} entry;

static int by_word(const void *a, const void *b)
{
    return strcmp(a, b);
}

static int by_count_desc(const void *a, const void *b)
{
    const entry *x = a;
    const entry *y = b;
    return (y->count > x->count) - (y->count < x->count); /* heap pops the "smallest": most frequent first */
}

static void alphabetical(const void *key, void *value, void *ctx)
{
    (void)ctx;
    printf("  %-6s %d\n", (const char *)key, ((entry *)value)->count);
}

static void to_heap(const char *key, void *value, void *ctx)
{
    (void)key;
    if (dcds_heap_push(ctx, value) != DCDS_OK) {
        fputs("out of memory\n", stderr);
        exit(1);
    }
}

int main(void)
{
    const char *words[] = {"the", "cat", "sat", "on", "the", "mat", "the", "cat"};
    dcds_hashmap counts;
    dcds_hashmap_init(&counts, free); /* the map owns the entry structs */

    for (size_t i = 0; i < sizeof words / sizeof words[0]; i++) {
        entry *e = dcds_hashmap_get(&counts, words[i]);
        if (e == NULL) {
            e = calloc(1, sizeof *e);
            if (e == NULL || dcds_hashmap_put(&counts, words[i], e) != DCDS_OK) {
                fputs("out of memory\n", stderr);
                return 1;
            }
        }
        e->count++;
    }

    /* Ordered view: AVL keyed by word. Keys point at the static strings above, values at the map's entries. */
    dcds_avl sorted;
    dcds_avl_init(&sorted, by_word, NULL);
    for (size_t i = 0; i < sizeof words / sizeof words[0]; i++) {
        entry *e = dcds_hashmap_get(&counts, words[i]);
        e->word = words[i];
        if (dcds_avl_insert(&sorted, (void *)words[i], e) != DCDS_OK) {
            return 1;
        }
    }
    puts("alphabetical:");
    dcds_avl_foreach(&sorted, alphabetical, NULL);

    dcds_heap top;
    dcds_heap_init(&top, by_count_desc, NULL);
    dcds_hashmap_foreach(&counts, to_heap, &top);
    puts("top 2:");
    for (int i = 0; i < 2; i++) {
        void *out = NULL;
        if (dcds_heap_pop(&top, &out) != DCDS_OK) {
            break;
        }
        printf("  %-6s %d\n", ((entry *)out)->word, ((entry *)out)->count);
    }

    dcds_heap_clear(&top);
    dcds_avl_clear(&sorted);
    dcds_hashmap_clear(&counts);
    return 0;
}
