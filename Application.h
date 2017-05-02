#pragma once

#include "ModemNames.h"
#include "Monitor.h"
#include "ProtelList.h"
#include "SocketListener.h"
#include "ProtelSerial.h"
#include "ProtelSocket.h"
#include "ProfileValues.h"
#include "EventTrace.h"
#include <time.h>

// I am *HARD*CODING* this connection string - BECAUSE
// This function should never be part of the final program --
// This function is for testing purposes only!
//#define __MANUAL_POLL_CONNECTION_STRING__	"Provider=OraOLEDB.Oracle.1;Password=IVS2_TEST;Persist Security Info=True;User ID=IVS2_TEST;Data Source=TEST_NEW"
#define __MANUAL_POLL_CONNECTION_STRING__	"Provider=OraOLEDB.Oracle.1;Password=IVS2_DEV;User ID=IVS2_DEV;Data Source=TEST_NEW;"

class CApplication
{
private:
	struct InitializeStructure
	{
		HANDLE hShutdown;
	};

protected:
	CProtelList* m_protelList;
	HANDLE m_hShutDown;
	HANDLE m_hProfileChanged;
	bool UseModems;
	bool UseSockets;
	CMonitor* m_pMonitor;
	bool m_bThreadRunning;
	CEventTrace m_EventTrace;

public:
	CApplication(void)
	{
		CoInitialize(NULL);
		m_pMonitor = NULL;
		m_protelList = NULL;
		m_hShutDown = NULL;
		m_hProfileChanged = NULL;

		m_bThreadRunning = false;
		CProfileValues profileValues;

		UseModems = profileValues.GetUseModems();
		UseSockets = profileValues.GetUseSockets();
	}

	virtual ~CApplication(void)
	{
#ifdef __DEBUG_MEMORY_CHECK_UTILITIES__
#ifdef _DEBUG
		_CrtMemDumpAllObjectsSince( NULL );
		_CrtDumpMemoryLeaks();
#endif	//	_DEBUG
#endif	//	__DEBUG_MEMORY_CHECK_UTILITIES__
		CoUninitialize();
	}

	bool Start( void )
	{
		// This (CMonitor::InitializeMonitor) needs to be the first function executed!
		// This function makes a call into the Oracle database and waits to return until
		// the oracle DLLs are loaded and the oracle procedure complete!!!
		m_pMonitor = new CMonitor();

		m_hShutDown = CreateEvent( NULL, TRUE, FALSE, NULL );
		m_hProfileChanged = CreateEvent( NULL, TRUE, FALSE, NULL );
		m_protelList = new CProtelList();

		if ( UseModems == true )
		{
			CEventTrace eventTrace;

			CModemNames ModemNames;
			int Offset = 0;
			for ( int Offset = 0; Offset < ModemNames.Count; Offset++ )
			{
				char szPortName [ 1024 ];
				StringCchPrintf ( szPortName, sizeof ( szPortName ), "\\\\.\\%s", ModemNames.Ports[Offset] );

				//eventTrace.Event( CEventTrace::Information, "%s\t%s\r\n", szPortName, ModemNames.Names[Offset] );
				eventTrace.BeginXML ( CEventTrace::Information );
				eventTrace.XML( CEventTrace::Information, "Port", ( char* )ModemNames.Ports[Offset] );
				eventTrace.XML( CEventTrace::Information, "Modem", ( char* )ModemNames.Names[Offset] );
				eventTrace.XML( CEventTrace::Information, "Win32", szPortName );
				eventTrace.EndXML ( CEventTrace::Information );

				CProtelSerial* protelSerial = new CProtelSerial ( m_hShutDown, ModemNames.Names[Offset], szPortName, CommandTermination::CRLF );
				m_protelList->Add( protelSerial );
			}
		}

		if ( UseSockets == true )
		{
			CSocketListener::InitializeListener( m_protelList, &m_hShutDown );
		}

		HANDLE hMonitorThread = CreateThread( NULL, 0, InitializePolling, this, 0, NULL );

#if _DEBUG
		//Sleep( 15000 );
		//AddManualPoll("6073509", "ROJ75917");
		//AddManualPoll("6073509", "ROJ75917");
		//AddManualPoll("6073509", "ROJ75917");
		//////AddManualPoll("6073509");
		//////AddManualPoll("8137132758");
		//////AddManualPoll("6073509");
#endif	//	_DEBUG

		return true;
	}

	bool Stop( void )
	{
#ifdef _DEBUG
		OutputDebugString ( "CApplication::Stop()\n" );
#endif
		if( m_protelList != NULL )
		{
			CProtelHost* protelHost = m_protelList->MoveFirst();
			while ( protelHost != NULL )
			{
#ifdef _DEBUG
				OutputDebugString ( "CApplication::Stop() -->Shutting down CProtelHost\n" );
#endif
				protelHost->Shutdown();
				protelHost = m_protelList->Next();
			}
		}

		if ( m_pMonitor != NULL )
		{
#ifdef _DEBUG
			OutputDebugString ( "CApplication::Stop() -->Shutting down CMonitor\n" );
#endif
			m_pMonitor->Shutdown();
			delete m_pMonitor;
			m_pMonitor = NULL;
		}

		Sleep ( 250 );
		SetEvent( m_hShutDown );
		Sleep ( 250 );

		if( m_protelList != NULL )
		{
#ifdef _DEBUG
			OutputDebugString ( "CApplication::Stop() -->CProtelList::RemoveAll()\n" );
#endif
			m_protelList->RemoveAll();
			delete m_protelList;
			m_protelList = NULL;
		}

		CloseHandle( m_hShutDown );
		m_hShutDown = NULL;

		CloseHandle( m_hProfileChanged );
		m_hProfileChanged = NULL;

		return true;
	}

	bool GetSockets ( void )
	{
		return UseSockets;
	}
	void SetSockets ( bool bSockets )
	{
		UseSockets = bSockets;
	}


	bool GetModems ( void )
	{
		return UseModems;
	}
	void SetModems ( bool bModem )
	{
		UseModems = bModem;
	}




protected:
	static DWORD WINAPI InitializePolling( LPVOID lpParam )
	{
		CoInitialize(NULL);
		CApplication* application = (CApplication*)lpParam;
		application->m_bThreadRunning = true;
		application->ThreadProc();
		application->m_bThreadRunning = false;
		CoUninitialize();
		return 0;
	}

	void ThreadProc(void)
	{
		if( m_protelList == NULL )
		{
			return;
		}

		DWORD dwSeconds = 0;
		{
			CProfileValues profileValues;
			dwSeconds = profileValues.GetManualPolling();
		}
		DWORD WaitTime = dwSeconds * 1000;
		//DWORD Seconds = 1000;
		//DWORD Minutes = Seconds * 60;
		//DWORD WaitTime = Minutes * 5;
		//m_EventTrace.Event( CEventTrace::Details, "START -- void CApplication::ThreadProc(void)" );
		while ( true )
		{
			DWORD dwResult = WaitForSingleObject( m_hShutDown, WaitTime );
			if ( dwResult == WAIT_OBJECT_0 )
			{
				break;
			}
			GetManualPoll();
		}
	}

	void GetManualPoll (void)
	{
		// First--See if *any* Modems are capable of making an outbound call!
		// If there aren't any available, there is no use to dequeue a request
		// and go through the rest of the procedure.

		time_t TargetTime;
		time ( &TargetTime );
		TargetTime -= 60;

		CProtelHost* protelHost = m_protelList->MoveFirst();
#if FALSE
		while ( protelHost != NULL )
		{
			CProtelSerial* protelSerial = dynamic_cast<CProtelSerial*>(protelHost);
			if ( protelSerial != NULL )
			{
				if ( protelSerial->AvailableToDial() == false )
				{
					if ( protelSerial->LastActivity < TargetTime )
					{
						protelSerial->StopThisCall();
					}
				}
			}
			protelHost = m_protelList->Next();
		}
#endif

		bool AvailableToDial = false;
		protelHost = m_protelList->MoveFirst();
		while ( protelHost != NULL )
		{
			CProtelSerial* protelSerial = dynamic_cast<CProtelSerial*>(protelHost);
			if ( protelSerial != NULL )
			{
				if ( protelSerial->AvailableToDial() == true )
				{
					AvailableToDial = true;
					break;
				}
			}
			protelHost = m_protelList->Next();
		}
		if ( AvailableToDial == false )
		{
			return;
		}
#if 0
		static bool bFirstPass = true;
		if ( bFirstPass == true )
		{
			bFirstPass = false;
			CProtelHost* protelHost = m_protelList->MoveFirst();
			while ( protelHost != NULL )
			{
				CProtelSerial* protelSerial = dynamic_cast<CProtelSerial*>(protelHost);
				if ( protelSerial != NULL )
				{
					if ( protelSerial->Dial( "6073509", 0, ( double ) 0 ) == true )
					{
						break;
					}
				}
				protelHost = m_protelList->Next();
			}
		}
		return;
#endif
		//procedure getManualPoll (
		//   po_call_number out integer, 
		//   pi_modems    in     smallint,
		//   pi_sockets   in     smallint,
		//   po_auditor      out varchar2,
		//--                   po_esn          out varchar2, --Removed per email with David p
		//   po_call_start_time   in     timestamp default null,
		//   po_central_auditor in varchar2  default null,                   
		//   po_phone_nr out varchar2
		//   , pi_call_type integer default 1 --Land line
		//   , pi_wait  in integer default dbms_aq.no_wait
		//    );
		char szAuditor [ 1024 ];
		long EIN;
		double callStartTime;
		char szCentralAuditor [ 1024 ];
		long CallNumber;
		char szPhoneNumber [ 1024 ];

		char szCallStartTime [ 64 ];
		char szCallStartTimeMS [ 64 ];
		char szCallNumber [ 64 ];

		ZeroMemory ( szAuditor, sizeof ( szAuditor ));
		EIN = 0;
		callStartTime = ( double ) 0;
		ZeroMemory ( szCentralAuditor, sizeof ( szCentralAuditor ));
		CallNumber = 0;
		ZeroMemory ( szPhoneNumber, sizeof ( szPhoneNumber ));

		CProfileValues profileValues;
		CAdoConnection adoConnection;
		adoConnection.ConnectionStringOpen( profileValues.GetConnectionString());
		CAdoStoredProcedure adoStoredProcedure ( "PKG_COMM_SERVER.getManualPoll" );

		_variant_t vtCallNumber (( long ) 0, VT_I4 );
		adoStoredProcedure.AddParameter( "po_call_number", vtCallNumber, ADODB::DataTypeEnum::adInteger, ADODB::ParameterDirectionEnum::adParamOutput, sizeof ( long ));

		_variant_t vtModems (( short ) 1, VT_I2 );
		adoStoredProcedure.AddParameter("pi_modems", vtModems, ADODB::adInteger, ADODB::adParamInput, sizeof ( short ));

		_variant_t vtSockets (( short ) 0, VT_I2 );
		adoStoredProcedure.AddParameter("pi_sockets", vtSockets, ADODB::adInteger, ADODB::adParamInput, sizeof ( short ));

		_variant_t vtAuditor ( _bstr_t ( "" ));
		adoStoredProcedure.AddParameter("po_auditor", vtAuditor, ADODB::adBSTR, ADODB::adParamOutput, 64 );

// Added to force getmanual poll to work.... via bug 3016 7/7/2011   adParamInputOutput
		double TdCallStartTime;
        SYSTEMTIME TCallStartTime;
        GetSystemTime ( &TCallStartTime );
        SystemTimeToVariantTime ( &TCallStartTime, &TdCallStartTime );
// end added code
            _variant_t vtCallStartTime ( TdCallStartTime, VT_DATE );     // Note: time was set by ProtelHost::Initialize
            adoStoredProcedure.AddParameter( "pio_CALL_START_TIME", vtCallStartTime, ADODB::DataTypeEnum::adDate, ADODB::ParameterDirectionEnum::adParamInputOutput, sizeof ( double ));
// this code works and will replace the 2 commented out lines below. WJS 7/7/2011

		//_variant_t vtCallStartTime (( double ) VT_NULL, VT_DATE );
		//adoStoredProcedure.AddParameter( "po_call_start_time", vtCallStartTime, ADODB::DataTypeEnum::adDBTimeStamp, ADODB::ParameterDirectionEnum::adParamOutput, sizeof ( double ));

		_variant_t vtCentralAuditor ( _bstr_t ( "" ));
		adoStoredProcedure.AddParameter("po_central_auditor", vtCentralAuditor, ADODB::adBSTR, ADODB::adParamOutput, 64 );

		_variant_t vtPhoneNumber ( _bstr_t ( "" ));
		adoStoredProcedure.AddParameter("po_phone_nr", vtPhoneNumber, ADODB::adBSTR, ADODB::adParamOutput, 64 );

		_variant_t vtCallType (( short ) 1, VT_I2 );
		adoStoredProcedure.AddParameter("pi_call_type", vtCallType, ADODB::adInteger, ADODB::adParamInput, sizeof ( short ));


#if _DEBUG
		//adoConnection.StoredProcedureTrace ( &adoStoredProcedure );
#endif

		bool bExecutedSuccessfully = adoConnection.ExecuteNonQuery( adoStoredProcedure, false );
		if ( bExecutedSuccessfully == false )
		{
			//OutputDebugString ( "TIMEOUT OCCURRED!\n" );
			return;
		}

		vtAuditor = adoStoredProcedure.GetParameter("po_auditor");
		_bstr_t bstrAuditor (( _bstr_t ) vtAuditor );
		StringCbCopy ( szAuditor, sizeof ( szAuditor ), ( const char* ) bstrAuditor );

		vtCallStartTime = adoStoredProcedure.GetParameter("pio_call_start_time");
		callStartTime = ( double ) vtCallStartTime;

		vtCentralAuditor = adoStoredProcedure.GetParameter("po_central_auditor");
		_bstr_t bstrCentralAuditor = bstrAuditor;
		if ( vtCentralAuditor.vt != VT_NULL && vtCentralAuditor.vt != VT_EMPTY )
		{
			bstrCentralAuditor = ( _bstr_t ) vtCentralAuditor;
		}
		StringCbCopy ( szCentralAuditor, sizeof ( szCentralAuditor ), ( const char* ) bstrCentralAuditor );

		vtCallNumber = adoStoredProcedure.GetParameter("po_call_number");
		CallNumber = ( long ) vtCallNumber;

		vtPhoneNumber = adoStoredProcedure.GetParameter("po_phone_nr");
		_bstr_t bstrPhoneNumber (( _bstr_t ) vtPhoneNumber );
		StringCbCopy ( szPhoneNumber, sizeof ( szPhoneNumber ), ( const char* ) bstrPhoneNumber );

		_bstr_t bstrCallStartTime (( _bstr_t ) vtCallStartTime );

		StringCchPrintf ( szCallNumber, sizeof ( szCallNumber ), "%d", CallNumber );
		StringCchPrintf ( szCallStartTime, sizeof ( szCallStartTime ), "%f", callStartTime );


		SYSTEMTIME SystemTimeMS;
		VariantTimeToSystemTime( callStartTime, &SystemTimeMS );
		StringCchPrintf ( szCallStartTimeMS, sizeof ( szCallStartTimeMS ), "%02d/%02d/%04d %02d:%02d:%02d.%03d", SystemTimeMS.wMonth, SystemTimeMS.wDay, SystemTimeMS.wYear, SystemTimeMS.wHour, SystemTimeMS.wMinute, SystemTimeMS.wSecond, SystemTimeMS.wMilliseconds );

		//m_EventTrace.BeginXML ( CEventTrace::Information );
		//m_EventTrace.XML ( CEventTrace::Information, "GetManualPoll", "Found Record" );
		//m_EventTrace.XML ( CEventTrace::Information, "CallNumber", szCallNumber );
		//m_EventTrace.XML ( CEventTrace::Information, "PhoneNumber", szPhoneNumber );
		//m_EventTrace.XML ( CEventTrace::Information, "Auditor", ( char* )(( LPCTSTR ) bstrAuditor ));
		//m_EventTrace.XML ( CEventTrace::Information, "CallStartTime", szCallStartTimeMS );
		//m_EventTrace.BeginXML ( CEventTrace::Information );

		protelHost = m_protelList->MoveFirst();
		while ( protelHost != NULL )
		{
			CProtelSerial* protelSerial = dynamic_cast<CProtelSerial*>(protelHost);
			if ( protelSerial != NULL )
			{
				if ( protelSerial->Dial( szPhoneNumber, CallNumber ) == true )
				{
					return;
				}
			}
			protelHost = m_protelList->Next();
		}
		try
		{
			CAdoStoredProcedure adoStoredProcedure2 ( "PKG_COMM_SERVER.FINISH" );
			//procedure finish ( 
			//    pi_callnumber in integer
			//    , pi_centralAuditor in varchar2 default null
			//    , pi_callstarttime in timestamp default null
			//    , pi_callstoptime in timestamp
			//    , pi_success in integer 
			//    , pi_errormsg in varchar2 default null
			//);
			adoStoredProcedure2.AddParameter( "po_call_number", vtCallNumber, ADODB::DataTypeEnum::adInteger, ADODB::ParameterDirectionEnum::adParamInput, sizeof ( long ));
			adoStoredProcedure2.AddParameter( "pi_centralAuditor", vtCentralAuditor, ADODB::DataTypeEnum::adBSTR, ADODB::ParameterDirectionEnum::adParamInput, bstrCentralAuditor.length());
			adoStoredProcedure2.AddParameter( "pi_callstarttime", vtCallStartTime, ADODB::DataTypeEnum::adDBTimeStamp, ADODB::ParameterDirectionEnum::adParamInput, sizeof ( double ));
			adoStoredProcedure2.AddParameter( "pi_callstoptime", vtCallStartTime, ADODB::DataTypeEnum::adDBTimeStamp, ADODB::ParameterDirectionEnum::adParamInput, sizeof ( double ));

			_variant_t vtSuccess (( short ) 0, VT_I2 );
			adoStoredProcedure2.AddParameter( "pi_success", vtSuccess, ADODB::DataTypeEnum::adInteger, ADODB::ParameterDirectionEnum::adParamInput, sizeof ( short ));

			_bstr_t bstrMessage( "No available modems for outbound dial");
			_variant_t vtMessage ( bstrMessage );
			adoStoredProcedure2.AddParameter( "pi_errormsg", vtMessage, ADODB::DataTypeEnum::adBSTR, ADODB::ParameterDirectionEnum::adParamInput, bstrMessage.length());

			//m_EventTrace.BeginXML ( CEventTrace::Information );
			//m_EventTrace.XML ( CEventTrace::Information, "GetManualPoll", "Failure" );
			//m_EventTrace.XML ( CEventTrace::Information, "CentralAuditor", ( char* )(( LPCTSTR ) bstrCentralAuditor ));
			//m_EventTrace.XML ( CEventTrace::Information, "ErrorMessage", ( char* )(( LPCTSTR ) bstrMessage ));
			//m_EventTrace.XML ( CEventTrace::Information, "CallStartTime", ( char* )(( LPCTSTR ) bstrCallStartTime ));
			//m_EventTrace.XML ( CEventTrace::Information, "callStartTime", szCallStartTime );
			//m_EventTrace.XML ( CEventTrace::Information, "CallNumber", szCallNumber );
			//m_EventTrace.EndXML ( CEventTrace::Information );
			adoConnection.ExecuteNonQuery( adoStoredProcedure2, false );
		}
		catch ( _com_error &comError )
		{
			m_EventTrace.Event ( CEventTrace::Information, "CApplication::GetManualPoll <--> ERROR: %s", CErrorMessage::ReturnComErrorMessage ( comError ));
            CAdoStoredProcedure szOracleProcedureName (  "PKG_COMM_SERVER.addSysLogRecAutonomous" );
            _bstr_t bstrOutText( "CApplication::GetManualPoll <--> ERROR: %s" );
            _variant_t vtszOutput ( bstrOutText );
           szOracleProcedureName.AddParameter( "pi_TEXT", vtszOutput, ADODB::DataTypeEnum::adBSTR, ADODB::ParameterDirectionEnum::adParamInput, bstrOutText.length());
		}
	}

#if 0
	void AddManualPoll (char* pszPhoneNumber, char* pszMonitorSerialNumber = NULL)
	{
		if ( pszMonitorSerialNumber == NULL )
		{
			m_EventTrace.EventXML ( CEventTrace::Information, "AddManualPoll", "Adding . . .", "PhoneNumber", pszPhoneNumber, "Monitor", "null", NULL  );
		}
		else
		{
			m_EventTrace.EventXML ( CEventTrace::Information, "AddManualPoll", "Adding . . .", "PhoneNumber", pszPhoneNumber, "Monitor", pszMonitorSerialNumber, NULL  );
		}
		try
		{
			// I am *HARD*CODING* this connection string - BECAUSE
			// This function should never be part of the final program --
			// This function is for testing purposes only!
			CAdoConnection adoConnection;
			adoConnection.ConnectionStringOpen( __MANUAL_POLL_CONNECTION_STRING__ );
			CAdoStoredProcedure adoStoredProcedure ( "manual_poll_q_pkg.enqueue_request" );


			//procedure enqueue_request (
			//	pi_CENTRAL_MONITORID   varchar2    --The Central Monitor at time of request
			//	, pi_MACHINEID         INTEGER     --Unique identifier of Machine record for which the manual poll was requested.
			//	, pi_MONITORID         VARCHAR2    --Monitor assigned to the machine at time of request.
			//	, pi_PHONE_NR          VARCHAR2    --The Phone Number passed for the CS to dial to reach the monitor (land line) or use for the SMS message (Cellular)
			//	, pi_SIM_ESN           VARCHAR2    --The SIM ESN associated with the machine at the time the manual poll was requested (null=land line)
			//	, pi_REPLICATION_FIRST INTEGER     --(Boolean t/f-1/0)    Indicates of configuration data should be replicated first (default 1=true)
			//	, pi_attempts  integer default 0--The current attempts made to contact the machine
			//	, pi_max_attempts integer default 3--The maximum nubmer of attempts the comm should make
			//	, pi_userid integer default null--The userid of the person requesting the poll <future>
			//	, pi_schema varchar2 default user
			//	, pi_recipient varchar2 default gv_consumer_name
			//	, pi_priority integer default 1
			//	, pi_expiration binary_integer default dbms_aq.never
			//	);

			_bstr_t bstrMonitor ( "ROJ75917" );
			if ( pszMonitorSerialNumber != NULL )
			{
				bstrMonitor = pszMonitorSerialNumber;
			}
			_variant_t vtMonitor ( bstrMonitor );
			adoStoredProcedure.AddParameter("pi_CENTRAL_MONITORID", vtMonitor, ADODB::adBSTR, ADODB::adParamInput, bstrMonitor.length());

			_variant_t vtMachineID (( long ) -1, VT_I4 );
			adoStoredProcedure.AddParameter("pi_MACHINEID", vtMachineID, ADODB::DataTypeEnum::adInteger, ADODB::ParameterDirectionEnum::adParamInput, sizeof(long));

			adoStoredProcedure.AddParameter("pi_MONITORID", vtMonitor, ADODB::adBSTR, ADODB::adParamInput, bstrMonitor.length());

			_bstr_t bstrPhoneNumber ( pszPhoneNumber );
			_variant_t vtPhoneNumber ( bstrPhoneNumber );
			adoStoredProcedure.AddParameter("pi_PHONE_NR", vtPhoneNumber, ADODB::adBSTR, ADODB::adParamInput, bstrPhoneNumber.length());

			_bstr_t bstrEmpty ( "" );
			_variant_t vtEmptyString ( bstrEmpty );
			adoStoredProcedure.AddParameter("pi_SIM_ESN", vtEmptyString, ADODB::adBSTR, ADODB::adParamInput, bstrEmpty.length());

			_variant_t vtReplicationFirst (( long ) 0, VT_I4 );
			adoStoredProcedure.AddParameter("PI_REPLICATION_FIRST", vtReplicationFirst, ADODB::DataTypeEnum::adInteger, ADODB::ParameterDirectionEnum::adParamInput, sizeof(long));


			_variant_t vtAttempts (( long ) 0, VT_I4 );
			adoStoredProcedure.AddParameter("PI_ATTEMPTS", vtAttempts, ADODB::DataTypeEnum::adInteger, ADODB::ParameterDirectionEnum::adParamInput, sizeof(long));

			_variant_t vtMaxAttempts (( long ) 1, VT_I4 );
			adoStoredProcedure.AddParameter("PI_MAX_ATTEMPTS", vtMaxAttempts, ADODB::DataTypeEnum::adInteger, ADODB::ParameterDirectionEnum::adParamInput, sizeof(long));

			//adoConnection.StoredProcedureTrace ( &adoStoredProcedure );
			m_EventTrace.Event ( CEventTrace::Details, "CApplication::AddManualPoll -->adoConnection->ExecuteNonQuery" );
			adoConnection.ExecuteNonQuery( adoStoredProcedure, false );
			m_EventTrace.Event ( CEventTrace::Details, "CApplication::AddManualPoll <--adoConnection->ExecuteNonQuery" );
		}
		catch ( _com_error &comError )
		{
			m_EventTrace.Event ( CEventTrace::Information, "CApplication::AddManualPoll <--> ERROR: %s", CErrorMessage::ReturnComErrorMessage ( comError ));
		}
	}
#endif	//	_DEBUG

	__declspec(property(get = GetModems,put = SetModems)) bool Modems;
	__declspec(property(get = GetSockets,put = SetSockets)) bool Sockets;
};
