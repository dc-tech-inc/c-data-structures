# Contributing

1. Branch from `main` (`feat/<scope>` or `fix/<scope>`).
2. Use [conventional commits](https://www.conventionalcommits.org/) (`feat:`, `fix:`, `test:`, `docs:`, `ci:`, `build:`, `chore:`).
3. Run `make test` and `make test SAN=1` (and `CC=clang` if you have it) before pushing. `make valgrind`, `make coverage` and `make example` are available too.
4. New behaviour needs tests in `tests/`; keep randomised tests deterministic (use `mu_rand`). Public functions need a doc comment in the header that states the complexity and the ownership of items.
5. Warnings are errors (`-Wall -Wextra -Wpedantic -Wshadow -Wconversion`). Do not add suppressions without a comment saying why.
6. Open a pull request. All CI checks must be green; merges use a merge commit.

Never commit secrets, credentials, personal data or code you do not have the right to publish. CI runs gitleaks on the whole history.
