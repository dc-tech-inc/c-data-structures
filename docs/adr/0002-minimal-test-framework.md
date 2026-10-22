# ADR 0002: A small test header instead of Unity or greatest

Status: accepted

## Context
Unity and greatest are fine C test frameworks. Both are third-party code that would need to be vendored (or fetched at build time, which breaks offline builds and needs a pinned hash).

## Decision
`tests/minunit.h` provides `CHECK`, `RUN`, a summary and a deterministic xorshift64* generator. `CHECK` records a failure and continues, so one run reports every broken assertion with `file:line`. Each module has its own test executable, registered with CTest and the Makefile.

## Consequences
- No dependency to vendor, update or audit; the whole framework fits in one screen.
- No test discovery, fixtures, mocking or XML reports. If the suite grows past what this supports, switching to Unity is a mechanical change because the test bodies only use `CHECK`.
- Randomised tests use a fixed seed, so a failure reproduces exactly. They compare each container with a trivially correct model (array, presence table, counts).
