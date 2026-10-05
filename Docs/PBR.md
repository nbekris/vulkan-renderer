# Physically based shading

The directional-light shader uses `EvaluateBrdf` in Shaders/brdf.slang: GGX normal distribution,
height-correlated Smith visibility, Schlick Fresnel, and energy-partitioned diffuse/specular lobes.
The metallic/roughness factors, camera position, and world-space vertex position are carried through
matching CPU/Slang layouts. Static glTF imports read scalar factors. Unlit and alpha-mask behavior
is retained. README.md summarizes usage, controls, verification, and remaining scope.

The formulation follows the standard microfacet model described in Google's
[Filament rendering documentation](https://google.github.io/filament/main/filament.html).
The implementation is written in Slang and follows this project's coding standard.

## Created files

- [Shaders/brdf.slang](E:/MastersHW/VulkanProj/vulkan-renderer/Shaders/brdf.slang)
- [Tests/Rendering/PbrTests.h](E:/MastersHW/VulkanProj/vulkan-renderer/Tests/Rendering/PbrTests.h)
- [Tests/Rendering/PbrTests.cpp](E:/MastersHW/VulkanProj/vulkan-renderer/Tests/Rendering/PbrTests.cpp)
- [Docs/PBR.md](E:/MastersHW/VulkanProj/vulkan-renderer/Docs/PBR.md)

## Changed files

- [Shaders/mesh-common.slang](E:/MastersHW/VulkanProj/vulkan-renderer/Shaders/mesh-common.slang)
- [Shaders/mesh.vert.slang](E:/MastersHW/VulkanProj/vulkan-renderer/Shaders/mesh.vert.slang)
- [Shaders/mesh.frag.slang](E:/MastersHW/VulkanProj/vulkan-renderer/Shaders/mesh.frag.slang)
- [Shaders/vert.spv](E:/MastersHW/VulkanProj/vulkan-renderer/Shaders/vert.spv)
- [Shaders/frag.spv](E:/MastersHW/VulkanProj/vulkan-renderer/Shaders/frag.spv)
- [Shaders/mesh-attributes.spv](E:/MastersHW/VulkanProj/vulkan-renderer/Shaders/mesh-attributes.spv)
- [Source/Rendering/RenderData.h](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Rendering/RenderData.h)
- [Source/Rendering/Resources/Material.h](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Rendering/Resources/Material.h)
- [Source/Rendering/Resources/Material.cpp](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Rendering/Resources/Material.cpp)
- [Source/Scene/Scene.cpp](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Scene/Scene.cpp)
- [Source/Scene/Lighting.h](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Scene/Lighting.h)
- [Source/Application/DemoScene.cpp](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Application/DemoScene.cpp)
- [Source/Assets/GltfLoader.cpp](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Assets/GltfLoader.cpp)
- [Tests/Rendering/LightingTextureTests.cpp](E:/MastersHW/VulkanProj/vulkan-renderer/Tests/Rendering/LightingTextureTests.cpp)
- [Tests/Rendering/ModernRendererTests.cpp](E:/MastersHW/VulkanProj/vulkan-renderer/Tests/Rendering/ModernRendererTests.cpp)
- [Tests/Assets/GltfTests.cpp](E:/MastersHW/VulkanProj/vulkan-renderer/Tests/Assets/GltfTests.cpp)
- [VulkanProj.vcxproj](E:/MastersHW/VulkanProj/vulkan-renderer/VulkanProj.vcxproj)
- [VulkanProj.vcxproj.filters](E:/MastersHW/VulkanProj/vulkan-renderer/VulkanProj.vcxproj.filters)
- [README.md](E:/MastersHW/VulkanProj/vulkan-renderer/README.md)
- [Docs/ARCHITECTURE.md](E:/MastersHW/VulkanProj/vulkan-renderer/Docs/ARCHITECTURE.md)

No files were deleted or renamed for this task.
