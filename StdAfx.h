#pragma once
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commoncontrols.h>
#include <objbase.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <shlguid.h>
#include <shlwapi.h>
#include <uxtheme.h>
#include <d3d9.h>
#include <ddraw.h>
#include <d3dx9.h>
#include <wincodec.h>
#include <atlcomcli.h>
#include <tchar.h>

#include <functional>
#include <memory>
#include <vector>
#include <iterator>

using namespace std;

#ifdef _UNICODE
typedef wstring tstring;
#else
typedef string tstring;
#endif

extern "C" IMAGE_DOS_HEADER __ImageBase;
#define THIS_HINSTANCE reinterpret_cast<HINSTANCE>(&__ImageBase)

#include "Startup.h"
#include "Deleter.h"
#include "Crest.h"
#include "ComObject.h"
#include "Window.h"