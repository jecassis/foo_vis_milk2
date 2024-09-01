/*
  LICENSE
  -------
  Copyright 2005-2013 Nullsoft, Inc.
  Copyright 2021-2024 Jimmy Cassis
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

#ifndef __NULLSOFT_API_H__
#define __NULLSOFT_API_H__

//#include <Wasabi/api/service/api_service.h>
//#include <Wasabi/api/service/waservicefactory.h>
//#include <Agave/language/api_language.h>
//#include <Wasabi/api/syscb/api_syscb.h>
//extern api_syscb* WASABI_API_SYSCB;
//#define WASABI_API_SYSCB sysCallbackApi
//#include <Wasabi/api/application/api_application.h>
//extern api_application* applicationApi;
//#define WASABI_API_APP applicationApi

#define WASABI_API_LNG_HINST NULL
#ifndef WASABI_API_ORIG_HINST
#define WASABI_API_ORIG_HINST GetInstance()
#endif
#define WASABI_API_LNGSTRW GetStringW
#define WASABI_API_LNGSTRINGW(uID) WASABI_API_LNGSTRW(WASABI_API_LNG_HINST, WASABI_API_ORIG_HINST, uID)
#define WASABI_API_LNGSTRINGW_HINST(hinst, uID) WASABI_API_LNGSTRW(WASABI_API_LNG_HINST, hinst, uID)
#define WASABI_API_LNGSTRINGW_BUF(uID, buf, len) WASABI_API_LNGSTRW(WASABI_API_LNG_HINST, WASABI_API_ORIG_HINST, uID, buf, len)
#define WASABI_API_LNGSTRINGW_BUF_HINST(hinst, uID, buf, len) WASABI_API_LNGSTRW(WASABI_API_LNG_HINST, hinst, uID, buf, len)

#define WASABI_API_DLGBOXPARAMW LDialogBoxParamW
#define WASABI_API_DIALOGBOXW(id, parent, proc) WASABI_API_DLGBOXPARAMW(WASABI_API_LNG_HINST, WASABI_API_ORIG_HINST, id, parent, (DLGPROC)proc, 0)
#define WASABI_API_DIALOGBOXPARAMW(id, parent, proc, param) WASABI_API_DLGBOXPARAMW(WASABI_API_LNG_HINST, WASABI_API_ORIG_HINST, id, parent, (DLGPROC)proc, param)
#endif