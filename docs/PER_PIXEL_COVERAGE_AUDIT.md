# Per-pixel lighting coverage audit

## Scope and tested states

Diagnosis-only audit of the completed Run-4 build. No eligibility rule, lighting equation, shader, renderer structure, RT behavior, semantic ownership, or production configuration was changed.

- Build: parent `a641aa034dfe16d3056b0c399412092fd73ddac3`, RT64 base `c77e16f936c653ebb1d730772a932908dc2b645d`, executable SHA-256 `E094DF88F91D5D79C9CCAE977852C70E133FACA9ACA5173158E40A54C8AE5CFD`.
- Runtime: isolated copied profile under `_working-directory/diagnostics/2026-09-07-coverage/runtime`; the normal user profile was not used. Its pre-audit save and `debug_mode=false` were restored, and all test processes were stopped.
- Settings: Enhanced per-pixel lighting, Original fog, day 1 at 12:00, original cutout AA; RT local lights, AO, Environment Fill, and RT shadows off. `RT64_LIGHTING_DIAGNOSTICS=1` supplied counts. Fresh launches separated normal shading from `RT64_LIGHTING_COVERAGE=1`; Run-4 `RT64_LIGHTING_DEBUG` local-light views were not used.
- Primary view: South Clock Town central platform, including Link, NPCs, props, floor/stalls, and large buildings.
- Additional views: Stock Pot Inn lobby and Termina Field immediately outside the south gate. A user-supplied East Clock Town view corroborates the primary outdoor result.

Counts are representative repeated values for the captured workload, not immutable scene totals. Camera and actor changes move them. “Relevant” means only viewport triangle draws that enter the smooth-shaded lighting classifier; gray flat/raw/rectangle/outside-classifier draws have no current counter. “Enhanced” below means CPU classifier candidate. With RT semantic local lights off, a tagged positional candidate can still execute legacy in the pixel shader; the existing diagnostic does not separately count that subset.

Evidence and the machine-readable summary are under `_working-directory/diagnostics/2026-09-14-per-pixel-coverage/`. The normal/coverage pairs are `clock-town-normal-shaded.png` / `clock-town-coverage.png`, `stock-pot-inn-normal.png` / `stock-pot-inn-coverage-user.png`, and `termina-field-normal.png` / `termina-field-coverage.png`. Runtime logs are preserved beside them. Exact setup and per-file associations are in `coverage-summary.json`.

## Coverage summary

| Scene | Relevant draws | Enhanced | Legacy | Main rejection classes |
| ----- | -------------: | -------: | -----: | ---------------------- |
| South Clock Town | 633 | 514 | 119 | `set` 119; mixed/transform/positional 0 |
| Stock Pot Inn lobby | 439 | 197 | 242 | `set` 242; mixed/transform/positional 0 |
| Termina Field, south gate | 428 | 300 | 128 | `set` 128; mixed/transform/positional 0 |
| East Clock Town, supplementary | 525 | 410 | 115 | `set` 115; mixed/transform/positional 0 |

The rough “about 300 Enhanced / 100 Legacy” observation is directionally valid outdoors, but the view-dependent totals are wider. More important than the count: Clock Town and Termina Field show Link, ordinary characters/props, floors, buildings, trees, posts, and distant opaque scenery green. In those outdoor views the only large visible blue region is sky/background. Stock Pot Inn is the material exception: most of the coherent room is blue while Link and a few object/character draws are green.

## Rejection classes

### `set` — blue

- **Current condition:** the first referenced vertex has `lightCount == 0`, `lightCount > 8`, or a light range outside `rspLights` ([`rt64_state.cpp`](../lib/rt64/src/hle/rt64_state.cpp#L1194-L1203)). Ambient-only count 1 is supported. `G_LIGHTING` off records count 0, and `gSPModifyVertex(...RGBA...)` also clears the stored light count ([`rt64_rsp.cpp`](../lib/rt64/src/hle/rt64_rsp.cpp#L626-L629), [`rt64_rsp.cpp`](../lib/rt64/src/hle/rt64_rsp.cpp#L796)).
- **Prevalence:** every counted Legacy draw in every tested view: 115–128 outdoors and 242 in the selected Inn coverage workload.
- **Visible geometry:** outdoors, large world geometry is green and the large blue region is sky/background; visual relevance is low/special-path. In Stock Pot Inn, the room shell, floor, counter, and shelving form a high-relevance coherent blue group.
- **Available/missing information:** RT64 has per-vertex normal/color bytes, light index/count, and transform identity. This rejection means it has no valid compact ambient-plus-0..7-directional set for the draw. The current bucket does not distinguish ordinary unlit/authored SHADE, modified vertex color, count greater than eight, or an invalid range.
- **Assessment:** outdoor sky/background is **intentionally / correctly unsupported**. The Stock Pot Inn group **requires architectural decision** unless targeted Astra inspection finds a narrower provenance. If it is the expected zero-light/authored-SHADE path, enabling the current equation locally would invent a light set; a richer authored-SHADE/normal or receiver representation is needed. Existing evidence does not support a locally relaxable eligibility edit.

### `mixed` — magenta

- **Current condition:** referenced vertices disagree in light count, have invalid light/world indices, or their semantically compared light colors/directions/coefficients/source records differ ([`rt64_state.cpp`](../lib/rt64/src/hle/rt64_state.cpp#L1204-L1229)). Equivalent reloaded sets are already accepted.
- **Prevalence/visible geometry:** zero in all representative workloads; no magenta content was visible.
- **Available/missing information:** the per-vertex records exist, but there is no single draw-wide light record when the difference is genuine.
- **Assessment:** **requires architectural decision** (draw decomposition or richer per-vertex representation) if a future targeted scene proves it visually important. It is not a demonstrated current bottleneck.

### `transform` — red

- **Current condition:** a nonfinite 3x3 coefficient, or a multi-matrix draw whose matrices are not affine, approximately uniform-scale, and orthogonal ([`rt64_state.cpp`](../lib/rt64/src/hle/rt64_state.cpp#L1231-L1263)). A single shared matrix already supports nonuniform scale/shear through the exact local equation.
- **Prevalence/visible geometry:** zero in all representative workloads; no red content was visible.
- **Available/missing information:** matrices and per-vertex matrix identity exist, but an incompatible multi-matrix draw lacks one shared rotated-normal basis equivalent to the RSP equation.
- **Assessment:** **requires architectural decision** for genuine multi-matrix incompatibility; nonfinite input is **intentionally / correctly unsupported**. No evidence supports relaxing the tolerance locally.

### `positional` — cyan

- **Current condition:** a non-ambient light with `kc != 0` lacks a verified semantic source with positive range ([`rt64_state.cpp`](../lib/rt64/src/hle/rt64_state.cpp#L1264-L1271)). Run-4 owned positional sets are candidates, not this rejection.
- **Prevalence/visible geometry:** zero rejected draws in all representative workloads; no cyan content was visible.
- **Available/missing information:** light position/direction, color, attenuation coefficients, local vertex position, normals, and transforms exist in legacy processing. Faithful per-pixel support still needs correct interpolated position/scale and ownership/absence semantics.
- **Assessment:** **requires architectural decision** and should remain conservative. It is not a demonstrated current coverage problem.

### Outside classifier — gray, uncounted

Non-viewport projections, no-triangle calls, flat shading, and rectangles never enter the classifier ([`rt64_state.cpp`](../lib/rt64/src/hle/rt64_state.cpp#L1183-L1192)). Raw RDP triangles do not carry the indexed RSP normal/light/world metadata used here. These UI, sprite, flat, raw, and special paths are **intentionally / correctly unsupported** by the current authored per-pixel equation. There is no remaining normal-length rejection: zero, short, and varying authored normal magnitudes are supported.

## Highest-value opportunities

1. **Stock Pot Inn’s coherent blue environment group.** It is the only sampled high-value gap: large opaque interior surfaces remain Legacy while nearby Link/object draws are Enhanced. Astra should identify the exact `set` provenance on one or two representative room draws before choosing any policy.
2. **Disambiguate high-area `set` draws, not the entire counter.** The coarse bucket hides zero-light/authored SHADE versus malformed-range cases. A targeted draw inspection of the Inn group has more value than scene-wide instrumentation or relaxing `lightCount > 0`.
3. **Treat other authored/unlit interiors as the likely follow-on class only after the Inn result.** The outdoor baseline provides no evidence that the current normal, mixed-state, transform, or positional restrictions are over-conservative. Sky/background and gray special paths are low-value expansion targets.

## Astra handoff

- Inspect the Stock Pot Inn blue room group first. Establish whether its representative draws are `G_LIGHTING`-off/authored SHADE, modified vertex RGB, or genuinely invalid light ranges. Then decide whether a new authored-SHADE/receiver representation is warranted.
- Probably leave sky/background, gray flat/raw/rectangle/UI paths, and the currently unseen mixed/transform/unowned-positional cases alone unless implementation-specific evidence changes their priority.
- GI receiver/surface coverage: the important warning is the large Legacy Inn shell. A future GI surface policy cannot assume that per-pixel-lighting eligibility covers all visually important opaque receivers. Outdoor ground/buildings/trees in the sampled views are already green; blue sky is not a receiver gap.
- Important world surfaces are currently Legacy in the Inn, but this audit does **not** show that they already have a sufficient valid normal-plus-light-set record for the current equation. No important rejected surface with demonstrably sufficient current per-pixel inputs was found, and no rejection class is presently justified as “likely locally relaxable.”
