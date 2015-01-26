#pragma once

#define WM_ENSURELOADED WM_USER

class Window
{
public:
	Window();

	static HRESULT Register();
	static void Unregister();

	HRESULT Create();
	void Destroy();

	void Enumerate();

private:
	static LRESULT CALLBACK StaticWindowProc(HWND, UINT, WPARAM, LPARAM);
	LRESULT WindowProc(UINT, WPARAM, LPARAM);

	bool OnCreate(CREATESTRUCT*);
	void OnSize();
	void OnActivate(UINT);
	void OnClose();
	LRESULT OnNotify(NMHDR*);
	void OnLvDblClk(NMITEMACTIVATE*);
	void OnLvGetEmptyMarkup(NMLVEMPTYMARKUP*);
	void OnLvGetDispInfo(NMLVDISPINFO*);
	void OnLvCacheHint(NMLVCACHEHINT*);
	void OnLvBeginDrag(NMLISTVIEW*);

	HRESULT EnsureCrestLoaded(int);

private:
	static const TCHAR CLASSNAME[];

	static const COLORREF BGCOLOR;
	static const COLORREF FGCOLOR;

	static HBRUSH s_brush;
	static ATOM s_atom;

	HWND m_hWnd;
	HWND m_hCtlLv;
	CComPtr<IImageList2> m_iml;

	vector<CrestPtr> m_crests;

private:
	class DropSource : public ComObject<DropSource, IDropSource>
	{
	public:
		STDMETHOD(QueryContinueDrag)(BOOL, DWORD);
		STDMETHOD(GiveFeedback)(DWORD);
	};

	class DataObject : public ComObject<DataObject, IDataObject>
	{
	public:
		DataObject(HWND, vector<CrestPtr>&&, map<Crest*, int>&&);
		int GetDataIndex(const FORMATETC*) const;

		STDMETHOD(GetData)(FORMATETC*, STGMEDIUM*);
		STDMETHOD(GetDataHere)(FORMATETC*, STGMEDIUM*);
		STDMETHOD(QueryGetData)(FORMATETC*);
		STDMETHOD(GetCanonicalFormatEtc)(FORMATETC*, FORMATETC*);
		STDMETHOD(SetData)(FORMATETC*, STGMEDIUM*, BOOL);
		STDMETHOD(EnumFormatEtc)(DWORD, IEnumFORMATETC**);
		STDMETHOD(DAdvise)(FORMATETC*, DWORD, IAdviseSink*, DWORD*);
		STDMETHOD(DUnadvise)(DWORD);
		STDMETHOD(EnumDAdvise)(IEnumSTATDATA**);

	private:
		HWND m_hWnd;
		vector<CrestPtr> m_crests;
		map<Crest*, int> m_idxmap;
		FORMATETC m_fe[2];
	};
};