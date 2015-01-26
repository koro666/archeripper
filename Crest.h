#pragma once

class Crest;
typedef shared_ptr<Crest> CrestPtr;

class CrestLoader;
typedef shared_ptr<CrestLoader> CrestLoaderPtr;

class CrestSource;
typedef shared_ptr<CrestSource> CrestSourcePtr;

class CrestSource : public enable_shared_from_this<CrestSource>
{
public:
	explicit CrestSource(LPCTSTR);

	static vector<CrestSourcePtr> Enumerate();
	HRESULT Load();
	vector<CrestPtr> Scan();

private:
	static const TCHAR DIRECTORY[];
	static const TCHAR WILDCARD[];

	tstring m_path;
	LARGE_INTEGER m_size;
	unique_ptr<BYTE, MappedViewDeleter<BYTE>> m_data; 
};

class CrestLoader
{
public:
	HRESULT Initialize();

	IDirect3DDevice9Ex* GetDevice() const { return m_dev; };
	IWICImagingFactory* GetWIC() const { return m_wic; };

private:
	CComPtr<IDirect3D9Ex> m_d3d;
	CComPtr<IDirect3DDevice9Ex> m_dev;
	CComPtr<IWICImagingFactory> m_wic;
};

class Crest
{
public:
	Crest(const CrestSourcePtr&, LPCTSTR, const BYTE*, DWORD);

	const tstring& GetName() const { return m_name; }

	static vector<CrestPtr> Enumerate(const CrestLoaderPtr&);
	void SetLoader(const CrestLoaderPtr&);

	bool CanLoad() const;
	HRESULT Load();
	void Disconnect();

	HRESULT CreateBitmap(UINT, UINT, HBITMAP*);

	HRESULT GetImage(IImageList*, int);
	HRESULT Save(IStream*);

private:
	CrestSourcePtr m_source;
	CrestLoaderPtr m_loader;
	tstring m_name;
	const BYTE* m_data;
	DWORD m_size;
	CComPtr<IWICBitmap> m_bitmap;
};