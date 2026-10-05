# Vulkan 3D renderer

Build `VulkanProj.vcxproj` with Visual Studio/MSBuild (x64). The Vulkan SDK's
`slangc` compiles the Slang shaders and `spirv-val` validates the resulting SPIR-V. Run from the repository root so `Shaders/*.spv`
can be found. `x64/Debug/VulkanProj.exe --frames 120` runs a finite smoke test.

Requires a Vulkan 1.3 loader/device with `dynamicRendering`, `synchronization2`,
`bufferDeviceAddress`, `runtimeDescriptorArray`, `descriptorBindingPartiallyBound`,
`shaderSampledImageArrayNonUniformIndexing`, and dynamic indexing of uniform/storage/sampled-image arrays. `DeviceRequirements`
checks features and descriptor limits before selecting a device. Shaders are
compiled for Vulkan 1.3. Unsupported GPUs exit with a feature-requirement message.

## Frame buffering, memory, and descriptors

Double buffering is the default. Use `--frames-in-flight 3` for triple buffering;
this controls CPU/GPU frame slots independently of swapchain image count.
Each slot owns a command pool, command buffer, mapped VMA uniform buffer,
acquisition semaphore, and fence. Only that slot's fence is waited before its
pool is reset or uniform data overwritten. Present semaphores are indexed by
swapchain image, because a frame fence does not establish presentation completion.
Normal frame recording does not call `vkDeviceWaitIdle`; session teardown and
resize wait before releasing dependencies.

VMA suballocates vertex, index, uniform, material, and staging buffers from its
default memory blocks, with a preferred large-heap block size of 64 MiB. Mesh
buffers prefer device-local memory; a mapped staging buffer uploads their data.
Upload completion is fenced before staging resources are released. Mapped writes
are flushed for noncoherent memory. Upload barriers and frame submission use
Synchronization2 (`vkCmdPipelineBarrier2` / `vkQueueSubmit2`). Swapchain images
explicitly transition from discarded contents to color attachment and then present.

Rendering uses `vkCmdBeginRendering` / `vkCmdEndRendering`, with attachment
formats declared through `VkPipelineRenderingCreateInfo`. No render passes or
framebuffers are created. Indexed meshes use vertex pulling: each vertex
buffer has `VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT`, VMA enables device-address
allocations, and the shader reads a buffer reference supplied through push constants.

One descriptor set per presentation session contains three uniform buffer slots,
32 material storage-buffer slots, and 32 combined image/sampler slots. All
bindings allow partially bound arrays. Materials contain a linear tint and a texture
index; the fragment shader samples the texture through `nonuniformEXT` indexing.
The table is populated once per session and bound once per scene, with no descriptor
allocation or updates per draw. An 80-byte push-constant block supplies the vertex
address, frame index, material index, and model matrix. Unpopulated material and
texture references are rejected before GPU access. Default materials zero and one
use a white texture with white and green tints; the demo adds two checker materials.

`SceneResources` owns meshes, immutable material buffers, and sampled textures across
all presentation sessions. Call `Initialize`, add assets, then `Freeze` before creating
descriptors. Frozen tables reject asset additions, preventing updates to descriptors
or referenced resources while frames are in flight. Lighting and object transforms
remain editable through per-frame uniforms and per-draw push constants.

## Resource ownership

`Application::Run` composes noncopyable RAII owners in dependency order. Each
component initializes through `Initialize` and releases its own resources in
its destructor, including after partial initialization. Constructors bind
dependencies without performing Vulkan setup. Consumers hold borrowed references
to their dependencies, which must be initialized first and outlive them.
`FrameRenderer` waits for the device before freeing commands or allowing other
components to unwind. `VulkanContext` also waits before destroying the device.
Queues, physical devices, swapchain images, and command buffers are borrowed
handles: their device, swapchain, or command pool owns their lifetime.

`ShaderModule` releases temporary shader modules on every pipeline creation
exit path. `AllocatedBuffer` owns a buffer and its VMA allocation together.
Declaration order ensures buffers are destroyed before their allocator, and
the allocator is destroyed before the logical device.

Swapchain images remain owned by Vulkan and must not be allocated or freed through VMA.

## Components

All project classes live in the `VulkanRenderer` namespace.

| Component | Responsibility |
| --- | --- |
| `Application` | Composition and the window event loop |
| `Window` | GLFW session and native window |
| `DeviceSelection` | Device capability queries and suitability |
| `DeviceRequirements` | Vulkan 1.3 feature and descriptor-limit contract |
| `VulkanContext` | Instance, surface, device, and borrowed queues |
| `MemoryAllocator`, `AllocatedBuffer` | VMA allocator and buffer ownership |
| `SwapChain` | Presentation images and image views |
| `ResourceUploader` | Fenced staging copies into device-local buffers and image mip chains |
| `FrameResources`, `FrameRing` | Independent CPU/GPU frame slots |
| `GlobalDescriptors` | Shared per-frame uniform, material, and texture descriptor table |
| `SceneResources` | Persistent mesh, material, and texture ownership |
| `Material`, `Texture` | Immutable material data and sampled image/sampler ownership |
| `TexturePixels` | Linear-space RGBA8 mip generation |
| `Lighting` | Directional light direction/color/intensity and ambient intensity |
| `ShaderModule` | SPIR-V loading and shader module ownership |
| `GraphicsPipeline` | Graphics pipeline creation and binding |
| `Mesh`, `MeshData` | Arbitrary device-address vertex data and 32-bit indexed drawing |
| `MeshPrimitives` | CPU triangle/cube/plane geometry with normals and UVs |
| `SceneObject`, `Scene` | Shared mesh references, independent transforms/materials, and camera |
| `MeshRenderer` | Shared pipeline and descriptor binding for all scene meshes |
| `FrameRenderer` | Command recording, submission, and synchronization |
| `PresentationSession` | One generation of swapchain-dependent resources |

`FrameRenderer` depends on the single-method `IDrawCommands` interface instead
of a specific scene. Implement `Record` to add a different scene without changing
frame scheduling; it receives the frame index and runs inside dynamic rendering.
`GraphicsPipeline` accepts shader paths, attachment format, and descriptor layout.
This keeps responsibilities separate, favors composition, and provides a narrow
substitution point without adding interfaces to every resource owner.

The window supports resizing. Framebuffer callbacks, out-of-date acquisition,
and suboptimal/out-of-date presentation retire the current `PresentationSession`.
Its RAII owners wait for the GPU and release the old resources; the next session
recreates the swapchain, viewport/scissor pipeline, descriptor table,
frame slots, and synchronization at the current framebuffer size. The window,
Vulkan device, VMA allocator, scene, meshes, material buffers, textures, and samplers persist across sessions. Minimized and
zero-sized windows wait for events instead of drawing; closing exits that wait.

Code follows the root `CODING_STANDARD.md`: PascalCase methods, camelCase
variables, underscored private fields, explicit noncopyability, virtual
destructors, namespaces, tabs, braces, and CRLF. `.clang-format` encodes the
existing formatting conventions. Project-owned source files remain below
300 lines; the upstream VMA header is preserved unchanged.

The project links the bundled GLFW DLL import library and copies `glfw3.dll`
beside the executable during builds. This avoids mixing the release static
GLFW library's C runtime with the Debug project's C runtime.

## Verification

Run from the repository root with the Vulkan SDK validation layer available:

```powershell
$env:VK_INSTANCE_LAYERS = 'VK_LAYER_KHRONOS_validation'
.\x64\Debug\VulkanProj.exe --frames 120
.\x64\Debug\VulkanProj.exe --frames 120 --frames-in-flight 3
```

Build with `/p:RendererSmokeTests=true` to select `Tests/Integration/RendererSmokeTests.cpp`
as the entry point and write `x64/Debug/RendererSmokeTests.exe`. This integration
test requires a Vulkan-capable GPU. It checks repeated application lifetimes,
VMA allocation release and upload bounds, shader loading failure after a module
has been created, and alternate drawing commands that throw with frames in flight.
It also checks repeated window resizing, framebuffer/swapchain extent agreement,
minimize/restore, and closing while minimized.
`ModernRendererTests` verifies 128 simultaneous buffer suballocations share VMA
blocks and checks actual offscreen GPU pixels for buffer-address vertex pulling,
material index selection, and distinct uniform data in all three frame slots.

Enable synchronization validation and then GPU-assisted validation with the
separate bundled profiles:

```powershell
$env:VK_LAYER_SETTINGS_PATH = "$PWD\Tests\Validation"
.\x64\Debug\RendererSmokeTests.exe
$env:VK_LAYER_SETTINGS_PATH = "$PWD\Tests\GpuValidation"
.\x64\Debug\RendererSmokeTests.exe
```

Validation settings are opt-in and are not enabled for normal application runs.
GPU-assisted validation may report that it enables additional device features
for shader instrumentation; these are layer setup diagnostics, not application errors.

## VMA dependency

Vulkan Memory Allocator **3.3.0** is vendored under `Source/ThirdParty/VMA`,
including its MIT license, from:
https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator/tree/v3.3.0

`Source/ThirdParty/Vma.cpp` is the only translation unit defining `VMA_IMPLEMENTATION`.
`VmaConfig.h` sets Vulkan 1.3 support to match the application's API version.

## Camera and object transforms

The demo renders two independently transformed cubes with separate materials through a perspective camera.
Use **WASD** to move, **Q/E** to descend/ascend, and **Left Shift** to move faster.
Hold the **right mouse button** to look; release it to release the cursor.
Press **Escape** to close the application through the normal GPU-safe shutdown path.
Camera input pauses when the window loses focus. Movement uses elapsed time and normalized input.

`Camera` owns the view and Vulkan zero-to-one perspective projection; `Transform` composes
translation, Euler rotation (radians, Z/Y/X order), and scale. `Scene` owns persistent CPU state.
Each frame slot receives camera data after its fence completes, and each draw supplies an
80-byte push constant containing the vertex address, descriptor indices, and model matrix.
Resizing updates the projection aspect ratio without resetting the camera or object transforms.
Meshes are double-sided and depth-tested, with ambient/directional lighting and textured materials.

## Depth buffering

Every presentation frame slot owns a VMA-backed depth image and view. The engine queries
format support, preferring D32 floating-point depth and falling back to supported alternatives.
Dynamic rendering clears depth to 1.0, and pipelines use depth writes with the LESS comparison.
Synchronization2 barriers cover early and late fragment tests. Combined depth/stencil formats
transition both aspects without requiring separate depth/stencil layout features.

Depth images match the swapchain extent and are recreated on resize after GPU work completes.
GPU readback tests verify draw-order-independent occlusion, clearing on reuse, near-plane clipping,
distinct depth images for all three slots, and a supported combined depth/stencil format.

## Reusable mesh rendering

`Mesh::Initialize` accepts arbitrary vertex/index spans and uploads validated triangle-list geometry
through VMA staging buffers. Indices are 32-bit. Empty data, incomplete triangles, out-of-range indices,
and repeated initialization are rejected before new GPU work is recorded.

The 64-byte, 16-byte-aligned `Vertex` contains position, color, normal, and UV fields; static layout
checks match the shader's std430 buffer-reference layout. The vertex shader transforms normals
using the inverse transpose of the model matrix, including nonuniform scale, and passes UVs through
to the fragment interface. The color shader multiplies sampled albedo, vertex color, and material tint,
then applies a Cook-Torrance BRDF for directional illumination plus a diffuse ambient approximation. Singular object scales are rejected.

`Scene::AddObject` accepts a shared initialized mesh and a material index. Each `SceneObject` owns
its independent `Transform`, and `MeshRenderer` draws visible objects using their mesh addresses, index
count, material index, and model matrix. Objects can share geometry or reference different meshes.
Mesh owners must outlive outstanding GPU work and their VMA allocator. The application retires GPU
work before scene/mesh destruction, and resizing preserves mesh buffers and CPU scene state.

The demo cube has 24 vertices and 36 indices, with distinct face normals, colors, and UVs. GPU tests
cover different meshes and materials in one scene, indices beyond 65,535, and normal/UV correctness
under nonuniform scale. Shader filenames are `Shaders/mesh.vert.slang` and `Shaders/mesh.frag.slang`.

## Persistent assets, lighting, and textures

`SceneResources::CreateMesh` returns shared immutable geometry. `AddTexture` accepts an extent and
RGBA8 pixel span, generates every mip down to 1x1, stages the complete chain, and creates a repeat
sampler with linear/trilinear filtering. Use `TextureColorSpace::Srgb` for albedo color or `Linear`
for data textures. sRGB RGB mip filtering happens in linear space; alpha is averaged linearly.
The pixel API accepts decoded pixels; `GltfLoader` decodes PNG/JPEG files and embedded images.

`AddMaterial` accepts `MaterialData` with a linear RGBA tint and an existing texture index. The
current table supports 32 materials and 32 textures, including the defaults. Uploads happen
before `Freeze`; each new presentation descriptor set references the same persistent allocations.
Resource teardown waits for GPU completion before destroying material buffers, sampled images,
samplers, and owned meshes. Resizing rebuilds presentation resources without uploading assets.

`Scene::GetLighting` configures a direction toward the light, linear RGB color, directional intensity,
and ambient intensity. The per-frame uniform contains view/projection plus lighting data and is
visible in both vertex and fragment stages. The fragment shader uses normalized world normals
and `max(dot(normal, lightDirection), 0)` for diffuse illumination, then adds ambient illumination.
The demo uses a mipmapped checker texture on two differently tinted cubes.

`LightingTextureTests` reads GPU pixels to verify texture-index selection, sRGB decoding versus
linear data, minified gamma-correct mip sampling, directional response, ambient response, and
independent light color/intensity in all three frame slots. Resize tests assert that mesh addresses,
material buffer handles, texture image handles, and camera state survive session recreation.
Existing allocation checks verify that teardown leaves no live VMA allocations.

## glTF import, frustum culling, and profiling

Run from the repository root so the existing shader paths resolve:

```powershell
.\x64\Release\VulkanProj.exe --model Assets/Samples/SampleScene.glb
.\x64\Release\VulkanProj.exe --model Assets/Benchmarks/BenchmarkScene.gltf --benchmark 240 --auto-frame off --culling on
```

`--model` loads a static glTF 2.0 or GLB scene before freezing persistent resources. Imported scenes
are framed automatically; `--auto-frame off` keeps the default camera. `--culling on|off` controls
conservative world AABB/frustum tests using the exact camera matrix uploaded for that frame.
Culling handles rotation, mirrored scale, nonuniform scale, shear, resize, and Vulkan zero-to-one depth.
Meshes cache local bounds at upload, and repeated nodes share the same uploaded primitive.

`GltfDocument` owns cgltf parsing and buffer data; `GltfGeometry` handles accessors and topology;
`GltfImages` resolves image sources and sampler settings; `ImageDecoder` owns stb_image decoding.
`GltfLoader::Read` produces CPU model data. `Import` checks descriptor capacity, stages persistent assets,
and rolls back resources and objects if any step fails. Node hierarchy transforms are preserved as
matrices and composed with each object's editable local transform. Materials preserve base-color
factor, base-color texture, OPAQUE/MASK alpha, and KHR_materials_unlit. Vertex RGBA colors participate
in alpha masking. KHR_texture_transform and its UV-set override are baked into imported UVs.
External images, embedded GLB images, and base64 images are supported, with glTF sampler filtering,
wrap modes, sRGB mipmaps, indexed/unindexed triangles, triangle strips/fans, interleaved attributes,
normalized integers, sparse vertex attributes, and generated flat normals when normals are absent.
The importer uses pinned, licensed cgltf 1.15 and stb_image copies under `Source/ThirdParty`.

Static geometry uses metallic/roughness factors with direct-light PBR. It is not a complete glTF renderer.
Metallic/roughness texture maps, normal, occlusion, and emissive maps are not applied; animation playback and glTF
cameras/lights are not imported. Skinning, morph targets, alpha blending, sparse index accessors,
Draco/meshopt compression, unsupported required extensions, and EXT_mesh_gpu_instancing produce
explicit errors. KTX2/WebP texture extensions require a PNG/JPEG fallback. Empty selected scenes,
invalid data, singular node transforms, and descriptor capacity exhaustion are rejected.
Paths must be readable by cgltf's native file API; Unicode Windows filenames may require a future
custom file callback. All geometry currently uses the existing double-sided pipeline.

The fragment shader's alpha discard requires `shaderDemoteToHelperInvocation`; device selection
checks and enables this Vulkan 1.3 feature along with existing dynamic-rendering/synchronization features.
The 32-slot texture and material tables include one default texture and two default materials.

`--benchmark count` bounds the run and prints average draw, cull, triangle, CPU recording, and GPU
rendering counters. CPU recording covers scene validation, matrix/culling work, and mesh commands.
GPU timing uses two timestamp queries per frame slot around dynamic rendering, reads completed
slots after their existing fences, and handles timestamp wraparound. Upload, acquisition, presentation,
and frame-pacing waits are excluded from these durations. Unsupported timestamps report unavailable;
final in-flight queries are omitted from the sample count. Benchmarking does not disable presentation
or report these durations as end-to-end frame times.

A 240-frame Release comparison on this machine at 800x600, fixed default camera, validation disabled,
using the authored 2,048-object `BenchmarkScene.gltf` produced:

| Culling | Draws | Culled | Triangles | CPU recording | GPU rendering (238 samples) |
| --- | ---: | ---: | ---: | ---: | ---: |
| off | 2,048 | 0 | 24,576 | 0.4092 ms | 0.0368 ms |
| on | 146 | 1,902 | 1,752 | 0.2550 ms | 0.0225 ms |

This small shared-cube scene measures submission/culling behavior; it is not a production content
benchmark. Compare representative imported assets before choosing further batching or GPU culling.

`GltfTests` verifies external/GLB/base64 images, shared meshes, hierarchy transforms, interleaved and
sparse attributes, normalized colors, UV transforms, unlit and masked material pixels, atomic rollback,
and identical visible pixels with culling on/off. `FrustumTests` covers all six planes, near depth,
plane contact, transformed bounds, camera movement, and aspect changes. Smoke tests also run a
triple-buffered imported GLB with timestamps, in addition to the existing resize and lifetime tests.

## Project folders and Visual Studio filters

Source/header pairs are grouped by responsibility under `Source/Application`, `Platform`, `Math`,
`Scene`, `Assets`, `Rendering`, and `Diagnostics`. Vulkan rendering is divided into Core, Memory,
Frames, Pipeline, Presentation, Descriptors, and Resources. `Source/ThirdParty` contains dependency
headers and their implementation wrappers. Tests are grouped into Unit, Rendering, Integration,
Support, and Assets. Samples live in `Assets/Samples`; benchmark scenes in `Assets/Benchmarks`.

Visual Studio filters mirror the physical directory tree, grouping each component's header and source
together. `Source` and `Tests` are explicit include roots. Run executables from the repository root
as before. `Docs/PROJECT_ORGANIZATION.md` lists the exact file moves and configuration edits.

## Slang shaders

The renderer and attribute-probe shaders are authored in Slang. `Shaders/mesh-common.slang`
shares vertex, frame-uniform, material, push-constant, and stage-interface types. The Vulkan SDK's
`slangc` compiles each `Main` source entry to SPIR-V 1.6 for Vulkan 1.3; `spirv-val --target-env vulkan1.3`
checks every output during the Visual Studio build. Shader warnings are treated as errors.
Compilation failure stops the custom build before validation, so stale binaries cannot mask a failure.
Changing the common include rebuilds dependent shader outputs through `AdditionalInputs`.

This conversion was tested with the installed Slang 2026.8 compiler. The SDK must provide `slangc.exe`
and `spirv-val.exe` under its Bin folder. Override the MSBuild `SlangCompiler` or `SpirvValidator`
properties if using separate tool installations. The outputs remain `Shaders/vert.spv`, `frag.spv`,
and `mesh-attributes.spv`; pipeline SPIR-V entry names remain `main` (Slang's default output naming).

`-matrix-layout-column-major` matches GLM data. Explicit descriptor bindings preserve set 0 binding 0
(frame buffers), 1 (material storage buffers), and 2 (combined samplers). Typed vertex pointers
preserve buffer-device-address vertex pulling and the 64-byte vertex stride. Push constants retain
pointer/frame/material/model offsets 0/8/12/16 and total size 80 bytes. Material buffers retain a
48-byte layout. The vertex stage uses `SV_VulkanVertexID` to preserve indexed Vulkan vertex IDs.
Normal transformation uses cofactor rows divided by the determinant, equivalent to inverse-transpose.

Slang 2026.8 declares a broad capability set for `NonUniformResourceIndex`; the fragment build
explicitly supplies that set through `SlangBindlessCapabilities` instead of disabling compiler warnings.
These options permit compiler operations; actual emitted SPIR-V capabilities determine Vulkan feature
requirements. Inspection and validation confirmed that the conversion requires no additional enabled
device features. The existing alpha-mask demote feature remains enabled.

The existing GPU tests cover transforms, normals/UVs, material indexing, textures/mips, lighting,
alpha masking, unlit materials, and imported scenes using the Slang-generated binaries.

Slang shaders follow the applicable rules in `CODING_STANDARD.md`: tab indentation, UTF-8/CRLF,
120-column formatting, PascalCase functions/types, uppercase constants, camelCase mutable values,
and namespaced shared types/resources. Source entry functions are `Main`; Slang emits `main` for
the Vulkan pipeline. `.editorconfig` applies the text-formatting rules to `.slang` files in Visual Studio.

## Physically based directional lighting

`Shaders/brdf.slang` implements `EvaluateBrdf`: Cook-Torrance specular using GGX/Trowbridge-Reitz
normal distribution, height-correlated Smith visibility, and Schlick Fresnel, combined with a
Fresnel-weighted Lambert diffuse term. Dielectrics use F0=0.04; metals use base color as F0 and have
no diffuse lobe. Perceptual roughness is squared to obtain microfacet alpha. Roughness is floored
at 0.045 during shading to keep zero-roughness materials numerically stable.

`MaterialData::metallic` and `roughness` accept finite values from zero to one; engine defaults are
metallic=0 and roughness=0.5. glTF imports preserve the material's scalar factors. The CPU/shader
material layout is 48 bytes: base color at 0, texture index at 16, alpha cutoff at 20, unlit at 24,
metallic at 28, roughness at 32, and padding at 36/40/44. `FrameUniform` is now 112 bytes with camera
position at offset 96. World position is interpolated from the vertex shader so specular highlights
respond to camera movement. Descriptor bindings and the 80-byte draw push constants are unchanged.

The fragment stage evaluates BRDF * directional radiance * max(N dot L, 0). Lighting and base colors
are linear; sRGB textures decode on sampling and the sRGB swapchain handles output encoding.
Unlit materials bypass BRDF evaluation and alpha masks remain supported. The demo compares a
smooth dielectric checker cube with a blue metallic cube. A 10 by 10 unit gray floor at Y = -1
uses a nonmetallic material with roughness 0.8 beneath the cubes. The floor uses the existing
depth testing and PBR lighting, including directional PCF shadows. Collision is not implemented. Imported models
retain their own scene geometry.

Ambient light is currently an adjustable diffuse approximation and contributes no metallic diffuse.
Image-based environment/specular lighting, metallic/roughness maps, normal maps, HDR rendering,
tone mapping, and shadows are not included in this change. Bright specular peaks clamp in the
existing swapchain output. This is a single-scattering BRDF, without multiscattering compensation.

`PbrTests` checks closed-form normal-incidence dielectric and metal values, tinted metallic specular,
dielectric neutral specular, roughness changes, view-dependent highlights, back-facing directions,
zero-roughness stability, all three frame slots, unlit preservation, and invalid material values.
`LightingTextureTests` retains texture/mip/color-space tests and checks the new BRDF light response.
See `Docs/PBR.md` for implementation references and the complete file list.

### Directional shadows

The directional light now casts shadows using a scene-fitted orthographic depth pass and a 3x3 PCF kernel.
Each frame owns a 2048x2048 VMA shadow image; dynamic rendering and Synchronization2 handle writing and sampling.
Alpha cutout materials preserve their silhouettes in the shadow pass, including off-camera shadow casters.
Only direct PBR illumination receives shadows; ambient illumination and unlit materials retain their behavior.
GPU timing includes both the shadow and main passes. Large scenes share one shadow map, so shadow detail
falls as scene bounds grow. Cascades, variable penumbra softness, and point-light shadows are future extensions.
See [shadow implementation and file inventory](Docs/SHADOWS.md) for settings, validation, and affected files.
