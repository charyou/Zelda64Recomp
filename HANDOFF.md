# Handoff — primary environment direct, 2026-09-22

## Current implementation and decision

Primary direct responsibility is implemented and built; ADR-012 and docs/PRIMARY_ENVIRONMENT_DIRECT.md replace the provisional architecture checkpoint. Resolved primary RGB is an energy reference: gain = 1 + DirectAuthority * smoothstep(0.08,0.65,max(RGB)), bounded 1–2x. Raster replaces a uniquely matched non-positional primary contribution; GI uses the shared source interpretation once. Source validity and RT visibility are independent. Ambient/fill keeps existing ownership. No game semantics/packet expansion, solar policy, extra ray class/pass or exposure compensation.

Default rt_primary_direct=true within Enhanced follows global authority. Interpretation off / Direct authority 0 gives authored primary magnitude. F1 provides interpretation toggle, session override (-1 follows global), primary diagnostic views 25–29, existing directional visibility/GI controls and session GI local candidate budget 0–2. Graphics JSON persists only the interpretation enable alongside existing global authority. Launch overrides and equations are in the contract doc.

Changed: RT64 WorkloadQueue/DrawParams, framebuffer renderer/shared params, PrimaryEnvironment helper, PerPixelLighting/RasterPS/PrimaryHitRT, F1/F6/capture labels and existing held-workload fixture. Parent config/render-context and N64ModernRuntime GraphicsConfig add the setting. FramebufferParams 112->128 bytes; CPU assertions and DXIL reflection agree on offsets 80/96/112. RT environment remains six float4s; entry 4.y is candidate budget, .w Direct authority. No varying or camera-push ABI change.

## Validation checkpoint

Full Zelda64Recompiled build passed with actual DXIL/SPIR-V shader generation (dynamic/library/specialization/flat/MSAA and primary RT) and CPU link. Executable SHA256 5EEBA34A6B22D9F2322FF4C242EBD593A101ABCC0B968209C0F88BC2DDAB18E4. Route/log: _working-directory/diagnostics/2026-09-22-primary-direct/build.ps1 and build.log. Raster shader reflection: raster-abi.txt.

All evidence below is under that diagnostics directory. run.ps1 clones mutable runtime state and uses existing playback/capture hooks; analyze.py reads native buffers. Desktop launches need unsandboxed execution; sandbox launches had no usable window. Initial full-size burst had honest writer-busy drops. Compact noon/night sequences each captured all16 states without drops.

- noon-compact-a997e236-4dbb-4607-b5d9-3a15ac9e3010: held Clock Town0x6F room 0, day 1 noon. Authority0/0.5/1 final crop means 0.30119/0.32271/0.33618; primary 2x within storage quantization. 544 blocked pixels have zero applied primary; 2611 clear pixels exactly match interpreted direct. Ambient and visibility/spatial/local buffers unchanged. Visibility-off applied primary exactly matches unoccluded interpretation. GI uses the same bounded source. F1 toggle and budget exercised through actual UI.
- night-d34615fb-e404-486e-898d-c14b328e1ed7: day 1 23:00, RGB 40/50/60, valid negative-Y direction; gain ~1.182 and final crop byte-identical across authority endpoints/intermediate. GI candidate 2->0 changes 228 raw-GI pixels with two sources present, max channel delta 0.00812.
- Clean benchmark-authored / benchmark-enhanced runs each completed 16 samples without readbacks or resource growth. Whole-workload median 0.9492/0.9006 ms; fused RT 0.1485/0.1565 ms. See performance.json. Short sanity check only; no speedup claim.

## Limits and next action

Implementation, full build, focused runtime/visual checks, performance sanity check and final diff checks are complete. No required work remains for this bounded responsibility; wider qualification remains below. Unknown/ambiguous fallback is structurally checked; no separate fixed/non-celestial scene was captured (shader has no elevation policy). This is not broad D3D12/Metal/MSAA/HFR/mod/artistic certification. Camera-motion shadow collapse, submitted/cull-dependent caster coverage, source cadence/stepping and unrelated reconstruction instability remain open.

Changes are uncommitted in parent, RT64 and N64ModernRuntime; preserve supplied untracked research/review files and lib/rt64.7z. Prior Stage4 handoff is saved as handoff-before.md in diagnostics; that instrumentation contract is unchanged. Generated CHANGELOG.md is not manually edited; user-facing note: Enhanced now offers stronger bounded primary environment direct with a Conservative endpoint.
