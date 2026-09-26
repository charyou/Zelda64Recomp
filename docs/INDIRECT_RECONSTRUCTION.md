# Bounded diffuse indirect reconstruction

Canonical contract for reconstructing RT+'s bounded diffuse GI signal. Decisions: ADR-011 (replaceable reconstruction) and ADR-017 (temporal history validity). GI generation, source ownership and composition are specified in [SPATIAL_LIGHTING.md](SPATIAL_LIGHTING.md). The RT scene provenance this relies on is ADR-016 and its amendment.

## Ownership

- **Renderer (RT64 `RaytracingDebug`)** produces the raw signal and canonical guides for each framebuffer occurrence. It also decides history continuity and stock-motion alignment for the occurrence.
- **Backend (RT64 `IndirectReconstruction`)** consumes only the canonical inputs and owns its history resources. It produces RGB and confidence.
- **Composition (`RasterPS`)** is unchanged. It still requires the current raw sample to be valid (`raw.w >= 0`) and uses output W as support confidence. History can lower variance. It never creates support or validity.
- **Game adapter:** nothing. There are no MM semantics in reconstruction.

## Canonical inputs (per framebuffer occurrence)

| Input | Format | Meaning |
|---|---|---|
| Signal | RGBA16F `rawIndirect` | RGB is incident indirect irradiance at the primary receiver: bounce transport times the bounded tint of the *hit* surface. Receiver response is applied only in composition, so the signal is location-based (demodulated by construction). W is the mean hit distance; W<0 means invalid. |
| Primary guide | RGBA32F `visibility` | Y: call+1 (replay-local; never a temporal identity). Z: clip W = positive linear view depth. W: primitive+1. |
| Normal | RGBA16F `spatial.zw` | Octahedral world-space geometric normal, facing the camera side. |
| Motion (history consumers only) | RGBA16F `motion` | 2.5D motion to the stored previous occurrence. XY: previous UV − current UV. Z: predicted previous − current linear depth. W: admissibility (0 none, 1 static, 2 interpolated). Only admissible surface motion is written. |
| Frame | CPU | `historyContinuous`, sequence index, and current and previous view-projection and screen transform. |

Motion guide derivation (`PrimaryHitRT::motionGuideAt`):
- The per-surface admissibility comes from the WP3 `Motion` class, uploaded as `surfaceMotion`.
- **Static** surfaces use the hit position itself. **Interpolated** surfaces subtract the barycentric stock `worldVelBuffer` displacement times the alignment scale. The previous camera then projects the result.
- The previous camera is the view-projection and screen transform stored with the history occurrence, not stock `prevViewProj`. Camera reprojection is therefore exact across skipped HFR ticks.

Stock object motion alignment (`RaytracingDebug::prepareTemporal`). Stock motion spans `prevFrameWeight → curFrameWeight`:
- **Within one Workload:** rescaled to the stored occurrence's weight. This is exact under linear interpolation.
- **Adjacent Workload:** the stored weight lies before the interval origin, so the motion is extended at constant velocity. The extension is limited to half a game frame; heavy frame skipping is refused.
- **Held replays** of an unchanged weight get zero motion.
- **Anything else** makes Interpolated motion inadmissible for the occurrence. Static motion stays valid.

History continuity requires all of:
- a stored occurrence from the immediately preceding RT occurrence of this slot;
- the same framebuffer key and size;
- scene framebuffer continuity;
- the same or the adjacent Workload.

Reset reasons are reported in the scene series.

## Project reference backend

1. **Spatial stage** (`IndirectReconstructCS`, unchanged from ADR-011): 7×7 taps at two-pixel spacing. It rejects invalid samples, relative depth differences above 2.5% and normal agreement below 0.9. Output is RGB plus support confidence. With temporal off, this is the complete backend and composition reads it directly. The GI sample rotation then stays fixed per pixel, because a moving seed without accumulation would only add shimmer.
2. **Temporal stage** (`IndirectTemporalCS`), only while the consumer is active:
   - *Lookup:* reproject bilinearly using the motion guide.
   - *Per-tap correspondence:* the stored FP16 linear depth must agree with the predicted previous depth (relative tolerance 3%). The stored normal must agree with the current normal (cosine ≥ 0.9). Surface identity is never consulted.
   - *Orientation ageing:* irradiance depends on position and orientation. Each tap's history length is scaled by `((cos − 0.9)/0.1)²`, so rotating surfaces become responsive. Measured on a rapidly turning Link, this reduced the history excess from +35% to +19%.
   - *Clamp:* history is clamped to the raw signal's 5×5 mean ± 2σ. This bounds responsiveness; it is not a validity test. The earlier window over the spatial result collapsed onto the current noisy estimate, which both discarded accumulation and darkened converged history by 3.5%.
   - *Bound:* 6 game frames converted to occurrences using the slot's last interpolation count, with a 4-bit cap of 15 (6 at 20 Hz; 15 ≈ 2 game frames at 144 Hz). Game lighting and actor state change per Workload; interpolated occurrences do not add new game state.
   - *Output:* reconstructed RGB, with W = the current spatial support confidence (−1 invalid). A packed guide is also written: FP16 depth, 6:6 normal, 4-bit length.
   - *Restart:* a failed or partial lookup (bilinear weight below 0.05) restarts from the spatial result, with length 1.

The GI hemisphere rotation advances each occurrence by a golden-ratio sequence only while the temporal stage is active.

## Controls and diagnostics

- Session toggle (default on within GI): F1 Lighting → "GI temporal history (session)". Launch override: `RT64_RT_GI_TEMPORAL=0/1`. It requires GI. Turning it off returns exactly to the history-free spatial path, and its resources are no longer written.
- `RT64_RT_GI_RECONSTRUCTION=rayregen` selects the optional Ray Regeneration temporal backend (below). Scene-series rows report `temporal.backend`.
- `RT64_RT_GI_TEMPORAL_TUNING=maxLength,clampGamma,depthTolerance,normalAgreement` is a developer qualification override with a fixed occurrence length. It is not a product setting.
- Native snapshots (`RT64_LIGHTING_CAPTURE=snapshot|burst`) add `motion`, `spatialReconstruction` and `temporalGuide`. `reconstruction` is the composition input.
- Scene series rows (`RT64_RT_SCENE_SERIES`) gain a `temporal` object: continuity, reset reason, object-motion admissibility and scale, history reuse, weights and effective length.
- Qualification scripts are in `_working-directory/diagnostics/2026-09-25-temporal/`:
  - `burst.ps1`: deterministic Clock Town bursts;
  - `temporal_analyze.py`: validity, reuse and noise, plus an independent CPU re-derivation of static reprojection from the recorded matrices, and the reprojected frame-to-frame instability;
  - `classdiff.py` and `energy.py`: energy by pixel class;
  - `bench-set.ps1`: timing.

  Run folders were moved into `runs/`. The shared `run.ps1` writes them under `2026-09-25-wp3/` first.

## Qualification (Vulkan, RX 9070 XT, Clock Town noon, deterministic playback)

Default settings, Original refresh, 400×240 unless noted.

| Case | Result |
|---|---|
| GPU motion vs CPU re-derivation (static pixels, fast turn ~32 px/frame) | median 0.013 px, p99 0.03 px |
| History reuse, fast turn | 79–93% of GI pixels (actors 78–98%); disocclusions restart |
| Reprojected instability of the composition input, fast turn | 0.0046 (old fixed-seed spatial path) → 0.0016 |
| Noise proxy (mean absolute deviation from 3×3 mean), fast turn | 0.0045 → 0.0027 |
| Still camera, live NPCs | reuse 99.7%; noise 0.0056 → 0.0023; converged energy 0.99 of spatial |
| HFR 144 Hz series (1,171 occurrences) | 1,166 continuous with aligned object motion; in-Workload scale exactly 1.0; boundary extension scale 1.0–5.0; 4 boundaries refused (extension > ½ frame) |

Captures slow rendering enough that stock HFR frame skipping drops interpolated occurrences. That makes HFR image bursts unrepresentative. The alignment guard correctly refused the resulting whole-frame extrapolation, which is why HFR alignment is qualified from the uncaptured series.

## Cost

Resources exist only while the temporal consumer is active: +32 B/pixel.
- RGBA16F motion guide;
- two RGBA16F histories;
- two R32 packed guides.

That is +49 MB at the product target of 1600×960 and +265 MB at 3840×2160. Existing RT+ per-pixel targets are 68 B/pixel. When the consumer is inactive, the motion guide shrinks to a bound 4×4 placeholder.

Timings are GPU medians of 16 benchmark samples in Clock Town at noon with a still camera (`bench-set.ps1`, final executable `4B25C202…6E129`), temporal on / off:

| Setting | Reconstruction ms | Trace ms | Whole Workload ms |
|---|---|---|---|
| Original 400×240, 20 Hz | 0.050 / 0.034 | 0.161 / 0.163 | 0.784 / 0.763 |
| Original 400×240, 144 Hz | 0.041 / 0.042 | 0.153 / 0.149 | 0.472 / 0.477 |
| Product 1600×960, 144 Hz, MSAA4X | 0.245 / 0.208 | 1.223 / 1.230 | 2.146 / 2.171 |

The temporal stage adds about 0.04 ms at the product target. The motion-guide writes and velocity reads produce no measurable trace change. Whole-Workload differences are within run noise. CPU RT resource preparation rises from about 35 to 47 µs, covering two small uploads and continuity bookkeeping. Classification is unchanged.

A fused spatial+temporal dispatch would save the 8 B/pixel spatial result and one pass. It is not done, because the separate spatial stage is the exact fallback and a qualification snapshot.

## Known limits

- **Radiometric change is only bounded, never detected.** A moving actor's indirect occlusion or bounce lags on nearby receivers. On ground within about 50 units of a rapidly turning Link at 20 Hz this measured 8–20%, and it does not depend on the history bound, because those pixels already hold 2–4 samples. Light switching, local-source flicker and value changes behave the same way: they are smoothed within the clamp and the bound.
- **HFR game-frame boundaries:** object motion over the stored occurrence's remaining part of the previous interval is a constant-velocity guess (at most ½ frame). The geometric test guards large errors.
- **Correspondence tolerance:** a coincident different surface (same depth and normal within tolerance) is accepted. This is correct for location-based irradiance but not for receiver-dependent signals.
- **Scope:** D3D12 is qualified (ADR-018), on intermediate signals and, since the Plume descriptor-layout fix, on the final composed image. HFR image metrics remain unqualified. So do receivers under local-heavy source churn, and mods with dense deforming geometry, where `Unknown` motion means history is always restarted.
- **Resolution:** the temporal stage runs at the full target resolution. There is no half-resolution or upscaling path.

## Optional vendor backend: AMD FSR Ray Regeneration 1.2 (ADR-018)

The backend is D3D12-only, needs RDNA4 or later and SM 6.6, and keeps its history inside the vendor context.
- **Selection:** `RT64_RT_GI_RECONSTRUCTION=rayregen` while the temporal consumer is active.
- **Build:** configure `RT64_FFX_SDK_DIR` to the SDK's `Kits/FidelityFX` directory. The MIT API headers are not vendored.
- **Runtime:** place `amd_fidelityfx_loader_dx12.dll` and `amd_fidelityfx_denoiser_dx12.dll` next to the executable. `RT64_FFX_LOADER_PATH` overrides the loader location.
- **Fallback:** when the backend is unavailable (Vulkan, missing DLL, no provider, SM < 6.6), the project temporal backend runs and the reason is logged once.

Pipeline:
1. The unchanged project spatial stage runs.
2. An adapter pass runs (`IndirectRayRegenInputsCS`).
3. The vendor dispatch runs on the same command list. Afterwards Plume is told that the descriptor heaps changed.
4. An output pass (`IndirectRayRegenOutputCS`) writes the canonical contract. RGB is the vendor result. W is the spatial support confidence (−1 invalid), so composition validity and authority are identical across backends.

| Vendor input | Adapter mapping |
|---|---|
| Indirect diffuse RGBA16F, A = hit distance | `rawIndirect` bound directly (demodulated; W<0 inactive) |
| Linear depth R32F | clip W. The vendor documents +Z-forward view space; the adapter mirrors the N64 −Z view and folds the mirror into the projection. |
| Motion, scale (1,1,1) | the canonical guide XYZ where admissible. Where W = 0: best-effort camera-only reprojection of the primary hit, located on the pixel ray by its clip W. The vendor owns disocclusion. |
| Normals (RGBA16F) | vendor octahedral encoding of the geometric guide normal; roughness 1; material 0 for active pixels, 1 where raw W < 0. The material channel carries validity only: without it the vendor mixed inactive zero RGB into receivers. |
| Diffuse albedo | labelled neutral-white proxy (sqrt 1). Never SHADE, `surfaceResponse` or authored colour. |
| Specular albedo | zero-initialized resource (no specular signal; the vendor validator requires it) |
| View / projection | RSP world-to-view, and `inverse(view) × viewProjection × screen`. It must be a perspective; otherwise the occurrence uses the spatial result and the next dispatch resets. |
| Camera delta, frame index, reset | inverse-view translation delta; per dispatch; reset when the ADR-017 continuity chain breaks or a dispatch was skipped |

Tuning:
- Stability bias defaults to **0.25**. The vendor default of 1.0 converged to 0.86 of the spatial energy on RT+'s sparse four-ray signal.
- Measured energy against the spatial estimate, by stability bias: 0 → 0.963, 0.25 → 0.965, 0.5 → 0.956, 1.0 → 0.86. These are 400×240 values with the old receiver coverage; product values are below.
- Pre-exposure ×16 and `radiance_clip_std_k` = 1000 changed nothing.
- Overrides:
  - `RT64_RT_RAYREGEN_CONFIG=key=value,…` sets the vendor keys (1 cross-bilateral normal strength, 2 stability bias, 3 max radiance, 4 radiance clip k, 5 kernel relaxation, 6 disocclusion threshold).
  - `RT64_RT_RAYREGEN_VALIDATION=1` turns on vendor validation. The final build runs clean.

### Comparison (D3D12, RX 9070 XT, Clock Town noon, product target 2134×1200)

`Auto` resolution snaps to multiples of 240 lines, so ~1080-line windows give a 2134×1200 target. Captures are full 1920×1080 centred crops. Scripts: `_working-directory/diagnostics/2026-09-25-d3d12-rr/` (`matrix.ps1`, `score.py`, `energybins.py`, `montage.py`, `bench-set.ps1`, `benchsum.py`). The binary for images carries the old receiver gate. The composition input is scored: `rawIndirect` for raw, `reconstruction` otherwise.

| Metric | Raw | Spatial | Project temporal | Ray Regeneration |
|---|---|---|---|---|
| Noise, still | 0.0100 | 0.0029 | 0.00088 | 0.00045 |
| Noise, fast turn | 0.0103 | 0.0024 | 0.00122 | 0.00036 |
| Reprojected instability, turn | 0.0112 | 0.0029 | 0.00090 | 0.00077 |
| Energy vs spatial, still / turn | — | 1 | 0.997 / 1.029 | 0.980 / 0.991 |
| Soft-edge step, absolute (edge − interior) | 0.0059 | 0.0034 | 0.0023 | 0.0024 |
| Edge / interior ratio | 1.42 | 1.92 | 4.11 | 14.8 |

With the widened receiver gate the whole frame participates. Project and Ray Regeneration then measured noise 0.00064 / 0.00031 and energy 1.001 / 0.989.

Images:
- Ray Regeneration is the cleanest on flat ground and walls.
- During the fast turn it softens real low-frequency contact detail: the crisp darkening under crates becomes a soft gradient.
- The project backend keeps the detail but shows low-frequency mottling.
- The final composites differ subtly, because GI is a bounded share of ambient.

Cost: GPU medians of 16 samples, product configuration (MSAA4X, 144 Hz, still camera), `bench-set.ps1`.

| Setting | Reconstruction ms | Trace ms | Whole Workload ms |
|---|---|---|---|
| D3D12, GI off | — | 1.617 | 2.534 |
| D3D12 spatial | 0.319 | 2.011 | 3.321 |
| D3D12 project temporal | 0.404 | 2.040 | 3.396 |
| Vulkan project temporal | 0.443 | 2.014 | 3.449 |
| D3D12 Ray Regeneration | 3.392 | 2.064 | 6.489 |

Memory:
- **Ray Regeneration:** the vendor reports 321.2 MiB at 2134×1200 (236.4 MiB aliasable, so about 85 MiB persistent), and 23.8 MiB at 400×240. The adapter adds 40 B/pixel (about 100 MB).
- **Project temporal:** +32 B/pixel (82 MB).

Conclusions:
- The backend fits the boundary without changing guides, composition or source handling.
- It is the stronger denoiser.
- At about 8× the reconstruction cost, for a component that is a bounded share of ambient, it is not the product default.

### Low-poly faceting

Faceting originates in the signal and normal representation.
- GI is gathered over the per-triangle geometric normal hemisphere (`cross(b − a, c − a)`).
- The normal guide is that same normal: 11–12° across soft edges on low-poly actors.
- The raw signal already carries the full edge step. No backend adds it.
- Denoising removes the interior noise that masked it, so facets are *more* visible with better backends. Ray Regeneration is the most visible, because it respects normal discontinuities.

Changing the normal semantics (for example, a smooth shading-normal GI hemisphere) is a separate GI-generation decision. It was not taken here.

### Other Ray Regeneration signals (not integrated)

| Signal | Existing RT+ source | Missing authoritative input |
|---|---|---|
| AO | `spatial.x` contact visibility | stochastic per-occurrence sampling. Today it is a fixed 12-direction deterministic term, so a temporal denoiser gains nothing. Owned by spatial visibility. |
| Dominant-light visibility | primary sun visibility (`visibility.x`) | cone-sampled shadow rays with hit distance and a light angular radius. Today it is a single hard ray. |
| Direct diffuse | none | a demodulated local direct radiance signal; composition computes local direct analytically |
| Indirect / direct specular, specular occlusion | none | specular transport and specular albedo |
