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