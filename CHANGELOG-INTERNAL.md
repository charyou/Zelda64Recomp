# Internal Changelog

> Konvention: Neueste Session oben, detailliert (Änderungen + Erkenntnisse).
> Einträge, die älter als 3–4 Sessions sind, werden zu Kurzfassungen kompaktiert.
> Details stehen dann nur noch in der Git-History beziehungsweise den verlinkten Projektdokumenten.

## 2026-09-02 — Fog-Kalibrierung, Höhennebel und persistente Grafikoption

### Ergebnisse

- Der Helligkeits-/Sichtweitenunterschied zwischen Original und Faithful ist aufgeklärt: Original begrenzt Fog an den Vertices und interpoliert anschließend, Faithful interpoliert Tiefe und begrenzt die identische lineare Antwort pro Pixel. Der geometrieabhängige Unterschied ist historisches Vertex-Sampling; Faithful erhielt bewusst keinen globalen Multiplikator.
- Atmospheric überführt Faithful in optische Tiefe und verteilt einen semantisch gewichteten Anteil in ein Beer-Lambert-Höhenmedium um. Die zuvor versehentlich additive Kombination wurde nach dem Benutzerbefund „Intro zu stark“ entfernt; die Korrektur vermeidet echten Doppelnebel statt ihn mit einem Dichtemultiplikator zu kaschieren.
- Eine exponentielle bodennahe Dichte, langsame Welt-XZ-Variation und ein aus `sunPos` abgeleiteter Morgenanteil wurden ergänzt. MM's bereits aufgelöste Fog-Farbe bleibt maßgeblich; Sonnenrichtung beeinflusst nur Dichteverteilung und subtile gerichtete Streuung.
- Der Fog-Modus ist jetzt als persistente Option im normalen Grafikmenü verfügbar. Original bleibt Standard; F5 bleibt ein temporärer A/B-Zyklus.
- Atmospheric verlangt neben der Environment-Fog-Signatur nun eine perspektivische Projektion, deren Kamera und Blickbasis MM's aktiver Weltkamera entsprechen. Dadurch erhalten lokale Effekte weiter Faithful und MM's separate Pause-Menü-Perspektivkamera keinen Weltnebel.
- Im F1-Game-Editor gibt es einen nichtpersistenten `Atmosphere`-Tab: aktiver/gerenderter Modus, MM fogNear/zFar, semantische Stärke und fünf Live-Regler für Höhenblend, Skalenhöhe, Variation und gerichtete Streuung.

### Evidenz und Validierung

- Benutzer-Captures decken eine fog-starke Waldsequenz, eine gewöhnliche Low-Fog-Außenszene und den Atmospheric-spezifischen Pause-Menü-Streifenfehler ab.
- Der Benutzer bestätigte die Pause-Menü-Korrektur. Die fast vollständige Modusgleichheit in der Mittags-Außenszene ist durch MM's nahezu inaktives Fog-Signal erwartungsgemäß; Atmospheric ist nicht generell an Morgenzeit gebunden.
- Der separate lokale MM-Decomp bestätigt sowohl MM's 50-Einheiten-Fog-Influence als auch `sunPos` am Morgen und die erzwungene Pause-Perspektivkamera um `(0, 0, 64)`.
- Vollständiger Clang/LLD-Windows-Build einschließlich aller DXIL-/SPIR-V-Varianten erfolgreich. Grafik-RML ist wohlgeformt, IDs sind eindeutig, Struct-Layout-Assertions und `git diff --check` bestehen.
- Der vollständige Build mit optischer Umverteilung und Live-Reglern ist erfolgreich und läuft für den finalen Benutzer-Sichttest in Wald und normaler Außenszene. Bis zu dessen Abnahme beginnt keine Per-Pixel-Beleuchtung.

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
