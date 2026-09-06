# Runtime validation on Windows

Use a disposable, copied runtime profile and the native Computer Use plugin to
launch a candidate, load a checkpoint, and capture its output. Preserve the known
good and bad oracle build directories. A successful build is not visual evidence.

## Prepare a candidate

Stop applications writing to the source profile, then run from the repository:

```powershell
./tools/Prepare-RuntimeProfile.ps1 `
  -ProfileDirectory ./_working-directory/profile-seed `
  -Executable ./_working-directory/build-control/Zelda64Recompiled.exe `
  -AssetDirectory ./assets `
  -OutputDirectory ./_working-directory/runtime-checks/control
```

Use the actual configured profile as `ProfileDirectory`; the path above is an
example. The output must be a new directory outside the source directories.
`RuntimeDirectory` optionally selects the source of DLLs/controller mappings when
they are not beside the candidate executable. `AdditionalProfilePath` accepts
relative mod-owned files or directories outside the standard profile roots.

The tool copies the five core configuration files and their backups, configured
ROM, saves, all installed mods and mod configuration, candidate executable,
runtime DLLs, controller mappings, and assets. It uses independent files rather
than shared links so candidate saves and configuration writes cannot change the
source. Large texture packs make complete snapshots several gigabytes; reuse an
unchanged seed for comparisons and prepare only candidates needed by the current
experiment. Nested source links are rejected; materialize them first.

Allow free disk space for the complete copied profile, executable, DLLs and assets.
Before writing anything, the preparation tool checks local drive-letter volume
capacity against the enumerated file sizes plus a 16 MiB cushion and rejects an
insufficient-space run. This is a best-effort check: network paths, mounted-volume
redirects, concurrent disk use and filesystem allocation overhead can still cause
a copy failure. The actual large mod installation alone is approximately 3.5 GB.

Each copied file is SHA-256 checked against its source. `manifest.json` records
the initial snapshot and source locations; `portable.txt` selects the copied
profile. A failed preparation retains `PREPARATION-INCOMPLETE.txt`; do not launch
that directory. The manifest does not prove executable source provenance or the
runtime's resolved mod load order. Record a separate build manifest and retain
runtime logs. The copying check is not an atomic filesystem snapshot; source
writers must remain stopped throughout preparation.

Keep profiles, ROMs, manifests and captures in ignored `_working-directory/`.
Never publish their copyrighted contents. The tool itself does not launch the
game or change the copied graphics settings.

## Launch on the interactive desktop

**The working directory is essential.** On Windows,
`src/main/support.cpp::get_program_path()` returns an empty path: assets and the
controller database resolve from CWD, as does `portable.txt`. Starting an
executable by full path does not select its profile or assets automatically.

```powershell
$runDirectory = (Resolve-Path ./_working-directory/runtime-checks/control).Path
Start-Process -FilePath (Join-Path $runDirectory 'Zelda64Recompiled.exe') `
  -WorkingDirectory $runDirectory -WindowStyle Normal -PassThru
```

In the 2026-09-06 Codex environment, an ordinary sandboxed `exec` launch produced
a process/window that native `sky` desktop enumeration could not target. An
approved `exec_command` call with `sandbox_permissions: "require_escalated"`,
explicit CWD, and `Start-Process -WindowStyle Normal` exposed the interactive
frontend to `sky`. Use the normal approval mechanism for that launch when this
environment requires it. This is an observed environment boundary, not evidence
of an application startup defect. Do not use a hidden-window launch for a game
that must be visually inspected.

`Failed to set working set size!` / `Failed to preload executable!` are nonfatal
warnings: `main()` continues. Empty redirected stdout can reflect buffering and
does not locate a startup stall.

## Observe and control

Read the installed `computer-use:computer-use` skill and its guidance before
automation. The native Windows path is the persistent `node_repl` session with
the supported `@oai/sky` package. It is separate from the browser CUA interface,
which may report native APIs disabled.

Initialize `sky` as directed by the skill, then call `sky.list_apps()` or
`sky.list_windows()` and select exactly one returned window belonging to the
candidate's executable. Capture it with
`sky.get_window_state({ window, include_screenshot: true, include_text: true })`.
Inspect the returned state before acting. Use `sky.click` for the observed
`Start Game` button and `sky.press_key` for game inputs, each followed by a fresh
observation. Save returned screenshot data to ignored capture files according to
the skill. Do not build custom Win32 input or screenshot helpers.

Native mouse control can press `Start Game`. In this environment native keyboard
injection did not consistently reach SDL gameplay input, so an explicit developer
playback path is available below. Quick-save/state restoration code is deliberately
disabled and must not be treated as a functioning reproducibility mechanism.

## Opt-in startup and controller playback

Set `ZELDA64RECOMP_DEV_AUTOSTART=1` in the candidate's launch environment to start
the configured, validated ROM at the first initialized UI draw. This uses the
same start/hide-menu operations as the launcher button. A missing ROM or another
nonempty variable value logs a rejection and leaves the launcher available.

Set `ZELDA64RECOMP_DEV_NATIVE=1` for a Native/RDRAM presentation run. After renderer
setup succeeds it invokes the same RT64 presentation switch as F3, once. Without
the variable, enhanced presentation is unchanged. Any other supplied value logs
a rejection and retains enhanced presentation. When the variable is supplied,
stderr records the selected graphics API, configured fog mode, and presentation
choice. This permits separate Native/enhanced runs with identical copied profiles
and input scripts when OS keyboard injection is unavailable. Native remains the
original compatibility rendering path regardless of the configured enhanced fog.

Set `ZELDA64RECOMP_DEV_INPUT` to a UTF-8 JSON file path to replace controller 0
with a finite input sequence. Relative paths resolve from runtime CWD. The file
is read once, on the first controller-0 read. For example:

```json
[
  { "reads": 300, "buttons": 0, "x": 0, "y": 0 },
  { "reads": 2, "buttons": 4096, "x": 0, "y": 0 },
  { "reads": 120, "buttons": 0, "x": 0, "y": 0 }
]
```

This example waits 300 controller reads, holds N64 Start for two reads, then
releases it. It is a format example, not a qualified save-loading recipe. Button
masks include Start=4096, A=32768, B=16384; combine buttons by adding their masks.
Every step must contain exactly `reads`, `buttons`, `x`, `y`. Reads must be integers
from 1 to 1,000,000; buttons integers from 0 to 65,535; stick axes finite numbers
from -1 to 1. A file may contain 1–4096 steps, at most 10,000,000 reads total, and
must be at most 1 MiB.

Playback advances per controller-0 read, independent of output refresh rate.
It bypasses physical controller-0 input and menu input suppression; keep the
launcher/config menus closed during playback. After its last sample, controller 0
stays neutral until process exit. Malformed or unavailable files log a rejection
and also keep controller 0 neutral, so an invalid script cannot silently become
an uncontrolled run. Start and completion are logged to stderr. Neither variable
changes normal behavior when absent or empty; restart without them for normal play.

Use the copied profile because the game may autosave. Record the script hash and
all developer variables beside the run manifest; the profile-preparation tool does not
set launch environment variables. The override covers N64 controller input;
separate gyro/mouse/free-camera or mod-specific input paths are not replayed.
Read-count playback does not establish deterministic simulation: verify the
scene, time, camera, and animation state in captured evidence.

The 2026-09-06 copied profile was loaded successfully with these nine steps:
neutral 300 reads, Start 3, neutral 120, A 3, neutral 120, A 3,
neutral 120, A 3, neutral 300. This selects and confirms file 1; it is qualified
for that seed, not every save/mod configuration. The actual JSON and a variant
that subsequently presses Start to pause are in the session diagnostics directory
as `load-save.json` and `load-save-pause.json`. Restore the seed save before each
run; autosaves and the running game clock otherwise change the checkpoint.

`ZELDA64RECOMP_FOG_MODE=original|faithful|atmospheric` selects an explicit fog
reference at launch. Restart per mode when keyboard F5 injection is unreliable.

## Compare evidence

1. Start every candidate from the same stopped profile seed. Record candidate
   hash, graphics API, resolution, fog mode, refresh setting, config/mod manifest,
   and save hash. Record any settings changed after preparation.
2. Load the same save and reach the same scene, camera, day/time, and game state.
   Keep the same enabled mod configuration; archives alone do not establish
   identical mod behavior. Use a copied no-mod profile as a separate experiment.
3. Capture several enhanced frames and an F3 Native comparison. F5 cycles the
   session fog mode; the Graphics menu stores its persistent choice. Verify which
   mode is active rather than inferring it from the image.
4. Save screenshots and a short observation log with checkpoint details. Camera,
   animation, weather, and timing differences can invalidate pixel-difference
   metrics; do not describe manual checkpoint captures as deterministic replay.

Existing game hooks can queue a debug warp and time through `src/game/debug.cpp`
and consume them in `patches/debug_patches.c`. They are potential future aids if
checkpoint setup becomes the bottleneck. Controller playback is implemented at
`recomp::get_n64_input`; pushing SDL keyboard events alone does not update the
keyboard-state array used by gameplay input.
