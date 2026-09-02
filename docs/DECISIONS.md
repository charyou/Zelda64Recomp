# Architecture Decisions

> Durable architectural decisions for this project.
> Add an entry only when a choice materially constrains or guides future work.
> Do not use this file as a session log; current implementation state belongs in `HANDOFF.md`.

## ADR-001 — Native rendering is the compatibility reference

**Status:** Accepted

**Context:** Modern per-pixel effects can alter data observed by framebuffer feedback, reinterpretation, and other game-visible N64 behavior.

**Decision:** Keep Original vertex fog and other original behavior in the Native/RDRAM renderer. Apply modern fog only during enhanced Workload replay.

**Why:** This preserves game-visible rendering behavior while allowing high-resolution beauty rendering to improve spatial fidelity.

**Alternatives considered:** Applying the feature to both renderers; gating by output resolution. Both are less explicit and risk changing compatibility behavior.

**Consequences:** New beauty-render features need an explicit enhanced-renderer gate and must not rely on Native output changing with them.

## ADR-002 — MM environment semantics are Workload metadata

**Status:** Accepted

**Context:** Atmospheric rendering needs resolved MM environment state that is lost when `Play_SetFog` quantizes it to RSP coefficients.

**Decision:** Zelda64Recomp extracts resolved `LightContext` fog color, fogNear, and zFar plus the resolved environment sun vector and active world-camera state, then passes them through the native renderer adapter. RT64 snapshots the generic atmosphere structure onto the Workload before display-list processing.

**Why:** A Workload is the stable per-frame render record and avoids render-thread/HFR races. MM logic remains in the game integration layer; RT64 receives only generic resolved inputs.

**Alternatives considered:** Extended GBI commands for frame-global values; render-thread mutable globals; recreating time/weather logic in RT64.

**Consequences:** Extended GBI remains reserved for draw-/transform-local information. Future frame-global semantic inputs should extend Workload metadata unless they truly vary within a display list.

## ADR-003 — Atmospheric fog replaces only the classified environment baseline

**Status:** Accepted for the experimental first implementation

**Context:** MM frequently overrides fog for actors and effects, then restores `Play_SetFog`. Applying one atmosphere model to every fogged draw would erase those local choices or double-fog them.

**Decision:** Compare each draw's final RSP fog coefficients and fog RGB with the signature derived from the resolved environment state. Atmospheric eligibility additionally requires a perspective projection whose inferred camera position and orientation match MM's active world camera. Matching world draws may use Atmospheric fog, with a semantically gated share of the authored optical depth redistributed into a height-dependent medium rather than stacked as a second fog curve. Nonmatching local/effect and secondary-camera/UI draws use Faithful Per-Pixel fog; mixed per-vertex-state draws use Original.

**Why:** It preserves per-draw authoring and existing N64 blender semantics without requiring MM-specific scene profiles inside RT64.

**Alternatives considered:** Frame-global replacement of every fog draw; fullscreen depth fog; adding atmosphere after legacy fog. These lose local semantics, mishandle transparency, or visibly double fog.

**Consequences:** Signature derivation must remain behaviorally aligned with MM's `Play_SetFog`/`Gfx_SetFogWithSync` conversion. Camera matching must remain semantic rather than scene-ID-specific so pause models and other secondary projections cannot interpret their coordinates as world-space atmosphere. Runtime captures should validate classification before the mode is presented as non-experimental.

## ADR-004 — Preserve RT64's raster-stage linkage ABI for modern fog

**Status:** Accepted

**Context:** Passing clip-space fog depth through a new `TEXCOORD1` varying produced severe color and geometry corruption in Vulkan and a D3D12 driver crash on an AMD RDNA4 GPU. An in-game A/B build with the added varying removed rendered correctly in all three fog modes while retaining the extended RDP parameter layout.

**Decision:** Do not add a raster-stage varying for atmospheric fog depth. Reconstruct clip W in the pixel shader from reciprocal `SV_Position.w`; keep RT64's established vertex/pixel linkage signature unchanged.

**Why:** The reconstruction supplies the required per-fragment distance input without expanding the linked library or specialized shader ABI. It also preserves the compatibility-tested attribute locations across Vulkan and D3D12.

**Alternatives considered:** Keeping the extra varying; disabling modern fog on RDNA4; adding a separate shader permutation. The first is demonstrably broken on current hardware, the second loses the feature, and the third increases shader-cache and linkage complexity unnecessarily.

**Consequences:** Future raster enhancements should prefer values already available from system semantics or existing interpolants. Any new cross-stage varying requires in-game validation on both Vulkan and D3D12, including AMD hardware, before acceptance.
