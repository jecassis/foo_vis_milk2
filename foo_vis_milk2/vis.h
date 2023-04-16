//
// Vis.h
//

#pragma once

//#include "DeviceResources.h"
#include "StepTimer.h"

// Creates a D3D11 device and provides a visualization loop.
class Vis final // : public DX::IDeviceNotify
{
  public:
    Vis() noexcept(false);
    ~Vis();

    Vis(Vis&&) = default;
    Vis& operator=(Vis&&) = default;

    Vis(Vis const&) = delete;
    Vis& operator=(Vis const&) = delete;

    // Initialization and management
    void Initialize(HWND window, int width, int height);

    // Basic visualization loop
    void Tick();

    // IDeviceNotify
    void OnDeviceLost();//override;
    void OnDeviceRestored();//override;

    //HRESULT Render();
    HRESULT RenderChunk(const audio_chunk& chunk);
    bool PrevPreset();
    bool NextPreset();
    bool LoadPreset(int select);
    bool RandomPreset();
    bool LockPreset(bool lockUnlock);
    //bool IsLocked() { return g_plugin.m_bPresetLockedByUser; }
    bool GetPresets(std::vector<std::string>& presets);
    int GetActivePreset();
    char* WideToUTF8(const wchar_t* WFilename);

    // Messages
    void OnActivated();
    void OnDeactivated();
    void OnSuspending();
    void OnResuming();
    void OnWindowMoved();
    void OnDisplayChange();
    void OnWindowSizeChanged(int width, int height);

    // Properties
    void GetDefaultSize(int& width, int& height) const noexcept;
    void SetPwd(std::string pwd) noexcept;
    
    unsigned char waves[2][576];

  private:
    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    void XM_CALLCONV DrawGrid(DirectX::FXMVECTOR xAxis, DirectX::FXMVECTOR yAxis, DirectX::FXMVECTOR origin,
                              size_t xdivs, size_t ydivs, DirectX::GXMVECTOR color);

    // Device resources
    //std::unique_ptr<DX::DeviceResources> m_deviceResources;

    // Rendering loop timer
    DX::StepTimer m_timer;

    // Component paths
    std::string m_pwd;

    // DirectXTK objects
    std::unique_ptr<DirectX::CommonStates> m_states;
    std::unique_ptr<DirectX::BasicEffect> m_batchEffect;
    std::unique_ptr<DirectX::EffectFactory> m_fxFactory;
    std::unique_ptr<DirectX::GeometricPrimitive> m_shape;
    std::unique_ptr<DirectX::Model> m_model;
    std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>> m_batch;
    std::unique_ptr<DirectX::SpriteBatch> m_sprites;
    std::unique_ptr<DirectX::SpriteFont> m_font;

    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_texture1;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_texture2;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> m_batchInputLayout;

    DirectX::SimpleMath::Matrix m_world;
    DirectX::SimpleMath::Matrix m_view;
    DirectX::SimpleMath::Matrix m_projection;
};
