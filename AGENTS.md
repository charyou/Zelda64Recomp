# Project Agent Guidance

## Purpose

This repository is an independent modernization fork of Zelda64Recomp.

The goal is to substantially improve Majora's Mask technically and visually while preserving its gameplay semantics, identity, atmosphere, and artistic intent.

Upstream acceptance is not a project goal. The upstream Zelda64Recomp maintainer does not want AI-assisted feature contributions submitted upstream.

Future upstream compatibility still matters as an engineering property: prefer designs that make later Zelda64Recomp, RT64, N64ModernRuntime, and related updates practical to integrate when this does not materially compromise the fork.

Within repository-level guidance, explicit instructions in the current user task take precedence over this file when they intentionally grant additional scope or architectural freedom.

Do not treat historical implementation choices as immutable simply because they already exist.

## Sources of truth

Before substantial work, establish the current state from project documentation instead of inferring it from stale code comments or previous assumptions.

Use these sources according to their roles:

* `HANDOFF.md` is authoritative for the current build, runtime state, active blockers, and immediate working context.
* `docs/DECISIONS.md` records durable architectural decisions and their rationale.
* `CHANGELOG-INTERNAL.md` provides recent historical context but must not override a newer `HANDOFF.md`.
* `CHANGELOG.md` is for relevant user-visible release notes, not internal work history.
* `_working-directory/` is disposable local scratch space for research, diagnostics, captures, experiments, temporary scripts, benchmark data, and other non-project memory.
* Relevant detailed research under `_working-directory/` may be used as prior work when explicitly referenced by the current task or `HANDOFF.md`.

For original Majora's Mask behavior and semantics, use:

`G:\_Development\Github\mm`

or the repository's appropriate MM decompilation snapshot when available.

The MM decompilation is the semantic reference for original game behavior. It is not automatically the preferred runtime architecture.

## Architectural principles

Majora-specific semantics should normally be understood and extracted as early as practical, before they are irreversibly reduced to generic N64 rendering state.

As a default ownership model:

* Majora's Mask / the decompilation defines original game semantics.
* Zelda64Recomp owns Majora-specific extraction, adaptation, patches, metadata, and enhanced-runtime integration.
* RT64 owns generic rendering capabilities.
* N64ModernRuntime and related projects own their generic runtime responsibilities.

These are architectural defaults, not prohibitions.

This fork may modify RT64 when the correct abstraction belongs in the renderer or when avoiding RT64 changes would produce a worse architecture.

When modifying RT64:

* keep Majora-specific policy outside RT64 where practical;
* isolate generic renderer changes cleanly;
* minimize unnecessary coupling;
* preserve understandable integration boundaries;
* make future RT64 rebases or replacements tractable;
* document consequential divergence.

Do not contort the architecture merely to keep the RT64 diff small.

Conversely, do not fork generic infrastructure unnecessarily when the same result can be achieved cleanly in Zelda64Recomp.

Follow architectural decisions recorded in `docs/DECISIONS.md`. If current evidence warrants changing one, make the better decision and update the ADR rather than silently working around it.

## Compatibility and fidelity

Keep the original/Native/RDRAM behavior available as a compatibility and correctness reference unless a documented architectural decision explicitly supersedes this policy.

Enhanced rendering may deliberately improve beyond N64 output. Preserve game semantics and artistic intent rather than reproducing every original hardware limitation.

Prefer semantic solutions and general systems over:

* scene-ID hacks;
* hardcoded visual exceptions;
* assumptions about exact vanilla geometry;
* heuristics that could have used available game semantics instead.

Existing mods matter.

Enhanced features should degrade sensibly when mods alter actors, scenes, assets, rendering data, or behavior relative to vanilla Majora's Mask.

Where practical:

* vanilla content should gain enhanced behavior automatically;
* existing mods should continue working through reasonable fallback behavior;
* future mods should be able to opt into richer semantics or APIs.

Do not assume that the user's currently loaded mod stack represents vanilla behavior.

## Upstream and alternative architectures

This is a fork, so architectural improvement takes priority over minimizing the size of the fork.

Still consider maintenance cost and future upstream integration when comparing otherwise viable designs.

You may investigate hybrid runtimes, alternative renderers, custom engine components, Godot, Unreal Engine, or other architectures when evidence suggests they could materially improve the long-term project.

Treat major rewrites as engineering hypotheses.

Do not begin a large migration primarily because a modern engine exposes attractive features. First evaluate the compatibility surface and test the hardest assumption with a focused prototype when appropriate.

## Working style

Infer reasonable task scope from the user's request, repository state, documentation, and available evidence.

Bias toward action and carry requested work toward a useful, validated outcome.

For reversible project-local work, do not stop solely to ask for permission when the user's goal already authorizes the work.

Before asking a clarifying question, perform useful investigation, inspection, diagnostics, experiments, or other reversible work that could resolve the uncertainty.

Do not stop at a plan, research summary, recommendation, or list of files when implementation can reasonably continue.

If independent work can be parallelized with subagents and doing so can save time or improve quality, use them. Give subagents clearly separable responsibilities and synthesize their results before making consequential decisions.

Challenge previous conclusions when evidence warrants it.

Avoid repeating expensive research whose answer is already adequately established unless:

* upstream changed;
* current code contradicts it;
* new runtime evidence conflicts with it;
* or a decision-critical gap remains.

## Runtime and visual validation

Rendering work should be evaluated at runtime whenever reasonably possible.

A successful compile is not sufficient evidence that a visual change is correct.

Use available GUI/computer-control capabilities when they allow direct interaction with the application.

Where direct control is unavailable or unreliable, favor small developer hooks that improve reproducibility, such as:

* direct developer launch paths;
* deterministic game states;
* debug warps or entrances;
* save-state or test-state loading;
* deterministic input playback;
* screenshots;
* short video or frame captures;
* renderer instrumentation;
* image comparisons;
* graphics-mode A/B runs;
* benchmark sequences.

Do not build a large testing framework without evidence that its value justifies the complexity.

Prefer a small reusable validation loop that allows a future agent to reproduce the same visual state before and after a renderer change.

Use the original rendering path and known-good artifacts as comparison references when relevant.

## Testing

Match verification effort to the risk and scope of the change.

Run tests and builds that meaningfully validate the implementation.

Do not repeatedly run broad test suites after unrelated small changes once appropriate checks have passed, unless new changes, failures, or unresolved concerns justify doing so.

Do not create low-value tests that merely duplicate implementation details.

For graphics work, prioritize meaningful visual or runtime evidence where available.

## Build and repository hygiene

Keep ROMs, generated recompilation sources, build products, captures, temporary diagnostics, extracted copyrighted assets, and local experiments in ignored or disposable locations.

Never commit ROM content or distributable copyrighted ROM-derived assets.

Do not assume old build hashes, toolchain pins, generator roles, or workarounds remain current merely because they appeared in an earlier session. Consult `HANDOFF.md` and relevant durable documentation for the current build state.

Use the project-local toolchain and build conventions documented for the current checkout.

The external MM decompilation is primarily a reference source. Do not modify it as part of ordinary work on this fork unless the current task specifically requires that.

## Project memory

After substantial work:

* rewrite `HANDOFF.md` so it accurately represents the current state;
* prepend a concise entry to `CHANGELOG-INTERNAL.md`;
* update `docs/DECISIONS.md` when a consequential architectural decision is made or changed;
* update `CHANGELOG.md` for relevant user-visible changes according to its existing release-note workflow.

Keep `HANDOFF.md` factual and current.

Keep recent internal changelog entries concise. Record outcomes, discoveries, validation, important failures, unresolved issues, and relevant commits rather than raw command history.

Move durable architectural reasoning into `docs/DECISIONS.md` rather than duplicating it across session documents.

Add rules to this `AGENTS.md` only when they are broadly reusable project policy. Do not add transient bugs, current hashes, temporary hypotheses, or session-specific implementation details here.
