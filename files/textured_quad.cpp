// textured_quad.cpp
//
// KROM Engine — Textured Quad (DX11)
//
// Renders a textured quad using the DX11 backend.
//
// Demonstrates:
//   - DeviceFactory adapter selection
//   - AssetPipeline: shader and texture loading
//   - MaterialSystem: material with texture binding
//   - ECS: entity with MeshComponent, MaterialComponent, TransformComponent
//   - PlatformRenderLoop: full frame loop
//
// Build (Visual Studio, x64, Windows):
//   Link: engine_core, engine_forward, engine_platform_win32
//   Self-registering addon: krom_link_self_registering_addon(target engine_dx11)
//   Assets: copy the assets/ folder next to the executable.

#include "ForwardFeature.hpp"
#include "DX11Device.hpp"
#include "assets/AssetPipeline.hpp"
#include "assets/AssetRegistry.hpp"
#include "core/Debug.hpp"
#include "core/Math.hpp"
#include "ecs/Components.hpp"
#include "ecs/World.hpp"
#include "events/EventBus.hpp"
#include "platform/StdTiming.hpp"
#include "platform/Win32Platform.hpp"
#include "renderer/IDevice.hpp"
#include "renderer/MaterialSystem.hpp"
#include "renderer/PlatformRenderLoop.hpp"
#include "renderer/RendererTypes.hpp"
#include <filesystem>

using namespace engine;

int main()
{
#ifdef _WIN32
    Debug::MinLevel = LogLevel::Info;
    RegisterAllComponents();

    // -------------------------------------------------------------------------
    // Adapter
    // -------------------------------------------------------------------------
    const auto adapters = renderer::DeviceFactory::EnumerateAdapters(
        renderer::DeviceFactory::BackendType::DirectX11);
    if (adapters.empty()) { Debug::LogError("no DX11 adapters"); return -1; }

    const uint32_t adapterIndex = renderer::DeviceFactory::FindBestAdapter(adapters);
    Debug::Log("selected adapter: %s", adapters[adapterIndex].name.c_str());

    // -------------------------------------------------------------------------
    // Platform + render loop
    // -------------------------------------------------------------------------
    platform::win32::Win32Platform winPlatform;
    if (!winPlatform.Initialize()) return -1;

    events::EventBus bus;
    renderer::PlatformRenderLoop loop;
    loop.GetRenderSystem().RegisterFeature(
        renderer::addons::forward::CreateForwardFeature());

    platform::WindowDesc wDesc{};
    wDesc.title  = "KROM - Textured Quad (DX11)";
    wDesc.width  = 1280u;
    wDesc.height = 720u;

    renderer::IDevice::DeviceDesc dDesc{};
    dDesc.enableDebugLayer = true;
    dDesc.adapterIndex     = adapterIndex;

    if (!loop.Initialize(renderer::DeviceFactory::BackendType::DirectX11,
                         winPlatform, wDesc, &bus, dDesc))
    {
        winPlatform.Shutdown();
        return -2;
    }

    // -------------------------------------------------------------------------
    // Assets
    // -------------------------------------------------------------------------
    assets::AssetRegistry registry;
    loop.GetRenderSystem().SetAssetRegistry(&registry);

    assets::AssetPipeline pipeline(registry, loop.GetRenderSystem().GetDevice());

    const auto assetRoot =
        std::filesystem::path(__FILE__).parent_path() / "assets";
    pipeline.SetAssetRoot(assetRoot.string());

    const ShaderHandle vs  = pipeline.LoadShader("quad_unlit.hlslvs",  assets::ShaderStage::Vertex);
    const ShaderHandle ps  = pipeline.LoadShader("quad_unlit.hlslps",  assets::ShaderStage::Fragment);
    const ShaderHandle tvs = pipeline.LoadShader("fullscreen.hlslvs",  assets::ShaderStage::Vertex);
    const ShaderHandle tps = pipeline.LoadShader("passthrough.hlslps", assets::ShaderStage::Fragment);

    const TextureHandle tex    = pipeline.LoadTexture("krom.bmp");
    pipeline.UploadPendingGpuAssets();
    const TextureHandle gpuTex = pipeline.GetGpuTexture(tex);

    // -------------------------------------------------------------------------
    // Quad mesh
    // -------------------------------------------------------------------------
    auto meshAsset = std::make_unique<assets::MeshAsset>();
    assets::SubMeshData quad;
    quad.positions = {
        -0.5f, -0.5f, 0.f,   0.5f, -0.5f, 0.f,
         0.5f,  0.5f, 0.f,  -0.5f,  0.5f, 0.f,
    };
    quad.normals = {
        0.f, 0.f, 1.f,  0.f, 0.f, 1.f,
        0.f, 0.f, 1.f,  0.f, 0.f, 1.f,
    };
    quad.uvs     = { 0.f,1.f, 1.f,1.f, 1.f,0.f, 0.f,0.f };
    quad.indices = { 0,1,2, 2,3,0 };
    meshAsset->submeshes.push_back(std::move(quad));
    const MeshHandle mesh = registry.meshes.Add(std::move(meshAsset));

    // -------------------------------------------------------------------------
    // Material
    // -------------------------------------------------------------------------
    renderer::MaterialSystem materials;

    renderer::VertexLayout layout;
    layout.attributes = {
        { renderer::VertexSemantic::Position,  renderer::Format::RGB32_FLOAT, 0,  0 },
        { renderer::VertexSemantic::Normal,    renderer::Format::RGB32_FLOAT, 0, 12 },
        { renderer::VertexSemantic::TexCoord0, renderer::Format::RG32_FLOAT,  0, 24 },
    };
    layout.bindings = { { 0, 32u } };

    renderer::MaterialParam albedo{};
    albedo.name    = "albedo";
    albedo.type    = renderer::MaterialParam::Type::Texture;
    albedo.texture = gpuTex;

    renderer::MaterialParam sampler{};
    sampler.name       = "sampler_albedo";
    sampler.type       = renderer::MaterialParam::Type::Sampler;
    sampler.samplerIdx = 0u;

    renderer::MaterialDesc mat{};
    mat.name           = "QuadUnlit";
    mat.passTag        = renderer::RenderPassTag::Opaque;
    mat.vertexShader   = vs;
    mat.fragmentShader = ps;
    mat.vertexLayout   = layout;
    mat.colorFormat    = renderer::Format::RGBA16_FLOAT;
    mat.depthFormat    = renderer::Format::D24_UNORM_S8_UINT;
    mat.params         = { albedo, sampler };
    const MaterialHandle matHandle = materials.RegisterMaterial(std::move(mat));

    // Tonemap (fullscreen passthrough)
    renderer::DepthStencilState noDepth{};
    noDepth.depthEnable = false;
    noDepth.depthWrite  = false;

    renderer::MaterialParam tmSampler{};
    tmSampler.name = "linearclamp";
    tmSampler.type = renderer::MaterialParam::Type::Sampler;

    renderer::MaterialDesc tmDesc{};
    tmDesc.name           = "Passthrough";
    tmDesc.passTag        = renderer::RenderPassTag::Opaque;
    tmDesc.vertexShader   = tvs;
    tmDesc.fragmentShader = tps;
    tmDesc.depthStencil   = noDepth;
    tmDesc.colorFormat    = renderer::Format::RGBA8_UNORM_SRGB;
    tmDesc.depthFormat    = renderer::Format::D24_UNORM_S8_UINT;
    tmDesc.params         = { tmSampler };
    const MaterialHandle tmHandle = materials.RegisterMaterial(std::move(tmDesc));
    loop.GetRenderSystem().SetDefaultTonemapMaterial(tmHandle, materials);

    // -------------------------------------------------------------------------
    // ECS scene
    // -------------------------------------------------------------------------
    ecs::World world;

    const auto entity = world.CreateEntity();
    world.Add<TransformComponent>(entity);
    world.Add<WorldTransformComponent>(entity);
    world.Add<MeshComponent>(entity, mesh);
    world.Add<MaterialComponent>(entity, matHandle);
    world.Add<BoundsComponent>(entity, BoundsComponent{
        .centerWorld  = { 0.f, 0.f, 0.f },
        .extentsWorld = { 0.5f, 0.5f, 0.01f },
        .boundingSphere = 0.71f,
    });

    // -------------------------------------------------------------------------
    // Camera
    // -------------------------------------------------------------------------
    renderer::RenderView view{};
    view.view           = math::Mat4::LookAtRH({ 0.f,0.f,3.f }, {}, math::Vec3::Up());
    view.cameraPosition = { 0.f, 0.f, 3.f };
    view.cameraForward  = { 0.f, 0.f,-1.f };

    const auto updateProj = [&]() {
        const auto* sc = loop.GetRenderSystem().GetSwapchain();
        const float w  = sc ? static_cast<float>(sc->GetWidth())  : 1280.f;
        const float h  = sc ? static_cast<float>(sc->GetHeight()) : 720.f;
        view.projection = math::Mat4::PerspectiveFovRH(
            60.f * math::DEG_TO_RAD, w / h, 0.1f, 100.f);
    };
    updateProj();

    // -------------------------------------------------------------------------
    // Game loop
    // -------------------------------------------------------------------------
    platform::StdTiming timing;
    while (!loop.ShouldExit())
    {
        if (auto* input = loop.GetInput();
            input && input->IsKeyPressed(platform::Key::Escape))
            loop.GetWindow()->RequestClose();

        updateProj();
        if (!loop.Tick(world, materials, view, timing))
            break;
    }

    loop.Shutdown();
    winPlatform.Shutdown();
    return 0;
#else
    return 0;
#endif
}
