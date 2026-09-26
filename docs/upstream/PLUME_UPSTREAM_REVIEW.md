# Plume upstream review — uncommitted D3D12 changes (2026-09-26)

This is working input for the Git checkpoint task that follows. It classifies every uncommitted Plume hunk and recommends whether and how to propose it upstream. Nothing has been proposed upstream, no PR has been opened, and no remote has been changed.

## Scope and baselines

- **Local Plume:** `lib/rt64/src/contrib/plume`, branch `codex/raytracing-as-fixes`, HEAD `259e254`.
  - Uncommitted changes are in `plume_d3d12.cpp` and `plume_d3d12.h` only.
- **Upstream:** `https://github.com/renderbag/plume.git`. `main` = `d72379344dac` (`git ls-remote`, 2026-09-26).
  - The local `origin/main` ref is stale (`d890ac8`, 2026-07-22).
  - The upstream statements below were checked against a raw download of upstream `main` `plume_d3d12.cpp`. Line numbers refer to that file.
- **Out of scope:** four fork commits are already committed on top of the stale base: `f425d3e`, `28f2fe9`, `91e6711`, `259e254` (Vulkan AS build contracts, timestamp query diagnostics, Vulkan cropped readback, in-place BLAS updates). They are not reviewed here.
- **Commit mechanics:** index-only patches `plume-P1.patch` … `plume-P4.patch`, verified to reproduce the working tree exactly. They are in `_working-directory/diagnostics/2026-09-26-d3d12-perpixel/commit-plan/`. `COMMIT_PLAN.md` there has the full RT64 and parent plan.

## Classification key

- **A — generic Plume backend/API correctness bug:** independently defensible for any Plume consumer; a strong upstream candidate.
- **B — RT64 misuse of an existing Plume contract:** the fix belongs in RT64, not upstream.
- **C — generic diagnostics / robustness:** potentially useful upstream, not required for correctness.
- **D — fork-specific integration or temporary debugging:** stays local or is removed.

No uncommitted Plume hunk is class B or D. The RT64-side misuse found in the same work (RT+ read-write textures created without `RenderTextureFlag::UNORDERED_ACCESS`) was fixed in RT64 (RT64 commit R2a), not in Plume.

## Summary

| Local commit | Change | Class | Upstream `main` affected | Public contract change | Recommendation |
|---|---|---|---|---|---|
| P1 | Immutable samplers take no descriptor-heap slot | A | Yes | No (bug fix; `setSampler` on an immutable index becomes a no-op) | **Propose, first priority.** Own small PR with a minimal repro. |
| P2 | `copyTextureRegion` sets sample positions only for texture destinations | A | Yes | No | **Propose.** Own small PR; the repro is trivial. |
| P3 | DXR: application layout as global root signature; identifier-only shader records | A (inconsistency), but behaviour-changing | Yes | **Yes** (D3D12 ignores the SBT `descriptorSets` argument) | **Discuss with the maintainers first** (issue). Then its own PR with a repro, removing the now-dead `rtDummyGlobalPipelineLayout`. |
| P4 | HRESULT failure logging; `PLUME_D3D12_DRED` | C | N/A (additive) | No (the env var is new optional behaviour) | **Optional.** Logging can go upstream as a small PR; DRED only in whatever form the maintainers prefer. |

Never bundle these with Zelda64Recomp or RT64 work, or with each other. Each upstream PR must be rebased onto upstream `main`, not onto the fork branch.

## P1 — D3D12 descriptor-heap layout vs. root signature (class A)

**Hunks.**
- `plume_d3d12.h`: `static constexpr uint32_t ImmutableSamplerHeapIndex = UINT32_MAX;` in `D3D12DescriptorSet`.
- `plume_d3d12.cpp`, `D3D12DescriptorSet::D3D12DescriptorSet` → `addDescriptor`: an immutable sampler records `ImmutableSamplerHeapIndex` and takes no view or sampler heap slot.
- `plume_d3d12.cpp`, `D3D12DescriptorSet::setSampler`: returns early for `ImmutableSamplerHeapIndex`.

**Contract violation.** A descriptor set's heap layout must match the descriptor-table layout of the root signature that reads it.
- In `D3D12DescriptorSet::D3D12DescriptorSet` (upstream around l.893–915), every range that is not a *dynamic* sampler gets a view-heap slot. That includes immutable samplers: `isDynamicSampler` is false for them, so they fall into the view branch.
- `D3D12PipelineLayout::D3D12PipelineLayout` (upstream around l.3447, comment at l.3509) turns immutable samplers into static samplers "filtered out of the table entirely". It does not advance `viewTableOffset`.
- So every view declared after N immutable samplers in a set is **written** at heap offset k+N and **read** at table offset k.
- The root signature is valid and nothing reports an error; shaders silently read unwritten or unrelated descriptors.

**Affected path.** Any set with an immutable sampler before a view descriptor. Sets whose immutable samplers are last are unaffected, which is why this can stay latent.
- In RT64, the common raster set has 18 immutable samplers at bindings 7–24. On D3D12, every binding ≥ 25 read zero or wrong data.
- Enhanced per-pixel lighting (t37 lights, t68–t70 normals and transforms) lost about half its energy, and rendered black without RT GI.
- Also affected in RT64: `RaytracingComposeDescriptorSet` (sampler at binding 0), and the Zelda64Recomp UI sampler set (immutable sampler plus a CBV).

**Evidence** (fork, RX 9070 XT):
- A diagnostic view of per-pixel normal inputs read zero on D3D12 and correct on Vulkan. P1 alone restored it.
- Final composed images now match across APIs: mean luminance Vulkan 0.2532 vs D3D12 0.2531 at noon, 0.1977 vs 0.1981 at night, at a 2134×1200 target.
- Full evidence: `_working-directory/diagnostics/2026-09-26-d3d12-perpixel/REPORT.md` and ADR-018 fix 7 in `docs/DECISIONS.md`.

**Public behaviour.** No API change. Descriptor indices returned by `RenderDescriptorSetBuilder` are unchanged, and only the internal heap placement changes. A set now allocates fewer view slots. `setSampler` on an immutable-sampler index previously wrote an unrelated sampler-heap slot and is now a no-op; immutable samplers are baked into the root signature.

**Before proposing.**
- Provide a minimal standalone repro: a set with `{addImmutableSampler(0), addStructuredBuffer(1)}`, where a compute or pixel shader reads the buffer. It returns zero or garbage on D3D12 and correct data on Vulkan.
- A backend-level test that builds a set plus a layout and compares heap indices with table offsets would be the ideal focused test, if upstream has test infrastructure.
- Submit as its own small PR.

## P2 — `copyTextureRegion` with a buffer (placed-footprint) destination (class A)

**Hunk.** `D3D12CommandList::copyTextureRegion`: call `setSamplePositions` and `resetSamplePositions` only when `dstLocation.texture != nullptr`.

**Bug.** Upstream (l.2302–2317) unconditionally calls `setSamplePositions(dstLocation.texture)`. For a texture → buffer copy (`RenderTextureCopyLocation` placed footprint), `texture` is null. `setSamplePositions` asserts `texture != nullptr` in Debug and dereferences it in Release (`interfaceTexture->desc…`).

**Affected path.** Any D3D12 texture readback through `copyTextureRegion` to a buffer. The Vulkan backend already supports this path.

**Public behaviour.** No API change; the path now works as documented.

**Before proposing.** The repro is trivial: copy any texture region to a readback buffer on D3D12. Submit as its own small PR.

## P3 — DXR root-signature and shader-binding model (class A inconsistency, behaviour change)

**Hunks.**
- `D3D12RaytracingPipeline::D3D12RaytracingPipeline`:
  - two fewer subobjects;
  - the local-root-signature subobject and its export association are removed;
  - the global-root-signature subobject now uses the application `pipelineLayout` instead of `device->rtDummyGlobalPipelineLayout`;
  - every export keeps the existing empty local root signature.
- `D3D12Device::setShaderBindingTableInfo`:
  - the record stride is `D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES`, rounded to the table alignment;
  - descriptor-handle collection and copying are removed;
  - records hold only shader identifiers.

**Inconsistency upstream.**
- The state object associates the application layout as a *local* root signature (l.3349) and uses a dummy *global* one (l.3360). It writes descriptor-table handles into SBT records (l.4092).
- At the same time, `setRaytracingPipelineLayout`, `setRaytracingDescriptorSet` and `setRaytracingPushConstants` (l.2120ff) set the application layout as the command list's *compute* root signature. D3D12 requires that to match the state object's global root signature.
- Push constants on the command list therefore cannot reach ray-tracing shaders.
- The Vulkan backend binds ray-tracing resources through the pipeline layout and descriptor sets. The two backends thus expose different binding models behind one API.

**Behaviour change.**
- With P3, D3D12 binds like Vulkan, and the `descriptorSets` argument of `setShaderBindingTableInfo` is **ignored on D3D12**.
- Consumers must bind with `setRaytracing*`, which they already need for Vulkan.
- A consumer that relied only on SBT-embedded tables on D3D12 would break. Upstream RT64's dormant full-RT path passes descriptor sets to both the command list and the SBT, so it would keep working.

**Evidence.** Required for genuine D3D12 RT+ in the fork. It is part of ADR-018 fix 3, and D3D12/Vulkan RT+ signals match within the harness floor.

**Recommendation.**
- Open an issue or discussion with the maintainers first: this is a contract decision, not just a bug fix.
- Then submit its own PR with a standalone repro: a ray-generation shader reading a descriptor-set resource and a push constant, compared across backends.
- The upstream version should also remove `rtDummyGlobalPipelineLayout` (creation around l.3816, reset around l.3891), which is dead after this change. The fork deliberately leaves it in place to keep the local diff minimal.
- Also note in the PR that record size now equals the identifier size (32 bytes, identical to `D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT`), so the `memcpy` size change is cosmetic.

## P4 — D3D12 failure logging and DRED (class C)

**Hunks.**
- `#include <atomic>`; a forward declaration of `reportDeviceRemoved`.
- `D3D12CommandList::end` logs a failed `Close()` HRESULT.
- The two `CreateResource` failure paths and `CreateGraphicsPipelineState` log the HRESULT and call `reportDeviceRemoved`.
- `dredRequested()` and `reportDeviceRemoved()`: env-gated by `PLUME_D3D12_DRED=1`. They print breadcrumbs and page-fault allocations once, on the first `DXGI_ERROR_DEVICE_REMOVED`.
- `D3D12Interface::D3D12Interface` enables DRED auto-breadcrumbs and page faults when requested.

These hunks are interdependent and must stay in one commit.

**Value.** It is not required for correctness. It made the D3D12 bring-up diagnosable without the SDK debug layers or a debugger; a PSO or `Close` failure was previously silent.

**Public behaviour.** Additive only. Logging goes to stderr on failure paths, and DRED is off unless the env var is set.

**Recommendation.** Optional.
- The HRESULT logging is a reasonable small upstream PR.
- DRED enablement is policy, so offer it as an opt-in API or config flag if the maintainers prefer that to an env var, or keep it local.
- It does not need a repro, only a short rationale.

## Suggested upstream order (after the local checkpoint)

1. **P1** (descriptor layout): the highest-impact silent bug.
2. **P2** (readback copy).
3. **P3** as an issue or discussion, then a PR if the maintainers agree on the binding model.
4. **P4** logging, optionally.

Each is prepared on a fresh branch from upstream `main`, with no fork-only commits. The work is re-validated on that base: P1–P3 applied on current upstream `main`, a Plume example or repro run on D3D12 and Vulkan. This file records recommendations only; submitting anything requires explicit user approval.
