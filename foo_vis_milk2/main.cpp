/*
 * main.cpp - Sets the MilkDrop 2 visualization component's file name
 *            and version.
 *
 * Copyright (c) 2023-2024 Jimmy Cassis
 * SPDX-License-Identifier: MPL-2.0
 */

#include "pch.h"
#include "version.h"

#define APPLICATION_INFORMATION_URLS \
    APPLICATION_FILE_NAME " releases and documentation can be found here:\n" \
    " * " APPLICATION_RELEASE_URL "\n" \
    " * " APPLICATION_DOCUMENTATION_URL "\n" \
    " * " APPLICATION_DISCUSSION_URL "\n\n"

#define APPLICATION_SOURCE_URLS \
    APPLICATION_FILE_NAME " is released under the Mozilla Public License (MPL) version 2.0. Component source code can be obtained from:\n" \
    " * " APPLICATION_SOURCE_URL " (Mozilla Public License 2.0, BSD 3-Clause License, MIT License)\n" \
    " * " PLUGIN_WEB_URL " (Nullsoft BSD License)\n" \
    " * https://www.foobar2000.org/SDK (foobar2000 SDK License, zlib License)\n" \
    " * https://wtl.sourceforge.io/ (Microsoft Public License)\n" \
    " * https://www.cockos.com/EEL2/ (zlib License)\n" \
    " * https://github.com/projectM-visualizer/projectm-eval (MIT License)\n" \
    " * https://github.com/microsoft/DirectXTK (MIT License)\n" \
    " * https://wtl.sourceforge.io/ (Microsoft Public License)\n" \
    " * https://learn.microsoft.com/en-us/cpp/atl/atl-com-desktop-components (Proprietary License)\n" \
    " * https://developer.microsoft.com/en-us/windows/downloads/windows-sdk/ (Proprietary License)"

// clang-format off
DECLARE_COMPONENT_VERSION("MilkDrop 2 Visualisation", APPLICATION_VERSION,
                          "MilkDrop 2 music visualizer ported from Winamp.\n\n"
                          APPLICATION_FILE_NAME " " APPLICATION_VERSION "\n"
                          "Copyright (c) " APPLICATION_COPYRIGHT ". All rights reserved.\n\n"
                          "MilkDrop 2.25k\n"
                          "Copyright (c) 2005-2013 Nullsoft, Inc.\n"
                          "Copyright (c) 2021-" APPLICATION_COPYRIGHT ".\nAll rights reserved.\n\n"
                          "foobar2000 component and DirectX 11 port by " APPLICATION_COMPANY_NAME ".\n"
                          "Built with foobar2000 SDK " STR(FOOBAR2000_SDK_VERSION) " on " __DATE__ " " __TIME__ " " STR(BUILD_TIMEZONE) ".\n\n"
                          APPLICATION_INFORMATION_URLS
                          APPLICATION_SOURCE_URLS)
// clang-format on

VALIDATE_COMPONENT_FILENAME(APPLICATION_FILE_NAME _APPLICATION_VERSION_EXTENSION)