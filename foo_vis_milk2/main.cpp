/*
 *  main.cpp - Sets the MilkDrop 2 visualization component's file name and version.
 */

#include "pch.h"
#include "version.h"

#ifdef _DEBUG
#define APPLICATION_SOURCE_URLS \
    " * " PLUGIN_WEB_URL "\n" \
    " * " APPLICATION_SOURCE_URL
#else
#define APPLICATION_SOURCE_URLS " * " PLUGIN_WEB_URL
#endif

// clang-format off
DECLARE_COMPONENT_VERSION("MilkDrop 2 Visualisation", APPLICATION_VERSION,
                          "MilkDrop 2 music visualizer ported from Winamp.\n\n"
                          APPLICATION_FILE_NAME " " APPLICATION_VERSION "\n"
                          "Copyright (c) " APPLICATION_COPYRIGHT ". All rights reserved.\n\n"
                          "MilkDrop 2.25k\n"
                          "Copyright (c) 2005-2013 Nullsoft, Inc. All rights reserved.\n\n"
                          "foobar2000 plugin and DirectX 11 port by " APPLICATION_COMPANY_NAME ".\n"
                          "Built with foobar2000 SDK " STR(FOOBAR2000_SDK_VERSION) " on " __DATE__ " " __TIME__ " -0800.\n\n"
                          "Documentation and source code can be obtained from:\n"
                          APPLICATION_SOURCE_URLS)
// clang-format on

VALIDATE_COMPONENT_FILENAME(APPLICATION_FILE_NAME _APPLICATION_VERSION_EXTENSION)