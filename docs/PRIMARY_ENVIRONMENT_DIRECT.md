# Primary environment direct responsibility

Implemented 2026-09-22; see ADR-012 and ADR-013. This is a stylized Enhanced responsibility, not a physical lighting model.

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

Historical receiver coverage is not a universal semantic-source restriction. The adapter now explicitly grants bounded unowned response through `environmentColor[0].w` (.35 for MM; generic default zero). This is source permission, not source energy or a confidence estimate. Eligible spatial receivers already carry executable opaque/depth-writing world-camera and monotonic SHADE semantics with unmodified provenance. Raster must additionally match the existing RT call/depth guide; neither source validity nor historical matching bypasses that validation.

On these receivers, **zero** matching Primary slots permits bounded additional responsibility; one match uses only ADR-012 replacement; multiple matches retain authored fallback. Positional fallback is excluded. The additional term uses world shading normals and authored magnitude for supported lit surfaces, geometric normals for authored-color surfaces:

```
increment = RGB * (primaryEnvironmentGain(RGB, authority) - 1) * permission * saturate(diffuse)
increment *= min(1, .15 / max(max(increment), epsilon))
increment *= 1 - saturate(max(currentShade))
appliedIncrement = increment * geometricVisibility
```

All bounds are scalar, preserving source hue. Current SHADE is remaining appearance headroom, never albedo or an exactly decomposed historical source budget. No uncertain historical contribution is removed, and no full second source is added. Composition occurs after existing fill/local/indirect handling, before the combiner. Missing source permission, source semantics or receiver guides denies expansion. Zero Direct authority restores the previous response continuously; missing guides retain the existing owned-path visibility fallback independently.

## Other responsibilities

`PrimaryEnvironment.hlsli` is the single shader source-gain definition. Raster applies it once to the owned primary term, or uses only its bounded gain-minus-one on separately permitted unowned receivers. GI applies it once to the same resolved source at the bounce, then retains existing incident/reflectance/transport caps. Direct and indirect composition remain separate; GI is not another copy of primary raster direct. GI's primary visibility continues even when the developer disables raster directional shadows.

Ambient/fill retains its existing confidence-weighted authority transfer. Stronger direct intentionally increases contrast/brightness where allowed; there is no promise of unchanged average scene brightness. No mean-visibility compensation, exposure feedback, framebuffer correction, new traversal, ray class or visibility pass was introduced. The primary ray, directional visibility and four finite bounce samples remain the existing paths.

## Controls and defaults

Enhanced primary interpretation is enabled by default (`rt_primary_direct: true` in graphics JSON), following `rt_lighting_authority`. The session expansion gate defaults on; on supported hardware it requests the existing RT receiver guides when needed. Owned interpretation alone still needs no hardware RT. It can run on eligible per-pixel raster surfaces without hardware RT; visibility then uses the existing fallback. Native/original compatibility remains separate.

- Conservative Direct: disable interpretation or set Direct authority to 0. This preserves authored primary magnitude, with independently selectable modern visibility. Global authority 0 also reaches the existing Conservative ambient/fill endpoint.
- Enhanced Direct: authority 1, up to 2x source gain. Intermediate authority transfers only the additional primary response continuously.
- F1 Lighting: interpretation enable; global authority; session Direct override (unchecked follows global); existing RT sun-shadow toggle; GI enable/raw controls; session GI local candidate budget [0,2], default 2. F1 changes do not write the graphics JSON. There is no new permanent Graphics-menu slider.
- F6/F1 views 25–29: authored unoccluded primary, interpreted unoccluded primary, applied primary including visibility (these three display at gain 0.5), effective Direct authority, geometric primary visibility. Magenta is unavailable/unsupported. Existing composition views remain available.
- Launch overrides: `RT64_PRIMARY_DIRECT=0|1`, `RT64_PRIMARY_DIRECT_AUTHORITY=-1..1` (negative follows global), `RT64_GI_LOCAL_CANDIDATES=0..2`. Existing `RT64_LIGHTING_AUTHORITY`, `RT64_RT_SHADOWS`, `RT64_RT_GI`, and `RT64_LIGHTING_DEBUG` remain.

### Responsibility and master controls

- F1 **Primary spatial responsibility (session)** / `RT64_PRIMARY_EXPANSION=0|1` independently restores ADR-012 coverage. Primary interpretation off or Direct authority zero also removes expansion completely. Views30/31 show owned green / expanded blue / ambiguous yellow / fallback magenta and the applied increment at display gain4. Expansion never enters GI; its shared source interpretation is unchanged.
- **F9: RT+ MASTER ON/OFF** / `RT64_RT_PLUS_MASTER=0|1` overrides effective settings. OFF restores authored VS/PS lighting, original fog and cutout coverage, and disables Primary, semantic locals/spatial local additions, GI/fill transfer, AO/enclosure and RT directional visibility/debug paths. ON restores configured values and developer overrides. F1 and stdout show the state. Persistent configuration is untouched. Ordinary MSAA sample count, resolution/presentation and texture/mod replacements remain configured baseline features; MSAA resources cannot safely be switched through a per-frame flag.

## Layout/build contract

ADR-013 adds no layout size or offset changes: `lightingAuthority.z` is effective unowned permission; spatial mode bit64 requests the existing receiver/normal guide path even when other spatial effects are off. Source color W is permission only; all energy consumers still use RGB. No new traversal or Primary visibility ray is added.

FramebufferParams is 128 bytes (previously 112); authority/color/direction offsets are 80/96/112, verified by CPU static assertions and DXIL reflection. `lightingAuthority.y` is effective Direct authority; the new final float4 carries normalized semantic direction and validity. The existing 256-byte upload allocation is sufficient. The six-float4 RT environment buffer keeps its size; entry 4 is local bounce weight / local candidate limit / environment validity / Direct authority. Its previously unused `.y` no longer carries global authority. Camera push constants and raster varyings did not change.

Full Zelda64Recompiled build passed, including actual DXIL and SPIR-V generation for primary RT, raster dynamic/library/specialization/flat/MSAA consumers and CPU relinking. D3D12 compilation is not D3D12 runtime qualification.

## Focused evidence and limits

ADR-013 evidence is under `_working-directory/diagnostics/2026-09-22-responsibility/`; each compact Town/Inn sequence wrote all16 native captures. `run.ps1 -ResponsibilitySequence -FixedRepeat -Capture burst -CaptureCount 16 -FramebufferPair 1 -CropWidth 64 -CropHeight 64` exercises previous coverage, authority .5/1/0, role/increment views, visibility off, interpretation off, master off and restored states. Add `-Entrance 0xBC00 -SkipGameSnapshots 650` for Inn; `-ClearPrimary` holds the existing directional visibility control off without changing sources/geometry. The fixture releases controls after completion.

Town: 3954 owned crop pixels were identical with expansion off/on; positive owned gain1.69089 matched within3.1e-5 storage error. Inn: 3393 broader receivers executed, with zero increment at3148 blocked pixels and unchanged owned terms. The same Inn visibility-off diagnostic changed33 final pixels (max RGB delta .021561, max shader increment .074403), while640 owned pixels stayed identical. Its authority0/.5/1 crop means were .122949/.124204/.125298. Expansion disable restored previous raster exactly; Direct0 matched interpretation off; master OFF->ON restored the exact configured raster. Expansion left raw GI, locals, spatial guides and visibility byte-identical. Source values differ between held scenes; gains follow the same definition.

The narrow shaded contact sheets and live scenes were inspected. Normal Inn obstruction prevented a visible added Primary term in the measured crop; the positive unowned composition result is explicitly a **visibility-off diagnostic**, not proof of broad natural outdoor coverage. Expansion defaults enabled with bounded permission in Enhanced. Secondary/Local/Ambient behavior was not reworked or newly certified. No additional ray class/pass or resource layout was introduced. A clean16-sample Inn benchmark had median whole-workload .68406ms, fused RT .17510ms and zero reported resource growth/incomplete samples; this is a sanity check, not a speedup claim.

Master state restoration is runtime-qualified through production queue controls. Physical F9 delivery and its notification remain unqualified: available keyboard injection also failed for existing F1 and Escape. Missing semantic source, ambiguous duplicates, isolated mode64-only operation, Native, fog and MSAA/cutout visual endpoints remain structural checks in this run. The initial larger readback burst had writer-busy drops and is not used to certify missing states. Current artifact paths and final build are in HANDOFF.md.

The following is prior ADR-012 evidence, retained for its distinct gain/weak-source contract:

Evidence lives under `_working-directory/diagnostics/2026-09-22-primary-direct/`. `run.ps1` uses the existing isolated profile/playback/held-workload capture route. `-PrimarySequence -FixedRepeat -Capture burst -CaptureCount 16 -FramebufferPair 1 -CropWidth 64 -CropHeight 64` exercises the exact queue controls used by F1. `analyze.py RUN` reads native formats/strides and emits analysis.json plus a labeled contact sheet. Use desktop execution for the game; sandbox-only launches did not expose a usable window. The initial full-size burst dropped writer-busy attempts; the two compact runs wrote all 16 captures without drops.

- Day 1 noon, Clock Town scene 0x6F/room 0, current real mod profile: 3954 owned crop pixels. The positive primary term measured 2x within UNORM quantization. Raster crop mean RGB progressed 0.30119 -> 0.32271 -> 0.33618 for Direct authority 0 -> 0.5 -> 1. Native visibility/spatial/local/spatialDirect buffers were byte-identical. The ambient diagnostic was identical. 544 blocked pixels had exactly zero applied primary; 2611 unobstructed pixels matched the interpreted term exactly. Disabling raster visibility reproduced the unoccluded primary view byte-for-byte. GI changed only through its existing bounded source response.
- Day 1 23:00: resolved primary RGB (40,50,60)/255 and valid negative-Y direction. Source gain was approximately 1.182; the final raster crop remained byte-identical across authority 0/0.5/1. The weak source did not become daylight. Two semantic local sources were present; reducing the GI candidate budget from 2 to 0 changed 228 raw-GI crop pixels (maximum absolute channel difference 0.00812), confirming actual selection/contribution control.
- Clean 16-sample benchmark A/B at 400x240, RX 9070 XT: median whole workload 0.9492/0.9006 ms and fused RT 0.1485/0.1565 ms for Direct authority 0/1. No incomplete samples, native readbacks or reported resource growth. This is a short sanity check, not evidence of a speedup or a comparison against the old binary; neither traversal topology nor ray/sample counts changed. Details: performance.json and benchmark-authored/benchmark-enhanced run directories.
- F1 interpretation toggle visibly changed the held daytime result and displayed authority request 0. The session GI budget was set to 0 through the actual UI. All primary diagnostic views and intermediate authority ran in the controlled native sequence.

These are focused Vulkan/non-MSAA checks, not broad artistic, platform, mod, HFR or receiver-coverage certification. No separate fixed/non-celestial scene was captured; the generic shader has no elevation branch, so negative Y carries no special policy. Unknown/ambiguous-source fallback is structurally checked, not exhaustively runtime-fixtured. Camera-motion shadow collapse, incomplete submitted/cull-dependent caster population, cadence/stepping and unrelated reconstruction instability remain open. Stronger lighting may make those existing limitations more visible.
