---
title: Load 3D Model
image: assets/3dmodel.png
download: files/load_3d_model.cpp
downloadLabel: Download
---
Load a glTF model from disk and drive it with custom ECS components and systems, using the high-level `Krom::` wrapper on the OpenGL backend. A dragon model spins via a user-defined system while a free-fly camera explores the scene.

Covers `Krom::CreateEntityFromAsset` (glTF/`.glb` import), custom components and systems via `Krom::RegisterComponent` / `AddComponent` / `RegisterSystem` and `world.View<>`, PBR materials with normal maps and UV scaling (`Krom::SetPBR`), child-entity access (`Krom::GetChild`), and a pivot-based FPS camera (WASD + Q/E + arrow keys).
