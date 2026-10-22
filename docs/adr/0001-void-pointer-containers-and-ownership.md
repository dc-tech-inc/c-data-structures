# ADR 0001: Containers of `void *` with one ownership rule

Status: accepted

## Context
C has no generics. The usual options are macros that stamp out a type per element, a container that copies `elem_size` bytes, or a container of `void *`.

## Decision
Every container stores `void *`. The container never dereferences or copies an item. A single, uniform ownership rule applies (documented in `include/dcds/common.h`): a non-NULL free function makes the container the owner; anything that hands an item back through an `out` pointer transfers ownership to the caller; a NULL `out` releases the item.

## Consequences
- One implementation per container, readable headers, no macro tricks, and it works with `-Wpedantic`.
- No type safety and one pointer of indirection per element. Storing small values means boxing them (the benchmarks store pointers into one array, which flatters the vector). A caller who needs value semantics wraps the container.
- `dcds_vec_get` and `dcds_hashmap_get` cannot tell a stored `NULL` from "absent"; `dcds_hashmap_contains` and the `dcds_avl_get` status cover the cases where that matters.
