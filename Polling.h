#pragma once

#include "AdoConnection.h"
#include "ProfileValues.h"
#include "EventTrace.h"
#include "Application.h"

class CApplication;

class CPolling
{
private:
	bool m_bShutDown;
	bool m_bThreadRunning;
	CEventTrace m_EventTrace;
	CApplication* m_pApplication;

public:

	CPolling(CApplication* pApp) :
		m_pApplication(pApp),
		m_bShutDown ( false ),
		m_bThreadRunning ( false )
	{
		HANDLE hMonitorThread = CreateThread( NULL, 0, InitializePolling, this, 0, NULL );
	}

	virtual ~CPolling(void)
	{
	}

	bool Shutdown ( void )
	{
		m_bShutDown = true;
		while ( m_bThreadRunning == true )
		{
			Sleep( 250 );
		}
		return true;
	}

protected:
	static DWORD WINAPI InitializePolling( LPVOID lpParam )
	{
		CoInitialize(NULL);
		CPolling* polling = (CPolling*)lpParam;
		polling->m_bThreadRunning = true;
		polling->ThreadProc();
		polling->m_bThreadRunning = false;
		CoUninitialize();
		return 0;
	}

	void ThreadProc(void)
	{
		m_EventTrace.Event( CEventTrace::Details, "START -- void CPolling::ThreadProc(void)" );
		while ( m_bShutDown == false )
		{
			WaitMessage();
		}
	}

	void WaitMessage (void)
	{
#if FALSE
 procedure getManualPoll (
                   pi_modems    in     smallint,
                   pi_sockets   in     smallint,
                   po_auditor      out varchar2,
                   po_ein          out integer,
                   po_call_start_time   out     timestamp,
                   po_central_auditor out varchar2 ,
                   po_call_number out integer, 
                   po_phone_nr out varchar2
                   , pi_wait  in integer default dbms_aq.no_wait
                    );
#endif
		char szAuditor [ 1024 ];
		long EIN;
		double callStartTime;
		char szCentralAuditor [ 1024 ];
		long CallNumber;
		char szPhoneNumber [ 1024 ];

		ZeroMemory ( szAuditor, sizeof ( szAuditor ));
		EIN = 0;
		callStartTime = ( double ) 0;
		ZeroMemory ( szCentralAuditor, sizeof ( szCentralAuditor ));
		CallNumber = 0;
		ZeroMemory ( szPhoneNumber, sizeof ( szPhoneNumber ));

		CAdoConnection adoConnection;
		adoConnection.ConnectionStringOpen( CProfileValues::GetConnectionString());
		CAdoStoredProcedure adoStoredProcedure ( "PKG_COMM_SERVER.getManualPoll" );

		_variant_t vtModems (( short ) 1, VT_I2 );
		adoStoredProcedure.AddParameter("pi_modems", vtModems, ADODB::adInteger, ADODB::adParamInput, sizeof ( short ));

		_variant_t vtSockets (( short ) 0, VT_I2 );
		adoStoredProcedure.AddParameter("pi_sockets", vtSockets, ADODB::adInteger, ADODB::adParamInput, sizeof ( short ));

		_variant_t vtAuditor ( _bstr_t ( "" ));
		adoStoredProcedure.AddParameter("po_auditor", vtAuditor, ADODB::adBSTR, ADODB::adParamOutput, 64 );

		_variant_t vtEIN ( _bstr_t ( "" ));
		adoStoredProcedure.AddParameter("po_ein", vtEIN, ADODB::adBSTR, ADODB::adParamOutput, 64 );

		_variant_t vtCallStartTime (( double ) 0, VT_DATE );
		adoStoredProcedure.AddParameter( "po_call_start_time", vtCallStartTime, ADODB::DataTypeEnum::adDate, ADODB::ParameterDirectionEnum::adParamOutput, sizeof ( double ));

		_variant_t vtCentralAuditor ( _bstr_t ( "" ));
		adoStoredProcedure.AddParameter("po_central_auditor", vtCentralAuditor, ADODB::adBSTR, ADODB::adParamOutput, 64 );

		_variant_t vtCallNumber (( long ) 0, VT_I4 );
		adoStoredProcedure.AddParameter( "po_call_number", vtCallNumber, ADODB::DataTypeEnum::adInteger, ADODB::ParameterDirectionEnum::adParamOutput, sizeof ( long ));

		_variant_t vtPhoneNumber ( _bstr_t ( "" ));
		adoStoredProcedure.AddParameter("po_phone_nr", vtPhoneNumber, ADODB::adBSTR, ADODB::adParamOutput, 64 );

		bool bExecutedSuccessfully = adoConnection.ExecuteNonQuery( adoStoredProcedure, false );
		if ( bExecutedSuccessfully == false )
		{
			OutputDebugString ( "TIMEOUT OCCURRED!\n" );
			Sleep( 10000 );
			return;
		}

		vtAuditor = adoStoredProcedure.GetParameter("po_auditor");
		_bstr_t bstrAuditor (( _bstr_t ) vtAuditor );
		StringCbCopy ( szAuditor, sizeof ( szAuditor ), ( const char* ) bstrAuditor );

		vtEIN = adoStoredProcedure.GetParameter("po_ein");
		EIN = ( long ) vtEIN;

		vtCallStartTime = adoStoredProcedure.GetParameter("po_call_start_time");
		callStartTime = ( double ) vtCallStartTime;

		vtCentralAuditor = adoStoredProcedure.GetParameter("po_central_auditor");
		_bstr_t bstrCentralAuditor (( _bstr_t ) vtCentralAuditor );
		StringCbCopy ( szCentralAuditor, sizeof ( szCentralAuditor ), ( const char* ) bstrCentralAuditor );

		vtCallNumber = adoStoredProcedure.GetParameter("po_call_number");
		CallNumber = ( long ) vtCallNumber;

		vtPhoneNumber = adoStoredProcedure.GetParameter("po_phone_nr");
		_bstr_t bstrPhoneNumber (( _bstr_t ) vtPhoneNumber );
		StringCbCopy ( szPhoneNumber, sizeof ( szPhoneNumber ), ( const char* ) bstrPhoneNumber );

		m_pApplication->Dial(( const char* ) bstrPhoneNumber, CallNumber, callStartTime );
	}
};
