#include "StdAfx.h"

const TCHAR CrestSource::DIRECTORY[] = TEXT("C:\\ArcheAge\\Documents\\USER\\UCC\\down");
const TCHAR CrestSource::WILDCARD[] = TEXT("*.sst");

CrestSource::CrestSource(LPCTSTR path)
:m_path(path)
{
	m_size.QuadPart = 0;
}

vector<CrestSourcePtr> CrestSource::Enumerate()
{
	vector<CrestSourcePtr> result;

	TCHAR buffer1[MAX_PATH];
	PathCombine(buffer1, DIRECTORY, WILDCARD);

	WIN32_FIND_DATA wfd;
	HANDLE h = FindFirstFile(buffer1, &wfd);
	if (h != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (!(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				TCHAR buffer2[MAX_PATH];
				PathCombine(buffer2, DIRECTORY, wfd.cFileName);

				result.emplace_back(make_shared<CrestSource>(buffer2));
			}

		} while (FindNextFile(h, &wfd));

		FindClose(h);
	}

	return result;
}

HRESULT CrestSource::Load()
{
	if (m_data)
		return S_FALSE;

	unique_ptr<HANDLE, HANDLE_Deleter> hFile(
		TranslateFileHandle(
			CreateFile(
				m_path.c_str(),
				GENERIC_READ,
				FILE_SHARE_READ|FILE_SHARE_DELETE,
				NULL,
				OPEN_EXISTING,
				0,
				NULL)));

	if (!hFile)
		return HRESULT_FROM_WIN32(GetLastError());

	if (!GetFileSizeEx(hFile.get(), &m_size))
		return HRESULT_FROM_WIN32(GetLastError());

	if (m_size.HighPart || m_size.LowPart < (sizeof(DWORD) + sizeof(DDSURFACEDESC2)))
		return E_FAIL;

	unique_ptr<HANDLE, HANDLE_Deleter> hSection(
		CreateFileMapping(hFile.get(), NULL, PAGE_READONLY, 0, 0, NULL));

	if (!hSection)
		return HRESULT_FROM_WIN32(GetLastError());

	m_data.reset(reinterpret_cast<LPBYTE>(
		MapViewOfFile(hSection.get(), FILE_MAP_READ, 0,0,0)));

	if (!m_data)
		return HRESULT_FROM_WIN32(GetLastError());

	return S_OK;
}

vector<CrestPtr> CrestSource::Scan()
{
	vector<CrestPtr> result;

	TCHAR name[MAX_PATH];
	_tcscpy_s(name, PathFindFileName(m_path.c_str()));
	PathRemoveExtension(name);

	for (const BYTE* pdds = m_data.get();
		pdds < (m_data.get() + m_size.LowPart - 0x80);
		++pdds)
	{
		// Can't use DDSURFACEDESC2 on x64 because it has a different alignment
		auto pddsd = reinterpret_cast<__unaligned const DWORD*>(pdds);

		if (pddsd[0] == 0x20534444u && pddsd[1] == 0x0000007Cu)
		{
			DWORD offset = static_cast<DWORD>(pdds - m_data.get());

			TCHAR fullname[MAX_PATH];
			_sntprintf_s(fullname,
				MAX_PATH, _TRUNCATE,
				_T("%s_%08X"),
				name,
				offset);

			result.emplace_back(
				make_shared<Crest>(
					shared_from_this(),
					fullname,
					pdds,
					m_size.LowPart - offset));
				
			pdds += 0x7f;
			if (pddsd[2] & DDSD_LINEARSIZE) // flags
				pdds += pddsd[5]; // linear size
		}
	}

	return result;
}

HRESULT CrestLoader::Initialize()
{
	if (m_d3d && m_dev && m_wic)
		return S_FALSE;

	// Create dummy D3D device
	HRESULT hr = Direct3DCreate9Ex(D3D_SDK_VERSION, &m_d3d);
	if (FAILED(hr))
		return hr;

	D3DPRESENT_PARAMETERS d3dpp =
	{
		1, 1,
		D3DFMT_UNKNOWN,
		0,
		D3DMULTISAMPLE_NONE,
		0,
		D3DSWAPEFFECT_DISCARD,
		GetDesktopWindow(),
		TRUE
	};

	hr = m_d3d->CreateDeviceEx(
		D3DADAPTER_DEFAULT,
		D3DDEVTYPE_NULLREF,
		NULL,
		D3DCREATE_NOWINDOWCHANGES|D3DCREATE_SOFTWARE_VERTEXPROCESSING,
		&d3dpp,
		NULL,
		&m_dev);

	if (FAILED(hr))
	{
		m_d3d.Release();
		return hr;
	}

	// Create WIC factory
	hr = CoCreateInstance(
		CLSID_WICImagingFactory1,
		NULL,
		CLSCTX_INPROC,
		IID_IWICImagingFactory,
		reinterpret_cast<void**>(&m_wic));

	if (FAILED(hr))
	{
		m_dev.Release();
		m_d3d.Release();
		return hr;
	}

	return S_OK;
}

Crest::Crest(const CrestSourcePtr& source, LPCTSTR name, const BYTE* data, DWORD size)
:m_source(source),
m_name(name),
m_data(data),
m_size(size)
{
}

vector<CrestPtr> Crest::Enumerate(const CrestLoaderPtr& loader)
{
	vector<CrestPtr> result;
	
	for (auto& src : CrestSource::Enumerate())
	{
		if (FAILED(src->Load()))
			continue;

		auto scan = src->Scan();

		if (!scan.empty())
		{
			result.reserve(result.size() + scan.size());

			for (auto& crest : scan)
			{
				crest->SetLoader(loader);
				result.push_back(crest);
			}
		}
	}

	return result;
}

void Crest::SetLoader(const CrestLoaderPtr& loader)
{
	m_loader = loader;
}

bool Crest::CanLoad() const
{
	return m_source && m_loader && m_data && m_size && !m_bitmap;
}

HRESULT Crest::Load()
{
	if (m_bitmap)
		return S_FALSE;

	if (!m_source || !m_loader || !m_data || !m_size)
		return E_FAIL;

	// Load DDS from memory and convert to BGRA
	D3DXIMAGE_INFO d3dxii;
	CComPtr<IDirect3DTexture9> pd3dtx;
	HRESULT hr = D3DXCreateTextureFromFileInMemoryEx(
		m_loader->GetDevice(),
		m_data,
		m_size,
		D3DX_DEFAULT,
		D3DX_DEFAULT,
		1,
		D3DUSAGE_DYNAMIC,
		D3DFMT_A8R8G8B8,
		D3DPOOL_DEFAULT,
		D3DX_FILTER_NONE,
		D3DX_FILTER_NONE,
		0,
		&d3dxii,
		NULL,
		&pd3dtx);

	if (FAILED(hr))
		return hr;

	// Get surface data
	CComPtr<IDirect3DSurface9> pd3dsurf;
	hr = pd3dtx->GetSurfaceLevel(0, &pd3dsurf);

	if (FAILED(hr))
		return hr;

	D3DLOCKED_RECT lr;
	hr = pd3dsurf->LockRect(&lr, NULL, D3DLOCK_READONLY);

	if (FAILED(hr))
		return hr;

	// Create WIC bitmap from surface data
	hr = m_loader->GetWIC()->CreateBitmapFromMemory(
		d3dxii.Width,
		d3dxii.Height,
		GUID_WICPixelFormat32bppBGRA,
		lr.Pitch,
		lr.Pitch * d3dxii.Height,
		reinterpret_cast<BYTE*>(lr.pBits),
		&m_bitmap);

	pd3dsurf->UnlockRect();

	if (FAILED(hr))
		return hr;

	return S_OK;
}

void Crest::Disconnect()
{
	m_source.reset();
	m_data = NULL;
	m_size = 0;
}

HRESULT Crest::CreateBitmap(UINT width, UINT height, HBITMAP* phbmp)
{
	*phbmp = NULL;

	if (!m_loader || !m_bitmap)
		return E_FAIL;

	// Get source size
	UINT srcWidth, srcHeight;
	HRESULT hr = m_bitmap->GetSize(&srcWidth, &srcHeight);
	if (FAILED(hr))
		return hr;

	// Get destination size
	UINT dstWidth = width ? width : srcWidth,
		dstHeight = height ? height : srcHeight;

	// Scale if needed
	CComPtr<IWICBitmapSource> scaledBitmap;
	if (srcWidth == dstWidth && srcHeight == dstHeight)
		scaledBitmap = m_bitmap;
	else
	{
		CComPtr<IWICBitmapScaler> scaler;
		hr = m_loader->GetWIC()->CreateBitmapScaler(&scaler);

		if (FAILED(hr))
			return hr;

		hr = scaler->Initialize(
			m_bitmap,
			dstWidth,
			dstHeight,
			WICBitmapInterpolationModeCubic);

		if (FAILED(hr))
			return hr;

		scaledBitmap = scaler;
	}

	// Premultiply alpha
	CComPtr<IWICFormatConverter> premultipliedBitmap;
	hr = m_loader->GetWIC()->CreateFormatConverter(&premultipliedBitmap);

	if (FAILED(hr))
		return hr;

	hr = premultipliedBitmap->Initialize(
		scaledBitmap,
		GUID_WICPixelFormat32bppPBGRA,
		WICBitmapDitherTypeNone,
		NULL,
		0.0,
		WICBitmapPaletteTypeCustom);

	if (FAILED(hr))
		return hr;

	// Create GDI bitmap
	BITMAPINFO bi =
	{{
		sizeof(BITMAPINFOHEADER),
		dstWidth,
		-dstHeight,
		1,
		32,
		BI_RGB
	}};

	void* gdibits;
	unique_ptr<HBITMAP, HGDIOBJ_Deleter> gdibmp(
		CreateDIBSection(
			NULL,
			&bi,
			DIB_RGB_COLORS,
			&gdibits,
			NULL,
			0));

	if (!gdibmp)
		return HRESULT_FROM_WIN32(GetLastError());

	// Fill GDI bitmap
	WICRect wr = { 0, 0, dstWidth, dstHeight };
	hr = premultipliedBitmap->CopyPixels(
		&wr,
		dstWidth * sizeof(RGBQUAD),
		dstWidth * sizeof(RGBQUAD) * dstHeight,
		reinterpret_cast<BYTE*>(gdibits));

	if (FAILED(hr))
		return hr;

	// Return
	*phbmp = reinterpret_cast<HBITMAP>(gdibmp.release());
	return S_OK;
}

HRESULT Crest::GetImage(IImageList* piml, int position)
{
	// Get destination size
	UINT dstWidth, dstHeight;
	HRESULT hr = piml->GetIconSize(
		reinterpret_cast<int*>(&dstWidth),
		reinterpret_cast<int*>(&dstHeight));
	if (FAILED(hr))
		return hr;

	// Create bitmap
	unique_ptr<HBITMAP, HGDIOBJ_Deleter> gdibmp;
	{
		HBITMAP result;
		hr = CreateBitmap(dstWidth, dstHeight, &result);
		gdibmp.reset(result);
	}

	if (FAILED(hr))
		return hr;

	// Replace image in ImageList
	hr = piml->Replace(position, reinterpret_cast<HBITMAP>(gdibmp.get()), NULL);

	if (FAILED(hr))
		return hr;

	return S_OK;
}

HRESULT Crest::Save(IStream* strm)
{
	if (!m_loader || !m_bitmap)
		return E_FAIL;

	// Create and initialize encoder
	CComPtr<IWICBitmapEncoder> encoder;
	HRESULT hr = m_loader->GetWIC()->CreateEncoder(
		GUID_ContainerFormatPng,
		NULL,
		&encoder);
	if (FAILED(hr))
		return hr;

	hr = encoder->Initialize(strm, WICBitmapEncoderNoCache);
	if (FAILED(hr))
		return hr;

	// Create and initialize frame
	CComPtr<IWICBitmapFrameEncode> frame;
	hr = encoder->CreateNewFrame(&frame, NULL);
	if (FAILED(hr))
		return hr;

	hr = frame->Initialize(NULL);
	if (FAILED(hr))
		return hr;

	// Write bitmap
	hr = frame->WriteSource(m_bitmap, NULL);
	if (FAILED(hr))
		return hr;

	// Commit
	hr = frame->Commit();
	if (FAILED(hr))
		return hr;

	hr = encoder->Commit();
	if (FAILED(hr))
		return hr;

	return S_OK;
}