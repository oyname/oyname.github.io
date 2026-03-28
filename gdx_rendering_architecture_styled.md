# GDX Rendering Architecture

> Styled Markdown-Version im Screenshot-Look.  
> Hinweis: Das dunkle Box-Layout rendert am besten in Markdown-Viewern, die HTML in `.md` erlauben.

<div style="background:#171717; color:#e8e8e8; padding:28px; border-radius:18px; font-family:Segoe UI,Arial,sans-serif; line-height:1.35;">

  <div style="background:#4a3fa8; color:#ffffff; border:2px solid #7268d8; border-radius:14px; padding:14px 18px; text-align:center; margin:0 0 18px 0; box-shadow:0 0 0 1px rgba(255,255,255,0.05) inset;">
    <div style="font-size:28px; font-weight:700;">Public API — <code>gidx.h</code></div>
    <div style="font-size:16px; opacity:0.9;">Engine::Graphics() · CreateMesh/Material/Camera · Run(tickFn)</div>
    <div style="font-size:15px; opacity:0.8; margin-top:6px;">Flat OYNAME-style wrapper · Keine ECS-Typen sichtbar · Nur <code>LPENTITY</code>, <code>LPMATERIAL</code>, <code>LPMESH</code></div>
  </div>

  <div style="text-align:center; font-size:28px; color:#9a9a9a; margin:-4px 0 8px 0;">↓</div>

  <div style="background:#0d4d90; color:#ffffff; border:2px solid #2d79c7; border-radius:14px; padding:14px 18px; text-align:center; margin:0 0 18px 0;">
    <div style="font-size:28px; font-weight:700;">GDXECSRenderer <span style="opacity:0.8;">(engine bridge)</span></div>
    <div style="font-size:16px; opacity:0.92;">BeginFrame → EndFrame · ShaderVariantCache · PostProcess Management · FreeCamera</div>
    <div style="font-size:15px; opacity:0.84; margin-top:6px;">Besitzt <code>unique_ptr&lt;IGDXRenderBackend&gt;</code>, Registry und alle ResourceStores · einziger Eintrittspunkt pro Frame</div>
  </div>

  <div style="border:1px dashed #454545; border-radius:14px; padding:16px; margin:0 0 18px 0;">
    <div style="color:#a4a4a4; font-size:14px; margin:0 0 12px 2px;">ECS + Resources</div>

    <table style="width:100%; border-collapse:separate; border-spacing:12px 10px;">
      <tr>
        <td style="width:25%; background:#0b7264; border:1px solid #24a896; border-radius:12px; padding:14px; vertical-align:top; text-align:center; color:#dffaf4;">
          <div style="font-size:20px; font-weight:700;">Registry</div>
          <div style="font-size:15px; margin-top:8px;">EntityID</div>
          <div style="font-size:14px; opacity:0.9;">24-bit Index + 8-bit Gen</div>
          <div style="font-size:14px; margin-top:8px;">ComponentPool&lt;T&gt;</div>
        </td>
        <td style="width:25%; background:#0b7264; border:1px solid #24a896; border-radius:12px; padding:14px; vertical-align:top; text-align:center; color:#dffaf4;">
          <div style="font-size:20px; font-weight:700;">Systems</div>
          <div style="font-size:14px; margin-top:8px;">Transform</div>
          <div style="font-size:14px;">Camera</div>
          <div style="font-size:14px;">ViewCulling</div>
          <div style="font-size:14px;">RenderGather</div>
        </td>
        <td style="width:25%; background:#0b7264; border:1px solid #24a896; border-radius:12px; padding:14px; vertical-align:top; text-align:center; color:#dffaf4;">
          <div style="font-size:20px; font-weight:700;">ResourceStores</div>
          <div style="font-size:14px; margin-top:8px;">Mesh · Material · Shader</div>
          <div style="font-size:14px;">Texture · RenderTarget · PostProcess</div>
          <div style="font-size:14px; margin-top:8px;"><code>Handle&lt;Tag&gt;</code> · 32-bit generation handles</div>
        </td>
        <td style="width:25%; background:#0b7264; border:1px solid #24a896; border-radius:12px; padding:14px; vertical-align:top; text-align:center; color:#dffaf4;">
          <div style="font-size:20px; font-weight:700;">Scheduler</div>
          <div style="font-size:14px; margin-top:8px;">SystemScheduler</div>
          <div style="font-size:14px;">Bitmask batches</div>
          <div style="font-size:14px;"><code>Execute(nullptr)</code> seriell</div>
        </td>
      </tr>
    </table>
  </div>

  <div style="text-align:center; font-size:28px; color:#9a9a9a; margin:-4px 0 8px 0;">↓</div>

  <div style="border:1px dashed #454545; border-radius:14px; padding:16px; margin:0 0 18px 0;">
    <div style="color:#a4a4a4; font-size:14px; margin:0 0 12px 2px;">Frame pipeline <span style="opacity:0.8;">(namespace RFG)</span></div>

    <table style="width:100%; border-collapse:separate; border-spacing:12px 10px;">
      <tr>
        <td style="width:25%; background:#9b4b1f; border:1px solid #d67a42; border-radius:12px; padding:14px; vertical-align:top; text-align:center; color:#fff1e6;">
          <div style="font-size:20px; font-weight:700;">ViewPassData</div>
          <div style="font-size:14px; margin-top:8px;">ViewData · ExecuteData</div>
          <div style="font-size:14px;">VisibleSet · ViewStats</div>
          <div style="font-size:14px; margin-top:8px;">Queues: opaque / alpha / shadow</div>
        </td>
        <td style="width:25%; background:#9b4b1f; border:1px solid #d67a42; border-radius:12px; padding:14px; vertical-align:top; text-align:center; color:#fff1e6;">
          <div style="font-size:20px; font-weight:700;">FrameGraph</div>
          <div style="font-size:14px; margin-top:8px;">Node &lt;shadow / graphics / present&gt;</div>
          <div style="font-size:14px;">ordered executeFn lambdas</div>
        </td>
        <td style="width:25%; background:#9b4b1f; border:1px solid #d67a42; border-radius:12px; padding:14px; vertical-align:top; text-align:center; color:#fff1e6;">
          <div style="font-size:20px; font-weight:700;">FrameDispatch</div>
          <div style="font-size:14px; margin-top:8px;">ViewPrepCtx</div>
          <div style="font-size:14px;">CullGatherCtx</div>
          <div style="font-size:14px;">PostProcCtx · ExecCtx</div>
        </td>
        <td style="width:25%; background:#9b4b1f; border:1px solid #d67a42; border-radius:12px; padding:14px; vertical-align:top; text-align:center; color:#fff1e6;">
          <div style="font-size:20px; font-weight:700;">Post-process</div>
          <div style="font-size:14px; margin-top:8px;">Bloom · ToneMap</div>
          <div style="font-size:14px;">FXAA · GTAO</div>
          <div style="font-size:14px;">ping-pong surfaces</div>
        </td>
      </tr>
    </table>

    <div style="margin-top:10px; color:#c6c6c6; font-size:14px;">
      <strong>RFG::PipelineData</strong> enthält <code>mainView</code> + <code>rttViews</code>.  
      <strong>FrameDispatch</strong> hält alle Context-Structs mit Frame-Lifetime und beseitigt das frühere Stack-Lifetime-Problem in <code>EndFrame()</code>.
    </div>
  </div>

  <div style="text-align:center; font-size:28px; color:#9a9a9a; margin:-4px 0 8px 0;">↓</div>

  <div style="background:#555555; color:#ffffff; border:2px solid #8e8e8e; border-radius:14px; padding:14px 18px; text-align:center; margin:0 0 18px 0;">
    <div style="font-size:28px; font-weight:700;">IGDXRenderBackend <span style="opacity:0.82;">(neutral interface)</span></div>
    <div style="font-size:16px; opacity:0.92;">UploadShader/Texture/Mesh/Material · ExecuteRenderPass · ExecuteShadowPass · ExecutePostProcessChain</div>
    <div style="font-size:15px; opacity:0.86; margin-top:6px;">Backend-neutral · keine DX-Typen · keine <code>void*</code> · optionale Methoden mit No-op-Defaults</div>
  </div>

  <div style="text-align:center; font-size:28px; color:#9a9a9a; margin:-4px 0 8px 0;">↓</div>

  <table style="width:100%; border-collapse:separate; border-spacing:12px 10px; margin:0 0 18px 0;">
    <tr>
      <td style="width:50%; background:#0d4d90; border:1px solid #2d79c7; border-radius:12px; padding:16px; vertical-align:top; color:#e9f3ff;">
        <div style="font-size:24px; font-weight:700; text-align:center;">GDXDX11RenderBackend <span style="opacity:0.82;">(primary)</span></div>
        <div style="font-size:14px; text-align:center; margin-top:8px;">DX11-Implementierung des neutralen Interfaces</div>
        <div style="font-size:14px; margin-top:12px;">
          <strong>GPU-Ownership:</strong> <code>GDXDX11GpuRegistry</code> mit 6 <code>unordered_map</code>s  
          (Texture, RenderTarget, Shader, Mesh, Material, PostProcess)
        </div>
        <div style="font-size:14px; margin-top:10px;">
          <strong>DX11MeshGpu:</strong> getrennte VB-Streams pro Semantik  
          Position · Normal · UV · Tangent · Bone
        </div>
        <div style="font-size:14px; margin-top:10px;">
          <strong>DX11RenderTargetGpu:</strong> Color + Depth + Normal als RTV + SRV  
          für GTAO und weitere Screen-Space-Pässe
        </div>
      </td>
      <td style="width:25%; background:#4e4e4e; border:1px solid #858585; border-radius:12px; padding:16px; vertical-align:middle; color:#efefef; text-align:center;">
        <div style="font-size:20px; font-weight:700;">DX12 backend</div>
        <div style="font-size:15px; opacity:0.8;">stub</div>
      </td>
      <td style="width:25%; background:#4e4e4e; border:1px solid #858585; border-radius:12px; padding:16px; vertical-align:middle; color:#efefef; text-align:center;">
        <div style="font-size:20px; font-weight:700;">OpenGL</div>
        <div style="font-size:15px; opacity:0.8;">stub</div>
      </td>
    </tr>
  </table>

  <div style="text-align:center; font-size:28px; color:#9a9a9a; margin:-4px 0 8px 0;">↓</div>

  <div style="border:1px dashed #454545; border-radius:14px; padding:16px; margin:0 0 18px 0;">
    <div style="color:#a4a4a4; font-size:14px; margin:0 0 12px 2px;">HLSL shaders <span style="opacity:0.8;">(runtime D3DCompileFromFile)</span></div>

    <table style="width:100%; border-collapse:separate; border-spacing:12px 10px;">
      <tr>
        <td style="width:30%; background:#9a6209; border:1px solid #d79a2f; border-radius:12px; padding:14px; vertical-align:top; text-align:center; color:#fff5df;">
          <div style="font-size:22px; font-weight:700;">PBR shaders</div>
          <div style="font-size:14px; margin-top:8px;">PixelShader (Cook-Torrance)</div>
          <div style="font-size:14px;">GGX · ddx/ddy TBN</div>
          <div style="font-size:14px;">CSM PCF 3×3</div>
          <div style="font-size:14px;">IBL via t17 / t18 / t19</div>
        </td>
        <td style="width:23%; background:#9a6209; border:1px solid #d79a2f; border-radius:12px; padding:14px; vertical-align:top; text-align:center; color:#fff5df;">
          <div style="font-size:22px; font-weight:700;">CSM shadows</div>
          <div style="font-size:14px; margin-top:8px;">4 Varianten</div>
          <div style="font-size:14px;">plain</div>
          <div style="font-size:14px;">AlphaTest</div>
          <div style="font-size:14px;">Skinned</div>
          <div style="font-size:14px;">SkinnedAlphaTest</div>
        </td>
        <td style="width:27%; background:#9a6209; border:1px solid #d79a2f; border-radius:12px; padding:14px; vertical-align:top; text-align:center; color:#fff5df;">
          <div style="font-size:22px; font-weight:700;">Forward+</div>
          <div style="font-size:14px; margin-top:8px;"><code>TileLightCullCS.hlsl</code></div>
          <div style="font-size:14px;">16×16 Tiles</div>
          <div style="font-size:14px;">256 Lichter</div>
          <div style="font-size:14px;">Output: t20 / t21 / t22</div>
        </td>
        <td style="width:20%; background:#9a6209; border:1px solid #d79a2f; border-radius:12px; padding:14px; vertical-align:top; text-align:center; color:#fff5df;">
          <div style="font-size:22px; font-weight:700;">Post-PS</div>
          <div style="font-size:14px; margin-top:8px;">Bloom</div>
          <div style="font-size:14px;">ToneMap / ACES</div>
          <div style="font-size:14px;">FXAA</div>
          <div style="font-size:14px;">GTAO + Blur + Composite</div>
          <div style="font-size:14px;">Edge / Normal / Depth Debug</div>
        </td>
      </tr>
    </table>
  </div>

  <div style="background:#202020; border:1px solid #373737; border-radius:12px; padding:16px; margin-top:12px;">
    <div style="font-size:20px; font-weight:700; color:#f1f1f1; margin-bottom:10px;">Zusatzpunkte</div>
    <div style="font-size:14px; color:#d8d8d8;">
      <strong>IBL:</strong> <code>GDXIBLBaker</code> arbeitet asynchron und backt HDR-Input zu Irradiance, Prefiltered Maps und BRDF-LUT.  
      <strong>Designziel:</strong> Public API sauber halten, ECS intern kapseln, Frame-Daten einfrieren, Backend neutralisieren und Shader/Pass-Logik klar trennen.
    </div>
  </div>

</div>
