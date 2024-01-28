//
// Game.h
//

#pragma once
#include <DirectXColors.h>
#include <DirectXMath.h>
#include "../external/directxtk_desktop_2019.2024.1.1.1/include/CommonStates.h"
#include "../external/directxtk_desktop_2019.2024.1.1.1/include/DDSTextureLoader.h"
#include "../external/directxtk_desktop_2019.2024.1.1.1/include/DirectXHelpers.h"
#include "../external/directxtk_desktop_2019.2024.1.1.1/include/Effects.h"
#include "../external/directxtk_desktop_2019.2024.1.1.1/include/GeometricPrimitive.h"
#include "../external/directxtk_desktop_2019.2024.1.1.1/include/Model.h"
#include "../external/directxtk_desktop_2019.2024.1.1.1/include/PrimitiveBatch.h"
#include "../external/directxtk_desktop_2019.2024.1.1.1/include/SimpleMath.h"
#include "../external/directxtk_desktop_2019.2024.1.1.1/include/SpriteBatch.h"
#include "../external/directxtk_desktop_2019.2024.1.1.1/include/SpriteFont.h"
#include "../external/directxtk_desktop_2019.2024.1.1.1/include/VertexTypes.h"
#include <vis_milk2/deviceresources.h>
#include "steptimer.h"

namespace DX
{
// Helper class for COM exceptions.
class com_exception : public std::exception
{
  public:
    com_exception(HRESULT hr) noexcept : result(hr) {}

    const char* what() const override
    {
        static char s_str[64] = {};
        sprintf_s(s_str, "Failure with HRESULT of %08X", static_cast<unsigned int>(result));
        return s_str;
    }

  private:
    HRESULT result;
};

// Helper utility converts D3D API failures into exceptions.
inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        throw com_exception(hr);
    }
}
} // namespace DX

// A basic game implementation that creates a D3D11 device and
// provides a game loop.
class Game final : public DX::IDeviceNotify
{
  public:
    Game() noexcept(false);
    ~Game();

    Game(Game&&) = default;
    Game& operator=(Game&&) = default;

    Game(Game const&) = delete;
    Game& operator=(Game const&) = delete;

    // Initialization and management
    void Initialize(HWND window, int width, int height);
    void SetWindow(HWND window, int width, int height);

    // Basic game loop
    void Tick();

    // IDeviceNotify
    void OnDeviceLost() override;
    void OnDeviceRestored() override;

    // Messages
    void OnActivated();
    void OnDeactivated();
    void OnSuspending();
    void OnResuming();
    void OnWindowMoved();
    void OnDisplayChange();
    void OnWindowSizeChanged(int width, int height);
    //void NewAudioDevice();

    // Properties
    void GetDefaultSize(int& width, int& height) const noexcept;
    void SetPwd(std::string pwd) noexcept;

  private:
    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    void XM_CALLCONV DrawGrid(DirectX::FXMVECTOR xAxis, DirectX::FXMVECTOR yAxis, DirectX::FXMVECTOR origin, size_t xdivs, size_t ydivs, DirectX::GXMVECTOR color);

    // Device resources.
    std::unique_ptr<DX::DeviceResources> m_deviceResources;

    // Rendering loop timer.
    DX::StepTimer m_timer;

    // Component paths.
    std::string m_pwd;

    //// Input devices.
    //std::unique_ptr<DirectX::GamePad> m_gamePad;
    //std::unique_ptr<DirectX::Keyboard> m_keyboard;
    //std::unique_ptr<DirectX::Mouse> m_mouse;

    // DirectXTK objects.
    std::unique_ptr<DirectX::CommonStates> m_states;
    std::unique_ptr<DirectX::BasicEffect> m_batchEffect;
    std::unique_ptr<DirectX::EffectFactory> m_fxFactory;
    std::unique_ptr<DirectX::GeometricPrimitive> m_shape;
    std::unique_ptr<DirectX::Model> m_model;
    std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>> m_batch;
    std::unique_ptr<DirectX::SpriteBatch> m_sprites;
    std::unique_ptr<DirectX::SpriteFont> m_font;

    //// DirectXTK for Audio objects.
    //std::unique_ptr<DirectX::AudioEngine> m_audEngine;
    //std::unique_ptr<DirectX::WaveBank> m_waveBank;
    //std::unique_ptr<DirectX::SoundEffect> m_soundEffect;
    //std::unique_ptr<DirectX::SoundEffectInstance> m_effect1;
    //std::unique_ptr<DirectX::SoundEffectInstance> m_effect2;

    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_texture1;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_texture2;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> m_batchInputLayout;

    //uint32_t m_audioEvent;
    //float m_audioTimerAcc;

    //bool m_retryDefault;

    DirectX::SimpleMath::Matrix m_world;
    DirectX::SimpleMath::Matrix m_view;
    DirectX::SimpleMath::Matrix m_projection;
};