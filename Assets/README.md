# Authored glTF fixtures

All meshes, scene layouts, and checker pixels here were created for this repository; no external
model licenses are required. Samples/SampleScene.gltf uses Samples/SampleScene.bin and Samples/checker.png, while
Samples/SampleScene.glb contains its geometry and PNG. Both contain one cube primitive reused by two
nodes, including hierarchy, rotation, and nonuniform scale. Benchmarks/BenchmarkScene.gltf reuses the same
binary and PNG in a grid of 2,048 nodes; --auto-frame off makes most nodes outside the default view.

Tests/Assets contains data-URI variants, sparse attributes, interleaved geometry with texture
transforms/unlit/alpha-mask materials, and intentionally invalid fixtures for rejection/rollback tests.
These are regression fixtures, not examples of full physically based rendering.

Project-local OBJ/FBX and other models belong under Models/<Name>. See [model folders](Models/README.md).
