/**********************************************************************************************************************
 *                                        This file contains the CMonitor class.                                      *
 *                                                                                                                    *
 * CApplication constructs CMonitor. The constructor invokes its InitializeMonitor which spawns a ThreadProc thread.  *
 * This runs until CApplication calls CMonitor::Shutdown. The thread calls RecordHeartbeat periodically (set in       *
 * [heartbeat] minutes in the profile) - this at present doesn't appear to do anything but could presumably be        *
 * modified to place a record in the database showing that the server is running.                                     *
 *                                                                                                                    *
 *                                         Copyright (c) Protel Inc. 2009-2010                                        *
 **********************************************************************************************************************/

#pragma once

#include "AdoConnection.h"
#include "ProfileValues.h"

class CMonitor
 {
private:
    HANDLE m_hShutdown;
    CEventTrace m_EventTrace;                                                      // records events - see EventTrace.h

public:

    CMonitor(void)
    {
        /**************************************************************************************************************
         * CONSTRUCTOR.                                                                                               *
         **************************************************************************************************************/
        m_hShutdown = CreateEvent(
            NULL,                                         // lpEventAttributes [in] - NULL = handle cannot be inherited
            TRUE,                                                 // bManualReset [in] - TRUE = ResetEvent must be used
            FALSE,                                                // bInitialState [in] - FALSE = initially unsignalled
            NULL );                                                           // lpName [in] - NULL = object is unnamed
        RecordHeartbeat ( true );
        CloseHandle( CreateThread(
                NULL,                                           // lpThreadAttributes [in] - NULL = cannot be inherited
                0,                                           // dwStackSize [in] - initial stack size - 0 = use default
                InitializeMonitor,                                                // lpStartAddress [in] - in this file
                this,                                       // lpParameter [in] - parameter passed to InitializeMonitor
                0,                                         // dwCreationFlags [in] - 0 = run immediately after creation
                NULL ));                                                      // lpThreadId [out] - NULL = not returned
    }

    virtual ~CMonitor(void)
    {
        /**************************************************************************************************************
         * DESTRUCTOR.                                                                                                *
         **************************************************************************************************************/
    }

    bool Shutdown ( void )
    {
        /**************************************************************************************************************
         * This is called from CApplication during system shutdown. It signals the Monitor thread to end.             *
         **************************************************************************************************************/
        SetEvent ( m_hShutdown );
        // This sleep event is to allow the thread to exit and cleanup after itself.
        Sleep ( 250 );
        return true;
    }

private:
    static DWORD WINAPI InitializeMonitor( LPVOID lpParam )
    {
        /**************************************************************************************************************
         * This is the thread that is spawned by the class constructor. It calls ThreadProc below which continues     *
         * running until Shutdown (above) is called.                                                                  *
         **************************************************************************************************************/
        CoInitialize(NULL);                                                               // initialize the COM library
        CMonitor* monitor = (CMonitor*)lpParam;
        monitor->ThreadProc();
        CoUninitialize();                                        // close the COM library and clean up thread resources
        return 0;
    }

    void ThreadProc(void)                                                                                 // stays here
    {
        //m_EventTrace.Event( CEventTrace::Details, "START -- void CMonitor::ThreadProc(void)" );

        /*
         * We get the period to call RecordHeartbeat() according to the value (in minutes) in the profile.
         */
        DWORD dwMilliseconds = 0;
        {
            CProfileValues profileValues;
            int Minutes = profileValues.GetHeartbeat();
            dwMilliseconds = Minutes * 60;
            dwMilliseconds *= 1000;
        }

        while ( true )
        {
            /*
             * We wait up to the specified period for m_hShutdown to be signalled (by Shutdown()). If this happens,
             * we exit. Otherwise, the period must have expired. We call RecordHeartbeat() and wait again.
             */
            DWORD dwResult = WaitForSingleObject( m_hShutdown, dwMilliseconds );
            if ( dwResult == WAIT_OBJECT_0 )
            {
                break;
            }
            RecordHeartbeat( false );
        }
    }

    void RecordHeartbeat ( bool Initial )
    {
        /**************************************************************************************************************
         * This is called by the Monitor thread. It invokes stored database procedure PKG_COMM_SERVER.HEARTBEAT       *
         * which, at present, doesn't appear to do anything.                                                          *
         **************************************************************************************************************/
        //OutputDebugString ( "CMonitor:RecordHeartbeat()\n" );
        //m_EventTrace.Event( CEventTrace::Details, "CMonitor::RecordHeartbeat( %s )", Initial == true ? "true" : "false" );
        CProfileValues profileValues;
        CAdoConnection adoConnection;
        adoConnection.ConnectionStringOpen( profileValues.GetConnectionString() );
        CAdoStoredProcedure adoStoredProcedure ( "PKG_COMM_SERVER.HEARTBEAT" );

        int InitialCall = 0;
        if ( Initial == true )
        {
            InitialCall = 1;
        }
        _variant_t vtInitialHeartbeat (( short ) InitialCall, VT_I2 );
        adoStoredProcedure.AddParameter( "pi_INITIAL", vtInitialHeartbeat, ADODB::DataTypeEnum::adInteger, ADODB::ParameterDirectionEnum::adParamInput, sizeof ( short ));

         _bstr_t bstrCommServerVer( profileValues.GetCommServer() );
         _variant_t vtCommServerVer ( bstrCommServerVer );
         adoStoredProcedure.AddParameter( "pi_app_version", vtCommServerVer, ADODB::DataTypeEnum::adBSTR, ADODB::ParameterDirectionEnum::adParamInput, bstrCommServerVer.length());

        adoConnection.ExecuteNonQuery( adoStoredProcedure, false );
    }
 };
