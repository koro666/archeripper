#include "StdAfx.h"

#pragma comment(linker, "\"/manifestdependency:type='Win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

extern "C" int WINAPI _tWinMain(HINSTANCE, HINSTANCE, LPTSTR, int)
{
	SetDllDirectory(TEXT(""));

	static const INITCOMMONCONTROLSEX icc = { sizeof(INITCOMMONCONTROLSEX), ICC_LISTVIEW_CLASSES };
	InitCommonControlsEx(&icc);

	if (SUCCEEDED(OleInitialize(NULL)))
	{
		if (Window::Register())
		{
			{
				Window wnd;

				wnd.Create();
				wnd.Enumerate();

				MSG m;
				while (GetMessage(&m, NULL, 0, 0))
				{
					TranslateMessage(&m);
					DispatchMessage(&m);
				}

				wnd.Destroy();
			}

			Window::Unregister();
		}
		
		OleUninitialize();
	}

	return 0;
}