# Lighting instrumentation review — 2026-09-15

## Decision

**STAGE 4 IS UNBLOCKED FOR THE CURRENT VULKAN DEVELOPMENT PATH, BUT HAS NOT STARTED.** F1–F6 are corrected and have focused validation; the final schema-2 Vulkan snapshot and clean benchmark satisfy the minimum pre-Stage-4 qualification for this path. The accepted architecture and removability boundary are unchanged. The original findings below remain as review history; the implementation follow-ups supersede their status.

### F3–F6 and pre-Stage-4 follow-up — 2026-09-15

- **F3 — VERIFIED:** evidence producers now require the active generation and compatible mode. Off/finalization clears game tokens, RSP lineage is detailed-capture-only, and CPU clocks are benchmark-only. The fixture passed cold-off and post-cancel retained-Workload cases; the final Vulkan capture returned to inactive operation without retaining capture state.
- **F4 — VERIFIED for the current clean Vulkan path:** benchmark capture rejects lighting/RT inset and inspector-isolation diagnostics, retains a bounded sample window, and serializes only after completion. RT initialization failure, timing overlap, paused/interpolated/HFR/pacing/MSAA context, unused queries and untimed remainder are explicit. The final 16-sample run completed all samples and all 96 measured intervals with no resource growth, initialization failure or query overflow.
- **F5 — VERIFIED:** source origins are bounded during the existing traversal, with independent total/recorded/dropped summaries. A synthetic 10,000-origin population reached the byte cap and reconciled its counts without a second geometry traversal; the final runtime capture reported 18/18 origins and zero drops.
- **F6 — VERIFIED:** schema 2 records type-specific attempt parameters, bind-to-draw receipts, exact annotation command/payload words and rejection status, production tint, separate RT execution status, actual framebuffer/trace/matrix state, the exact published environment packet, and honest build/device/profile/mod/reproduction identities. Unobserved shader hashes and cache warmth remain explicitly unavailable rather than fabricated.
- **Focused and integration checks:** `_working-directory/validate_lighting_scope.ps1` passes F1–F6, `_working-directory/compile_lighting_scope.ps1` compiles all affected RT64 translation units, patch generation and the host serializer compile, and the full patch/SPIR-V/DXIL/Vulkan+D3D12 build/link passes. Current executable SHA256: `5B1B353CFBAC3DBE44363F89D2F0CD1080501C9A8D445D4AD94A1EDBCFAF4DB4`.
- **Runtime evidence:** final snapshot `_working-directory/diagnostics/2026-09-15-lighting-instrumentation/b02bf2cb-dda0-422b-bad4-d26c94aab3d5/capture-0`; final clean benchmark `_working-directory/diagnostics/2026-09-15-lighting-instrumentation/731057b2-a21b-49a5-aa62-494d41b87da5/capture-0`; Vulkan two-entry render/writer qualification `_working-directory/diagnostics/2026-09-15-lighting-instrumentation/b0bfe431-2afd-40f6-9260-51536dc4685a/capture-0`.
- **Multi-Workload scope:** the current render loop creates one Workload per GameFrame. A dormant, removable qualification hook exercised the real Vulkan render loop and two-entry occurrence writer by executing the current real Workload twice; the deterministic fixture separately proves two distinct Workload IDs serialize as one complete occurrence. This does not claim a naturally produced distinct multi-Workload Vulkan frame. That case remains open, but is not a gate for the current single-Workload Stage-4 path.
- **Remaining non-gating qualification:** natural distinct multi-Workload runtime; D3D12 and Metal runtime; modified-color lineage, cap64/empty dummy, and copied/replaced-mod cases; shader binary hashes; measured cache warmth/present-queue/vsync context; rare transitions, platform edge cases and a broader overhead repeat. These are explicit limits, not release or current Stage-4 gates. No Stage-4 output was implemented or captured.

### F1/F2 implementation follow-up — 2026-09-15

- **F1 — VERIFIED by focused fixture:** wire tokens are process-monotonic across rearms, with fail-closed refusal at 32-bit exhaustion. Capture generations bind parser/Workload evidence to a session. Retained evidence from another generation yields unavailable game evidence; stale submissions and completions are rejected. Delayed old wire tokens and host callbacks cannot match new publication keys. Session lookup/serialization is protected against concurrent rearm, and diagnostic RSP lineage is cleared at task-generation changes. Manifest association now requires available publication evidence rather than merely an observed marker.
- **F2 — VERIFIED by focused fixture and integration compilation:** interpreter-owned task/list/span context survives Workload splits. FullSync finalizes evidence before notifying the queue; task completion touches only the current unpublished slice. Calls, returns and taken branches maintain explicit list scope, with task IDs and inherited scope entries distinguished from executed marker PCs. Rendering copies immutable parse evidence into a fresh per-render sidecar passed explicitly to framebuffer collection. Repeated renders cannot append to the retained Workload. All Workloads of an occurrence are assembled into one writer batch, and only `finishOccurrence` spends the capture budget/disarms. Incomplete Workload sets fail closed.
- **Output layout:** single-Workload captures retain their existing files under `capture-N/`. Multi-Workload captures use `capture-N/workload-W/`, each manifest carrying `workload_count`, the same capture/render identity and its renderer Workload ordinal. A complete occurrence is assembled before enqueue. Consumers must check the complete Workload set; this is not a Stage-4 output implementation.
- **Validation at the F1/F2 checkpoint:** `lib/rt64/tests/lighting_scope.cpp` links the actual instrumentation implementation without a GPU/ROM/window. It passed retained-generation rejection, delayed old marker/host callback rejection, unavailable serialized game association, immutable split/list scope, fresh repeated-render rows and a two-Workload/one-occurrence accounting assertion with both serialized Workloads present. All ten changed RT64 translation units compiled using the existing project build configuration. A full executable link and runtime qualification were deferred at that checkpoint and are superseded by the F3–F6 follow-up above.
- **Reproduction/evidence:** `_working-directory/validate_lighting_scope.ps1`; `_working-directory/compile_lighting_scope.ps1`; logs and generated fixture artifacts under `_working-directory/diagnostics/2026-09-15-lighting-scope-fixes/`. The retained test source is in RT64; scripts/artifacts are disposable local diagnostics.
- **Scope at that checkpoint:** only RT64 F1/F2 implementation and the requested memory documents changed. No parent game patch, Plume, shader or lighting-policy change and no new architectural concern. F3–F6 and build/runtime qualification were deferred then and are completed to the extent recorded in the follow-up above.

Original review scope: actual working-tree changes against HEAD, including untracked implementation files; targeted source inspection and existing capture artifacts. That review did not perform broad renderer research, a rebuild or a runtime run.

## Actual inventory

All changes below were unstaged/uncommitted at review start. Recursive repository inspection found no additional dirty nested repository.

| Repository / HEAD | Actual changes | Risk classification |
|---|---|---|
| Parent `e4cdbd557b09e6c6c55521dbc6541df7c84a06ab` | `patches/graphics.h`, `input_latency.c`, `play_patches.c`, `semantic_lights.c`, `syms.ld`; new `patches/lighting_capture.c`; `src/game/recomp_api.cpp`, `src/main/main.cpp`; HANDOFF/internal changelog; dirty RT64 gitlink | Capture ABI/lifecycle/registration. Semantic receipt and draw-stream edits touch production execution and warranted inspection. |
| RT64 `b9bb4ca61e359678fe53467a739c8fd12497eee9` | `CMakeLists.txt`, `include/rt64_extended_gbi.h`; `src/gbi/rt64_gbi_extended.cpp`; `src/hle/rt64_{application,interpreter,rsp,state,workload,workload_queue}.cpp`, corresponding changed `rsp/workload/workload_queue` headers; new `rt64_lighting_instrumentation.cpp/.h`; `src/render/rt64_framebuffer_renderer.cpp/.h`, `rt64_raytracing_debug.cpp/.h`; dirty Plume gitlink | Mostly observation, but production source dedup/receiver branch restructuring, RSP storage and render hot-path hooks require semantic/off-cost review. No shader change. |
| Plume `f425d3e69a4bb28a3357b2312faf7e9cae4566fd` | `plume_render_interface.h`, `plume_{vulkan,d3d12,metal}.cpp/.h` | Generic timestamp API; also changes legacy Vulkan timestamp conversion, so not exclusively debug-only. |
| N64ModernRuntime `b0beeb99a89e1d5fca1c1c8b5bb34628d6e82530` | **Clean** | No task transport/runtime fork introduced. |
| Other nested repositories | **Clean**, including N64Recomp/dependency trees, mm-decomp, Zelda64RecompSyms and scratch reference repositories inspected by inventory | No hidden dirty implementation dependency found. |

Parent untracked `docs/LIGHTING_INSTRUMENTATION_CONTRACT.md`, `docs/input-research/` and `lib/rt64.7z` are pre-existing supplied/reference material per HANDOFF, not instrumentation implementation. Preserve them; no unrelated code change was identified. Tracked diff sizes: parent +532/−7, RT64 +439/−34, Plume +111/−15; these exclude new files.

## Findings and exact corrections

### F1 — BLOCKER, now fixed: original session identity finding

`rt64_lighting_instrumentation.cpp:206–267,346–359,482–565,642` resets `tokenCounter` and replaces the publication store on every arm, but Workload evidence and RSP lineage have no session/generation identity. Old queued/paused Workloads can retain token 1; a later session publishes a different token 1. `submitOccurrence` then resolves the old marker against the new store. This can silently attach the wrong game state; matching annotation bytes does not protect the game join. Evidence mode can also remain detailed across a rearm into benchmark mode.

**Correction:** carry and validate a capture generation through immutable Workload evidence and publication lookup, and prevent token reuse from making delayed command streams appear current (process-monotonic tokens with explicit exhaustion handling are one option). Reject old-session Workloads/lineage. Preserve owned publication references while needed, or report explicit unavailable evidence. Derive manifest association validity from the resolved joins: currently any marker yields `observed`, even token zero/expired publication or partially missing task spans. Exercise rearm with a retained Workload and delayed old commands.

### F2 — BLOCKER, now fixed: original render/task/list scope finding

`rt64_workload_queue.cpp:397–975` calls `submitOccurrence` inside the Workload loop; `submitOccurrence:495,684` spends the occurrence budget and disarms for each Workload. A one-occurrence capture therefore stops after Workload ordinal 0, omitting the rest of the same render occurrence. This is a code defect independent of the missing runtime fixture.

Framebuffer source/receiver/configuration rows append into mutable `Workload::lightingEvidence` on every render (`rt64_framebuffer_renderer.cpp:1401,1476,1994`); only `Workload::reset` clears them. Rows have no render occurrence identity. Repeated/HFR rendering mixes old matrices/settings-dependent decisions into the latest capture and eventually truncates them. **Existing evidence confirms accumulation:** benchmark session `75156ee2-ffee-4cc2-a14f-ee5c10c5a760`, captures 0 and 1 both use Workload 738 but have 3 and 5 effective-configuration rows respectively.

Task/list bookkeeping also needs correction: `Application::processDisplayLists` retains its entry Workload reference through interpretation, whereas `State::fullSync` advances/notifies the queue before `finishTask` runs. Finalization thus occurs after publication and does not follow later Workloads within the task. `beginList` only increments on task entry/pushed return address; returning to a caller does not restore its list identity, and non-push branches have no new list identity. The resulting number is not an exact executed-list identity.

**Correction:** freeze parse evidence before queue publication; retain task/span context across Workload splits; give list entry/return/branch execution explicit scope. Build fresh render-occurrence evidence referencing immutable parse evidence, and finish/count a capture only after all Workloads in the selected occurrence. Validate two Workloads, repeated same Workload with changed settings, and nested list return/branch cases before attaching Stage-4 outputs.

### F3 — FIXED; original finding: Off is not the contracted inactive path

`RSP::loadVertices` (`rt64_rsp.cpp:618`) always pushes lineage storage, including when off. CPU clocks run unconditionally in `threadRenderFrame` and `RaytracingDebug::record`. More seriously, framebuffer collection gates on retained Workload flags rather than the active session, so paused/repeated captured Workloads keep constructing JSON after disarm. In the game patch, neither the off return nor task finalization clears `bindingFrame.token` (`lighting_capture.c:184,223`); `semantic_lights.c::capture_frame` tests only that token and continues recording after a capture ends.

**Correction:** clear/gate producer state at finalization/off; gate allocations, clocks and render-row construction against the active capture generation/mode; allocate aligned lineage storage only for eligible captures. Verify off both from cold startup and after capture/cancel with a retained Workload. Existing matching surface/source counts and PresentMon comparisons do not prove this stricter invariance.

### F4 — FIXED; original finding: clean benchmark is a caller convention

`benchmarkCapture` enables timestamps but does not reject diagnostic lighting views, the RT inset or inspector overlays. The inset can even be recorded as a benchmark interval. `submitOccurrence` constructs/parses/dumps the complete JSON envelope on the render thread and queues file writes after every sample, while the measurement window is still running; the contract calls for a bounded sample array serialized after measurement. This perturbs subsequent CPU/scheduling measurements even though the measured GPU command interval excludes JSON construction.

**Correction:** enforce or reject non-clean effective settings; retain bounded samples and serialize after the window; record/exclude initialization/growth and interruption consistently. Mark RT initialization failure separately from a disabled/not-executed pass. Document that CPU command-record time includes the nested RT resource-preparation interval, or make these measurements disjoint. Record the benchmark workload/paused/interpolation and pacing context. The existing clean Vulkan run supports its particular GPU samples, not a general Stage-3 COMPLETE claim.

### F5 — FIXED; original finding: capture bounds do not cover construction

`rt64_framebuffer_renderer.cpp:1440–1481` builds an uncapped nested `sourceGroups` array for every positive-range RSP light; one outer row consumes only one of the 8192 slots. The outstanding-byte budget is checked after JSON construction/copying in `submitOccurrence`, and excludes these resident Workload rows and temporary representations. A large/modded Workload can therefore consume unbounded diagnostic memory before the advertised drop. The 1 MiB drop artifact only exercises the final enqueue check.

**Correction:** cap origin records during the existing traversal, preserve aggregate counts/dropped-origin counts, and enforce the collection budget before retaining/expanding payloads. Do not add a second geometry traversal. Qualify a synthetic large origin population; no broad runtime test is needed.

### F6 — FIXED; original finding: required facts are missing or falsely numeric

The source and receiver envelope is incomplete in decision-relevant ways:

- Directional binding attempts are zero-initialized but never copy their direction/RGB (`semantic_lights.c:124`); the host exports these zeros as actual parameters. Copy directional fields with a type-specific schema; use unavailable for genuinely unobserved fields.
- Receipt outcomes are frame aggregates, so a failed/mismatched receipt cannot be joined to its bind/draw. Executed annotations export neither actual command/payload bytes nor rejection records for the early invalid-slot return. Add bounded event facts sufficient to explain failed associations, not just successful lineage IDs.
- Receiver rows omit the actual tint proxy and separate RT initialization/failure status. Effective configuration is a subset of requested DrawParams, not actual TraceParams/FramebufferParams and renderer matrices. The game snapshot omits the exact adapter-published generic environment/profile packet, preventing the contract's publication-versus-consumption comparison.
- Manifests omit component dirty identity, profile/mod order/reproduction references and much of MSAA/HFR/pacing/cache context; the shader label is not a shader hash. Record available facts and explicit unavailable statuses, with units/enum interpretation. Do not imply the existing executable hash plus RT64 HEAD identifies dirty sources.

**Correction:** complete these bounded Stage-1–3 facts and their keys; do not implement Stage-4 image exports to compensate. Keep observed-population aggregates distinguishable from capped-row aggregates.

## Trust assessment / verified boundaries

| Stage | Assessment |
|---|---|
| 1 | **QUALIFIED for the current single-Workload Vulkan path:** exact token joins, generation isolation, immutable occurrence evidence and fail-closed unavailable states passed fixture and runtime checks. Natural distinct multi-Workload Vulkan production remains an explicit non-gating gap. |
| 2 | **QUALIFIED for the Stage-4 scenarios about to be used:** bounded schema-2 source/receiver/provenance facts and exact publication/consumption state were observed in the final Vulkan snapshot. Modified-color, cap64/dummy and copied/replaced mod command cases remain **QUALIFICATION GAP**, not defects merely because unexercised. |
| 3 | **QUALIFIED for the current clean Vulkan/AMD path:** the final 16-sample bounded window passed. D3D12 runtime and Metal build/runtime remain **QUALIFICATION GAP**. |

**VERIFIED by targeted source inspection:** production SemanticLight equality/48-byte payload remains unchanged; metadata is not used to grant lighting authority. Dedup restructuring preserves full-byte equality, ordering and cap64; empty upload dummy is counted separately. Receiver gates use production branch results, with safe short-circuit access to indexed-triangle fields. RSP reload/edit clears semantic lineage; vertex snapshot indices preserve its connection without altering production light equality. No lighting shader/equation change, new GPU wait or new submission was found. Vulkan checks result status and used range; D3D12 reads resolved entries after the existing worker wait. Invalid intervals are not serialized as zero durations.

The snapshot available to the original review contained the claimed 1331 receiver rows and two source-group rows, and its executable matched the then-current HANDOFF (`E24A3539…846E10B2`). Those checks established historical artifact/build correspondence. The replacement build and qualification evidence are recorded in the follow-up above.

## Plume and removability

**Plume verdict: ACCEPTABLE WITH FOLLOW-UP.** Exact-range/status reads belong in the backend abstraction: Vulkan availability/period handling and D3D12 readback/frequency handling cannot be implemented honestly by an RT64 caller using the old void API. The contract explicitly requests this generic addition. Keeping it is independently defensible.

Before treating it as a generally reusable API, document nanosecond output, completed-and-written-range preconditions, failure semantics and queue support. Vulkan currently assumes DIRECT queue valid bits and unwraps timestamps in query-index order; that assumption holds for this recorder's sequential writes, not arbitrary generic query-pool use. D3D12 likewise uses the DIRECT frequency. Either explicitly constrain the API or carry the relevant queue information. Metal's range copy alone is not platform validity qualification. These are **NON-BLOCKING for the qualified DIRECT Vulkan caller**, not permission to claim other queues/backends tested.

Only the instrumentation path calls the new overload. Removing the recorder calls permits RT64 to use normal Plume again without redesigning production rendering. The legacy Vulkan conversion edit is a separate generic change that should be reviewed/retained or reverted explicitly.

Cut points are recognizable: four patch-host APIs, one extended marker endpoint, optional Workload/RSP sidecars, framebuffer observation hooks and a nullable timing recorder. N64ModernRuntime and shader ABI are untouched; production lighting has no dependency on evidence identity. There is no build-time exclusion switch, but F3 repaired inactive-path gating with fresh occurrence sidecars and generation-aware gates. Future removal will require deleting these explicit hooks/includes/storage, but no lighting-policy unwind or permanent private-Plume dependency.

## Original handoff corrections and current disposition

HANDOFF's “implementation complete”, “Stage 1 partial solely for multiple Workloads”, capture-only/off-invariance language and general clean-measurement status are stronger than the code. Preserve its historical artifacts and platform limitations, but treat this review as the current gate. No insufficiency of the accepted contract was found.

F3–F6, the full rebuild/link and the short clean Vulkan measurement window are now complete. D3D12/Metal and the other explicitly listed gaps remain unqualified until separately tested. The original scene-labeled gate is satisfied for the current single-Workload Vulkan development path; Stage 4 itself was not started.
