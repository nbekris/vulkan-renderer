# Project organization

Source and header pairs live together under folders named for their responsibility. Visual Studio
filters mirror those physical folders. The project explicitly includes Source and Tests as header
search roots, so includes such as "Rendering/Memory/AllocatedBuffer.h" are independent of caller depth.
Runtime shaders and assets still resolve from the repository root. Third-party headers retain their
upstream layout. Shader outputs remain in Shaders for the existing pipeline and regression probes.

| Folder | Responsibility |
| --- | --- |
| Source/Application | Entry point, composition root, demo setup |
| Source/Platform | Window and platform events |
| Source/Math | Bounds, frustum, transforms, GLM configuration |
| Source/Scene | Scene objects, camera, input controller, lighting |
| Source/Assets | glTF import, image decoding, CPU geometry and mip data |
| Source/Rendering/Core | Vulkan context and device requirements |
| Source/Rendering/Memory | VMA allocation wrappers and uploads |
| Source/Rendering/Frames | Frame slots, scheduling, depth targets |
| Source/Rendering/Pipeline | Graphics pipeline and shader modules |
| Source/Rendering/Presentation | Swapchain and presentation session |
| Source/Rendering/Descriptors | Shared descriptor table |
| Source/Rendering/Resources | Persistent GPU meshes, materials, textures |
| Source/Rendering | Draw interface, scene recorder, GPU data layouts |
| Source/Diagnostics | CPU benchmark aggregation and GPU timestamps |
| Source/ThirdParty | Vendored libraries and implementation wrappers |
| Tests/Unit | Camera/transform and frustum checks |
| Tests/Rendering | GPU rendering regression tests |
| Tests/Integration | Application, resize, and ownership smoke tests |
| Tests/Support | Shared offscreen rendering probe |
| Tests/Assets | Import tests and glTF regression fixtures |
| Assets/Samples | Sample model, binary data, and texture |
| Assets/Benchmarks | Representative scene referencing sample resources |
| Docs | Implementation inventory and project organization |

## Files moved (code includes and runtime paths updated where applicable)

| Previous path | Current path |
| --- | --- |
| Assets/BenchmarkScene.gltf | [Assets/Benchmarks/BenchmarkScene.gltf](E:/MastersHW/VulkanProj/vulkan-renderer/Assets/Benchmarks/BenchmarkScene.gltf) |
| Assets/SampleScene.bin | [Assets/Samples/SampleScene.bin](E:/MastersHW/VulkanProj/vulkan-renderer/Assets/Samples/SampleScene.bin) |
| Assets/SampleScene.glb | [Assets/Samples/SampleScene.glb](E:/MastersHW/VulkanProj/vulkan-renderer/Assets/Samples/SampleScene.glb) |
| Assets/SampleScene.gltf | [Assets/Samples/SampleScene.gltf](E:/MastersHW/VulkanProj/vulkan-renderer/Assets/Samples/SampleScene.gltf) |
| Assets/checker.png | [Assets/Samples/checker.png](E:/MastersHW/VulkanProj/vulkan-renderer/Assets/Samples/checker.png) |
| IMPLEMENTATION_6.md | [Docs/IMPLEMENTATION_6.md](E:/MastersHW/VulkanProj/vulkan-renderer/Docs/IMPLEMENTATION_6.md) |
| Source/AllocatedBuffer.cpp | [Source/Rendering/Memory/AllocatedBuffer.cpp](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Rendering/Memory/AllocatedBuffer.cpp) |
| Source/AllocatedBuffer.h | [Source/Rendering/Memory/AllocatedBuffer.h](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Rendering/Memory/AllocatedBuffer.h) |
| Source/AllocatedImage.cpp | [Source/Rendering/Memory/AllocatedImage.cpp](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Rendering/Memory/AllocatedImage.cpp) |
| Source/AllocatedImage.h | [Source/Rendering/Memory/AllocatedImage.h](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Rendering/Memory/AllocatedImage.h) |
| Source/Application.cpp | [Source/Application/Application.cpp](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Application/Application.cpp) |
| Source/Application.h | [Source/Application/Application.h](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Application/Application.h) |
| Source/Benchmark.cpp | [Source/Diagnostics/Benchmark.cpp](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Diagnostics/Benchmark.cpp) |
| Source/Benchmark.h | [Source/Diagnostics/Benchmark.h](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Diagnostics/Benchmark.h) |
| Source/Bounds.h | [Source/Math/Bounds.h](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Math/Bounds.h) |
| Source/Camera.cpp | [Source/Scene/Camera.cpp](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Scene/Camera.cpp) |
| Source/Camera.h | [Source/Scene/Camera.h](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Scene/Camera.h) |
| Source/CameraController.cpp | [Source/Scene/CameraController.cpp](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Scene/CameraController.cpp) |
| Source/CameraController.h | [Source/Scene/CameraController.h](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Scene/CameraController.h) |
| Source/Cgltf.cpp | [Source/ThirdParty/Cgltf.cpp](E:/MastersHW/VulkanProj/vulkan-renderer/Source/ThirdParty/Cgltf.cpp) |
| Source/DemoScene.cpp | [Source/Application/DemoScene.cpp](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Application/DemoScene.cpp) |
| Source/DemoScene.h | [Source/Application/DemoScene.h](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Application/DemoScene.h) |
| Source/DepthTarget.cpp | [Source/Rendering/Frames/DepthTarget.cpp](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Rendering/Frames/DepthTarget.cpp) |
| Source/DepthTarget.h | [Source/Rendering/Frames/DepthTarget.h](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Rendering/Frames/DepthTarget.h) |
| Source/DeviceRequirements.cpp | [Source/Rendering/Core/DeviceRequirements.cpp](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Rendering/Core/DeviceRequirements.cpp) |
| Source/DeviceRequirements.h | [Source/Rendering/Core/DeviceRequirements.h](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Rendering/Core/DeviceRequirements.h) |
| Source/DeviceSelection.cpp | [Source/Rendering/Core/DeviceSelection.cpp](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Rendering/Core/DeviceSelection.cpp) |
| Source/DeviceSelection.h | [Source/Rendering/Core/DeviceSelection.h](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Rendering/Core/DeviceSelection.h) |
| Source/FrameRenderer.cpp | [Source/Rendering/Frames/FrameRenderer.cpp](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Rendering/Frames/FrameRenderer.cpp) |
| Source/FrameRenderer.h | [Source/Rendering/Frames/FrameRenderer.h](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Rendering/Frames/FrameRenderer.h) |
| Source/FrameResources.cpp | [Source/Rendering/Frames/FrameResources.cpp](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Rendering/Frames/FrameResources.cpp) |
| Source/FrameResources.h | [Source/Rendering/Frames/FrameResources.h](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Rendering/Frames/FrameResources.h) |
| Source/FrameRing.cpp | [Source/Rendering/Frames/FrameRing.cpp](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Rendering/Frames/FrameRing.cpp) |
| Source/FrameRing.h | [Source/Rendering/Frames/FrameRing.h](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Rendering/Frames/FrameRing.h) |
| Source/Frustum.cpp | [Source/Math/Frustum.cpp](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Math/Frustum.cpp) |
| Source/Frustum.h | [Source/Math/Frustum.h](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Math/Frustum.h) |
| Source/GlobalDescriptors.cpp | [Source/Rendering/Descriptors/GlobalDescriptors.cpp](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Rendering/Descriptors/GlobalDescriptors.cpp) |
| Source/GlobalDescriptors.h | [Source/Rendering/Descriptors/GlobalDescriptors.h](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Rendering/Descriptors/GlobalDescriptors.h) |
| Source/GltfDocument.cpp | [Source/Assets/GltfDocument.cpp](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Assets/GltfDocument.cpp) |
| Source/GltfDocument.h | [Source/Assets/GltfDocument.h](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Assets/GltfDocument.h) |
| Source/GltfGeometry.cpp | [Source/Assets/GltfGeometry.cpp](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Assets/GltfGeometry.cpp) |
| Source/GltfGeometry.h | [Source/Assets/GltfGeometry.h](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Assets/GltfGeometry.h) |
| Source/GltfImages.cpp | [Source/Assets/GltfImages.cpp](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Assets/GltfImages.cpp) |
| Source/GltfImages.h | [Source/Assets/GltfImages.h](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Assets/GltfImages.h) |
| Source/GltfLoader.cpp | [Source/Assets/GltfLoader.cpp](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Assets/GltfLoader.cpp) |
| Source/GltfLoader.h | [Source/Assets/GltfLoader.h](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Assets/GltfLoader.h) |
| Source/GpuTiming.cpp | [Source/Diagnostics/GpuTiming.cpp](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Diagnostics/GpuTiming.cpp) |
| Source/GpuTiming.h | [Source/Diagnostics/GpuTiming.h](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Diagnostics/GpuTiming.h) |
| Source/GraphicsPipeline.cpp | [Source/Rendering/Pipeline/GraphicsPipeline.cpp](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Rendering/Pipeline/GraphicsPipeline.cpp) |
| Source/GraphicsPipeline.h | [Source/Rendering/Pipeline/GraphicsPipeline.h](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Rendering/Pipeline/GraphicsPipeline.h) |
| Source/IDrawCommands.h | [Source/Rendering/IDrawCommands.h](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Rendering/IDrawCommands.h) |
| Source/ImageDecoder.cpp | [Source/Assets/ImageDecoder.cpp](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Assets/ImageDecoder.cpp) |
| Source/ImageDecoder.h | [Source/Assets/ImageDecoder.h](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Assets/ImageDecoder.h) |
| Source/Lighting.cpp | [Source/Scene/Lighting.cpp](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Scene/Lighting.cpp) |
| Source/Lighting.h | [Source/Scene/Lighting.h](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Scene/Lighting.h) |
| Source/Material.cpp | [Source/Rendering/Resources/Material.cpp](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Rendering/Resources/Material.cpp) |
| Source/Material.h | [Source/Rendering/Resources/Material.h](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Rendering/Resources/Material.h) |
| Source/Math.h | [Source/Math/Math.h](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Math/Math.h) |
| Source/MemoryAllocator.cpp | [Source/Rendering/Memory/MemoryAllocator.cpp](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Rendering/Memory/MemoryAllocator.cpp) |
| Source/MemoryAllocator.h | [Source/Rendering/Memory/MemoryAllocator.h](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Rendering/Memory/MemoryAllocator.h) |
| Source/Mesh.cpp | [Source/Rendering/Resources/Mesh.cpp](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Rendering/Resources/Mesh.cpp) |
| Source/Mesh.h | [Source/Rendering/Resources/Mesh.h](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Rendering/Resources/Mesh.h) |
| Source/MeshData.h | [Source/Assets/MeshData.h](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Assets/MeshData.h) |
| Source/MeshPrimitives.cpp | [Source/Assets/MeshPrimitives.cpp](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Assets/MeshPrimitives.cpp) |
| Source/MeshPrimitives.h | [Source/Assets/MeshPrimitives.h](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Assets/MeshPrimitives.h) |
| Source/MeshRenderer.cpp | [Source/Rendering/MeshRenderer.cpp](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Rendering/MeshRenderer.cpp) |
| Source/MeshRenderer.h | [Source/Rendering/MeshRenderer.h](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Rendering/MeshRenderer.h) |
| Source/ModelData.h | [Source/Assets/ModelData.h](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Assets/ModelData.h) |
| Source/PresentationSession.cpp | [Source/Rendering/Presentation/PresentationSession.cpp](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Rendering/Presentation/PresentationSession.cpp) |
| Source/PresentationSession.h | [Source/Rendering/Presentation/PresentationSession.h](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Rendering/Presentation/PresentationSession.h) |
| Source/RenderData.h | [Source/Rendering/RenderData.h](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Rendering/RenderData.h) |
| Source/ResourceUploader.cpp | [Source/Rendering/Memory/ResourceUploader.cpp](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Rendering/Memory/ResourceUploader.cpp) |
| Source/ResourceUploader.h | [Source/Rendering/Memory/ResourceUploader.h](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Rendering/Memory/ResourceUploader.h) |
| Source/Scene.cpp | [Source/Scene/Scene.cpp](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Scene/Scene.cpp) |
| Source/Scene.h | [Source/Scene/Scene.h](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Scene/Scene.h) |
| Source/SceneObject.cpp | [Source/Scene/SceneObject.cpp](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Scene/SceneObject.cpp) |
| Source/SceneObject.h | [Source/Scene/SceneObject.h](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Scene/SceneObject.h) |
| Source/SceneResources.cpp | [Source/Rendering/Resources/SceneResources.cpp](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Rendering/Resources/SceneResources.cpp) |
| Source/SceneResources.h | [Source/Rendering/Resources/SceneResources.h](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Rendering/Resources/SceneResources.h) |
| Source/ShaderModule.cpp | [Source/Rendering/Pipeline/ShaderModule.cpp](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Rendering/Pipeline/ShaderModule.cpp) |
| Source/ShaderModule.h | [Source/Rendering/Pipeline/ShaderModule.h](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Rendering/Pipeline/ShaderModule.h) |
| Source/StbImage.cpp | [Source/ThirdParty/StbImage.cpp](E:/MastersHW/VulkanProj/vulkan-renderer/Source/ThirdParty/StbImage.cpp) |
| Source/SwapChain.cpp | [Source/Rendering/Presentation/SwapChain.cpp](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Rendering/Presentation/SwapChain.cpp) |
| Source/SwapChain.h | [Source/Rendering/Presentation/SwapChain.h](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Rendering/Presentation/SwapChain.h) |
| Source/Texture.cpp | [Source/Rendering/Resources/Texture.cpp](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Rendering/Resources/Texture.cpp) |
| Source/Texture.h | [Source/Rendering/Resources/Texture.h](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Rendering/Resources/Texture.h) |
| Source/TexturePixels.cpp | [Source/Assets/TexturePixels.cpp](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Assets/TexturePixels.cpp) |
| Source/TexturePixels.h | [Source/Assets/TexturePixels.h](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Assets/TexturePixels.h) |
| Source/TextureSampling.h | [Source/Rendering/Resources/TextureSampling.h](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Rendering/Resources/TextureSampling.h) |
| Source/Transform.cpp | [Source/Math/Transform.cpp](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Math/Transform.cpp) |
| Source/Transform.h | [Source/Math/Transform.h](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Math/Transform.h) |
| Source/Vma.cpp | [Source/ThirdParty/Vma.cpp](E:/MastersHW/VulkanProj/vulkan-renderer/Source/ThirdParty/Vma.cpp) |
| Source/VmaConfig.h | [Source/Rendering/Memory/VmaConfig.h](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Rendering/Memory/VmaConfig.h) |
| Source/VulkanCheck.h | [Source/Rendering/Core/VulkanCheck.h](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Rendering/Core/VulkanCheck.h) |
| Source/VulkanContext.cpp | [Source/Rendering/Core/VulkanContext.cpp](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Rendering/Core/VulkanContext.cpp) |
| Source/VulkanContext.h | [Source/Rendering/Core/VulkanContext.h](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Rendering/Core/VulkanContext.h) |
| Source/Window.cpp | [Source/Platform/Window.cpp](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Platform/Window.cpp) |
| Source/Window.h | [Source/Platform/Window.h](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Platform/Window.h) |
| Source/main.cpp | [Source/Application/main.cpp](E:/MastersHW/VulkanProj/vulkan-renderer/Source/Application/main.cpp) |
| Tests/CameraTransformTests.cpp | [Tests/Unit/CameraTransformTests.cpp](E:/MastersHW/VulkanProj/vulkan-renderer/Tests/Unit/CameraTransformTests.cpp) |
| Tests/CameraTransformTests.h | [Tests/Unit/CameraTransformTests.h](E:/MastersHW/VulkanProj/vulkan-renderer/Tests/Unit/CameraTransformTests.h) |
| Tests/DepthBufferTests.cpp | [Tests/Rendering/DepthBufferTests.cpp](E:/MastersHW/VulkanProj/vulkan-renderer/Tests/Rendering/DepthBufferTests.cpp) |
| Tests/DepthBufferTests.h | [Tests/Rendering/DepthBufferTests.h](E:/MastersHW/VulkanProj/vulkan-renderer/Tests/Rendering/DepthBufferTests.h) |
| Tests/FrustumTests.cpp | [Tests/Unit/FrustumTests.cpp](E:/MastersHW/VulkanProj/vulkan-renderer/Tests/Unit/FrustumTests.cpp) |
| Tests/FrustumTests.h | [Tests/Unit/FrustumTests.h](E:/MastersHW/VulkanProj/vulkan-renderer/Tests/Unit/FrustumTests.h) |
| Tests/GltfTests.cpp | [Tests/Assets/GltfTests.cpp](E:/MastersHW/VulkanProj/vulkan-renderer/Tests/Assets/GltfTests.cpp) |
| Tests/GltfTests.h | [Tests/Assets/GltfTests.h](E:/MastersHW/VulkanProj/vulkan-renderer/Tests/Assets/GltfTests.h) |
| Tests/LightingTextureTests.cpp | [Tests/Rendering/LightingTextureTests.cpp](E:/MastersHW/VulkanProj/vulkan-renderer/Tests/Rendering/LightingTextureTests.cpp) |
| Tests/LightingTextureTests.h | [Tests/Rendering/LightingTextureTests.h](E:/MastersHW/VulkanProj/vulkan-renderer/Tests/Rendering/LightingTextureTests.h) |
| Tests/MeshTests.cpp | [Tests/Rendering/MeshTests.cpp](E:/MastersHW/VulkanProj/vulkan-renderer/Tests/Rendering/MeshTests.cpp) |
| Tests/MeshTests.h | [Tests/Rendering/MeshTests.h](E:/MastersHW/VulkanProj/vulkan-renderer/Tests/Rendering/MeshTests.h) |
| Tests/ModernRendererTests.cpp | [Tests/Rendering/ModernRendererTests.cpp](E:/MastersHW/VulkanProj/vulkan-renderer/Tests/Rendering/ModernRendererTests.cpp) |
| Tests/ModernRendererTests.h | [Tests/Rendering/ModernRendererTests.h](E:/MastersHW/VulkanProj/vulkan-renderer/Tests/Rendering/ModernRendererTests.h) |
| Tests/RenderProbe.h | [Tests/Support/RenderProbe.h](E:/MastersHW/VulkanProj/vulkan-renderer/Tests/Support/RenderProbe.h) |
| Tests/RendererSmokeTests.cpp | [Tests/Integration/RendererSmokeTests.cpp](E:/MastersHW/VulkanProj/vulkan-renderer/Tests/Integration/RendererSmokeTests.cpp) |

## Other files changed

- VulkanProj.vcxproj: updated paths and include roots; exposed documentation/shader binaries.
- VulkanProj.vcxproj.filters: rebuilt matching folder filters with unique items and identifiers.
- README.md: updated sample commands and documented folder layout.
- Assets/README.md: updated fixture locations.
- Assets/Benchmarks/BenchmarkScene.gltf: adjusted relative buffer/image URIs after the move.
- Docs/IMPLEMENTATION_6.md: updated historical inventory links to moved files.

Created: Docs/PROJECT_ORGANIZATION.md (this file). No source behavior changes are intended.
