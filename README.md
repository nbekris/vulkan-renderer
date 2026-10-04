# Vulkan triangle

Build `VulkanProj.vcxproj` with Visual Studio/MSBuild (x64). The Vulkan SDK's
`glslc` compiles the shaders. Run from the repository root so `Shaders/*.spv`
can be found. `x64/Debug/VulkanProj.exe --frames 120` runs a finite smoke test.

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

The triangle's positions and colors are uploaded into a VMA vertex buffer.
Mapped writes are flushed to support noncoherent memory. Swapchain images
remain owned by Vulkan and must not be allocated or freed through VMA.

## Components

All project classes live in the `VulkanRenderer` namespace.

| Component | Responsibility |
| --- | --- |
| `Application` | Composition and the window event loop |
| `Window` | GLFW session and native window |
| `DeviceSelection` | Device capability queries and suitability |
| `VulkanContext` | Instance, surface, device, and borrowed queues |
| `MemoryAllocator`, `AllocatedBuffer` | VMA allocator and buffer ownership |
| `SwapChain` | Presentation images and image views |
| `RenderTargets` | Color render pass and framebuffers |
| `ShaderModule` | SPIR-V loading and shader module ownership |
| `GraphicsPipeline` | Graphics pipeline creation and binding |
| `TriangleMesh` | Vertex data, layout, and mesh drawing |
| `TrianglePass` | Composition of the triangle pipeline and mesh |
| `FrameRenderer` | Command recording, submission, and synchronization |
| `PresentationSession` | One generation of swapchain-dependent resources |

`FrameRenderer` depends on the single-method `IDrawCommands` interface instead
of a specific scene. Implement `Record` to add a different scene without changing
frame scheduling; it runs inside the active render pass. `GraphicsPipeline`
accepts shader paths and a vertex layout instead of knowing about triangle data.
This keeps responsibilities separate, favors composition, and provides a narrow
substitution point without adding interfaces to every resource owner.

The window supports resizing. Framebuffer callbacks, out-of-date acquisition,
and suboptimal/out-of-date presentation retire the current `PresentationSession`.
Its RAII owners wait for the GPU and release the old resources; the next session
recreates the swapchain, render targets, viewport/scissor pipeline, triangle,
commands, and synchronization at the current framebuffer size. The window,
Vulkan device, and VMA allocator persist across sessions. Minimized and
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
```

Build with `/p:RendererSmokeTests=true` to select `Tests/RendererSmokeTests.cpp`
as the entry point and write `x64/Debug/RendererSmokeTests.exe`. This integration
test requires a Vulkan-capable GPU. It checks repeated application lifetimes,
VMA allocation release and upload bounds, shader loading failure after a module
has been created, and alternate drawing commands that throw with frames in flight.
It also checks repeated window resizing, framebuffer/swapchain extent agreement,
minimize/restore, and closing while minimized.
Run it with validation enabled to check Vulkan resource destruction as well.

## VMA dependency

Vulkan Memory Allocator **3.3.0** is vendored under `Source/ThirdParty/VMA`,
including its MIT license, from:
https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator/tree/v3.3.0

`Source/Vma.cpp` is the only translation unit defining `VMA_IMPLEMENTATION`.
`VmaConfig.h` sets Vulkan 1.0 support to match the application's API version.
