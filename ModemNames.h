#pragma once
#include <comdef.h>

#include <Wbemidl.h>
#pragma comment(lib, "wbemuuid.lib")

class CModemNames
{
private:
	char* _names[ 1024 ];
	char* _ports[ 1024 ];
	int count;

public:
	CModemNames(void)
	{
		HRESULT hResult = CoInitialize(NULL);
		ZeroMemory ( _names, sizeof ( _names ) / sizeof ( _names[ 0 ] ));
		ZeroMemory ( _ports, sizeof ( _ports ) / sizeof ( _ports[ 0 ] ));
		count = 0;




		char szError [ 4096 ];
		HRESULT hres;

		// Set general COM security levels --------------------------
		// Note: If you are using Windows 2000, you need to specify -
		// the default authentication credentials for a user by using
		// a SOLE_AUTHENTICATION_LIST structure in the pAuthList ----
		// parameter of CoInitializeSecurity ------------------------

		hres =  CoInitializeSecurity(
			NULL, 
			-1,                          // COM authentication
			NULL,                        // Authentication services
			NULL,                        // Reserved
			RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication 
			RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation  
			NULL,                        // Authentication info
			EOAC_NONE,                   // Additional capabilities 
			NULL                         // Reserved
			);


		//if (FAILED(hres))
		//{
		//	StringCchPrintf ( szError, sizeof ( szError ), "Failed to initialize security. Error code = 0x%x", hres );
		//	throw szError;                    // Program has failed.
		//}

		// Obtain the initial locator to WMI -------------------------

		IWbemLocator *pLoc = NULL;

		hres = CoCreateInstance(
			CLSID_WbemLocator,             
			0, 
			CLSCTX_INPROC_SERVER, 
			IID_IWbemLocator, (LPVOID *) &pLoc);

		if (FAILED(hres))
		{
			StringCchPrintf ( szError, sizeof ( szError ), "Failed to create IWbemLocator object. Error code = 0x%x", hres );
			throw szError;
		}

		// Connect to WMI through the IWbemLocator::ConnectServer method

		IWbemServices *pSvc = NULL;

		// Connect to the root\cimv2 namespace with
		// the current user and obtain pointer pSvc
		// to make IWbemServices calls.
		hres = pLoc->ConnectServer(
			_bstr_t(L"ROOT\\CIMV2"), // Object path of WMI namespace
			NULL,                    // User name. NULL = current user
			NULL,                    // User password. NULL = current
			0,                       // Locale. NULL indicates current
			NULL,                    // Security flags.
			0,                       // Authority (e.g. Kerberos)
			0,                       // Context object 
			&pSvc                    // pointer to IWbemServices proxy
			);

		if (FAILED(hres))
		{
			StringCchPrintf ( szError, sizeof ( szError ), "Could not connect. Error code = 0x%x", hres );
			pLoc->Release();     
			throw szError;                // Program has failed.
		}

#ifdef _DEBUG
		OutputDebugString ( "\nConnected to ROOT\\CIMV2 WMI namespace\n\n" );
#endif

		// Step 5: --------------------------------------------------
		// Set security levels on the proxy -------------------------

		hres = CoSetProxyBlanket(
			pSvc,                        // Indicates the proxy to set
			RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx
			RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx
			NULL,                        // Server principal name 
			RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx 
			RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
			NULL,                        // client identity
			EOAC_NONE                    // proxy capabilities 
			);

		if (FAILED(hres))
		{
			StringCchPrintf ( szError, sizeof ( szError ), "Could not set proxy blanket. Error code = 0x%x", hres );
			pSvc->Release();
			pLoc->Release();     
			throw szError;		// Program has failed.
		}

		// Step 6: --------------------------------------------------
		// Use the IWbemServices pointer to make requests of WMI ----

		// For example, get the name of the operating system
		IEnumWbemClassObject* pEnumerator = NULL;
		hres = pSvc->ExecQuery(
			bstr_t("WQL"), 
			//bstr_t("SELECT * FROM Win32_OperatingSystem"),
			bstr_t("SELECT * FROM Win32_POTSModem"),
			WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, 
			NULL,
			&pEnumerator);

		if (FAILED(hres))
		{
			StringCchPrintf ( szError, sizeof ( szError ), "Query for operating system name failed. Error code = 0x%x", hres );
			pSvc->Release();
			pLoc->Release();
			throw szError;		// Program has failed.
		}

		// Step 7: -------------------------------------------------
		// Get the data from the query in step 6 -------------------

		IWbemClassObject *pclsObj;
		ULONG uReturn = 0;

		int Length;
		while (pEnumerator)
		{
			HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, 
				&pclsObj, &uReturn);

			if(0 == uReturn)
			{
				break;
			}

			VARIANT vtPropName;
			VARIANT vtPropAttachedTo;
			VARIANT vtPropStatus;

			// Get the value of the Name property
			hr = pclsObj->Get(L"Name", 0, &vtPropName, 0, 0);
			hr = pclsObj->Get(L"AttachedTo", 0, &vtPropAttachedTo, 0, 0);
			hr = pclsObj->Get(L"Status", 0, &vtPropStatus, 0, 0);
			_bstr_t bstrName ( vtPropName.bstrVal );
			_bstr_t bstrAttachedTo ( vtPropAttachedTo.bstrVal );
			_bstr_t bstrStatus ( vtPropStatus.bstrVal );
			if( StrCmp((const char*)bstrStatus, "OK")==0)
			{
				Length = bstrName.length() + 1;
				_names[ count ] = new char [ Length ];
				ZeroMemory ( _names[ count ], Length );
				StringCbCopy ( _names[ count ], Length, ( const char* ) bstrName );

				Length = bstrAttachedTo.length() + 1;
				_ports[ count ] = new char [ Length ];
				ZeroMemory ( _ports[ count ], Length );
				StringCbCopy ( _ports[ count ], Length, ( const char* ) bstrAttachedTo );

				count++;
				StringCchPrintf ( szError, sizeof ( szError ), "Modem Name : <<%s>>\n", ( const char* ) bstrName );
#ifdef _DEBUG
				OutputDebugString ( szError );
#endif
			}
			VariantClear(&vtPropName);
			VariantClear(&vtPropAttachedTo);
			VariantClear(&vtPropStatus);
			pclsObj->Release();
		}

		// Cleanup
		// ========

		pSvc->Release();
		pLoc->Release();
		pEnumerator->Release();
		if ( hResult == S_OK || hResult == S_FALSE )
		{
			CoUninitialize();
		}
	}

	~CModemNames(void)
	{
		for ( int Offset = 0; Offset < count; Offset++ )
		{
			delete [] _names[ Offset ];
			delete [] _ports[ Offset ];
			_names[ Offset ] = NULL;
			_ports[ Offset ] = NULL;
		}
		ZeroMemory ( _names, sizeof ( _names ) / sizeof ( _names[ 0 ] ));
		ZeroMemory ( _ports, sizeof ( _ports ) / sizeof ( _ports[ 0 ] ));
		count = 0;
	}


	int GetCount ( void )
	{
		return count;
	}

	LPCTSTR GetPort ( int offset )
	{
		if ( offset >= 0 && offset < count )
		{
			return _ports[ offset ];
		}
		return NULL;
	}

	LPCTSTR GetName ( int offset )
	{
		if ( offset >= 0 && offset < count )
		{
			return _names[ offset ];
		}
		return NULL;
	}
	//long GetCount( ); __declspec(property(get=GetCount)) long Count;
	//PropertyPtr GetItem( const _variant_t & Index ); __declspec(property(get=GetItem)) PropertyPtr Item[];
	// This allows the class to be used like a C# class that has properties
	__declspec(property(get = GetCount)) int Count;
	__declspec(property(get=GetPort)) LPCTSTR Ports[];
	__declspec(property(get=GetName)) LPCTSTR Names[];
};
