---
title: IBL Sphere
image: assets/sphere.png
download: files/ibl_sphere.cpp
downloadLabel: Download
---
A polished, metallic UV-sphere lit entirely by an HDR image-based lighting environment, on the DX11 backend. Shows how an `.hdr` environment drives reflections and ambient light on a low-roughness PBR surface.

Covers `Krom::CreateEnvironmentFromHDR` + `Krom::SetActiveEnvironment`, procedural sphere generation (stacks/slices) via `Krom::AddVertex` / `AddTriangle`, a high-metallic / low-roughness `Krom::PBR` material, and an orbiting FPS-style camera built from a pivot + child entity, controlled with WASD and the arrow keys.
