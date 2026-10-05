# Model import

Install the pinned Assimp 6.0.2 static dependency once, then build the renderer:

```powershell
powershell -ExecutionPolicy Bypass -File Tools/SetupAssimp.ps1
msbuild VulkanProj.vcxproj /p:Configuration=Debug /p:Platform=x64
.\x64\Debug\VulkanProj.exe --model Assets/Models/SamplePyramid
```

Setup needs network access, CMake, tar, and Visual Studio C++ tools. Use `-CMake <path>`
to select Visual Studio's bundled CMake if the system version lacks the VS 2026 generator.
VS 2022 users can pass `-Generator 'Visual Studio 17 2022'` for the dependency build.
Debug and Release libraries use the corresponding DLL C runtime. Generated files stay
under ignored `Libraries/Assimp`; no machine-wide package installation is needed.

Place new assets under `Assets/Models/<Name>/`, preserving their relative texture paths.
`--model` accepts a file or a folder with one model at its top level. Ambiguous folders
produce an error asking for a file. No option retains the original demo scene.

glTF/GLB retains the existing cgltf importer. Other Assimp-supported formats use
`AssimpLoader`, which triangulates polygons, generates missing smooth normals, preserves
node transforms and shared mesh instances, and imports vertex colors and UV set zero.
Assimp UVs are flipped to match the decoded image convention used by this renderer.

Base-color/diffuse textures can be external PNG/JPEG or embedded images. Material tint,
scalar metallic/roughness values, and repeat/clamp/mirror addressing are imported.
Legacy diffuse materials use dielectric defaults; this is an approximation of their shading.
Normal, specular, emissive, and metallic/roughness maps are not rendered. Assimp animation,
skinning, morph targets, transparent materials, and nonzero texture UV sets are rejected.
Source coordinate/unit conventions are preserved; export with the intended orientation/scale.

Both frontends produce `ModelData`. A shared upload step checks descriptor capacity and
rolls back objects and GPU resources on failure. Asset tables still have 32 material and
texture slots including defaults. Imports happen at startup before assets are frozen.

Assimp is distributed under its BSD-style license; it is preserved under `Source/ThirdParty/Assimp/LICENSE` and in the install folder.
See the pinned [upstream source](https://github.com/assimp/assimp/tree/v6.0.2).
