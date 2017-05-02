/**********************************************************************************************************************
 *                                     This file contains the CAdoConnection class.                                   *
 *                                                                                                                    *
 * This represents a connection to an Oracle database. It can be opened using its ConnectionStringOpen (e.g. from     *
 * ProtelHost) and provides methods (e.g. ExecuteNonQuery) to execute database operations. The connection is closed   *
 * by the destructor or if ConnectionStringOpen is used again.                                                        *
 *                                                                                                                    *
 * NOTE: ADO = ActiveX Data Object, in this case used specifically for Oracle access.                                 *
 *                                                                                                                    *
 *                                         Copyright (c) Protel Inc. 2009-2011                                        *
 **********************************************************************************************************************/

#pragma once

#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")
#pragma warning( push )
#pragma warning( disable : 4146 )

#define __ADO_2_7__
#if defined ( __ADO_2_7__ )
#pragma message( "Using ADO 2.7 / msado27.tlb" )
#import <C:\Program Files\Common Files\System\ado\msado27.tlb> rename("EOF", "adoEOF")
#include "msado27.tli"
#include "msado27.tlh"
#pragma message( "Using ADO2.7 TLI / TLH" )
#elif defined ( __ADO_2_6__ )
#pragma message( "Using ADO 2.6 / msado26.tlb" )
#import <C:\Program Files\Common Files\System\ado\msado26.tlb> rename("EOF", "adoEOF")
#pragma message( "Using ADO2.6 TLI / TLH" )
#ifdef  _DEBUG
#include "debug\msado26.tli"
#include "debug\msado26.tlh"
#else
#include "Release\msado26.tli"
#include "Release\msado26.tlh"
#endif
#elif defined ( __ADO_2_5__ )
#pragma message( "Using ADO 2.5 / msado25.tlb" )
#import <C:\Program Files\Common Files\System\ado\msado25.tlb> rename("EOF", "adoEOF")
#pragma message( "Using ADO 2.5 TLI / TLH" )
#ifdef  _DEBUG
#include "debug\msado25.tli"
#include "debug\msado25.tlh"
#else
#include "Release\msado25.tli"
#include "Release\msado25.tlh"
#endif
#elif defined ( __ADO_2_1__ )
#pragma message( "Using ADO 2.1 / msado21.tlb" )
#import <C:\Program Files\Common Files\System\ado\msado21.tlb> rename("EOF", "adoEOF")
#pragma message( "Using ADO 2.1 TLI / TLH" )
#ifdef  _DEBUG
#include "debug\msado21.tli"
#include "debug\msado21.tlh"
#else
#include "Release\msado21.tli"
#include "Release\msado21.tlh"
#endif
#else
#pragma message( "Using ADO 2.0 / msado20.tlb" )
#import <C:\Program Files\Common Files\System\ado\msado20.tlb> rename("EOF", "adoEOF")
#pragma message( "Using ADO 2.0 TLI / TLH" )
#ifdef  _DEBUG
#include "debug\msado20.tli"
#include "debug\msado20.tlh"
#else
#include "Release\msado20.tli"
#include "Release\msado20.tlh"
#endif
#endif


#include "AdoStoredProcedure.h"
#include "EventTrace.h"
#include "ErrorMessage.h"

// _CrtMemState memstate;


class CAdoStoredProcedure;

class CAdoConnection
 {
public:
    CAdoConnection(void)
    {
        /**************************************************************************************************************
         * CONSTRUCTOR.                                                                                               *
         **************************************************************************************************************/
        //InitializeCriticalSectionAndSpinCount ( &m_criticalSection, 0x80000400 );
        m_pConnection = NULL;
    }

    virtual ~CAdoConnection(void)
    {
        /**************************************************************************************************************
         * DESTRUCTOR. If there is a current connection, we release it.                                               *
         **************************************************************************************************************/
        //DeleteCriticalSection ( &m_criticalSection );
        if ( m_pConnection != NULL )
        {
#if 0
            try
            {
                m_pConnection->Close();
                m_pConnection = NULL;
            }
            catch (_com_error &comError)
            {
                CErrorMessage::PrintComError ( comError );
            }
#else
            try
            {
                m_pConnection.Release();
            }
            catch (_com_error &comError)
            {
                CErrorMessage::PrintComError ( comError );
            }
#endif
        }
    }

    bool ConnectionStringOpen ( char* pszConnection )
    {
        /**************************************************************************************************************
         * This releases any existing connection, then uses connection string pszConnection to open a database        *
         * connection. It returns TRUE on success.                                                                    *
         **************************************************************************************************************/
        //OutputDebugString ( "CAdoConnection::ConnectionStringOpen ( \"" );
        //OutputDebugString ( pszConnection );
        //OutputDebugString ( "\" )\n" );
        if ( m_pConnection != NULL )
        {
#if 0
            if ( m_pConnection->State == ADODB::ObjectStateEnum::adStateOpen )
            {
                m_pConnection->Close();
            }
            //m_pConnection.Release();
#else
            try
            {
                m_pConnection.Release();
            }
            catch (_com_error &comError)
            {
                CErrorMessage::PrintComError ( comError );
            }
#endif
            m_pConnection = NULL;
        }
        try
        {
            HRESULT hResult = m_pConnection.CreateInstance ( __uuidof ( ADODB::Connection ));
            if( hResult != S_OK )
            {
                _com_issue_error ( hResult );
            }

            _bstr_t bstrConnection ( pszConnection );
            _variant_t vtEmpty ( DISP_E_PARAMNOTFOUND, VT_ERROR );
            _bstr_t bstrEmpty ( L"" );
            m_pConnection->ConnectionString = bstrConnection;
            m_pConnection->Open ( bstrEmpty, bstrEmpty, bstrEmpty, -1 );
        }
        catch (_com_error &comError)
        {
            CErrorMessage::PrintComError ( comError );
//_CrtMemDumpAllObjectsSince(&memstate);
//_CrtMemCheckpoint(&memstate);
            m_pConnection = NULL;                                                      // make sure we know it's failed
            return false;
        }
        return true;
    }

    bool udlOpen ( char* pszUdlFileName = NULL )
    {
        /**************************************************************************************************************
         * This can be used to open a connection using details stored in the Universal Data Link (UDL) file specified *
         * by pszUdlFileName or, if this is null or zero-length, in a udl file with a default name given by           *
         * GetUdlFileName() below. It appears to be unused.                                                           *
         **************************************************************************************************************/
        if ( m_pConnection != NULL )
        {
#if 0
            m_pConnection->Close();
#else
            try
            {
                m_pConnection.Release();
            }
            catch (_com_error &comError)
            {
                CErrorMessage::PrintComError ( comError );
            }
#endif
            m_pConnection = NULL;
        }
        try
        {
            HRESULT hResult = m_pConnection.CreateInstance ( __uuidof   ( ADODB::Connection ));
            if( hResult != S_OK )
            {
                _com_issue_error ( hResult );
            }

            _bstr_t bstrUDL;
            if ( pszUdlFileName == NULL || lstrlen ( pszUdlFileName ) < 1 )
            {
                GetUdlFileName ( bstrUDL );
            }
            else
            {
                bstrUDL = pszUdlFileName;
            }
            _bstr_t bstrConnection ( L"FILE NAME=" );
            bstrConnection += bstrUDL;
            _variant_t vtEmpty ( DISP_E_PARAMNOTFOUND, VT_ERROR );
            _bstr_t bstrEmpty ( L"" );
            m_pConnection->ConnectionString = bstrConnection;
            m_pConnection->Open ( bstrEmpty, bstrEmpty, bstrEmpty, -1 );
        }
        catch (_com_error &comError)
        {
            CErrorMessage::PrintComError ( comError );
            return false;
        }
        return true;
    }

    void GetUdlFileName ( _bstr_t& bstrUDL )
    {
        /**************************************************************************************************************
         * This generates a default UDL file name, It is only used by udlOpen() and BuildUDLFile: both of which are   *
         * currently unused.                                                                                          *
         **************************************************************************************************************/
        char szUdlFile [ 1024 ];
        ZeroMemory ( szUdlFile, sizeof ( szUdlFile ));

#if 1
        /*
         * We get the name of the file containing the executable of this process and change its extension (if any) to
         * .udl.
         */
        GetModuleFileName(
            NULL,                              // hModule [in] - handle to module requested - NULL = current executable
            szUdlFile,                                                                     // lpFilename [out] - buffer
            sizeof ( szUdlFile ));                                                   // nSize [in] - size of lpFilename
        char* pszExtension = PathFindExtension( szUdlFile );
        if ( pszExtension != NULL )
        {
            *( pszExtension ) = '\0';
        }
        StringCbCat ( szUdlFile, sizeof ( szUdlFile ), ".udl" );
#else
        StringCbCopy ( szUdlFile, BuildDocumentName ( GetDocumentFolder ( __COMPANY_PRODUCT__ ), GetExecutableName(), ".udl" ));
#endif
        bstrUDL = szUdlFile;
    }

    void BuildUDLFile ( char* pszMdbFileName, char* pszUdlFileName = NULL )
    {
        /**************************************************************************************************************
         * This can be used to create a Universal Data Link (UDL) file. It appears to be unused.                      *
         **************************************************************************************************************/
        _bstr_t bstrUDL;
        if ( pszUdlFileName == NULL || lstrlen ( pszUdlFileName ) < 1 )
        {
            GetUdlFileName ( bstrUDL );
        }
        else
        {
            bstrUDL = pszUdlFileName;
        }
        char szUDL [ 4096 ];
        HANDLE hUDL = CreateFile (
            ( const char* ) bstrUDL,                                          // lpFileName [in] - name of file to open
            GENERIC_WRITE,                                                         // dwDesiredAccess [in] - write only
            0,                                                               // dwShareMode [in] - 0 = cannot be shared
            NULL,                                             // lpSecurityAttributes [in] - NULL = cannot be inherited
            CREATE_ALWAYS,                                  // dwCreationDisposition [in] - overwrite any existing file
            FILE_ATTRIBUTE_NORMAL,                             // dwFlagsAndAttributes [in] - no special attributes set
            NULL );                                                // hTemplateFile [in] - sets attributes, NULL = none
        if ( hUDL == INVALID_HANDLE_VALUE )
        {
            char szMessage [ 4096 ];
            StringCbCopy ( szMessage, sizeof ( szMessage ), "Unable To Create File:\n" );
            StringCbCat ( szMessage, sizeof ( szMessage ), ( const char* ) bstrUDL );
            StringCbCat ( szMessage, sizeof ( szMessage ), "\n" );

            char* pszMessageBuffer;
            FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID ( LANG_NEUTRAL, SUBLANG_DEFAULT ), (char*) &pszMessageBuffer, 0, NULL );
            StringCbCat ( szMessage, sizeof ( szMessage ), pszMessageBuffer );
            LocalFree( pszMessageBuffer );
            StringCbCat ( szMessage, sizeof ( szMessage ), "\nAborting . . ." );

            MessageBox ( NULL, szMessage, "Initialization Failure", MB_OK | MB_ICONSTOP );
            exit ( EXIT_FAILURE );
        }
        char* pszUDL = "[oledb]\r\n; Everything after this line is an OLE DB initstring\r\nProvider=Microsoft.Jet.OLEDB.4.0;Persist Security Info=False;Data Source=";
        StringCbCopy ( szUDL, sizeof ( szUDL ), pszUDL );
        StringCbCat ( szUDL, sizeof ( szUDL ), pszMdbFileName );
        StringCbCat ( szUDL, sizeof ( szUDL ), "\r\n" );
        bstrUDL = szUDL;
        void* pszWUDL = ( void* )(( wchar_t* ) bstrUDL );
        int nLength = lstrlenW (( wchar_t* ) pszWUDL ) * 2;
        DWORD dwBytes = 0;
        BYTE bytePrefix [ 2 ] = { 0xff, 0xfe };
        WriteFile (
            hUDL,                                                                      // hFile [in] - file to write to
            bytePrefix,                                                                // lpBuffer [in] - data to write
            2,                                                             // nNumberOfBytesToWrite [in] - size of data
            &dwBytes,                                          // lpNumberOfBytesWritten [out] - bytes actually written
            NULL );                                                                // lpOverlapped [in, out] - not used
        WriteFile ( hUDL, pszWUDL, nLength, &dwBytes, NULL );
        CloseHandle ( hUDL );
    }


#if 0
    bool Execute ( char* pszStatement, bool bSupressMessages )
    {
        /**************************************************************************************************************
         * This appears to be unused.                                                                                 *
         **************************************************************************************************************/
        try
        {
            m_pConnection->Execute (                       // do it! Note: this is ADO Connection (vs. Command) Execute
                pszStatement,                                                                           // Command text
                NULL,                                                                      // Records affected - unused
                ADODB::adCmdText | ADODB::adExecuteNoRecords ); // evaluate as command + don't create records if no results
            return true;
        }
        catch (_com_error &comError)
        {
            if ( bSupressMessages == false )
            {
                CErrorMessage::PrintComError ( comError );
            }
        }
        return false;
    }
#endif

    bool WriteLogDB (CAdoConnection* m_padoConnection, System::String ^Name )

    {

		System::IntPtr ptr = System::Runtime::InteropServices::Marshal::StringToBSTR(Name);

		CComBSTR bstr;
		bstr.Attach(static_cast<BSTR>(ptr.ToPointer()));

            CAdoStoredProcedure szOracleProcedureName (  "PKG_COMM_SERVER.addSysLogRecAutonomous" );
 //           _bstr_t bstrOutText( *Name );
            _variant_t vtszOutput ( bstr );
           szOracleProcedureName.AddParameter( "pi_TEXT", vtszOutput, ADODB::DataTypeEnum::adBSTR, ADODB::ParameterDirectionEnum::adParamInput, sizeof(bstr));
		   m_padoConnection->ExecuteNonQuery( szOracleProcedureName, false, true );
		   return true;
	}

    bool ExecuteNonQuery ( CAdoStoredProcedure& adoStoredProcedure, bool bSupressMessages, bool OracleBlob = false )
    {
        /**************************************************************************************************************
         * This executes stored procedure adoStoredProcedure on the connected database and returns TRUE on success.   *
         *                                                                                                            *
         * OracleBlob should be set TRUE if any parameters of the stored procedure are BLOB, CLOB or NCLOB data type. *
         * (When OracleBlob is FALSE, database operations are optimised to avoid overheads associated with these      *
         * types.)                                                                                                    *
         **************************************************************************************************************/
        bool bReturnValue = false;                                                                  // in case it fails
        // Request ownership of the critical section.
        //WaitCount++;
        //printf ( "%d\t>> ExecuteNonQuery ( CAdoStoredProcedure& adoStoredProcedure )\n", WaitCount );
        //EnterCriticalSection ( &m_criticalSection );

        if ( this == NULL || m_pConnection == NULL )
        {
            return false;                                  // connection isn't open - continuing will cause a crash!!!!
        }

        try
        {
            adoStoredProcedure.m_pCommand->ActiveConnection = m_pConnection;
                                                                          // associate the command with this connection
            if ( OracleBlob == true )
            {
                ADODB::PropertyPtr prop = adoStoredProcedure.m_pCommand->Properties->GetItem ( "SPPrmsLOB" );
                prop->PutValue ( _variant_t ( VARIANT_TRUE, VT_BOOL )) ;
            }
            _variant_t recordsAffected (( long ) 0 );
            adoStoredProcedure.m_pCommand->Execute (       // do it! Note: this is ADO Command (vs. Connection) Execute
                &recordsAffected,                                                                              // [out]
                0,                                                                                 // parameters - none
                ADODB::adCmdStoredProc );                            // options - evaluate source as a stored procedure
            if ( OracleBlob == true )
            {
                ADODB::PropertyPtr prop = adoStoredProcedure.m_pCommand->Properties->GetItem ( "SPPrmsLOB" );
                prop->PutValue(_variant_t(VARIANT_FALSE,VT_BOOL)) ;
            }
            bReturnValue = true;                                                                             // success
        }
        catch (_com_error &comError)
        {
            if ( bSupressMessages == false )
            {
                /*
                 * There was an error and we are configured to record it. We do this.
                 */
                CErrorMessage::PrintComError ( comError );
                StoredProcedureTrace ( adoStoredProcedure );
            }
        }
        // Release ownership of the critical section.
        //LeaveCriticalSection ( &m_criticalSection );
        //WaitCount--;
        //printf ( "%d\t<< ExecuteNonQuery ( CAdoStoredProcedure& adoStoredProcedure )\n", WaitCount );
        return bReturnValue;
    }
    void StoredProcedureTrace ( CAdoStoredProcedure& adoStoredProcedure )
    {
        /**************************************************************************************************************
         * This records details of stored procedure adoStoredProcedure, including its parameters, as an event.        *
         **************************************************************************************************************/
        long nCount = adoStoredProcedure.m_pCommand->Parameters->Count;
        CEventTrace eventTrace;                                                    // records events - see EventTrace.h
        eventTrace.BeginXML ( CEventTrace::Details );
        eventTrace.XML ( CEventTrace::Details, "Command", ( char* )(( LPCTSTR ) adoStoredProcedure.m_pCommand->CommandText ));
        for ( int nLoop = 0; nLoop < nCount; nLoop++ )
        {
            ADODB::_ParameterPtr ParameterPtr = adoStoredProcedure.m_pCommand->Parameters->GetItem( _variant_t (( short ) nLoop ));
            _bstr_t bstrName = ParameterPtr->GetName();
            _variant_t vtValue = ParameterPtr->GetValue();
            _bstr_t bstrValue = "";
            if ( vtValue.vt == VT_NULL )
            {
                bstrValue = "null value";
            }
            else if ( vtValue.vt == VT_EMPTY )
            {
                bstrValue = "empty value";
            }
            else
            {
                bstrValue = vtValue;
            }
            eventTrace.XML ( CEventTrace::Details, ( char* )(( LPCTSTR ) bstrName ), ( char* )(( LPCTSTR ) bstrValue ));
        }
        eventTrace.EndXML( CEventTrace::Details );
    }

#if 0
    _variant_t ExecuteScalar ( CAdoStoredProcedure& adoStoredProcedure, bool bSupressMessages )
    {
        /**************************************************************************************************************
         * This appears to be unused.                                                                                 *
         **************************************************************************************************************/
        _variant_t vtReturn;
        vtReturn.Clear();
        try
        {
            adoStoredProcedure.m_pCommand->ActiveConnection = m_pConnection;
            _variant_t recordsAffected (( long ) 0 );
            ADODB::_RecordsetPtr pRecordset = adoStoredProcedure.m_pCommand->Execute ( &recordsAffected, NULL, ADODB::adExecuteRecord );
            if ( pRecordset != NULL && ( recordsAffected.lVal == 1 || recordsAffected.iVal == 1 ))
            {
                _variant_t vtIndex (( short ) 0 );
                ADODB::FieldPtr fieldPtr = pRecordset->Fields->GetItem( vtIndex );
                if ( fieldPtr->Type != ADODB::adEmpty )
                {
                    vtReturn = fieldPtr->GetValue();
                }
            }
        }
        catch (_com_error &comError)
        {
            if ( bSupressMessages == false )
            {
                CErrorMessage::PrintComError ( comError );
            }
        }
        return vtReturn;
    }

    // Parameter Type:
    // adCmdUnspecified -1 Does not specify the command type argument.
    // adCmdText 1 Evaluates CommandText as a textual definition of a command or stored procedure call.
    // adCmdTable 2 Evaluates CommandText as a table name whose columns are all returned by an internally generated SQL query.
    // adCmdStoredProc 4 Evaluates CommandText as a stored procedure name.
    // adCmdUnknown 8 Default. Indicates that the type of command in the CommandText property is not known.
    // adCmdFile 256 Evaluates CommandText as the file name of a persistently stored Recordset. Used with Recordset.Open or Requery only.
    // adCmdTableDirect 512 Evaluates CommandText as a table name whose columns are all returned. Used with Recordset.Open or Requery only. To use the Seek method, the Recordset must be opened with adCmdTableDirect.
    //This value cannot be combined with the ExecuteOptionEnum value adAsyncExecute.
    ADODB::_RecordsetPtr ReturnRecordset ( char* pszStatement, bool bUpdateable, long nType )       // is this used????
    {
        /**************************************************************************************************************
         * Note that this is polymorphic. It is only used by AdoRecordset which does nothing and has been disabled.   *
         **************************************************************************************************************/
        ADODB::_RecordsetPtr pRecordset = NULL;
        // Request ownership of the critical section.
        //WaitCount++;
        //printf ( "%d\t>> ReturnRecordset ( char* pszStatement )\n", WaitCount );
        //EnterCriticalSection ( &m_criticalSection );

        try
        {
            if ( bUpdateable == true )
            {
                HRESULT hResult = pRecordset.CreateInstance ( __uuidof  ( ADODB::Recordset ));
                pRecordset->Open ( pszStatement, (IDispatch *)m_pConnection, ADODB::adOpenDynamic, ADODB::adLockPessimistic, nType );
            }
            else
            {
                pRecordset = m_pConnection->Execute ( pszStatement, NULL, nType );
            }
        }
        catch (_com_error &comError)
        {
            CErrorMessage::PrintComError ( comError );
        }
        // Release ownership of the critical section.
        //LeaveCriticalSection ( &m_criticalSection );
        //WaitCount--;
        //printf ( "%d\t<< ReturnRecordset ( char* pszStatement )\n", WaitCount );
        return pRecordset;
    }

    ADODB::_RecordsetPtr ReturnRecordset ( CAdoStoredProcedure& adoStoredProcedure, bool bSupressMessages )
    {
        /**************************************************************************************************************
         * Note that this is polymorphic. It is only used by AdoRecordset which does nothing and has been disabled.   *
         **************************************************************************************************************/
        ADODB::_RecordsetPtr pRecordset = NULL;
        // Request ownership of the critical section.
        //WaitCount++;
        //printf ( "%d\t>> ReturnRecordset ( CAdoStoredProcedure& adoStoredProcedure )\n", WaitCount );
        //EnterCriticalSection ( &m_criticalSection );

        try
        {
            adoStoredProcedure.m_pCommand->ActiveConnection = m_pConnection;
            _variant_t recordsAffected (( long ) 0 );
            pRecordset = adoStoredProcedure.m_pCommand->Execute ( &recordsAffected, 0, ADODB::adOptionUnspecified);

        }
        catch (_com_error &comError)
        {
            if ( bSupressMessages == false )
            {
                CErrorMessage::PrintComError ( comError );
            }
        }
        // Release ownership of the critical section.
        //LeaveCriticalSection ( &m_criticalSection );
        //WaitCount--;
        //printf ( "%d\t<< ReturnRecordset ( CAdoStoredProcedure& adoStoredProcedure )\n", WaitCount );
        return pRecordset;
    }


    // Fields returned
    // TABLE_CATALOG DBTYPE_WSTR Catalog name. NULL if the provider does not support catalogs.
    // TABLE_SCHEMA DBTYPE_WSTR Unqualified schema name. NULL if the provider does not support schemas.
    // TABLE_NAME DBTYPE_WSTR Table name. This column cannot contain NULL.
    // TABLE_TYPE DBTYPE_WSTR Table type. One of the following or a provider-specific value: "ALIAS", "TABLE", "SYNONYM", "SYSTEM TABLE", "VIEW", "GLOBAL TEMPORARY", "LOCAL TEMPORARY", "SYSTEM VIEW", This column cannot contain NULL.
    // TABLE_GUID DBTYPE_GUID GUID that uniquely identifies the table. Providers that do not use GUIDs to identify tables should return NULL in this column.
    // DESCRIPTION DBTYPE_WSTR Human-readable description of the table. Null if there is no description associated with the column.
    // TABLE_PROPID DBTYPE_UI4 Property ID of the table. Providers that do not use PROPIDs to identify columns should return NULL in this column.
    // DATE_CREATED DBTYPE_DATE Date when the table was created or NULL if the provider does not have this information. Note that 1.x providers do not return this column.
    // DATE_MODIFIED DBTYPE_DATE Date when the table definition was last modified or NULL if the provider does not have this information., 1.x providers do not return this column.
    ADODB::_RecordsetPtr CAdoConnection::ReturnTableNames ( void )
    {
        try
        {
            return m_pConnection->OpenSchema( ADODB::adSchemaTables );
        }
        catch (_com_error &comError)
        {
            CErrorMessage::PrintComError ( comError );
            return NULL;
        }
    }
#endif

protected:
    CRITICAL_SECTION m_criticalSection;                                                             // currently unused
    ADODB::_ConnectionPtr m_pConnection;                                                              // the connection
 };

#pragma warning( pop )
