# Project models

Keep each model and its material/texture sidecars together in `Assets/Models/<Name>/`.
Paths inside OBJ/MTL, FBX, and other model files should remain valid relative to the model.

Run from the repository root:

```powershell
.\x64\Debug\VulkanProj.exe --model Assets/Models/SamplePyramid
.\x64\Debug\VulkanProj.exe --model Assets/Models/MyModel/model.fbx
```

A folder must contain exactly one supported model file at its top level. For folders with
multiple models, choose the file explicitly. Texture subfolders are supported; models are
not found recursively. Quoted paths support spaces. Imported models are framed automatically.

`SamplePyramid` is authored for this repository and exercises OBJ, MTL, and an external PNG.
Keep licensing information alongside any third-party models you add.
