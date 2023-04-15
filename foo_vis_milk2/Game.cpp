//
// Game.cpp
//

#include "pch.h"
#include "Game.h"

extern void ExitGame() noexcept;

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

Game::Game() noexcept(false)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>();
    m_deviceResources->RegisterDeviceNotify(this);

    m_pwd = ".\\";
}

Game::~Game()
{
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(HWND window, int width, int height)
{
    m_deviceResources->SetWindow(window, width, height);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();
}

#pragma region Frame Update
// Executes the basic game loop.
void Game::Tick()
{
    m_timer.Tick([&]() { Update(m_timer); });

    Render();
}

// Updates the world.
void Game::Update(DX::StepTimer const& timer)
{
    const Vector3 eye(0.0f, 0.7f, 1.5f);
    const Vector3 at(0.0f, -0.1f, 0.0f);

    m_view = Matrix::CreateLookAt(eye, at, Vector3::UnitY);

    m_world = Matrix::CreateRotationY(float(timer.GetTotalSeconds() * XM_PIDIV4));

    m_batchEffect->SetView(m_view);
    m_batchEffect->SetWorld(Matrix::Identity);
}
#pragma endregion

#pragma region Frame Render
// Draws the scene.
void Game::Render()
{
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

    Clear();

    m_deviceResources->PIXBeginEvent(L"Render");
    auto context = m_deviceResources->GetD3DDeviceContext();

    // Draw procedurally generated dynamic grid
    const XMVECTORF32 xaxis = {20.f, 0.f, 0.f};
    const XMVECTORF32 yaxis = {0.f, 0.f, 20.f};
    DrawGrid(xaxis, yaxis, g_XMZero, 20, 20, Colors::Gray);

    // Draw sprite
    m_deviceResources->PIXBeginEvent(L"Draw sprite");
    m_sprites->Begin();
    m_sprites->Draw(m_texture2.Get(), XMFLOAT2(10, 75), nullptr, Colors::White);

    m_font->DrawString(m_sprites.get(), L"DirectXTK Simple Sample", XMFLOAT2(100, 10), Colors::Yellow);
    m_sprites->End();
    m_deviceResources->PIXEndEvent();

    // Draw 3D object
    m_deviceResources->PIXBeginEvent(L"Draw teapot");
    XMMATRIX local = m_world * Matrix::CreateTranslation(-2.f, -2.f, -4.f);
    m_shape->Draw(local, m_view, m_projection, Colors::White, m_texture1.Get());
    m_deviceResources->PIXEndEvent();

    m_deviceResources->PIXBeginEvent(L"Draw model");
    const XMVECTORF32 scale = {0.01f, 0.01f, 0.01f};
    const XMVECTORF32 translate = {3.f, -2.f, -4.f};
    const XMVECTOR rotate = Quaternion::CreateFromYawPitchRoll(XM_PI / 2.f, 0.f, -XM_PI / 2.f);
    local = m_world * XMMatrixTransformation(g_XMZero, Quaternion::Identity, scale, g_XMZero, rotate, translate);
    m_model->Draw(context, *m_states, local, m_view, m_projection);
    m_deviceResources->PIXEndEvent();

    m_deviceResources->PIXEndEvent();

    // Show the new frame.
    m_deviceResources->Present();
}

// Helper method to clear the back buffers.
void Game::Clear()
{
    m_deviceResources->PIXBeginEvent(L"Clear");

    // Clear the views.
    auto context = m_deviceResources->GetD3DDeviceContext();
    auto renderTarget = m_deviceResources->GetRenderTargetView();
    auto depthStencil = m_deviceResources->GetDepthStencilView();

    context->ClearRenderTargetView(renderTarget, Colors::CornflowerBlue);
    context->ClearDepthStencilView(depthStencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    context->OMSetRenderTargets(1, &renderTarget, depthStencil);

    // Set the viewport.
    auto const viewport = m_deviceResources->GetScreenViewport();
    context->RSSetViewports(1, &viewport);

    m_deviceResources->PIXEndEvent();
}

void XM_CALLCONV Game::DrawGrid(FXMVECTOR xAxis, FXMVECTOR yAxis, FXMVECTOR origin, size_t xdivs, size_t ydivs, GXMVECTOR color)
{
    m_deviceResources->PIXBeginEvent(L"Draw grid");

    auto context = m_deviceResources->GetD3DDeviceContext();
    context->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
    context->OMSetDepthStencilState(m_states->DepthNone(), 0);
    context->RSSetState(m_states->CullCounterClockwise());

    m_batchEffect->Apply(context);

    context->IASetInputLayout(m_batchInputLayout.Get());

    m_batch->Begin();

    xdivs = std::max<size_t>(1, xdivs);
    ydivs = std::max<size_t>(1, ydivs);

    for (size_t i = 0; i <= xdivs; ++i)
    {
        float fPercent = float(i) / float(xdivs);
        fPercent = (fPercent * 2.0f) - 1.0f;
        XMVECTOR vScale = XMVectorScale(xAxis, fPercent);
        vScale = XMVectorAdd(vScale, origin);

        const VertexPositionColor v1(XMVectorSubtract(vScale, yAxis), color);
        const VertexPositionColor v2(XMVectorAdd(vScale, yAxis), color);
        m_batch->DrawLine(v1, v2);
    }

    for (size_t i = 0; i <= ydivs; i++)
    {
        float fPercent = float(i) / float(ydivs);
        fPercent = (fPercent * 2.0f) - 1.0f;
        XMVECTOR vScale = XMVectorScale(yAxis, fPercent);
        vScale = XMVectorAdd(vScale, origin);

        const VertexPositionColor v1(XMVectorSubtract(vScale, xAxis), color);
        const VertexPositionColor v2(XMVectorAdd(vScale, xAxis), color);
        m_batch->DrawLine(v1, v2);
    }

    m_batch->End();

    m_deviceResources->PIXEndEvent();
}
#pragma endregion

#pragma region Message Handlers
// Message handlers
void Game::OnActivated()
{
}

void Game::OnDeactivated()
{
}

void Game::OnSuspending()
{
}

void Game::OnResuming()
{
    m_timer.ResetElapsedTime();
}

void Game::OnWindowMoved()
{
    auto const r = m_deviceResources->GetOutputSize();
    m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}

void Game::OnDisplayChange()
{
    m_deviceResources->UpdateColorSpace();
}

void Game::OnWindowSizeChanged(int width, int height)
{
    if (!m_deviceResources->WindowSizeChanged(width, height))
        return;

    CreateWindowSizeDependentResources();
}

// Properties
void Game::GetDefaultSize(int& width, int& height) const noexcept
{
    width = 128;
    height = 128;
}

void Game::SetPwd(std::string pwd) noexcept
{
    m_pwd.assign(pwd);
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Game::CreateDeviceDependentResources()
{
    auto context = m_deviceResources->GetD3DDeviceContext();
    auto device = m_deviceResources->GetD3DDevice();

    m_states = std::make_unique<CommonStates>(device);

    m_fxFactory = std::make_unique<EffectFactory>(device);
    m_fxFactory->SetDirectory(std::wstring(m_pwd.begin(), m_pwd.end()).c_str());

    m_sprites = std::make_unique<SpriteBatch>(context);

    m_batch = std::make_unique<PrimitiveBatch<VertexPositionColor>>(context);

    m_batchEffect = std::make_unique<BasicEffect>(device);
    m_batchEffect->SetVertexColorEnabled(true);

    DX::ThrowIfFailed(CreateInputLayoutFromEffect<VertexPositionColor>(device, m_batchEffect.get(),
                                                                       m_batchInputLayout.ReleaseAndGetAddressOf()));

    std::string spritefont = m_pwd + "SegoeUI_18.spritefont";
    std::string sdkmesh = m_pwd + "tiny.sdkmesh";
    std::string dds1 = m_pwd + "seafloor.dds";
    std::string dds2 = m_pwd + "windowslogo.dds";

    m_font = std::make_unique<SpriteFont>(device, std::wstring(spritefont.begin(), spritefont.end()).c_str());

    m_shape = GeometricPrimitive::CreateTeapot(context, 4.f, 8);

    // SDKMESH has to use clockwise winding with right-handed coordinates, so textures are flipped in U
    m_model = Model::CreateFromSDKMESH(device, std::wstring(sdkmesh.begin(), sdkmesh.end()).c_str(), *m_fxFactory);

    // Load textures
    DX::ThrowIfFailed(CreateDDSTextureFromFile(device, std::wstring(dds1.begin(), dds1.end()).c_str(), nullptr,
                                               m_texture1.ReleaseAndGetAddressOf()));

    DX::ThrowIfFailed(CreateDDSTextureFromFile(device, std::wstring(dds2.begin(), dds2.end()).c_str(), nullptr,
                                               m_texture2.ReleaseAndGetAddressOf()));
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    auto const size = m_deviceResources->GetOutputSize();
    const float aspectRatio = float(size.right) / float(size.bottom);
    float fovAngleY = 70.0f * XM_PI / 180.0f;

    // This is a simple example of change that can be made when the app is in
    // portrait or snapped view.
    if (aspectRatio < 1.0f)
    {
        fovAngleY *= 2.0f;
    }

    // This sample makes use of a right-handed coordinate system using row-major matrices.
    m_projection = Matrix::CreatePerspectiveFieldOfView(fovAngleY, aspectRatio, 0.01f, 100.0f);

    m_batchEffect->SetProjection(m_projection);
}

void Game::OnDeviceLost()
{
    m_states.reset();
    m_fxFactory.reset();
    m_sprites.reset();
    m_batch.reset();
    m_batchEffect.reset();
    m_font.reset();
    m_shape.reset();
    m_model.reset();
    m_texture1.Reset();
    m_texture2.Reset();
    m_batchInputLayout.Reset();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion
