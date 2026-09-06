# Enhanced per-pixel lighting

The enhanced renderer evaluates the existing RSP ambient and directional lights on interpolated, normalized model normals. This smooths diffuse shading across character triangles without inventing lights, changing textures, adding specular highlights, or changing the N64 combiner/blender.

Zelda enables it at startup. Set `ZELDA64RECOMP_LIGHTING=original` for legacy vertex lighting, or use the session-local checkbox in **F1 → Game editor → Lighting** when debug mode is enabled. The lighting selection is independent of fog mode. Native/RDRAM always uses original vertex lighting.

## Eligibility and compatibility

RT64 checks the actual vertices referenced by each indexed, smooth-shaded draw. All must agree on their RSP light set and contain usable signed normals with consistent magnitude (at most two byte units of length variation). Each vertex may use its own affine, approximately uniform-scale world transform, allowing the usual batched skeleton limbs. Supported sets contain ambient plus one to seven directional lights. No actor, scene, model, or mod IDs are used.

The runtime content commonly uses normals of length 120, not 127. The shader preserves the draw's mean authored magnitude instead of brightening every normal to unit strength. Larger length variation can encode deliberate shading and retains legacy behavior.

Mixed-light draws, raw RDP geometry, flat shading, unlit vertex colors, modified vertex colors, degenerate or varying-magnitude normals, incompatible transforms (nonuniform scale, shear or projective terms), and true positional microcode lights retain the complete original shade path. MM's actor-relative point lights that have already been resolved to directional RSP lights work automatically. Actual microcode positional lights remain legacy; their original attenuation is not approximated or discarded. `gSPModifyVertex` RGB overrides clear RT64's vertex light count and therefore opt out automatically.

Replacement models using ordinary lighting display lists require no new API. A mod can retain authored colors/flat shading through existing commands. A future richer lighting API can extend eligibility without making it a prerequisite for existing content.

## Renderer boundary

No additional MM semantic bridge is needed for this capability. RT64 already records normals and per-vertex light indices/counts at RSP vertex load time. A draw receives a compact `RDPParams.pixelLighting` record only when those inputs are compatible.

The vertex shader uses `SV_VertexID` to read the original signed normal and its transform index, rotates the normal into the shared RSP lighting space, and puts it in the existing smooth RGB interpolant for eligible enhanced draws. Shade alpha, UVs, positions, flat color, and the raster output linkage remain unchanged. The pixel shader normalizes the interpolated normal and evaluates directional diffuse lighting before the existing combiner. The uniform-scale gate makes this change of basis equivalent to the original normalized local light direction (within fixed-matrix quantization tolerance). Native and fallback draws pass their original shade RGB through both stages.

The light buffer and HFR-interpolated world-transform buffer are the same resources used by RSP processing. This avoids freezing lighting transforms at the original game frame. The shader ABI adds the vertex system input and extends RDP parameters from 320 to 336 bytes; all shader consumers and generated wrappers must be rebuilt together.

The seven possible directional lights are explicitly unrolled. RT64's re-spirv specialization optimizer does not support the cyclic instruction graph introduced by a dynamically bounded light loop.

## Build verification

The bundled DXC's `-MD` mode only writes dependencies; it does **not** compile an object. RT64 custom commands compile first and collect dependencies in a separate `-M -MF` invocation. A logged “Generating” step alone does not establish that a new shader binary was produced. Verify changed output hashes and reflected layout after shader-interface changes.

Runtime evidence and current limitations are recorded in `HANDOFF.md`; disposable A/B captures, playback and the DXC source-change probe are under `_working-directory/diagnostics/2026-09-06-lighting/`.

`RT64_LIGHTING_DIAGNOSTICS=1` logs eligibility and rejection counts periodically for developer runs. This distinguishes a successful legacy fallback from an enhancement that is actually active; it is not needed for ordinary play.
