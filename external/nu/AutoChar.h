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

#ifndef NULLSOFT_UTILITY_AUTOCHAR_H
#define NULLSOFT_UTILITY_AUTOCHAR_H

#include <minwindef.h>
#include <stringapiset.h>
#include <shlwapi.h>

#ifndef FILENAME_SIZE
#define FILENAME_SIZE (MAX_PATH * 4)
#define REMOVE_FILENAME_SIZE
#endif

class AutoChar
{
  public:
    explicit AutoChar(const wchar_t* filename)
    {
        out[0] = '\0';

        if (!filename)
            return;

        if (PathIsURL(filename))
        {
            WideCharToMultiByte(CP_ACP, 0, filename, -1, out, FILENAME_SIZE, NULL, NULL);
            return;
        }

        BOOL notConvertible = FALSE;
        WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, filename, -1, out, FILENAME_SIZE, NULL, &notConvertible);

        if (notConvertible)
        {
            wchar_t temp[MAX_PATH] = {0};
            if (GetShortPathName(filename, temp, MAX_PATH))
                WideCharToMultiByte(CP_ACP, 0, temp, -1, out, FILENAME_SIZE, NULL, NULL);
        }
    }

    operator const char*() const { return out; }

  private:
    char out[FILENAME_SIZE];
};

#ifdef REMOVE_FILENAME_SIZE
#undef FILENAME_SIZE
#endif

#endif