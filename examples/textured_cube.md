---
title: Textured Cube
image: assets/trinagle.png
download: files/textured_cube.cpp
downloadLabel: Download
---
A single textured cube — six faces with per-face normals — rotating on two axes each frame, built with the high-level `Krom::` wrapper on the OpenGL backend.

Demonstrates manual mesh construction (`Krom::CreateSurface` / `AddVertex` / `AddTriangle`) via a small per-face helper, a PBR material through `Krom::SetPBR` + `Krom::Texture`, a camera, and a directional light. Rotation is applied with `Krom::SetRotationEulerDeg`.
