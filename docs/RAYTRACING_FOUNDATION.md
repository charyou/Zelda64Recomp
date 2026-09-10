# Experimental hardware RT foundation

Current implementation and validation: 2026-09-09. This is an opt-in developer primary-hit inset, not production ray-traced lighting. Vulkan runtime proof exists on an AMD Radeon RX 9070 XT.

## Thesis and ownership

The thesis held: reconstruct only the missing RT64 orchestration and stage shaders around existing Plume APIs and presentation-time geometry. The old `RT_ENABLED` feature remains disabled. No removed legacy classes, full DI/GI renderer, historical donor code, renderer abstraction, CPU geometry upload, or raster world-position varying was introduced.

Reused roles: WorkloadQueue's `rtEnabled`, the `DeveloperShortcut::RayTracing` route, RSPWorldCS/output buffers, executable indexed draw ranges, framebuffer replay, and Plume AS/pipeline/SBT/dispatch APIs. Reconstructed roles: `RaytracingDebug`, its isolated descriptor/pipeline/resource lifetime, and `PrimaryHitRT.hlsl`. Generic Vulkan AS defects/contract gaps also needed repair; the prepared architectural reconciliation was correct about API availability but did not establish runtime correctness of every backend operation.

## Implemented files and behavior

Paths below are relative to `lib/rt64` unless explicitly prefixed otherwise.

| File / symbol | Change and reason |
| --- | --- |
| `src/render/rt64_raytracing_debug.h/.cpp`, `RaytracingDebug::reset/record` | New per-framebuffer owner for BLAS/TLAS, scratch, instance/SBT uploads, descriptors, RT pipeline, debug texture and inset copy pipeline. `record` builds, traces and composites; initialization exceptions disable that owner and leave raster output. |
| `src/render/rt64_framebuffer_renderer.h/.cpp`, `Framebuffer::debugRT`, `addFramebuffer`, `recordFramebuffer` | Reset eligibility each replay; collect supported executable indexed ranges while retaining their normal raster draws; record RT after the framebuffer's raster scenes. Only Enhanced participates. |
| `src/hle/rt64_workload_queue.cpp` | Initialize `rtEnabled` from exact `RT64_RT_PRIMARY_HIT=1`; snapshot it with `raytracing && bufferDeviceAddress` capability gates. Request the existing VertexProcessor whenever debug RT is enabled, including unmatched frames. No new world-position producer. |
| `src/shaders/PrimaryHitRT.hlsl` | New ray-generation, miss and closest-hit library. Barycentric colors prove triangle intersections. Dark navy means miss; cyan marks the inset border. |
| `CMakeLists.txt` | Compile the new C++ owner and invoke the surviving `build_ray_shader`; generate real DXIL/SPIR-V and their C/header wrappers. Apple uses SPIR-V generation only and remains unqualified. |
| `src/contrib/plume/plume_render_interface_types.h` | Add optional `RenderTopLevelASInstance::bottomLevelStructure`, allowing Vulkan to use the actual AS device address while retaining the existing DXR backing-buffer contract. |
| `src/contrib/plume/plume_vulkan.h/.cpp` | Query AS scratch alignment and honor it in allocation; preserve one BLAS build range per geometry in backend-private build data; use the same TLAS flags for size query/build; query the actual BLAS AS address when supplied. |

**Plume is a nested submodule. Its edits are inside `lib/rt64/src/contrib/plume`, not represented by the outer RT64 diff alone.** All changes remain uncommitted. No submodules were reset, updated, replaced, or moved to upstream state. N64ModernRuntime and MM patches are unchanged.

## Actual data flow

```text
Workload RSP data + current/interpolated transforms and vertex velocity
  -> existing VertexProcessor / RSPWorldCS
  -> OutputBuffers.worldPosBuffer (float4 stride, XYZ for intersection)
  + DrawBuffers.faceIndicesBuffer
  -> addFramebuffer's eligible executable indexed ranges
  -> one multi-geometry world-space BLAS per participating framebuffer
  -> one identity, double-sided TLAS instance
  -> isolated SceneBVH descriptor + PrimaryHitRT pipeline + three SBT records
  -> traceRays at half target width and half target height
  -> RGBA8 UAV primary-hit texture
  -> existing FullScreenVS/TextureCopyPS with a target-matched copy pipeline
  -> upper-left quarter of the Enhanced color target
  -> ordinary resolve/presentation
```

The inset remains in the renderer's framebuffer path, including normal target resolve. It is diagnostic content, so it can also appear in Enhanced framebuffer feedback. Native/RDRAM output does not receive it.

## Geometry and camera policy

- Perspective `IndexedTriangles` only, with positive face count and nonempty executable scissor; exclude VertexTestZ-rewritten ranges, extended commands, rectangles, raw triangles and orthographic/UI draws.
- Require depth compare and update, `ZMODE_OPA`, pixel Z, no alpha compare, no coverage-times-alpha, no clear-on-coverage, and no framebuffer alpha blending according to `Blender::usesAlphaBlend`.
- `triangles.indexStart` and `triangles.faceCount * 3` select an in-bounds slice of `faceIndicesBuffer`. Vertex format is `R32G32B32_FLOAT`, stride 16; index format is `R32_UINT`. The vertex count is the Workload's actual vertex count. `isOpaque=true` is deliberate.
- The first eligible projection index per framebuffer supplies `inverse(modViewProjTransforms[index])`. Other projection indices are excluded, even if they might be compatible. There are no scene, actor, asset, texture or mod identity gates.
- Whole triangles participate; raster viewport/scissor clipping is not reproduced inside the AS. TLAS disables face culling for this double-sided debug view. This is not yet a production shadow opacity/culling contract.
- Rays unproject NDC depth 0 and 0.99, start at the former and stop at the latter. This bounded diagnostic interval is not full raster near/far coverage; distant geometry can be absent. Custom viewport placement, multiple cameras and singular/custom projective world transforms are not qualified.
- The world buffer is generated at the same replay weights as the renderer. BLAS and TLAS rebuild every enabled framebuffer replay; no refit, compaction or object-space cache. Unmatched frames explicitly run VertexProcessor as well.
- Per-framebuffer resources retain capacity and grow as needed. The existing WorkloadQueue graphics fence is waited before renderer reuse. CPU mesh lists clear every replay; a disabled/empty scene performs no RT dispatch. Descriptors and AS objects survive GPU execution; AS objects are released before replacing their backing allocations.

## Shader, descriptor and synchronization contract

No changes to RasterVS/RasterPS linkage, RDPParams (336 bytes), per-pixel-light resources, normal-magnitude `TEXCOORD1`, dynamic/specialized raster wrappers, or fog ABI.

The isolated RT layout has one 64-byte raygen push-constant matrix at `b0`, one AS `SceneBVH` at `t1/space0`, and one `RWTexture2D<float4>` at `u2/space0`. CPU descriptor ordinals come from the descriptor builder. Payload is 16 bytes; attributes are two floats; recursion depth is one. Shader symbols are `PrimaryRayGen`, `PrimaryMiss`, `PrimaryHit`; the closest-hit group is `PrimaryHitGroup`. SBT contains one raygen, one miss and one hit-group record. Plume computes handle/record/table alignment; the SBT buffer uses `SHADER_BINDING_TABLE` and is uploaded once. Descriptor sets are passed to the SBT builder for its cross-backend contract.

The output is single-sample RGBA8 UNORM with storage usage. The inset copy uses its own layout with the existing `TextureCopyCB` (16 bytes, pixel push constants at b0) and t1 texture descriptor. Its pipeline matches the destination sample count and format, drawing into the actual color target before normal resolve. MSAA runtime was not tested.

AS input transitions follow RSPWorldCS writes. Plume COMPUTE barriers include Vulkan AS-build and ray-tracing stages. Ordering is world/index read + BLAS/scratch write -> BLAS build -> BLAS read/scratch reuse + TLAS write -> TLAS build -> TLAS/SBT read -> output GENERAL -> trace -> output SHADER_READ + target COLOR_WRITE -> raster inset. Instance uploads have AS-input usage; scratch has AS-scratch/storage usage; results use AS-buffer allocation. Uploads and reused resources depend on the existing waited graphics fence. D3D12 generic code and DXIL compile, but its runtime/barrier behavior is unqualified.

## Focused validation actually performed

Initial identities: parent `codex/rt64-raytracing` at `ef88a580f28776ae8a984a151bfa17178a9e2d7c`; RT64 `codex/raytracing` at `cfbe357c82c6b7611f46d5252e89b97333d1d0ed`. RT64 was initially clean; supplied research and `lib/rt64 und md files.zip` were already untracked in the parent. The requested `git submodule status` could not execute because that Git shell could not resolve `basename`, `sed`, and `git-sh-setup`; direct repository status/HEAD checks succeeded. No recovery mutation was performed.

Build command (PowerShell 7, documented VS/LLVM 19.1.3/LLD/Ninja environment):

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File _working-directory/diagnostics/2026-09-06/build-control.ps1
```

The script configures the existing `_working-directory/build-zelda-validation`, then runs `cmake --build ... --target Zelda64Recompiled --parallel 12`. New `PrimaryHitRT.hlsl.spv/.dxil`, both generated `.c/.h` wrappers, RT64, Plume and the executable compiled. The initial C++ attempt used a nonexistent RenderMultisampling inequality operator; it was corrected to compare sample count. No new raster shaders required regeneration because their interfaces were unchanged. Existing patch generation ran as part of the documented target; no patch sources were changed.

Final executable SHA-256: `925F6959DD675608C9A14FEC5739B6A47DCA51D0896C0FC38A21DC6D7C6068BC`.

New SPIR-V SHA-256: `C6E5DBA30A8C06BE1FA87630FA78C24689F92E7335899BBD8AFDF2E85AA51586`.

New DXIL SHA-256: `CA3D21B4998C41AC3C162A9E91D24D652D6E7A0079E89BD75BBA0A28D7C23E18`.

Runtime evidence is in `_working-directory/diagnostics/2026-09-09-rt/`. `run-rt.ps1` reuses the already isolated copied profile in `2026-09-07-coverage/runtime`, restores its seed before launch, and starts `rt-primary-hit.exe` with explicit CWD and Normal window style. It selects Vulkan, Atmospheric fog, per-pixel lighting, `RT64_CUTOUT_AA=original`, developer autostart and the existing finite `load-town.json`. `ZELDA64RECOMP_DEV_NATIVE=0` deliberately causes the existing diagnostic to report rejection and retained Enhanced mode; it does not select Native.

Save `a` (Town) loaded through the nine-step/972-read playback. The seed save hash is `B12F0C6F5546C59DF8E9CD26970A81F8F7CD11803E9F7D4E2E13E6D03D8D1C9B`; playback hash is `0257A44B80D56047E88FA810EA06E3A577801077F67DFA8B8F0837C8138E1C1C`. The copied real mod stack was retained. This is one stack smoke test, not exhaustive compatibility evidence.

Failures and results:

1. Initial `rt-on` run on RX 9070 XT recorded 4 meshes/96 triangles (early title view), TLAS and `traceRays 800x480`, then device loss (`0xFFFFFFFC`). User also reported preexisting driver instability, so the crash alone does not attribute causality. Inspection nevertheless found a definite backend bug: a multi-geometry BLAS build passed just one aggregate build-range structure. The per-geometry range fix, scratch alignment, consistent TLAS flags and actual AS address contract are present in the final candidate.
2. Corrected sandbox launch had no repeated loss but no targetable native window. Relaunched on the interactive desktop using the documented execution boundary.
3. `rt-interactive.stderr.log` confirms Vulkan, Enhanced, RX 9070 XT hardware RT pipeline, 256-byte SBT, BLAS/TLAS and trace/inset recording. The 4/96 count is the first logged title submission, not Town's geometry count.
4. `primary-hit-town.png` visibly proves hits on actual Town architecture, ground and Link. Subsequent observations showed Link's idle animation changing in the RT image. This confirms real GPU intersection output, not merely successful CPU calls. The inset's barycentric colors are deliberate, not a raster corruption symptom.
5. The native capture helper crashed once (exit 3221225477); resetting/rebinding and retrying captured the game successfully. No subsequent game device-loss errors appeared in the corrected run.
6. One injected F2 press did not visibly disable RT. The existing F2/DeveloperShortcut route remains wired, but native keyboard injection is unqualified. Stopped the process and relaunched the same binary with `run-rt.ps1 -Name baseline-off -Off`. `baseline-off-town.png` confirms the inset is absent and Enhanced rendering is restored, with no RT initialization records or severe visual regression in its log/capture. Images are not frame-matched.
7. Final candidate showed no black frame, obvious geometry wedge/corruption, crash or device loss during the focused check. No validation-layer-enabled run was performed; clean stderr is not a claim of full Vulkan validation-layer qualification. D3D12 compiled as part of Plume and DXIL generation; no D3D12 runtime run. Native, HFR edge cases, scene transitions, MSAA and performance were not separately qualified in this task.

All test game processes were stopped and the copied seed save restored. The original user profile was not used for game writes. Build logs include `build-address.log`; temporary evidence, shaders and ROM-derived products remain ignored. Final parent/RT64/Plume diff whitespace checks passed.

## Limits and immediate next step

The delivered endpoint is a working hardware primary-hit debug view. It has no production shadows, GI, reflections, alpha tests, receiver integration, material shading, object-space reuse or performance qualification. The scene is a conservative subset of one projection, traced double-sided within the documented finite diagnostic depth interval. Some visible raster surfaces and effects are intentionally absent.

Next coding step: add a second debug visibility ray toward one existing directional RSP light at a primary hit, reusing this TLAS/pipeline/output. Keep the current primary-hit mode as the correctness reference; do not require a new raster varying or production shadow integration for that step. Before interpreting visibility as production shadows, establish culling, clipping and alpha policy.
