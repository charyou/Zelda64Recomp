# Enhanced-renderer regression: resolution and evidence

> Updated 2026-09-06. The radial geometry regression is isolated to the experimental camera-publication path at a reproduced modded Southern Swamp checkpoint. The four-command omission candidate renders that checkpoint cleanly. Final source retires the experiment; its final build hash and qualification status belong in `HANDOFF.md`.

## Outcome and remaining qualification

The earlier default-off float selectors were not a valid old-path control. Publishing the extra Extended GBI camera commands changed enhanced transform reconstruction even when RT64 selected fixed matrices. A fresh current-source build still produced the radial wedges. Removing all four camera-publication commands produced a clean enhanced view with the same copied save/mod profile and controller-read sequence.

This is a useful causal boundary, not complete renderer qualification. Native/RDRAM also rendered the reproduced checkpoint correctly. Final-build smoke checks now include enhanced Original and Faithful at the owl, Atmospheric pause inventory, and the same save without external mods. The complete scene/effect matrix and Faithful/Atmospheric art-direction qualification remain outstanding. Do not represent the shader dependency fix alone as the geometry fix: its fresh shaders did not remove the wedges.

The final implementation retires the `View_ApplyPerspective` float-publication experiment and its RT64 float selectors. MM's original fixed perspective/view path remains the reference. The semantic camera arrays retain their corrected identity sentinels for atmospheric classification and reconstruction. A future precision implementation must preserve affine model/world transforms and undergo independent qualification.

## Preserved historical binary oracles

### Final retirement build smoke checks

Final executable SHA-256: `D6F7628E560797A20FCCF25FAA8CBF7F8B8244FF360215B8393527B0BEB46E51`.
The final incremental build regenerated patches and registration together, loaded the copied mod profile, and passed these visual checks:

- `captures/final-original-swamp.png`: enhanced Original, clean geometry at the owl, including visible rain.
- `captures/final-faithful-swamp.png`: Faithful, clean geometry at the same saved location.
- `captures/final-atmospheric-pause.png`: Atmospheric, clean pause inventory without the prior banding/wedges.
- `captures/final-no-external-mods.png`: external archives temporarily removed from the isolated profile; copied save loads with original textures and clean geometry. Built-in patches remain. Archives/configuration/save were restored after testing.

These captures are visual smoke checks, not pixel-identical A/B or fog calibration: elapsed game time, weather, idle animation and camera distance vary. The tested profile has 47 archives / 46 enabled entries, not proven coverage of every mod in the user's broader installation. `final-manifest.json` records executable, seed and archive hashes. `Launch-Final.cmd` starts the restored copied profile without developer playback; original user profile/oracle directories are untouched.

### Historical references

Preserve both original build directories. Their executables are binary references, not proven source checkpoints.

| Oracle | Path | SHA-256 |
| --- | --- | --- |
| Confirmed good, 2026-09-02 16:19:51 +02:00, 32,333,312 bytes | `G:\_Development\Github\Zelda64Recomp - Kopie\_working-directory\build-zelda-clang\Zelda64Recompiled.exe` | `F9F3CD786F6A23A948A15D8BCDEA53C6AB4883A62BF083D51ABDFF8A8DFD3B1D` |
| Confirmed bad, 2026-09-02 22:15:27 +02:00, 32,341,504 bytes | `G:\_Development\Github\Zelda64Recomp\_working-directory\build-zelda-clang\Zelda64Recompiled.exe` | `7529CC047FFAE1C11D52E3924F587101E6E10A20000632B85D2FB9A4F3EFDB0D` |

Both hashes were reconfirmed before this investigation. The good executable works with the user's practical large mod installation; attributing the defect to the number of mods was unsupported. `f5cac21` predates that executable, but uncommitted work may have been included. Neither it nor the subsequent `e8f82f6` is proven to reconstruct the oracle.

The original symptom was large black/green wedges radiating approximately from the viewport centre, affecting ordinary geometry and skybox. F3/Native was correct. Changing output refresh mode to Original did not resolve it.

## Reproduced comparison

All evidence below is local and ignored under `_working-directory/diagnostics/2026-09-06/`. These executable copies are preserved separately from the active validation build directory.

| Candidate | SHA-256 | Result |
| --- | --- | --- |
| `current-control.exe`: fresh prechange source | `75F9F676782E3FE34FACB22BF60AACEF26FE47E6863D75EAC213647A6FD59B05` | Complete clean Clang build, fresh shaders and regenerated patches; radial wedges remain in attract/gameplay observations. |
| `tools-control.exe`: developer playback plus semantic-array sentinels | `B2B67524ACFB6BD0C077D36470B826185400BBA2C6851C9DB60D5C9B6AD7D821` | Loads copied modded save; Southern Swamp owl view still has wedges. |
| `camera-omission.exe`: four publication commands omitted, initial incremental build | `AB3CA84A6D10BBEAA10A233C0995EC5F80F565B49ECDFC1FF0671D4E7F5516EA` | Invalid graphics comparison: stale patch registration causes blank-name mod-loading error. |
| `camera-omission-synced.exe`: same omission, corrected generated dependencies | `322DF33F7EFB8B4909543E6BC72BC231CBE9E8E7193746874E68D6F43A7298CC` | Loads mods and renders the reproduced Southern Swamp view cleanly; separate Native/RDRAM run is also clean. |

The discriminating captures are:

- `captures/control-swamp-wedges.png`: enhanced control with publication present.
- `captures/omission-swamp-clean.png`: enhanced corrected omission candidate.
- `captures/native-swamp-reference.png`: Native/RDRAM reference using that candidate.

`runtime/native.stderr.log` explicitly records `API=Vulkan; configured fog=Atmospheric; presentation=Native/RDRAM`. The other run logs are `runtime/tools-control.*.log` and `runtime/omission-synced.*.log`. Native presentation bypasses enhanced fog regardless of the configured fog value.

The runs use the copied profile, restored seed save and `load-save.json`: nine steps totaling 972 controller-0 reads, followed by neutral input. The sequence reaches the same owl-save view. It does not prove deterministic simulation or pixel-identical animation timing. Scene, camera and visible content were checked in captures. See [Runtime validation](RUNTIME_VALIDATION.md) for profile preparation, launch variables, playback limitations and capture workflow.

## Source hazards established independently

### Camera publication changes the enhanced basis

The four experimental commands publish float projection and view matrices to OPA and XLU display lists. Selecting fixed sources afterward still establishes an extended camera basis; it does not restore the preceding identity basis.

Source analysis found two associated hazards:

- `RSP::setVertexCommon` can move the extended view/projection basis into transforms treated downstream as model/world transforms. Perspective matrices violate the affine assumptions used by lighting and decomposed model interpolation. `decomposeMatrix` normalizes by the homogeneous element and recomposition does not preserve that factor.
- The full correction `inverse(EV * EP) * (V * P)` generally differs from separately composing `inverse(EV) * V` and `inverse(EP) * P`. Matrix multiplication is noncommutative. The enhanced projection processor's separate-factor reconstruction cannot be assumed equivalent to the Native full correction when this basis is active.

The four-command runtime omission establishes the failing feature boundary. It does not identify which of these downstream mechanisms generated each bad triangle. Retiring the incompatible experiment is safer than repairing one algebraic symptom while leaving the model-space contract broken. Detailed analysis and a numeric counterexample are in `matrix-analysis.md` and `matrix-correction-example.py`.

### Semantic camera arrays lacked index-zero sentinels

`viewTransforms`, `projTransforms` and `viewProjTransforms` begin with identity entries. The new `rspViewTransforms` and `rspViewProjTransforms` originally did not, shifting semantic lookups by one index. Both now receive identity sentinels, with insertion-size assertions. This fixes camera metadata association; the tools-control result demonstrates that this correction alone did not remove the geometry regression.

## Build reproducibility fixes and proof

### Transitive shader dependencies

The shader custom commands previously depended only on each top-level HLSL file. Editing a shared C++/HLSL layout could rebuild CPU code and edited pixel shaders while leaving vertex shaders stale.

The fresh control found 103 of 109 RT64 shader binaries identical to the bad oracle. The differing six were all five raster vertex variants and DebugPS SPIR-V. All six were identical between the historical good and bad oracles. DXC reflection shows the preserved RasterVSDynamic DXIL expecting an RDPParams element size of **176 bytes**, versus **320 bytes** in the current source/fresh shader. DebugPS DXIL has no retained RDPParams resource; its SPIR-V dependency chain includes the shared header.

`lib/rt64/CMakeLists.txt` now uses DXC-generated depfiles for DXIL, SPIR-V, the Metal SPIR-V stage, and embedded preprocessed HLSL. CMake 3.21 is required for DEPFILE support across Ninja/Make and Visual Studio/Xcode generators.

Targeted Windows Ninja/Clang proof:

- A second unchanged targeted build reports no work.
- Touching only `rt64_rdp_params.h` rebuilds all five selected VS variants and DebugPS SPIR-V; unrelated Bicubic CS and RenderParams preprocessing remain untouched.
- Touching `rt64_render_params.h` rebuilds embedded RenderParams text.
- All 109 freshly compiled shader hashes remain unchanged after dependency tracking is added.

Evidence: `stale-shader-evidence.json`, `raster-vs-bad-dump.txt`, `raster-vs-control-dump.txt`, and `shader-deps-{initial,noop,header-touch,preprocess-touch}.log`. Other platform generators were not executed.

### Patch registration must match regenerated patch code

The first omission build regenerated `patches.c`, the binary and registration headers at 14:35:22, but retained `register_patches.cpp.obj` from 14:33:42. Its mod loader then reported out-of-function branches while recompiling base patch functions. This reproduces the previously unexplained blank-name mod-loading error with a concrete stale-artifact boundary.

Ninja's compiler-discovered dependencies used relative `../../RecompiledPatches/...` names, while generator outputs used absolute names. These were distinct graph nodes: a header could appear current during initial dependency scanning and then be regenerated without scheduling its registration consumer in that invocation.

Root `CMakeLists.txt` now declares `patches_bin.h` as a byproduct and explicitly attaches the three absolute generated-header paths to `register_patches.cpp` using `OBJECT_DEPENDS`. Patch generation also tracks the recompiler executable, its configuration and referenced symbol files. `file_to_c` is an explicit binary-embedding dependency.

A **single** corrected build regenerated patches, compiled `register_patches.cpp` afterward, and linked the candidate that successfully loaded the modded save. Evidence: `camera-omission-build.log`, `camera-omission-patch-deps-build.log`, `patch-deps-fixed-manifest.json`, and `runtime/omission.stderr.log`. No second-build workaround is required by the repaired dependency graph.

## Reproducibility and next qualification

The initial source state is preserved in `prechange-manifest.json`, `prechange-source-hashes.json`, project/RT64/runtime binary diff files and `untracked-prechange/`. Project HEAD was `e8f82f63a58594a71e609ead92f56c5d1b8d5f83`, RT64 HEAD `c8ce62b79d31884e1774e4b0fc247166c9359f33`, and runtime HEAD `222fbfd2d41eeca63f54bb97206b79cef0083201`; all had uncommitted work. The initial control manifest verifies no recorded source changed during compilation.

`build-control.ps1` records the reproducible local toolchain: VS x64 development environment, Clang/LLD 19.1.3, Ninja, RelWithDebInfo, cached SDL sources, the project-local patch-build wrapper and explicit Windows SDK `mt.exe`. LLVM's bundled `llvm-mt` fails with `no libxml2` in this environment. Never point this script at either preserved oracle build directory. Generated sources, ROMs, copied saves/mods and captures remain ignored local artifacts.

The next qualification should expand beyond the recorded smoke checks: interiors/skyboxes, camera cuts, distortion, water/framebuffer feedback, particles/transparency and high refresh rates. Atmospheric rain/storm, saturated opening fog and cutscene classification remain art-direction checks. Use source/binary/profile/input manifests and the small existing playback/capture loop for further renderer work.
