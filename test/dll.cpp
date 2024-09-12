/*
 * dll.cpp - Tests for foobar2000 component's exported interface.
 *
 * Copyright (c) 2023-2024 Jimmy Cassis
 * SPDX-License-Identifier: MPL-2.0
 */

#include "pch.h"
#include <CppUnitTest.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace MilkDrop2
{
BEGIN_TEST_MODULE_ATTRIBUTE()
TEST_MODULE_ATTRIBUTE(L"Date", L"2024")
END_TEST_MODULE_ATTRIBUTE()

TEST_MODULE_INITIALIZE(Milk2Initialize)
{
    Logger::WriteMessage("MilkDrop 2 Test Module Initialize\n");
}

TEST_MODULE_CLEANUP(Milk2Cleanup)
{
    Logger::WriteMessage("MilkDrop 2 Test Module Cleanup\n");
}

TEST_CLASS(Milk2DllTest)
{
  private:
    //static ServiceManager* service;

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            //case WM_WA_IPC:
            //    {
            //        switch (lParam)
            //        {
            //            case IPC_GET_API_SERVICE:
            //                return reinterpret_cast<LRESULT>(service);
            //        }
            //    }
            //    break;
            case WM_DESTROY:
                //if (service)
                //{
                //    delete service;
                //}
                PostQuitMessage(0);
                break;
            default:
                return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }
        return NULL;
    }

  public:
    Milk2DllTest()
    {
        Logger::WriteMessage("Milk2DllTest()\n");
    }

    ~Milk2DllTest()
    {
        Logger::WriteMessage("~Milk2DllTest()");
    }

    TEST_CLASS_INITIALIZE(Milk2DllTestInitialize)
    {
        Logger::WriteMessage("MilkDrop 2 DLL Test Initialize\n");
    }

    TEST_CLASS_CLEANUP(Milk2DllTestCleanup)
    {
        Logger::WriteMessage("MilkDrop 2 DLL Test Cleanup\n");
    }

    BEGIN_TEST_METHOD_ATTRIBUTE(SmokeTest)
        TEST_OWNER(L"foo_vis_milk2")
        TEST_PRIORITY(1)
    END_TEST_METHOD_ATTRIBUTE()

    TEST_METHOD(SmokeTest)
    {
        Logger::WriteMessage("Smoke DLL Load Test\n");

        static const GUID guid_ui_element_milk2 = { 0x0B52C703, 0x1586, 0x42F7, {0xA8, 0x4C, 0x70, 0x54, 0xCD, 0xC8, 0x22, 0x55} }; // {0B52C703-1586-42F7-A84C-7054CDC82255} foo_vis_milk2.dll!void(* service_factory_single_t<ui_element_milk2>::`vftable'[2])()
        static const GUID guid_milk2 = { 0x204b0345, 0x4df5, 0x4b47, {0xad, 0xd3, 0x98, 0x9f, 0x81, 0x1b, 0xd9, 0xa5} }; // {204B0345-4DF5-4B47-ADD3-989F811BD9A5}
        static const GUID preferences_page_guid = { 0x7255e8d0, 0x3fcf, 0x4781, {0xb9, 0x3b, 0xd0, 0x6c, 0xb8, 0x8d, 0xfa, 0xfa} }; // {7255E8D0-3FCF-4781-B93B-D06CB88DFAFA} foo_vis_milk2.dll!void(* preferences_page_factory_t<preferences_page_milk2>::`vftable'[2])()
        static const GUID initquit_guid = { 0x113773c4, 0xb387, 0x4b48, {0x8b, 0xdf, 0xab, 0x58, 0xbc, 0x6c, 0xe5, 0x38} }; // {113773C4-B387-4b48-8BDF-AB58BC6CE538}
        static const GUID advconfig_entry_guid = { 0x7e84602e, 0xdc49, 0x4047, {0xaa, 0xee, 0x63, 0x71, 0x8b, 0xbc, 0x5a, 0x1f} }; // {7E84602E-DC49-4047-AAEE-63718BBC5A1F}
        static const GUID componentversion_guid = { 0x10bb3ebd, 0xddf7, 0x4975, {0xa3, 0xcc, 0x23, 0x8, 0x48, 0x29, 0x45, 0x3e} }; // {10BB3EBD-DDF7-4975-A3CC-23084829453E}

        static const wchar_t* class_name = L"Messages";
        WNDCLASSEX wx = {0};
        wx.cbSize = sizeof(WNDCLASSEX);
        wx.lpfnWndProc = WindowProc;
        wx.hInstance = GetModuleHandle(NULL);
        wx.lpszClassName = class_name;
        Assert::IsNotNull(wx.hInstance);

        g_foobar2000_api = new foobar2000_api_impl();

        RegisterClassEx(&wx);
        HWND hWnd = CreateWindowEx(0, class_name, L"Message Window", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);
        Assert::IsNotNull(hWnd);

        foobar2000_client* component = foobar2000_get_interface(g_foobar2000_api, wx.hInstance);
        Assert::IsNotNull(component);

        uint32_t ver = component->get_version();
        service_factory_base* serv = component->get_service_list();
        stream_writer* ws = nullptr;
        stream_reader* rs = nullptr;
        component->get_config(ws, fb2k::noAbort);
        component->set_config(rs, fb2k::noAbort);
        component->set_library_path("path", "test");
        component->services_init(true);
        bool vis = serv->is_service_present(guid_milk2);
        bool prefs = serv->is_service_present(preferences_page_guid);
        bool iq = serv->is_service_present(initquit_guid);
        bool debug = component->is_debug();

        Assert::IsTrue(vis, L"Visualization UI element service not installed.");
        Assert::IsTrue(prefs, L"Preferences page not installed.");
        Assert::IsTrue(iq, L"InitQuit not installed.");
        Assert::AreEqual(static_cast<uint32_t>(FOOBAR2000_TARGET_VERSION), ver, L"foobar2000 component client version mismatches");
#ifdef _DEBUG
        Assert::IsTrue(debug, L"foobar2000 component client is not in debug mode.");
#else
        Assert::IsFalse(debug, L"foobar2000 component client is in debug mode.");
#endif

        //MSG msg = {};
        //while (GetMessage(&msg, NULL, 0, 0))
        //{
        //    TranslateMessage(&msg);
        //    DispatchMessage(&msg);
        //}

        hWnd = NULL;
    }

    TEST_METHOD(PassTest)
    {
        Logger::WriteMessage("Pass Test\n");
        Assert::AreEqual(0, 0, L"Pass test failed!");
    }

    TEST_METHOD_INITIALIZE(MethodInit)
    {
        Logger::WriteMessage("Test Method Initialize\n");
    }

    TEST_METHOD_CLEANUP(MethodCleanup)
    {
        Logger::WriteMessage("Test Method Cleanup\n");
    }

    //TEST_METHOD(FailTest)
    //{
    //    Assert::Fail(L"Fail Test\n");
    //}
};
}