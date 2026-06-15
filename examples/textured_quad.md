---
title: Textured Quad
image: assets/quad.png
download: files/textured_quad.cpp
downloadLabel: Download
---
Render a textured, rotating quad with the high-level `Krom::` wrapper on the OpenGL backend. Geometry is built by hand, a PBR material with a base-color texture is applied, and a camera and directional light are set up.

Covers `Krom::SetAssetRoot`, `Krom::CreateCamera`, `Krom::CreateDirectionalLight`, mesh construction via `Krom::CreateMesh` / `CreateSurface` / `AddVertex` / `AddTriangle`, and material setup via `Krom::SetPBR` with `Krom::Texture`. The quad is spun each frame with `Krom::TurnEntity`.
