/*
  LICENSE
  -------
  Copyright 2005-2013 Nullsoft, Inc.
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    * Neither the name of Nullsoft nor the names of its contributors may be used to
      endorse or promote products derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
  FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
  OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
        if (size > 0 && (wide = static_cast<wchar_t*>(malloc(size << 1))) != NULL)
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