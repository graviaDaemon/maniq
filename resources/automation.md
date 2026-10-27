# Test automation: one command, every change

Goal of this phase (roadmap phase 6): one command builds `maniq` and runs every test for every stage, a failure tells
you exactly what changed, and the suite runs by itself on every change, without anyone having to remember to.

This guide is different from the earlier ones: most of the machinery already exists, because we grew it alongside
phases 2–5. So it first explains what's there and how it works, then the gaps, then the decisions that are yours.

---

## 1. What "automate" means here

Three levels, each building on the one before:

1. **One command runs everything.** Not "build, then remember the test command, then read 118 lines".
2. **Adding a test is cheap.** If a new sample costs six hand-written files, you'll stop adding samples.
3. **It runs without you.** On every push, someone (a machine) runs the suite and complains when it fails.

The roadmap's **Done when** ("a single command runs the whole suite and reports pass/fail per file") is level 1. Levels 2
and 3 are what make the suite still useful in phase 7, when every feature adds samples.

## 2. What already exists

| Stage (phase) | Golden file                   | What's compared                                |
|---------------|-------------------------------|------------------------------------------------|
| lexer (2)     | `tests/lex/<name>.tokens`     | `maniq lex` stdout                             |
| parser (3)    | `tests/parse/<name>.ast`      | `maniq parse` stdout                           |
| errors (3–4)  | `tests/err/<name>.err`        | `maniq build` stderr, and the exit code        |
| checker (4)   | `tests/types/<name>.types`    | `out.types`                                    |
| codegen (5)   | `tests/c/<name>.c`            | `out.c`                                        |
| run (5)       | `tests/run/<name>.out`        | what the compiled exe prints, and its exit code |

Every golden file is compared with the output for `samples/<name>.mq`. Today that's 118 tests, run with
`ctest --test-dir build`, and CLion runs the same tests from its test window.

## 2.5 What this means for you (after this phase)

**The gap today:** CMake finds tests by looking for *golden files*, not samples. A new `samples/foo.mq` without golden
files gets zero tests, and nothing warns you. This phase closes that hole with two things: blessing (§4.4) writes the
golden files for you, and the completeness check (§4.5) fails when a sample is missing one.

**You only ever write the `.mq` file.** Names, so they don't get mixed up: `out.*` (`out.c`, `out.log`, …) are the
compiler's scratch files, thrown away after every build. Golden files live in `tests/<stage>/<name>.<ext>`.
`out.log` (gcc's messages) is never a golden file.

**A new sample:**

1. write `samples/foo.mq`;
2. bless: the compiler writes `tests/lex/foo.tokens`, `tests/parse/foo.ast`, `tests/err/foo.err` and, if it builds,
   `tests/types/foo.types`, `tests/c/foo.c` and `tests/run/foo.out`;
3. read them (`git diff` shows them as new files). Check `tests/run/foo.out` the most carefully: does the program
   print what you expect?
4. from now on they're locked in. Forget step 2, and the stage tests fail with "missing golden file".

**A change to the grammar or the compiler:**

1. change the code and run the tests;
2. only the tests whose output changed fail, each showing the changed lines;
3. if every change is intended, bless: the affected golden files are rewritten, the others aren't touched;
4. read `git diff tests/`: every changed line must be one you expected.

Renaming a token kind changes every `.tokens` file, and you write none of them: you skim one diff.

> **Important:** a golden file written by the compiler proves "the output didn't change since I approved it", not
> "the output is right". *Right* comes from your review the first time you see it. That's why blessing always ends
> with reading the diff.

## 3. How it works, step by step

Two files do all of it. It helps to follow one test through them.

**`CMakeLists.txt`, when CMake configures.** `golden_tests(run run out)` means: find every `tests/run/*.out`, and
for each one register a test. For `tests/run/v0-good.out` that is a test named `run/v0-good`, whose command is:

```text
cmake -DMANIQ=.../maniq.exe -DCOMMAND=run -DSOURCE=samples/v0-good.mq
      -DGOLDEN=tests/run/v0-good.out -DACTUAL=build/run/v0-good.out -P tests/golden.cmake
```

`cmake -P` runs a CMake file as a *script*, the same way `python file.py` runs Python. The `-D` values are its
arguments. So the test is "run this script with these arguments", and it passes when the script exits with 0.

The `CONFIGURE_DEPENDS` on the glob makes CMake look for new golden files at every build. That's why adding a file is
enough: there's no list to update.

**`tests/golden.cmake`, when the test runs.** It produces the *actual* output, then compares:

1. depending on `COMMAND`: run `maniq lex`/`parse`/`build`, or build in a fresh folder and run the exe;
2. write what came out to `ACTUAL` (`build/run/v0-good.out`);
3. check the exit code: 1 exactly when the output contains `Error[`, 0 otherwise;
4. `compare_files --ignore-eol ACTUAL GOLDEN`: any difference fails the test.

**Check:** break one golden file on purpose (change a `10` to `11` in `tests/run/v0-good.out`), run
`ctest --test-dir build -R run/v0-good`, see it fail, and put it back.

## 4. The gaps

Five things are missing. Each is small; together they're this phase.

### 4.1 One command, including the build

`ctest` only tests what's already built. Forget `cmake --build` first and you're testing an old `maniq`, which passes
(or fails) for the wrong reasons.

CMake has this built in: **workflow presets** in a `CMakePresets.json` next to `CMakeLists.txt`. A preset names a
configure step, a build step and a test step once; then one command runs all three:

```bash
cmake --workflow --preset dev
```

CLion reads the same file, so the IDE and the command line use the same build folder and the same settings. It also
replaces the "how do I build this?" part of the README with one line.

### 4.2 Failures that explain themselves

Today a failing test says "run output differs", with two paths. You then open both files and compare them yourself.
The script should print the **diff** itself, so the CTest output shows exactly which lines changed:

```text
run/v0-good: run output differs
-10
+11
```

`git diff --no-index --color=never <golden> <actual>` does that, and git is always installed where the repo is.

### 4.3 Running tests in parallel

`ctest -j8` runs eight tests at once and takes the suite from seconds to under a second. But today some tests share
a folder: every `build/*` test writes `out.tokens`, `out.c`, `v0-good.exe`, … into `build/` itself, and `maniq parse`
writes `out.ast` there too. Two tests at once overwrite each other's files, and fail at random: the worst kind of test
failure, because it isn't about your code.

The fix is the one the `types`, `c` and `run` tests already use: every test runs in its own folder
(`build/err/v0-good.err.dir/`). Then parallel is safe.

### 4.4 Updating golden files on purpose ("blessing")

When you *mean* to change output (a better error message, a new runtime helper), every affected golden file has to
change too. Today that's by hand, or with a throwaway loop. That's level 2 from §1, and it decides whether you'll
keep adding tests in phase 7.

The usual solution has a name: **blessing** (Rust's compiler calls it `--bless`; snapshot-testing tools call it
"accept"). Run the suite in a mode where a difference *overwrites* the golden file instead of failing:

```bash
cmake --workflow --preset bless
git diff tests/
```

The second command is the important one. Blessing writes whatever the compiler produces, including bugs. **A golden
file is only as good as the review of its diff.** The workflow is: change the code, bless, read the `git diff` of
`tests/`, and every changed line must be one you expected. That's the same check you did by hand for the `.ast` and
`.err` files, only on the diff instead of the whole file.

Blessing can also *create* golden files: a new sample gets its `.tokens`, `.ast`, … written for you, and you review
them as new files. Which stages a new sample should get is §4.5.

### 4.5 Nothing forgotten

Some rules are only in our heads today:

- every `samples/v0-*.mq` has a `.tokens` and an `.ast` file (except samples whose *parse* fails: `MQ1xxxx`, `MQ2xxxx`);
- every `v0-*` sample has an `.err` file (empty when it builds cleanly);
- a sample that builds has `.types`, `.c` and `.out` files; a sample with an error has none of these three;
- every golden file has a sample (CTest already fails when the sample is missing).

**How it's enforced (as built):** the tests are registered from the *samples*, not from the golden files. Every
`v0-*` sample gets all six stage tests (`lex/foo`, `parse/foo`, `err/foo`, `types/foo`, `c/foo`, `run/foo`); any other
sample only `lex/foo`. Each stage test first works out whether its stage *applies* to this sample: `parse` applies
when parsing succeeds, `types`/`c`/`run` when the build succeeds, `lex` and `err` always. Then:

| Stage applies? | Golden file exists? | Normal run                        | Bless                  |
|----------------|---------------------|-----------------------------------|------------------------|
| yes            | yes                 | compare; a difference fails       | overwrite if different |
| yes            | no                  | fail: "missing golden file"       | create it              |
| no             | yes                 | fail: "should not exist"          | delete it              |
| no             | no                  | pass                              | nothing to do          |

One more test, `suite/orphans`, fails on golden files whose sample is gone (bless deletes them). So the rules above
are checked by the stage tests themselves; there's no separate list to keep in sync. (The guide's first draft had a
separate `suite/complete` test; this does the same job with less code.)

It found one on the first run: `samples/v0-example.mq` builds cleanly but had no `.err` or `.types` file. Blessing
created both (the `.err` is empty, as it should be for a clean build).

A crash is never "doesn't apply": an exit code other than 0 or 1 always fails the test.

## 5. Running without you

Level 3. Two places can run the suite automatically, and they do different jobs:

| Where                        | When                       | Catches                                             |
|------------------------------|----------------------------|-----------------------------------------------------|
| a git hook on your machine   | before every `git push`    | you pushing a broken commit                         |
| GitHub Actions (CI)          | on GitHub, after each push | everything, on a clean machine, visible to everyone |

**A git hook** is a script in `.git/hooks/` that git runs at a certain moment. A `pre-push` hook that runs the workflow
preset stops a push when a test fails. It's local: hooks aren't committed or pushed, so it only protects your machine.
(`pre-push` rather than `pre-commit`, so committing stays instant; you commit yourself, and only pushing is gated.)

**GitHub Actions** is the same thing as your `close-prs.yml`, with a different job: on every push, start a fresh Linux
machine, install gcc and CMake, check out the repo, and run the workflow preset. The result shows up as a ✓ or ✗ next
to the commit on GitHub. Public repositories get it for free.

CI does one more thing for you: it runs on **Linux**. Your compiler, the C it generates, and the tests themselves
(paths, line endings, `.exe`) are then proven on a second operating system from now on. That's a head start on phase
10, and it catches Windows-only assumptions the day they're written instead of months later.

> **Pitfall:** the CI machine needs gcc 14 or newer (C++26 for `maniq`, `-std=c23` for the generated C) and CMake
> 3.28 or newer. Which versions the GitHub runner images include changes over time; we check this when we set it up.

## 6. Other designs, and why not (for now)

You'll meet these in other compilers, so it's worth knowing what they are:

- **Expectations inside the sample**, as comments: `print(10); // expect: 10` (Crafting Interpreters' test runner
  works this way; LLVM's `lit` tool uses `// CHECK:` lines). Everything about a test is in one file. The price: the
  runner has to parse those comments, and the samples double as test scripts. We already have 118 tests as sidecar
  files, and sidecars show *every* stage's output, which is what you want while learning.
- **A test runner written in C++** (a second program, or `maniq test`). More control and faster startup. But
  `golden.cmake` already does the job in 50 lines, and CTest gives CLion integration, `-j`, `-R` and `--rerun-failed`
  for free. Worth revisiting in phase 9, when `maniq` grows subcommands anyway.
- **A unit-test framework** (GoogleTest, Catch2) for testing single C++ functions. Golden files test the compiler from
  the outside, which survives refactoring. Unit tests can come later, for the tricky internals (the GC in phase 8).

## 7. Decide first

1. **Blessing:** as a workflow preset (`cmake --workflow --preset bless`), or a separate script? Recommended: the
   preset, so it's one list of commands, also in CLion.
2. **Automatic runs:** the `pre-push` hook, GitHub Actions, or both? Recommended: both, Actions first.
3. **The completeness check (§4.5):** do the rules above match how you think about samples? In particular: should
   the non-v0 samples (`hello.mq`, `arith.mq`, …) stay lexer-only until phase 7 makes them compile?

> **Decided (2026-10-27):** 1. the `bless` preset. 2. both: Actions on every push to any branch (`main` for now,
> `dev`/`accept` once alpha is being tested), and the `pre-push` hook. 3. yes; the non-v0 samples stay lexer-only until
> phase 7 gives them a grammar.

## 8. Order of work

| Step | Change                                                  | Check                                               |
|------|---------------------------------------------------------|-----------------------------------------------------|
| 1    | every test in its own folder (§4.3)                     | `ctest -j8` passes three times in a row             |
| 2    | the diff on failure (§4.2)                              | a broken golden file shows the changed line         |
| 3    | `CMakePresets.json` with `dev` (§4.1)                   | a fresh clone builds and tests with one command     |
| 4    | blessing (§4.4)                                         | delete a golden file, bless, it's back unchanged    |
| 5    | stage tests check their own golden file (§4.5)          | remove an `.err` file: `err/<name>` names it         |
| 6    | GitHub Actions, then the `pre-push` hook (§5)           | a pushed commit shows ✓; a broken one shows ✗       |

**Done when:** `cmake --workflow --preset dev` builds and runs the whole suite in parallel, a failure prints its diff,
blessing updates golden files for review, and every push to GitHub is tested on Linux.

---

## Summary checklist

- [x] the three §7 decisions made
- [x] every test runs in its own folder; `ctest -j` is safe
- [x] a failing comparison prints the diff
- [x] `CMakePresets.json`: `cmake --workflow --preset dev` configures, builds and tests
- [x] blessing writes golden files; the habit: bless, then review `git diff tests/`
- [x] every stage test checks that its golden file exists exactly when the stage applies; `suite/orphans`
- [ ] GitHub Actions runs the suite on Linux on every push (written; confirmed after the first push)
- [x] optional: a local `pre-push` hook
