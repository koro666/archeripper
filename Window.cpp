#include "StdAfx.h"

const TCHAR Window::CLASSNAME[] = TEXT("ArcheRipperWndClass");

const COLORREF Window::BGCOLOR = RGB(232, 232, 216);
const COLORREF Window::FGCOLOR = RGB(104, 64, 16);

HBRUSH Window::s_brush;
ATOM Window::s_atom;

Window::Window()
:m_hWnd(NULL),
m_hCtlLv(NULL)
{
}

bool Window::Register()
{
	s_brush = CreateSolidBrush(BGCOLOR);

	const WNDCLASSEX wcx =
	{
		sizeof(WNDCLASSEX),
		CS_HREDRAW|CS_VREDRAW,
		StaticWindowProc,
		0, sizeof(Window*),
		THIS_HINSTANCE,
		LoadIcon(THIS_HINSTANCE, MAKEINTRESOURCE(1)),
		LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW)),
		s_brush,
		NULL,
		CLASSNAME,
		reinterpret_cast<HICON>(LoadImage(THIS_HINSTANCE, MAKEINTRESOURCE(1), IMAGE_ICON,
			GetSystemMetrics(SM_CXSMICON),
			GetSystemMetrics(SM_CYSMICON),
			LR_SHARED))
	};

	s_atom = RegisterClassEx(&wcx);

	return !!s_atom;
}

void Window::Unregister()
{
	UnregisterClass(MAKEINTATOM(s_atom), THIS_HINSTANCE);
	DeleteObject(s_brush);
}

bool Window::Create()
{
	if (CreateWindowEx(
		0,
		CLASSNAME,
		TEXT("Archeage Crest Ripper"),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		NULL,
		NULL,
		THIS_HINSTANCE,
		this))
	{
		ShowWindow(m_hWnd, SW_SHOWDEFAULT);
		UpdateWindow(m_hWnd);
		return true;
	}

	return false;
}

void Window::Destroy()
{
	DestroyWindow(m_hWnd);
}

void Window::Enumerate()
{
	auto loader = make_shared<CrestLoader>();
	if (FAILED(loader->Initialize()))
		return;

	m_crests = Crest::Enumerate(loader);
	if (!m_crests.empty() &&
		SUCCEEDED(m_iml->SetImageCount(m_crests.size())))
		SendMessage(m_hCtlLv, LVM_SETITEMCOUNT, m_crests.size(), 0);
}

LRESULT CALLBACK Window::StaticWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	Window* pwnd;

	if (uMsg == WM_NCCREATE)
	{
		pwnd = reinterpret_cast<Window*>(reinterpret_cast<CREATESTRUCT*>(lParam)->lpCreateParams);
		if (pwnd)
		{
			pwnd->m_hWnd = hWnd;
			SetWindowLongPtr(hWnd, 0, reinterpret_cast<LONG_PTR>(pwnd));
		}
	}
	else
		pwnd = reinterpret_cast<Window*>(GetWindowLongPtr(hWnd, 0));

	LRESULT result;

	if (pwnd)
		result = pwnd->WindowProc(uMsg, wParam, lParam);
	else
		result = DefWindowProc(hWnd, uMsg, wParam, lParam);

	if (uMsg == WM_NCDESTROY)
	{
		pwnd->m_hWnd = NULL;
		SetWindowLongPtr(hWnd, 0, 0);
	}

	return result;
}

LRESULT Window::WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_CREATE:
			return OnCreate(reinterpret_cast<CREATESTRUCT*>(lParam)) ? 0 : -1;
		case WM_SIZE:
			OnSize();
			return 0;
		case WM_ACTIVATE:
			OnActivate(LOWORD(wParam));
			return 0;
		case WM_CLOSE:
			OnClose();
			return 0;
		case WM_NOTIFY:
			OnNotify(reinterpret_cast<NMHDR*>(lParam));
			return 0;
		case WM_ENSURELOADED:
			OnEnsureCrestLoaded(reinterpret_cast<Crest*>(lParam));
			return 0;
		default:
			return DefWindowProc(m_hWnd, uMsg, wParam, lParam);
	}
}

bool Window::OnCreate(CREATESTRUCT* cs)
{
	m_hCtlLv = CreateWindowEx(
		WS_EX_NOPARENTNOTIFY,
		WC_LISTVIEW,
		NULL,
		WS_CHILD|WS_VISIBLE|WS_VSCROLL|LVS_OWNERDATA|LVS_AUTOARRANGE|LVS_SHAREIMAGELISTS,
		0, 0, 0, 0,
		m_hWnd,
		NULL,
		THIS_HINSTANCE,
		NULL);

	if (!m_hCtlLv)
		return false;

	DWORD exStyle = LVS_EX_DOUBLEBUFFER|LVS_EX_HIDELABELS;
	SendMessage(m_hCtlLv, LVM_SETEXTENDEDLISTVIEWSTYLE, exStyle, exStyle);

	ImageList_CoCreateInstance(CLSID_ImageList, NULL, IID_IImageList2, reinterpret_cast<void**>(&m_iml));

	int dpiX, dpiY;
	HDC hDC = GetDC(m_hWnd);
	dpiX = GetDeviceCaps(hDC, LOGPIXELSX);
	dpiY = GetDeviceCaps(hDC, LOGPIXELSX);
	ReleaseDC(m_hWnd, hDC);

	int imgWidth = MulDiv(128, dpiX, 96),
		imgHeight = MulDiv(128, dpiY, 96);
	
	m_iml->Initialize(imgWidth, imgWidth, ILC_COLOR32, 0, 0);
	SendMessage(m_hCtlLv, LVM_SETIMAGELIST, LVSIL_NORMAL, reinterpret_cast<LPARAM>(m_iml.p));

	int itemWidth = MulDiv(imgWidth, 9, 8),
		itemHeight = MulDiv(imgHeight, 9, 8);

	SendMessage(m_hCtlLv, LVM_SETICONSPACING, 0, MAKELPARAM(itemWidth, itemHeight));

	SendMessage(m_hCtlLv, LVM_SETBKCOLOR, 0, BGCOLOR);
	SendMessage(m_hCtlLv, LVM_SETTEXTBKCOLOR, 0, BGCOLOR);
	SendMessage(m_hCtlLv, LVM_SETTEXTCOLOR, 0, FGCOLOR);

	SetWindowTheme(m_hCtlLv, L"Explorer", NULL);
	SetFocus(m_hCtlLv);
	return true;
}

void Window::OnSize()
{
	RECT r;
	GetClientRect(m_hWnd, &r);

	HDWP hDwp = BeginDeferWindowPos(1);
	hDwp = DeferWindowPos(hDwp, m_hCtlLv, NULL, 0, 0, r.right, r.bottom, SWP_NOZORDER|SWP_NOACTIVATE);
	EndDeferWindowPos(hDwp);
}

void Window::OnActivate(UINT state)
{
	if (state && m_hCtlLv)
		SetFocus(m_hCtlLv);
}

void Window::OnClose()
{
	PostQuitMessage(0);
}

void Window::OnNotify(NMHDR* nmh)
{
	if (nmh->hwndFrom == m_hCtlLv)
	{
		switch (nmh->code)
		{
			case NM_DBLCLK:
				OnLvDblClk(reinterpret_cast<NMITEMACTIVATE*>(nmh));
				break;
			case LVN_GETDISPINFO:
				OnLvGetDispInfo(reinterpret_cast<NMLVDISPINFO*>(nmh));
				break;
			case LVN_ODCACHEHINT:
				OnLvCacheHint(reinterpret_cast<NMLVCACHEHINT*>(nmh));
				break;
			case LVN_BEGINDRAG:
				OnLvBeginDrag(reinterpret_cast<NMLISTVIEW*>(nmh));
				break;
		}
	}
}

void Window::OnLvDblClk(NMITEMACTIVATE* nmia)
{
	HRESULT hr = EnsureCrestLoaded(nmia->iItem);
	if (FAILED(hr))
		return;

	// Create and setup Save As dialog
	CComPtr<IFileSaveDialog> dlg;
	hr = CoCreateInstance(
		CLSID_FileSaveDialog,
		NULL, CLSCTX_INPROC,
		IID_IFileDialog,
		reinterpret_cast<void**>(&dlg));
	if (FAILED(hr))
		return;

	static const COMDLG_FILTERSPEC filter[] = { L"PNG image", L"*.png" };
	hr = dlg->SetFileTypes(_countof(filter), filter);
	if (FAILED(hr))
		return;

	hr = dlg->SetFileName(m_crests[nmia->iItem]->GetName().c_str());
	if (FAILED(hr))
		return;

	hr = dlg->SetDefaultExtension(L"png");
	if (FAILED(hr))
		return;

	// Show dialog
	hr = dlg->Show(m_hWnd);
	if (FAILED(hr))
		return;

	// Get the selected item
	CComPtr<IShellItem> psi;
	hr = dlg->GetResult(&psi);
	if (FAILED(hr))
		return;

	// Create a write stream for the selected item
	CComPtr<IBindCtx> ctx;
	hr = CreateBindCtx(0, &ctx);
	if (FAILED(hr))
		return;

	BIND_OPTS bo =
	{ 
		sizeof(BIND_OPTS), 0,
		STGM_WRITE|STGM_SHARE_EXCLUSIVE|STGM_CREATE
	};
	hr = ctx->SetBindOptions(&bo);
	if (FAILED(hr))
		return;

	CComPtr<IStream> strm;
	hr = psi->BindToHandler(
		ctx, BHID_Stream,
		IID_IStream, reinterpret_cast<void**>(&strm));
	if (FAILED(hr))
		return;

	// Save crest in stream
	hr = m_crests[nmia->iItem]->Save(strm);
	if (FAILED(hr))
	{
		TCHAR buffer[64];
		_sntprintf_s(buffer, _countof(buffer), _TRUNCATE, _T("Saving crest failed.\r\n\r\nError 0x%08X."), hr);
		MessageBox(m_hWnd, buffer, NULL, MB_ICONSTOP);
	}
}

void Window::OnLvGetDispInfo(NMLVDISPINFO* nmdi)
{
	LVITEM* lvi = &nmdi->item;
	if (lvi->iSubItem)
		return;

	if (lvi->mask & LVIF_IMAGE)
	{
		EnsureCrestLoaded(lvi->iItem);
		lvi->iImage = lvi->iItem;
	}
}

void Window::OnLvCacheHint(NMLVCACHEHINT* nmch)
{
	for (int i = nmch->iFrom; i <= nmch->iTo; ++i)
		EnsureCrestLoaded(i);
}

void Window::OnLvBeginDrag(NMLISTVIEW* nmlv)
{
	int count = SendMessage(m_hCtlLv, LVM_GETSELECTEDCOUNT, 0, 0);
	if (!count)
		return;

	vector<CrestPtr> selected;
	selected.reserve(count);

	LVITEMINDEX lvii = { -1, 0 };
	while (SendMessage(m_hCtlLv, LVM_GETNEXTITEMINDEX, reinterpret_cast<WPARAM>(&lvii), LVNI_SELECTED))
		selected.push_back(m_crests[lvii.iItem]);

	CComPtr<IDropSource> dropSrc(new DropSource());
	CComPtr<IDataObject> dataObj(new DataObject(m_hWnd, move(selected)));

#if 0 // Doesn't work because our IDataObject does not implement SetData
	{
		CComPtr<IDragSourceHelper> dragHlp;
		HRESULT hr = CoCreateInstance(
			CLSID_DragDropHelper,
			NULL, CLSCTX_ALL,
			IID_IDragSourceHelper,
			reinterpret_cast<void**>(&dragHlp));

		if (SUCCEEDED(hr))
			dragHlp->InitializeFromWindow(m_hCtlLv, &nmlv->ptAction, dataObj);
	}
#endif

	DWORD effect;
	DoDragDrop(dataObj, dropSrc, DROPEFFECT_COPY, &effect);
}

void Window::OnEnsureCrestLoaded(Crest* crest)
{
	vector<CrestPtr>::iterator it;

	for (it = m_crests.begin(); it != m_crests.end(); ++it)
	{
		if (it->get() == crest)
			break;
	}

	if (it == m_crests.end())
		return;

	EnsureCrestLoaded(distance(m_crests.begin(), it));
}

HRESULT Window::EnsureCrestLoaded(int i)
{
	if (i >= m_crests.size())
		return E_INVALIDARG;

	auto& crest = m_crests[i];
	if (!crest->CanLoad())
		return S_FALSE;

	HCURSOR hCsrOld = SetCursor(
		LoadCursor(NULL, MAKEINTRESOURCE(IDC_APPSTARTING)));

#ifdef _DEBUG
	TCHAR buffer[64];
	_sntprintf_s(buffer, _countof(buffer), _TRUNCATE, _T("Loading Crest #%d.\r\n"), i);
	OutputDebugString(buffer);
#endif

	HRESULT hr = crest->Load();
	if (SUCCEEDED(hr))
		crest->GetImage(m_iml, i);

	crest->Disconnect();
	SetCursor(hCsrOld);

	return hr;
}

STDMETHODIMP Window::DropSource::QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState)
{
	if (fEscapePressed)
		return DRAGDROP_S_CANCEL;

	if (!(grfKeyState & (MK_LBUTTON|MK_RBUTTON)))
		return DRAGDROP_S_DROP;

	return S_OK;
}

STDMETHODIMP Window::DropSource::GiveFeedback(DWORD dwEffect)
{
	return DRAGDROP_S_USEDEFAULTCURSORS;
}

Window::DataObject::DataObject(HWND hWnd, vector<CrestPtr>&& crests)
:m_hWnd(hWnd),
m_crests(crests)
{
	memset(&m_fe, 0, sizeof(m_fe));

	m_fe[0].cfFormat = RegisterClipboardFormat(CFSTR_FILEDESCRIPTOR);
	m_fe[0].dwAspect = DVASPECT_CONTENT;
	m_fe[0].lindex = -1;
	m_fe[0].tymed = TYMED_HGLOBAL;

	m_fe[1].cfFormat = RegisterClipboardFormat(CFSTR_FILECONTENTS);
	m_fe[1].dwAspect = DVASPECT_CONTENT;
	m_fe[1].tymed = TYMED_ISTREAM;
}

int Window::DataObject::GetDataIndex(const FORMATETC* fe) const
{
	for (int i = 0; i < _countof(m_fe); ++i)
	{
		const FORMATETC* dfe = m_fe + i;

		if (fe->cfFormat == dfe->cfFormat &&
			fe->tymed & dfe->tymed &&
			fe->dwAspect == dfe->dwAspect &&
			(dfe->lindex == -1 ?
				(fe->lindex == -1) :
				(fe->lindex >= 0 && fe->lindex < m_crests.size())))
			return i;
	}

	return -1;
}

STDMETHODIMP Window::DataObject::GetData(FORMATETC* pformatetcIn, STGMEDIUM* pmedium)
{
	memset(pmedium, 0, sizeof(STGMEDIUM));
	int index = GetDataIndex(pformatetcIn);
	if (index == -1)
		return DV_E_FORMATETC;

	switch (index)
	{
		case 0:
		{
			pmedium->tymed = TYMED_HGLOBAL;
			pmedium->hGlobal = GlobalAlloc(
				GMEM_ZEROINIT,
				FIELD_OFFSET(FILEGROUPDESCRIPTOR, fgd) + 
				(m_crests.size() * sizeof(FILEDESCRIPTOR)));

			auto gdesc = reinterpret_cast<FILEGROUPDESCRIPTOR*>(GlobalLock(pmedium->hGlobal));
			gdesc->cItems = m_crests.size();

			for (size_t i = 0; i < m_crests.size(); ++i)
			{
				auto fdesc = gdesc->fgd + i;
				auto& crest = m_crests[i];

				fdesc->dwFlags = FD_PROGRESSUI;
#ifdef UNICODE
				fdesc->dwFlags |= FD_UNICODE;
#endif

				_sntprintf_s(fdesc->cFileName,
					MAX_PATH, _TRUNCATE,
					_T("%s.png"),
					crest->GetName().c_str());
			}

			GlobalUnlock(pmedium->hGlobal);
			return S_OK;
		}
		case 1:
		{
			if (pformatetcIn->lindex < 0 || pformatetcIn->lindex >= m_crests.size())
				return E_INVALIDARG;

			CComPtr<IStream> pstm(SHCreateMemStream(NULL, 0));
			if (!pstm)
				return E_OUTOFMEMORY;

			auto& crest = m_crests[pformatetcIn->lindex];
			SendMessage(m_hWnd, WM_ENSURELOADED, 0, reinterpret_cast<LPARAM>(crest.get()));

			HRESULT hr = crest->Save(pstm);
			if (FAILED(hr))
				return hr;

			pmedium->tymed = TYMED_ISTREAM;
			pmedium->pstm = pstm.Detach();

			return S_OK;
		}
		default:
			return E_FAIL;
	}
}

STDMETHODIMP Window::DataObject::GetDataHere(FORMATETC* pformatetc, STGMEDIUM* pmedium)
{
	return E_NOTIMPL;
}

STDMETHODIMP Window::DataObject::QueryGetData(FORMATETC* pformatetc)
{
	return GetDataIndex(pformatetc) == -1 ? S_FALSE : S_OK;
}

STDMETHODIMP Window::DataObject::GetCanonicalFormatEtc(FORMATETC* pformatectIn, FORMATETC* pformatetcOut)
{
	*pformatetcOut = *pformatectIn;
	pformatetcOut->ptd = NULL;
	return DATA_S_SAMEFORMATETC;
}

STDMETHODIMP Window::DataObject::SetData(FORMATETC* pformatetc, STGMEDIUM* pmedium, BOOL fRelease)
{
	return E_NOTIMPL;
}

STDMETHODIMP Window::DataObject::EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC** ppenumFormatEtc)
{
	if (dwDirection == DATADIR_GET)
		return SHCreateStdEnumFmtEtc(_countof(m_fe), m_fe, ppenumFormatEtc);
	else
	{
		*ppenumFormatEtc = NULL;
		return E_NOTIMPL;
	}
}

STDMETHODIMP Window::DataObject::DAdvise(FORMATETC* pformatetc, DWORD advf, IAdviseSink* pAdvSink, DWORD* pdwConnection)
{
	*pdwConnection = 0;
	return OLE_E_ADVISENOTSUPPORTED;
}

STDMETHODIMP Window::DataObject::DUnadvise(DWORD dwConnection)
{
	return OLE_E_ADVISENOTSUPPORTED;
}

STDMETHODIMP Window::DataObject::EnumDAdvise(IEnumSTATDATA** ppenumAdvise)
{
	*ppenumAdvise = NULL;
	return OLE_E_ADVISENOTSUPPORTED;
}