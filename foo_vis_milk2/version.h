//
// version.h: Defines common version information for the DLL's version resource
//            and the component version shown in foobar2000.
//            Note: Newline at EOF required for resource compiler (RC1004).
//

#pragma once

#include <vis_milk2/defines.h>

// Component filename (string literal)
#define COMPONENT_FILENAME "foo_vis_milk2.dll"

// Component name (string literal)
#define COMPONENT_NAME SHORTNAME

// Component author
#define COMPONENT_AUTHOR "KernelOops"

// The parts of the component version number (integer literals)
#define COMPONENT_VERSION_MAJOR 0
#define COMPONENT_VERSION_MINOR 0
#define COMPONENT_VERSION_PATCH INT_VERSION
#define COMPONENT_VERSION_PRE "alpha"

#define LIBRARY_VERSION_MAJOR 2 // INT_VERSION / 100
#define LIBRARY_VERSION_MINOR 25 // INT_VERSION % 100
#define LIBRARY_VERSION_PATCH INT_SUBVERSION

// Year for copyright notice (string literal)
#define COMPONENT_COPYRIGHT_YEAR "2023"

// Helper macros for converting integers to string literals and concatenating them
#define MAKE_STRING(text) #text
#ifndef COMPONENT_VERSION_PRE
#define COMPONENT_VERSION_PRE ""
#define MAKE_COMPONENT_VERSION(major, minor, patch, pre) MAKE_STRING(major) "." MAKE_STRING(minor) "." MAKE_STRING(patch)
#else
#define MAKE_COMPONENT_VERSION(major, minor, patch, pre) MAKE_STRING(major) "." MAKE_STRING(minor) "." MAKE_STRING(patch) "-" COMPONENT_VERSION_PRE
#endif
#define MAKE_LIBRARY_VERSION(major, minor, patch) MAKE_STRING(major) "." MAKE_STRING(minor) "." MAKE_STRING(patch)

// Assemble the component version as string and as comma-separated list of integers
#define COMPONENT_VERSION MAKE_COMPONENT_VERSION(COMPONENT_VERSION_MAJOR, COMPONENT_VERSION_MINOR, COMPONENT_VERSION_PATCH, COMPONENT_VERSION_PRE)
#define COMPONENT_VERSION_NUMERIC COMPONENT_VERSION_MAJOR, COMPONENT_VERSION_MINOR, COMPONENT_VERSION_PATCH, 0
#define LIBRARY_VERSION MAKE_LIBRARY_VERSION(LIBRARY_VERSION_MAJOR, LIBRARY_VERSION_MINOR, LIBRARY_VERSION_PATCH)
#define LIBRARY_VERSION_NUMERIC LIBRARY_VERSION_MAJOR, LIBRARY_VERSION_MINOR, LIBRARY_VERSION_PATCH, 0
