# hanzoai/yoga

A fork of facebook/yoga (remote `origin` = react/yoga, `fork` = hanzoai/yoga)
carrying CSS Grid on top of upstream, published to npm as **`@hanzo/layout`**.

`main` is upstream's `main` plus grid. Keep it that way: rebase grid work onto a
fresh upstream tip rather than merging upstream into a long-lived grid branch.

## The one grid API

Upstream landed the grid *style layer* itself (`#1893`, `CSS Grid 1/9`) before we
integrated, and it chose an **index-based** C API:

    YGNodeStyleSetGridTemplateColumnsCount(node, 3);
    YGNodeStyleSetGridTemplateColumn(node, 0, YGGridTrackTypePoints, 40);
    YGNodeStyleSetGridTemplateColumnMinMax(node, 1, YGGridTrackTypeAuto, 0,
                                           YGGridTrackTypeFr, 1);

An earlier revision of that same PR used an opaque builder (`YGGridTrackListCreate`
/ `YGPoints` / `YGFr` / `YGMinMax`). **That builder is gone and must not come back.**
Both produced the identical `GridTrackSize`, so there is nothing to recover — and
the index API needs no allocation, which is why the generated tests and the
benchmark no longer leak a list per layout.

Every binding expresses the same model:

| Surface | Shape |
|---|---|
| C | count + one call per index (above) |
| C++ | `Style::resizeGridTemplateColumns` / `setGridTemplateColumnAt` |
| JNI | parallel arrays -> `setGridTracks` helper in `java/jni/YGJNIVanilla.cpp` |
| Kotlin | `YogaGridTrackList` of `YogaGridTrackValue`, typed by `YogaGridTrackType` |
| JS | `GridTrackValue[]`, applied by `setGridTracks` in `wrapAssembly.ts` |

The grid *algorithm* (`yoga/algorithm/grid/`) reads `Style` only, and compiles
against upstream's handle-based `Style` refactor (`#1922`) unchanged.

## gentest: generated files are generated

`gentest/` is the Node/TypeScript generator upstream landed in `#1889`. The C++,
Java and JavaScript emitters are the source of truth for everything under
`tests/generated/`, `java/tests/generated/` and `javascript/tests/generated/`.

Those files are **SignedSource-signed** and `yarn gentest-validate` checks them.
If you edit one by hand, re-sign it — a stale signature fails CI, not the build.

Regenerating needs Chrome via selenium (`gentest/src/ChromePool.ts`); it recomputes
expected layouts from the browser, so a Chrome version bump can legitimately move
numbers in tests unrelated to your change. Prefer changing an emitter and
regenerating only the fixtures you touched.

## Traps that cost real time

- **Upstream fixes hide inside a "grid" diff.** The grid branch predated four
  upstream fixes and silently reverted them on merge: the `./types.ts` import
  casing (`#1929`), the `document.fonts.ready` wait in `gentest/src/cli.ts`, the
  `instrinsic` -> `intrinsic` rename (`#1928`), and `emitTestEpilogue`'s push form.
  When reconciling, diff *each* file against upstream and confirm the result is
  **purely additive** — `diff <(git show main:$f) $f | grep '^-'` should show only
  lines you meant to change.
- **`git reset --soft` leaves everything staged.** A following `git commit` takes
  the whole index, not the paths you just added. Use `git reset` (mixed) between.
- **`git add -A` sweeps untracked build trees.** `build-clang18/`,
  `build-clang21-libcxx/` and `build-jni/` are not ignored. Stage by path.
- **Object corruption reads as a merge failure.** `fatal: unable to read <sha>`
  from `git patch-id`/`diff` means missing objects, not a bad diff. Repair with
  `git fetch fork --refetch '+refs/heads/*:refs/remotes/fork/*'`, then confirm
  `git rev-list --objects grid main` exits 0.

## Verifying

    ./unit_tests Release        # or Debug; both must be green
    cd gentest && npx tsc --noEmit
    cd javascript && npx tsc --noEmit
    npx eslint .

`unit_tests` builds `tests/` only. The count is the check that matters: compare
against a pristine `main` worktree and make sure the upstream test *names* are a
subset, not just that the total went up.

The JNI builds separately (`cmake --build build-jni`, source `java/`); its grid
entry points are `static` and registered through `methods[]`, so they never show
up in `nm -D` — grep the `.so` strings instead.

## Releasing

1. Set `javascript/package.json` `version`, commit, tag `vX.Y.Z`, push both.
2. `.github/workflows/publish-npm-release.yml` runs `yarn publish` in `javascript/`,
   which bootstraps emsdk itself (`installEmsdkTask`) and builds the WASM. No
   emscripten preinstall is needed, and no local publish should ever be run.
3. `NPM_TOKEN` comes from the hanzoai **org** secrets; there is none on the repo.

The package name is also the specifier every test, the generator and the website
workspace import, so it only ever moves as one piece.

**On a fork, a tag push does not start a workflow** — GitHub suppresses the `push`
event until the repo owner enables Actions for the fork in the Actions tab.
`gh workflow run publish-npm-release.yml --ref main` works regardless and is the
reliable trigger.
