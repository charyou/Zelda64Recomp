# Handoff

> Current state: 2026-09-09. Hardware RT primary-hit vertical slice implemented and visibly validated on Vulkan. Stop at this foundation; production lighting is separate work.

## Current deliverable

- Parent work branch: `codex/rt64-raytracing`, base HEAD `ef88a580f28776ae8a984a151bfa17178a9e2d7c`.
- RT64 work branch: `codex/raytracing`, base HEAD `cfbe357c82c6b7611f46d5252e89b97333d1d0ed`.
- Changes are uncommitted in the parent, RT64, and **RT64's nested `src/contrib/plume` submodule**. Commit/preserve all three layers when checkpointing. No submodule resets or upstream updates occurred. Prepared research and `lib/rt64 und md files.zip` were already untracked and are preserved.
- Executable: `_working-directory/build-zelda-validation/Zelda64Recompiled.exe`.
- SHA-256: `925F6959DD675608C9A14FEC5739B6A47DCA51D0896C0FC38A21DC6D7C6068BC`.
- Isolated runtime copy: `_working-directory/diagnostics/2026-09-07-coverage/runtime/rt-primary-hit.exe`.
- Launch with `RT64_RT_PRIMARY_HIT=1` for the cyan-bordered upper-left primary-hit inset. Without it, normal Enhanced rendering remains the default. The existing F2 / `DeveloperShortcut::RayTracing` route is wired to the same state, but keyboard injection did not visibly toggle it during this test; use launch-time A/B for reproducible validation.

## Required implementation handoff

Read **`docs/RAYTRACING_FOUNDATION.md`** for exact files/symbols, geometry gates, resource lifetimes, shader bindings/SBT, backend fixes, commands, hashes, failures and runtime evidence. ADR-008 records the durable decision.

The thesis held: reuse Plume and the surviving Workload/framebuffer insertion points; reconstruct only a small RT64 owner and minimal RT shader library. `RT_ENABLED` remains undefined/disabled. No legacy DI/GI restoration, alternative backend abstraction, CPU geometry upload or raster world-position varying.

Actual flow:

```text
Workload + current/HFR transforms and velocity
-> existing VertexProcessor / RSPWorldCS worldPosBuffer
+ faceIndicesBuffer / supported executable indexed draw ranges
-> multi-geometry world-space BLAS
-> identity TLAS
-> isolated SceneBVH descriptor / PrimaryHitRT pipeline / SBT
-> traceRays
-> barycentric hit / dark-miss RGBA8 texture
-> target-matched raster copy into Enhanced inset
-> normal resolve / presentation
```

Eligibility is semantic: one perspective projection per framebuffer, indexed opaque depth-tested/depth-writing triangles, no alpha-compare/coverage-alpha/framebuffer alpha blend, no VertexTestZ-rewritten indices. Unsupported content keeps its raster rendering. BLAS/TLAS rebuild each enabled replay from presentation-time world positions; unmatched frames now request the same existing world processor. No actor/scene/mod identities. The debug TLAS is deliberately double-sided, and the diagnostic depth interval is NDC 0 to 0.99; this is not full raster clipping or a production shadow material policy.

Plume fixes: per-geometry Vulkan BLAS ranges instead of an erroneous single aggregate range, queried scratch alignment, consistent TLAS query/build flags, and optional AS-object reference for Vulkan's actual AS device address. These live in the nested Plume submodule.

## Validation completed; do not broaden this run

- Documented project-local Clang 19.1.3/LLD/Ninja build passed via `pwsh -NoProfile -ExecutionPolicy Bypass -File _working-directory/diagnostics/2026-09-06/build-control.ps1`.
- New RT DXIL/SPIR-V and C/header wrappers actually generated. No raster ABI or raster shader sources changed. Build log: `_working-directory/diagnostics/2026-09-09-rt/build-address.log`.
- Vulkan on AMD Radeon RX 9070 XT, save `a` in Town, copied real mod profile, per-pixel lighting + Atmospheric fog, cutout-AA explicitly disabled. `run-rt.ps1` in that evidence directory restores the copied seed and uses the existing finite Town playback.
- `primary-hit-town.png`: real ray intersections visibly reconstruct Town architecture, ground and animated Link inside the inset. Logs confirm hardware pipeline, 256-byte SBT, BLAS/TLAS and `traceRays`. The first logged 4-mesh/96-triangle count is an early title submission, not the Town geometry count.
- `baseline-off-town.png`: same binary relaunched with `run-rt.ps1 -Name baseline-off -Off`; normal Enhanced rendering restored without RT initialization. Captures are not frame-matched. No severe visible corruption or device-loss errors in the corrected focused run.
- Initial candidate encountered Vulkan device loss and the user reported a driver crash with known preexisting instability. The crash alone does not prove attribution, but a concrete backend multi-geometry AS range defect was found and repaired. Do not reuse the initial `rt-on` candidate as a working result.
- Native screenshot helper crashed once; reset/rebind recovered it. Sandboxed launches were not targetable; the documented interactive desktop launch worked. See `docs/RUNTIME_VALIDATION.md` and the computer-use skill.
- No validation-layer-enabled run. D3D12 backend and DXIL compiled, but D3D12 runtime, MSAA, broad scene transitions, HFR edge cases and performance remain unqualified. No `bastian` scene tour.
- Test game processes stopped; copied seed save restored. Source profile was not used for game writes. Seed SHA remains `B12F0C6F5546C59DF8E9CD26970A81F8F7CD11803E9F7D4E2E13E6D03D8D1C9B`.

## Baseline constraints retained

Read `docs/PER_PIXEL_LIGHTING.md` and ADR-007. Authored normal magnitude, equivalent light values and shared-matrix lighting remain in place. Genuine mixed lights/transforms and true positional microcode lights still fall back. Normal magnitude remains `TEXCOORD1`; RDPParams is 336 bytes. F1 Lighting and `ZELDA64RECOMP_LIGHTING=original` remain the lighting A/B controls. Native uses original shading.

**Shader build correction remains critical:** bundled DXC `-MD` emits dependencies only, not objects. Current CMake compiles first, then runs a separate `-M -MF` dependency pass. Never infer regenerated shader binaries from a logged generation step alone. Earlier RDP320/336 mismatches caused severe corruption. Dynamic directional-light cases remain explicitly unrolled for re-spirv compatibility.

Modern fog/camera behavior is unchanged. Original remains the persistent fog default; Faithful/Atmospheric retain their semantic camera/signature gates. Preserve ADR-006's withdrawal of camera-basis publication. Water influence and conservative skyless atmosphere remain intact. Do not repeat atmosphere research or qualification without new evidence.

The parked cutout-MSAA prototype is still unqualified and defaults on for eligible MSAA draws; validation explicitly used `RT64_CUTOUT_AA=original`. Previous local launcher and evidence remain in `2026-09-07-coverage`. Historical good/bad build oracles are preserved; `_working-directory/build-zelda-clang` remains the bad historical oracle. Base-ROM generation uses the corrected runtime recompiler; patch generation uses the documented CI-pinned tool. Do not run competing patch candidates concurrently.

## Immediate next coding step

Preserve this working primary-hit reference. Add a second **debug** visibility ray toward an existing directional RSP light at a primary hit, reusing the same TLAS/pipeline/output. Production shadows must first resolve culling, clipping, alpha and receiver semantics; GI, reflections, denoising and object-space caching are not prerequisites.
