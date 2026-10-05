# Slang shader migration

The three GLSL shaders are replaced by Slang entry-point files and a shared include. Runtime SPIR-V
names, Vulkan descriptors, buffer layouts, and pipeline entry-point names are preserved. The existing
C++ renderer needs no API changes. README.md documents compiler requirements and build flags.

## Files created

- Shaders/mesh-common.slang
- Shaders/mesh.vert.slang
- Shaders/mesh.frag.slang
- Shaders/mesh-attributes.frag.slang
- Docs/SLANG_MIGRATION.md

## Files changed

- Shaders/vert.spv
- Shaders/frag.spv
- Shaders/mesh-attributes.spv
- VulkanProj.vcxproj
- VulkanProj.vcxproj.filters
- README.md
- Docs/IMPLEMENTATION_6.md: historical shader links updated to the new sources.

## Files removed

- Shaders/mesh.vert
- Shaders/mesh.frag
- Shaders/mesh-attributes.frag
