/**********************************************************************************************************************
 *                                     This file contains the CProtelSerial class.                                    *
 *                                                                                                                    *
 * The Start method of CApplication constructs an instance of this class for each modem in the system. It sets up the *
 * port, then spawns a CProtelSerial::SerialThreadProc thread which is responsible for setting up the modem and       *
 * answering and handling possibly multiple sequential connections until the system shuts down.                       *
 *                                                                                                                    *
 * ProtelSerial inherits ProtelHost so its constructor also constructs a ProtelHost which it uses to send commands to *
 * any device connected via the modem and handle its responses.                                                       *
 *                                                                                                                    *
 *                                        Copyright (c) Protel Inc. 2009-2011                                         *
 **********************************************************************************************************************/

#pragma once
#include "ProtelHost.h"
#include <time.h>

#define __TIMEOUT_VALUE__   500                                                           // check state every 500 msec

enum CommandTermination
{
    CR,
    LF,
    CRLF
};

class CProtelSerial :
    public CProtelHost
{
protected:
    char m_szDevice [ 1024 ];                                     // modem device name (e.g. "Standard 1200 bps Modem")
    char m_szPort [ 1024 ];                                                        // modem port name (e.g. "\\.\COM3")
    char m_szModemCommandString [ 1024 ];
    HANDLE m_hComPort;                                                             // serial port associated with modem
    OVERLAPPED m_overLapped;                                                          // allows non-blocking serial I/O
    CommandTermination m_commandTermination;                                      // enumeration above (CR, LF or CRLF)

    enum                                                                                  // current state of the modem
    {
        Uninit,                                                                                       // not set up yet
        Idle,                                                                     // set up and ready to dial or answer
        Command,                                                                                // (not currently used)
        Answering,                                                                  // ATA sent, waiting for connection
        Dialing,                                                              // sending ATD and waiting for connection
        Connected,                                                                        // ready to send/receive data
        ConnCommand,                                              // connected but in command mode (not currently used)
        DiscReq,                                               // near end disconnection requested (not currently used)
        Disconnected,                                                            // carrier lost (far end disconnected)
    } m_eModemState;


    time_t lastActivity;                                    // time of last modem activity (rcvd data or status change)

public:
    CProtelSerial(HANDLE hShutDown, LPCTSTR szDevice, LPCTSTR szPort, CommandTermination commandTermination )
        : CProtelHost ( hShutDown ), m_commandTermination ( commandTermination ), lastActivity ( 0 )
    {
        /**************************************************************************************************************
         * CONSTRUCTOR. This associates the class with modem szDevice. It initialises things, then spawns a           *
         * CProtelSerial::ThreadProc which is responsible for originating, answering and handling calls via the       *
         * modem.                                                                                                     *
         **************************************************************************************************************/
        CallNumber = 0;
        dCallStartTime = ( double ) 0;
        m_eModemState = Uninit;
        char* pszPort = StrRChr ( szPort, NULL, '\\' );
        if ( pszPort == NULL )
        {
            pszPort = ( char* ) szPort;
        }
        else
        {
            pszPort++;
        }

        char szTimerName [ 1024 ];
        ZeroMemory ( szTimerName, sizeof ( szTimerName ));
        StringCbCopy ( szTimerName, sizeof ( szTimerName ), szDevice );
        StringCbCat ( szTimerName, sizeof ( szTimerName ), ":" );
        StringCbCat ( szTimerName, sizeof ( szTimerName ), pszPort );

        m_hTimer = CreateWaitableTimer (
            NULL,                                      // lpTimerAttributes [in] - NULL = defaults, cannot be inherited
            FALSE,                                                 // bManualReset [in] - FALSE = synchronisation timer
            szTimerName );                                                 // lpTimerName [in] - device:port from above

        StringCbCopy ( m_szDevice, sizeof ( m_szDevice ), szDevice );                              // modem device name
        StringCbCopy ( m_szPort, sizeof ( m_szPort ), szPort );                                      // modem port name
        m_EventTrace.Identifier ( "", GetDevice(),GetPort());
        //m_EventTrace.Event( CEventTrace::Details, "CProtelSerial::CProtelSerial -- m_hTimer = CreateWaitableTimer ( NULL, TRUE, %s );", szTimerName );
        DWORD dwLastError = GetLastError();
        if (( m_hTimer != NULL && dwLastError == ERROR_ALREADY_EXISTS ) || m_hTimer == NULL )
        {
            m_EventTrace.Event( CEventTrace::Error, "CreateWaitableTimer failed -- %s", CErrorMessage::ErrorMessageFromSystem ( dwLastError ));
        }

        //m_EventTrace.Event( CEventTrace::Details, "CProtelSerial::CProtelSerial(char* szDevice, char* szPort)" );

        ZeroMemory ( &m_overLapped, sizeof ( m_overLapped ));
        m_overLapped.hEvent = CreateEvent(
            NULL,                                         // lpEventAttributes [in] - NULL = handle cannot be inherited
            TRUE,                                                 // bManualReset [in] - TRUE = ResetEvent must be used
            FALSE,                                                // bInitialState [in] - FALSE = initially unsignalled
            NULL );                                                           // lpName [in] - NULL = object is unnamed

        m_hComPort = ::CreateFile (
            m_szPort,                                                       // lpFileName [in] - name of device to open
            GENERIC_READ|GENERIC_WRITE,                                        // dwDesiredAccess [in] - read and write
            0,                                                               // dwShareMode [in] - 0 = cannot be shared
            NULL,                                             // lpSecurityAttributes [in] - NULL = cannot be inherited
            OPEN_EXISTING,                                 // dwCreationDisposition [in] - open already existing device
            FILE_FLAG_OVERLAPPED,                                       // dwFlagsAndAttributes [in] - non-blocking I/O
            NULL );                                                // hTemplateFile [in] - sets attributes, NULL = none

        PurgeComm ( m_hComPort, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR );

        DCB dcb;
        ZeroMemory ( &dcb, sizeof ( dcb ));
        dcb.DCBlength = sizeof ( dcb );
        GetCommState( m_hComPort, &dcb );

        dcb.BaudRate = CBR_115200;
//        dcb.BaudRate = CBR_1200;
        dcb.ByteSize = 8;
        dcb.Parity = NOPARITY;
        dcb.StopBits = ONESTOPBIT;
 
        SetCommState( m_hComPort, &dcb );

        DWORD dwThreadId;                                                                             // not needed!!!!

        /*
         * We spawn a thread that handles connections. This returns a handle which we don't use: we close it
         * immediately to avoid a handle leak. Note that this doesn't affect the thread - it continues to run to
         * completion.
         */
        CloseHandle(CreateThread(
                NULL,                                           // lpThreadAttributes [in] - NULL = cannot be inherited
                0,                                           // dwStackSize [in] - initial stack size - 0 = use default
                SerialThreadProc,                                                 // lpStartAddress [in] - in this file
                this,                                        // lpParameter [in] - parameter passed to SerialThreadProc
                0,                                         // dwCreationFlags [in] - 0 = run immediately after creation
                &dwThreadId ));                                                // lpThreadId [out] - Can it be NULL????
        //Reset();
//        Shutdown();
    }

    virtual ~CProtelSerial(void)
    {
        /**************************************************************************************************************
         * DESTRUCTOR.                                                                                                *
         **************************************************************************************************************/
        CloseHandle ( m_overLapped.hEvent );
        CloseHandle ( m_hComPort );
        //m_EventTrace.Event( CEventTrace::Details, "CProtelSerial::~CProtelSerial(void)" );
    }

    bool AvailableToDial(void)
    {
        /**************************************************************************************************************
         * The polling thread spawned by CApplication calls this to see if the associated modem is available to make  *
         * a polling call. If so, it returns true.                                                                    *
         *                                                                                                            *
         * Note: we check for a database connection, ringing in or carrier (RLSD) as well as checking that the modem  *
         * is idle. There are conditions where the modem could be marked as idle but one of these conditions would be *
         * just starting or not yet ended.                                                                            *
         **************************************************************************************************************/
#ifdef _DEBUG
        OutputDebugString ( "bool AvailableToDial(void)\n" );
#endif
        if ( m_padoConnection != NULL )                       // there is an active database connection - modem is busy
        {
#ifdef _DEBUG
            OutputDebugString ( "m_padoConnection is not NULL!\n" );
#endif
            return false;
        }

        DWORD dwModemStatus;
        GetCommModemStatus ( m_hComPort, &dwModemStatus );
        DisplayCommunicationsStatus();
        if( dwModemStatus & MS_RING_ON )                                       // the modem is ringing in so can't dial
        {
#ifdef _DEBUG
            OutputDebugString ( "The ring indicator signal is on.!\n" );
#endif
            return false;
        }
        else if( dwModemStatus & MS_RLSD_ON )       // Receive Line Signal Detect is on - consider checking DSR too!!!!
        {
#ifdef _DEBUG
            OutputDebugString ( "The RLSD (receive-line-signal-detect) signal is on.!\n" );
#endif
            return false;                                                                                 // can't dial
        }
        return m_eModemState == Idle;                                                      // can dial if modem is Idle
    }

    bool Dial(LPCTSTR pszNumber, long callNumber)
    {
        /**************************************************************************************************************
         * The polling thread spawned by CApplication calls this to start a polling call to callNumber via the        *
         * associated modem. If the modem is available to dial, the call is started and true is returned. This        *
         * ProtelSerial continues handling the call and cleans up after. If the modem is not available, false is      *
         * returned and the calling thread must do the cleanup.                                                       *
         **************************************************************************************************************/

        //m_EventTrace.BeginXML ( CEventTrace::Information );
        //m_EventTrace.XML ( CEventTrace::Information, "Dial", "Requested" );
        //m_EventTrace.XML ( CEventTrace::Information, "Modem", GetDevice());
        //m_EventTrace.XML ( CEventTrace::Information, "Port", GetPort());
        //m_EventTrace.EndXML ( CEventTrace::Information );
        if ( AvailableToDial() == false )                                                       // modem is unavailable
        {
            return false;
        }

        m_eModemState = Dialing;
        char szCallNumber [ 64 ];
        _itoa_s( callNumber, szCallNumber, sizeof ( szCallNumber ), 10 );

        Initialize();
        CallNumber = callNumber;

        _variant_t vtDate ( dCallStartTime, VT_DATE );
        _bstr_t bstrDate = ( _bstr_t ) vtDate;

        //m_EventTrace.Identifier( szCallNumber, GetDevice(), GetPort());
        //m_EventTrace.BeginXML ( CEventTrace::Information );
        //m_EventTrace.XML ( CEventTrace::Information, "Dial", ( char* ) pszNumber );
        //m_EventTrace.XML ( CEventTrace::Information, "CallNumber", szCallNumber );
        //m_EventTrace.XML ( CEventTrace::Information, "CallTime", ( char* )(( LPCTSTR ) bstrDate ));
        //m_EventTrace.EndXML ( CEventTrace::Information );

        DWORD dwBytesWritten = 0;
        char szDialString [ 1024 ];
        ZeroMemory ( szDialString, sizeof ( szDialString ));
#if 0
        StringCbCopy ( szDialString, sizeof ( szDialString ), "AT M1 DT" );
#else
        StringCbCopy ( szDialString, sizeof ( szDialString ), "AT M0 DT" );
#endif
        StringCbCat ( szDialString, sizeof ( szDialString ), pszNumber );
        SendCommandToModem ( szDialString );                                                                    // dial
        SetTimeoutTimer( m_hTimer, 60);                                                 // 60 second timeout for answer

        return true;
    }

    virtual void Send(LPBYTE pszBuffer, int BufferLength)
    {
        /**************************************************************************************************************
         * This overrides the Send method of ProtelHost. It sends BufferLength bytes from pszBuffer to or via the     *
         * modem (depending if it is command or data mode).                                                           *
         **************************************************************************************************************/
        DWORD dwBytesWritten = 0;
        WriteFile (
            m_hComPort,                                                               // hFile [in] - port to output to
            pszBuffer,                                                                // lpBuffer [in] - data to output
            BufferLength,                                                  // nNumberOfBytesToWrite [in] - size of data
            &dwBytesWritten,             // lpNumberOfBytesWritten [out] - bytes actually written. Can this be NULL????
            &m_overLapped );                                     // lpOverlapped [in, out] - allows non-blocking output
    }

    virtual void Shutdown ( void )
    {
        /**************************************************************************************************************
         * This overrides the Shutdown method of ProtelHost. It sends commands to disconnect and initialise the       *
         * modem.                                                                                                     *
         **************************************************************************************************************/

        if ( m_eModemState == Idle )
            return;                                             // already disconnected and initialised - nothing to do

        /*
         * We send an H0 to ensure the modem disconnects and an &F to reset it to factory defaults. (Using Z to reset
         * a modem is unwise: you never know what strange configuration someone might have previously stored in it.)
         */
        SendCommandToModem("AT H0 &F");
        SendCommandToModem("AT E0");			// echo off WJS 4/14/2011

        /*
         * We send the following modem configuration options:
         *  M0 to silence the speaker
         *  &C1 to allow the DCD line to be used to detect far-end disconnection
         *  &D2 to allow the DTR line to be used to disconnect and return to command mode
         *  S10=14 to ensure industry-standard carrier loss detection (some UPMS1200s are odd)
         */
        SendCommandToModem("AT M0 &C1 &D2 S10=14");			//WJS 4/12/2011
        //SendCommandToModem("AT M0 &C1 &D2 S10=14 S0=3");
        m_eModemState = Idle;                                                    // mark the modem initialised and idle
    }


protected:
    bool SendCommandToModem(LPCTSTR pszModemString)
    {
        /**************************************************************************************************************
         * This ensures the modem is in command mode, then sends command string pszModemString (which should include  *
         * the "AT" but not any end-of-line). It always returns true.                                                 *
         *                                                                                                            *
         * Note that, if the modem initialisation includes &D2 or &D3 and the modem responds correctly to a DTR wink, *
         * it will disconnect before the command is sent. If you don't want this, use &D0 or &D1. However, this may   *
         * cause an extra delay since +++ will be sent with guard timings if the DCD/RLSD line is active.             *
         **************************************************************************************************************/
        m_nCommsErrs = 0;
        m_nLastTransmission = 0;
        ZeroMemory ( m_LastTransmission, sizeof ( m_LastTransmission ));
        CancelTimer( m_hTimer );

        char szTimeNow [ 64 ];
        SYSTEMTIME TimeNow;
        GetLocalTime ( &TimeNow );
        StringCbPrintf ( szTimeNow, sizeof ( szTimeNow ), "%02d:%02d:%02d.%03d", TimeNow.wHour, TimeNow.wMinute, TimeNow.wSecond, TimeNow.wMilliseconds );

#ifdef _DEBUG
        OutputDebugString ( "SendCommandToModem ( \"" );
        OutputDebugString ( pszModemString );
        OutputDebugString ( "\" ) " );
        OutputDebugString ( szTimeNow );
        OutputDebugString ( "\n" );
#endif
        DWORD dwBytesWritten = 0;
        //m_EventTrace.BeginXML ( CEventTrace::Information );
        //m_EventTrace.XML ( CEventTrace::Information, "SendCommandToModem", ( char* ) pszModemString );
        //m_EventTrace.XML ( CEventTrace::Information, "LocalTime", szTimeNow );
        //m_EventTrace.EndXML ( CEventTrace::Information );

        ZeroMemory ( m_szModemCommandString, sizeof ( m_szModemCommandString ));

        DWORD dwModemStatus;

        if ( m_eModemState == Answering || m_eModemState == Dialing )
        {
            /*
             * We were waiting for the modem to connect. We send a single space which normally aborts the operation.
             */
            WriteFile (
                m_hComPort,                                                           // hFile [in] - port to output to
                " ",                                                                  // lpBuffer [in] - data to output
                1,                                                         // nNumberOfBytesToWrite [in] - size of data
                &dwBytesWritten,         // lpNumberOfBytesWritten [out] - bytes actually written. Can this be NULL????
                &m_overLapped );                                 // lpOverlapped [in, out] - allows non-blocking output
        }

        Sleep(500);                                                             // minimum delay before command is sent
        if ( m_eModemState != Idle && m_eModemState != Disconnected )
        {
            /*
             * We check if the modem DCD/RLSD (Carrier Detect) signal is on. If it is, the modem may be currently
             * connected and we momentarily drop DTR to try to disconnect it and return it to command mode.
             */
            GetCommModemStatus ( m_hComPort, &dwModemStatus );
            if ( dwModemStatus & MS_RLSD_ON )
            {
                EscapeCommFunction( m_hComPort, CLRDTR );
                Sleep(500);
                EscapeCommFunction( m_hComPort, SETDTR );
                Sleep(500);
                GetCommModemStatus ( m_hComPort, &dwModemStatus );

                if ( dwModemStatus & MS_RLSD_ON )
                {
                    /*
                     * DCD/RLSD is still on (e.g. the modem was set up with &D0 or &D1). We send the escape sequence
                     * (+++) to the modem with guard timings. This should return it to command mode.
                     *
                     * The guard delay is set by the modem's S12 register and is normally 50 (1 second). It's already
                     * at least 1.5 seconds since we sent anything.
                     */
                    WriteFile (
                        m_hComPort,                                                   // hFile [in] - port to output to
                        "+++",                                                        // lpBuffer [in] - data to output
                        3,                                                 // nNumberOfBytesToWrite [in] - size of data
                        &dwBytesWritten, // lpNumberOfBytesWritten [out] - bytes actually written. Can this be NULL????
                        &m_overLapped );                         // lpOverlapped [in, out] - allows non-blocking output
                    Sleep(1500);                                              // guard delay again with 500 msec safety

                }
            }
        }

        StringCbCat ( m_szModemCommandString, sizeof ( m_szModemCommandString ), pszModemString );
        switch ( m_commandTermination )
        {
            case CommandTermination::CR:
                StringCbCat ( m_szModemCommandString, sizeof ( m_szModemCommandString ), "\r" );
                break;
            case CommandTermination::LF:
                StringCbCat ( m_szModemCommandString, sizeof ( m_szModemCommandString ), "\n" );
                break;
            case CommandTermination::CRLF:
                StringCbCat ( m_szModemCommandString, sizeof ( m_szModemCommandString ), "\r\n" );
                break;
        }
//printf("PROTELSERIAL P800 #%s#\n", m_szModemCommandString);
        WriteFile (
            m_hComPort,                                                               // hFile [in] - port to output to
            m_szModemCommandString,                                                   // lpBuffer [in] - data to output
            lstrlen ( m_szModemCommandString ),                            // nNumberOfBytesToWrite [in] - size of data
            &dwBytesWritten,             // lpNumberOfBytesWritten [out] - bytes actually written. Can this be NULL????
            &m_overLapped );                                     // lpOverlapped [in, out] - allows non-blocking output
        return true;
    }

    void DisplayCommunicationsStatus( void )
    {
        /**************************************************************************************************************
         * Not currently used.                                                                                        *
         **************************************************************************************************************/
        DWORD dwModemStatus;
        GetCommModemStatus ( m_hComPort, &dwModemStatus );
        //m_EventTrace.BeginXML ( CEventTrace::Information );
        //m_EventTrace.XML ( CEventTrace::Information, "CTS", ( dwModemStatus & MS_CTS_ON ) ? "ON" : "OFF" );
        //m_EventTrace.XML ( CEventTrace::Information, "DSR", ( dwModemStatus & MS_DSR_ON ) ? "ON" : "OFF" );
        //m_EventTrace.XML ( CEventTrace::Information, "RING", ( dwModemStatus & MS_RING_ON ) ? "ON" : "OFF" );
        //m_EventTrace.XML ( CEventTrace::Information, "RLSD", ( dwModemStatus & MS_RLSD_ON ) ? "ON" : "OFF" );
        //m_EventTrace.EndXML ( CEventTrace::Information );
    }

    static DWORD WINAPI SerialThreadProc( LPVOID lpParam )
    {
        /**************************************************************************************************************
         * This is the thread that is spawned for each CProtelSerial constructed (i.e. each modem in the system). It  *
         * calls ThreadProc below which is responsible for originating, answering and handling calls via that modem.  *
         * The thread continues running until shutdown occurs.                                                        *
         **************************************************************************************************************/
        CoInitialize(NULL);                                               // initialise the COM library for this thread
        CProtelSerial* protelSerial = (CProtelSerial*)lpParam;
        protelSerial->ThreadProc();
        CoUninitialize();                                        // close the COM library and clean up thread resources
        return 0;
    }

    void ThreadProc(void)                                                                          // called from above
    {
        m_EventTrace.Event( CEventTrace::Details, "%s\tSTART -- void CProtelSerial::ThreadProc(void)", m_szDevice );

        OVERLAPPED ov;
        ZeroMemory ( &ov, sizeof ( ov ));
        ov.hEvent = CreateEvent(
            NULL,                                         // lpEventAttributes [in] - NULL = handle cannot be inherited
            TRUE,                                                 // bManualReset [in] - TRUE = ResetEvent must be used
            FALSE,                                                // bInitialState [in] - FALSE = initially unsignalled
            NULL );                                                           // lpName [in] - NULL = object is unnamed


        //HANDLE hWaitObjects [ 4 ];
        HANDLE hWaitObjects [ 3 ];
        hWaitObjects[ 0 ] = m_hShutDown;
        hWaitObjects[ 1 ] = ov.hEvent;
        hWaitObjects[ 2 ] = m_hTimer;

        CloseDevice(3);

        /*
           EV_BREAK    // A break was detected on input.
           EV_CTS      // The CTS (clear-to-send) signal changed state.
           EV_DSR      // The DSR (data-set-ready) signal changed state.
           EV_ERR      // A line-status error occurred. Line-status errors are CE_FRAME, CE_OVERRUN, and CE_RXPARITY.
           EV_RING     // A ring indicator was detected.
           EV_RLSD     // The DCD/RLSD (receive-line-signal-detect) signal changed state.
           EV_RXCHAR   // A character was received and placed in the input buffer.
           EV_RXFLAG   // The event character was received and placed in the input buffer. The event character is specified in the device's DCB structure, which is applied to a serial port by using the SetCommState function.
           EV_TXEMPTY  // The last character in the output buffer was sent.
         */
        DWORD dwMask = EV_BREAK | EV_CTS | EV_DSR | EV_ERR | EV_RING | EV_RLSD | EV_RXCHAR | EV_RXFLAG | EV_TXEMPTY;
        SetCommMask ( m_hComPort, dwMask );

        COMMTIMEOUTS CommTimeouts;
        CommTimeouts.ReadIntervalTimeout         = MAXDWORD;              //These values ensure that the read operation
        CommTimeouts.ReadTotalTimeoutMultiplier  = 0;               //returns immediately with the characters that have
        CommTimeouts.ReadTotalTimeoutConstant    = 0;                //already been received, even if none are received
        CommTimeouts.WriteTotalTimeoutMultiplier = 0;                                              //Time-outs not used
        CommTimeouts.WriteTotalTimeoutConstant   = 0;                                             //for write operation
        SetCommTimeouts ( m_hComPort, &CommTimeouts );

        DisplayCommunicationsStatus();

        bool bShutdown = false;
        DWORD dwEvent = 0;                                                                             // just starting
		char tembuf[256]; 
		unsigned char n = 0;			// WJS 4/14/2011 
        while ( bShutdown == false )
        {
            DWORD dwEventReceived = 0;
            if (dwEvent < WAIT_OBJECT_0 + 3)
                /*
                 * Either we are just starting or something other than serial timeout occurred last time. We wait for
                 * a comm event (this avoids excessive CPU loading).
                 */
                WaitCommEvent (
                    m_hComPort,                                                         // hFile [in] - port to monitor
                    &dwEventReceived,                              // lpEvtMask [out] - indicates which events occurred
                    &ov );                                               // lpOverlapped [in] - allows non-blocking I/O
            else
                dwEventReceived = 0;                                                  // was serial timeout - no events
            dwEvent = WaitForMultipleObjects(
                sizeof ( hWaitObjects ) / sizeof ( hWaitObjects[ 0 ] ), // nCount [in] - number of objects in *lpHandles
                hWaitObjects,                                                   // lpHandles [in] - objects to wait for
                FALSE,                                   // bWaitAll [in] - FALSE - return when any object is signalled
                __TIMEOUT_VALUE__ );                                                   // dwMilliseconds [in] - timeout

            /*
             * We check the modem's DCD (Carrier Detect) line. (Note that any change causes EV_RLSD so we will get
             * here immediately.) Consider checking DSR also!!!!
             *
             * If DCD is active and we were dialing or answering a call, we mark that the modem is connected, wait
             * briefly for things to stabilise, purge the receive buffer and send the first command (I) to the
             * auditor.
             *
             * If DCD is inactive and we were connected, we mark far-end disconnection and clean up.
             */
            DWORD dwModemStatus;
            GetCommModemStatus ( m_hComPort, &dwModemStatus );
            if ( ( dwModemStatus & MS_RLSD_ON ) == MS_RLSD_ON )                                      // DCD/RLSD active
            {
                if ( m_eModemState == Dialing || m_eModemState == Answering )
                {
                    m_eModemState = Connected;
                    CancelTimer( m_hTimer );
                    Sleep(2000);
                    PurgeComm(
                        this->m_hComPort,                                                 // hFile [in] - the comm port
                        PURGE_RXCLEAR | PURGE_RXABORT ); // dwFlags [in] - end read operations, clear buffer and return immediately
                    Transmit_I_Command();
                }
            }
            else if ( m_eModemState == Connected )                                             // far-end disconnection
	        {
		        m_eModemState = Disconnected;
			    CloseDevice(0);
			}

            time ( &lastActivity );

            switch (dwEvent)
            {
                case WAIT_OBJECT_0:                                                     // system shutdown is signaled.

#ifdef _DEBUG
                    OutputDebugString ( "WAIT_OBJECT_0:// shutdown was signaled.\n" );
#endif
                    // Perform tasks required by this event.
                    bShutdown = true;
					CloseDevice(0);
                    break;

                case WAIT_OBJECT_0 + 1:                                                     // modem event is signaled.
                    {
                        //if ( dwEventReceived == EV_RING) //wjs 4/13/2011                           // ring-in
                        //if ( dwEventReceived == EV_RING && m_eModemState == Idle ) //wjs 4/13/2011                           // ring-in
       //                 if ( (( dwEventReceived & EV_RXCHAR ) == EV_RXCHAR)  && m_eModemState == Idle ) //wjs 4/13/2011                           // ring-in
       //                 {
							//
							////if (m_eModemState == Idle ) {
       //                     //pszFlag[ Offset ] = "EV_RING";
       //                     //Offset++;
       //                     Initialize();
       //                     if ( Database_AddNewCall() == true )
       //                     {
       //                         m_eModemState = Answering;         // Database connection is open, tell modem to answer
       //                         SendCommandToModem ( "ATA" );
       //                         SetTimeoutTimer( m_hTimer, 60);                          // timeout if connection fails
       //                     }
							////}
       //                 }

                        if (( dwEventReceived & EV_RXCHAR ) == EV_RXCHAR )                     // character(s) received
                        {
                            DWORD dwBytesRead = 0;
                            char szReadBuffer [ 1024 ];
                            ZeroMemory ( szReadBuffer, sizeof ( szReadBuffer ));
                            BOOL bOK = ReadFile (
                                m_hComPort,                                               // hFile [in] - the comm port
                                szReadBuffer,                                         // lpBuffer [out] - data received
                                sizeof ( szReadBuffer ),           // nNumberOfBytesToRead [in] - maximum bytes to read
                                &dwBytesRead,                // lpNumberOfBytesRead [out] - actual number of bytes read
                                &ov );                              // lpOverlapped [in, out] - allows non-blocking I/O
                            if ( bOK == FALSE )
                            {
                                DWORD dwError = GetLastError();
                                if ( dwError == ERROR_IO_PENDING )
                                {
                                    GetOverlappedResult(
                                       m_hComPort,                                        // hFile [in] - the comm port
                                       &ov,                         // lpOverlapped [in, out] - allows non-blocking I/O
                                       &dwBytesRead,         // lpNumberOfBytesRead [out] - actual number of bytes read
                                       TRUE );                      // bWait - TRUE = wait until operation is completed
                                }
							}

							if( dwBytesRead > 0 && m_eModemState == Idle )
							{
								DWORD nLoop = 0;
								while (nLoop++ < dwBytesRead)
								{
				
								   tembuf[n] = szReadBuffer [ nLoop ] ;                                    // show on screen
								   if (tembuf[n++] == 0x0a)
								   { 
										tembuf[n] = 0;
										//int tst = strstr(tembuf, "RING");
										if (strstr(tembuf, "RING") != NULL) 
										{
											Initialize();
											if ( Database_AddNewCall() == true )
											{
												m_eModemState = Answering;         // Database connection is open, tell modem to answer
												SendCommandToModem ( "ATA" );
												SetTimeoutTimer( m_hTimer, 60);                          // timeout if connection fails
											}
											else
											{
												Transmit_A_Command(false);   // bug 3010 sez bail out gracefully when this is the case
											}
											memset(tembuf, 0, sizeof(tembuf));
											n = 0;
											break;
										}
										memset(tembuf, 0, sizeof(tembuf));
										n = 0;
								   }
								}
							}


							if ( dwBytesRead > 0 && m_eModemState == Connected )
                            {
                                /*
                                 * We received something via the connected modem and saved it in szReadBuffer. We
                                 * pass it to ProtelHost::MessageReceived() via our MessageReceived().
                                 */
                                MessageReceived(( BYTE* ) szReadBuffer, dwBytesRead );
                            }
						}


//#endif
       //                     if( dwBytesRead > 0 && m_eModemState == Idle )
							//{
							//	tembuff = szReadBuffer.substring(0, bn) bn = atart position for CR
							//		if (instr(tembuf, "RING")) > 0 then continue processing
							//		else
							//			read until Ring found


							//
       //                     Initialize();
       //                     if ( Database_AddNewCall() == true )
       //                     {
       //                         m_eModemState = Answering;         // Database connection is open, tell modem to answer
       //                         SendCommandToModem ( "ATA" );
       //                         SetTimeoutTimer( m_hTimer, 60);                          // timeout if connection fails
       //                     }
							//}
                           
                        //}
                    }
                    break;

                case WAIT_OBJECT_0 + 2:                                                                              //
                    {
                        /*
                         * Timer (time expired while waiting) was signaled.
                         */
                        CancelTimer( m_hTimer );
                        if ( m_eModemState == Connected )
                        {
                            /*
                             * Timeout while we were processing commands. Note that the timer was set by
                             * ProtelHost::Transmit or ProtelHost::Retransmit.
                             */
#ifdef _DEBUG
                            OutputDebugString ( "WAIT_OBJECT_0 + 2:// Timer (time expired while waiting) was signaled.\n" );
#endif
                            if ( ContinueComms(false) == false )
                            {
#ifdef _DEBUG
                                OutputDebugString ( "Too many retries -- stopping call\n" );
#endif
                            m_EventTrace.BeginXML ( CEventTrace::Information );
                            m_EventTrace.XML ( CEventTrace::Information, "Timeout", "Waiting for response from cellular" );
                            //m_EventTrace.XML ( CEventTrace::Information, "Modem", GetDevice() );
                            m_EventTrace.XML ( CEventTrace::Information, "Port", GetPort());
                            m_EventTrace.EndXML ( CEventTrace::Information );
                                Transmit_A_Command(false);                     // abort connection, signalling to retry
                                Sleep(2000);                                                // allow command to be sent
                                CloseDevice(0);
                            }
                            else
                            {
#ifdef _DEBUG
                                OutputDebugString ( "Retransmit();\n" );
#endif
                                Retransmit();                             // resend last command - method of ProtelHost
                            }
                        }

                        else if ( m_eModemState == Dialing || m_eModemState == Answering )         // connection failed
                        {
#ifdef _DEBUG
                            OutputDebugString ( "Timeout while waiting for a response from the Modem -- stopping call\n" );
#endif
                            CloseDevice(0);
                            //m_EventTrace.BeginXML ( CEventTrace::Information );
                            //m_EventTrace.XML ( CEventTrace::Information, "Timeout", "Waiting for response" );
                            //m_EventTrace.XML ( CEventTrace::Information, "Modem", GetDevice() );
                            //m_EventTrace.XML ( CEventTrace::Information, "Port", GetPort());
                            //m_EventTrace.EndXML ( CEventTrace::Information );
                        }
                    }
                    break;

            }
        }

        /*
         * The system itself is shutting down. We tidy everything up.
         */
        CloseDevice(2);
        CancelIo ( m_hComPort );
        CloseHandle ( ov.hEvent );
        m_EventTrace.Event( CEventTrace::Details, "%s\tSTOP --- void CProtelSerial::ThreadProc(void)", m_szDevice );
    }

    virtual void CloseDevice ( int typeclose )
    {
        /**************************************************************************************************************
         * This is called (including from ProtelHost::Process_A_Response) when the modem needs to be returned to idle *
         * state and/or reinitialised - it may or may not be currently connected. The method issues commands to the   *
         * modem as needed and sets other things up.                                                                  *
		 * typeclose values:
		 * 0 => failed call
		 * 1 => sucessfull call
		 * 2 => end call with no database call
         **************************************************************************************************************/
#ifdef _DEBUG
        OutputDebugString ( "virtual void CloseDevice ( int typeclose )\n" );
#endif
        Shutdown();                                                                                // commands to modem
        CProtelHost::CloseDevice( typeclose );
        m_EventTrace.Identifier ( "", GetDevice(),GetPort());
        // CancelTimer( m_hTimer ); <-- Called by CProtelHost::CloseDevice();
        this->CallNumber = 0;
        this->dCallStartTime = ( double ) 0;
    }

    virtual void Initialize ( void )
    {
        /**************************************************************************************************************
         * Called when we dial or answer a call.                                                                      *
         **************************************************************************************************************/
//        CProfileValues profileValues;
        CProtelHost::Initialize();
    }

    void MessageReceived ( BYTE* pBuffer, int bytesRead )
    {
        /**************************************************************************************************************
         * This is called from this class when any data (bytesRead in pBuffer) is received.                           *
         *                                                                                                            *
         * Provided that the modem is currently connected, we assume it is part of a response to a command and pass   *
         * the data to CProtelHost::MessageReceived(). This appends the data to its buffer. If this completes a valid *
         * response, it is processed and usually another command is sent.                                             *
         **************************************************************************************************************/
        if ( m_eModemState == Connected )
        {
            CProtelHost::MessageReceived( pBuffer, bytesRead );
        }
        else
        {
            m_EventTrace.HexDump( CEventTrace::Information, (LPBYTE)pBuffer, bytesRead );
        }
    }

    virtual char* GetPort ( void )
    {
        /**************************************************************************************************************
         * This overrides the GetPort method of ProtelHost. It returns the human-readable modem port name (e.g.       *
         * "\\.\COM3").                                                                                               *
         **************************************************************************************************************/
        return m_szPort;
    }

    virtual char* GetDevice ( void )
    {
        /**************************************************************************************************************
         * This overrides the GetDevice method of ProtelHost. It returns the human-readable modem device name (e.g.   *
         * "Standard 1200 bps Modem").                                                                                *
         **************************************************************************************************************/
        return m_szDevice;
    }

    virtual int GetWaitSeconds ( void )
    {
        /**************************************************************************************************************
         * This overrides the GetWaitSeconds method of ProtelHost. It returns the timeout in seconds to use if a      *
         * valid response to a command isn't received.                                                                *
         **************************************************************************************************************/
        return 10;
    }

    virtual int CommsErrsLimit ( void )
    {
        /**************************************************************************************************************
         * This overrides the CommsErrsLimit method of ProtelHost. It returns the error limit that causes the call to *
         * be aborted if a valid response isn't received (see ProtelHost::ContinueComms).                             *
         **************************************************************************************************************/
        return 9;                                           // e.g. 9 - disconnect after three transmissions of command
    }
};
