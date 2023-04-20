//
// Vis.cpp
//

#include "pch.h"
#include "vis.h"

#include "../vis_milk2/plugin.h"
#include "../vis_milk2/defines.h"

CPlugin g_plugin;

extern void ExitVis() noexcept;

//using namespace DirectX;
//using namespace DirectX::SimpleMath;

//using Microsoft::WRL::ComPtr;

Vis::Vis() noexcept(false)
{
    //m_deviceResources = std::make_unique<DX::DeviceResources>();
    //m_deviceResources->RegisterDeviceNotify(this);

    m_pwd = ".\\";
}

Vis::~Vis()
{
}

// Initialize the Direct3D resources required to run.
void Vis::Initialize(HWND window, int width, int height)
{
    //m_deviceResources->SetWindow(window, width, height);

    //m_deviceResources->CreateDeviceResources();
    //CreateDeviceDependentResources();

    //m_deviceResources->CreateWindowSizeDependentResources();
    //CreateWindowSizeDependentResources();
 
    swprintf_s(g_plugin.m_szPluginsDirPath, L"%hs" /* L"%hs\\resources\\" */, const_cast<char*>(m_pwd.c_str()));
    swprintf_s(g_plugin.m_szConfigIniFile, L"%hs%ls", const_cast<char*>(m_pwd.c_str()), INIFILE);
    g_plugin.PluginPreInitialize(window, NULL);
    g_plugin.PluginInitialize(0, 0, width, height);
}

#pragma region Frame Update
// Executes the basic game loop.
void Vis::Tick()
{
    m_timer.Tick([&]() { Update(m_timer); });

    Render();
}

// Updates the world.
void Vis::Update(DX::StepTimer const& timer)
{
    //const Vector3 eye(0.0f, 0.7f, 1.5f);
    //const Vector3 at(0.0f, -0.1f, 0.0f);

    //m_view = Matrix::CreateLookAt(eye, at, Vector3::UnitY);

    //m_world = Matrix::CreateRotationY(float(timer.GetTotalSeconds() * XM_PIDIV4));

    //m_batchEffect->SetView(m_view);
    //m_batchEffect->SetWorld(Matrix::Identity);
}
#pragma endregion

#pragma region Frame Render
// Draws the scene.
void Vis::Render()
{
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

    Clear();

    g_plugin.PluginRender(waves[0], waves[1]); //Render();

    //m_deviceResources->PIXBeginEvent(L"Render");
    //auto context = m_deviceResources->GetD3DDeviceContext();

    //// Draw procedurally generated dynamic grid
    //const XMVECTORF32 xaxis = {20.f, 0.f, 0.f};
    //const XMVECTORF32 yaxis = {0.f, 0.f, 20.f};
    //DrawGrid(xaxis, yaxis, g_XMZero, 20, 20, Colors::Gray);

    //// Draw sprite
    //m_deviceResources->PIXBeginEvent(L"Draw sprite");
    //m_sprites->Begin();
    //m_sprites->Draw(m_texture2.Get(), XMFLOAT2(10, 75), nullptr, Colors::White);

    //m_font->DrawString(m_sprites.get(), L"DirectXTK Simple Sample", XMFLOAT2(100, 10), Colors::Yellow);
    //m_sprites->End();
    //m_deviceResources->PIXEndEvent();

    //// Draw 3D object
    //m_deviceResources->PIXBeginEvent(L"Draw teapot");
    //XMMATRIX local = m_world * Matrix::CreateTranslation(-2.f, -2.f, -4.f);
    //m_shape->Draw(local, m_view, m_projection, Colors::White, m_texture1.Get());
    //m_deviceResources->PIXEndEvent();

    //m_deviceResources->PIXBeginEvent(L"Draw model");
    //const XMVECTORF32 scale = {0.01f, 0.01f, 0.01f};
    //const XMVECTORF32 translate = {3.f, -2.f, -4.f};
    //const XMVECTOR rotate = Quaternion::CreateFromYawPitchRoll(XM_PI / 2.f, 0.f, -XM_PI / 2.f);
    //local = m_world * XMMatrixTransformation(g_XMZero, Quaternion::Identity, scale, g_XMZero, rotate, translate);
    //m_model->Draw(context, *m_states, local, m_view, m_projection);
    //m_deviceResources->PIXEndEvent();

    //m_deviceResources->PIXEndEvent();

    //// Show the new frame.
    //m_deviceResources->Present();
}

// Helper method to clear the back buffers.
void Vis::Clear()
{
    //m_deviceResources->PIXBeginEvent(L"Clear");

    //// Clear the views.
    //auto context = m_deviceResources->GetD3DDeviceContext();
    //auto renderTarget = m_deviceResources->GetRenderTargetView();
    //auto depthStencil = m_deviceResources->GetDepthStencilView();

    //context->ClearRenderTargetView(renderTarget, Colors::CornflowerBlue);
    //context->ClearDepthStencilView(depthStencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    //context->OMSetRenderTargets(1, &renderTarget, depthStencil);

    //// Set the viewport.
    //auto const viewport = m_deviceResources->GetScreenViewport();
    //context->RSSetViewports(1, &viewport);

    //m_deviceResources->PIXEndEvent();
}

void XM_CALLCONV Vis::DrawGrid(FXMVECTOR xAxis, FXMVECTOR yAxis, FXMVECTOR origin, size_t xdivs, size_t ydivs, GXMVECTOR color)
{
    //m_deviceResources->PIXBeginEvent(L"Draw grid");

    //auto context = m_deviceResources->GetD3DDeviceContext();
    //context->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
    //context->OMSetDepthStencilState(m_states->DepthNone(), 0);
    //context->RSSetState(m_states->CullCounterClockwise());

    //m_batchEffect->Apply(context);

    //context->IASetInputLayout(m_batchInputLayout.Get());

    //m_batch->Begin();

    //xdivs = std::max<size_t>(1, xdivs);
    //ydivs = std::max<size_t>(1, ydivs);

    //for (size_t i = 0; i <= xdivs; ++i)
    //{
    //    float fPercent = float(i) / float(xdivs);
    //    fPercent = (fPercent * 2.0f) - 1.0f;
    //    XMVECTOR vScale = XMVectorScale(xAxis, fPercent);
    //    vScale = XMVectorAdd(vScale, origin);

    //    const VertexPositionColor v1(XMVectorSubtract(vScale, yAxis), color);
    //    const VertexPositionColor v2(XMVectorAdd(vScale, yAxis), color);
    //    m_batch->DrawLine(v1, v2);
    //}

    //for (size_t i = 0; i <= ydivs; i++)
    //{
    //    float fPercent = float(i) / float(ydivs);
    //    fPercent = (fPercent * 2.0f) - 1.0f;
    //    XMVECTOR vScale = XMVectorScale(yAxis, fPercent);
    //    vScale = XMVectorAdd(vScale, origin);

    //    const VertexPositionColor v1(XMVectorSubtract(vScale, xAxis), color);
    //    const VertexPositionColor v2(XMVectorAdd(vScale, xAxis), color);
    //    m_batch->DrawLine(v1, v2);
    //}

    //m_batch->End();

    //m_deviceResources->PIXEndEvent();
}
#pragma endregion

#pragma region Message Handlers
// Message handlers
void Vis::OnActivated()
{
}

void Vis::OnDeactivated()
{
}

void Vis::OnSuspending()
{
}

void Vis::OnResuming()
{
    m_timer.ResetElapsedTime();
}

void Vis::OnWindowMoved()
{
    //auto const r = m_deviceResources->GetOutputSize();
    //m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}

void Vis::OnDisplayChange()
{
    //m_deviceResources->UpdateColorSpace();
}

void Vis::OnWindowSizeChanged(int width, int height)
{
    //if (!m_deviceResources->WindowSizeChanged(width, height))
    //    return;

    CreateWindowSizeDependentResources();
}

// Properties
void Vis::GetDefaultSize(int& width, int& height) const noexcept
{
    width = 800;
    height = 600;
}

void Vis::SetPwd(std::string pwd) noexcept
{
    m_pwd.assign(pwd);
    //#define BUFSIZE MAX_PATH
    //
    //wchar_t Buffer[BUFSIZE];
    //DWORD dwRet;
    //if (argc != 2)
    //{
    //    _tprintf(TEXT("Usage: %s <dir>\n"), argv[0]);
    //    return;
    //}
    //
    //dwRet = GetCurrentDirectory(BUFSIZE, Buffer);
    //
    //if (dwRet == 0)
    //{
    //    printf("GetCurrentDirectory failed (%d)\n", GetLastError());
    //    return;
    //}
    //if (dwRet > BUFSIZE)
    //{
    //    printf("Buffer too small; need %d characters\n", dwRet);
    //    return;
    //}
    //
    //if (!SetCurrentDirectory(argv[1]))
    //{
    //    printf("SetCurrentDirectory failed (%d)\n", GetLastError());
    //    return;
    //}
    //_tprintf(TEXT("Set current directory to %s\n"), argv[1]);
    //
    //if (!SetCurrentDirectory(Buffer))
    //{
    //    printf("SetCurrentDirectory failed (%d)\n", GetLastError());
    //    return;
    //}
    //_tprintf(TEXT("Restored previous directory (%s)\n"), Buffer);
}
#pragma endregion

//HRESULT Vis::Render()
//{
//    //g_plugin.PluginRender(waves[0], waves[1]);
//    return S_OK;
//}

bool Vis::NextPreset()
{
    //g_plugin.NextPreset(1.0f);
    return true;
}

bool Vis::PrevPreset()
{
    //g_plugin.PrevPreset(1.0f);
    return true;
}

bool Vis::LoadPreset(int select)
{
    //g_plugin.m_nCurrentPreset = select + g_plugin.m_nDirs;

    //wchar_t szFile[MAX_PATH] = {0};
    //wcscpy_s(szFile, g_plugin.m_szPresetDir); // Note: m_szPresetDir always ends with '\'
    //wcscat_s(szFile, g_plugin.m_presets[g_plugin.m_nCurrentPreset].szFilename.c_str());

    //g_plugin.LoadPreset(szFile, 1.0f);
    return true;
}

bool Vis::LockPreset(bool lockUnlock)
{
    //g_plugin.m_bPresetLockedByUser = lockUnlock;
    return true;
}

bool Vis::RandomPreset()
{
    //g_plugin.LoadRandomPreset(1.0f);
    return true;
}

bool Vis::GetPresets(std::vector<std::string>& presets)
{
    //if (!m_IsInitialized)
    //    return false;

    //while (!g_plugin.m_bPresetListReady) {}

    //for (int i = 0; i < g_plugin.m_nPresets - g_plugin.m_nDirs; ++i)
    //{
    //    PresetInfo& Info = g_plugin.m_presets[i + g_plugin.m_nDirs];
    //    presets.push_back(WideToUTF8(Info.szFilename.c_str()));
    //}

    return true;
}

int Vis::GetActivePreset()
{
    //if (m_IsInitialized)
    //{
    //    int CurrentPreset = g_plugin.m_nCurrentPreset;
    //    CurrentPreset -= g_plugin.m_nDirs;
    //    return CurrentPreset;
    //}

    return -1;
}

char* Vis::WideToUTF8(const wchar_t* WFilename)
{
    int SizeNeeded = WideCharToMultiByte(CP_UTF8, 0, &WFilename[0], -1, NULL, 0, NULL, NULL);
    char* utf8Name = new char[SizeNeeded];
    WideCharToMultiByte(CP_UTF8, 0, &WFilename[0], -1, &utf8Name[0], SizeNeeded, NULL, NULL);
    return utf8Name;
}

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Vis::CreateDeviceDependentResources()
{
    //auto context = m_deviceResources->GetD3DDeviceContext();
    //auto device = m_deviceResources->GetD3DDevice();

    //m_states = std::make_unique<CommonStates>(device);

    //m_fxFactory = std::make_unique<EffectFactory>(device);
    //m_fxFactory->SetDirectory(std::wstring(m_pwd.begin(), m_pwd.end()).c_str());

    //m_sprites = std::make_unique<SpriteBatch>(context);

    //m_batch = std::make_unique<PrimitiveBatch<VertexPositionColor>>(context);

    //m_batchEffect = std::make_unique<BasicEffect>(device);
    //m_batchEffect->SetVertexColorEnabled(true);

    //DX::ThrowIfFailed(CreateInputLayoutFromEffect<VertexPositionColor>(device, m_batchEffect.get(), m_batchInputLayout.ReleaseAndGetAddressOf()));

    //std::string spritefont = m_pwd + "SegoeUI_18.spritefont";
    //std::string sdkmesh = m_pwd + "tiny.sdkmesh";
    //std::string dds1 = m_pwd + "seafloor.dds";
    //std::string dds2 = m_pwd + "windowslogo.dds";

    //m_font = std::make_unique<SpriteFont>(device, std::wstring(spritefont.begin(), spritefont.end()).c_str());

    //m_shape = GeometricPrimitive::CreateTeapot(context, 4.f, 8);

    //// SDKMESH has to use clockwise winding with right-handed coordinates, so textures are flipped in U
    //m_model = Model::CreateFromSDKMESH(device, std::wstring(sdkmesh.begin(), sdkmesh.end()).c_str(), *m_fxFactory);

    //// Load textures
    //DX::ThrowIfFailed(CreateDDSTextureFromFile(device, std::wstring(dds1.begin(), dds1.end()).c_str(), nullptr, m_texture1.ReleaseAndGetAddressOf()));

    //DX::ThrowIfFailed(CreateDDSTextureFromFile(device, std::wstring(dds2.begin(), dds2.end()).c_str(), nullptr,
    //                                           m_texture2.ReleaseAndGetAddressOf()));
}

// Allocate all memory resources that change on a window SizeChanged event.
void Vis::CreateWindowSizeDependentResources()
{
    //auto const size = m_deviceResources->GetOutputSize();
    //const float aspectRatio = float(size.right) / float(size.bottom);
    //float fovAngleY = 70.0f * XM_PI / 180.0f;

    //// This is a simple example of change that can be made when the app is in
    //// portrait or snapped view.
    //if (aspectRatio < 1.0f)
    //{
    //    fovAngleY *= 2.0f;
    //}

    //// This sample makes use of a right-handed coordinate system using row-major matrices.
    //m_projection = Matrix::CreatePerspectiveFieldOfView(fovAngleY, aspectRatio, 0.01f, 100.0f);

    //m_batchEffect->SetProjection(m_projection);
}

void Vis::OnDeviceLost()
{
    g_plugin.PluginQuit();
    //m_states.reset();
    //m_fxFactory.reset();
    //m_sprites.reset();
    //m_batch.reset();
    //m_batchEffect.reset();
    //m_font.reset();
    //m_shape.reset();
    //m_model.reset();
    //m_texture1.Reset();
    //m_texture2.Reset();
    //m_batchInputLayout.Reset();
}

void Vis::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion
