//
// vis.cpp
//

#include "pch.h"
#include "vis.h"

#include "../vis_milk2/plugin.h"
#include "../vis_milk2/defines.h"

CPlugin g_plugin;

extern void ExitVis() noexcept;

Vis::Vis() noexcept(false)
{
    m_pwd = ".\\";
}

Vis::~Vis()
{
}

// Initialize the Direct3D resources required to run.
bool Vis::Initialize(HWND window, int width, int height)
{ 
    swprintf_s(g_plugin.m_szPluginsDirPath, L"%hs", const_cast<char*>(m_pwd.c_str()));
    swprintf_s(g_plugin.m_szConfigIniFile, L"%hs%ls", const_cast<char*>(m_pwd.c_str()), INIFILE);

    if (FALSE == g_plugin.PluginPreInitialize(window, NULL))
        return false;

    if (FALSE == g_plugin.PluginInitialize(0, 0, width, height, static_cast<float>(width) / static_cast<float>(height)))
        return false;

    m_milk2 = true;
    return true;
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
}
#pragma endregion

#pragma region Frame Render
// Draws the scene.
void Vis::Render()
{
    // Do not try to render anything before the first `Update()`.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

    Clear();

    g_plugin.PluginRender(waves[0], waves[1]); //Render();
}

// Helper method to clear the back buffers.
void Vis::Clear()
{
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
}

void Vis::OnDisplayChange()
{
}

void Vis::OnWindowSizeChanged(int width, int height)
{
     g_plugin.OnUserResizeWindow();

    CreateWindowSizeDependentResources();
}

// Properties
void Vis::GetDefaultSize(int& width, int& height) const noexcept
{
    width = 640;
    height = 480;
}

void Vis::SetPwd(std::string pwd) noexcept
{
    m_pwd.assign(pwd);
}
#pragma endregion

bool Vis::NextPreset()
{
    g_plugin.NextPreset(1.0f);
    return true;
}

bool Vis::PrevPreset()
{
    g_plugin.PrevPreset(1.0f);
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
    g_plugin.m_bPresetLockedByUser = lockUnlock;
    return true;
}

bool Vis::RandomPreset()
{
    g_plugin.LoadRandomPreset(1.0f);
    return true;
}

bool Vis::GetPresets(std::vector<std::string>& presets)
{
    if (!m_milk2)
        return false;

    while (!g_plugin.m_bPresetListReady) {}

    for (int i = 0; i < g_plugin.m_nPresets - g_plugin.m_nDirs; ++i)
    {
        PresetInfo& Info = g_plugin.m_presets[i + g_plugin.m_nDirs];
        presets.push_back(WideToUTF8(Info.szFilename.c_str()));
    }

    return true;
}

int Vis::GetActivePreset()
{
    //if (m_milk2)
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
}

// Allocate all memory resources that change on a window SizeChanged event.
void Vis::CreateWindowSizeDependentResources()
{
}

void Vis::OnDeviceLost()
{
    g_plugin.PluginQuit();
}

void Vis::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion
