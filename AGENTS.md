# Project Agent Guidance

This fork is incrementally modernizing Zelda64Recomp's renderer while preserving the original N64 path as the compatibility reference.

## Required context

- Read `HANDOFF.md` before substantial implementation work; it records the current build and runtime state.
- Read `docs/DECISIONS.md` before changing renderer/game ownership boundaries.
- The primary technical pre-research for the first graphics upgrade is `_working-directory/research/deep-research-report.md` when that local file is available.

## Renderer boundaries

- `zeldaret/mm` semantics (or this repository's `lib/mm-decomp` snapshot) are the source of truth for Majora's Mask behavior.
- Zelda64Recomp owns Majora-specific semantic extraction and adaptation.
- RT64 owns generic raster-renderer features.
- Keep Native/RDRAM rendering unchanged as the compatibility reference. Modern fog is gated to the enhanced framebuffer replay.
- Frame-global MM environment semantics belong on RT64 Workloads. Keep Extended GBI for draw- or transform-local data. See ADR-002.
- Local actor/effect fog remains authoritative per draw. Atmospheric mode may replace only a draw that matches the resolved environment baseline; other fogged draws use faithful per-pixel evaluation. See ADR-003.

## RT64 integration

- The qualified upstream baseline is RT64 `5473732a822a4423b5696e7cb18fecc425a59875`; local renderer work is based directly on it.
- Existing MatrixGroup sites use behavior-neutral texcoord and LookAt components (`G_EX_COMPONENT_SKIP`) and automatic aspect behavior until a specific path is validated.
- Do not implement Float Projection from only `fovy/aspect/zNear/zFar`: MM applies `View_StepDistortion` after `guPerspective`. Preserve the final distorted matrix exactly or leave the fixed path in place.
- Current upstream clipping, framebuffer, rect, and LOD fixes are part of the baseline. Keep Zelda's existing `forceBranch` and resolution-scaled texture LOD policy unless regression evidence says otherwise.

## Build hygiene

- Keep ROMs, generated recompilation sources, build products, captures, and local diagnostics under ignored/disposable paths; never commit them.
- `_working-directory/` is scratch space, not persistent project memory.
- On this Windows checkout, Git's shell helpers may require `C:\Program Files\Git\usr\bin` and `C:\Program Files\Git\mingw64\bin` on `PATH` plus `MSYS2_PATH_TYPE=inherit`.
- Full Windows application builds should use Clang/clang-cl; upstream ultramodern currently emits Clang warning flags that `cl.exe` rejects.
- Base-ROM generation uses the N64Recomp nested under N64ModernRuntime, including this fork's `osSetTime` reimplementation fix. The older CI-pinned N64Recomp commit `a13e5cff96686776b0e03baf23923e3c1927b770` is still required for this checkout's strict `patches.toml`; do not swap the two roles without retesting both generators. See `HANDOFF.md` for current build details.
- A full clean build requires Clang/LLD for MIPS patches and compatible US-ROM-generated sources. Targeted Zelda C++ objects and the embedded `rt64` target can still validate renderer integration without those inputs.

## End-of-session memory

- Rewrite `HANDOFF.md` with current facts after substantial work.
- Add durable cross-session operating lessons here only when they are broadly reusable.
- Record consequential architecture choices in `docs/DECISIONS.md`; do not duplicate their full rationale here.
- Follow the repository's release-note workflow for user-visible release history; do not use a changelog as an agent log.
