/*
  Copyright 1997-2013 Nullsoft, Inc.
  Copyright 2021-2024 Jimmy Cassis

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
 */

#ifndef NULLSOFT_UTILITY_AUTOWIDE_H
#define NULLSOFT_UTILITY_AUTOWIDE_H

#include <minwindef.h>
#include <stringapiset.h>

inline wchar_t* AutoWideDup(const char* convert, UINT codePage = CP_ACP)
{
    wchar_t* wide = nullptr;
    if (convert)
    {
        const int size = MultiByteToWideChar(codePage, 0, convert, -1, NULL, 0);
        if (size > 0 && (wide = static_cast<wchar_t*>(malloc(static_cast<size_t>(size) << 1))) != NULL)
        {
            if (!MultiByteToWideChar(codePage, 0, convert, -1, wide, size))
            {
                free(wide);
                wide = nullptr;
            }
            else
            {
                wide[size - 1] = L'\0';
            }
        }
    }

    return wide;
}

class AutoWide
{
  public:
    AutoWide(const char* convert, UINT codePage = CP_ACP) { wide = AutoWideDup(convert, codePage); }

    ~AutoWide()
    {
        free(wide);
        wide = nullptr;
    }

    operator wchar_t*() const { return wide; }

  private:
    wchar_t* wide;
};

#endif