/**********************************************************************************************************************
 *                                      This file contains the CFileWatcher class.                                    *
 *                                                                                                                    *
 * This class appears to be unused.                                                                                   *
 *                                                                                                                    *
 *                                         Copyright (c) Protel Inc. 2009-2010                                        *
 **********************************************************************************************************************/

#pragma once

#include "ErrorMessage.h"

class CFileWatcher
 {
private:
    struct InitializeStructure                  // holds data passed to InitializeWatcher and used by WatcherThreadProc
    {
        HANDLE hShutdown;
        HANDLE hFileChanged;
        char* pszFileToWatch;
    };

public:
    static bool InitializeWatcher(HANDLE& rhShutdown, HANDLE& rhFileChanged, char* pszFileToWatch)
    {
        InitializeStructure* pInitializeStructure = new InitializeStructure;
        pInitializeStructure->hShutdown = rhShutdown;
        pInitializeStructure->hFileChanged = rhFileChanged;
        pInitializeStructure->pszFileToWatch = pszFileToWatch;

        HANDLE hWatcherThread = CreateThread(
            NULL,                                               // lpThreadAttributes [in] - NULL = cannot be inherited
            0,                                               // dwStackSize [in] - initial stack size - 0 = use default
            WatcherThreadProc,                                                    // lpStartAddress [in] - in this file
            pInitializeStructure,                                             // lpParameter [in] - parameter set above
            0,                                             // dwCreationFlags [in] - 0 = run immediately after creation
            NULL );                                                           // lpThreadId [out] - NULL = not returned
        if ( hWatcherThread == NULL )
        {
            return false;
        }
        return true;
    }

private:
    static DWORD WINAPI WatcherThreadProc ( LPVOID lpParameter )
    {
        InitializeStructure* pInitializeStructure = ( InitializeStructure* ) lpParameter;

        char szFullFileName [ 1024 ];
        ZeroMemory ( szFullFileName, sizeof ( szFullFileName ));
        CopyMemory ( szFullFileName, pInitializeStructure->pszFileToWatch, sizeof ( szFullFileName ));

        CoInitialize(NULL);                                               // initialise the COM library for this thread

        char* pszFileNameToWatch = PathFindFileName ( szFullFileName );
        PathRemoveFileSpec ( szFullFileName );

        HANDLE hDir = CreateFile (
            szFullFileName,                                                   // lpFileName [in] - name of file to open
            FILE_LIST_DIRECTORY,                                      // dwDesiredAccess [in] - list directory contents
            FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, // dwShareMode [in] - others can read, write and delete
            NULL,                                             // lpSecurityAttributes [in] - NULL = cannot be inherited
            OPEN_EXISTING,                                   // dwCreationDisposition [in] - open already existing file
            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, // dwFlagsAndAttributes [in] - override security, simultaneous read/write OK
            NULL );                                                // hTemplateFile [in] - sets attributes, NULL = none



        OVERLAPPED ov;
        ZeroMemory ( &ov, sizeof ( ov ));
        ov.hEvent = CreateEvent(
            NULL,                                         // lpEventAttributes [in] - NULL = handle cannot be inherited
            TRUE,                                                 // bManualReset [in] - TRUE = ResetEvent must be used
            FALSE,                                                // bInitialState [in] - FALSE = initially unsignalled
            NULL );                                                           // lpName [in] - NULL = object is unnamed

        int nBufferSize = 4096;
        char* pszBuffer = new char [ nBufferSize ];
        ZeroMemory ( pszBuffer, nBufferSize );


        HANDLE hWaitObjects [ 2 ];
        hWaitObjects [ 0 ] = pInitializeStructure->hShutdown;
        hWaitObjects [ 1 ] = ov.hEvent;

        FILE_NOTIFY_INFORMATION* pszFileNotifyInformation;
        char szFileName [ 1024 ];
        char szAction [ 1024 ];

        bool bShutDown = false;
        DWORD dwBytes = 0;
        BOOL bOK = ReadDirectoryChangesW (
            hDir,                                                      // hDirectory [in] - file (directory) to monitor
            pszBuffer,                                                           // lpBuffer [out] - buffer for results
            nBufferSize,                                              // nBufferLength [in] - size of lpBuffer in bytes
            FALSE,                                         // bWatchSubtree [in] - FALSE = don't monitor subdirectories
            FILE_NOTIFY_CHANGE_LAST_WRITE,       // dwNotifyFilter [in] - notify if last-write time of any file changes
            &dwBytes,                             // lpBytesReturned [out] - bytes placed in buffer (should be ignored)
            &ov,                                                  // lpOverlapped [in, out] - for nonblocking operation
            NULL );                                                       // lpCompletionRoutine [in] - NULL = not used
        bool bPostEvent = false;
        DWORD dwMilliseconds = INFINITE;

        while ( bShutDown == false )
        {
            DWORD dwResult = WaitForMultipleObjects( sizeof ( hWaitObjects ) / sizeof ( hWaitObjects [ 0 ] ), hWaitObjects, FALSE, dwMilliseconds );
            dwMilliseconds = INFINITE;
            switch ( dwResult )
            {
                case WAIT_FAILED:
                    OutputDebugString ( "WAIT_FAILED\r\n" );
                    OutputDebugString ( CErrorMessage::ErrorMessageFromSystem ( GetLastError()));
                    return 9999;

                case WAIT_TIMEOUT:
                    if ( bPostEvent == true )
                    {
                        OutputDebugString ( "SetEvent ( pInitializeStructure->hFileChanged );\n" );
                        SetEvent ( pInitializeStructure->hFileChanged );
                    }
                    bPostEvent = false;
                    break;

                case WAIT_OBJECT_0:
                    OutputDebugString ( "WAIT_OBJECT_0\r\n" );
                    bShutDown = true;
                    break;

                case WAIT_OBJECT_0+1:
                    dwMilliseconds = 250;
                    pszFileNotifyInformation = ( FILE_NOTIFY_INFORMATION* )pszBuffer;
                    ZeroMemory ( szFileName, sizeof ( szFileName ));
                    WideCharToMultiByte(
                        CP_ACP,                                                       // CodePage [in] - ANSI code page
                        0,                                                                            // dwFlags [in] -
                        pszFileNotifyInformation->FileName,              // lpWideCharStr [in] - string to be converted
                        pszFileNotifyInformation->FileNameLength,          // cchWideChar [in] - chars in lpWideCharStr
                        szFileName,                                 // lpMultiByteStr [out] - receives converted string
                        sizeof ( szFileName ),                    // cbMultiByte [in] - size of lpMultiByteStr in bytes
                        NULL,                   // lpDefaultChar [in] - NULL = use system default char for unknown ones
                        NULL );                      // lpUsedDefaultChar [out] - NULL = don't indicate if default used
                    ResetEvent ( ov.hEvent );
                    bOK = ReadDirectoryChangesW (
                        hDir,                                          // hDirectory [in] - file (directory) to monitor
                        pszBuffer,                                               // lpBuffer [out] - buffer for results
                        nBufferSize,                                  // nBufferLength [in] - size of lpBuffer in bytes
                        FALSE,                             // bWatchSubtree [in] - FALSE = don't monitor subdirectories
                        FILE_NOTIFY_CHANGE_LAST_WRITE, // dwNotifyFilter [in] - notify if last-write time of any file changes
                        &dwBytes,                 // lpBytesReturned [out] - bytes placed in buffer (should be ignored)
                        &ov,                                      // lpOverlapped [in, out] - for nonblocking operation
                        NULL );                                           // lpCompletionRoutine [in] - NULL = not used
                    if ( StrCmpI ( szFileName, pszFileNameToWatch ) == 0 )
                    {
                        switch ( pszFileNotifyInformation->Action )
                        {
                        case FILE_ACTION_ADDED:                                 // The file was added to the directory.
                            StringCbCopy ( szAction, sizeof ( szAction ), "FILE_ACTION_ADDED" );
                            break;
                        case FILE_ACTION_REMOVED:                           // The file was removed from the directory.
                            StringCbCopy ( szAction, sizeof ( szAction ), "FILE_ACTION_REMOVED" );
                            break;
                        case FILE_ACTION_MODIFIED: // The file was modified. This can be a change in the time stamp or attributes.
                            StringCbCopy ( szAction, sizeof ( szAction ), "FILE_ACTION_MODIFIED" );
                            break;
                        case FILE_ACTION_RENAMED_OLD_NAME:            // The file was renamed and this is the old name.
                            StringCbCopy ( szAction, sizeof ( szAction ), "FILE_ACTION_RENAMED_OLD_NAME" );
                            break;
                        case FILE_ACTION_RENAMED_NEW_NAME:            // The file was renamed and this is the new name.
                            StringCbCopy ( szAction, sizeof ( szAction ), "FILE_ACTION_RENAMED_NEW_NAME" );
                            break;
                    }
                    OutputDebugString ( "WAIT_OBJECT_0+1\t[" );
                    OutputDebugString ( szAction );
                    OutputDebugString ( "]\t[" );
                    OutputDebugString ( szFileName );
                    OutputDebugString ( "]\r\n" );
                    bPostEvent = true;
                }
                break;
            }
        }

        CloseHandle ( hDir );
        CloseHandle ( ov.hEvent );
        CoUninitialize();                                        // close the COM library and clean up thread resources
        return 0;
    }
 };
