# Primary environment direct responsibility

Implemented 2026-09-22; see ADR-012. This is a stylized Enhanced responsibility, not a physical lighting model.

## Ownership and interpretation

The adapter publishes resolved primary direction and RGB through the existing environment snapshot. It owns game interpretation. RT64 knows only a generic primary directional source. No new game packet, celestial classification, clock, scene/actor ID, elevation policy or visual-sun dependency was added.

Authored direction, hue, relative environment strength and zero energy remain authoritative. Absolute RGB magnitude is an **energy reference**, not the Enhanced ceiling. The modern source gain is:

```
s = max(resolvedPrimaryRGB)
gain = 1 + DirectAuthority * smoothstep(0.08, 0.65, s)
```

Gain stays in [1,2]. Weak sources are never normalized into daylight; below 0.08 there is no amplification. The smooth response preserves continuous environment changes and RGB ratios. The existing per-pixel diffuse equation, authored normal magnitude and final stylized SHADE ceiling remain. This run replaces the historical source-magnitude ceiling, not every historical realization choice.

For raster ownership, a non-positional RSP light must match normalized direction (dot > 0.9998) and RGB (each channel within 0.008). Exactly one matching contribution is required. Its evaluated term is replaced by `authoredTerm * gain * geometricVisibility`. Ambient, secondary and local terms are not multiplied by that visibility. Equal-valued multiple matches are ambiguous and retain authored behavior. No second primary contribution is added.

Source validity is separate from RT-guide validity: a finite, nonzero direction and finite, nonnegative, nonzero RGB in a valid environment authorize interpretation. Missing or mismatched RT guides use visibility 1, not a darkening guess. Missing source semantics, unsupported per-pixel state, positional fallback and ambiguous ownership preserve their appropriate authored path.

Historical receiver coverage is not a universal semantic-source restriction. This implementation retains the existing raster eligibility because the current packet does not establish safe direct ownership or permission on baked/unlit artwork. It does not infer that a known source illuminates every raster receiver. A future generic permission/decomposition extension can broaden coverage independently of this energy model. GI already evaluates the semantic primary at eligible bounce surfaces without requiring an RSP slot match.

## Other responsibilities

`PrimaryEnvironment.hlsli` is the single shader source-gain definition. Raster applies it once to the owned primary term. GI applies it once to the same resolved source at the bounce, then retains existing incident/reflectance/transport caps. Direct and indirect composition remain separate; GI is not another copy of primary raster direct. GI's primary visibility continues even when the developer disables raster directional shadows.

Ambient/fill retains its existing confidence-weighted authority transfer. Stronger direct intentionally increases contrast/brightness where allowed; there is no promise of unchanged average scene brightness. No mean-visibility compensation, exposure feedback, framebuffer correction, new traversal, ray class or visibility pass was introduced. The primary ray, directional visibility and four finite bounce samples remain the existing paths.

## Controls and defaults

Enhanced primary interpretation is enabled by default (`rt_primary_direct: true` in graphics JSON), following `rt_lighting_authority`. This does not enable any previously disabled RT feature. It can run on eligible per-pixel raster surfaces without hardware RT; visibility then uses the existing fallback. Native/original compatibility remains separate.

- Conservative Direct: disable interpretation or set Direct authority to 0. This preserves authored primary magnitude, with independently selectable modern visibility. Global authority 0 also reaches the existing Conservative ambient/fill endpoint.
- Enhanced Direct: authority 1, up to 2x source gain. Intermediate authority transfers only the additional primary response continuously.
- F1 Lighting: interpretation enable; global authority; session Direct override (unchecked follows global); existing RT sun-shadow toggle; GI enable/raw controls; session GI local candidate budget [0,2], default 2. F1 changes do not write the graphics JSON. There is no new permanent Graphics-menu slider.
- F6/F1 views 25–29: authored unoccluded primary, interpreted unoccluded primary, applied primary including visibility (these three display at gain 0.5), effective Direct authority, geometric primary visibility. Magenta is unavailable/unsupported. Existing composition views remain available.
- Launch overrides: `RT64_PRIMARY_DIRECT=0|1`, `RT64_PRIMARY_DIRECT_AUTHORITY=-1..1` (negative follows global), `RT64_GI_LOCAL_CANDIDATES=0..2`. Existing `RT64_LIGHTING_AUTHORITY`, `RT64_RT_SHADOWS`, `RT64_RT_GI`, and `RT64_LIGHTING_DEBUG` remain.

## Layout/build contract

FramebufferParams is 128 bytes (previously 112); authority/color/direction offsets are 80/96/112, verified by CPU static assertions and DXIL reflection. `lightingAuthority.y` is effective Direct authority; the new final float4 carries normalized semantic direction and validity. The existing 256-byte upload allocation is sufficient. The six-float4 RT environment buffer keeps its size; entry 4 is local bounce weight / local candidate limit / environment validity / Direct authority. Its previously unused `.y` no longer carries global authority. Camera push constants and raster varyings did not change.

Full Zelda64Recompiled build passed, including actual DXIL and SPIR-V generation for primary RT, raster dynamic/library/specialization/flat/MSAA consumers and CPU relinking. D3D12 compilation is not D3D12 runtime qualification.

## Focused evidence and limits

Evidence lives under `_working-directory/diagnostics/2026-09-22-primary-direct/`. `run.ps1` uses the existing isolated profile/playback/held-workload capture route. `-PrimarySequence -FixedRepeat -Capture burst -CaptureCount 16 -FramebufferPair 1 -CropWidth 64 -CropHeight 64` exercises the exact queue controls used by F1. `analyze.py RUN` reads native formats/strides and emits analysis.json plus a labeled contact sheet. Use desktop execution for the game; sandbox-only launches did not expose a usable window. The initial full-size burst dropped writer-busy attempts; the two compact runs wrote all 16 captures without drops.

- Day 1 noon, Clock Town scene 0x6F/room 0, current real mod profile: 3954 owned crop pixels. The positive primary term measured 2x within UNORM quantization. Raster crop mean RGB progressed 0.30119 -> 0.32271 -> 0.33618 for Direct authority 0 -> 0.5 -> 1. Native visibility/spatial/local/spatialDirect buffers were byte-identical. The ambient diagnostic was identical. 544 blocked pixels had exactly zero applied primary; 2611 unobstructed pixels matched the interpreted term exactly. Disabling raster visibility reproduced the unoccluded primary view byte-for-byte. GI changed only through its existing bounded source response.
- Day 1 23:00: resolved primary RGB (40,50,60)/255 and valid negative-Y direction. Source gain was approximately 1.182; the final raster crop remained byte-identical across authority 0/0.5/1. The weak source did not become daylight. Two semantic local sources were present; reducing the GI candidate budget from 2 to 0 changed 228 raw-GI crop pixels (maximum absolute channel difference 0.00812), confirming actual selection/contribution control.
- Clean 16-sample benchmark A/B at 400x240, RX 9070 XT: median whole workload 0.9492/0.9006 ms and fused RT 0.1485/0.1565 ms for Direct authority 0/1. No incomplete samples, native readbacks or reported resource growth. This is a short sanity check, not evidence of a speedup or a comparison against the old binary; neither traversal topology nor ray/sample counts changed. Details: performance.json and benchmark-authored/benchmark-enhanced run directories.
- F1 interpretation toggle visibly changed the held daytime result and displayed authority request 0. The session GI budget was set to 0 through the actual UI. All primary diagnostic views and intermediate authority ran in the controlled native sequence.

These are focused Vulkan/non-MSAA checks, not broad artistic, platform, mod, HFR or receiver-coverage certification. No separate fixed/non-celestial scene was captured; the generic shader has no elevation branch, so negative Y carries no special policy. Unknown/ambiguous-source fallback is structurally checked, not exhaustively runtime-fixtured. Camera-motion shadow collapse, incomplete submitted/cull-dependent caster population, cadence/stepping and unrelated reconstruction instability remain open. Stronger lighting may make those existing limitations more visible.
