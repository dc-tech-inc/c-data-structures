# Security policy

This is a small C library. A memory-safety bug in it is a security bug for anyone who links it.

## Reporting a vulnerability

Use GitHub's private vulnerability reporting ("Security" tab, "Report a vulnerability") instead of a public issue. We aim to acknowledge reports within a few business days.

Useful reports: an out-of-bounds read or write, a leak or double free reachable through the public API, integer overflow in a size calculation, or a way to make a documented O(log n) operation degrade (hash flooding of the string-keyed map is a known limitation, see the README).

## Scope

In scope: everything in this repository (`include/`, `src/`, the build files and the CI workflows). Out of scope: third-party tools the CI downloads (gitleaks, valgrind, compilers).

## Supported versions

Only the latest commit on `main`. There are no tagged releases yet.

## Not covered

The library is not thread-safe (no operation may run concurrently with another on the same container) and does not zero memory before freeing it. Do not store secrets in it expecting either property.
