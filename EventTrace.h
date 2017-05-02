/**********************************************************************************************************************
 *                                      This file contains the CEventTrace class.                                     *
 *                                                                                                                    *
 * Multiple threads can construct this class, set an identity with its Identifier method, then use the Event or       *
 * HexDump methods to record a complete event. Alternatively, the BeginXML, XML and EndXML methods can be used to     *
 * create a complete event. An event is recorded by generating XML, broadcasting it to the specified [debug] port and *
 * appending it to the [database] file specified in the profile (.INI file).                                          *
 *                                                                                                                    *
 * Events are only recorded if the level specified equals or exceeds [debug] level specified in the profile (.INI     *
 * file).                                                                                                             *
 *                                                                                                                    *
 *                                         Copyright (c) Protel Inc. 2009-2010                                        *
 **********************************************************************************************************************/
#pragma once
#include <windows.h>
//#include <form1.h>

#define ANSI
#include <stdarg.h>
#include "ProfileValues.h"
#include "HexDump.h"
#include <atlbase.h>
#include <atlconv.h>
#include <vcclr.h>


//#include <msclr/marshal.h>
static SOCKET socketEventTrace = INVALID_SOCKET;                                     // used to broadcast events to LAN
static HANDLE ghMutex = NULL;                                    // ensures only one thread writes to logfile at a time

class CEventTrace 
 {
public:
    enum TraceLevel                                                                      // used to specify event level
    {
        InvalidLevel,                                                                                   // 0 in profile
        Details,                                                                                        // 1 in profile
        Information,                                                                                    // 2 in profile
        Warning,                                                                                        // 3 in profile
        Error,                                                                                          // 4 in profile
        SevereError,                                                                                     // 5 in profile
//		ResetLogFile
    };

protected:
    int _port;                                                          // set from [debug] port in profile (.INI file)
//    char szUniqueID [ 256 ];             // set by constructor from hostname and time - doesn't appear to be used!!!!
    HANDLE m_hFileHandle;                                 // opened on file from [database] file in profile (.INI file)
    TraceLevel m_TraceLevel;                                           // set from [debug] level in profile (.INI file)
    char* m_pszIdentity;                                     // set using this::Identifier(), included in generated XML
    char* m_pszPort;                                         // set using this::Identifier(), included in generated XML
    char* m_pszDevice;                                       // set using this::Identifier(), included in generated XML


    // for future reference!
    // use the szUniqueID for a) filename, b) xml identifier
    // convert output to xml
    // write xml into uniquely named file
    // transmit xml via UDP socket
    // suggested xml
    // <Trace ID="szUniqueID" Date="date" Time="time" Payload="__results_from_function__" />
    // encode payload using EscapeXML Windows API function
    // modify file log name to file log path
    // append szUniqueID to file log path using PathAppend( pszPath, szMore ) function
    // finish local file name with PathAddExtension( pszPath, pszExtension ) function

public:
    CEventTrace(void)
    {
        /**************************************************************************************************************
         * CONSTRUCTOR. This creates the mutex, sets the trace level, creates or opens the log file and opens the     *
         * debug socket.                                                                                              *
         **************************************************************************************************************/
        if ( ghMutex == NULL )
        {
            ghMutex = CreateMutex(
                NULL,                                                                    // default security attributes
                FALSE,                                                                           // initially not owned
                NULL );                                                                                // unnamed mutex
        }

#if 0
        DOUBLE dUniqueTime;
        SYSTEMTIME UniqueTime;
        GetSystemTime ( &UniqueTime );
        SystemTimeToVariantTime ( &UniqueTime, &dUniqueTime );

        ZeroMemory ( szUniqueID, sizeof ( szUniqueID ));
        gethostname ( szUniqueID, sizeof ( szUniqueID ));

        size_t nszOutputLength = 0;
        StringCbLength ( szUniqueID, sizeof ( szUniqueID ), &nszOutputLength );
        StringCchPrintf ( szUniqueID + nszOutputLength, sizeof ( szUniqueID ) - nszOutputLength, ":%f", dUniqueTime );
#endif

        m_pszIdentity = NULL;
        m_pszPort = NULL;
        m_pszDevice = NULL;

        CProfileValues profileValues;
        m_TraceLevel = ( TraceLevel ) profileValues.GetDebugLevel();

        m_hFileHandle = CreateFile(
            profileValues.GetLogFile(),                                       // lpFileName [in] - name of file to open
            GENERIC_WRITE,                                                         // dwDesiredAccess [in] - write only
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,           // dwShareMode [in] - other processes can also read and write
            NULL,                                             // lpSecurityAttributes [in] - NULL = cannot be inherited
            OPEN_ALWAYS,                                   // dwCreationDisposition [in] - create unless already exists
            FILE_ATTRIBUTE_NORMAL,                             // dwFlagsAndAttributes [in] - no special attributes set
            NULL );                                                // hTemplateFile [in] - sets attributes, NULL = none
        if ( socketEventTrace == INVALID_SOCKET )                                                  // socket isn't open
        {
            DWORD dwCmd = 0L;                                                              // socket (FIONBIO) blocking
            BOOL bBroadcast = TRUE;              // socket (SOL_SOCKET) enable transmitting/receiving broadcast packets
            socketEventTrace = socket (
                AF_INET,                                                                              // af [in] - IPv4
                SOCK_DGRAM,                                                // type [in] - connectionless (supports UDP)
                IPPROTO_UDP );                                                                   // protocol [in] - UDP
            ioctlsocket (
                socketEventTrace,                                                                // s [in] - the socket
                FIONBIO,                                                        // cmd [in] - set blocking/non-blocking
                &dwCmd );                                                         // *argp [inout] - set blocking above
            setsockopt (
                socketEventTrace,                                                                // s [in] - the socket
                SOL_SOCKET,                                                     // level [in] - protocol level affected
                SO_BROADCAST,                 // optname [in] - disable/enable transmitting/receiving broadcast packets
                (LPCSTR)&bBroadcast,                                               // *optval [in] - enable (set above)
                sizeof( bBroadcast ));                                                                   // optlen [in]
        }
        _port = profileValues.GetDebugPort();                 // get the broadcast port ([debug] port) from the profile
    }

    virtual ~CEventTrace(void)
    {
        /**************************************************************************************************************
         * DESTRUCTOR                                                                                                 *
         **************************************************************************************************************/
        if ( m_hFileHandle != INVALID_HANDLE_VALUE )
        {
            CloseHandle ( m_hFileHandle );
            m_hFileHandle = INVALID_HANDLE_VALUE;
        }
        if ( m_pszIdentity != NULL )
        {
            delete [] m_pszIdentity;
            m_pszIdentity = NULL;
        }
        if ( m_pszPort != NULL )
        {
            delete [] m_pszPort;
            m_pszPort = NULL;
        }
        if ( m_pszDevice != NULL )
        {
            delete [] m_pszDevice;
            m_pszDevice = NULL;
        }
    }

    void Identifier ( char* pszIdentity, char* pszDevice, char* pszAddress )
    {
        /**************************************************************************************************************
         * This sets identifiers that will be included in generated XML.                                              *
         **************************************************************************************************************/
        if ( m_pszIdentity != NULL )
        {
            delete [] m_pszIdentity;
            m_pszIdentity = NULL;
        }
        if ( m_pszPort != NULL )
        {
            delete [] m_pszPort;
            m_pszPort = NULL;
        }
        if ( m_pszDevice != NULL )
        {
            delete [] m_pszDevice;
            m_pszDevice = NULL;
        }
        int Length = lstrlen ( pszIdentity ) + 1;
        m_pszIdentity = new char [ Length ];
        ZeroMemory ( m_pszIdentity, Length );
        StringCbCopy ( m_pszIdentity, Length, pszIdentity );

        Length = lstrlen ( pszDevice ) + 1;
        m_pszDevice = new char [ Length ];
        ZeroMemory ( m_pszDevice, Length );
        StringCbCopy ( m_pszDevice, Length, pszDevice );

        Length = lstrlen ( pszAddress ) + 1;
        m_pszPort = new char [ Length ];
        ZeroMemory ( m_pszPort, Length );
        StringCbCopy ( m_pszPort, Length, pszAddress );
    }

    void Broadcast ( char* pszMessage )
    {
        /**************************************************************************************************************
         * This sends pszMessage to the port specified in the profile at the broadcast address of the LAN.            *
         **************************************************************************************************************/
//        SOCKADDR_IN     sinIn;                                                       // doesn't seem to do anything!!!!
        SOCKADDR_IN     sinOut;

//        sinIn.sin_family =   AF_INET;
//        sinIn.sin_addr.s_addr = INADDR_ANY;
//        sinIn.sin_port = htons ( _port );

        sinOut.sin_family = AF_INET;
        sinOut.sin_port = htons ( _port );
        sinOut.sin_addr.s_addr = INADDR_BROADCAST;               // 255.255.255.255 - translates to LAN broadcast addrs

        sendto (
            socketEventTrace,                                                             // s [in] - socket descriptor
            pszMessage,                                                                 // *buf [in] - data to transmit
            lstrlen ( pszMessage ),                                                        // len [in] - length of data
            0,                                                                                    // flags [in] - flags
            ( struct sockaddr* )&sinOut,                                // *to [in] - address of target (remote) socket
            sizeof ( sinOut ));                                                         // tolen [in] - size of address
    }

    void Event ( TraceLevel Level, char* pszFormat, ...)
     {
        /**************************************************************************************************************
         * A thread uses this method of its instance of this class to record an event for debugging purposes. If      *
         * Level equals or exceeds [debug] level set in the profile (.INI file), arguments 3 onwards are formatted    *
         * according to pszFormat, encapsulated as a complete event in XML, broadcast to the port set in [debug] port *
         * in the profile and appended to the file set in [database] file in the profile.                             *
         **************************************************************************************************************/
        if ( Level >= m_TraceLevel )
        {
            /*
             * The level is sufficient to record the event. We set up strings and start the XML (including
             * identifiers and time).
             */
            char szOutput [ 4096 ];                                        // string to broadcast and append to logfile
            char szOutput2 [ 4096 ];                     // szOutputPrint with problem characters escaped (& sequences)
            char szOutputPrint [ 4096 ];                                // ... args as a string formatted per pszFormat
            ZeroMemory ( szOutput, sizeof ( szOutput ));
            ZeroMemory ( szOutput2, sizeof ( szOutput2 ));
            ZeroMemory ( szOutputPrint, sizeof ( szOutputPrint ));
            BuildXmlStart ( szOutput, sizeof ( szOutput ));
            StringCbCat( szOutput, sizeof ( szOutput ), " trace=\"" );

            /*
             * We format the ... args per pszFormat.
             */
            va_list args;                                          // to access variable arguments to this method (...)
            va_start ( args, pszFormat );
            StringCchVPrintf ( szOutputPrint, sizeof ( szOutputPrint ), pszFormat, args );
            va_end ( args );

            /*
             * We escape problem chars, append the formatted args and finish the XML, then broadcast it and append it
             * to the logfile.
             */
            XmlEncode ( szOutputPrint, sizeof ( szOutputPrint ), szOutput2, sizeof ( szOutput2 ));
            StringCbCat( szOutput, sizeof ( szOutput ), szOutput2 );
            StringCbCat( szOutput, sizeof ( szOutput ), "\" />\r\n" );
            Output ( szOutput );
        }
    }
    //<event date="2009/8/11" time="11:49:16.888" >
    //<Port>COM3</Port>
    //<Modem>SmartUSB56 Voice Modem</Modem>
    //<Win32>\\.\COM3</Win32>
    //</event>


    void BeginXML ( TraceLevel Level )
    {
        /**************************************************************************************************************
         * A thread uses this method of its instance of this class to start recording an event for debugging          *
         * purposes. If Level equals or exceeds [debug] level set in the profile (.INI file), it generates XML to     *
         * start the event, broadcasts it to the port set in [debug] port in the profile and appends it to the file   *
         * set in [database] file in the profile.                                                                     *
         **************************************************************************************************************/
        if ( Level >= m_TraceLevel )
        {
            char szOutput [ 4096 ];
            ZeroMemory ( szOutput, sizeof ( szOutput ));
            BuildXmlStart ( szOutput, sizeof ( szOutput ));

            StringCbCat( szOutput, sizeof ( szOutput ), ">\r\n" );
            Output ( szOutput );
        }
    }

    void XML ( TraceLevel Level, char* pszName, char* pszValue )
    {
        /**************************************************************************************************************
         * A thread uses this after using BeginXML above to add pszValue to the XML-encoded event with the name       *
         * pszName.                                                                                                   *
         **************************************************************************************************************/
        if ( Level >= m_TraceLevel )
        {
            char szOutput [ 4096 ];
            StringCbPrintf ( szOutput, sizeof ( szOutput ), "\t<%s>%s</%s>\r\n", pszName, pszValue, pszName );
            Output ( szOutput );
        }
    }

    void EndXML ( TraceLevel Level )
    {
        /**************************************************************************************************************
         * A thread uses this after using BeginXML and XML above to end the XML-encoded event.                        *
         **************************************************************************************************************/

/*		if ( Level == ResetLogFile )
		{
			CEventTrace::ResetTheLogFile();
		}else */
		if ( Level >= m_TraceLevel )
        {
            Output ( "</event>\r\n" );
        }
		
    }

    void HexDump ( TraceLevel Level, BYTE* pData, int DataLength )
    {
        /**************************************************************************************************************
         * A thread uses this method of its instance of this class to record DataLength bytes of binary data in pData *
         * as an event for debugging purposes. If Level equals or exceeds [debug] level set in the profile (.INI      *
         * file), the data is recorded in hexadecimal and also as a string showing printable characters in it. The    *
         * generated XML is broadcast to the port set in [debug] port in the profile and appended to the file set in  *
         * [database] file in the profile.                                                                            *
         **************************************************************************************************************/
        if ( Level >= m_TraceLevel )
        {
            char szOutput [ 4096 ];
            BuildXmlStart ( szOutput, sizeof ( szOutput ));
            StringCbCat( szOutput, sizeof ( szOutput ), ">\r\n\t<hex>" );
            StringCbCat( szOutput, sizeof ( szOutput ), CHexDump::GetHexString ( pData, DataLength ));
            StringCbCat( szOutput, sizeof ( szOutput ), "</hex>\r\n\t<ascii>" );
            StringCbCat( szOutput, sizeof ( szOutput ), CHexDump::GetAsciiString ( pData, DataLength ));
            StringCbCat( szOutput, sizeof ( szOutput ), "</ascii>\r\n</event>\r\n" );
            Output ( szOutput );
        }
    }

protected:
    void BuildXmlStart ( char* pszOutput, size_t nOutputSize )
    {
        /**************************************************************************************************************
         * This places the start of the required event XML in pszOutput (with maximum length nOutputSize). Generated  *
         * XML includes information set with the Identifier method and the current date and time.                     *
         **************************************************************************************************************/
        SYSTEMTIME SystemTime;
        GetLocalTime ( &SystemTime );

        size_t nszOutputLength = 0;

        StringCbCopy( pszOutput, nOutputSize, "<event " );
        if ( m_pszIdentity != NULL )
        {
            StringCbCat( pszOutput, nOutputSize, "identity=\"" );
            StringCbCat( pszOutput, nOutputSize, m_pszIdentity );
            StringCbCat( pszOutput, nOutputSize, "\" " );
        }

        if ( m_pszPort != NULL )
        {
            StringCbCat( pszOutput, nOutputSize, "port=\"" );
            StringCbCat( pszOutput, nOutputSize, m_pszPort );
            StringCbCat( pszOutput, nOutputSize, "\" " );
        }

        if ( m_pszDevice != NULL )
        {
            StringCbCat( pszOutput, nOutputSize, "device=\"" );
            StringCbCat( pszOutput, nOutputSize, m_pszDevice );
            StringCbCat( pszOutput, nOutputSize, "\" " );
        }

        StringCbLength ( pszOutput, nOutputSize, &nszOutputLength );
        StringCchPrintf ( pszOutput + nszOutputLength, nOutputSize - nszOutputLength, "date=\"%d/%d/%d\" time=\"%02d:%02d:%02d.%03d\" ", SystemTime.wYear, SystemTime.wMonth, SystemTime.wDay, SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond, SystemTime.wMilliseconds );
        StringCbLength ( pszOutput, nOutputSize, &nszOutputLength );
    }

    void Output ( char* pszOutput )
    {
        /**************************************************************************************************************
         * This broadcasts pszOutput then appends it to the logfile.                                                  *
         **************************************************************************************************************/
        //Broadcast ( pszOutput );
        DWORD dwWaitResult = WaitForSingleObject( ghMutex, INFINITE );
        SetFilePointer (
            m_hFileHandle,                                                                     // hFile [in] - the file
            0,                                           // lDistanceToMove [in] - relative to specified starting point
            NULL,                                                          // lpDistanceToMoveHigh [in, out] - not used
            FILE_END );                                            // dwMoveMethod [in] - starting point is end of file
        DWORD dwBytesWritten = 0;
        BOOL bOK = WriteFile(
            m_hFileHandle,                                                             // hFile [in] - file to write to
            pszOutput,                                                                 // lpBuffer [in] - data to write
            lstrlen ( pszOutput ),                                         // nNumberOfBytesToWrite [in] - size of data
            &dwBytesWritten,                                   // lpNumberOfBytesWritten [out] - bytes actually written
            NULL );                                                                // lpOverlapped [in, out] - not used
        ReleaseMutex ( ghMutex );
    }

    void XmlEncode ( char* pszInput, size_t nInputLength, char* pszOutput, size_t nOutputLength )
    {
        /**************************************************************************************************************
         * This copies nInputLength chars from pszInput to pszOutput, escaping problem chars to & sequences as        *
         * required. Maximum lengths of pszInput and pszOutput must be specified.                                     *
         **************************************************************************************************************/
        size_t nInputLength2 = 0;                                                      // actual length of input string
        StringCbLength ( pszInput, nInputLength, &nInputLength2 );
        ZeroMemory ( pszOutput, nOutputLength );
        int nOutputOffset = 0;
        for ( unsigned int nOffset = 0; nOffset < nInputLength2; nOffset++ )
        {
            switch ( *( pszInput + nOffset ))
            {
                case 0x3c:                                                                         //change '<' to &lt;
                    *( pszOutput+ nOutputOffset ) = '&';
                    *( pszOutput+ nOutputOffset + 1 ) = 'l';
                    *( pszOutput+ nOutputOffset + 2 ) = 't';
                    *( pszOutput+ nOutputOffset + 3 ) = ';';
                    nOutputOffset += 4;
                    break;
                case 0x3e:                                                                         //change '>' to &gt;
                    *( pszOutput+ nOutputOffset ) = '&';
                    *( pszOutput+ nOutputOffset + 1 ) = 'g';
                    *( pszOutput+ nOutputOffset + 2 ) = 't';
                    *( pszOutput+ nOutputOffset + 3 ) = ';';
                    nOutputOffset += 4;
                    break;
                case 0x26:                                                                        //change '&' to &amp;
                    *( pszOutput+ nOutputOffset ) = '&';
                    *( pszOutput+ nOutputOffset + 1 ) = 'a';
                    *( pszOutput+ nOutputOffset + 2 ) = 'm';
                    *( pszOutput+ nOutputOffset + 3 ) = 'p';
                    *( pszOutput+ nOutputOffset + 4 ) = ';';
                    nOutputOffset += 5;
                    break;
                case 0x27:                                                              //change single quote to &apos;
                    *( pszOutput+ nOutputOffset ) = '&';
                    *( pszOutput+ nOutputOffset + 1 ) = 'a';
                    *( pszOutput+ nOutputOffset + 2 ) = 'p';
                    *( pszOutput+ nOutputOffset + 3 ) = 'o';
                    *( pszOutput+ nOutputOffset + 4 ) = 's';
                    *( pszOutput+ nOutputOffset + 5 ) = ';';
                    nOutputOffset += 6;
                    break;
                case 0x22:                                                              //change double quote to &quot;
                    *( pszOutput+ nOutputOffset ) = '&';
                    *( pszOutput+ nOutputOffset + 1 ) = 'q';
                    *( pszOutput+ nOutputOffset + 2 ) = 'u';
                    *( pszOutput+ nOutputOffset + 3 ) = 'o';
                    *( pszOutput+ nOutputOffset + 4 ) = 't';
                    *( pszOutput+ nOutputOffset + 5 ) = ';';
                    nOutputOffset += 6;
                    break;
                default:
                    *( pszOutput+ nOutputOffset ) = *( pszInput + nOffset );
                    nOutputOffset++;
                    break;
            }
        }
    }

 };
