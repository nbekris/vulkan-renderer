# Vulkan 3D renderer

A Vulkan 1.3 renderer with Slang shaders, static glTF import, physically based lighting,
directional shadows, Assimp model import, and GPU validation tests.

## Build

Use Visual Studio/MSBuild with the C++ toolchain and Vulkan SDK installed. The SDK must
provide `slangc` and `spirv-val`; GLFW, GLM, VMA, cgltf, and stb_image are bundled.
Run `Tools/SetupAssimp.ps1` once to build Assimp (requires CMake and Visual Studio C++ tools).

```powershell
powershell -ExecutionPolicy Bypass -File Tools/SetupAssimp.ps1
msbuild VulkanProj.vcxproj /p:Configuration=Debug /p:Platform=x64
.\x64\Debug\VulkanProj.exe
```

Run from the repository root to resolve shaders and assets. Builds compile and validate
SPIR-V and copy the GLFW DLL beside the executable. Separate shader tools can be selected
with the `SlangCompiler` and `SpirvValidator` MSBuild properties.

## GPU requirements

Requires Vulkan 1.3 with dynamic rendering, Synchronization2, buffer device addresses,
partially bound descriptor arrays, runtime descriptor arrays, nonuniform sampled-image
indexing, uniform/storage/sampled-image array dynamic indexing, and shader demote support.
Device selection checks required features and descriptor limits and reports unsupported hardware.

## Controls

**WASD** moves, **Q/E** descends/ascends, and **Left Shift** increases speed. Hold the
**right mouse button** to look and press **Escape** to close. Input pauses when unfocused.
The default scene contains two textured cubes and a floor.

## Models and benchmarks

```powershell
.\x64\Debug\VulkanProj.exe --model Assets/Models/SamplePyramid
.\x64\Debug\VulkanProj.exe --model Assets/Samples/SampleScene.glb
.\x64\Debug\VulkanProj.exe --frames 120 --frames-in-flight 3
.\x64\Release\VulkanProj.exe --model Assets/Benchmarks/BenchmarkScene.gltf --benchmark 240 --auto-frame off --culling on
```

Use `--model` with a file or a folder containing one supported model. OBJ, FBX, and other
Assimp formats share the existing asset upload path; glTF/GLB retains its original loader.
Models are framed automatically; `--auto-frame off` keeps the default
camera. `--culling on|off` toggles frustum culling. Benchmarks report draws, culled objects,
triangles, CPU recording time, and GPU rendering time, excluding frame-pacing waits.

## Rendering

Dynamic rendering uses indexed vertex pulling, VMA allocations, and shared material/texture
descriptors. Two frame slots are the default; three are supported. Meshes, materials, textures,
and camera state persist across resize. Assets are frozen before rendering begins.

Rendering includes depth testing, sRGB mipmaps, metallic/roughness directional PBR, diffuse
ambient lighting, and a 2048x2048 directional shadow map with 3x3 PCF. Unlit and alpha-mask
materials are supported. Large scenes share one shadow map, limiting shadow detail.

## Import limitations

Import supports shared meshes, node transforms, PNG/JPEG images, vertex colors, UV transforms,
and scalar metallic/roughness factors. Material maps beyond base color, animation, and glTF
cameras/lights are not applied. Skinning, morph targets, alpha blending, compressed geometry,
and unsupported required extensions are rejected. Material and texture tables each have 32 slots,
including defaults. Environment lighting, HDR, and tone mapping are not implemented.

## Verification

Build the GPU integration suite and run with the Vulkan SDK validation layer:

```powershell
msbuild VulkanProj.vcxproj /p:Configuration=Debug /p:Platform=x64 /p:RendererSmokeTests=true
$env:VK_INSTANCE_LAYERS = 'VK_LAYER_KHRONOS_validation'
$env:VK_LAYER_SETTINGS_PATH = "$PWD\Tests\Validation"
.\x64\Debug\RendererSmokeTests.exe
$env:VK_LAYER_SETTINGS_PATH = "$PWD\Tests\GpuValidation"
.\x64\Debug\RendererSmokeTests.exe
```

Tests cover resource lifetimes, resize/minimize, frame buffering, GPU pixels, textures,
lighting, PBR, import, and culling. They require a Vulkan-capable GPU. Validation profiles
are opt-in; normal application runs do not enable them.

## Project and documentation

`Source` groups application, platform, math, scene, assets, rendering, and diagnostics code.
`Tests` contains unit, rendering, integration, support, and asset fixtures. `Shaders` contains
Slang sources; `Assets` contains sample and benchmark scenes. Visual Studio filters mirror these folders.

- [Model import and asset folders](Docs/MODEL_IMPORT.md)
- [Coding standard](Docs/CODING_STANDARD.md)
- [Architecture and application flow](Docs/ARCHITECTURE.md)
- [Project organization](Docs/PROJECT_ORGANIZATION.md)
- [Slang shader setup](Docs/SLANG_MIGRATION.md)
- [PBR implementation](Docs/PBR.md)
- [Directional shadows](Docs/SHADOWS.md)

VMA 3.3.0 is vendored with its MIT license under `Source/ThirdParty/VMA`;
`Vma.cpp` supplies its implementation and `VmaConfig.h` selects Vulkan 1.3.
