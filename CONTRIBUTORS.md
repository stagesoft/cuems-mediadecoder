<!--
SPDX-FileCopyrightText: 2026 Stagelab Coop SCCL
SPDX-License-Identifier: GPL-3.0-or-later
-->

# Contributing to cuems-mediadecoder

Thank you for your interest in `cuems-mediadecoder`. This library is linked into
**cuems-videocomposer** and **cuems-audioplayer** on every CueMS player node — a
regression in demuxing or decoding can break show playback. These guidelines exist to
keep contributions reliable while the project remains open to external patches.

---

## Table of Contents

1. [Prerequisites](#1-prerequisites)
2. [Development Setup](#2-development-setup)
3. [Contribution Tiers](#3-contribution-tiers)
4. [Branch Naming](#4-branch-naming)
5. [Spec-First Requirement](#5-spec-first-requirement)
6. [TDD Workflow — Non-Negotiable](#6-tdd-workflow--non-negotiable)
7. [Commit Hygiene](#7-commit-hygiene)
8. [Developer Certificate of Origin (DCO)](#8-developer-certificate-of-origin-dco)
9. [Pull Request Requirements](#9-pull-request-requirements)
10. [Acceptance Criteria](#10-acceptance-criteria)
11. [Review Process](#11-review-process)
12. [Changelog Line](#12-changelog-line)
13. [Dependency Governance](#13-dependency-governance)
14. [License](#14-license)

---

## 1. Prerequisites

| Tool | Version | Notes |
|---|---|---|
| C++ compiler | g++ ≥ 12 (C++17) | clang++ ≥ 14 also supported |
| CMake | ≥ 3.10 (≥ 3.16 recommended) | matches `cmake_minimum_required` in `CMakeLists.txt` |
| pkg-config | any recent | required to locate FFmpeg |
| Git | any recent | DCO sign-off required (see §8) |
| FFmpeg dev packages | distro FFmpeg 4.x–6.x | `libavformat`, `libavcodec`, `libavutil`; `libswscale` and `libswresample` strongly recommended |
| lcov / gcov | optional | only needed to produce coverage locally once tests exist |

System packages (Debian/Ubuntu example):

```bash
sudo apt-get install -y \
  build-essential cmake pkg-config \
  libavformat-dev libavcodec-dev libavutil-dev \
  libswscale-dev libswresample-dev
```

This repository has **no git submodules**. Parent projects (`cuems-videocomposer`,
`cuems-audioplayer`) consume it via `add_subdirectory` or as a submodule in their
own trees.

---

## 2. Development Setup

```bash
git clone https://github.com/stagesoft/cuems-mediadecoder.git
cd cuems-mediadecoder

# Configure (debug build)
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# Build the static library
cmake --build build -j$(nproc)
```

Lint (apply before opening a PR):

```bash
# Format check — exits non-zero if any tracked C++ file would change
clang-format --dry-run --Werror $(git ls-files '*.cpp' '*.h')

# Static analysis on changed files (after configuring with compile_commands.json)
clang-tidy -p build $(git diff --name-only main -- '*.cpp')
```

**Tests (aspirational):** there is not yet a `tests/` tree or `enable_testing()` in
`CMakeLists.txt`. When tests land, the expected workflow will be:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure
```

Coverage build (once `ctest` targets exist):

```bash
cmake -B build \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="--coverage" \
  -DCMAKE_EXE_LINKER_FLAGS="--coverage"
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure
lcov --capture --directory build --output-file coverage.info --ignore-errors inconsistent
lcov --remove coverage.info '/usr/*' '*/tests/*' --output-file coverage.info
```

Embedding in a parent project:

```cmake
add_subdirectory(path/to/cuems-mediadecoder)
target_link_libraries(your_target PRIVATE cuems-mediadecoder)
```

---

## 3. Contribution Tiers

The review requirements depend on what you change.

### Tier 1 — Trivial

No change to logic under `src/` or `include/` beyond a one-line correction. Covers:
README/CHANGELOG/CONTRIBUTORS edits, comment fixes, typo fixes in headers, CI/packaging
config only.

**Gates:** lint (when CI exists) + green build; one owner approval. No spec required.

### Tier 2 — Non-trivial

Any addition, modification, or deletion of behaviour in `src/` or `include/`. Includes
bug fixes that change decoding or seeking semantics, new public methods, FFmpeg API
migrations, and performance changes.

**Gates:** spec + plan + tasks on the branch (when the spec kit is adopted in this
repo); failing test before implementation (once tests exist); CI green; constitution
compliance where applicable; one mandatory owner approval.

---

## 4. Branch Naming

```
feat/NNN-short-description       ← new feature  (NNN = spec number when used)
fix/NNN-short-description        ← bug fix referencing a spec or issue number
chore/short-description          ← non-production changes (CI, tooling, docs)
build/short-description          ← packaging / build-system changes
```

Branches without a sensible prefix may be returned for rename before merge.

---

## 5. Spec-First Requirement

For **Tier 2** changes, before opening a PR for review you SHOULD commit specification
artifacts on your feature branch. The CueMS family uses directories such as:

```
specs/NNN-feature/spec.md
specs/NNN-feature/plan.md
specs/NNN-feature/tasks.md
```

This repository does not yet ship a `.specify/` tree; until it does, open a
[GitHub issue](https://github.com/stagesoft/cuems-mediadecoder/issues) describing the
behaviour, edge cases, and parent-project impact (`cuems-videocomposer` /
`cuems-audioplayer`) and link that issue from the PR.

The PR description must link to the spec or issue. A Tier 2 PR without design context
will be marked draft and returned for pre-work.

---

## 6. TDD Workflow — Non-Negotiable

For all **Tier 2** changes, follow Test-Driven Development once the test harness
exists (it is planned but not yet present):

```
1. Write a failing test that precisely describes the intended behaviour.
2. Confirm CI fails on that commit (or run ctest locally and record the failure).
3. Write the minimum production code required to make the test pass.
4. Refactor without changing observable behaviour, keeping all tests green.
```

Your git log on the feature branch MUST show this order. The PR template (when added)
will ask for the commit SHA of your failing-test commit.

Until `ctest` targets exist, document manual verification steps in the PR (sample
media file, URL, or device path used, FFmpeg version, and observed vs expected
behaviour).

---

## 7. Commit Hygiene

`cuems-mediadecoder` uses [Conventional Commits](https://www.conventionalcommits.org/) v1.0.

```
<type>[optional scope]: <description>

[optional body]

[optional footer(s)]
Signed-off-by: Your Name <your@email.com>
```

Allowed types: `feat`, `fix`, `test`, `refactor`, `docs`, `chore`, `ci`, `perf`,
`build`, `patch`, `spec`.

Breaking changes: append `!` after the type and include a `BREAKING CHANGE:` footer
when public headers change incompatibly.

Rules:

- Each commit MUST represent one logical change.
- Do not squash unrelated changes into a single commit.
- Force-pushing to `main` is forbidden.

---

## 8. Developer Certificate of Origin (DCO)

Every commit must carry a `Signed-off-by` line, asserting that you have the right to
submit the contribution under the project's license (LGPL-3.0-or-later for library
source), as per the
[Developer Certificate of Origin](https://developercertificate.org).

```bash
git commit -s -m "fix: handle EOF after flush in VideoDecoder"
```

To add sign-off to all commits in a branch automatically:

```bash
git config --local format.signOff true
```

PRs that contain unsigned commits will not be merged.

---

## 9. Pull Request Requirements

Open your PR against **`main`**.

Every PR MUST include in its description:

1. **Summary** — what changed and why (2–5 sentences).
2. **Changelog Line** — see §12.
3. **Spec / issue links** (Tier 2) — link to `specs/NNN-feature/` or a GitHub issue.
4. **Failing-test commit SHA** (Tier 2, when tests exist) — commit where CI was red
   before implementation.
5. **Parent-project impact** — whether `cuems-videocomposer` and/or `cuems-audioplayer`
   require matching changes.
6. **Verification** — media samples, FFmpeg version, and manual test notes until CI
   covers the change.

Draft PRs are welcome for early feedback. Convert to Ready when all gates pass.

---

## 10. Acceptance Criteria

A PR is ready to merge when ALL of the following are true:

| Criterion | How verified |
|---|---|
| Spec or issue linked (Tier 2) | Reviewer reads linked artifacts |
| Failing test before implementation (Tier 2, when tests exist) | SHA in PR; git log order |
| `cmake --build` succeeds on Ubuntu CI | CI green (when workflow exists) |
| `ctest` passes (when tests exist) | CI green |
| Coverage gate on changed files (when enabled) | Codecov (planned) |
| `clang-format --dry-run --Werror` on changed C++ files | CI lint gate (planned) |
| `clang-tidy` no new warnings on changed files | CI lint gate (planned) |
| No new `pkg_check_modules` without justification | Reviewer checks `CMakeLists.txt` |
| SPDX header on all new source files matching LGPL-3.0-or-later | Reviewer inspects files |
| DCO sign-off on all commits | GitHub DCO check |
| At least one owner approval | GitHub branch protection |

---

## 11. Review Process

All PRs to `main` require approval from at least one repository owner:

- **Ion Reguera** ([@ibiltari](https://github.com/ibiltari))
- **Adrià Masip** ([@backenv](https://github.com/backenv))

**What owners check:**

- Behaviour matches the spec or issue (Tier 2).
- TDD sequence is evidenced when tests exist.
- Public API changes are documented in `README.md`.
- No undocumented FFmpeg ABI assumptions (version guards where needed).
- SPDX `LGPL-3.0-or-later` header on new `.h` / `.cpp` files.
- Optional dependencies (`libswscale`, `libswresample`) remain optional at CMake level
  only if the implementation truly compiles without them.

Expect review turnaround within 5 business days. For urgent playback fixes, open an
issue and tag a maintainer.

---

## 12. Changelog Line

You do not edit `CHANGELOG.md` at merge time for every PR — maintainers consolidate at
release. Include a **Changelog Line** in your PR description:

```
[TYPE] Past-tense sentence describing what changed and why it matters to integrators.
```

Types: `Added`, `Changed`, `Fixed`, `Removed`, `Security`, `Performance`.

Examples:

```
[Added] MediaFileReader::isSeekable() reports AVIO_SEEKABLE_NORMAL from the demuxer.
[Fixed] VideoDecoder no longer enables slice threads for AV1 after seek.
[Changed] AudioDecoder defaults SwrContext output to AV_SAMPLE_FMT_FLT planar disabled.
```

---

## 13. Dependency Governance

No new entry under `pkg_check_modules(...)` in `CMakeLists.txt` may be introduced
without:

1. A written justification in the PR (why FFmpeg's existing libraries are insufficient).
2. Explicit acknowledgement from a repository owner.

Current **required** pkg-config modules:

- `libavformat`, `libavcodec`, `libavutil`

Current **optional** modules (define `HAVE_SWSCALE` / `HAVE_SWRESAMPLE` when found):

- `libswscale` — used by `VideoDecoder` format conversion helpers
- `libswresample` — used by `AudioDecoder` resampling helpers

Parent projects remain responsible for hardware device setup (VA-API, CUDA, etc.),
`libsoxr` quality resampling in **cuems-audioplayer**, and show-level threading policy.

---

## 14. License

Library source in this repository is licensed under **GNU Lesser General Public License
v3.0 or later (LGPL-3.0-or-later)**. Documentation files (`README.md`, `CHANGELOG.md`,
this file) use **GPL-3.0-or-later** for the SPDX tag on Markdown, per StageLab
convention. The repository root [LICENSE](./LICENSE) file contains the full **GPL v3**
text (shared with other CueMS repos such as `cuems-dmxplayer`).

By contributing C++ code, you agree your contributions are licensed under
**LGPL-3.0-or-later**.

New `.h` / `.cpp` files MUST carry:

```cpp
/*
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * Copyright (C) <year> Stagelab Coop SCCL
 * ...
 */
```

New Markdown files:

```markdown
<!--
SPDX-FileCopyrightText: <year> Stagelab Coop SCCL
SPDX-License-Identifier: GPL-3.0-or-later
-->
```
