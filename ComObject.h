#pragma once

template <typename DerivedType, typename InterfaceType>
class __declspec(novtable) ComObject : public InterfaceType
{
public:
	ComObject() : m_ref(0) {}

	STDMETHOD(QueryInterface)(REFIID riid, void** ppvObject)
	{
		if (riid == IID_IUnknown || riid == __uuidof(InterfaceType))
		{
			*ppvObject = static_cast<InterfaceType*>(this);
			AddRef();
			return S_OK;
		}
		else
		{
			*ppvObject = NULL;
			return E_NOINTERFACE;
		}
	}

	STDMETHOD_(ULONG, AddRef)()
	{
		return InterlockedIncrement(&m_ref);
	}

	STDMETHOD_(ULONG, Release)()
	{
		ULONG newRefCount = InterlockedDecrement(&m_ref);

		if (!newRefCount)
			delete static_cast<DerivedType*>(this);

		return newRefCount;
	}

private:
	ULONG m_ref;
};