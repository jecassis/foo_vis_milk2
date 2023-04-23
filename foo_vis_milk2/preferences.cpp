//
// preferences.cpp: Configuration settings accessible through a preferences page and advanced preferences.
//

#include "pch.h"
#include "resource.h"

#include <helpers/atl-misc.h>
#include <helpers/advconfig_impl.h>
#include <sdk/cfg_var.h>
#include <helpers/DarkMode.h>

// Dark Mode:
// (1) Add fb2k::CDarkModeHooks member.
// (2) Initialize it in WM_INITDIALOG handler.
// (3) Tell foobar2000 that this preferences page supports dark mode, by returning correct get_state() flags.

static const GUID guid_cfg_bogoSetting1 = {
    0x8a6c8c08, 0xc298, 0x4485, {0xb9, 0xb5, 0xa3, 0x1d, 0xd4, 0xed, 0xfa, 0x4b}
}; // {8A6C8C08-C298-4485-B9B5-A31DD4EDFA4B}

static const GUID guid_cfg_bogoSetting2 = {
    0x96ba1b84, 0x1ff3, 0x47d3, {0x81, 0x9f, 0x35, 0xa7, 0xdb, 0xff, 0xde, 0x37}
}; // {96BA1B84-1FF3-47D3-819F-35A7DBFFDE37}

static const GUID guid_advconfig_branch = {
    0xd7ab1cd7, 0x7956, 0x4497, {0x9b, 0x1d, 0x74, 0x78, 0x7e, 0xde, 0x1d, 0xbc}
}; // {D7AB1CD7-7956-4497-9B1D-74787EDE1DBC}

static const GUID guid_cfg_bogoSetting3 = {
    0xca68c998, 0x3e53, 0x4acc, {0x98, 0xa2, 0x41, 0x46, 0x7a, 0xbc, 0x67, 0xef}
}; // {CA68C998-3E53-4ACC-98A2-41467ABC67EF}

static const GUID guid_cfg_preset_lock = {
    0xe5be745e, 0xab65, 0x4b69, {0xa1, 0xf3, 0x1e, 0xfb, 0x08, 0xff, 0x4e, 0xcf}
};

static const GUID guid_cfg_preset_shuffle = {
    0x659c6787, 0x97bb, 0x485b, {0xa0, 0xfc, 0x45, 0xfb, 0x12, 0xb7, 0x3a, 0xa0}
};

static const GUID guid_cfg_preset_name = {
    0x186c5741, 0x701e, 0x4f2c, {0xb4, 0x41, 0xe5, 0x57, 0x5c, 0x18, 0xb0, 0xa8}
};

static const GUID guid_cfg_preset_duration = {
    0x48d9b7f5, 0x4446, 0x4ab7, {0xb8, 0x71, 0xef, 0xc7, 0x59, 0x43, 0xb9, 0xcd}
};

static cfg_bool cfg_preset_lock(guid_cfg_preset_lock, false);
static cfg_bool cfg_preset_shuffle(guid_cfg_preset_shuffle, true);
static cfg_string cfg_preset_name(guid_cfg_preset_name, "");
static cfg_int cfg_preset_duration(guid_cfg_preset_duration, 20);

static bool try_fullscreen = false;

enum milk2_default_settings
{
    default_cfg_bogoSetting1 = 1337,
    default_cfg_bogoSetting2 = 666,
    default_cfg_bogoSetting3 = 42,
};

static cfg_uint cfg_bogoSetting1(guid_cfg_bogoSetting1, default_cfg_bogoSetting1),
    cfg_bogoSetting2(guid_cfg_bogoSetting2, default_cfg_bogoSetting2);

static advconfig_branch_factory g_advconfigBranch("MilkDrop", guid_advconfig_branch, advconfig_branch::guid_branch_tools, 0);
static advconfig_integer_factory cfg_bogoSetting3("Bogo setting 3", guid_cfg_bogoSetting3, guid_advconfig_branch, 0,
                                                  default_cfg_bogoSetting3, 0 /*minimum value*/, 9999 /*maximum value*/);

class milk2_preferences_page_instance : public preferences_page_instance, public CDialogImpl<milk2_preferences_page_instance>
{
  public:
    milk2_preferences_page_instance(preferences_page_callback::ptr callback) : m_callback(callback) {}

    enum milk2_dialog_id
    {
        IDD = IDD_PROPPAGE_0 // Dialog resource ID
    };
    // preferences_page_instance methods (not all of them - get_wnd() is supplied by preferences_page_impl helpers)
    t_uint32 get_state();
    void apply();
    void reset();

    //WTL message map
    BEGIN_MSG_MAP_EX(CMyPreferences)
    MSG_WM_INITDIALOG(OnInitDialog)
    COMMAND_HANDLER_EX(IDC_BETWEEN_TIME, EN_CHANGE, OnEditChange)
    COMMAND_HANDLER_EX(IDC_BLEND_AUTO, EN_CHANGE, OnEditChange)
    END_MSG_MAP()
  private:
    BOOL OnInitDialog(CWindow, LPARAM);
    void OnEditChange(UINT, int, CWindow);
    bool HasChanged();
    void OnChanged();

    const preferences_page_callback::ptr m_callback;

    // Dark mode hooks object, must be a member of dialog class.
    fb2k::CDarkModeHooks m_dark;
};

BOOL milk2_preferences_page_instance::OnInitDialog(CWindow, LPARAM)
{
    // Enable dark mode
    // One call does it all, applies all relevant hacks automatically
    m_dark.AddDialogWithControls(*this);

    SetDlgItemInt(IDC_BETWEEN_TIME, (UINT)cfg_bogoSetting1, FALSE);
    SetDlgItemInt(IDC_BLEND_AUTO, (UINT)cfg_bogoSetting2, FALSE);
    return FALSE;
}

void milk2_preferences_page_instance::OnEditChange(UINT, int, CWindow)
{
    OnChanged();
}

t_uint32 milk2_preferences_page_instance::get_state()
{
    // IMPORTANT: Always return dark_mode_supported - tell foobar2000 that this preferences page is dark mode compliant.
    t_uint32 state = preferences_state::resettable | preferences_state::dark_mode_supported;
    if (HasChanged())
        state |= preferences_state::changed;
    return state;
}

void milk2_preferences_page_instance::reset()
{
    SetDlgItemInt(IDC_BETWEEN_TIME, default_cfg_bogoSetting1, FALSE);
    SetDlgItemInt(IDC_BLEND_AUTO, default_cfg_bogoSetting2, FALSE);
    OnChanged();
}

void milk2_preferences_page_instance::apply()
{
    cfg_bogoSetting1 = GetDlgItemInt(IDC_BETWEEN_TIME, NULL, FALSE);
    cfg_bogoSetting2 = GetDlgItemInt(IDC_BLEND_AUTO, NULL, FALSE);

    OnChanged(); //our dialog content has not changed but the flags have - our currently shown values now match the settings so the apply button can be disabled
}

bool milk2_preferences_page_instance::HasChanged()
{
    //returns whether our dialog content is different from the current configuration (whether the apply button should be enabled or not)
    return GetDlgItemInt(IDC_BETWEEN_TIME, NULL, FALSE) != cfg_bogoSetting1 || GetDlgItemInt(IDC_BLEND_AUTO, NULL, FALSE) != cfg_bogoSetting2;
} 

// Tells the host that state has changed to enable or disable the "Apply" button appropriately.
void milk2_preferences_page_instance::OnChanged()
{    
    m_callback->on_state_changed();
}

class milk2_preferences_page : public preferences_page_impl<milk2_preferences_page_instance>
{
  public:
    const char* get_name() { return "MilkDrop"; }
    GUID get_guid()
    {
        static const GUID guid = {
            0x5feadec6, 0x37f3, 0x4ebf, {0xb3, 0xfd, 0x57, 0x11, 0xfa, 0xe3, 0xa3, 0xe8}
        }; // {5FEADEC6-37F3-4EBF-B3FD-5711FAE3A3E8}
        return guid;
    }
    GUID get_parent_guid() { return guid_tools; }
};

static preferences_page_factory_t<milk2_preferences_page> g_preferences_page_milk2_factory;