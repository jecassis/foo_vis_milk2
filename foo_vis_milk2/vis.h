//
// vis.h
//

#pragma once

#include "steptimer.h"

// Provides a visualization loop and communication between foobar2000 and MilkDrop.
class Vis final
{
  public:
    Vis() noexcept(false);
    ~Vis();

    Vis(Vis&&) = default;
    Vis& operator=(Vis&&) = default;

    Vis(Vis const&) = delete;
    Vis& operator=(Vis const&) = delete;

    // Initialization and management
    bool Initialize(HWND window, int width, int height);

    // Basic visualization loop
    void Tick();

    // Device status
    void OnDeviceLost();
    void OnDeviceRestored();

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

    // Window Messages
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

    // MilkDrop status
    bool m_milk2 = false;

    // Rendering loop timer
    DX::StepTimer m_timer;

    // Component paths
    std::string m_pwd;
};
