---
title: Adapter Enumeration
image: assets/helloworld.png
download: files/adapter_enum.cpp
downloadLabel: Download
---
List every GPU adapter for each available renderer backend (DX11, DX12, OpenGL, Vulkan) using the high-level `Krom::` API. No window or renderer is created — useful as a diagnostic tool and as a starting point for picking a specific GPU.

Covers `Krom::IsRendererAvailable`, `Krom::EnumerateAdapters`, `Krom::RendererName`, and the adapter fields (`index`, `name`, `isDiscrete`, `dedicatedVRAM`, `featureLevel`). The example simply prints all adapters per backend; it does not auto-select one.
