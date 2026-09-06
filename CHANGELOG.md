<!-- This file is compiled automatically during the release workflow. -->
<!-- Do not edit manually — your changes will be overwritten. -->
<!-- To update the draft: ask the agent to use the draft-release-notes skill. -->
<!-- To finalize a release: ask the agent to use the release-bump skill. -->

## Unreleased

### Added

- Added experimental Faithful Per-Pixel and Atmospheric fog modes while retaining Original fog as the compatibility default. The modes can currently be tested with F5 or `ZELDA64RECOMP_FOG_MODE`.
- Added enhanced per-pixel diffuse lighting for compatible smooth-shaded models, using their existing lights and normals. Native and unsupported or modified draws retain original lighting; F1's Lighting tab and `ZELDA64RECOMP_LIGHTING=original` provide an A/B switch.

### Changed

- Updated the embedded RT64 renderer to the qualified Plume baseline required for the modern fog work.
- Atmospheric fog now uses resolved Majora's Mask environment color and distance data while preserving draw-local actor and effect fog.
- Nearby active water surfaces now strengthen wet-air atmosphere automatically. Skyless world views retain conservative distance haze, while short interiors stay nearly clear and mod overrides remain authoritative.

### Fixed

- Fixed radial geometry corruption by removing the experimental camera-matrix publication path.
- Fixed incremental builds retaining stale shaders or patch registration metadata, including the resulting code-mod loading error.
- Fixed DXC dependency generation suppressing actual shader compilation, which could pair new renderer layouts with stale shader binaries.
- Fixed severe flickering and color and geometry corruption on AMD Radeon RX 9000-series GPUs caused by the initial modern-fog raster shader linkage.
- Added automatic Vulkan selection for affected AMD RDNA4 GPUs to avoid a known Direct3D 12 startup crash.
