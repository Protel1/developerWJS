/**********************************************************************************************************************
 *                                    This file contains the CSocketListener class.                                   *
 *                                                                                                                    *
 * CApplication calls CSocketListener::InitializeListener. It spawns a CSocketListener::ListenThreadProc thread which *
 * is responsible for constructing a CProtelSocket instance for each incoming socket connection and destroying these  *
 * when the connection ends.                                                                                          *
 *                                                                                                                    *
 *                                         Copyright (c) Protel Inc. 2009-2010                                        *
 **********************************************************************************************************************/

#pragma once
#include "StdAfx.h"
#include "ProtelList.h"
#include "ProtelSocket.h"
#include "EventTrace.h"

#include <stdio.h>

//struct InitSockets                                                                     // Doesn't appear to be used!!!!
// {
//    WSADATA wsaData;
//    InitSockets()
//    {
 //----------------------
 // Initialize Winsock
//        int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
//        if (iResult != NO_ERROR)
//        {
//            CEventTrace eventTrace; // records events - see EventTrace.h
//            eventTrace.Event( CEventTrace::SevereError, "Error at WSAStartup()" );
//        }
//    }
//    ~InitSockets()
//    {
//        WSACleanup();
//    }
// } _init_InitSockets_;


class CSocketListener
 {
private:
    struct InitializeStructure                  // holds data passed to InitializeListener and used by ListenThreadProc
    {
        HANDLE* phShutdown;
        CProtelList* protelList;
    };

public:
    static bool InitializeListener(CProtelList* protelList, HANDLE* phShutdown)
    {
        /**************************************************************************************************************
         * This is called by CApplication on startup. protelList points to the ProtelList (ProtelHosts associated     *
         * with modems and socket connections) created by CApplication. CApplication signals the event pointed to by  *
         * phShutdown when the system is shutting down.                                                               *
         *                                                                                                            *
         * It spawns a ListenThreadProc and returns TRUE on success.                                                  *
         **************************************************************************************************************/
        InitializeStructure* pInitializeStructure = new InitializeStructure;         // save the data to pass to thread
        pInitializeStructure->protelList = protelList;
        pInitializeStructure->phShutdown = phShutdown;

        HANDLE hListenThread = CreateThread(
            NULL,                                               // lpThreadAttributes [in] - NULL = cannot be inherited
            0,                                               // dwStackSize [in] - initial stack size - 0 = use default
            ListenThreadProc,                                                     // lpStartAddress [in] - in this file
            pInitializeStructure,                                                    // lpParameter [in] - set up above
            0,                                             // dwCreationFlags [in] - 0 = run immediately after creation
            NULL );                                                           // lpThreadId [out] - NULL = not returned
        if ( hListenThread == NULL )
        {
            return false;
        }
        CloseHandle( hListenThread );                             // we won't use this handle - thread continues anyway
        return true;
    }

private:
    static DWORD WINAPI ListenThreadProc ( LPVOID lpParameter )
    {
        /**************************************************************************************************************
         * This is the thread that is spawned by InitializeListener at startup. It binds to the port specified in the *
         * profile and continues running until shutdown occurs, waiting for incoming socket connections. For each     *
         * that is opened, it constructs a CProtelSocket. This in turn spawns a CprotelSocket::SocketThreadProc which *
         * is responsible for handling the connection. CProtelSockets marked as closed are destroyed by this thread   *
         * when a connection ends.                                                                                    *
         **************************************************************************************************************/
        InitializeStructure* pInitializeStructure = ( InitializeStructure* ) lpParameter;

        CoInitialize(NULL);                                               // initialise the COM library for this thread
        CEventTrace eventTrace;                                                    // records events - see EventTrace.h

//        char szMessageBuffer [ 1024 ];                                                              // Is this used????
//        ZeroMemory ( szMessageBuffer, sizeof ( szMessageBuffer ));

        /*
         * We initialise a listening socket and bind it to the specified port.
         */
        SOCKET listenSocket = socket(
            AF_INET,                                                            // [in] address family - AF_INET = IPv4
            SOCK_STREAM,                                         // [in] type - SOCK_STREAM = reliable connection-based
            IPPROTO_TCP);                                                          // [in] protocol - IPPROTO_TCP = TCP
        sockaddr_in listenAddress;
        ZeroMemory ( &listenAddress, sizeof ( listenAddress ));
        listenAddress.sin_family = AF_INET;
        listenAddress.sin_addr.s_addr = htonl(INADDR_ANY);
        {
            CProfileValues profileValues;
            listenAddress.sin_port = htons( profileValues.GetListenPortNumber());
        }
        bind (
            listenSocket,                                                                          // [in] - the socket
            (SOCKADDR *) &listenAddress,                               // name [in] - local address to assign to socket
            sizeof ( listenAddress ));                                          // namelen [in] - size of name in bytes

        /*
         * We associate event type FD_ACCEPT with the listening socket and a new Winsock event.
         */
        WSAEVENT listenEvent = WSACreateEvent();
        WSAEventSelect(
            listenSocket,                                                                  // hSocket [in] - the socket
            listenEvent,                                            // hEventObject [in] - event object to be signalled
            FD_ACCEPT );                               // lNetworkEvents [in] - FD_ACCEPT = connection request accepted

        /*
         * We set up to listen for incoming connection requests on the socket. and set up the events we will handle.
         */
        if (listen(
                listenSocket,                                                                      // [in] - the socket
                SOMAXCONN )                               // [in] queue length - SOMAXCONN = reasonable value set by OS
            == SOCKET_ERROR)
        {
            int nError = WSAGetLastError();
            eventTrace.Event( CEventTrace::SevereError, "Error (%ld) listening on socket.", nError );
        }
        eventTrace.Event( CEventTrace::Details, "Listening on socket...");
        HANDLE hEvents [ 3 ];
        hEvents [ 0 ] = *( pInitializeStructure->phShutdown );                               // system is shutting down
        hEvents [ 1 ] = listenEvent;                                              // new connection request (FD_ACCEPT)
        hEvents [ 2 ] = CreateEvent( NULL, TRUE, FALSE, NULL );  // a connection ended (set by CProtelSocket::Shutdown)

//  _CrtMemState memstate;
//  _CrtMemCheckpoint(&memstate);


        bool bContinue = true;
        while ( bContinue == true )
        {
            /*
             * We wait for an event (system shutdown, connection request or connection ended), then handle it. We do
             * this until system shutdown.
             */
            DWORD dwEvent = WaitForMultipleObjects(
                sizeof ( hEvents ) / sizeof ( hEvents[0] ),            // nCount [in] - number of objects in *lpHandles
                hEvents,                                                        // lpHandles [in] - objects to wait for
                FALSE,                                   // bWaitAll [in] - FALSE - return when any object is signalled
                INFINITE );                                       // dwMilliseconds [in] - INFINITE = wait indefinitely
            switch( dwEvent )
            {
                case 0:                                                                      // system is shutting down
                    bContinue = false;
                    break;

                case 1:                                                                      // new connection accepted
                    {
                        WSANETWORKEVENTS wsaNetworkEvents;
                        ZeroMemory ( &wsaNetworkEvents, sizeof ( wsaNetworkEvents ));
                        WSAEnumNetworkEvents(
                            listenSocket,                                                  // hSocket [in] - the socket
                            listenEvent,                             // hEventObject [in] - an optional object to reset
                            &wsaNetworkEvents );                       // lpNetworkEvents [out] - the events and errors
                        if ( wsaNetworkEvents.lNetworkEvents == FD_ACCEPT )
                        {
                            /*
                             * The connection was accepted OK. We set up the socket, construct an associated
                             * CProtelSocket and add it to the ProtelList. (The CProtelSocket constructor spawns a
                             * new thread to handle the connection - this will signal hEvents[2] when it is
                             * finished).
                             */
                            sockaddr_in acceptedAddress; // does this do anything???? (optional in accept, otherwise unused)
                            int acceptedAddressLength = sizeof ( acceptedAddress );
                            ZeroMemory ( &acceptedAddress, acceptedAddressLength );
                            SOCKET dataSocket = accept(
                                listenSocket,                                                      // [in] - the socket
                                ( sockaddr* )&acceptedAddress,                 // addr [out] - address of remote device
                                &acceptedAddressLength );                  // addrlen [in, out] - length of addr struct
                            CProtelSocket* p = new CProtelSocket( dataSocket, *( pInitializeStructure->phShutdown ),
                                hEvents [ 2 ] );
#ifdef __DEBUG_MEMORY_CHECK_UTILITIES__
                            _ASSERT ( _CrtIsValidPointer ( p, sizeof ( CProtelSocket ), FALSE ));
#endif  //  __DEBUG_MEMORY_CHECK_UTILITIES__
                            pInitializeStructure->protelList->Add( p );
                            eventTrace.Event( CEventTrace::Information, "connected (%ld)", GetTickCount());
                        }
                    }
                    break;

                case 2:                                                           // a CProtelSocket::Shutdown occurred
                    while ( true )
                    {
                        CProtelHost* protelHost = pInitializeStructure->protelList->MoveFirst();
                        CProtelHost* protelClosedHost = NULL;
                        while ( protelHost != NULL )
                        {
                            /*
                             * We work through the list of ProtelHosts. When we find one that is marked as closed
                             * (only socket connections do this), we record it and stop.
                             */
                            if ( protelHost->Closed == true )
                            {
                                protelClosedHost = protelHost;
                                break;
                            }
                            protelHost = pInitializeStructure->protelList->Next();
                        }
                        /*
                         * Either we found a closed ProtelHost or reached the end of the list (protelClosedHost
                         * NULL). We remove the ProtelHost and destroy it (does nothing if protelClosedHost NULL).
                         * Unless we reached the end of the list without finding a closed host, we will return to the
                         * start of the list.
                         */
                        pInitializeStructure->protelList->Remove ( protelClosedHost );
                        if ( protelClosedHost == NULL )
                        {
                            break;
                        }
                    }
//    _CrtDumpMemoryLeaks();
//  _CrtMemState memstate;
//  _CrtMemCheckpoint(&memstate);
//  _CrtMemDumpStatistics(&memstate);

//              char *s = new char[15];
//              strcpy_s(s, 15, "Hello world!");
//      bstr_t bstrName ( "Hello world" );
//  _CrtMemDumpAllObjectsSince(&memstate);
//  delete[] s;

                    ResetEvent ( hEvents [ 2 ] );
                    break;

            }
        }

        /*
         * The system is shutting down.
         */
        delete pInitializeStructure;
        eventTrace.Event( CEventTrace::Details, "Exiting ListenProcedure");
        CoUninitialize();                                        // close the COM library and clean up thread resources
        return 0;
    }
 };

