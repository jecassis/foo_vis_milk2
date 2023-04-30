#include "pch.h"
#include "version.h"

#ifdef _DEBUG
#define APPLICATION_SOURCE_URLS \
    " * " PLUGIN_WEB_URL "\n" \
    " * " APPLICATION_SOURCE_URL
#else
#define APPLICATION_SOURCE_URLS " * " PLUGIN_WEB_URL
#endif

DECLARE_COMPONENT_VERSION("MilkDrop 2 Visualisation", APPLICATION_VERSION,
                          "MilkDrop 2 music visualizer ported from Winamp.\n\n"
                          "MilkDrop 2 is Copyright (c) 2005-2013 Nullsoft, Inc.\n"
                          "foobar2000 plugin and DirectX 11 port by " APPLICATION_COMPANY_NAME "\n"
                          "Source code can be obtained from:\n"
                          APPLICATION_SOURCE_URLS);

VALIDATE_COMPONENT_FILENAME(APPLICATION_FILE_NAME _APPLICATION_VERSION_EXTENSION);