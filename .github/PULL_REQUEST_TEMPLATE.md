<!--
Thanks for the PR! Please read CONTRIBUTING.md and AI.md first.

The checkboxes below mirror AI.md. They are not optional: PRs that
skip required items will be returned for changes.
-->

## Summary

<!-- 1-3 sentences. What does this PR change and why? -->

Closes #

## Changes

<!-- Bullet list of notable changes. For renames / API changes,
     also list downstream call sites that were updated. -->

-

## Type of change

- [ ] feat — new feature
- [ ] fix — bug fix
- [ ] refactor — no behavior change
- [ ] perf — performance improvement
- [ ] docs — documentation only
- [ ] test — test-only
- [ ] chore / ci / build

## Verification

<!-- How did you confirm this works? For rendering / interaction changes,
     state how you verified QML and Widget parity (AI.md §3.1). -->

- [ ] `cmake -S . -B build && cmake --build build` succeeds locally
- [ ] `ctest --test-dir build --output-on-failure` passes
- [ ] `clang-format --dry-run --Werror -i $(find src tests examples -name '*.cpp' -o -name '*.h')` clean
- [ ] `cppcheck @cppcheck.options src` clean

## AI.md compliance

- [ ] §1.1 Apache-2.0 header on new files; no Qt / GPL source copied
- [ ] §1.3 Targets Qt 6.7+, no deprecated API
- [ ] §1.4 Effective C++ (const correctness, `explicit`, `enum class`, RAII, virtual dtor only when needed)
- [ ] §1.5 File name == class name (CamelCase); global/macro headers lowercase
- [ ] §2.1 Branch from a single issue; no direct `main` push
- [ ] §2.3 Commit / PR references issue (`Refs #N` or `Closes #N`)
- [ ] §3.1 View/interaction changes update **both** QML and Widget + parity stated
- [ ] §3.2 No enum / magic-number duplication
- [ ] §3.3 Parsers validate, not just check presence
- [ ] §3.4 Disabled workarounds removed in the same PR
- [ ] §3.5 Single-concern branch (or exception §3.5.2 justified below)
- [ ] §3.6 Bug fix includes a regression test
- [ ] §4.2 No hard-coded version strings (CMakeLists.txt is the single source)

### §3.5.2 exception (if applicable)

<!-- If this PR bundles tightly-coupled changes, justify here. -->

## Notes for reviewer

<!-- Anything reviewers should pay attention to, risky parts, etc. -->

## Screenshots / captures

<!-- For UI changes: before/after, GIFs for animations. -->
