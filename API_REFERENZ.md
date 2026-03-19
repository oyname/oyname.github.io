# GIDX Engine API Referenz

Diese Referenz beschreibt die nutzbare Engine-API des Snapshots **gidx26 framegraph readonly snapshot**.

Der Fokus liegt auf dem echten Engine-Weg:

- Engine-Initialisierung
- Fenster- und Frame-Loop
- Event-System und ESC-Verhalten
- ECS-Nutzung
- Device-Enumeration unter DX11
- Renderer- und Ressourcen-API
- minimales Startbeispiel
- erstes Dreieck

Hinweis zur Einordnung:
Die Engine kann auf zwei Wegen genutzt werden. Der primäre Weg läuft über die eigentliche Engine-Struktur mit direktem Zugriff auf Fenster, Backend, Renderer, ECS und Ressourcen. Zusätzlich existiert mit `gidx.h` eine vereinfachte Schicht, die viele Standardschritte kapselt. Diese Referenz beschreibt zuerst den direkten Weg.

---

## 1. Architekturüberblick

Die Engine ist in mehrere klar getrennte Schichten aufgeteilt:

### `GDXEngine`
Top-Level-Laufzeit für:
- Fenster-Loop
- Event-Verarbeitung
- DeltaTime
- Frame-Ablauf

### `GDXECSRenderer`
Zentrale Runtime-API für:
- Renderer-Initialisierung
- Ressourcen-Erzeugung
- ECS-Registry
- Tick-Callback
- Frame-Ausführung
- Render Targets und Post-Processing

### `IGDXRenderBackend`
Abstrakte Backend-Schnittstelle.

### `GDXDX11RenderBackend`
Konkrete DX11-Implementierung.

### `Registry`
ECS-Storage für Entities und Komponenten.

### `ResourceStore<T, Tag>`
Handle-basierte Verwaltung für:
- Meshes
- Materialien
- Shader
- Texturen
- Render Targets
- Post-Process-Ressourcen

Praktisch ist DX11 im Snapshot der reale Hauptpfad.

---

## 2. Engine initialisieren

### 2.1 `GDXEngine`

```cpp
class GDXEngine
{
public:
    GDXEngine(std::unique_ptr<IGDXWindow> window,
              std::unique_ptr<IGDXRenderer> renderer,
              GDXEventQueue& events);

    bool Initialize();
    void Run();
    bool Step();

    float GetDeltaTime() const;
    float GetTotalTime() const;

    using EventFn = std::function<void(const Event&)>;
    void SetEventCallback(EventFn fn);

    void Shutdown();
};
```

### Zweck

`GDXEngine` besitzt Fenster und Renderer und steuert den Laufzeitzyklus.

### Wichtige Methoden

#### `bool Initialize()`
Initialisiert die Engine.

Typisches Verhalten:
- prüft Fenster und Renderer
- ruft `renderer->Initialize()` auf
- setzt initiale Viewport-Größe über `renderer->Resize(...)`
- startet den Frame-Timer

#### `void Run()`
Hauptloop der Anwendung. Ruft intern wiederholt `Step()` auf.

#### `bool Step()`
Führt genau einen Frame aus.

Typischer Frame-Ablauf:
1. Input-Status zurücksetzen
2. Fenster-Events pollen
3. DeltaTime berechnen
4. Events verarbeiten
5. bei minimiertem Fenster Rendern überspringen
6. sonst:
   - `BeginFrame()`
   - `Tick(dt)`
   - `EndFrame()`

Rückgabewert:
- `true` = weiterlaufen
- `false` = beenden

#### `float GetDeltaTime() const`
Delta-Zeit des letzten Frames in Sekunden.

#### `float GetTotalTime() const`
Gesamtlaufzeit seit Start.

#### `void Shutdown()`
Fährt die Engine sauber herunter. Ist laut Header idempotent, also mehrfach sicher aufrufbar.

---

## 3. Event-System und ESC-Verhalten

### 3.1 Event-Typen

```cpp
enum class Key
{
    Unknown,
    Escape, Space,
    A, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    Left, Right, Up, Down
};

struct QuitEvent {};
struct WindowResizedEvent { int width; int height; };
struct KeyPressedEvent { Key key; bool repeat; };
struct KeyReleasedEvent { Key key; };

using Event = std::variant<QuitEvent, WindowResizedEvent, KeyPressedEvent, KeyReleasedEvent>;
```

### 3.2 `GDXEventQueue`

Die Event-Queue wird von Plattformcode beschrieben und von der Engine pro Frame gelesen.

Zweck:
- Events sammeln
- im Frame konsistent verarbeiten

### 3.3 ESC-Verhalten

ESC wird bereits durch die Engine selbst behandelt. Das ist wichtig: man muss dafür nicht zwingend eigenen Spielcode schreiben.

Praktisch bedeutet das:
- `Escape` setzt den Lauf auf Beenden
- zusätzlich kann eigener Event-Code registriert werden

### 3.4 Eigener Event-Callback

```cpp
engine.SetEventCallback([&](const Event& e)
{
    std::visit([&](auto&& ev)
    {
        using T = std::decay_t<decltype(ev)>;

        if constexpr (std::is_same_v<T, KeyPressedEvent>)
        {
            if (ev.key == Key::Space)
            {
                // Eigene Reaktion auf Space
            }
        }
    }, e);
});
```

---

## 4. Input-API

### `GDXInput`

```cpp
class GDXInput
{
public:
    static void BeginFrame();
    static void OnEvent(const Event& e);

    static bool KeyDown(Key key);
    static bool KeyHit(Key key);
    static bool KeyReleased(Key key);
};
```

### Bedeutung

#### `KeyDown(key)`
Taste ist aktuell gehalten.

#### `KeyHit(key)`
Taste wurde in diesem Frame frisch gedrückt.

#### `KeyReleased(key)`
Taste wurde in diesem Frame losgelassen.

### Beispiel

```cpp
if (GDXInput::KeyDown(Key::Left))
{
    // kontinuierliche Bewegung nach links
}

if (GDXInput::KeyHit(Key::Space))
{
    // einmalige Aktion beim Drücken
}
```

---

## 5. DX11 Device-Enumeration und Context-Erzeugung

### `GDXWin32DX11ContextFactory`

```cpp
class GDXWin32DX11ContextFactory
{
public:
    static std::vector<GDXDXGIAdapterInfo> EnumerateAdapters();

    static unsigned int FindBestAdapter(
        const std::vector<GDXDXGIAdapterInfo>& adapters);

    std::unique_ptr<IGDXDXGIContext> Create(
        IGDXWin32NativeAccess& nativeAccess,
        unsigned int adapterIndex) const;
};
```

### Zweck

Diese Factory erstellt einen DX11-Context für ein Win32-Fenster.

### `EnumerateAdapters()`
Liest verfügbare Hardware-Adapter aus, ohne dauerhaft ein Device zu erzeugen.

Nutzen:
- GPU-Liste anzeigen
- Benutzer Auswahl geben
- beste GPU automatisch wählen

### `FindBestAdapter(...)`
Bestimmt den Adapter mit dem höchsten Feature-Level.

Das ist der schnellste Standardweg, wenn keine manuelle GPU-Auswahl nötig ist.

### `Create(...)`
Erzeugt einen DX11-Context für das Win32-Fenster und den gewählten Adapter.

Rückgabewert:
- gültiger `IGDXDXGIContext`, wenn erfolgreich
- `nullptr`, wenn die Erzeugung fehlschlägt

### Beispiel: Adapter enumerieren

```cpp
auto adapters = GDXWin32DX11ContextFactory::EnumerateAdapters();
for (size_t i = 0; i < adapters.size(); ++i)
{
    // Name, Vendor, Feature-Level usw. ausgeben
}
```

### Beispiel: besten Adapter wählen

```cpp
auto adapters = GDXWin32DX11ContextFactory::EnumerateAdapters();
unsigned int bestAdapter = GDXWin32DX11ContextFactory::FindBestAdapter(adapters);
```

---

## 6. Renderer-API

### 6.1 `IGDXRenderer`

```cpp
class IGDXRenderer
{
public:
    virtual ~IGDXRenderer() = default;

    virtual bool Initialize() = 0;
    virtual void BeginFrame() = 0;
    virtual void Tick(float dt) = 0;
    virtual void EndFrame() = 0;
    virtual void Resize(int w, int h) = 0;
    virtual void Shutdown() = 0;
};
```

Das ist die abstrakte Basis. Die eigentliche Runtime-Arbeit läuft hier über `GDXECSRenderer`.

### 6.2 `GDXECSRenderer`

```cpp
class GDXECSRenderer final : public IGDXRenderer
{
public:
    explicit GDXECSRenderer(std::unique_ptr<IGDXRenderBackend> backend);
    ~GDXECSRenderer() override;

    bool Initialize() override;
    void BeginFrame() override;
    void EndFrame() override;
    void Resize(int w, int h) override;
    void Shutdown() override;

    using TickFn = std::function<void(float)>;
    void SetTickCallback(TickFn fn);
    void Tick(float dt);

    Registry& GetRegistry();

    ShaderHandle   CreateShader(const std::wstring& vsFile,
                                const std::wstring& psFile,
                                uint32_t vertexFlags = GDX_VERTEX_DEFAULT);

    ShaderHandle   CreateShader(const std::wstring& vsFile,
                                const std::wstring& psFile,
                                uint32_t vertexFlags,
                                const GDXShaderLayout& layout);

    TextureHandle  LoadTexture(const std::wstring& filePath, bool isSRGB = true);
    TextureHandle  CreateTexture(const ImageBuffer& image,
                                 const std::wstring& debugName,
                                 bool isSRGB = true);

    MeshHandle     UploadMesh(MeshAssetResource mesh);
    MaterialHandle CreateMaterial(MaterialResource mat);

    ShaderHandle   GetDefaultShader() const;
    void SetShadowMapSize(uint32_t size);
    void SetSceneAmbient(float r, float g, float b);

    ResourceStore<MeshAssetResource, MeshTag>& GetMeshStore();
    ResourceStore<MaterialResource, MaterialTag>& GetMatStore();
    ResourceStore<GDXShaderResource, ShaderTag>& GetShaderStore();
    ResourceStore<GDXTextureResource, TextureTag>& GetTextureStore();

    RenderTargetHandle CreateRenderTarget(uint32_t w, uint32_t h,
                                          const std::wstring& name,
                                          GDXTextureFormat colorFormat = GDXTextureFormat::RGBA8_UNORM);

    TextureHandle GetRenderTargetTexture(RenderTargetHandle h);

    PostProcessHandle CreatePostProcessPass(const PostProcessPassDesc& desc);
    bool SetPostProcessConstants(PostProcessHandle h, const void* data, uint32_t size);
    bool SetPostProcessEnabled(PostProcessHandle h, bool enabled);
    void ClearPostProcessPasses();

    void SetClearColor(float r, float g, float b, float a = 1.0f);
};
```

### Kernaufgaben

`GDXECSRenderer` ist die eigentliche High-Level-Runtime für:
- ECS-Zugriff
- Ressourcen-Erzeugung
- Render-Frame-Ausführung
- Shader/Texture/Mesh/Material-Verwaltung
- RTT und Post-Processing

---

## 7. ECS verwenden

### 7.1 `Registry`

Die `Registry` ist das zentrale ECS-Objekt.

Wichtige Operationen:

```cpp
EntityID CreateEntity();
void DestroyEntity(EntityID id);
bool IsAlive(EntityID id) const;
size_t EntityCount() const;

template<typename T, typename... Args>
T& Add(EntityID id, Args&&... args);

template<typename T>
T* Get(EntityID id);

template<typename T>
bool Has(EntityID id) const;

template<typename T>
void Remove(EntityID id);

template<typename First, typename... Rest, typename Func>
void View(Func&& func);
```

### Grundmuster

```cpp
Registry& registry = renderer.GetRegistry();

EntityID entity = registry.CreateEntity();
registry.Add<TagComponent>(entity, TagComponent{"Triangle"});
```

### Komponenten lesen

```cpp
auto* transform = registry.Get<TransformComponent>(entity);
if (transform)
{
    transform->localPosition = {0.0f, 0.0f, 5.0f};
    transform->dirty = true;
}
```

### Über Entities iterieren

```cpp
registry.View<TransformComponent, RenderableComponent>(
    [&](EntityID id, TransformComponent& transform, RenderableComponent& renderable)
    {
        // alle Entities mit diesen Komponenten
    });
```

### Wichtig

Die Registry arbeitet handle- und komponentenbasiert. Render-Ressourcen selbst liegen nicht direkt in Komponenten, sondern werden über Handles referenziert.

---

## 8. Wichtige Komponenten

### `TagComponent`
Lesbarer Name einer Entity.

```cpp
struct TagComponent
{
    std::string name;
};
```

### `TransformComponent`
Lokale Transform-Daten.

```cpp
struct TransformComponent
{
    GIDX::Float3 localPosition;
    GIDX::Float4 localRotation;
    GIDX::Float3 localScale;

    bool dirty;
    uint32_t localVersion;
    uint32_t worldVersion;

    void SetEulerDeg(float pitchDeg, float yawDeg, float rollDeg);
};
```

Wichtig:
- enthält nur lokale Daten
- Weltmatrix liegt separat in `WorldTransformComponent`

### `WorldTransformComponent`
Berechnete Weltmatrix und Inverse. Wird vom TransformSystem geschrieben.

### `RenderableComponent`
Verknüpft Entity mit renderbarer Ressource.

```cpp
struct RenderableComponent
{
    MeshHandle mesh;
    MaterialHandle material;
    uint32_t submeshIndex = 0u;
    bool enabled = true;

    bool dirty = true;
    uint32_t stateVersion = 1u;
};
```

### `VisibilityComponent`
Sichtbarkeit, Aktivität, Layer und Schattenverhalten.

```cpp
struct VisibilityComponent
{
    bool visible = true;
    bool active = true;
    uint32_t layerMask = 0x00000001u;
    bool castShadows = true;
    bool receiveShadows = true;

    bool dirty = true;
    uint32_t stateVersion = 1u;
};
```

### `RenderBoundsComponent`
Bounds für Culling.

### `CameraComponent`
Projektionsparameter.

```cpp
struct CameraComponent
{
    float fovDeg = 60.0f;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;
    float aspectRatio = 16.0f / 9.0f;

    bool  isOrtho = false;
    float orthoWidth = 10.0f;
    float orthoHeight = 10.0f;

    uint32_t cullMask = 0xFFFFFFFFu;
};
```

### `ActiveCameraTag`
Markiert die aktive Kamera.

### `RenderTargetCameraComponent`
Kamera, die in ein Offscreen-Target rendert.

### `LightComponent`
Beleuchtungsdaten für Directional-, Point- und Spot-Lights.

---

## 9. Ressourcen erzeugen

### 9.1 Shader

```cpp
ShaderHandle shader = renderer.CreateShader(
    L"shader/ECSVertexShader.hlsl",
    L"shader/ECSPixelShader.hlsl",
    GDX_VERTEX_DEFAULT);
```

Oder mit explizitem Layout:

```cpp
ShaderHandle shader = renderer.CreateShader(vsFile, psFile, vertexFlags, customLayout);
```

### 9.2 Texturen

Aus Datei:

```cpp
TextureHandle tex = renderer.LoadTexture(L"assets/albedo.png", true);
```

Aus CPU-Bilddaten:

```cpp
TextureHandle tex = renderer.CreateTexture(imageBuffer, L"GeneratedTexture", true);
```

### 9.3 Meshes

```cpp
MeshAssetResource mesh = /* Meshdaten aufbauen */;
MeshHandle meshHandle = renderer.UploadMesh(std::move(mesh));
```

### 9.4 Materialien

```cpp
MaterialResource mat{};
MaterialHandle matHandle = renderer.CreateMaterial(std::move(mat));
```

---

## 10. Render Targets und Post Processing

### Render Target erzeugen

```cpp
RenderTargetHandle rt = renderer.CreateRenderTarget(
    1024,
    1024,
    L"OffscreenColor",
    GDXTextureFormat::RGBA8_UNORM);
```

### zugehörige Textur holen

```cpp
TextureHandle rtTexture = renderer.GetRenderTargetTexture(rt);
```

### Post-Process-Pass anlegen

```cpp
PostProcessPassDesc desc{};
PostProcessHandle pp = renderer.CreatePostProcessPass(desc);
```

### Konstanten setzen

```cpp
renderer.SetPostProcessConstants(pp, &myData, sizeof(myData));
```

### aktivieren/deaktivieren

```cpp
renderer.SetPostProcessEnabled(pp, true);
```

### alle Pässe löschen

```cpp
renderer.ClearPostProcessPasses();
```

---

## 11. Renderer konfigurieren

### Clear Color

```cpp
renderer.SetClearColor(0.05f, 0.05f, 0.12f, 1.0f);
```

### Scene Ambient

```cpp
renderer.SetSceneAmbient(0.08f, 0.08f, 0.12f);
```

### Shadow-Map-Größe

```cpp
renderer.SetShadowMapSize(2048);
```

---

## 12. Tick-Callback verwenden

Der Renderer kann Spiel- oder Demo-Update-Code pro Frame ausführen.

```cpp
renderer.SetTickCallback([&](float dt)
{
    // Spiel-Update, ECS-Änderungen, Animation, Kamera usw.
});
```

Die Engine ruft im Frame dann `renderer.Tick(dt)` auf.

---

## 13. Minimale Initialisierung der Engine

Dieses Beispiel zeigt den direkten Weg über Window, DX11-Context, Backend, Renderer und Engine.

```cpp
#include "GDXEngine.h"
#include "GDXECSRenderer.h"
#include "GDXWin32Window.h"
#include "GDXWin32DX11ContextFactory.h"
#include "GDXDX11RenderBackend.h"
#include "WindowDesc.h"

int main()
{
    GDXEventQueue events;

    WindowDesc desc{};
    desc.width = 1280;
    desc.height = 720;
    desc.title = "GIDX Demo";

    auto window = std::make_unique<GDXWin32Window>(desc, events);

    auto adapters = GDXWin32DX11ContextFactory::EnumerateAdapters();
    if (adapters.empty())
        return 1;

    unsigned int bestAdapter = GDXWin32DX11ContextFactory::FindBestAdapter(adapters);

    GDXWin32DX11ContextFactory factory;
    auto* nativeAccess = dynamic_cast<IGDXWin32NativeAccess*>(window.get());
    if (!nativeAccess)
        return 1;

    auto dxContext = factory.Create(*nativeAccess, bestAdapter);
    if (!dxContext)
        return 1;

    auto backend = std::make_unique<GDXDX11RenderBackend>(std::move(dxContext));
    auto renderer = std::make_unique<GDXECSRenderer>(std::move(backend));

    GDXEngine engine(std::move(window), std::move(renderer), events);

    if (!engine.Initialize())
        return 1;

    engine.Run();
    engine.Shutdown();
    return 0;
}
```

Hinweis:
Der Factory-Kommentar im Code sagt explizit, dass die DX11-Factory direkt mit `IGDXWin32NativeAccess` arbeitet und keine `dynamic_cast`-Abhängigkeit im eigentlichen Factory-Design will. Der Cast oben ist daher nur ein pragmatisches Beispiel auf Anwendungsebene.

---

## 14. Erstes Dreieck rendern

Die ehrliche Antwort ist:
Ein reines Dreieck entsteht in dieser Engine nicht über einen simplen Immediate-Mode-Draw-Call, sondern über den normalen ECS-/Mesh-/Material-Pfad. Das heißt, man muss mindestens:

1. Engine initialisieren
2. Mesh erzeugen und hochladen
3. Material erzeugen
4. Kamera-Entity anlegen
5. Renderable-Entity anlegen

### 14.1 Aktive Kamera anlegen

```cpp
Registry& registry = renderer.GetRegistry();

EntityID camera = registry.CreateEntity();
registry.Add<TagComponent>(camera, TagComponent{"MainCamera"});
registry.Add<TransformComponent>(camera, TransformComponent{0.0f, 0.0f, -3.0f});
registry.Add<WorldTransformComponent>(camera, WorldTransformComponent{});

CameraComponent cam{};
cam.fovDeg = 60.0f;
cam.nearPlane = 0.1f;
cam.farPlane = 100.0f;
cam.aspectRatio = 1280.0f / 720.0f;
registry.Add<CameraComponent>(camera, cam);
registry.Add<ActiveCameraTag>(camera, ActiveCameraTag{});
```

### 14.2 Dreiecks-Mesh erzeugen

Dafür muss ein `MeshAssetResource` mit genau einem Submesh aufgebaut werden. Die genauen Felder hängen an `MeshAssetResource` und `SubmeshData`. Das Prinzip ist:

```cpp
MeshAssetResource mesh{};

// Vertex-Daten für 3 Punkte setzen
// Index-Daten: 0, 1, 2
// passenden Vertex-Layout-/Flag-Satz wählen
// ein Submesh anlegen

MeshHandle meshHandle = renderer.UploadMesh(std::move(mesh));
```

### 14.3 Material erzeugen

```cpp
MaterialResource material{};
// Shader-/Textur-/Materialparameter setzen

MaterialHandle materialHandle = renderer.CreateMaterial(std::move(material));
```

### 14.4 Renderbare Entity anlegen

```cpp
EntityID triangle = registry.CreateEntity();
registry.Add<TagComponent>(triangle, TagComponent{"Triangle"});
registry.Add<TransformComponent>(triangle, TransformComponent{0.0f, 0.0f, 0.0f});
registry.Add<WorldTransformComponent>(triangle, WorldTransformComponent{});
registry.Add<RenderableComponent>(triangle, RenderableComponent{meshHandle, materialHandle, 0u});
registry.Add<VisibilityComponent>(triangle, VisibilityComponent{});
registry.Add<RenderBoundsComponent>(triangle,
    RenderBoundsComponent::MakeSphere({0.0f, 0.0f, 0.0f}, 1.0f));
```

### 14.5 Laufender Update-Code

```cpp
renderer.SetTickCallback([&](float dt)
{
    auto* t = registry.Get<TransformComponent>(triangle);
    if (t)
    {
        t->SetEulerDeg(0.0f, engine.GetTotalTime() * 50.0f, 0.0f);
        t->dirty = true;
    }
});
```

### Wichtig

Der Knackpunkt ist nicht die Engine-Initialisierung, sondern der konkrete Aufbau von `MeshAssetResource` und `MaterialResource`. Das Grundprinzip ist klar, aber für ein komplett kompilierbares Dreiecks-Sample muss man diese beiden Typen exakt nach ihren Feldern befüllen.

---

## 15. Typischer minimaler ECS-Renderpfad

Für eine tatsächlich sichtbare Entity braucht man in der Regel:

- `TransformComponent`
- `WorldTransformComponent`
- `RenderableComponent`
- `VisibilityComponent`
- `RenderBoundsComponent`

Für eine aktive Kamera:

- `TransformComponent`
- `WorldTransformComponent`
- `CameraComponent`
- `ActiveCameraTag`

Wenn diese Basis fehlt, rendert nichts, selbst wenn Mesh und Material korrekt existieren.

---

## 16. Was `gidx.h` anders macht

Zusätzlich zur direkten Engine-API existiert mit `gidx.h` eine vereinfachte Zugriffsschicht.

Das ist sinnvoll für:
- schnelle Demos
- Samples
- einfache Tools
- schnellen Einstieg

Das ist nicht der primäre Low-/Mid-Level-Zugriff, sondern eine Komfortschicht, die Teile der Initialisierung und Standardnutzung kapselt.

Wenn du die Engine-Struktur wirklich verstehen oder gezielt erweitern willst, solltest du mit dem direkten Weg über `GDXEngine`, Window, Backend und `GDXECSRenderer` arbeiten.

---

## 17. Zusammenfassung

Die eigentliche Engine-Nutzung läuft über:

1. Win32-Fenster erzeugen
2. DX11-Adapter enumerieren
3. DX11-Context erstellen
4. Backend erzeugen
5. `GDXECSRenderer` erzeugen
6. `GDXEngine` erzeugen und initialisieren
7. ECS-Entities, Kamera, Meshes und Materialien anlegen
8. Frame-Loop mit `Run()` oder `Step()` ausführen

Für das erste sichtbare Objekt reicht kein nackter Draw-Call. Die Engine ist klar auf ECS- und Ressourcenfluss aufgebaut. Ein erstes Dreieck ist deshalb ein normaler Mesh-/Material-/Entity-Setup-Fall und kein Spezialpfad.
