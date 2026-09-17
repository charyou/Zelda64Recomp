# Lighting instrumentation Stage 4

Implementation notes for the accepted `LIGHTING_INSTRUMENTATION_CONTRACT.md`. Current qualification/build/artifacts belong in `../HANDOFF.md`. Stages 5–6 are not implemented.

## Existing resources and exact scope

Detailed snapshot/burst captures copy selected existing resources immediately after the real trace/reconstruction producer, before raster use or framebuffer reuse. The existing raster color target is copied after that framebuffer's raster draw and optional diagnostic inset. It is **not the presented image**: later framebuffer operations, resolve, presentation and postprocessing may still alter it.

Resources: `visibility` RGBA32F; `spatial` RGBA16F; `local` RGBA32F (W is a bit container); `spatialDirect`, `rawIndirect`, `reconstruction` RGBA16F; `output` RGBA8 (primary inset, not beauty); and `raster` RGBA8 or RGBA16_UNORM. No format conversion, filtering or float serialization occurs in native evidence. The accepted contract defines each channel's meaning and validity.

Each manifest buffer descriptor records capture generation, exact render occurrence, Workload ID/ordinal, framebuffer pair, primary-guide projection, actual target dimensions, crop, native format/enum, little-endian storage, byte stride, producer and validity. Raster targets may contain several projections: their primary-guide projection is **context only**, not unique fragment attribution. An RT hit/call/depth guide cannot establish final raster visibility under overdraw, transparency or MSAA.

No producer, missing resource, disabled path, bad crop, unsupported format/backend, allocation/map failure or exhausted budget produces a black image. These paths have unavailable descriptors with a reason, null artifact paths and no binary/PNG. Invalid texels in an available resource retain their real bits and sentinels. Valid zero radiance remains valid zero radiance.

## Crop and pick coordinates

Default crop is a centered region no larger than128x128 render texels. Launch overrides:

- `RT64_LIGHTING_FRAMEBUFFER_PAIR`: optional exact framebuffer pair. Absent selects existing framebuffer paths within the cap; invalid/unmatched selection remains unavailable.
- `RT64_LIGHTING_CROP_X`, `_Y`, `_WIDTH`, `_HEIGHT`: nonnegative integer render texels; width/height must be positive and the whole crop must fit. Absent X/Y center the crop. No silent clipping/scaling.

Native texel(x,y) is at `y * row_pitch_bytes + x * bytes_per_pixel` and corresponds to render texel(crop.x+x,crop.y+y). Pitch is aligned to256bytes; source pixel bits survive exactly. Padding is not source texel evidence and is canonical zero padding, explicitly labeled in the descriptor. Trace pixel centers and scale/offset-to-NDC mapping are recorded alongside actual renderer matrices.

Derived PNG previews include a32-row label banner. PNG(x,y+32) references native crop(x,y). The banner is not render evidence. Descriptors record preview mapping, channel interpretation, visualization gain, diagnostic view name/gain and limitations. Magenta denotes invalid/unavailable/nonfinite visualization. Native binaries retain nonfinite payloads and packed W words unchanged. Radiance preview gain4 saturates and cannot measure energy; local XYZ preview is world position*0.001+0.5, never a float interpretation of packed W.

## Lifetime, limits and completion

Copies record in the existing Workload graphics command list. CPU mapping occurs only after the ordinary `execute(); wait();` completion point; no telemetry submission, wait, flush or diagnostic renderer owner is added. Copy-source transitions restore production resource layouts.

At most16 framebuffer paths are collected per Workload, with32MiB native staging cap, existing at-most16 occurrence bound,256MiB default outstanding budget and existing bounded writer queue. Selection truncation marks evidence truncated/dropped. Storage reserves three times native byte count **before allocation** for staging/CPU/writer overlap. Shared reservations follow immutable CPU evidence and queued writer ownership; cancellation/drop releases them naturally. Rearm refuses until older retained storage retires, even after writer shutdown. Writer-busy/budget drops do not stall rendering.

Off and benchmark paths issue no Stage-4 copies, allocations, native records or PNG writes. They also avoid the snapshot-completion framebuffer traversal. Diagnostic captures declare themselves non-benchmark and have no GPU timing claim.

Vulkan required two generic Plume fixes: image-to-buffer region copies and invalidation of noncoherent READBACK mappings after completed GPU writes. No Plume API expansion or MM policy was added. D3D12 uses its existing footprint copy path but remains runtime-unqualified. Metal readback is explicitly unavailable. MSAA raster targets fail closed until an already resolved, exact-stage target can be associated honestly; single-sample RT resources remain capturable. Unsupported raster formats fail closed.

## Raster views

F1 Lighting and F6 expose views0–24. Existing views0–9 retain their meanings. New views observe production locals or the same production helper accumulation, with no raster varying/linkage ABI expansion. They do not grant lighting, source or receiver authority. Highlight/ubershader tint is suppressed in lighting views.

| View | Actual observed value | Encoding/gain |
|---|---|---|
|10|World shading normal, aliased from production spatialWorldNormal output|XYZ*0.5+0.5|
|11|World geometric normal decoded from production guide|XYZ*0.5+0.5|
|12|Authored normal magnitude(R), interpolated lighting-space direction magnitude(G)|gain1, saturated|
|13|Actual raster match/positional fallback|green match, orange mismatch, cyan positional fallback|
|14|Matched raw GI validity|green valid, red invalid, magenta unmatched|
|15|Actual support confidence used by composition|gain1|
|16|Actual effective indirect authority|gain1|
|17|Current ambient / whole authored current SHADE partition|RGB gain1|
|18|Computed replacement ambient / whole authored enhanced SHADE partition|RGB gain1|
|19|Preserved directional direct accumulated by production shade helper|RGB gain1|
|20|Production owned local direct|RGB gain1|
|21|Actually applied unowned addition after its production budget/modulation|RGB gain1|
|22|Final SHADE before the last composition clamp|RGB gain0.5|
|23|Final SHADE after that clamp|RGB gain0.5|
|24|Directional shade helper before its own clamp|RGB gain0.5|

Unavailable new-view values are magenta. Views17/18 on authored-color content show whole artistic SHADE partitions, not isolated physical ambient/albedo. Views22/24 distinguish final and helper clamp boundaries. All are raster visualizations, not unique fragment records or calibrated framebuffer energy. Production arithmetic/order is preserved; diagnostic accumulation is observational.

## Qualification-only reproduction

Ignored scripts under `_working-directory/diagnostics/2026-09-17-lighting-stage4/` clone mutable profile/save/mod configuration, reuse immutable assets, and start save a with the existing Clock Town playback. `run.ps1` records requested environment/profile/executable identity in launch.json. Qualification profile overrides are isolated; persistent user settings are untouched. Its snapshot skip must be verified against actual game receipts (an early skip30 capture was title mode, not Town).

`inspect.py` checks inventory checksums, exact renderer scope, native bits/pitch/padding/validity and lossless render-pixel picks with PNG coordinate linkage. Overlap comparison is bytewise, not a wall-clock/game-state join. `_working-directory/validate_stage4_native_readback.ps1` runs a known-pattern headless Vulkan fixture for formats, bit containers/nonfinite values, nonzero crop offsets, padded pitch and repeats.

Dormant `RT64_LIGHTING_QUALIFY_FIXED_REPEAT` enters the **existing** debugger pause loop once active detailed marker evidence exists. Simulation continues; renderer inputs are held with interpolation1. It adds no alternate submission/completion path. `RT64_LIGHTING_QUALIFY_VIEW_SEQUENCE` chooses views10–24 then0 by attempted capture ordinal. `RT64_LIGHTING_CAPTURE_COUNT` bounds existing burst mode to1–16. Writer-busy drops remain drops; this qualification sequence must never be claimed as a benchmark or a ray/burst ledger implementation.
