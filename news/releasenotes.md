---
title: 2026-03-25  Release Notes — Current Engine Status
---
## Added
- Dense ECS foundation with `EntityID`, generations, `ComponentPool`, and `Registry::View`
- Separated render frontend/backend structure with `IGDXRenderer` and `IGDXRenderBackend`
- Modularized render pipeline with view preparation, culling, gather, pass building, and execution
- First usable frame graph for pass and resource dependencies
- Typed resource handles and `ResourceStore`-based resource management
- Dedicated math core with engine-owned types instead of direct DX11 coupling
- `JobSystem` / `ParallelFor` for parallel frame preparation
- Camera, hierarchy, and transform systems
- Shadow map, light culling, and debug culling foundations
- Asset foundation for mesh, material, texture, shader, render target, and post-process resources
- Win32 platform path with window, input, and context creation
- DX11 backend as the primary functional rendering path

## Improved
- Better separation between Core, ECS, Scene, Renderer, Assets, and Platform
- The renderer is no longer a single monolithic execution path and is now split into distinct stages
- Math and Core layers are significantly more independent from the graphics backend
- Frame processing is easier to prepare and partially parallelized
- The architecture is more clearly prepared for future multi-backend expansion
- The resource model is more robust and controlled than direct pointer-heavy usage

## Changed
- Render logic has been separated more clearly from the actual API backend
- The ECS is now more data-oriented than a simple map-based approach
- Frame graph and pass-based design are increasingly replacing rigid linear rendering flow
- The project structure has been reorganized into logical modules instead of catch-all folders

## Current State
- DX11 is the primary real-world rendering path
- OpenGL and DX12 are present, but are still not in a state that should be considered complete
- The renderer, ECS, and resource foundation are solid enough for further expansion
- The architecture is clearly better structured, but not yet fully backend-neutral
- Some DX11-named systems are logically renderer-side code and should be split later

## Known Limitations
- The backend abstraction is still relatively high-level and not yet fully command-centric
- A frame graph is present, but it is not yet deep enough for large modern rendering pipelines
- Parts of lighting, tile culling, and special-case logic are still not cleanly separated architecturally
- The shader and pipeline system is still visibly shaped by the current DX11/HLSL path
- The OpenGL and DX12 paths are still in development and should not yet be treated as the architectural benchmark
- The test and sample structure is not yet systematically built out

## Next Focus
- Clear separation between general renderer logic and backend-specific implementation
- Expansion of the RHI layer for real DX11/OpenGL parity
- Further development of the frame graph
- Cleanup of incorrectly named DX11 files that actually carry renderer-side responsibility
- Stabilization of the rendering path before larger feature jumps such as GI, SSAO/GTAO, or more advanced post-processing
