# ADR 0005: What CI checks, and why those tools

Status: accepted

## Context
The library is plain C, so the usual failure modes are undefined behaviour, memory errors and leaks, plus portability differences between compilers. The repository starts private on a Free plan with a monthly Actions minutes budget.

## Decision
Jobs on every pull request and every push to `main` (all `ubuntu-latest`):
- `make test` for gcc and clang, each with and without AddressSanitizer + UndefinedBehaviorSanitizer (`-fno-sanitize-recover=undefined`, so UB fails the test), with `-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Werror`.
- CMake + CTest for both compilers (the integration path for other projects).
- Valgrind (memcheck, leaks are errors) on an unsanitized gcc build; sanitizers and valgrind do not combine, and each catches a few things the other misses.
- cppcheck and clang-tidy (`.clang-tidy`, warnings are errors).
- gcov/lcov coverage of `src/`, failing below 90 % lines.
- The benchmark, run with gcc and clang, written to the job summary and an artifact.
- gitleaks over the full history, from a release pinned by version and verified by sha256.

CodeQL is committed and runs once the repository is public (it is skipped while private).

## Consequences
- About a dozen short jobs per run. If the minutes budget becomes a problem the benchmark and the CMake job are the first to move to pull-request-only.
- Benchmark numbers come from shared GitHub-hosted runners, so they are noisy. They are quoted with the run id and compared only within a single run, never across runs.
- gcc is not installed on the maintainer's Windows machine; local verification used WSL, but CI is the reference result.
