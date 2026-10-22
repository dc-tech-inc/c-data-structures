# ADR 0003: AVL tree for the ordered map

Status: accepted

## Context
The ordered map needs worst-case O(log n) insert, find and remove. Red-black and AVL both qualify.

## Decision
AVL, implemented recursively with the height stored per node. Removal of a node with two children detaches the in-order successor and puts it in the node's place, so keys and values are never copied between nodes.

## Consequences
- Stricter balance (height at most about 1.44 log2 n against about 2 log2 n for red-black), so lookups touch fewer nodes. Inserts and removals may rotate a little more often than red-black. Neither effect is claimed as a win here; the benchmark prints what the machine measures.
- Fewer cases than red-black deletion. `dcds_avl_validate` checks order, stored heights, balance and the entry count; the tests call it after every 50 random operations and after every removal in the root-removal test.
- Recursion depth equals the tree height (a few dozen frames at most), so there is no stack-overflow risk from the tree shape.
- No parent pointers, so there is no iterator that survives modification; traversal is the callback `dcds_avl_foreach`.
