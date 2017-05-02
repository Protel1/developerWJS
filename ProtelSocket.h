/**********************************************************************************************************************
 *                                    This file contains the CProtelSocket class.                                     *
 *                                                                                                                    *
 * SocketListener::ListenThreadProc constructs an instance of this class for each incoming socket connection.         *
 *                                                                                                                    *
 * The class constructor spawns a new CprotelSocket::SocketThreadProc thread which is responsible for handling the    *
 * connection and is destroyed once the connection ends.                                                              *
 *                                                                                                                    *
 * ProtelSocket inherits ProtelHost so its constructor also constructs a ProtelHost which it uses to send commands to *
 * the connected device and handle its responses.                                                                     *
 *                                                                                                                    *
 *                                         Copyright (c) Protel Inc. 2009-2011                                        *
 **********************************************************************************************************************/

#pragma once
#include "ProtelHost.h"

class CProtelSocket :
    public CProtelHost
 {
public:
    CProtelSocket(SOCKET hSocket, HANDLE hShutDown, HANDLE hSocketClosed )
        : CProtelHost ( hShutDown ), m_hSocket ( hSocket ), m_hSocketClosed ( hSocketClosed )
    {
        /**************************************************************************************************************
         * CONSTRUCTOR.                                                                                               *
         **************************************************************************************************************/

        /*
         * We get the IP address of the connected device (peer) in human-readable form.
         */
        SOCKADDR_IN m_Sockaddr;
        int nSockaddr = sizeof ( m_Sockaddr );
        getpeername(
            m_hSocket,                                                                           // s [in] - the socket
            ( sockaddr* ) &m_Sockaddr,                                 // name [out] - receives the address of the peer
            &nSockaddr );                                                  // namelen [in, out] - size in bytes of name
        ZeroMemory ( m_szDevice, sizeof ( m_szDevice ));
        DWORD dwDeviceLength = sizeof ( m_szDevice );
        WSAAddressToString (
            ( LPSOCKADDR ) &m_Sockaddr,                          // lpsaAddress [in] - address to translate to a string
            sizeof ( m_Sockaddr ),                               // dwAddressLength [in] - size in bytes of lpsaAddress
            NULL,                                                    // lpProtocolInfo [in] - NULL = use first protocol
            m_szDevice,            // lpszAddressString [in, out] - buffer to receive the human-readable address string
            &dwDeviceLength );                              // lpdwAddressStringLength [in, out] - length of the string

        //char szTimerName [ 1024 ];
        //ZeroMemory ( szTimerName, sizeof ( szTimerName ));
        //StringCbCopy ( m_szDevice, sizeof ( m_szDevice ), szDevice );
        //StringCbCat ( m_szDevice, sizeof ( m_szDevice ), ":" );
        //StringCbCat ( m_szDevice, sizeof ( m_szDevice ), szPort );

        m_hTimer = CreateWaitableTimer (
            NULL,                                      // lpTimerAttributes [in] - NULL = defaults, cannot be inherited
            FALSE,                                                 // bManualReset [in] - FALSE = synchronisation timer
            m_szDevice );                                               // lpTimerName [in] - address string from above
        //m_EventTrace.Event( CEventTrace::Details, "CProtelSocket::CProtelSocket -- m_hTimer = CreateWaitableTimer ( NULL, TRUE, %s );", m_szDevice );

        DWORD dwLastError = GetLastError();
        if (( m_hTimer != NULL && dwLastError == ERROR_ALREADY_EXISTS ) || m_hTimer == NULL )
        {
            m_EventTrace.Event( CEventTrace::Error, "CreateWaitableTimer failed -- <%s>", CErrorMessage::ErrorMessageFromSystem ( dwLastError ));
            //OutputDebugString ( "Stopping . . .\n" );
        }

        //m_EventTrace.Event( CEventTrace::Details, "CProtelSocket::CProtelSocket(SOCKET hSocket, HANDLE hShutDown)" );

        /*
         * We associate event types FD_ACCEPT and FD_CLOSE with the listening socket and a new event. This event will
         * be used to trigger actions in the spawned thread.
         */
        m_protelEvent = WSACreateEvent();
        WSAEventSelect(
            m_hSocket,                                                                     // hSocket [in] - the socket
            m_protelEvent,                                          // hEventObject [in] - event object to be signalled
            FD_READ | FD_CLOSE );                               // lNetworkEvents [in] - data received or socket closed

        DWORD dwThreadId;

        /*
         * We spawn a thread to handle the connection. This returns a handle which we don't use: we close it
         * immediately to avoid a handle leak. Note that this doesn't affect the thread - it continues to run to
         * completion.
         */
        CloseHandle(CreateThread(
                NULL,
                0,
                SocketThreadProc,
                this,
                0,
                &dwThreadId ));
    }

    virtual ~CProtelSocket(void)
    {
        /**************************************************************************************************************
         * DESTRUCTOR.                                                                                                *
         **************************************************************************************************************/
        //m_EventTrace.Event( CEventTrace::Details, "CProtelSocket::~CProtelSocket(void)" );
        WSACloseEvent ( m_protelEvent );
        closesocket ( m_hSocket );
        m_hSocket = NULL;
        m_protelEvent = NULL;
    }

    virtual void Send(LPBYTE pszBuffer, int BufferLength)
    {
        /**************************************************************************************************************
         * This overrides the Send method of ProtelHost. It sends BufferLength bytes from pszBuffer via the socket.   *
         **************************************************************************************************************/
        send (
            m_hSocket,                                                              // hSocket [in] - socket to send on
            ( LPCTSTR )pszBuffer,                                                        // pBuffer [in] - data to send
            BufferLength,                                                     // nLength [in] - number of bytes to send
            0 );                                                                        // nFlags [in] - none specified
    }

    virtual void Shutdown ( void )
    {
        /**************************************************************************************************************
         * This overrides the Shutdown method of ProtelHost. It closes the socket connection and signals              *
         * m_hSocketClosed.                                                                                           *
         **************************************************************************************************************/
        shutdown (
            m_hSocket,                                                                     // hSocket [in] - the socket
            SD_RECEIVE | SD_SEND );                                       // nHow [in] - shutdown both receive and send
        SetEvent ( m_hSocketClosed );                                       // signals SocketListener::ListenThreadProc
    }

protected:
    SOCKET m_hSocket;                                                     // the socket associated with this connection
    HANDLE m_hSocketClosed;                    // used to signal SocketListener::ListenThreadProc when socket is closed
    WSAEVENT m_protelEvent;                                             // signalled when connection accepted or closed
    char m_szDevice [ 1024 ];           // human readable address of connected device (CProtelHost gets it via GetPort)

    static DWORD WINAPI SocketThreadProc( LPVOID lpParam )
    {
        /**************************************************************************************************************
         * This is the thread that is spawned for each CProtelSocket constructed (i.e. each incoming socket           *
         * connection to the system). It calls ThreadProc below which is responsible for handling the connection. The *
         * thread continues running until the connection ends.                                                        *
         **************************************************************************************************************/
        CoInitialize(NULL);                                               // initialise the COM library for this thread
        CProtelSocket* protelSocket = (CProtelSocket*)lpParam;
        protelSocket->ThreadProc();
        CoUninitialize();                                        // close the COM library and clean up thread resources
        return 0;
    }

    void ThreadProc(void)                                                                          // called from above
    {

        //m_EventTrace.Event( CEventTrace::Details, "%s\tSTART -- void CProtelSocket::ThreadProc(void)", m_szDevice );

        /*
         * We create a new row in the COMM_SERVER_CALL database table (getting the callnumber) then, providing this
         * succeeds, we start ProtelHost off by having it send an I command to the connected device. ProtelHost
         * handles the response when we call its MessageReceived and sends additional commands as required.
         *
         * If the database operation fails (perhaps there wasn't a database connection), we abort the connection.
         */
        bool bContinue = Database_AddNewCall();
        if ( bContinue == true )
        {
			Sleep(100);
            Transmit_I_Command();
        }
        else
        {
            Transmit_A_Command(false);                                         // abort connection, signalling to retry
            Sleep(2000);                                                                    // allow command to be sent
            Shutdown();
        }

        int bytesRead;
        char szBuffer [ 4096 ];
        ZeroMemory ( szBuffer, sizeof ( szBuffer ));

        WSANETWORKEVENTS wsaNetworkEvents;
        DWORD dwEventMask = 0;

        HANDLE hWaitObjects [ 3 ];
        hWaitObjects[ 0 ] = m_hShutDown;
        hWaitObjects[ 1 ] = m_protelEvent;
        hWaitObjects[ 2 ] = m_hTimer;

        //-ResetEvent ( m_hTimer );

        while ( bContinue == true )
        {
            DWORD dwEvent = WaitForMultipleObjects(
                sizeof ( hWaitObjects ) / sizeof ( hWaitObjects[0] ),  // nCount [in] - number of objects in *lpHandles
                hWaitObjects,                                                   // lpHandles [in] - objects to wait for
                FALSE,                                   // bWaitAll [in] - FALSE - return when any object is signalled
                INFINITE );                                       // dwMilliseconds [in] - INFINITE = wait indefinitely
            //OutputDebugString ( "DWORD dwEvent = WaitForMultipleObjects( sizeof ( hWaitObjects ) / sizeof ( hWaitObjects[0] ), hWaitObjects, FALSE, INFINITE );\n" );
            switch( dwEvent )
            {
                case WAIT_OBJECT_0:                                                     // system shutdown is signaled.

                    //OutputDebugString ( m_szDevice );
                    //OutputDebugString ( "\tWAIT_OBJECT_0\n" );
                    bContinue = false;
                    break;

                case WAIT_OBJECT_0 + 1:                                                // connection accepted or closed
                    ZeroMemory ( &wsaNetworkEvents, sizeof ( wsaNetworkEvents ));
                    WSAEnumNetworkEvents (
                        m_hSocket,                                                         // hSocket [in] - the socket
                        m_protelEvent,                               // hEventObject [in] - an optional object to reset
                        &wsaNetworkEvents );                           // lpNetworkEvents [out] - the events and errors
                    if ( wsaNetworkEvents.lNetworkEvents == FD_READ )
                    {
                        /*
                         * We received something via the socket. We save it in szBuffer, then pass it to ProtelHost
                         * using its MessageReceived().
                         */
                        //OutputDebugString ( m_szDevice );
                        //OutputDebugString ( "\tWAIT_OBJECT_1\tFD_READ\n" );
                        ZeroMemory ( szBuffer, sizeof ( szBuffer ));
                        bytesRead = recv (
                            m_hSocket,                                           // hSocket [in] - socket to receive on
                            szBuffer,                                                // pBuffer [out] - buffer for data
                            sizeof ( szBuffer ),                             // nLength [in] - size of pBuffer in bytes
                            0 );                                                        // nFlags [in] - none specified
                        MessageReceived(( BYTE* ) szBuffer, bytesRead );
                    }
                    else if ( wsaNetworkEvents.lNetworkEvents == FD_CLOSE )
                    {
                        /*
                         * The connection has closed (including normal completion of the ProtelHost command
                         * sequence). We will exit.
                         */
                        //OutputDebugString ( m_szDevice );
                        //OutputDebugString ( "\tWAIT_OBJECT_1\tFD_CLOSE\n" );
                        bContinue = false;
                    }
                    break;

                case WAIT_OBJECT_0 + 2:                             // Timer (time expired while waiting) was signaled.
                    {
                        //OutputDebugString ( m_szDevice );
                        //OutputDebugString ( "\tWAIT_OBJECT_2\n" );
                        //CancelWaitableTimer( m_hTimer );
                        CancelTimer( m_hTimer );
                        if ( ContinueComms( false ) == false )
                        {
                            Transmit_A_Command(false);                         // abort connection, signalling to retry
                            Sleep(2000);                                                    // allow command to be sent
                            Shutdown();
                            bContinue = false;
                        }
                        else
                        {
                            Retransmit();                                 // resend last command - method of ProtelHost
                        }
                    }
                    break;
            }
        }
        m_EventTrace.Event( CEventTrace::Details, "%s\tSTOP -- void CProtelSocket::ThreadProc(void)", m_szDevice );
            CAdoStoredProcedure szOracleProcedureName (  "PKG_COMM_SERVER.addSysLogRecAutonomous" );
            _bstr_t bstrOutText( "STOP -- void CProtelSocket::ThreadProc(void)" );
            _variant_t vtszOutput ( bstrOutText );
           szOracleProcedureName.AddParameter( "pi_TEXT", vtszOutput, ADODB::DataTypeEnum::adBSTR, ADODB::ParameterDirectionEnum::adParamInput, bstrOutText.length());
		   m_padoConnection->ExecuteNonQuery( szOracleProcedureName, false, true );
        //OutputDebugString ( m_szDevice );
        //OutputDebugString ( "\tSTOP --- void CProtelSocket::ThreadProc(void)\n" );
        CloseDevice(3);
        Closed = true;                          // Allow CSocketListener::ListenThreadProc to remove us from ProtelList
    }

    virtual void CloseDevice ( int typeclose )
    {
        /**************************************************************************************************************
         * This is called (including from ProtelHost::Process_A_Response) when the socket connection needs to end.    *
         * The method does this and signals m_hSocketClosed.                                                          *
		 * typeclose values:
		 * 0 => failed call
		 * 1 => sucessfull call
		 * 2 => end call with no database call
         **************************************************************************************************************/
        Shutdown();
        CProtelHost::CloseDevice(typeclose);
    }

    virtual void Initialize ( void )
    {
        /**************************************************************************************************************
         * Is this ever used????                                                                                      *
         **************************************************************************************************************/
        CProfileValues profileValues;                                                      // does this do anything????
        CProtelHost::Initialize();
    }

    virtual char* GetPort ( void )
    {
        /**************************************************************************************************************
         * This overrides the GetPort method of ProtelHost. It returns the human-readable address of the connected    *
         * device (e.g. "192.168.3.2:38308").                                                                         *
         **************************************************************************************************************/
        return m_szDevice;
    }

    virtual char* GetDevice ( void )
    {
        /**************************************************************************************************************
         * This overrides the GetDevice method of ProtelHost. It returns the socket device name.                      *
         **************************************************************************************************************/
        return "TCP/IP";
    }

    virtual int GetWaitSeconds ( void )
    {
        /**************************************************************************************************************
         * This overrides the GetWaitSeconds method of ProtelHost. It returns the timeout in seconds to use if a      *
         * valid response to a command isn't received.                                                                *
         **************************************************************************************************************/
        return 90;
    }

    virtual int CommsErrsLimit ( void )
    {
        /**************************************************************************************************************
         * This overrides the CommsErrsLimit method of ProtelHost. It returns the error limit that causes the call to *
         * be aborted if a valid response isn't received (see ProtelHost::ContinueComms).                             *
         **************************************************************************************************************/
        return 0;                                                                 // e.g. 0 - disconnect on any timeout
    }
 };
