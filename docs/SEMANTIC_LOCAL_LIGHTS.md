# Semantic local lights

Implementation: 2026-09-13. See HANDOFF.md for current validation/build state.

## Generic source and ownership

`SemanticLight` is a 48-byte record: world position/range, authored RGB/direct strength, and shadow authority/range/source radius/reserved-zero. It describes a stylized source, not physical radiance, a material or a visible emitter. `RSPLight` grows from 48 to 96 bytes. RT64 contains no MM source/actor/scene/texture branches.

`gEXSetLightSource` (extended opcode 0x34; two commands; zero-based slot; twelve float words) annotates an already loaded RSP light. Ordinary light loads/color edits invalidate it. Vertex loads snapshot the annotation alongside original light values; light-set equivalence includes semantic values. No nearest-color/direction search is used to associate unrelated lights.

MM's `patches/semantic_lights.c` instruments `Lights_BindAll` and `Lights_Draw`, reusing original bind functions. Up to 512 frame-local receipts verify the actual positional or reference-directional realization and the unchanged Lights group. Drawing consumes the receipt and emits immutable frame-allocated metadata. Inactive/unknown sources, modified bindings, consumed/reused receipts and overflow preserve original lighting. Mods bypassing the adapter need no new API to remain functional. Repeated drawing of the same group without rebinding conservatively loses enhancement after its first draw.

The first path enhances **bound sources**. It does not illuminate every surface from every LightContext source. Runtime-lit world geometry without an original local binding keeps its current direct lighting; supported characters, static props and positional-lit world geometry use the same modern path. Adding unbound receivers later needs an explicit original-contribution/absence contract. Source proximity alone cannot establish that contract.

## Rendering

The existing conservative opaque BLAS/TLAS, primary SurfaceHit, world positions and call/depth validation are reused. No second scene or GI system. RT descriptors add u8 local output, t9 RSP lights and t10 RDP records. RGBA32F local output stores receiver world position in RGB; W is a bit container with seven 2-bit visibility values, ownership bits21–27 and bit30 to avoid denormal flushing. Raster t5/space3 uses an unfiltered Load. Disabled/failure paths use valid dummy descriptors and zero production enables. Explicit resource barriers and per-framebuffer lifetimes remain.

TraceParams stays128 bytes, FramebufferParams80 and RDPParams336. SpatialFlags/modes.w bits0/1 remain AO/fill, bit2 enables locals, bits8+ select diagnostics. Local-only runs do not trace spatial rays. Each source uses a finite source segment; source radius0 uses one ray, nonzero radius requests three fixed source-sized samples. Sequential queries retain recursion depth1.

Raster removes only accepted owned RSP terms and adds:

`RGB * strength * (1 - (distance/range)^2)^2 * max(N dot L, 0) * authoredNormalMagnitude * shadowResponse`

Distance/range is clamped to0–1, giving smooth finite support. Shadow authority is continuously weighted by `1 - (distance/shadowRange)^2`. Visibility only changes that source's contribution. MM maps authored radius to range/shadow range, strength1, shadow authority=clamp(radius/160), source radius0. These are authored-energy interpretations, not photometry. Other directionals and independently shaped ambient remain, with final SHADE clamped to1 before the existing combiner/alpha/fog. Semantic locals are excluded from sun-direction matching. No glow or indirect energy is synthesized.

`pixelLighting.y` stores light count in bits0–7 and tagged-positional presence in bit8. Every positional term must have ownership to become a candidate. A new TEXCOORD2 float3 preserves original interpolated SHADE for exact fallback if RT data is unavailable on such draws. Dynamic/specialized SPIR-V and DXIL library wrappers share the signature. Untagged positional sets, mixed states, modified colors, unsupported transforms and Native preserve original rendering. Existing bounded authored-fill response remains independently available where compatible.

## Profiles and extension boundary

The source record is independent of its current RSP-slot attachment. Authored-light enhancement is the implemented source strategy. Bounded spatial treatment of authored appearance remains the Run-3 responsibility. A future game profile can opt into synthetic sources and geometric-normal receivers using these same source/visibility/response functions, with explicit receiver permission and a collection independent of RSP replacement slots. Missing semantics do not silently enable synthetic illumination. Profile semantics belong in adapters/source records; user enable/quality controls belong in renderer configuration. No speculative hierarchy, material framework, preset UI or synthetic-light implementation was added.

## Controls and diagnostics

Default-off JSON `rt_local_lights` uses existing GraphicsConfig apply/reset. F1 → Lighting has the toggle. `RT64_RT_LOCAL_LIGHTS=0|1` overrides at launch. F6 in developer mode or the F1 combo cycles shaded, ownership, influence, direct contribution, shadow response and fallback reasons. `RT64_LIGHTING_DEBUG=0..5` selects the launch view.

Ownership: amber local replacement; green runtime lighting; blue eligible authored-fill treatment; gray original/unsupported. Influence/direct/shadow modes show actual computed source values. Reasons: magenta mixed state, red transform, cyan positional fallback, orange RT mismatch, green no owned source. Existing coverage visualization remains available.

## Reproduction and known limits

Use `_working-directory/diagnostics/2026-09-13-local-lights/run.ps1` with the existing copied profile. Successful finite input playback now releases normal controller/keyboard input; malformed playback stays neutral with an error. `ZELDA64RECOMP_DEV_TIME=day,hour,minute` invokes the existing time action once in normal gameplay, excluding title attract. `RT64_DEV_INSPECTOR_LAYOUT=1` places the existing inspector inside the test window.

Sources update at simulation rate, without HFR source interpolation/history. Only submitted eligible opaque geometry casts; game-culled geometry and cutouts can be absent. Bound-source/receiver limitations above are intentional fallback, not universal local coverage. Existing Run-3 readability/motion artifacts and separate sun/camera transients are not claimed fixed. No indirect prototype is implemented.
