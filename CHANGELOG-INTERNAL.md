# Internal Changelog

> Konvention: Neueste Session oben, detailliert (Änderungen + Erkenntnisse).
> Einträge, die älter als 3–4 Sessions sind, werden zu Kurzfassungen kompaktiert.
> Details stehen dann nur noch in der Git-History beziehungsweise den verlinkten Projektdokumenten.

## 2026-09-09 — Hardware RT primary-hit foundation, Vulkan runtime proof

- Implemented RT64 `RaytracingDebug` using the existing HFR world-position/index buffers, conservative executable opaque ranges, world-space BLAS, identity TLAS, isolated descriptors, a minimal raygen/miss/closest-hit library, SBT, real `traceRays` and a visible barycentric-hit inset. `RT64_RT_PRIMARY_HIT=1` opts in; the existing RayTracing shortcut shares its state. No legacy `RT_ENABLED` restoration, raster linkage change, MM/runtime patch, or upstream archaeology.
- Corrected generic nested Plume Vulkan AS contracts: per-geometry BLAS build ranges, scratch alignment, consistent TLAS flags and actual AS device address. Initial runtime encountered device loss; user noted preexisting driver instability. A definite aggregate-range bug was independently found and fixed; do not attribute the driver crash solely from timing.
- Final Clang/LLD build and actual DXIL/SPIR-V/wrapper generation passed. Vulkan RX 9070 XT Town/save `a` visibly showed architecture, ground and animated Link in the RT inset. Relaunching the same binary with RT off restored coherent Enhanced output. F2 injection did not visibly toggle; launch-time A/B was used. Corrected run had no further device-loss errors or obvious severe corruption. No validation-layer run, D3D12 runtime, MSAA, broad HFR/scene-transition or performance qualification.
- Executable SHA `925F6959DD675608C9A14FEC5739B6A47DCA51D0896C0FC38A21DC6D7C6068BC`. Evidence: `_working-directory/diagnostics/2026-09-09-rt/`, including on/off captures, build log and reproducible launcher using the existing copied profile. Test processes stopped and copied seed restored. Native/lighting/fog baseline and parked cutout experiment preserved.
- Updated `HANDOFF.md`, added `docs/RAYTRACING_FOUNDATION.md` and ADR-008. The generated `CHANGELOG.md` is unchanged: this run delivers an opt-in developer diagnostic, not a normal release-facing lighting feature. Changes remain uncommitted in parent, RT64 and nested Plume; preserve all three layers. Next: a directional visibility ray within the debug pipeline, retaining primary-hit reference mode.

## 2026-09-09 — Broader semantic per-pixel lighting coverage

- Visible Town tint diagnostics identified the whole-draw normal-length restriction as the major avoidable environment rejection. Short/zero/varying normals now retain per-vertex strength through a scalar varying; the shader normalizes direction separately. No draw splitting or authored-color relighting.
- Light-set equality uses relevant values, not buffer identity. Shared-matrix draws reuse the original local directional equation, including nonuniform scale/shear. Mixed unsupported state and true positional lights remain legacy. Temporary aggressive modes removed; spatial diagnostics retained.
- Targeted build and Vulkan Town enhanced/original/Native checks passed with the copied 44-NRM/one-RTZ stack. No shader-sorting errors or obvious geometry/material corruption; captures are not frame-matched. Final executable SHA `7C3637B5034E3D30C6D6D06DEF66850AD288070663562949E1AA200A2D65E59A`. D3D12 runtime unqualified. Evidence: `_working-directory/diagnostics/2026-09-07-coverage/`.
- Resumed single-agent after restart. Shadow findings and prior cutout-AA prototype parked; cutout disabled in lighting runs/launcher. Atmosphere unchanged. Updated lighting documentation and ADR-007.

## 2026-09-07 — Per-pixel diffuse lighting and small semantic atmosphere follow-ups

- Implemented enhanced per-pixel ambient/directional lighting using actual RSP normals/light sets and HFR world matrices. Different skeleton limb transforms can share a draw; incompatible transforms, mixed lights, authored color overrides, varying normal magnitudes and actual positional microcode lights preserve legacy shading. Native remains original. Zelda enables the feature; F1 Lighting and `ZELDA64RECOMP_LIGHTING=original` provide A/B control. ADR-007 and `docs/PER_PIXEL_LIGHTING.md` document the boundary.
- Runtime feedback refined two initially over-conservative assumptions: uniform world indices excluded batched limbs, and a 127-unit normal gate excluded the sampled approximately 120-unit normals. Final code preserves consistent authored normal magnitude rather than brightening it to one. The title logged 66 eligible / 2 legacy draws; the Swamp checkpoint logged about 122-123 eligible draws. Completion-timed original/per-pixel captures show a modest model-shading improvement with clean geometry and the existing mod stack.
- The first prototype exposed a remaining shader-build defect: bundled DXC `-MD` emits only dependencies. Old binaries remained despite “Generating” messages, causing CPU RDP336 / shader RDP320 corruption even with lighting off. Separate compile and `-M -MF` passes now produce real objects and depfiles. Included-source mutation changes DXIL/SPIR-V hashes; reflection and targeted no-op checks pass. This supersedes the prior unchanged-hash dependency qualification. A dynamically bounded lighting loop also failed re-spirv sorting; explicitly unrolling the seven supported directional lights resolved that failure.
- Added water-derived atmospheric moisture from active room-scoped/static and dynamic collision water boxes, with area/altitude weighting and a masked mod override. Automatic skyless views now receive conservative distance haze; explicit mod OFF still wins and short rooms remain effectively clear. No scene IDs, Swamp constants, new precipitation logic, or broad fog survey.
- Full Clang/LLD build passed. Final executable SHA-256: `8C910FA1F98C139A94FFDDC78B0990D468291AF0607F75DDB32D0ADD20FF0C9A`. Final Atmospheric combined smoke and explicitly logged Native/RDRAM reference are clean; copied saves/configuration restored and test processes stopped. Primary runtime API is Vulkan; explicit D3D12 selection is forced back to Vulkan by the existing AMD driver workaround, so DXIL compilation is not D3D12 runtime qualification. Test stack: 44 NRM + 1 RTZ archives / 46 enabled entries; the prior 47 archive count included two loose source files. Evidence/launcher: `_working-directory/diagnostics/2026-09-06-lighting/`. See current `HANDOFF.md` for limits.

## 2026-09-06 — Geometry recovery and a working visual validation loop

- Reproduced radial wedges directly through native window capture, first in the bad oracle and then in a fresh current-source control. The copied modded Southern Swamp owl save remained broken with fresh shaders and corrected atmosphere-camera indices; omitting the four camera-publication EGBI commands restored clean geometry. Native/RDRAM also rendered the checkpoint correctly.
- Withdrew the entire float-camera perspective patch and precision switches; restored generic RT64 float commands for mods. ADR-006 now records the affine model/world constraint and limited causal A/B evidence. Original MM camera/distortion behavior and existing interpolation tags remain.
- Fixed two independently reproduced build defects. RT64 shaders lacked transitive include dependencies (historical vertex shaders had 176-byte RDP layouts, current CPU/shaders 320). Root patch generation could leave old registration tables linked with new patch code, reproducing the blank-name code-mod error. DXC depfiles and explicit generated-header dependencies now rebuild the correct consumers in one invocation.
- Fixed missing index-zero identity sentinels in the semantic camera arrays. Added assertions for projection-index alignment.
- Added opt-in autostart, finite controller-read playback and Native startup selection, plus isolated-profile preparation/hashes/free-space checks. Mouse/window capture works through native Computer Use on the interactive desktop; key injection is unreliable. A nine-step sequence now loads the copied file-1 save; an extended sequence reaches pause. No broad automation framework or save-state system was introduced.
- Full Clang/LLD build passed. Targeted dependency tests verified selective rebuilds and no-op behavior; playback/profile checks passed. Final executable SHA-256 is `D6F7628E560797A20FCCF25FAA8CBF7F8B8244FF360215B8393527B0BEB46E51`. Final Atmospheric pause inventory is clean; scene/mode results and remaining coverage are recorded in the regression document.
- Final smoke checks also passed enhanced Original and Faithful at the owl and the same save without external mod archives. The copied mod profile/seed was restored afterward. Both historical oracles are intact. Tested seed contains 47 mod archives / 46 enabled entries, not proven coverage of the user's full installation. A second large profile copy exhausted disk; only its incomplete output was removed. Broad fog/effect qualification remains pending.
- Evidence: `_working-directory/diagnostics/2026-09-06/`; durable workflow: `docs/RUNTIME_VALIDATION.md`; current build and next step: `HANDOFF.md`. Work remains uncommitted alongside prior fog changes.

## 2026-09-06 — Enhanced-Geometrieregression und neutrale Übergabe

### Befund

- Der Benutzer bestätigte im 22:15-Build große schwarze beziehungsweise grüne Dreiecke, die ungefähr radial aus der Bildschirmmitte laufen und sowohl Szenengeometrie als auch Skybox betreffen. F3/Native rendert denselben Zustand korrekt. `Refresh Rate Mode = Original` brachte keine Besserung.
- Der erhaltene Build vom 2026-09-02 16:19 läuft mit derselben praktischen großen Mod-Installation fehlerfrei. Damit ist die frühere Zuschreibung explodierender Geometrie an den gleichzeitig aktiven Mod-Gesamtstack allein nicht haltbar. Mod-Reihenfolge und Config-Root bleiben mögliche Interaktionsvariablen, aber die Executable-/Patch-/RT64-Differenz ist der zentrale Untersuchungsraum.
- Der gute Build wurde mit SHA-256 `F9F3CD...DFD3B1D`, der fehlerhafte mit `7529CC...EFDB0D` gesichert. `RecompiledFuncs.lib` ist byte-identisch; `PatchesLib.lib` und `rt64.lib` unterscheiden sich. ROM, geprüfte Modarchive, Runtime-DLLs und die zuvor verglichenen Shaderartefakte waren identisch.
- `Refresh Rate Mode` ist als alleinige Ursache ausgeschlossen. Ein Vanilla-Lauf steht noch aus.

### Config- und UI-Korrektur

- Der zuvor empfohlene F1-Test für Float View/Projection ist im Rasterbuild nicht erreichbar: Der gesamte `Game`-Tab liegt in RT64 innerhalb von `#if RT_ENABLED`. Beide Atomics sind session-lokal und standardmäßig `false`; eine alte `graphics.json` erklärt den fehlenden Tab nicht.
- Standardmäßig ausgeschaltete Atomics bilden trotzdem keinen echten Altpfad, weil der gepatchte `View_ApplyPerspective` die vier Float-Matrix-EGBI-Kommandos für OPA/XLU stets emittiert. Ein belastbarer A/B-Build muss diese Veröffentlichung beziehungsweise den gesamten Patchpfad umgehen.

### Übergabe und nächster Entscheid

- `docs/GRAPHICS_REGRESSION_HANDOFF.md` enthält die vollständige, bewusst ergebnisoffene Übergabe: Build-Hashes, bestätigte Ausschlüsse, Zeit-/Source-Grenzen, offene Hypothesen, A/B-Kandidaten und einen automatisierbaren Human-in-the-loop-Screenshotplan.
- Es wurde noch nicht entschieden, ob der aktuelle Stand gezielt repariert, in Source-Kohorten bisektiert oder von einem rekonstruierten guten Zustand neu aufgebaut wird. Der gute 16:19-Build ist ein binärer Oracle, aber keinem sicher reproduzierbaren Commit zugeordnet; ein unmittelbarer Rücksprung auf `f5cac21` wäre daher keine verlässliche Wiederherstellung.
- Bis zur Behebung und erneuten Fog-Qualifizierung beginnt kein Per-Pixel-Lighting- oder weiterer Renderer-Upgrade-Schritt.

## 2026-09-02 — Fog-Mod-API, Coverage-Diagnostik und Float-Kamera-A/B

### Ergebnisse

- Atmospheric stellt Code-Mods jetzt pro Gameplay-Frame das Event `recomp_on_atmosphere_override` bereit. Ein Bitmasken-Struct erlaubt unabhängige Overrides der acht Atmosphärenparameter und der Outdoor-Klassifikation; nicht beanspruchte Felder bleiben bei Renderer-/F1-Werten und der Zustand wird pro Frame neu initialisiert.
- Die öffentliche C-Definition liegt in `include/z64recomp_atmosphere_api.h`; `docs/MODERN_FOG_MODDING.md` dokumentiert scene-/room-spezifische Callbacks am Swamp-Beispiel. Maskierte Mod-Werte haben Vorrang vor Live-Reglern.
- Die automatische Outdoor-Erkennung besitzt nun eine separat über F1 schaltbare erweiterte Variante: normale Räume mit natürlichem `SKYBOX_NORMAL_SKY`/`SKYBOX_3` bleiben auch bei durch Cutscenes deaktiviertem Skybox-Draw outdoor. Der alte sichtbare-Skybox-Test läuft parallel für A/B; echte skyboxlose Shops bleiben konservativ.
- Der F1-Atmosphere-Tab zählt Atmospheric-Draws und den jeweils ersten Fallback-Grund (Viewport, Fog-State, Tiefe, Projektion, Kamera oder Fog-Signatur). Damit lassen sich fehlende Clock-Town-/Cutscene-Bereiche diagnostizieren, bevor Sicherheitsgrenzen gelockert werden.
- Der normale MM-Perspective-View-Pfad veröffentlicht parallel zur unveränderten Fixed-Matrix eine `guLookAtF`-View und eine `guPerspectiveF`-Projection. Die Float-Projection erhält nach einmaligem `View_StepDistortion` dieselbe endgültig aufgelöste Rotations-/Skalentransformation.
- RT64 kann Float View und Float Projection im F1-Game-Tab unabhängig auswählen; beide A/B-Schalter sind session-lokal und bis zur Sichtprüfung standardmäßig aus. Bei ausgeschaltetem Schalter wird explizit die bestehende Fixed-Matrix weiterverwendet.
- Workloads behalten zusätzlich die echten finalen Fixed-RSP-View-/ViewProjection-Matrizen. Fog-Kameraerkennung und World-Reconstruction verwenden diese statt der bei Float-EGBI entstehenden Korrekturmatrizen, sodass Float-Tests Atmospheric nicht versehentlich deaktivieren.

### Validierung und offene Prüfung

- Vollständiger Clang/LLD-Windows-Build einschließlich Patch-Rekompilierung, neuem Event, RT64 und aller DXIL-/SPIR-V-Varianten erfolgreich. Der Event-Symbol wurde in den generierten Patch-Overlays bestätigt; ABI-Assertions sichern 40 Byte Override und 112 Byte Environment-Payload.
- Eine anschließend gemeldete Mod-Loader-Regression mit leerem Modnamen wurde als Fehler der globalen Hook-Regenerierung eingegrenzt. Nach vollständiger Patch-Neugenerierung und Neulink durchlief ein instrumentierter Portable-Test 159 Hook-Slots, 99 regenerierte Vanilla-Funktionen sowie 13 Hooks auf acht Basispatch-Funktionen erfolgreich; auch der zuvor aktive alte `GameState_Update`-Hook lud. Die temporäre Instrumentierung wurde wieder restlos entfernt und der saubere Build erneut erfolgreich erzeugt. Der einmalige Fehler ist damit nicht reproduzierbar und spricht für inkonsistente/stale generierte Build-Artefakte statt für einen Bruch der additiven Event-API.
- Im Diagnoseprofil waren absichtlich nur ein Mod und keine vollständige bekannte Mod-Reihenfolge eingetragen. Dadurch aktivierte die bestehende First-Scan-Logik fast alle übrigen installierten Mods standardmäßig; der anschließende Lauf zeigte doppelte Figuren und explodierende Polygone. Die damalige Zuschreibung an den Mod-Gesamtstack wurde am 2026-09-06 durch den fehlerfreien 16:19-Build mit derselben praktischen Mod-Installation widerlegt. Diagnoseprofil, Log und Console-Kopie waren bereits entfernt worden.
- `git diff --check` besteht in Projekt, RT64 und N64ModernRuntime abgesehen von erwarteten Windows-Zeilenendhinweisen.
- Kein Gameplay-Test in dieser Session. Ausstehend sind die Outdoor-A/B-Prüfung in den auffälligen Clock-Town-/Cutscene-Bereichen sowie Float View/Projection einzeln und gemeinsam bei langsamen Schwenks, hoher Ausgabe-FPS, Cutscenes, Pause und einem Distortion-Effekt.
- Float-Matrizen adressieren Fixed-Point-Subpixeljitter und die Qualität der HFR-Interpolation, nicht MM's logische Kameraupdate-Rate oder absichtlich stufige Kameralogik.

### Dauerhafte Referenzen

- Mod-API: `docs/MODERN_FOG_MODDING.md` und `include/z64recomp_atmosphere_api.h`.
- Architektur: `docs/DECISIONS.md`, ADR-005 und ADR-006.
- Aktueller Build- und Teststand: `HANDOFF.md`.

## 2026-09-02 — Fog-Kalibrierung, Höhennebel und persistente Grafikoption

### Ergebnisse

- Der Helligkeits-/Sichtweitenunterschied zwischen Original und Faithful ist aufgeklärt: Original begrenzt Fog an den Vertices und interpoliert anschließend, Faithful interpoliert Tiefe und begrenzt die identische lineare Antwort pro Pixel. Der geometrieabhängige Unterschied ist historisches Vertex-Sampling; Faithful erhielt bewusst keinen globalen Multiplikator.
- Atmospheric überführt Faithful in optische Tiefe und verteilt einen semantisch gewichteten Anteil in ein Beer-Lambert-Höhenmedium um. Die zuvor versehentlich additive Kombination wurde nach dem Benutzerbefund „Intro zu stark“ entfernt; die Korrektur vermeidet echten Doppelnebel statt ihn mit einem Dichtemultiplikator zu kaschieren.
- Eine exponentielle bodennahe Dichte, entlang des Sichtstrahls gemittelte langsame Welt-XZ-Variation und ein aus `sunPos` abgeleiteter Morgenanteil wurden ergänzt. Ein optisches Headroom-Budget bildet die Benutzer-Kalibrierung `strength 1.0 -> ~0.02` und `strength 0.2 -> ~0.065` kontinuierlich ab. MM's bereits aufgelöste Fog-Farbe bleibt maßgeblich.
- Sichtbare Outdoor-Weltkameras erhalten eine schwache Clear-Air-Aerial-Perspective als Transmittanz-Untergrenze am MM-zFar. Aktuelle Regen-/Schneemengen sowie Sturm/Blitz erhöhen sie kontinuierlich; Innenräume und lokale Fog-Overrides bleiben ausgeschlossen.
- Der Fog-Modus ist jetzt als persistente Option im normalen Grafikmenü verfügbar. Original bleibt Standard; F5 bleibt ein temporärer A/B-Zyklus.
- Atmospheric verlangt neben der Environment-Fog-Signatur nun eine perspektivische Projektion, deren Kamera und Blickbasis MM's aktiver Weltkamera entsprechen. Dadurch erhalten lokale Effekte weiter Faithful und MM's separate Pause-Menü-Perspektivkamera keinen Weltnebel.
- Im F1-Game-Editor gibt es einen nichtpersistenten `Atmosphere`-Tab: Modus, MM fogNear/zFar, semantische Stärke, Outdoor-/Wetterdiagnostik und acht Live-Regler einschließlich optischem Sättigungsbudget sowie Clear-/Wet-Air-Transmittanz.

### Evidenz und Validierung

- Benutzer-Captures decken eine fog-starke Waldsequenz, eine gewöhnliche Low-Fog-Außenszene und den Atmospheric-spezifischen Pause-Menü-Streifenfehler ab.
- Der Benutzer bestätigte die Pause-Menü-Korrektur. Die fast vollständige Modusgleichheit in der Mittags-Außenszene ist durch MM's nahezu inaktives Fog-Signal erwartungsgemäß; Atmospheric ist nicht generell an Morgenzeit gebunden.
- Ein erster Test der neuen Clear-Air-Schicht zeigt nun klare Modusunterschiede und ein gutes normales Outdoor-Bild. Gerichtete Streuung ist sichtbar und gestalterisch wirksam; Regen/Sturm sowie der automatisch begrenzte dichte Intro-Nebel stehen noch aus.
- Der separate lokale MM-Decomp bestätigt sowohl MM's 50-Einheiten-Fog-Influence als auch `sunPos` am Morgen und die erzwungene Pause-Perspektivkamera um `(0, 0, 64)`.
- Vollständiger Clang/LLD-Windows-Build einschließlich aller DXIL-/SPIR-V-Varianten erfolgreich. Grafik-RML ist wohlgeformt, IDs sind eindeutig, Struct-Layout-Assertions und `git diff --check` bestehen.
- Der vollständige Build mit optischer Umverteilung und Live-Reglern ist erfolgreich und läuft für den finalen Benutzer-Sichttest in Wald und normaler Außenszene. Bis zu dessen Abnahme beginnt keine Per-Pixel-Beleuchtung.
- Die Upstream-Trennung bleibt erhalten: Projektcommit `0faf84a` ist das reine RT64-Upgrade direkt auf `dev`; Fog beginnt separat mit `9b3d2a1`/RT64 `05394e9`. Ein Maintenance-PR kann daher zunächst nur das Renderer-Upgrade übernehmen.
- Die lokalen Fog-RT64- und N64ModernRuntime-Stände sind über deren konfigurierte Upstream-Remotes nicht erreichbar; vor Veröffentlichung eines Fog-Superproject-Branches müssen die Gitlinks auf zugängliche Fork-Commits zeigen.

### Dauerhafte Referenzen

- Aktueller Stand und konkreter Sichttest: `HANDOFF.md`.
- Workload-Metadaten und Weltkamera-Klassifikation: `docs/DECISIONS.md`, ADR-002/ADR-003.

## 2026-09-02 — Moderne Fog-Modi und RDNA4-Laufzeitqualifizierung

### Änderungen

- Zelda64Recomp wurde auf den qualifizierten RT64-/Plume-Stand migriert; vorhandene MatrixGroup-Aufrufer erhielten verhaltensneutrale Texcoord-/LookAt-Komponenten.
- Original blieb der Kompatibilitätsstandard. Faithful Per-Pixel und Atmospheric wurden für den Enhanced-Framebuffer-Replay ergänzt und lassen sich vorläufig per F5 oder `ZELDA64RECOMP_FOG_MODE` auswählen.
- Aufgelöste Majora's-Mask-Umgebungsdaten aus `PlayState.lightCtx` werden als Workload-Metadaten an RT64 übergeben. Lokaler Actor-/Effect-Fog bleibt pro Draw maßgeblich.
- Der initiale zusätzliche Fog-Shader-Interpolant wurde entfernt. Atmospheric rekonstruiert Clip-W nun aus `SV_Position.w`, ohne RT64s Raster-Linkage-ABI zu erweitern.
- Für AMD Radeon RX 9000/RDNA4 wird im automatischen Grafikmodus Vulkan gewählt, um den bekannten D3D12-Startabsturz zu umgehen.
- Öffentliche und interne Änderungsverfolgung wurden mit `CHANGELOG.md` und diesem begrenzten Sitzungsprotokoll etabliert.

### Erkenntnisse

- Ein zusätzlicher `TEXCOORD1`-Varying verursachte auf einer RX 9070 XT unter Vulkan starkes RGB-Flackern sowie Farb-/Geometriekorruption; D3D12 stürzte in `D3D12Core.dll` mit `0xc0000005` ab.
- Ein A/B-Build ohne den neuen Cross-Stage-Interpolant lief fehlerfrei. RDP-/Framebuffer-Struct-Layouts, Submodule, HPFB und die Plume-Migration wurden als Ursachen ausgeschlossen.
- Original und Atmospheric waren in der ersten Waldszene deutlich dichter als Faithful. Das ist nun eine Kalibrierungs- und Klassifizierungsfrage, kein Stabilitätsfehler.
- Atmospheric modelliert bislang Distanzextinktion, aber noch keinen höhenabhängigen Boden- oder Talnebel.
- Der rohe Windows-Build ist absichtlich kein vollständiges Release-Paket: Er muss aus dem Repository-Root gestartet werden; andernfalls müssen `assets/` und `recompcontrollerdb.txt` als passende Sidecars neben der EXE liegen.

### Validierung

- Vollständiger Windows-RelWithDebInfo-Build mit Clang 19.1.3 und allen DXIL-/SPIR-V-Varianten erfolgreich.
- Original, Faithful Per-Pixel und Atmospheric auf einer AMD Radeon RX 9070 XT im Spiel ohne Farb- oder Geometriekorruption getestet.
- `Auto` mit HPFB `Auto` wählte nach dem RDNA4-Fallback nachweislich Vulkan (`vulkan-1.dll` und `amdvlk64.dll`); das Bild blieb korrekt.
- Alle sieben direkten und alle 15 verschachtelten RT64-Submodule stimmten mit ihren erwarteten Gitlink-SHAs überein.

### Offene Punkte

- Capture-Matrix für Übergänge, Transparenz, Wasser, Partikel, UI, Framebuffer-Feedback, Lens of Truth und hohe Frameraten vervollständigen.
- Faithful gegen Original kalibrieren, Atmospheric-Distanzextinktion abstimmen und einen subtilen optionalen Höhenterm ergänzen.
- Den persistenten Grafik-UI-Selector erst nach visueller Qualifizierung der Fog-Semantik freigeben.

### Referenzen

- Projekt: `0faf84a` (RT64-/Plume-Basis), `9b3d2a1` (MM-Umgebungsbridge), `4e2242f` (RDNA4-Laufzeitfix).
- RT64: `05394e9` (Fog-Modi), `c8ce62b` (Raster-ABI- und RDNA4-Fix).
- Architektur: `docs/DECISIONS.md`, ADR-001 bis ADR-004.
- Aktueller Arbeitsstand und nächste Schritte: `HANDOFF.md`.
