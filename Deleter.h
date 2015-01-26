#pragma once

struct HANDLE_Deleter
{
	typedef HANDLE pointer;
	void operator()(HANDLE h) { CloseHandle(h); }
};

inline HANDLE TranslateFileHandle(HANDLE h)
{
	return (h == INVALID_HANDLE_VALUE ? NULL : h);
}

template <typename T>
struct MappedViewDeleter
{
	typedef const T* pointer;
	void operator()(const T* p) { UnmapViewOfFile(p); }
};

struct HGDIOBJ_Deleter
{
	typedef HGDIOBJ pointer;
	void operator()(HGDIOBJ o) { DeleteObject(o); }
};