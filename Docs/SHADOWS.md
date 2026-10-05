# Directional PCF shadows

Each presentation frame owns a 2048x2048 sampled depth image and nearest border-white sampler.
ShadowTarget selects a depth-only format with attachment and sampled-image support (D32, then D16).
Nearest depth samples are compared manually and averaged over a 3x3 kernel; linear depth filtering
is not required. This smooths edges, rather than simulating distance-dependent soft penumbras.

ShadowPass fits an orthographic light projection to transformed scene bounds with padding.
It handles vertical light directions and flat scene bounds, uses Vulkan zero-to-one depth and flipped Y,
and includes every scene object independently of the main camera culling. Large scene extents reduce detail.

Dynamic rendering clears/stores the shadow depth before the main pass. Synchronization2 barriers
transition from depth writes to fragment sampling; existing per-frame fences retire all access before reuse.
Descriptors use binding 3 with one combined sampled image per frame. FrameUniform is now 192 bytes,
including lightTransform at offset 112 and shadowParameters at offset 176. Push constants remain 80 bytes.

shadowParameters contains enabled, constant bias (0.0005), slope bias (0.002), and texel size (1/2048).
These are normalized shadow depth biases and can be tuned in RenderData.h. Shadow resolution lives in
ShadowTarget::RESOLUTION; texel size must match if the resolution is changed.
Opaque and alpha cutout materials cast shadows; the shadow fragment shader applies the same texture,
vertex color, tint alpha and cutoff as the main shader. Unlit surfaces do not receive shadows.
Only direct BRDF lighting is attenuated; ambient remains the existing approximation.

The GPU regression suite verifies all three frame slots, reuse, camera-culled shadow casters,
ambient preservation, enabled/disabled shadows, fractional PCF averaging, moving casters, and alpha cutouts.
Existing depth, material, camera, mesh, import, resize and synchronization checks remain active.

## Files created

- [Source/Rendering/Frames/ShadowTarget.h](../Source/Rendering/Frames/ShadowTarget.h)
- [Source/Rendering/Frames/ShadowTarget.cpp](../Source/Rendering/Frames/ShadowTarget.cpp)
- [Source/Rendering/ShadowPass.h](../Source/Rendering/ShadowPass.h)
- [Source/Rendering/ShadowPass.cpp](../Source/Rendering/ShadowPass.cpp)
- [Shaders/shadow.vert.slang](../Shaders/shadow.vert.slang)
- [Shaders/shadow.frag.slang](../Shaders/shadow.frag.slang)
- [Shaders/shadow-common.slang](../Shaders/shadow-common.slang)
- [Shaders/shadow-vert.spv](../Shaders/shadow-vert.spv)
- [Shaders/shadow-frag.spv](../Shaders/shadow-frag.spv)
- [Tests/Rendering/ShadowTests.h](../Tests/Rendering/ShadowTests.h)
- [Tests/Rendering/ShadowTests.cpp](../Tests/Rendering/ShadowTests.cpp)
- [Docs/SHADOWS.md](../Docs/SHADOWS.md)

## Files changed

- [Source/Rendering/Frames/FrameResources.h](../Source/Rendering/Frames/FrameResources.h)
- [Source/Rendering/Frames/FrameResources.cpp](../Source/Rendering/Frames/FrameResources.cpp)
- [Source/Rendering/Frames/FrameRing.h](../Source/Rendering/Frames/FrameRing.h)
- [Source/Rendering/Frames/FrameRing.cpp](../Source/Rendering/Frames/FrameRing.cpp)
- [Source/Rendering/Frames/FrameRenderer.cpp](../Source/Rendering/Frames/FrameRenderer.cpp)
- [Source/Rendering/RenderData.h](../Source/Rendering/RenderData.h)
- [Source/Rendering/IDrawCommands.h](../Source/Rendering/IDrawCommands.h)
- [Source/Rendering/MeshRenderer.h](../Source/Rendering/MeshRenderer.h)
- [Source/Rendering/MeshRenderer.cpp](../Source/Rendering/MeshRenderer.cpp)
- [Source/Rendering/Pipeline/GraphicsPipeline.cpp](../Source/Rendering/Pipeline/GraphicsPipeline.cpp)
- [Source/Rendering/Descriptors/GlobalDescriptors.cpp](../Source/Rendering/Descriptors/GlobalDescriptors.cpp)
- [Source/Rendering/Presentation/PresentationSession.cpp](../Source/Rendering/Presentation/PresentationSession.cpp)
- [Shaders/mesh-common.slang](../Shaders/mesh-common.slang)
- [Shaders/mesh.frag.slang](../Shaders/mesh.frag.slang)
- [Shaders/vert.spv](../Shaders/vert.spv)
- [Shaders/frag.spv](../Shaders/frag.spv)
- [Shaders/mesh-attributes.spv](../Shaders/mesh-attributes.spv)
- [Tests/Support/RenderProbe.h](../Tests/Support/RenderProbe.h)
- [Tests/Rendering/ModernRendererTests.cpp](../Tests/Rendering/ModernRendererTests.cpp)
- [VulkanProj.vcxproj](../VulkanProj.vcxproj)
- [VulkanProj.vcxproj.filters](../VulkanProj.vcxproj.filters)
- [README.md](../README.md)
