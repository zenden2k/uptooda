#pragma once

#include <Objbase.h>

enum COMInitializerType
{
	COM_SINGLE_THREADED = COINIT_APARTMENTTHREADED,
	COM_MULTI_THREADED = COINIT_MULTITHREADED,
};

struct COMInitializer
{
	COMInitializer(COMInitializerType type)
	{
		hr_ = CoInitializeEx(nullptr, type);
	}

	~COMInitializer()
	{
		if(SUCCEEDED(hr_))
		{
			CoUninitialize();
		}
	}

	BOOL isCOMInitialized() const
	{
		//Checks if COM was initialized
		//RETURN:
		//      = TRUE if COM was initialized, or
		//      = FALSE if it was not (check GetLastError() for details)
		if(hr_ == S_OK ||
			hr_ == S_FALSE)
		{
			return TRUE;
		}

		//Error
		SetLastError(hr_);
		return FALSE;
	}

private:
	HRESULT hr_ = E_UNEXPECTED;

	//Copy constructor and assignment operators are not allowed!
	COMInitializer(COMInitializer const&) = delete;
	COMInitializer(COMInitializer const&&) = delete;
	COMInitializer& operator=(COMInitializer const&) = delete;
	COMInitializer& operator=(COMInitializer const&&) = delete;
};
