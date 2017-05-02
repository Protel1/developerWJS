/**********************************************************************************************************************
 *                                    This file contains the CAdoRecordset class.                                     *
 *                                                                                                                    *
 * This class is only used as a friend of CAdoStoredProcedure. It doesn't seem to do anything so it has been mostly   *
 * disabled.                                                                                                          *
 *                                                                                                                    *
 *                                         Copyright (c) Protel Inc. 2009-2010                                        *
 **********************************************************************************************************************/
#pragma once

#include "AdoConnection.h"
#include "variantBlob.h"

#if 0
class CAdoRecordset
 {
public:
    CAdoRecordset( CAdoConnection* pConnection )
    {
        m_pConnection = pConnection;
        m_pRecordSet = NULL;
    }

    virtual ~CAdoRecordset(void)
    {
        if ( m_pRecordSet != NULL )
        {
            m_pRecordSet->Close();
            m_pRecordSet = NULL;
        }
        m_pConnection = NULL;
    }

    bool AllTablesNames ( void )                                                                // appears to be unused
    {
        if ( m_pRecordSet != NULL )
        {
            m_pRecordSet->Close();
            m_pRecordSet = NULL;
        }
        m_pRecordSet = m_pConnection->ReturnTableNames();
        if ( m_pRecordSet == NULL )
        {
            return false;
        }
        return true;
    }

    bool ExecuteStoredProcedure ( CAdoStoredProcedure& adoStoredProcedure, bool bSupressMessages = false )
                                                                                                // appears to be unused
    {
        m_pRecordSet = m_pConnection->ReturnRecordset( adoStoredProcedure, bSupressMessages );
        return true;
    }

    bool Open ( char* pszSelect, bool bUpdateable = false, long nType = ADODB::adCmdText )          // is this used????
    {
        if ( m_pRecordSet != NULL )
        {
            m_pRecordSet->Close();
            m_pRecordSet = NULL;
        }
        if ( m_pConnection == NULL )
        {
            return false;
        }
        m_pRecordSet = m_pConnection->ReturnRecordset( pszSelect, bUpdateable, nType );
        return true;
    }

    int GetRecordCount( void )                                                                  // appears to be unused
    {
        if ( m_pRecordSet != NULL )
        {
            return m_pRecordSet->RecordCount;
        }
        return 0;
    }

    int FieldCount ( void )                                                                     // appears to be unused
    {
        if ( m_pRecordSet != NULL )
        {
            return m_pRecordSet->Fields->Count;
        }
        return 0;
    }

    unsigned int FieldSize ( int nField )                                                       // appears to be unused
    {
        if ( m_pRecordSet != NULL )
        {
            if ( nField >= 0 && nField < m_pRecordSet->Fields->GetCount())
            {
                _variant_t vtIndex;
                vtIndex.vt = VT_I2;
                vtIndex.iVal = nField;
                ADODB::FieldPtr fieldPtr = m_pRecordSet->Fields->GetItem( vtIndex );
                return fieldPtr->GetDefinedSize();
            }
        }
        return 0;
    }

    unsigned int FieldSize ( char* pszField )                              // used only by this::PutField (i.e. unused)
    {
        if ( m_pRecordSet != NULL )
        {
            _bstr_t bstrName ( pszField );
            _variant_t vtName ( bstrName );
            try
            {
                ADODB::FieldPtr fieldPtr = m_pRecordSet->Fields->GetItem( vtName );
                return fieldPtr->GetDefinedSize();
            }
            catch ( _com_error &comError )
            {
                comError;
                //PrintComError ( comError );
            }
        }
        return 0;
    }

    int FieldActualSize ( int nField )                                                          // appears to be unused
    {
        if ( m_pRecordSet != NULL )
        {
            if ( nField >= 0 && nField < m_pRecordSet->Fields->GetCount())
            {
                _variant_t vtIndex;
                vtIndex.vt = VT_I2;
                vtIndex.iVal = nField;
                ADODB::FieldPtr fieldPtr = m_pRecordSet->Fields->GetItem( vtIndex );
                return fieldPtr->GetActualSize();
            }
        }
        return 0;
    }

    int FieldActualSize ( char* pszField )                                                      // appears to be unused
    {
        if ( m_pRecordSet != NULL )
        {
            _bstr_t bstrName ( pszField );
            _variant_t vtName ( bstrName );
            try
            {
                ADODB::FieldPtr fieldPtr = m_pRecordSet->Fields->GetItem( vtName );
                return fieldPtr->GetActualSize();
            }
            catch ( _com_error &comError )
            {
                comError;
                //PrintComError ( comError );
            }
        }
        return 0;
    }

    int FieldType ( char* pszField )                                       // used only by this::PutField (i.e. unused)
    {
        if ( m_pRecordSet != NULL )
        {
            _bstr_t bstrName ( pszField );
            _variant_t vtName ( bstrName );
            try
            {
                ADODB::FieldPtr fieldPtr = m_pRecordSet->Fields->GetItem( vtName );
                return fieldPtr->GetType();
            }
            catch ( _com_error &comError )
            {
                comError;
                //PrintComError ( comError );
            }
        }
        return -1;
    }

    int FieldType ( int nField )                                                                // appears to be unused
    {
        if ( m_pRecordSet != NULL )
        {
            _variant_t vtIndex;
            vtIndex.vt = VT_I2;
            vtIndex.iVal = nField;
            ADODB::FieldPtr fieldPtr = m_pRecordSet->Fields->GetItem( vtIndex );
            return fieldPtr->GetType();
        }
        return -1;
    }


    int FieldType ( int nField, _bstr_t& bstrType )                                             // appears to be unused
    {
        if ( m_pRecordSet != NULL )
        {
            if ( nField >= 0 && nField < m_pRecordSet->Fields->GetCount())
            {
                _variant_t vtIndex;
                vtIndex.vt = VT_I2;
                vtIndex.iVal = nField;
                ADODB::FieldPtr fieldPtr = m_pRecordSet->Fields->GetItem( vtIndex );
                switch ( fieldPtr->GetType())
                {
                    case ADODB::adBigInt:
                        bstrType = "adBigInt";
                        break;
                    case ADODB::adBinary:
                        bstrType = "adBinary";
                        break;
                    case ADODB::adBoolean:
                        bstrType = "adBoolean";
                        break;
                    case ADODB::adBSTR:
                        bstrType = "adBSTR";
                        break;
                    case ADODB::adChapter:
                        bstrType = "adChapter";
                        break;
                    case ADODB::adChar:
                        bstrType = "adChar";
                        break;
                    case ADODB::adCurrency:
                        bstrType = "adCurrency";
                        break;
                    case ADODB::adDate:
                        bstrType = "adDate";
                        break;
                    case ADODB::adDBDate:
                        bstrType = "adDBDate";
                        break;
                    case ADODB::adDBTime:
                        bstrType = "adDBTime";
                        break;
                    case ADODB::adDBTimeStamp:
                        bstrType = "adDBTimeStamp";
                        break;
                    case ADODB::adDecimal:
                        bstrType = "adDecimal";
                        break;
                    case ADODB::adDouble:
                        bstrType = "adDouble";
                        break;
                    case ADODB::adEmpty:
                        bstrType = "adEmpty";
                        break;
                    case ADODB::adError:
                        bstrType = "adError";
                        break;
                    case ADODB::adFileTime:
                        bstrType = "adFileTime";
                        break;
                    case ADODB::adGUID:
                        bstrType = "adGUID";
                        break;
                    case ADODB::adIDispatch:
                        bstrType = "adIDispatch";
                        break;
                    case ADODB::adInteger:
                        bstrType = "adInteger";
                        break;
                    case ADODB::adIUnknown:
                        bstrType = "adIUnknown";
                        break;
                    case ADODB::adLongVarBinary:
                        bstrType = "adLongVarBinary";
                        break;
                    case ADODB::adLongVarChar:
                        bstrType = "adLongVarChar";
                        break;
                    case ADODB::adLongVarWChar:
                        bstrType = "adLongVarWChar";
                        break;
                    case ADODB::adNumeric:
                        bstrType = "adNumeric";
                        break;
                    case ADODB::adPropVariant:
                        bstrType = "adPropVariant";
                        break;
                    case ADODB::adSingle:
                        bstrType = "adSingle";
                        break;
                    case ADODB::adSmallInt:
                        bstrType = "adSmallInt";
                        break;
                    case ADODB::adTinyInt:
                        bstrType = "adTinyInt";
                        break;
                    case ADODB::adUnsignedBigInt:
                        bstrType = "adUnsignedBigInt";
                        break;
                    case ADODB::adUnsignedInt:
                        bstrType = "adUnsignedInt";
                        break;
                    case ADODB::adUnsignedSmallInt:
                        bstrType = "adUnsignedSmallInt";
                        break;
                    case ADODB::adUnsignedTinyInt:
                        bstrType = "adUnsignedTinyInt";
                        break;
                    case ADODB::adUserDefined:
                        bstrType = "adUserDefined";
                        break;
                    case ADODB::adVarBinary:
                        bstrType = "adVarBinary";
                        break;
                    case ADODB::adVarChar:
                        bstrType = "adVarChar";
                        break;
                    case ADODB::adVariant:
                        bstrType = "adVariant";
                        break;
                    case ADODB::adVarNumeric:
                        bstrType = "adVarNumeric";
                        break;
                    case ADODB::adVarWChar:
                        bstrType = "adVarWChar";
                        break;
                    case ADODB::adWChar :
                            bstrType = "adWChar ";
                        break;
                }
                return true;
            }
        }
        return false;
    }

    bool FieldName ( int nField, char** pszName )                                               // appears to be unused
    {
        if ( m_pRecordSet != NULL )
        {
            if ( nField >= 0 && nField < m_pRecordSet->Fields->GetCount())
            {
                if ( *pszName != NULL )
                {
                    delete [] *pszName;
                    *pszName = NULL;
                }
                _variant_t vtIndex;
                vtIndex.vt = VT_I2;
                vtIndex.iVal = nField;
                ADODB::FieldPtr fieldPtr = m_pRecordSet->Fields->GetItem( vtIndex );
                _bstr_t bstrName ( fieldPtr->GetName());
                *pszName = new char [ lstrlen (( const char* ) bstrName ) + 2 ];
                StringCbCopy ( *pszName, lstrlen (( const char* ) bstrName + 1 ), ( const char* ) bstrName );
                return true;
            }
        }
        return false;
    }

    bool FieldName ( int nField, _bstr_t& bstrName )                                            // appears to be unused
    {
        if ( m_pRecordSet != NULL )
        {
            if ( nField >= 0 && nField < m_pRecordSet->Fields->GetCount())
            {
                _variant_t vtIndex;
                vtIndex.vt = VT_I2;
                vtIndex.iVal = nField;
                ADODB::FieldPtr fieldPtr = m_pRecordSet->Fields->GetItem( vtIndex );
                bstrName = fieldPtr->GetName();
                return true;
            }
        }
        return false;
    }

    /*
     * All the following GettField methods appear to only be used by other GetFields, i.e. they are unused.
     */
    bool GetField ( char* pszField, _bstr_t& bstrValue )
    {
        _variant_t vtValue = GetField ( pszField );
        bstrValue = "";
        if ( vtValue.vt != VT_NULL && vtValue.vt != VT_EMPTY )
        {
            bstrValue = vtValue;
        }
        return true;
    }

    bool GetField ( char* pszField, int& intValue )
    {
        long nValue;
        bool bOK = GetField ( pszField, nValue );
        intValue = nValue;
        return bOK;
    }

    bool GetField ( char* pszField, long& longValue )
    {
        longValue = 0;
        _variant_t vtValue = GetField ( pszField );
        _variant_t vtLong;
        VariantChangeTypeEx( &vtLong, &vtValue, LOCALE_USER_DEFAULT, VARIANT_LOCALBOOL, VT_I4 );
        longValue = vtLong.lVal;
        return true;
    }

    bool GetField ( char* pszField, DATE& dateValue )
    {
        dateValue = 0;
        _variant_t vtValue = GetField ( pszField );
        _variant_t vtDate;
        VariantChangeTypeEx( &vtDate, &vtValue, LOCALE_USER_DEFAULT, VARIANT_LOCALBOOL, VT_DATE );
        dateValue = vtDate.date;
        return true;
    }

    bool GetField ( char* pszField, CURRENCY& curValue )
    {
        curValue.int64 = 0;
        _variant_t vtValue = GetField ( pszField );
        if ( vtValue.vt != VT_NULL )
        {
            _variant_t vtCurrency;
            VariantChangeTypeEx( &vtCurrency, &vtValue, LOCALE_USER_DEFAULT, VARIANT_LOCALBOOL, VT_CY );
            curValue = vtCurrency.cyVal;
        }
        return true;
    }

    bool GetField ( char* pszField, bool& bValue )
    {
        bValue = false;
        _variant_t vtValue = GetField ( pszField );
        _variant_t vtBool;
        VariantChangeTypeEx( &vtBool, &vtValue, LOCALE_USER_DEFAULT, VARIANT_LOCALBOOL, VT_BOOL );
        if ( vtBool.boolVal == 0 )
        {
            bValue = false;
        }
        else
        {
            bValue = true;
        }
        return true;
    }

    /*
     * All the following PutField methods appear to only be used by other PutFields, i.e. they are unused.
     */
    bool PutField ( char* pszField, char* pszValue )
    {
        _bstr_t bstrValue ( pszValue );
        return PutField ( pszField, bstrValue );
    }

    bool PutField ( char* pszField, _bstr_t& bstrValue )
    {
        unsigned int nLength = 0;
        bool bModifySize = false;
        int nType = FieldType ( pszField );
        if ( nType == ADODB::adBSTR || nType == ADODB::adChar || nType == ADODB::adLongVarChar || nType == ADODB::adLongVarWChar || nType == ADODB::adVarChar || nType == ADODB::adVarWChar || nType == ADODB::adWChar )
        {
            nLength = FieldSize ( pszField );
            if ( bstrValue.length() > nLength )
            {
                bModifySize = true;
            }
        }
        if ( bModifySize )
        {
            BSTR bStrNew = SysAllocStringLen ( bstrValue, nLength );
            _bstr_t bstrTheValue ( bStrNew );
            _variant_t vtValue ( bstrTheValue );
            SysFreeString ( bStrNew );
            return Puts ( pszField, vtValue );
        }
        else
        {
            _variant_t vtValue ( bstrValue );
            return Puts ( pszField, vtValue );
        }
    }

    bool PutField ( char* pszField, int& intValue )
    {
        _variant_t vtValue (( long ) intValue, VT_I4 );
        return Puts ( pszField, vtValue );
    }

    bool PutField ( char* pszField, long& longValue )
    {
        _variant_t vtValue ( longValue, VT_I4 );
        return Puts ( pszField, vtValue );
    }

    bool PutField ( char* pszField, SYSTEMTIME* ptimeValue )
    {
        DATE dateValue;
        SystemTimeToVariantTime ( ptimeValue, &dateValue );
        return PutField ( pszField, dateValue );
    }

    bool PutField ( char* pszField, SYSTEMTIME& timeValue )
    {
        DATE dateValue;
        SystemTimeToVariantTime ( &timeValue, &dateValue );
        return PutField ( pszField, dateValue );
    }

    bool PutField ( char* pszField, DATE& dateValue )
    {
        _variant_t vtValue ( dateValue, VT_DATE );
        return Puts ( pszField, vtValue );
    }

    bool PutField ( char* pszField, CURRENCY& curValue )
    {
        _variant_t vtValue;
        vtValue.Clear();
        vtValue.ChangeType ( VT_CY );
        vtValue.cyVal = curValue;
        return Puts ( pszField, vtValue );
    }

    bool PutField ( char* pszField, bool& bValue )
    {
        _variant_t vtValue ( bValue );
        return Puts ( pszField, vtValue );
    }



    bool Move ( long nRecords )                                                                 // appears to be unused
    {
        m_pRecordSet->Move ( nRecords );
        if ( m_pRecordSet->BOF || m_pRecordSet->adoEOF )
        {
            return false;
        }
        return true;
    }

    bool MoveFirst ( void )                                                                     // appears to be unused
    {
        m_pRecordSet->MoveFirst();
        if ( m_pRecordSet->BOF || m_pRecordSet->adoEOF )
        {
            return false;
        }
        return true;
    }

    bool MoveLast ( void )                                                                      // appears to be unused
    {
        m_pRecordSet->MoveLast();
        if ( m_pRecordSet->BOF || m_pRecordSet->adoEOF )
        {
            return false;
        }
        return true;
    }

    bool MoveNext ( void )                                                                      // appears to be unused
    {
        m_pRecordSet->MoveNext();
        if ( m_pRecordSet->BOF || m_pRecordSet->adoEOF )
        {
            return false;
        }
        return true;
    }

    bool MovePrevious ( void )                                                                  // appears to be unused
    {
        m_pRecordSet->MovePrevious();
        return true;
    }

    bool IsEOF ( void )                                                                         // appears to be unused
    {
        if ( m_pRecordSet->BOF || m_pRecordSet->adoEOF )
        {
            return true;
        }
        return false;
    }

    bool IsBOF ( void )                                                                         // appears to be unused
    {
        if ( m_pRecordSet->BOF )
        {
            return true;
        }
        return false;
    }

    bool AddNew ( void )                                                                        // appears to be unused
    {
        try
        {
            m_pRecordSet->AddNew();
            return true;
        }
        catch ( _com_error &comError )
        {
            CErrorMessage::PrintComError ( comError );
        }
        return false;
    }

    bool Update ( void )                                                                        // appears to be unused
    {
        try
        {
            m_pRecordSet->Update();
            return true;
        }
        catch ( _com_error &comError )
        {
            CErrorMessage::PrintComError ( comError );
        }
        return false;
    }


    bool GetLongBinary ( char* pszField, char** pszData, long& nFieldLength ) // used only by this::LoadPhoto (i.e. unused)
    {
        if ( *pszData != NULL )
        {
            delete *pszData;
            *pszData = NULL;
        }

        _bstr_t bstrName ( pszField );
        _variant_t vtName ( bstrName );
        ADODB::FieldPtr fieldPtr = m_pRecordSet->Fields->GetItem( vtName );
        nFieldLength = fieldPtr->ActualSize;

        if ( fieldPtr->ActualSize == 0 )
        {
            return false;
        }

        *pszData = new char [ nFieldLength + 1 ];
        ZeroMemory ( *pszData, nFieldLength + 1 );

        _variant_t varChunk = fieldPtr->Value;
        //VT_ARRAY|VT_UI1
        if (( varChunk.vt & VT_ARRAY ) == VT_ARRAY )
        {
            if ( varChunk.vt != ( VT_ARRAY | VT_UI1 ))
            {
                VariantChangeTypeEx(
                    &varChunk,                              // pvarDest [in] - destination (= source - coerce in place)
                    &varChunk,                                                      // pvarSrc [in] - source VARIANTARG
                    LOCALE_USER_DEFAULT,                                                       // lcid [in] - locale ID
                    VARIANT_LOCALBOOL,                            // wFlags [in] - use computer's locale in conversions
                    VT_ARRAY | VT_UI1 );                                                 // vt [in] - type to coerce to
            }
            void* pszFieldData;
            SafeArrayAccessData( varChunk.parray, &pszFieldData );
            CopyMemory ( *pszData, pszFieldData, nFieldLength );
            SafeArrayUnaccessData ( varChunk.parray );
        }
        else
        {
            _bstr_t bstrChunk ( varChunk );
            CopyMemory ( *pszData, ( char* ) bstrChunk, nFieldLength );
        }
        return true;
    }

    bool PutLongBinary ( char* pszField, BYTE* pszData, long nFieldLength ) // used only by this::SavePhoto (i.e. unused)
    {
        try
        {
            variantBlob varChunk ( pszData, nFieldLength );
            _bstr_t bstrName ( pszField );
            _variant_t vtName ( bstrName );
            ADODB::FieldPtr fieldPtr = m_pRecordSet->Fields->GetItem( vtName );
            fieldPtr->PutValue ( varChunk );
            return true;
        }
        catch ( _com_error &comError )
        {
            CErrorMessage::PrintComError ( comError );
        }
        return false;
    }


    void LoadPhoto( char* pszFieldName, char* pszImageFile )                                    // appears to be unused
    {
        char* pszImage = NULL;
        long nImageSize = 0;
        GetLongBinary ( pszFieldName, &pszImage, nImageSize );
        DWORD dwBytesWritten;
        HANDLE hFile = CreateFile (
            pszImageFile,                                                     // lpFileName [in] - name of file to open
            GENERIC_WRITE,                                                         // dwDesiredAccess [in] - write only
            FILE_SHARE_READ | FILE_SHARE_WRITE,           // dwShareMode [in] - other processes can also read and write
            NULL,                                             // lpSecurityAttributes [in] - NULL = cannot be inherited
            CREATE_ALWAYS,                                  // dwCreationDisposition [in] - overwrite any existing file
            FILE_ATTRIBUTE_NORMAL,                             // dwFlagsAndAttributes [in] - no special attributes set
            NULL );                                                // hTemplateFile [in] - sets attributes, NULL = none
        if ( hFile != INVALID_HANDLE_VALUE )
        {
            WriteFile (
                hFile,                                                                 // hFile [in] - file to write to
                pszImage,                                                              // lpBuffer [in] - data to write
                nImageSize,                                                // nNumberOfBytesToWrite [in] - size of data
                &dwBytesWritten,                               // lpNumberOfBytesWritten [out] - bytes actually written
                NULL );                                                            // lpOverlapped [in, out] - not used
            CloseHandle ( hFile );
        }
        delete [] pszImage;
    }

    void SavePhoto( char* pszFieldName, char* pszImageFile )                                    // appears to be unused
    {
        BYTE* pszImage = NULL;
        int nImageLength = 0;
        HANDLE hFile = CreateFile(
            pszImageFile,                                                     // lpFileName [in] - name of file to open
            GENERIC_READ,                                                           // dwDesiredAccess [in] - read only
            FILE_SHARE_READ,                                        // dwShareMode [in] - other processes can also read
            NULL,                                             // lpSecurityAttributes [in] - NULL = cannot be inherited
            OPEN_EXISTING,                                   // dwCreationDisposition [in] - open already existing file
            FILE_ATTRIBUTE_NORMAL,                             // dwFlagsAndAttributes [in] - no special attributes set
            NULL );                                                // hTemplateFile [in] - sets attributes, NULL = none
        if ( hFile != INVALID_HANDLE_VALUE )
        {
            nImageLength = GetFileSize( hFile, NULL );
            DWORD dwLength = 0;
            pszImage = new BYTE [ nImageLength ];
            BOOL bOK = ReadFile(
                hFile,                                                                // hFile [in] - file to read from
                pszImage,                                                                 // lpBuffer [out] - data read
                nImageLength,                                      // nNumberOfBytesToRead [in] - maximum bytes to read
                &dwLength,                                   // lpNumberOfBytesRead [out] - actual number of bytes read
                NULL );                                                            // lpOverlapped [in, out] - not used
            CloseHandle ( hFile );
            PutLongBinary ( pszFieldName, pszImage, nImageLength );
            delete [] pszImage;
        }
    }


protected:
    _variant_t GetField ( char* pszField )                                                      // appears to be unused
    {
        _variant_t vtReturn;
        vtReturn.Clear();
        if ( m_pRecordSet != NULL && m_pRecordSet->BOF == false && m_pRecordSet->adoEOF == false )
        {
            if ( pszField != NULL && lstrlen ( pszField ) > 0 )
            {
                _bstr_t bstrName ( pszField );
                _variant_t vtName ( bstrName );
                try
                {
                    ADODB::FieldPtr fieldPtr = m_pRecordSet->Fields->GetItem( vtName );
                    if ( fieldPtr->Type != ADODB::adEmpty )
                    {
                        vtReturn = fieldPtr->GetValue();
                    }
                }
                catch ( _com_error &comError )
                {
                    CErrorMessage::PrintComError ( comError );
                }
            }
        }
        return vtReturn;
    }

    bool Puts ( char* pszField, _variant_t& vtValue )              // used only by this::PutField methods (i.e. unused)
    {
        if ( m_pRecordSet != NULL && m_pRecordSet->BOF == false && m_pRecordSet->adoEOF == false )
        {
            if ( pszField != NULL && lstrlen ( pszField ) > 0 )
            {
                _bstr_t bstrName ( pszField );
                _variant_t vtName ( bstrName );
                try
                {
                    ADODB::FieldPtr fieldPtr = m_pRecordSet->Fields->GetItem( vtName );
                    _variant_t vtField = fieldPtr->Value;
                    if ( vtField.vt == VT_NULL )
                    {
                        fieldPtr->PutValue ( vtValue );
                    }
                    else
                    {
                        VariantChangeTypeEx(
                            &vtField,                                                    // pvarDest [in] - destination
                            &vtValue,                                               // pvarSrc [in] - source VARIANTARG
                            LOCALE_USER_DEFAULT,                                               // lcid [in] - locale ID
                            VARIANT_LOCALBOOL,                    // wFlags [in] - use computer's locale in conversions
                            vtField.vt );                                                // vt [in] - type to coerce to
                        fieldPtr->PutValue ( vtField );
                    }
                    return true;
                }
                catch ( _com_error &comError )
                {
                    CErrorMessage::PrintComError ( comError );
                }
            }
        }
        return false;
    }

    CAdoConnection* m_pConnection;
    ADODB::_RecordsetPtr m_pRecordSet;
 };
#endif
