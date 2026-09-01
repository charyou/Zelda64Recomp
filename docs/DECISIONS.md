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

**Decision:** Zelda64Recomp extracts resolved `LightContext` fog color, fogNear, and zFar and passes them through the native renderer adapter. RT64 snapshots the generic atmosphere structure onto the Workload before display-list processing.

**Why:** A Workload is the stable per-frame render record and avoids render-thread/HFR races. MM logic remains in the game integration layer; RT64 receives only generic resolved inputs.

**Alternatives considered:** Extended GBI commands for frame-global values; render-thread mutable globals; recreating time/weather logic in RT64.

**Consequences:** Extended GBI remains reserved for draw-/transform-local information. Future frame-global semantic inputs should extend Workload metadata unless they truly vary within a display list.

## ADR-003 — Atmospheric fog replaces only the classified environment baseline

**Status:** Accepted for the experimental first implementation

**Context:** MM frequently overrides fog for actors and effects, then restores `Play_SetFog`. Applying one atmosphere model to every fogged draw would erase those local choices or double-fog them.

**Decision:** Compare each draw's final RSP fog coefficients and fog RGB with the signature derived from the resolved environment state. Matching draws may use Atmospheric fog. Nonmatching local/effect draws use Faithful Per-Pixel fog; mixed per-vertex-state draws use Original.

**Why:** It preserves per-draw authoring and existing N64 blender semantics without requiring MM-specific scene profiles inside RT64.

**Alternatives considered:** Frame-global replacement of every fog draw; fullscreen depth fog; adding atmosphere after legacy fog. These lose local semantics, mishandle transparency, or visibly double fog.

**Consequences:** Signature derivation must remain behaviorally aligned with MM's `Play_SetFog`/`Gfx_SetFogWithSync` conversion. Runtime captures should validate classification before the mode is presented as non-experimental.
