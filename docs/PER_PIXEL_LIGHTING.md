# Enhanced per-pixel lighting

The enhanced renderer evaluates the existing RSP ambient and directional lights on interpolated, normalized model normals. This smooths diffuse shading across character triangles without inventing lights, changing textures, adding specular highlights, or changing the N64 combiner/blender.

Zelda enables it at startup. Set `ZELDA64RECOMP_LIGHTING=original` for legacy vertex lighting, or use the session-local checkbox in **F1 → Game editor → Lighting** when debug mode is enabled. The lighting selection is independent of fog mode. Native/RDRAM always uses original vertex lighting.

## Eligibility and compatibility

RT64 checks the actual vertices referenced by each indexed, smooth-shaded draw. All must have equivalent RSP lighting values. Different buffer indices are accepted when the relevant light colors, directions and coefficients agree; ambient requires only color equality. Supported sets contain ambient plus one to seven directional lights. No actor, scene, model, texture or mod IDs are used.

Normal direction and authored magnitude now interpolate separately. The pixel shader normalizes direction and restores interpolated magnitude before lighting. This preserves normals of length 120, shorter deliberately dim normals, and zero normals that contribute ambient only. It replaces the first implementation's draw-wide mean and two-byte variation gate. That gate rejected large Town walls/ground sections because a single short or zero normal excluded the entire draw. Supporting the existing per-vertex strength data removes that coarse fallback without splitting batches or flattening authored shading.

When all vertices share a world matrix, lighting is evaluated in local space with RT64's original `computeDirLight` equation. Nonuniform scale and shear are supported here. Draws spanning different matrices use the shared rotated-normal basis and still require affine, approximately uniform-scale transforms. Nonfinite normal-transform coefficients fall back.

Genuinely different light sets, raw RDP geometry, flat shading, unlit/modified vertex colors, incompatible mixed transforms, and true positional microcode lights retain original shading. `gSPModifyVertex` RGB overrides clear the vertex light count. MM actor-relative point lights already resolved into directional RSP lights work automatically. True positional lights retain legacy attenuation: faithful pixel support additionally needs world position and transform scale, not a directional approximation.

Replacement models using ordinary lighting display lists require no new API. A mod can retain authored colors/flat shading through existing commands. A future richer lighting API can extend eligibility without making it a prerequisite for existing content.

## Renderer boundary

No additional MM semantic bridge is needed for this capability. RT64 already records normals and per-vertex light indices/counts at RSP vertex load time. A draw receives a compact `RDPParams.pixelLighting` record only when those inputs are compatible.

The vertex shader reads signed normals with `SV_VertexID`. Smooth RGB carries local or rotated normal direction on eligible enhanced draws; a scalar `TEXCOORD1` carries authored magnitude. Shade alpha, UVs, positions and flat color retain their roles. All dynamic/specialized SPIR-V and linked DXIL wrappers declare the same new scalar. Native and fallback draws pass original shade RGB through both stages. The scalar linkage was checked on the current Vulkan runtime; D3D12 runtime remains unqualified.

The light buffer and HFR-interpolated world-transform buffer are shared with RSP processing. RDP parameters remain 336 bytes: `pixelLighting.xy` select the light set, `z` is the shared matrix index plus one (zero selects the rotated basis), and `w` is a developer tint category. Rebuild every shader consumer and generated wrapper after interface changes.

Light-state eligibility remains draw-level. The representative Town pass did not show mixed-state fallback dominating visible scenery; raster range splitting would add metadata and special handling for rewritten depth-test indices without addressing the observed cause. Preserve that option for evidence of a real mixed-state bottleneck.

The seven possible directional lights are explicitly unrolled. RT64's re-spirv specialization optimizer does not support the cyclic instruction graph introduced by a dynamically bounded light loop.

## Build verification

The bundled DXC's `-MD` mode only writes dependencies; it does **not** compile an object. RT64 custom commands compile first and collect dependencies in a separate `-M -MF` invocation. A logged “Generating” step alone does not establish that a new shader binary was produced. Verify changed output hashes and reflected layout after shader-interface changes.

Runtime evidence and current limitations are recorded in `HANDOFF.md`; disposable A/B captures, playback and the DXC source-change probe are under `_working-directory/diagnostics/2026-09-06-lighting/`.

`RT64_LIGHTING_DIAGNOSTICS=1` logs set/mixed/transform/positional rejection counts. `RT64_LIGHTING_COVERAGE=1` shows actual visible surfaces: green enhanced, blue absent/unsupported light set, magenta mixed vertex state, red incompatible transforms, cyan positional lights, gray outside the classifier. Both are developer-only launch diagnostics. Temporary aggressive/conservative modes were removed after the normal-strength experiment.

The 2026-09-09 continuation evidence is under `_working-directory/diagnostics/2026-09-07-coverage/` (the directory predates the restart). Earlier yellow captures identify the removed normal-length gate. The Clock Tower interior remained blue while characters were green; Town wall/ground sections changed from yellow to green. This is a focused real-mod-stack check, not a claim about every scene or mod.
