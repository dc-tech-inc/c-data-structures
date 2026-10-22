# ADR 0004: Open addressing with tombstones for the hash map

Status: accepted

## Context
A hash map needs a collision strategy. Separate chaining allocates a node per entry; open addressing keeps entries in one array.

## Decision
Linear probing over a power-of-two table, FNV-1a (64 bit) hashing, string keys copied on insert, tombstones for deletion. The table is rebuilt (dropping tombstones, and growing if live entries would pass half the new capacity) before used slots, live plus tombstones, would pass 75 % of the capacity. That guarantees every probe sequence meets an empty slot, so lookups of missing keys terminate.

## Consequences
- One allocation per key copy plus the table; good cache behaviour on probes.
- Churn (insert and delete the same number of keys) does not grow the table; `test_tombstone_churn` asserts the capacity stays small after 20 000 rounds.
- FNV-1a is unkeyed. Someone who controls the keys can craft collisions and degrade the map to O(n) per operation. This is documented as a limitation; a keyed hash (SipHash) is the fix and is on the roadmap.
- Only string keys. A generic key would need hash and equality callbacks.
