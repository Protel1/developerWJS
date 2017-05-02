/**********************************************************************************************************************
 *                                    This file contains the CErrorMessage class.                                     *
 *                                                                                                                    *
 * This provides methods to generate (and, in one case, output to the debugger) human-readable error messages.        *
 *                                                                                                                    *
 * WARNING: These methods don't look to be threadsafe.                                                                *
 *                                                                                                                    *
 *                                         Copyright (c) Protel Inc. 2009-2010                                        *
 **********************************************************************************************************************/
#pragma once

class CErrorMessage
 {
public:
    static char* ErrorMessageFromSystem ( DWORD dwError )
    {
        /**************************************************************************************************************
         * This returns the human-readable message corresponding to error number dwError.                             *
         **************************************************************************************************************/
        static char szErrorMessageBuffer [ 1024 ];
        ZeroMemory ( szErrorMessageBuffer, sizeof ( szErrorMessageBuffer ));

        LPVOID lpMessageBuffer = NULL;

        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, // flags [in] - get from system table, allocate buffer
            NULL,                                                      // source [in] - don't care because of flags set
            dwError,                                             // message_id [in] - message regarding specified error
            MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),                                          // language_id [in] -
            (LPTSTR) &lpMessageBuffer, // buffer pointer [out only due to flags] - points to buffer containing returned message
            0,                                                         // size [in] - minimum size of alllocated buffer
            NULL );                                                                            // arguments [in] - none

        StringCbCopy( szErrorMessageBuffer, sizeof ( szErrorMessageBuffer ), (char*)lpMessageBuffer );
        LocalFree ( lpMessageBuffer );
        return szErrorMessageBuffer;
    }

    static void PrintComError ( _com_error  &e )
    {
        /**************************************************************************************************************
         * This sends a human-readable error message for com (component, NOT communications) error e to the debugger. *
         **************************************************************************************************************/
#ifdef _DEBUG
        OutputDebugString ( ReturnComErrorMessage ( e ));                                   // Windows function + below
#endif
    }

    static char* ReturnComErrorMessage ( _com_error &e )
    {
        /**************************************************************************************************************
         * This returns a human-readable error message for com (component, NOT communications) error e, e.g. a thrown *
         * error.                                                                                                     *
         **************************************************************************************************************/
        _bstr_t bstrDescription ( e.Description());                                     // textual description of error
        HRESULT hError = e.Error();
        IErrorInfo* pErrorInfo = e.ErrorInfo();                                                     // is this used????
        TCHAR* pszErrorMessage = ( TCHAR* ) e.ErrorMessage();
        GUID guid = e.GUID();                                 // globally-unique ID of interface that defined the error
        DWORD dwHelpContext = e.HelpContext();                                                      // is this used????
        _bstr_t bHelpFile ( e.HelpFile());                                                          // is this used????
        //e.HRESULTToWCode();
        _bstr_t bstrSource ( e.Source());                                 // name of component that generated the error
        WORD wCode = e.WCode();                                                                     // is this used????

        static char szMessage [ 1024 ];
        StringCchPrintf ( szMessage, sizeof ( szMessage ), "Error\n\tCode = %08x\n\tCode meaning = %s\n\tSource = %s\n\tDescription = %s\n", hError, pszErrorMessage, (LPCSTR) bstrSource, (LPCSTR) bstrDescription );
        return szMessage;
    }
 };
