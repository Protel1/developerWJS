/**********************************************************************************************************************
 *                                   This file contains the CAdoStoredProcedure class.                                *
 *                                                                                                                    *
 * This is used to access a procedure stored on an Oracle database. It is constructed with the name (e.g.             *
 * "PKG_COMM_SERVER.ADDNEW") of the procedure. Note that these procedures can be viewed by logging into the database  *
 * server web interface (e.g. pvend2:8080/apex) and using Object Browser - Packages. The actual code for each         *
 * procedure is visible when Body is selected.                                                                        *
 *                                                                                                                    *
 * A procedure typically expects some named parameters. These are added using the AddParameter method of this class.  *
 * The procedure can then be executed using the ExecuteNonQuery method of the CAdoConnection to the database, passing *
 * it this class. Returned values can be obtained afterwards using the GetParameter method of this object.            *
 *                                                                                                                    *
 *                                         Copyright (c) Protel Inc. 2009-2010                                        *
 **********************************************************************************************************************/

#pragma once

#include "AdoConnection.h"
#include "ErrorMessage.h"

class CAdoStoredProcedure
 {
    friend class CAdoConnection;
//    friend class CAdoRecordset; // doesn't seem to do anything!!!!
public:
    CAdoStoredProcedure ( LPCTSTR pszCommandText )
    {
        /**************************************************************************************************************
         * CONSTRUCTOR.                                                                                               *
         **************************************************************************************************************/
        try
        {
            HRESULT hResult = m_pCommand.CreateInstance ( __uuidof  ( ADODB::Command ));
            if( hResult != S_OK )
            {
                _com_issue_error ( hResult );
            }

            _bstr_t bstrCommand ( pszCommandText );
            m_pCommand->CommandText = bstrCommand;
            m_pCommand->CommandType = ADODB::CommandTypeEnum::adCmdStoredProc;
        }
        catch (_com_error &comError)
        {
            CErrorMessage::PrintComError ( comError );
        }
    }

    virtual ~CAdoStoredProcedure(void)
    {
        /**************************************************************************************************************
         * DESTRUCTOR.                                                                                                *
         **************************************************************************************************************/
        if ( m_pCommand != NULL )
        {
            //m_pCommand->Release();
            m_pCommand = NULL;
        }
    }
    bool AddParameter ( LPCTSTR Name, _variant_t& Parameter, ADODB::DataTypeEnum Type, ADODB::ParameterDirectionEnum ParameterDirection, int ParameterSize )
    {
        /**************************************************************************************************************
         * This is used before executing the procedure to create and set the characteristics of a parameter named     *
         * Name. Note that the parameter may be output-only (see GetParameter) but a value must still be specified.   *
         **************************************************************************************************************/
        _bstr_t bstrName ( Name );                                               // NOTE: has destructor - runs on exit
        ADODB::_ParameterPtr ParameterPtr = m_pCommand->CreateParameter(
            bstrName,                                                                 // Name [in], e.g. "pi_MACHINEID"
            Type,                                                     // Type [in], e.g. ADODB::DataTypeEnum::adInteger
            ParameterDirection,                          // Direction, e.g. ADODB::ParameterDirectionEnum::adParamInput
            ParameterSize,                                                   // Size - maximum length in bytes of Value
            Parameter );                                                                           // Value - a variant
        HRESULT hResult = m_pCommand->Parameters->Append ( ParameterPtr );
        if( hResult == S_OK )
        {
            return true;
        }
        return false;
    }

    _variant_t GetParameter ( LPCTSTR Name )
    {
        /**************************************************************************************************************
         * This can be used after executing the procedure to obtain the value of the returned parameter named Name.   *
         **************************************************************************************************************/
        long nCount = m_pCommand->Parameters->Count;
        for ( int nLoop = 0; nLoop < nCount; nLoop++ )
        {
            ADODB::_ParameterPtr ParameterPtr = m_pCommand->Parameters->GetItem( _variant_t (( short ) nLoop ));
            _bstr_t bstrName = ParameterPtr->GetName();
            if ( StrCmpI (( LPCTSTR ) bstrName, Name ) == 0 )                         // this is the required parameter
            {
                return ParameterPtr->GetValue();
            }
        }
        _variant_t vtEmpty;
        return vtEmpty;                                                                       // parameter wasn't found
        //_bstr_t bstrName ( Name );
        //ADODB::_ParameterPtr ParameterPtr = m_pCommand->CreateParameter(bstrName, Type, ParameterDirection, ParameterSize );
        //HRESULT   hResult = m_pCommand->Parameters->Append ( ParameterPtr );
        //if(   hResult == S_OK )
        //{
        //  return true;
        //}
        //return false;
    }


protected:
    ADODB::_CommandPtr m_pCommand;  // the command. (It has member ActiveConnection which must be set before executing)
 };
