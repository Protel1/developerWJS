/**********************************************************************************************************************
 *                                      This file contains the CProtelHost class.                                     *
 *                                                                                                                    *
 * This represents a Master Auditor which is communicating with IVS and may control a number of ProtelDevices.        *
 * ProtelSerial and ProtelSocket inherit ProtelHost so their constructors also construct a ProtelHost which the       *
 * derived class uses to send a sequence of commands to a connected Master Auditor and handle its responses.          *
 *                                                                                                                    *
 * Overview of command/response process:                                                                              *
 *                                                                                                                    *
 * The derived class establishes a connection with the auditor, then calls ProtelHost::Transmit_I_Command. This       *
 * prepares the I command, sets up a response timeout according to the derived class GetWaitSeconds, then sends it to *
 * the auditor using the derived class Send method.                                                                   *
 *                                                                                                                    *
 * The derived class is responsible for getting the response data while monitoring for timeout or disconnection. Data *
 * received is passed to ProtelHost::MessageReceived. Once a valid response is completed, MessageReceived calls a     *
 * function according to the command that was sent to process it (e.g. Process_I_Response if an I command was sent).  *
 * Normally, this function sends another command and restarts the timeout but Process_A_Response calls the derived    *
 * class CloseDevice - this causes the connection to end which the derived class sees and handles.                    *
 *                                                                                                                    *
 * MessageReceived ignores a response without a valid checksum. If/when timeout occurs, the derived class can use     *
 * ProtelHost::Retransmit to send the previous command again.                                                         *
 *                                                                                                                    *
 * Commands are:                                                                                                      *
 *  A - abort communications                                                                                          *
 *  C - configuration packet for auditor/slaves                                                                       *
 *  D - auditor to dump DEX data                                                                                      *
 *  I - auditor to send its identification data                                                                       *
 *  N - auditor to send slaves' identification data                                                                   *
 *  O - operating system (firmware) packet for auditor/slaves                                                         *
 *  R - auditor to request DEX data from specified slave                                                              *
 *  S - auditor to set its date and time and return its status bytes                                                  *
 *  T - auditor to request date and time from specified slave                                                         *
 *  U - auditor to return DEX data packet                                                                             *
 *  V - remote control free vend configuration for slave                                                              *
 *  X - auditor to remove slave                                                                                       *
 *  Z - ping auditor                                                                                                  *
 *                                                                                                                    *
 * The full sequence of commands is: I -> S -> N -> U -> D -> V -> C -> O -> C -> A. U repeats until all DEX records  *
 * are uploaded. C and O repeat until all packets of the data are sent. I progresses directly to A if the host is     *
 * unknown or unauthorised. N progresses directly to V if there is no DEX data. Only one new firmware can be sent per *
 * connection but this may go to several devices. C commands are issued again after O commands if there are devices   *
 * with new firmware that need reconfiguration.                                                                       *
 *                                                                                                                    *
 *                                        Copyright (c) Protel Inc. 2009-2011                                         *
 **********************************************************************************************************************/
#pragma once

#include "AdoConnection.h"
#include "EventTrace.h"
#include "ProtelDevice.h"
#include "variantBlob.h"

#define _SECOND 10000000                                                           // multiplier for SetWaitableTimer
#define	MAXFAILCOUNTPERCALL  4				// Max fail responses per call b4 hangup
#define MAXFAILCOUNTPERCMD  3				// Max fail responses per cmd b4 hangup
//#define __WAIT_TIME__ 4

class CProtelHost
 {
protected:
    /*
     * The CENTRAL_AUDITOR database procedure executed when the I command response is processed can set the following
     * to cause the call to be terminated immediately if the monitor is unacceptable. This isn't currenty implemented
     * in the procedure.
     */
    enum ProtelCallFlag
    {
        ProcessNormally,
        HangupImmediately,
    } protelCallFlag;

    /*
     * The following is set when a command is sent which may take time for the auditor to execute. Ping (Z) commands
     * are sent periodically until the auditor indicates it is done. Another command is then sent depending on the
     * value set.
     */
    enum ReasonPinging
    {
        NotPinging,
        ConfigurationFile,
        FirmwareFile,
        RequestedDEXRead,
    } m_nReasonPinging;

    /*
     * The following controls the day of the week byte sent in the S command.
     */
    enum ProtelDayOfTheWeek : unsigned char
    {
        Sunday = 0x81,
        Monday = 0x82,
        Tuesday = 0x84,
        Wednesday = 0x88,
        Thursday = 0x90,
        Friday = 0xA0,
        Saturday = 0xC0,
    };

    /*
     * ProcessApplicationLayerError uses the following to record an error but the data is currently unused.
     */
    enum ApplicationLayerError : unsigned char
    {
        // Command is undefined or unknown
        UndefinedCommand = 0x00,
        // Communication with remote Failed
        RemoteCommunicationFailed = 0x01,
        // Host attempted to download OS and SRAM is not empty.  Need Upload/Dump first.
        UploadDexBeforeDownloadOS = 0x02,
        // Host attempted to download OS and monitor is in battery mode.
        MonitorInBatteryMode = 0x03,
    };

    enum LinkLayerError : unsigned char                                                                   // unused!!!!
    {
        // Bad Checksum
        BadChecksum = 0x00,
        // Bad Length
        BadLength = 0x01,
    };

    HANDLE m_hShutDown;                                                      // inherited, used by derived classes only
    HANDLE m_hTimer;                       // response timeout, set by ProtelHost, created and checked by derived class
    int m_nMessageBufferOffset;                                           // position in szMessageBuffer for next chunk
    BYTE m_szMessageBuffer [ 4096 ];                                                  // holds entire received response
    BYTE m_szPayload [ 4096 ];                                // command or response data (without command or checksum)
    BYTE m_transmitBuffer [ 4096 ];                                              // could this be local to Transmit????
    CAdoConnection* m_padoConnection;

    int m_nLastTransmission;                                                            // length of m_LastTransmission
    BYTE m_LastTransmission [ 4096 ];            // last command transmitted (used by Retransmit, also by ProtelSerial)
    int m_nCommsErrs;                                                    // errors count, maintained by ContinueComms()
	int m_nReXmitFailCountPercall;			// number of "F" received per call b4 hanging up call (max = 5)
	int m_nReXmitFailCountPercmd;			// number of "F" received per command b4 hanging up call (max = 3)
	BYTE m_nLastCmd;						// last transmitted command

    CEventTrace m_EventTrace;                                                      // records events - see EventTrace.h

    char m_SerialNumber [ 64 ];                                                              // from I command response
    char m_CellModemSimmID [ 64 ];                                                           // from I command response
    char m_CardReaderID [ 64 ];                                                              // from I command response
    char m_CardReaderRevision [ 64 ];                                                        // from I command response
    char m_CardReaderFirmwareVersion [ 64 ];                                                 // from I command response
    char m_CardReaderConfigVersion [ 64 ];                                                   // from I command response
    bool RamFull;                                                                            // from S command response
    bool HaveDexFiles;                                                                       // from S command response
    bool BatteryMode;                                                                        // from S command response
    int CallReason;                                                                          // from S command response
    int CallNumber;                                             // obtained from database by Database_AddNewCall method
    double dCallStartTime;                         // set by Initialize method from system clock at start of connection
    char m_szCurrentCommand [ 2 ];                            // current command (e.g. "I") as a null-terminated string

    CProtelDevice* m_pProtelDevices [ 32 ];                                          // devices controlled by this host

    int m_nCurrentAuditDevice;                                          // current position in m_pProtelDevices (above)
    int m_nAuditDevices;                                               // number of devices in m_pProtelDevices (above)
    long m_nConfigurationLength;                                                           // does this do anything????
    BYTE *m_pConfiguration;                                                                // does this do anything????
    int m_nDexFileRemoteAddress;                   // device whose DEX data is available (e.g. from R command response)
    char m_ActiveSerialNumber [ 64 ];          // used when receiving/saving DEX data (is it always == m_SerialNumber?)
    bool m_NormalShutdown;                                  // TRUE if call ended normally, used by Database_FinishCall
	bool FailThisCall;							// true if call failed after N-cmd received but got dex

//    ProtelCallFlag protelCallFlag;

    bool Download2ndConfiguration;           // TRUE once firmware downloads started (devices may need reconfiguration)

//    ReasonPinging m_nReasonPinging;

public:
    CProtelHost(HANDLE hShutDown) :
        Closed ( false ),                      // this is an initialization list which sets members to specified values
        m_hShutDown ( hShutDown ),
        m_nLastTransmission ( 0 ),
        m_nMessageBufferOffset ( 0 ),
        m_NormalShutdown ( true ),
        Download2ndConfiguration ( false ),
        CallNumber ( 0 ),
        dCallStartTime (( double ) 0 ),
        m_nReasonPinging ( ReasonPinging::NotPinging ),
        m_padoConnection ( NULL )
    {
        /**************************************************************************************************************
         * CONSTRUCTOR.                                                                                               *
         **************************************************************************************************************/
        ZeroMemory ( m_szMessageBuffer, sizeof ( m_szMessageBuffer ));
        ZeroMemory ( m_szPayload, sizeof ( m_szPayload ));
        ZeroMemory ( m_transmitBuffer, sizeof ( m_transmitBuffer ));
        ZeroMemory ( m_LastTransmission, sizeof ( m_LastTransmission ));
        ZeroMemory ( m_szCurrentCommand, sizeof ( m_szCurrentCommand ));
        for ( int nLoop = 0; nLoop < ( sizeof ( m_pProtelDevices ) / sizeof ( m_pProtelDevices[ 0 ] )); nLoop++ )
        {
            m_pProtelDevices[ nLoop ] = NULL;
        }

        m_hTimer = NULL;

        //m_EventTrace.Event( CEventTrace::Details, "CProtelHost::CProtelHost(void)" );
        Initialize();
    }

    virtual ~CProtelHost(void)
    {
        /**************************************************************************************************************
         * DESTRUCTOR.                                                                                                *
         **************************************************************************************************************/
        if ( m_hTimer != NULL )
        {
            CloseHandle ( m_hTimer );
            m_hTimer = NULL;
        }
        CleanupProtelDevices();                                                                       // delete devices
        if ( m_padoConnection != NULL )
        {
            delete m_padoConnection;
            m_padoConnection = NULL;
        }
    }

    virtual void Send(LPBYTE pszBuffer, int BufferLength)                   // overridden by ProtelSerial or ProtelHost
    {
        throw "Not Implemented";
    }

    virtual void Shutdown ( void )                                          // overridden by ProtelSerial or ProtelHost
    {
        throw "Not Implemented";
    }

    bool Closed;                                                    // set by CProtelSocket::SocketThreadProc when done

protected:
    void MessageReceived ( BYTE* pBuffer, int bytesRead )
    {
        /**************************************************************************************************************
         * This is called by a class such as CProtelSerial or CProtelSocket which has inherited CProtelHost to pass   *
         * received data (bytesRead in pBuffer - part or all of a command response) for processing. Note that, in     *
         * general, a new, perhaps different, command is issued by the code that processes the response once it is    *
         * complete (e.g. Process_D_Response issues a V command). This new command is sent using the Send method of   *
         * the derived class (e.g. CProtelSocket::Send).                                                              *
         **************************************************************************************************************/
        m_EventTrace.HexDump( CEventTrace::Details, pBuffer, bytesRead );

        if ( m_nMessageBufferOffset == 0 && pBuffer[0] != 'T' )
        {
            /*
             * This was the first response data received. It doesn't start with 'T' - perhaps there was line noise.
             * We look for a 'T' later on.
             */
            BYTE* Tpos = ( BYTE* ) StrChr(( LPCTSTR ) pBuffer, 'T' );
            if ( Tpos == NULL )
                return;                                                             // T not found, ignore all the data

            /*
             * We found the T. We ignore all data preceding it (i.e. noise).
             */
            bytesRead -= Tpos - pBuffer;
            pBuffer = Tpos;
        }

        if ( m_nMessageBufferOffset + bytesRead > sizeof(m_szMessageBuffer) )
        {
            /*
             * The received data would overflow the buffer. We limit it to what will fit.
             */
            bytesRead = sizeof m_szMessageBuffer - m_nMessageBufferOffset;
        }

        /*
         * We append the received bytes to this class' message buffer and add them to the count.
         */
        MoveMemory ( m_szMessageBuffer + m_nMessageBufferOffset, pBuffer, bytesRead );
        m_nMessageBufferOffset += bytesRead;
        if ( m_nMessageBufferOffset >= 3
            && m_szMessageBuffer[1] <= ( m_nMessageBufferOffset - 3 ) ) // full message (incl T, length, csum) received
        {
            Database_CommunicationsData ( false, false, m_szMessageBuffer, m_nMessageBufferOffset );
                                                                                     // record in COMM_SERVER_LOG, etc.
            if (m_nMessageBufferOffset >= 4                         // message long enough to include command character
                && IsValidChecksum( m_szMessageBuffer, m_szMessageBuffer[ 1 ] + 3 )                   // checksum is OK
                && IsMatchingResponse( m_szMessageBuffer, m_LastTransmission))              // response matches command
            {
                /*
                 * We got a valid response.
                 */
                CancelTimer( m_hTimer );                                                            // stop the timeout
                ContinueComms( true );                                                    // count response received OK
                //Database_Dialog ( false, m_szMessageBuffer, m_szMessageBuffer[1]+3 );  // doesn't appear to do anything
                int nPayloadLength = m_szMessageBuffer[1] - 1;                         // payload excludes command byte
                ZeroMemory ( m_szPayload, sizeof ( m_szPayload ));
                MoveMemory ( m_szPayload, m_szMessageBuffer + 3, nPayloadLength );

                switch ( m_szMessageBuffer[2] )                                                  // process the command
                {
                    // Abort command
                    case 'A':
                        Process_A_Response();
                        break;

                    // Configuration file
                    case 'C':
                        Process_C_Response();
                        break;

                    // Dump (erase) ram
                    case 'D':
                        Process_D_Response();
                        break;

                    // Application Layer error
                    case 'E':
                        ProcessApplicationLayerError( m_szPayload [ 0 ]);
                        break;

                    // Link Control Layer error
                    case 'F':
                        //ContinueComms( false );             // count an error - could cause disconnect on later timeout
                        Retransmit();
                        break;

                    // Identify
                    case 'I':
                        Process_I_Response();
                        break;

                    // New Audit Device Numbers needing configuration
                    case 'N':
                        Process_N_Response( nPayloadLength );
                        break;

                    // Operating system download
                    case 'O':
                        Process_O_Response();
                        break;

                    // Read DEX Data
                    case 'R':
                        Process_R_Response();
                        break;

                    // Status
                    case 'S':
                        Process_S_Response();
                        break;

                    // Time and Date request
                    case 'T':
                        break;

                    // Upload DEX files
                    case 'U':
                        Process_U_Response ( nPayloadLength );
                        break;

                    // Free Vend Configuration
                    case 'V':
                        Process_V_Response();
//                            bug_buf[8] = (BYTE) 'V';                                                      //diagnostics!!!!
//                            m_EventTrace.HexDump( CEventTrace::Details, bug_ptr, bug_len );
                        break;

                    // Remove remote
                    case 'X':
                        break;

                    // Ping
                    case 'Z':
                        Process_Z_Response( nPayloadLength );
                        break;
                }
            }
            else
            {
                /*
                 * The response appeared to be complete but was invalid (bad checksum, too short or not matching the
                 * command). We start looking for a new sync character (T).
                 *
                 * Note: if the response didn't match the command, it is ignored and we keep looking for a valid
                 * response until timeout occurs. If more responses are received that don't match, they will be
                 * ignored too but they get purged from the communications channel. If/when timeout occurs, the last
                 * command is retransmitted and eventually the matching response should be seen.
                 */
                m_nMessageBufferOffset = 0;
            }
        }
    }

    bool IsValidChecksum ( BYTE* Buffer, int BufferLength )
    {
        /**************************************************************************************************************
         * This returns true if the checksum of a message in Buffer of Length bytes, including the sync character and *
         * checksum position, is valid.                                                                               *
         **************************************************************************************************************/
        unsigned char Checksum = CalculateChecksum( Buffer, BufferLength );
        if (Checksum == *( Buffer + BufferLength - 1 ))
        {
            return true;
        }
        return false;
    }

    bool IsMatchingResponse ( BYTE* Response, BYTE* Command )
    {
        /**************************************************************************************************************
         * This returns true if Response, including any address or packet number, matches the command in Command.     *
         **************************************************************************************************************/
        unsigned char Cmd = Response[2];                                       // the command character in the response

        if ( Cmd == 'E' || Cmd == 'F' )
        {
            return true;                                                  // it was an error response - always accepted
        }

        if ( Cmd != Command[2] )
        {
            return false;                                                     // the command doesn't match what we sent
        }

        if ( Cmd == 'R' || Cmd == 'T' )
        {
            return Response[3] == Command[3];      // DEX or Time from specific slave: OK if the 1 byte address matches
        }

        else if ((Cmd == 'N' || Cmd == 'U') && Response[3] == 0xff && Response[4] == 0xff )
        {
            return true;                                                 // Slave list or DEX: Packet ffff is always OK
        }

        else if ( Cmd == 'C' || Cmd == 'N' || Cmd == 'O' || Cmd == 'U' )
        {
            return Response[3] == Command[3] && Response[4] == Command[4];
                                  // Configuration, slave list, firmware or DEX: OK if the 2 byte packet number matches
        }

        return true;                              // Other commands: OK - command doesn't have address or packet number
    }

    unsigned char CalculateChecksum ( BYTE* Buffer, int Length )
    {
        /**************************************************************************************************************
         * This returns the valid/expected checksum for a message in Buffer of Length bytes, including the sync       *
         * character and checksum position.                                                                           *
         **************************************************************************************************************/
        long BufferSum = 0;
        for (int nOffset = 0; nOffset < (Length - 1); nOffset++)
        {
            BufferSum += *( Buffer + nOffset );
        }
        long bitwiseNOT = ~BufferSum;
        return ((unsigned char)(bitwiseNOT & 0x000000ff));
    }

    void ResetReceivedBuffer ( void )
    {
        /**************************************************************************************************************
         * This is used when transmitting a message to clear the received message buffer in preparation for receiving *
         * the response.                                                                                              *
         **************************************************************************************************************/
        ZeroMemory ( m_szMessageBuffer, sizeof ( m_szMessageBuffer ));
        m_nMessageBufferOffset = 0;
    }

    virtual void Initialize ( void )
    {
        /**************************************************************************************************************
         * This is called from the class constructor and again from ProtelSerial or ProtelSocket Initialize method    *
         * when a new connection is made.                                                                             *
         **************************************************************************************************************/
        m_nMessageBufferOffset = 0;
        ZeroMemory ( m_szMessageBuffer, sizeof ( m_szMessageBuffer ));
        ZeroMemory ( m_szPayload, sizeof ( m_szPayload ));
        ZeroMemory ( m_transmitBuffer, sizeof ( m_transmitBuffer ));

        m_nLastTransmission = 0;
        ZeroMemory ( m_LastTransmission, sizeof ( m_LastTransmission ));
        m_nCommsErrs = 0;
		m_nReXmitFailCountPercall = 0;
		m_nReXmitFailCountPercmd = 0;
		m_nLastCmd = (BYTE)"";
		FailThisCall = false;					

        ZeroMemory ( m_SerialNumber, sizeof ( m_SerialNumber ));
        ZeroMemory ( m_CellModemSimmID, sizeof ( m_CellModemSimmID ));
        ZeroMemory ( m_CardReaderID, sizeof ( m_CardReaderID ));
        ZeroMemory ( m_CardReaderRevision, sizeof ( m_CardReaderRevision ));
        ZeroMemory ( m_CardReaderFirmwareVersion, sizeof ( m_CardReaderFirmwareVersion ));
        ZeroMemory ( m_CardReaderConfigVersion, sizeof ( m_CardReaderConfigVersion ));

        RamFull = false;
        HaveDexFiles = false;
        BatteryMode = false;
        CallReason = 0;
        CallNumber = 0;
        dCallStartTime = ( double ) 0;
        ZeroMemory ( m_szCurrentCommand, sizeof ( m_szCurrentCommand ));

        m_nCurrentAuditDevice = 0;
        m_nAuditDevices = 0;
        m_nConfigurationLength = 0;
        m_pConfiguration = NULL;
        m_nDexFileRemoteAddress = 0;
        ZeroMemory ( m_ActiveSerialNumber, sizeof ( m_ActiveSerialNumber ));
        m_NormalShutdown = true;

        protelCallFlag = ProtelCallFlag::ProcessNormally;
        Download2ndConfiguration = false;
        m_nReasonPinging = ReasonPinging::NotPinging;
        m_NormalShutdown = true;                                                                  // is this needed????
        protelCallFlag = ProtelCallFlag::ProcessNormally;                                         // is this needed????

        /*
         * We delete any existing database connection, then create and open a new one as specified by the connection
         * string in the .INI file.
         */
        if ( m_padoConnection != NULL )
        {
            delete m_padoConnection;
            m_padoConnection = NULL;
        }
        CProfileValues profileValues;
        m_padoConnection = new CAdoConnection;

        /*
         * We open the connection to the database. If this fails (because the database is busy), we wait 1 second and
         * retry. We make up to 10 tries, then delete the connection entirely.
         */
        int tries = 10;
        do
        {
            if ( m_padoConnection->ConnectionStringOpen( profileValues.GetConnectionString()) == true )
            {
                break;                                                                          // connection opened OK
            }
            else if (--tries)
            {
                Sleep(1000);                                                    // failed - wait 1 second and try again
            }
            else
            {
                delete m_padoConnection;                                                                   // giving up
                m_padoConnection = NULL;
            }
        }
        while ( m_padoConnection != NULL);

        //CancelWaitableTimer( m_hTimer );
        CancelTimer( m_hTimer );


        SYSTEMTIME CallStartTime;
        GetSystemTime ( &CallStartTime );
        SystemTimeToVariantTime ( &CallStartTime, &dCallStartTime );

        //m_EventTrace.Event( CEventTrace::Details, "CProtelHost::Initialize ( void )" );

        m_nCurrentAuditDevice = 0;
        m_nAuditDevices = 0;
        m_nDexFileRemoteAddress = 0;

        CleanupProtelDevices();                                                          // delete any existing devices
        for ( int nLoop = 0; nLoop < ( sizeof ( m_pProtelDevices ) / sizeof ( m_pProtelDevices[ 0 ] )); nLoop++ )
        {
            m_pProtelDevices[ nLoop ] = new CProtelDevice ( m_padoConnection );
        }
    }

    bool Transmit_A_Command ( bool NormalShutdown )                                      // Abort or end communications
    {
        m_NormalShutdown = NormalShutdown;
        BYTE ShutdownFlag = ( BYTE ) NormalShutdown;
		m_pProtelDevices [ m_nCurrentAuditDevice ]->SetNewConfigurationLength( (long) 0);			// wjs bugg fixing... cfg dwnload
        Transmit( 'A', &ShutdownFlag, sizeof ( ShutdownFlag ));
        return true;
    }

    void Process_A_Response ( void )
    {
		if (m_NormalShutdown == true)
			CloseDevice(1);		//call successfull disconnect
		else
			CloseDevice(0);    // call failed disconnect
    }

    bool Transmit_C_Command ( void )                                                              // Send configuration
    {
        BYTE* pBuffer = NULL;

        long nLength = 0;

		// the following 2 lines of code added to accomodate the special case of a remote monitor config download
		//pBuffer = m_pProtelDevices [ m_nCurrentAuditDevice ]->getConfigData();	// wjs 2/21/2011 get the configdata 4 download
		//nLength = m_pProtelDevices [ m_nCurrentAuditDevice ]->getConfigDatalength();
		CAdoStoredProcedure madoStoredProcedure ( "PKG_COMM_SERVER.FREEBEE_CMD_SUCCESSFUL " );
        while ( pBuffer == NULL )
        {
            /*
             * We see if there is a configuration chunk to send to the controlled device.
             
            m_EventTrace.Event( CEventTrace::Information, "void CProtelHost::1Transmit_C_Command [%d](%d)",1,1);*/
            pBuffer = m_pProtelDevices [ m_nCurrentAuditDevice ]->GetNextConfiguration( nLength );
           // m_EventTrace.Event( CEventTrace::Information, "void CProtelHost::2Transmit_C_Command [%d](%d)",1,1);

            if ( pBuffer == NULL ||  m_nCurrentAuditDevice >= m_nAuditDevices)
            {
                /*
                 * There isn't. Either the device didn't need configuration or its configuration has been completed.
                 */
                if ( m_nReasonPinging == ReasonPinging::ConfigurationFile )
                {
                    /*
                     * We completed a configuration and will ping the master until it's ready. (When it is, we get
                     * called again: GetNextConfiguration will return NULL again but m_nReasonPinging will have
                     * changed.)
                     */
                    Transmit_Z_Command();
                    return true;
                }

                /*
                 * The device didn't need configuration, already received it as a duplicate of one for a previous
                 * device or we reconfigured it and pinged the master and it is now ready. We go on to the next
                 * device.
                 */
                m_nCurrentAuditDevice++;
                if ( m_nCurrentAuditDevice >= m_nAuditDevices )
                {
                    /*
                     * There are no more devices. We check if firmware downloads have been done.
                     */
                    //if ( Download2ndConfiguration == true )
                    //{
					// **** code to call to freebee_cmd_successful added 4/2/15 bug 3844
						m_nCurrentAuditDevice = 0;
			            _variant_t vtCallStartTime ( dCallStartTime, VT_DATE );     // Note: time was set by ProtelHost::Initialize
						_bstr_t bstrCentralAuditor( m_SerialNumber );
						_variant_t vtCentralAuditor ( bstrCentralAuditor );
						//_bstr_t bstrAuditor( m_szSerialNumber );
						//_variant_t vtAuditor ( bstrAuditor );
						char* SerialNumber = m_pProtelDevices [ m_nCurrentAuditDevice ]->GetSerialNumber();
						_bstr_t bstrSerialNumber( SerialNumber );
						_variant_t vtAuditor ( bstrSerialNumber );
						char* FBeeSerialNumber = m_pProtelDevices [ m_nCurrentAuditDevice ]->GetFreeBeeSerialNumber();
						_bstr_t bstrFBSerialNumber( FBeeSerialNumber );
						_variant_t vtFbeeID ( bstrFBSerialNumber );
						int FBchannel = m_pProtelDevices [ m_nCurrentAuditDevice ]->GetFreeBeeControllerflag();
						_bstr_t bstrFBchannel( FBchannel );
						_variant_t vtFbchannel ( bstrFBchannel );
						if ( Transmit_V_Command() == true)             // they have - end connection, indicating normal shutdown
						{	
							madoStoredProcedure.AddParameter( "pi_CALL_START_TIME", vtCallStartTime, ADODB::DataTypeEnum::adDate, ADODB::ParameterDirectionEnum::adParamInput, sizeof ( double ));
				            madoStoredProcedure.AddParameter( "pi_centralAuditor", vtCentralAuditor, ADODB::DataTypeEnum::adBSTR, ADODB::ParameterDirectionEnum::adParamInput, bstrCentralAuditor.length());
							madoStoredProcedure.AddParameter( "pi_auditor", vtAuditor, ADODB::DataTypeEnum::adBSTR, ADODB::ParameterDirectionEnum::adParamInput, bstrSerialNumber.length());
//							int temFreebee = GetFreeBeeSerialNumber();
							madoStoredProcedure.AddParameter( "pi_freebeeid", vtFbeeID, ADODB::DataTypeEnum::adBSTR, ADODB::ParameterDirectionEnum::adParamInput, bstrFBSerialNumber.length());
							madoStoredProcedure.AddParameter( "pi_channel_num",  vtFbchannel, ADODB::adInteger, ADODB::adParamInputOutput, bstrFBchannel.length());
							m_padoConnection->ExecuteNonQuery( madoStoredProcedure, false ); 
						}
						else
						{
							return false;
						}
//                    //}
                    //else		removed wjs 4/22/2011
                    //{
                    //    m_nCurrentAuditDevice = 0;                // they haven't - check first device for new firmware
                    //    Transmit_O_Command();
                    //}
                    return true;
                } // end if ( m_nCurrentAuditDevice >= m_nAuditDevices )

            }		// end if ( pBuffer == NULL ||  m_nCurrentAuditDevice >= m_nAuditDevices)
			//m_nCurrentAuditDevice++;
        }		// end while ( pBuffer == NULL )

        /*
         * We have a configuration chunk to send. We do this after marking what we are doing.
         */
        m_nReasonPinging = ReasonPinging::ConfigurationFile;
		//m_pProtelDevices [ m_nCurrentAuditDevice ]->SetNewConfigurationLength(nLength - 4);
		//m_pProtelDevices [ m_nCurrentAuditDevice ]->setNewConfig(m_pConfiguration + 4, nLength - 4);
            //m_EventTrace.Event( CEventTrace::Information, "void CProtelHost::3Transmit_C_Command [%d](%d)",1,1);
        Transmit( 'C', pBuffer, nLength );
            //m_EventTrace.Event( CEventTrace::Information, "void CProtelHost::4Transmit_C_Command [%d](%d)",1,1);
  //      if ( m_pConfiguration[ 3 ] == 0xff && m_pConfiguration[ 4 ] == 0xff )       // this was the last packet
		//{
		//	m_nCurrentAuditDevice = m_nAuditDevices;
		//}

        return true;
    }

    void Process_C_Response ( void )
    {
        //m_EventTrace.Event( CEventTrace::Information, "void CProtelHost::Process_C_response [%d](%d)",1,1);
        Transmit_C_Command();                                                                         // get next chunk
    }

    void Transmit_D_Command ( void )                                                                // Dump (erase) RAM
    {
        Transmit( 'D', NULL, 0 );
    }

    void Process_D_Response ( void )
    {
        //m_nCurrentAuditDevice = 0;
		if (FailThisCall)
		{
			Transmit_A_Command( true );
			//CloseDevice( 0 );
		}
		else
		{
        Transmit_O_Command();
		}
    }

    void Transmit_I_Command ( void )                                         // Get device ID - this starts the session
    {
        Transmit ( 'I', NULL, 0 );
    }

    void Process_I_Response ( void )
    {
        /*
         * This parses the I command response to obtain serial numbers, etc. and set data members of the class
         * accordingly. It also checks/updates the database and sets the master serial number and call details for
         * all the ProtelDevices. If the database indicates to continue, it issues an S (clock set and get status)
         * command, otherwise it issues an A (abort) command.
         *
         * The response from the Master Auditor will be a repeat of the command and the 8 bytes of the serial number.
         * If the connection is cellular, the serial number will be followed by a colon as a field separator and then
         * a 20 byte SIM ID of the cellular modem.
         *  T | 0x1e | I | Monitor Serial Number | : | Cell Modem SIM ID | Checksum
         *
         * Monitor Serial # : SIM ID : Card Reader Serial # : --card reader revision -- : Card Reader Firmware
         * Version : Card Reader Config #
         *  Example: RMC79260:89310380106023138946:RFX02005::AC0000045A:1106
         */

//Transmit_A_Command(true);
//return;
        BYTE* pszField = m_szPayload;                                                     // Start of field (first one)
        BYTE* pszColon = pszField;                                                 // Colon position (make it not NULL)
		bool corrupt_serial = false;						// for corrupt serial n
		bool msimmid = false;
        for (int nFieldNum = 0; nFieldNum <= 5; nFieldNum++)                           // parse response and get fields
        {
            if ( pszColon != NULL )
            {
                /*
                 * There may be another colon. We search and, if found, we change it to an end-of-string.
                 */
                pszColon = ( BYTE* )StrChr (( LPCTSTR ) pszField, ':' );
                if ( pszColon != NULL )
                {
                    *( pszColon ) = '\0';
                }
            }

            /*
             * We copy the field as required.
             */
            switch ( nFieldNum )
            {
                case 0:                                                                             // Monitor Serial #
                    StringCbCopy ( m_SerialNumber, sizeof ( m_SerialNumber ), ( LPCTSTR )pszField );
					//code added to catch corrupt serial num string and give to sp from the database
					for (int snum = 0; snum <= sizeof ( m_SerialNumber ); snum++)                           
					{
						if (isalpha(m_SerialNumber[snum]) || isdigit(m_SerialNumber[snum]))
						{
						}
						else
						{
							corrupt_serial = true;
							break;
						}
					}
                    break;

                case 1:                                                                                       // SIM ID
                    StringCbCopy ( m_CellModemSimmID, sizeof ( m_CellModemSimmID ), ( LPCTSTR )pszField );
					// code to catch corrupt sn	6/30/15
					if (strlen(m_CellModemSimmID) > 0)
					{
						msimmid = true;
					}
                    break;

                case 2:                                                                         // Card Reader Serial #
                    StringCbCopy ( m_CardReaderID, sizeof ( m_CardReaderID ), ( LPCTSTR )pszField );
                    break;

                case 3:                                                                         // card reader revision
                    StringCbCopy ( m_CardReaderRevision, sizeof ( m_CardReaderRevision ), ( LPCTSTR )pszField );
                    break;

                case 4:                                                                 // Card Reader Firmware Version
                    StringCbCopy ( m_CardReaderFirmwareVersion, sizeof ( m_CardReaderFirmwareVersion ), ( LPCTSTR )pszField );
                    break;

                case 5:                                                                         // Card Reader Config #
                    StringCbCopy ( m_CardReaderConfigVersion, sizeof ( m_CardReaderConfigVersion ), ( LPCTSTR )pszField );
                }

            if ( pszColon == NULL )
            {
                /*
                 * No colon found: there isn't another field. We create a zero-length one.
                 */
                *( pszField ) = '\0';
            }
            else
            {
                /*
                 * There was a colon. We skip over it and start the next field.
                 */
                pszField = ++pszColon;
            }
        }

        Database_UpdateCentralAuditor(); // updates COMM_SERVER_CALL, COMM_SERVER_DETAILS and COMM_SERVER_LOG tables                                                             // in this class
        m_EventTrace.Event( CEventTrace::Information, "void D CProtelHost::Process_I_Response [%s](%d)",m_SerialNumber,0);
		// code to catch corrupt sn 6/30/15 WJS
		if (corrupt_serial)
		{
/*			char* valid_sn = corrected_serial(msimmid); */
            CAdoStoredProcedure adoStoredProcedure1 ( "MONITOR_RECOVERY_PKG.getSerialNumFromCorrupt" );


            _variant_t vtCallNumber (( long ) CallNumber, VT_I4 );
            adoStoredProcedure1.AddParameter("pi_call_number", vtCallNumber, ADODB::DataTypeEnum::adInteger, ADODB::ParameterDirectionEnum::adParamInput, sizeof(long));

            _bstr_t bstrCentralAuditor( m_SerialNumber );
            _variant_t vtCentralAuditor ( bstrCentralAuditor );
            adoStoredProcedure1.AddParameter( "pi_centralauditor ", vtCentralAuditor, ADODB::DataTypeEnum::adBSTR, ADODB::ParameterDirectionEnum::adParamInput, bstrCentralAuditor.length());
//			sn and corru
            adoStoredProcedure1.AddParameter( "pi_corrupt_serialnum ", vtCentralAuditor, ADODB::DataTypeEnum::adBSTR, ADODB::ParameterDirectionEnum::adParamInput, bstrCentralAuditor.length());

			_variant_t vtCallStartTime ( dCallStartTime, VT_DATE );     // Note: time was set by ProtelHost::Initialize
            adoStoredProcedure1.AddParameter( "pi_callstarttime", vtCallStartTime, ADODB::DataTypeEnum::adDate, ADODB::ParameterDirectionEnum::adParamInput, sizeof ( double ));


//			if (msimmid)		// has simm
//			{
				_bstr_t bstrCellModemSimmID( m_CellModemSimmID );
		        _variant_t vtCellModemSimmID ( bstrCellModemSimmID );
			    adoStoredProcedure1.AddParameter( "pi_esn", vtCellModemSimmID, ADODB::DataTypeEnum::adBSTR, ADODB::ParameterDirectionEnum::adParamInput, bstrCellModemSimmID.length());

            _bstr_t bstrCentralAuditor1( "" );
            _variant_t vtCentralAuditor1 ( bstrCentralAuditor1 );
            adoStoredProcedure1.AddParameter("po_ActualSerialNum", vtCentralAuditor1, ADODB::DataTypeEnum::adBSTR, ADODB::ParameterDirectionEnum::adParamOutput, bstrCentralAuditor.length());

				
				
//procedure getSerialNumFromCorrupt ( 
//                pi_call_number comm_server_log.callnumber%type
//                , pi_centralauditor comm_server_log.centralauditor%type
//                , pi_callstarttime comm_server_log.callstarttime%type
//                , pi_corrupt_serialnum varchar2
//                , pi_esn varchar2 default null 
//                , po_ActualSerialNum out COMM_SERVER_CALL.CENTRALAUDITOR%type                
//)
        m_EventTrace.Event( CEventTrace::Information, "void A CProtelHost::Process_I_Response [%s](%d)",m_SerialNumber,0);

				try
				{
		        if ( m_padoConnection == NULL )
				{
		        m_EventTrace.Event( CEventTrace::Information, "void NULL CProtelHost::Process_I_Response [%s](%d)",m_SerialNumber,0);
				}


					int ret =   m_padoConnection->ExecuteNonQuery( adoStoredProcedure1, false );

					_variant_t vtCorrectedSerial = adoStoredProcedure1.GetParameter("po_ActualSerialNum");
					_bstr_t bstrCorrectedSerial (( _bstr_t ) vtCorrectedSerial );
					StringCbCopy ( m_SerialNumber, sizeof ( m_SerialNumber ), ( const char* ) bstrCorrectedSerial );
        m_EventTrace.Event( CEventTrace::Information, "void CProtelHost::Process_I_Response [%s](%d)",m_SerialNumber,ret);

					Database_UpdateCentralAuditor(); // updates COMM_SERVER_CALL, COMM_SERVER_DETAILS and COMM_SERVER_LOG tables                                                             // in this class
        m_EventTrace.Event( CEventTrace::Information, "void 2 CProtelHost::Process_I_Response [%s](%d)",m_SerialNumber,ret);
				}
		        catch ( _com_error &comError )
				{
					m_EventTrace.Event ( CEventTrace::Information, "MONITOR_RECOVERY_PKG.getSerialNumFromCorrupt <--> ERROR: %s", CErrorMessage::ReturnComErrorMessage ( comError ));
		        }




		}

//        Database_UpdateCentralAuditor(); // updates COMM_SERVER_CALL, COMM_SERVER_DETAILS and COMM_SERVER_LOG tables                                                             // in this class

        /*
         * We set the central auditor serial number, call start time and call number for all devices controlled by
         * this central auditor.
         */
        for ( int nLoop = 0; nLoop < ( sizeof ( m_pProtelDevices ) / sizeof ( m_pProtelDevices[ 0 ] )); nLoop++ )
        {
            m_pProtelDevices[ nLoop ]->CentralAuditor = m_SerialNumber;
            m_pProtelDevices[ nLoop ]->CallStartTime = dCallStartTime;
            m_pProtelDevices[ nLoop ]->CallNumber = CallNumber;
        }
        if ( protelCallFlag == ProtelCallFlag::HangupImmediately ) // PKG_COMM_SERVER.CENTRAL_AUDITOR presently never sets this
        {
            Transmit_A_Command ( true );                                  // end connection, indicating normal shutdown
        }
        else
        {
            Transmit_S_Command();                                                         // database says OK, continue
        }
    }

    void Transmit_N_Command ( int DeviceNumber )                                          // Get next audit device data
    {
        BYTE DeviceNumberBytes[ 2 ];
        DeviceNumberBytes[ 0 ] = (( DeviceNumber & 0xff00 ) >> 8 );
        DeviceNumberBytes[ 1 ] = ( DeviceNumber & 0xff );
        Transmit( 'N', DeviceNumberBytes, sizeof ( DeviceNumberBytes ));
    }

    void Process_N_Response ( int nPayloadLength )
    {
        int Offset = 2;                                   // exclude packet number
		bool dbstatus = true;
        while ( Offset < nPayloadLength
            && m_nAuditDevices < ( sizeof ( m_pProtelDevices ) / sizeof ( m_pProtelDevices[ 0 ] )))  // check added!!!!
        {
            /*
             * The payload could contain one or more devices. We put each into the AuditDevice struct belonging to
             * the next ProtelDevice in the table, continuing until the payload is exhausted or the table is full.
             * (ProtelDevice::AuditDevice put method is SetAuditDevice, this parses the data into Serial Number,
             * Address, Firmware Version, etc. The last byte is the status - bit 7 is set if the device needs
             * reconfiguration.)
             */
            dbstatus = m_pProtelDevices[ m_nAuditDevices ]->AuditDevice = ( m_szPayload + Offset );
			if (dbstatus == false)
			{
                ZeroMemory ( m_ActiveSerialNumber, sizeof ( m_ActiveSerialNumber ));
                MoveMemory ( m_ActiveSerialNumber, m_SerialNumber, lstrlen ( m_SerialNumber ));
                m_nDexFileRemoteAddress = 0;            // DEX data will be from the host itself
				FailThisCall = true;					// bad cardreader serial number
				Transmit_U_Command( 1 );
				//CloseDevice ( 0 );
				return;
			}	
            m_nAuditDevices++;
            Offset += sizeof ( AuditDevice );
        }

        int indxOfFBMonitor = -5;
        if ( m_szPayload[ 0 ] == 0xff && m_szPayload[ 1 ] == 0xff )                         // this was the last packet
        {
            for ( int nAuditDevice = 0; nAuditDevice < m_nAuditDevices; nAuditDevice++ )
            {
                m_pProtelDevices[ nAuditDevice ]->GetConfiguration();
                m_pProtelDevices[ nAuditDevice ]->GetFirmware();
                indxOfFBMonitor = m_pProtelDevices[ nAuditDevice ]->GetFreeBee();
                if (indxOfFBMonitor >= 0)
                {                                                  // get index of freebee/audit in proteldevices array
                    m_pProtelDevices[ nAuditDevice ]->FreeBeeAuditDeviceidx = nAuditDevice;
                    indxOfFBMonitor = -1;                                                                      // reset
                }
            }

            /*
             * For each device, we check if there are any later ones that require an identical configuration. If so,
             * we mark them as having a duplicate configuration and add them to the list of devices to be configured
             * along with this one.
             */
            for ( int nAuditDevice = 0; nAuditDevice < m_nAuditDevices; nAuditDevice++ )
            {
                __int64 ConfigurationChecksum = m_pProtelDevices[ nAuditDevice ]->ConfigurationChecksum;
                for ( int nAuditDevice2 = ( nAuditDevice + 1 ); nAuditDevice2 < m_nAuditDevices; nAuditDevice2++ )
                {
                    if ( m_pProtelDevices[ nAuditDevice ]->ConfigurationLength == m_pProtelDevices[ nAuditDevice2 ]->ConfigurationLength ) // Were both nAuditDevice!!!!
                    {
                        if ( m_pProtelDevices[ nAuditDevice2 ]->ConfigurationChecksum == ConfigurationChecksum )
                        {
                            m_pProtelDevices[ nAuditDevice2 ]->ConfigurationDuplicate = true;
                            m_pProtelDevices[ nAuditDevice ]->AddConfigurationDuplicate( m_pProtelDevices[ nAuditDevice2 ]->Address );
                        }
                    }
                }
            }

            /*
             * We find devices with a non-zero firmware length.
             */
            bool firstDownload = true;
            for ( int nAuditDevice = 0; nAuditDevice < m_nAuditDevices; nAuditDevice++ )
            {

                if ( m_pProtelDevices[ nAuditDevice ]->FirmwareLength > 0 )
                {
                    if ( firstDownload == true )
                    {
                        /*
                         * This is the first such device. we check if there are any later ones that require identical
                         * firmware. If so, we mark them as having duplicate firmware and add them to the list of
                         * devices to receive the firmware along with this one.
                         */
                        firstDownload = false;
                        __int64 FirmwareCheckSum = m_pProtelDevices[ nAuditDevice ]->FirmwareChecksum;
                        for ( int nAuditDevice2 = ( nAuditDevice + 1 ); nAuditDevice2 < m_nAuditDevices; nAuditDevice2++ )
                        {
                            if ( m_pProtelDevices[ nAuditDevice ]->FirmwareLength == m_pProtelDevices[ nAuditDevice2 ]->FirmwareLength ) // Were both nAuditDevice!!!!
                            {
                                if ( m_pProtelDevices[ nAuditDevice2 ]->FirmwareChecksum == FirmwareCheckSum )
                                {
                                    m_pProtelDevices[ nAuditDevice2 ]->FirmwareDuplicate = true;
                                    m_pProtelDevices[ nAuditDevice ]->AddFirmwareDuplicate( m_pProtelDevices[ nAuditDevice2 ]->Address );
                                }
                            }
                        }
                    }
                    else
                    {
                        /*
                         * This is not the first device requiring firmware. Unless its firmware matches that of the
                         * first device, we mark not to send it during this connection (only one firmware download is
                         * allowed at a time).
                         */
                        if ( m_pProtelDevices[ nAuditDevice ]->FirmwareDuplicate == false )
                        {
                            m_pProtelDevices[ nAuditDevice ]->DontDownloadFirmware();
                        }
                    }
                }
            }

            if( RamFull == true || HaveDexFiles == true )
            {
                /*
                 * There is DEX data to upload (RamFull indicates this is essential). We send a U command to get the
                 * first record.
                 */
                ZeroMemory ( m_ActiveSerialNumber, sizeof ( m_ActiveSerialNumber ));
                MoveMemory ( m_ActiveSerialNumber, m_SerialNumber, lstrlen ( m_SerialNumber ));
                m_nDexFileRemoteAddress = 0;                                   // DEX data will be from the host itself
                Transmit_U_Command( 1 );
            }

            else
            {
//              Transmit_A_Command(true); // !!!! This was the original sequence
                Transmit_O_Command();
            }
        }
        else                                                                    // there is another packet - request it
        {
            int LastPacketNumber = ( m_szPayload[ 0 ] * 256 ) + m_szPayload[ 1 ];
            Transmit_N_Command( LastPacketNumber + 1 );
        }
    }

    bool Transmit_O_Command ( void )                                                                   // Send firmware
    {
        BYTE* pBuffer = NULL;
        long nLength = 0;
        while ( pBuffer == NULL )
        {
            /*
             * We see if there is a firmware chunk to send to the controlled device. This didn't work at all - it has
             * been extensively rewritten to fix and tidy it!!!!
             */
            pBuffer = m_pProtelDevices [ m_nCurrentAuditDevice ]->GetNextFirmware( nLength );
            if ( pBuffer == NULL && (Download2ndConfiguration == true || ++m_nCurrentAuditDevice >= m_nAuditDevices))
            {
                /*
                 * There isn't and either the download has been completed or there are no devices requiring firmware.
                 * We do a database update for any other devices that received the same firmware.
                 *
                 * NOTE: since we only send one firmware download per connection, we must explicitly update the
                 * database for any other devices that received the same firmware.
                 */
                for ( int nAuditDevice = 0; nAuditDevice < m_nAuditDevices; nAuditDevice++ )
                {
                    if ( m_pProtelDevices[ nAuditDevice ]->FirmwareDuplicate == true )
                    {
                        m_pProtelDevices[ nAuditDevice ]->UpdateDatabaseForFirmware();
                    }
                }

                /*
                 * We mark firmware download done and check for devices needing reconfiguration as a result.
                 */
				//m_EventTrace.Event( CEventTrace::Information, "void CProtelHost::1transmit_O_command [%d](%d)",1,1);
                Download2ndConfiguration = true;                                        // in case no firmware was sent
                for ( int nAuditDevice = 0; nAuditDevice < m_nAuditDevices; nAuditDevice++ )
                {
                    m_pProtelDevices[ nAuditDevice ]->GetSecondConfiguration();
                }
				//m_EventTrace.Event( CEventTrace::Information, "void CProtelHost::2transmit_O_command [%d](%d)",1,1);

                /*
                 * We start reconfiguration.
                 */
		        m_nCurrentAuditDevice = 0;	// wjs 2/24/2011 cfg dwnld problem..
                Transmit_C_Command();
				//m_EventTrace.Event( CEventTrace::Information, "void CProtelHost::transmit_O_command [%d](%d)",1,1);
                return true;
            }
        }

        /*
         * We have a firmware chunk to send. We do this after marking what we are doing.
         */
        Download2ndConfiguration = true;
        m_nReasonPinging = ReasonPinging::FirmwareFile;
        Transmit( 'O', pBuffer, nLength );
        return true;
    }

    void Process_O_Response ( void )
    {
        Transmit_O_Command();                                                                         // get next chunk
    }

    void Transmit_R_Command ( int Address )          // Request master to read DEX data from slave - not currently used
    {
        m_nReasonPinging = ReasonPinging::RequestedDEXRead;
        BYTE RemoteAddress = ( BYTE ) Address;
        Transmit( 'R', &RemoteAddress, sizeof ( RemoteAddress ));
    }

    void Process_R_Response ( void )
    {
        m_nDexFileRemoteAddress = m_szPayload [ 0 ];
        Transmit_Z_Command();
    }

    bool Transmit_S_Command ( void )         // Send date and time to the Master Auditor and request the 2 status bytes
    {
        RamFull = false;
        HaveDexFiles = false;
        BatteryMode = false;
        CallReason = 0x00;                                                                            // 00 = Undefined

        // The date and time will always be sent to the Auditor in one byte ASCII format.
        // The time will be military format running from 0001, being 1 minute after midnight,
        // to 2400, which is midnight.
        SYSTEMTIME utc;
        GetSystemTime ( &utc );

        BYTE payload [ 11 ];
        ZeroMemory ( payload, sizeof ( payload ));

        StringCchPrintf (( LPTSTR ) payload, sizeof ( payload ), "%02d%02d%02d%02d%02d", utc.wYear - 2000, utc.wMonth, utc.wDay, utc.wHour, utc.wMinute );
        switch ( utc.wDayOfWeek )
        {
            case 0:                                                                                       // Sunday = 0
                payload[10] = ProtelDayOfTheWeek::Sunday;
                break;
            case 1:                                                                                       // Monday = 1
                payload[10] = ProtelDayOfTheWeek::Monday;
                break;
            case 2:                                                                                      // Tuesday = 2
                payload[10] = ProtelDayOfTheWeek::Tuesday;
                break;
            case 3:                                                                                    // Wednesday = 3
                payload[10] = ProtelDayOfTheWeek::Wednesday;
                break;
            case 4:                                                                                     // Thursday = 4
                payload[10] = ProtelDayOfTheWeek::Thursday;
                break;
            case 5:                                                                                       // Friday = 5
                payload[10] = ProtelDayOfTheWeek::Friday;
                break;
            case 6:                                                                                     // Saturday = 6
                payload[10] = ProtelDayOfTheWeek::Saturday;
                break;
            default:
                throw "Invalid Date";
            }
        Transmit('S', payload, 11);
        return true;
    }

    void Process_S_Response ( void )
    {
        /*
         * We set class members such as HaveDexFiles according to the response, update the database, then send an
         * N(1) (first audit device data) command.
         */
        RamFull = false;
        if ( m_szPayload[ 0 ] & 0x01 )
        {
            RamFull = true;
        }

        HaveDexFiles = false;
        if ( m_szPayload[ 0 ] & 0x02 )
        {
            HaveDexFiles = true;
        }

        BatteryMode = false;
        if ( m_szPayload[ 0 ] & 0x04 )
        {
            BatteryMode = true;
        }

        CallReason = ( m_szPayload[ 1 ] & 0x07 );         // 0 - UHC handheld, 1 - pushbutton, 2 - alarm, 3 - scheduled
        Database_UpdateCallStatus();
        // Pass a '1' to initiate the conversation with the master auditor!
        Transmit_N_Command( 1 );
    }

    void Transmit_U_Command ( int PacketNumber )                                                     // Upload DEX data
    {
        BYTE PacketNumberBytes[ 2 ];
        PacketNumberBytes[ 0 ] = HIBYTE ( PacketNumber );                          //(( PacketNumber & 0xff00 ) >> 8 );
        PacketNumberBytes[ 1 ] = LOBYTE ( PacketNumber );                                    //( PacketNumber & 0xff );
        Transmit( 'U', PacketNumberBytes, sizeof ( PacketNumberBytes ));
    }

    void Process_U_Response ( int nPayloadLength )
    {
        /*
         * We save the received DEX record in the database. If this was the last record (ffff), we send the D
         * command, otherwise we request the next record.
         */
        int LastPacketNumber = ( m_szPayload[ 0 ] * 256 ) + m_szPayload[ 1 ];
        Database_DexData ( m_ActiveSerialNumber, CallNumber, LastPacketNumber, m_szPayload + 2, nPayloadLength - 2 );
        if ( LastPacketNumber == 0xffff )
        {
            Transmit_D_Command();                                                     // done - dump records in auditor

        }
        else
        {
            Transmit_U_Command( LastPacketNumber + 1 );                                          // request next record
        }
    }

    bool Transmit_V_Command ( void )                                     // Free Vend Configuration - trace through!!!!
    {
        BYTE disableFreeBee = 0xff;                                                          // sent to disable freebee
        BYTE szPayload [ 11 ];                                         // loop until audit device with freebee is found
        int FreeBeeAuditDeviceIndx;
        int lenOfPayload = 10;                                          // "T.V0xff00000000cksum" or "T.V"AdrIdXXXNNNNN
        BYTE* m_temAuditDevice;
        ZeroMemory ( szPayload, sizeof ( szPayload ));

        while ( m_nCurrentAuditDevice <= m_nAuditDevices )                           /// changed < to <= wjs 10/07/2010
        {
            // added by WJS as there was no functioning freebee config download code
            FreeBeeAuditDeviceIndx = m_pProtelDevices [ m_nCurrentAuditDevice ]->FreeBeeAuditDeviceidx;
            if (FreeBeeAuditDeviceIndx >= 0)
            {                                                                                                  // found
                m_temAuditDevice =  m_pProtelDevices [ m_nCurrentAuditDevice ]->AuditDevice;
            }else
            {
                m_nCurrentAuditDevice++;
                continue;                                                                      // get next audit device
            }

            // check if a freebeedownload or a disable frebee is to be sent.
            // freebee controllerflag set to the address of freebee controller if freebee download
            FreeBeeAuditDeviceIndx = m_pProtelDevices [ m_nCurrentAuditDevice ]->FreeBeeControllerflag;
            if (FreeBeeAuditDeviceIndx < 0)                                                     // send freebee disable
            {
                szPayload [ 0 ] = ( BYTE ) disableFreeBee;                   // wjs 10/11/2010 get audit device address
            }else                                                             // send freebee download (enable freebee)
            {
                // now get address of audit device
                AuditDevice m_TemAuditDevice;
                memcpy(&m_TemAuditDevice, m_temAuditDevice, sizeof (AuditDevice));

                szPayload [ 0 ] = ( BYTE ) m_TemAuditDevice.Address;         // wjs 10/11/2010 get audit device address
                szPayload [ 1 ] = ( BYTE ) m_pProtelDevices [ m_nCurrentAuditDevice ]->FreeBeeController - 1;
                if (strlen (m_pProtelDevices [ m_nCurrentAuditDevice ]->FreeBeeSerialNumber) > 1)
                    StringCbCat (( LPTSTR ) szPayload + 2, 9, m_pProtelDevices [ m_nCurrentAuditDevice ]->FreeBeeSerialNumber );
            }
            // now transmit data
            Transmit( 'V', szPayload, lenOfPayload);
			return true;
        }
        m_nCurrentAuditDevice = 0;
		m_NormalShutdown = true;
        //m_EventTrace.Event( CEventTrace::Information, "void CProtelHost::Transmit_V_Command [%d](%d)",1,1);
        Transmit_A_Command( true );
        return true;
    }

    void Process_V_Response ( void )
    {
		/* altered this function to check if a firmware download has taken place then download config (do not
		/* download if firmware download pending). Download config always after firmware download
		/* and never before a pending firmware download.
		/*	WJS */
  //      m_nCurrentAuditDevice = 0;
  //          m_EventTrace.Event( CEventTrace::Information, "void CProtelHost::Transmit_V_Command [%d](%d)",1,1);
		//	if (((m_pProtelDevices [ m_nCurrentAuditDevice ]->GetNeedsFirmware() == true) && \
		//		(m_pProtelDevices [ m_nCurrentAuditDevice ]->GetHasDownloadFirmware() == true)) || \
		//		(m_pProtelDevices [ m_nCurrentAuditDevice ]->GetNeedsFirmware() == false))
		//{
		//	Transmit_C_Command();
		//}
		//else
		//{
            m_nCurrentAuditDevice = 0;                // they haven't - check first device for new firmware
            Transmit_A_Command( true );
		//}
    }

    void Transmit_Z_Command ( void )                                                                            // Ping
    {
        /*
         * At present, this is only used once a configuration has been completely sent or when a response to the
         * (presently unused) R command is received.
         */
        Transmit( 'Z', NULL, 0 );
    }

    void Process_Z_Response ( int nPayloadLength )
    {
        // Response from Master auditor-
        // The response from the Master auditor includes a code to let the Host know exactly
        // what the Master auditor is doing. If the Master auditor is not working on an
        // assigned task, but just waiting for the Host to send a command, the code within
        // the response to the ping command would be 00h or idle.
        // Responses:
        //  0x00 = Idle, still processing
        //  0x01 = Task in progress
        //  0x02 = Task complete, request to report results

        if ( m_szPayload [ 0 ] == 0x01 )                                                            // Task in progress
        {
            SleepEx(1000,TRUE);                      // Pause for 1 second before continuing -- don't overwhelm remote!
            Transmit_Z_Command();
            return;
        }

        if ( m_szPayload [ 0 ] == 0x02 )                                                              // Task completed
        {
            if ( nPayloadLength > 2 )
            {
                /*
                 * Some slaves weren't successfully updated. The following code appears to be wrong, pointless and
                 * dangerous.
                 */
                int nFailureCount = ( int ) m_szPayload [ 1 ];
                int nOffset = -1;
                for ( int nLoop = 0; nLoop < nFailureCount; nLoop++ )
                {
                    nOffset = m_szPayload [ nLoop + 1 ]; // this starts off by getting the FailureCount again. Should get an address!!!!
//                    m_pProtelDevices [ nOffset ]->SerialNumber; // this gets the serial number of the wrong device, does nothing with it, could cause a crash!!!!
                }
            }
        }

        switch ( m_nReasonPinging )
        {
            case ReasonPinging::ConfigurationFile:
                m_nReasonPinging = ReasonPinging::NotPinging;
            //m_EventTrace.Event( CEventTrace::Information, "void CProtelHost::Process_Z_Response [%d](%d)",1,1);
                Transmit_C_Command();                                                // Finish up and write to database
                break;
            case ReasonPinging::FirmwareFile:
                m_nReasonPinging = ReasonPinging::NotPinging;
                Transmit_O_Command();
                break;
            case ReasonPinging::RequestedDEXRead:
                m_nReasonPinging = ReasonPinging::NotPinging;
                Transmit_U_Command( 1 );                                             // Got device's DEX, start reading
                break;
        }
    }

    bool ContinueComms ( bool r )
    {
        /**************************************************************************************************************
         * This is called with r false when a communication error (timeout) occurs. Provided there haven't been too   *
         * many such errors, it returns true and a retransmit can occur. Once the error threshold is reached, it      *
         * returns false and the connection should be aborted.                                                        *
         *                                                                                                            *
         * It is called with r true to adjust the error count when a command response is successfully received.       *
         **************************************************************************************************************/

        if ( r == true )
        {
            if ( m_nCommsErrs >= 1 )
            {
                m_nCommsErrs -= 1;                                        // response received and error count non-zero
            }
            return true;
        }

        m_nCommsErrs += 3;                                                                           // error (timeout)
        return ( m_nCommsErrs < CommsErrsLimit( ) );                                         // true if too many errors
    }

    void Retransmit ( void )
    {
        /**************************************************************************************************************
         * This is used if a valid response to the last command transmitted is not received before timeout to         *
         * retransmit the command.                                                                                    *
         **************************************************************************************************************/
        if ( m_nLastTransmission > 0 )
        {
			if (m_nLastCmd != m_LastTransmission[2])
			{
				m_nReXmitFailCountPercmd = 1;
				m_nReXmitFailCountPercall++;
				m_nLastCmd = m_LastTransmission[2];
            //m_EventTrace.Event( CEventTrace::Information, "void CProtelHost::Retransmit 1 percmd [%d] percall(%d)",m_nReXmitFailCountPercmd,m_nReXmitFailCountPercall);
			}else if (m_nLastCmd == m_LastTransmission[2])
						
			{
				if ((m_nReXmitFailCountPercmd < MAXFAILCOUNTPERCMD - 1) && (m_nReXmitFailCountPercall < MAXFAILCOUNTPERCALL - 1))
				{
				m_nReXmitFailCountPercmd++;
				m_nReXmitFailCountPercall++;
            //m_EventTrace.Event( CEventTrace::Information, "void CProtelHost::Retransmit 12 percmd [%d] percall(%d)",m_nReXmitFailCountPercmd,m_nReXmitFailCountPercall);
				} else // one has failed 
				{
				Transmit_A_Command ( false );
				m_nReXmitFailCountPercmd = 0;
				m_nReXmitFailCountPercall = 0;
				m_nLastCmd = (BYTE)"";
            //m_EventTrace.Event( CEventTrace::Information, "void CProtelHost::Retransmit 5 percmd [%d] percall(%d)",m_nReXmitFailCountPercmd,m_nReXmitFailCountPercall);
				return;

				}
			}


            Database_CommunicationsData ( true, true, m_LastTransmission, m_nLastTransmission );

            ResetReceivedBuffer();                                                          // prepare for new response

            Send ( m_LastTransmission, m_nLastTransmission );                                             // retransmit
            //m_EventTrace.Event( CEventTrace::Information, "void CProtelHost::Retransmit 6 lastcmd [%c] percall(%d)",m_nLastCmd,m_nReXmitFailCountPercall);

            //SetTimeoutTimer( m_hTimer, __WAIT_TIME__ );
            SetTimeoutTimer( m_hTimer, GetWaitSeconds());
        }
    }

    void Transmit ( char command, BYTE* Payload, int PayloadLength)
    {
        /**************************************************************************************************************
         * This assembles the specified command (e.g. command == 'I') including the sync character (T), length,       *
         * payload and checksum, " uses the overridden Send method to send it to the remote master, then sets the     *
         * response timeout. The payload of PayloadLength bytes is as specified in Payload.                           *
         *                                                                                                            *
         * The assembled command is saved in m_LastTransmission in case it needs to be retransmitted (preceding       *
         * method).                                                                                                   *
         **************************************************************************************************************/
        m_szCurrentCommand [ 0 ] = command;
        m_szCurrentCommand [ 1 ] = '\0';
        ResetReceivedBuffer();                                                              // prepare for new response
        ZeroMemory ( m_transmitBuffer, sizeof ( m_transmitBuffer ));

        if (Payload != NULL && PayloadLength > 0)
        {
            MoveMemory ( m_transmitBuffer + 3, Payload, PayloadLength );
        }

        m_transmitBuffer[0] = (BYTE)'T';                                                              // sync character
        m_transmitBuffer[1] = (BYTE)PayloadLength + 1;
        m_transmitBuffer[2] = (BYTE)command;

        BYTE Checksum = CalculateChecksum ( m_transmitBuffer, PayloadLength + 4 );
        m_transmitBuffer [ PayloadLength + 3 ] = Checksum;


        //Database_Dialog ( true, m_transmitBuffer, PayloadLength + 4 );                      // (currently does nothing)
        m_nLastTransmission = PayloadLength + 4;
        ZeroMemory ( m_LastTransmission, sizeof ( m_LastTransmission ));
        CopyMemory ( m_LastTransmission, m_transmitBuffer, m_nLastTransmission );                     // for Retransmit
        Database_CommunicationsData ( true, false, m_LastTransmission, m_nLastTransmission ); // record command in database

        //m_EventTrace.Event( CEventTrace::Details, "void CProtelHost::Transmit ( '%c', BYTE*, %d )", command, PayloadLength );
        m_EventTrace.HexDump( CEventTrace::Information, m_transmitBuffer, PayloadLength + 4 );
        Send ( m_transmitBuffer, PayloadLength + 4 );

        //SetTimeoutTimer( m_hTimer, __WAIT_TIME__ );
        SetTimeoutTimer( m_hTimer, GetWaitSeconds());
    }

    virtual void CloseDevice ( int typeclose )
    {
        /**************************************************************************************************************
         * This is overridden by ProtelSerial and ProtelSocket but is called by the overriding method.                *
		 * typeclose values:
		 * 0 => failed call
		 * 1 => sucessfull call
		 * 2 => end call with no database call
         **************************************************************************************************************/
        CancelTimer( m_hTimer );
		switch ( typeclose )
		{
		case 0 : 
			Database_FinishCall( false );
			break;
		case 1 :
			Database_FinishCall( true );
			break;
		default :
			break;
		}
//        Database_FinishCall();
        if ( m_padoConnection != NULL )
        {
            delete m_padoConnection;
            m_padoConnection = NULL;
        }
    }

    bool Database_AddNewCall ( void )
    {
        /**************************************************************************************************************
         * This is used by ProtelSerial and ProtelSocket when a new connection to a master auditor begins. The called *
         * procedure creates a new row in the COMM_SERVER_CALL table, obtaining the CALLNUMBER and setting the        *
         * CALLSTARTTIME, COMM_DEVICE_DESC and COMM_DEVICE_PORT.                                                      *
         **************************************************************************************************************/
        bool bReturn = false;

        if ( m_padoConnection == NULL )
        {
            return bReturn;                                                // the connection to the database isn't open
        }

        try
        {
            CAdoStoredProcedure adoStoredProcedure ( "PKG_COMM_SERVER.ADDNEW" );

            //PROCEDURE ADDNEW (
            //    po_call_number               out integer
            //    , pi_CALL_START_TIME   IN       TIMESTAMP default null
            //    , pi_device_type in integer
            //    , pi_device_desc in varchar2
            //    , pi_port in varchar2
            //    , po_central_auditor in varchar2 default null
            //
            //);
            _variant_t vtCallNumber (( long ) 0, VT_I4 );
            adoStoredProcedure.AddParameter("po_call_number", vtCallNumber, ADODB::DataTypeEnum::adInteger, ADODB::ParameterDirectionEnum::adParamOutput, sizeof(long));

            _variant_t vtCallStartTime ( dCallStartTime, VT_DATE );     // Note: time was set by ProtelHost::Initialize
            adoStoredProcedure.AddParameter( "pi_CALL_START_TIME", vtCallStartTime, ADODB::DataTypeEnum::adDate, ADODB::ParameterDirectionEnum::adParamInput, sizeof ( double ));

            int DeviceType = 0;
            if ( lstrcmpi ( GetDevice(), "TCP/IP" ) == 0 )
            {
                DeviceType = 1;                                                                               // TCP/IP
            }
            else
            {
                DeviceType = 2;                                                                                // Modem
            }

            _variant_t vtDeviceType (( long ) DeviceType, VT_I4 ); // Was setting pi_device_type to vtCallNumber (always 0)!!!!
            adoStoredProcedure.AddParameter("pi_device_type", vtDeviceType, ADODB::DataTypeEnum::adInteger, ADODB::ParameterDirectionEnum::adParamInput, sizeof(long));

            _bstr_t bstrDevice( GetDevice());
            _variant_t vtDevice ( bstrDevice );
            adoStoredProcedure.AddParameter( "pi_device_desc", vtDevice, ADODB::DataTypeEnum::adBSTR, ADODB::ParameterDirectionEnum::adParamInput, bstrDevice.length());

            _bstr_t bstrPort( GetPort());
            _variant_t vtPort ( bstrPort );
            adoStoredProcedure.AddParameter( "pi_port", vtPort, ADODB::DataTypeEnum::adBSTR, ADODB::ParameterDirectionEnum::adParamInput, bstrPort.length());



            m_padoConnection->ExecuteNonQuery( adoStoredProcedure, false );                                   // do it!
            //m_EventTrace.Event ( CEventTrace::Details, "CProtelHost::Database_AddNewCall <--g_pAdoConnection->ExecuteNonQuery" );

            _variant_t vtReturnedCallNumber = adoStoredProcedure.GetParameter("po_call_number");
            CallNumber = ( long ) vtReturnedCallNumber;
            char szCallNumber [ 64 ];
            _itoa_s( CallNumber, szCallNumber, sizeof ( szCallNumber ), 10 );
			m_EventTrace.Event ( CEventTrace::Details, "CProtelHost::Database_AddNewCall -->g_pAdoConnection->ExecuteNonQuery : id %d", CallNumber);
            m_EventTrace.Identifier( szCallNumber, GetDevice(), GetPort());
			if (CallNumber <= 0 )
			{
				bReturn =  false;
			}
			else
			{
	            bReturn = true;
			}
        }
        catch ( _com_error &comError )
        {
            m_EventTrace.Event ( CEventTrace::Information, "CProtelHost::Database_AddNewCall <--> ERROR: %s", CErrorMessage::ReturnComErrorMessage ( comError ));
        }

        return bReturn;
    }

    void Database_FinishCall ( bool success )
    {
        /**************************************************************************************************************
         * This is used by CloseDevice above when a connection to a master auditor ends. The called procedure records *
         * the current time and success flag in the current call's record in the COMM_SERVER_CALL database table      *
         * record. This also cleans some things up.                                                                   *
         **************************************************************************************************************/
        if ( CallNumber == 0 && dCallStartTime == ( double ) 0 )
        {
            return;
        }

        SYSTEMTIME CallStopTime;
        GetSystemTime ( &CallStopTime );
        double dCallStopTime;
        SystemTimeToVariantTime ( &CallStopTime, &dCallStopTime );
        try
        {
            m_nCommsErrs = 0;
            //CancelWaitableTimer( m_hTimer );
            CancelTimer( m_hTimer );
            //m_EventTrace.Event( CEventTrace::Details, "CAdoStoredProcedure adoStoredProcedure ( PKG_COMM_SERVER.FINISH );" );
            CAdoStoredProcedure adoStoredProcedure ( "PKG_COMM_SERVER.FINISH" );
            //procedure finish (
            //    pi_callnumber in integer
            //    , pi_centralAuditor in varchar2 default null
            //    , pi_callstarttime in timestamp default null
            //    , pi_callstoptime in timestamp
            //    , pi_success in integer
            //    , pi_errormsg in varchar2 default null
            //);

            _variant_t vtCallNumber (( long ) CallNumber, VT_I4 );
            adoStoredProcedure.AddParameter( "pi_callnumber", vtCallNumber, ADODB::DataTypeEnum::adInteger, ADODB::ParameterDirectionEnum::adParamInput, sizeof ( long ));

            _bstr_t bstrCentralAuditor( m_SerialNumber );
            _variant_t vtCentralAuditor ( bstrCentralAuditor );
            adoStoredProcedure.AddParameter( "pi_centralAuditor", vtCentralAuditor, ADODB::DataTypeEnum::adBSTR, ADODB::ParameterDirectionEnum::adParamInput, bstrCentralAuditor.length());

            _variant_t vtCallStartTime ( dCallStartTime, VT_DATE );
            adoStoredProcedure.AddParameter( "pi_callstarttime", vtCallStartTime, ADODB::DataTypeEnum::adDate, ADODB::ParameterDirectionEnum::adParamInput, sizeof ( double ));

            _variant_t vtCallStopTime ( dCallStopTime, VT_DATE );
            adoStoredProcedure.AddParameter( "pi_callstoptime", vtCallStopTime, ADODB::DataTypeEnum::adDate, ADODB::ParameterDirectionEnum::adParamInput, sizeof ( double ));

            short nNormalShutdown = 0;
            if ( success == true )
            {
                nNormalShutdown = 1;
            }
            _variant_t vtSuccess (( short ) nNormalShutdown, VT_I2 );
            adoStoredProcedure.AddParameter( "pi_success", vtSuccess, ADODB::DataTypeEnum::adInteger, ADODB::ParameterDirectionEnum::adParamInput, sizeof ( short ));

            _bstr_t bstrErrorMessage( "" );
            _variant_t vtErrorMessage ( bstrErrorMessage );
            adoStoredProcedure.AddParameter( "pi_errormsg", vtErrorMessage, ADODB::DataTypeEnum::adBSTR, ADODB::ParameterDirectionEnum::adParamInput, bstrErrorMessage.length());

			//m_EventTrace.Event ( CEventTrace::Details, "CProtelHost::Database_FinishCall -->g_pAdoConnection->ExecuteNonQuery : id %d", CallNumber );
            m_padoConnection->ExecuteNonQuery( adoStoredProcedure, false );
            //m_EventTrace.Event ( CEventTrace::Details, "CProtelHost::Database_FinishCall <--g_pAdoConnection->ExecuteNonQuery" );
        }
        catch ( _com_error &comError )
        {
            m_EventTrace.Event ( CEventTrace::Information, "CProtelHost::Database_FinishCall <--> ERROR: %s", CErrorMessage::ReturnComErrorMessage ( comError ));
        }
        ZeroMemory ( m_SerialNumber, sizeof ( m_SerialNumber ));
        ZeroMemory ( m_CellModemSimmID, sizeof ( m_CellModemSimmID ));
        ZeroMemory ( m_CardReaderID, sizeof ( m_CardReaderID ));
        ZeroMemory ( m_CardReaderRevision, sizeof ( m_CardReaderRevision ));
        ZeroMemory ( m_CardReaderFirmwareVersion, sizeof ( m_CardReaderFirmwareVersion ));
        ZeroMemory ( m_CardReaderConfigVersion, sizeof ( m_CardReaderConfigVersion ));
        ZeroMemory ( m_szCurrentCommand, sizeof ( m_szCurrentCommand ));

        RamFull = false;
        HaveDexFiles = false;
        BatteryMode = false;

        CallReason = 0;
        CallNumber = 0;
        dCallStartTime = ( double ) 0;
    }


    void Database_DexData ( char* pszSerialNumber, int nCallNumber, int nSequence, BYTE* pPayload, int nPayloadLength )
    {
        /**************************************************************************************************************
         * This is used to save DEX data received in a U command response as a new record in the COMM_SERVER_DEX      *
         * database table. It also cleans some things up.                                                             *
         **************************************************************************************************************/
        try
        {
            //CAdoStoredProcedure adoStoredProcedure ( "PKG_COMM_SERVER.DEX" );
// begin dex2 code
            CAdoStoredProcedure adoStoredProcedure ( "PKG_COMM_SERVER.DEX2" );
// end this dex2 code

            //pi_callnumber            in    number,
            //pi_call_start_time   in timestamp,
            //pi_centralauditor    in varchar2 default null, --added by ESH
            //pi_serial_number     in varchar2 default null,
            //pi_sequence          in integer,
            //pi_dex_data          in blob,
            //pi_lastdexrecord     in smallint,
			//po_err_code		   out integer, - = 0 data saved, -n ora error
			//po_err_txt		   out varchar(200)); ora error verbage

            _variant_t vtCallNumber (( long ) CallNumber, VT_I4 );
            adoStoredProcedure.AddParameter( "pi_CALLNUMBER", vtCallNumber, ADODB::DataTypeEnum::adInteger, ADODB::ParameterDirectionEnum::adParamInput, sizeof ( long ));

            _variant_t vtCallStartTime ( dCallStartTime, VT_DATE );
            adoStoredProcedure.AddParameter( "pi_call_start_time", vtCallStartTime, ADODB::DataTypeEnum::adDate, ADODB::ParameterDirectionEnum::adParamInput, sizeof ( double ));

            _bstr_t bstrCentralAuditor( m_SerialNumber );
            _variant_t vtCentralAuditor ( bstrCentralAuditor );
            adoStoredProcedure.AddParameter( "pi_centralauditor", vtCentralAuditor, ADODB::DataTypeEnum::adBSTR, ADODB::ParameterDirectionEnum::adParamInput, bstrCentralAuditor.length());

            _bstr_t bstrSerialNumber( pszSerialNumber );
            _variant_t vtSerialNumber ( bstrSerialNumber );
            adoStoredProcedure.AddParameter( "pi_serial_number", vtSerialNumber, ADODB::DataTypeEnum::adBSTR, ADODB::ParameterDirectionEnum::adParamInput, bstrSerialNumber.length());

            _variant_t vtSequence (( long ) nSequence, VT_I4 );
            adoStoredProcedure.AddParameter( "pi_sequence", vtSequence, ADODB::DataTypeEnum::adInteger, ADODB::ParameterDirectionEnum::adParamInput, sizeof ( short ));

            variantBlob vtPayload ( pPayload, nPayloadLength );
            adoStoredProcedure.AddParameter( "pi_dex_data", vtPayload, ADODB::DataTypeEnum::adVarBinary, ADODB::ParameterDirectionEnum::adParamInput, nPayloadLength );

// code for PKG_COMM_SERVER.DEX2 for bug
			_variant_t vterrorcode (( long ) 0, VT_I4 );
			adoStoredProcedure.AddParameter( "po_err_code", vterrorcode, ADODB::adInteger, ADODB::adParamOutput, sizeof ( long ));

			_variant_t vtErrorText ( _bstr_t ( "" ));
			adoStoredProcedure.AddParameter("po_err_txt", vtErrorText, ADODB::adBSTR, ADODB::adParamOutput, 200 );
// end code for PKG_COMM_SERVER.DEX2

			char chLastDexRecord = 0;
            if ( nSequence == 0xffff )
            {
                chLastDexRecord = 1;
            }
            _variant_t vtLastDexRecord ( chLastDexRecord );
            adoStoredProcedure.AddParameter( "pi_lastdexrecord", vtLastDexRecord, ADODB::DataTypeEnum::adTinyInt, ADODB::ParameterDirectionEnum::adParamInput, sizeof ( chLastDexRecord ));

            //m_EventTrace.Event ( CEventTrace::Details, "CProtelHost::Database_DexData -->g_pAdoConnection->ExecuteNonQuery" );
			bool checkit = true; //execute failure
            checkit = m_padoConnection->ExecuteNonQuery( adoStoredProcedure, false );                                   // do it!
			if (checkit == false)
				CloseDevice(0);	// 0 => send failed call to the database
// begin code for PKG_COMM_SERVER.DEX2
            _variant_t vtReturnedOraError = adoStoredProcedure.GetParameter("po_err_code");
            long OraErrorNumber = ( long ) vtReturnedOraError;
			if (OraErrorNumber != 0)
			{
				_variant_t vtReturnedOraErrorText = adoStoredProcedure.GetParameter("po_err_txt");
				m_EventTrace.Event ( CEventTrace::Details, "CProtelHost::Database_DexData ExecuteNonQuery" + vtReturnedOraErrorText);
				CloseDevice(0);
			}
// end code for PKG_COMM_SERVER.DEX2
            //m_EventTrace.Event ( CEventTrace::Details, "CProtelHost::Database_DexData <--g_pAdoConnection->ExecuteNonQuery" );
        }
        catch ( _com_error &comError )
        {
            m_EventTrace.Event ( CEventTrace::Information, "CProtelHost::Database_DexData <--> ERROR: %s", CErrorMessage::ReturnComErrorMessage ( comError ));
			CloseDevice(0);	// 0 => send failed call to the database
        }
    }

    void Database_UpdateCallStatus ( void )
    {
        /**************************************************************************************************************
         * This is called from Process_S_Response. The called procedure updates the existing row in the               *
         * COMM_SERVER_CALL table where CALLNUMBER matches                                                            *
         **************************************************************************************************************/
        try
        {
            CAdoStoredProcedure adoStoredProcedure ( "PKG_COMM_SERVER.STATUS" );
            //PROCEDURE STATUS (
            //    pi_callnumber in number
            //    , pi_CENTRALAUDITOR     in varchar2 default null,
            //    pi_callstarttime in timestamp  default null,
            //    pi_ramfull        in smallint,
            //    pi_havedexfiles   in smallint,
            //    pi_batterymode    in smallint,
            //    pi_callreason     in smallint
            //);
            _variant_t vtCallNumber (( long ) CallNumber, VT_I4 );
            adoStoredProcedure.AddParameter( "pi_callnumber", vtCallNumber, ADODB::DataTypeEnum::adInteger, ADODB::ParameterDirectionEnum::adParamInput, sizeof ( long ));

            _bstr_t bstrCentralAuditor( m_SerialNumber );
            _variant_t vtCentralAuditor ( bstrCentralAuditor );
            adoStoredProcedure.AddParameter( "pi_CENTRALAUDITOR", vtCentralAuditor, ADODB::DataTypeEnum::adBSTR, ADODB::ParameterDirectionEnum::adParamInput, bstrCentralAuditor.length());

            _variant_t vtCallStartTime ( dCallStartTime, VT_DATE );
            adoStoredProcedure.AddParameter( "pi_callstarttime", vtCallStartTime, ADODB::DataTypeEnum::adDate, ADODB::ParameterDirectionEnum::adParamInput, sizeof ( double ));

            _variant_t vtRamFull (( short ) RamFull, VT_I2 );
            adoStoredProcedure.AddParameter( "pi_ramfull", vtRamFull, ADODB::DataTypeEnum::adInteger, ADODB::ParameterDirectionEnum::adParamInput, sizeof ( short ));

            _variant_t vtHaveDexFiles (( short ) HaveDexFiles, VT_I2 );
            adoStoredProcedure.AddParameter( "pi_havedexfiles", vtHaveDexFiles, ADODB::DataTypeEnum::adInteger, ADODB::ParameterDirectionEnum::adParamInput, sizeof ( short ));

            _variant_t vtBatteryMode (( short ) BatteryMode, VT_I2 );
            adoStoredProcedure.AddParameter( "pi_batterymode", vtBatteryMode, ADODB::DataTypeEnum::adInteger, ADODB::ParameterDirectionEnum::adParamInput, sizeof ( short ));
            _variant_t vtCallReason (( short ) CallReason, VT_I2 );
            adoStoredProcedure.AddParameter( "pi_callreason", vtCallReason, ADODB::DataTypeEnum::adInteger, ADODB::ParameterDirectionEnum::adParamInput, sizeof ( short ));

            //m_EventTrace.Event ( CEventTrace::Details, "CProtelHost::Database_UpdateCallStatus -->g_pAdoConnection->ExecuteNonQuery" );
            m_padoConnection->ExecuteNonQuery( adoStoredProcedure, false );
            //m_EventTrace.Event ( CEventTrace::Details, "CProtelHost::Database_UpdateCallStatus <--g_pAdoConnection->ExecuteNonQuery" );
        }
        catch ( _com_error &comError )
        {
            m_EventTrace.Event ( CEventTrace::Information, "CProtelHost::Database_UpdateCallStatus <--> ERROR: %s", CErrorMessage::ReturnComErrorMessage ( comError ));
        }
    }

    bool Database_Dialog ( bool Transmit, BYTE* Transmission, int TransmissionLength )
    {
        /**************************************************************************************************************
         * This appears to be intended to record the TransmissionLength bytes in Transmission that have been sent     *
         * (Transmit true) or received in the COMM_SERVER_LOG database table. The code in the database procedure that *
         * does this may be disabled.                                                                                 *
         **************************************************************************************************************/
        try
        {
            char chTransmit = 0;
            if ( Transmit == true )
            {
                chTransmit = 1;
            }

            CAdoStoredProcedure adoStoredProcedure ( "PKG_COMM_SERVER.DIALOG" );
            //PROCEDURE DIALOG (
            //  pi_CALL_NUMBER         IN   INTEGER,
            //  pi_CALL_START_TIME     IN   TIMESTAMP default null,
            //  pi_centralauditor    in varchar2 default null,
            //  pi_TOHOST              IN   SMALLINT,
            //  pi_COMMAND             IN   VARCHAR2,
            //  pi_LENGTH              IN   SMALLINT,
            //  pi_TRANSMISSION_DATA   IN   BLOB,
            //  pi_PAYLOAD             IN   BLOB DEFAULT NULL,
            //  pi_PAYLOAD_HEX         IN   VARCHAR2 DEFAULT NULL,
            //  pi_PAYLOAD_ASCII       IN   VARCHAR2 DEFAULT NULL
            //);

            _variant_t vtCallNumber (( long ) CallNumber, VT_I4 );
            adoStoredProcedure.AddParameter( "pi_CALL_NUMBER", vtCallNumber, ADODB::DataTypeEnum::adInteger, ADODB::ParameterDirectionEnum::adParamInput, sizeof ( long ));

            _variant_t vtCallStartTime ( dCallStartTime, VT_DATE );
            adoStoredProcedure.AddParameter( "pi_CALL_START_TIME", vtCallStartTime, ADODB::DataTypeEnum::adDate, ADODB::ParameterDirectionEnum::adParamInput, sizeof ( double ));

            _bstr_t bstrCentralAuditor( m_SerialNumber );
            _variant_t vtCentralAuditor ( bstrCentralAuditor );
            adoStoredProcedure.AddParameter( "pi_centralauditor", vtCentralAuditor, ADODB::DataTypeEnum::adBSTR, ADODB::ParameterDirectionEnum::adParamInput, bstrCentralAuditor.length());

            _variant_t vtToHost ( chTransmit );
            adoStoredProcedure.AddParameter( "pi_TOHOST", vtToHost, ADODB::DataTypeEnum::adTinyInt, ADODB::ParameterDirectionEnum::adParamInput, sizeof ( chTransmit ));

            char szCommand [ 2 ];
            szCommand [ 0 ] = *( Transmission + 2 );
            szCommand [ 1 ] = '\0';
            _bstr_t bstrCommand( szCommand );
            _variant_t vtCommand ( bstrCommand );
            adoStoredProcedure.AddParameter( "pi_COMMAND", vtCommand, ADODB::DataTypeEnum::adBSTR, ADODB::ParameterDirectionEnum::adParamInput, bstrCommand.length());

            short PayloadLength = TransmissionLength-4;
            if ( PayloadLength < 1 )
            {
                PayloadLength = 0;
            }
            _variant_t vtTransmissionLength (( short ) PayloadLength );
            adoStoredProcedure.AddParameter( "pi_LENGTH", vtTransmissionLength, ADODB::DataTypeEnum::adSmallInt, ADODB::ParameterDirectionEnum::adParamInput, sizeof ( short ));

            variantBlob vtTransmissionData ( Transmission, TransmissionLength );
            adoStoredProcedure.AddParameter( "pi_TRANSMISSION_DATA", vtTransmissionData, ADODB::DataTypeEnum::adVarBinary, ADODB::ParameterDirectionEnum::adParamInput, TransmissionLength );

            if ( PayloadLength > 0 )
            {
                variantBlob vtPayload ( Transmission+3, PayloadLength );
                adoStoredProcedure.AddParameter( "pi_PAYLOAD", vtPayload, ADODB::DataTypeEnum::adVarBinary, ADODB::ParameterDirectionEnum::adParamInput, PayloadLength );

                _bstr_t bstrPayloadHex( CHexDump::GetHexString( Transmission+3, PayloadLength ));
                _variant_t vtPayloadHex ( bstrPayloadHex );
                adoStoredProcedure.AddParameter( "pi_PAYLOAD_HEX", vtPayloadHex, ADODB::DataTypeEnum::adBSTR, ADODB::ParameterDirectionEnum::adParamInput, bstrPayloadHex.length());

                _bstr_t bstrPayloadAscii( CHexDump::GetAsciiString( Transmission+3, PayloadLength ));
                _variant_t vtPayloadAscii ( bstrPayloadAscii );
                adoStoredProcedure.AddParameter( "pi_PAYLOAD_ASCII", vtPayloadAscii, ADODB::DataTypeEnum::adBSTR, ADODB::ParameterDirectionEnum::adParamInput, bstrPayloadAscii.length());
            }

            //m_EventTrace.Event ( CEventTrace::Details, "CProtelHost::Database_Dialog -->g_pAdoConnection->ExecuteNonQuery" );
            m_padoConnection->ExecuteNonQuery( adoStoredProcedure, false );                                    // do it
            //m_EventTrace.Event ( CEventTrace::Details, "CProtelHost::Database_Dialog <--g_pAdoConnection->ExecuteNonQuery" );
            return true;
        }
        catch ( _com_error &comError )
        {
            m_EventTrace.Event ( CEventTrace::Information, "CProtelHost::Database_CommunicationsData :--: ERROR: %s", CErrorMessage::ReturnComErrorMessage ( comError ));
        }
        return false;
    }

    bool Database_CommunicationsData ( bool Transmit, bool Retransmit, BYTE* Transmission, int TransmissionLength )
    {
        /**************************************************************************************************************
         * This records the TransmissionLength bytes in Transmission that have been sent (Transmit true) or received  *
         * in the COMM_SERVER_DETAILS and COMM_SERVER_LOG database tables. Retransmit indicates if the command was    *
         * retransmitted.                                                                                             *
         **************************************************************************************************************/
        try
        {
            char chTransmit = 0;
            if ( Transmit == true )
            {
                chTransmit = 1;
            }

            CAdoStoredProcedure adoStoredProcedure ( "PKG_COMM_SERVER.LOG" );
            //PROCEDURE LOG (
            //    pi_callnumber          in integer,
            //    pi_call_start_time     in timestamp default null,
            //    pi_centralauditor      in varchar2 default null,
            //    pi_tohost              in smallint,
            //    pi_retransmit          in smallint,
            //    pi_command             in varchar2,
            //    pi_transmission_data   in blob   );

            _variant_t vtCallNumber (( long ) CallNumber, VT_I4 );
            adoStoredProcedure.AddParameter( "pi_callnumber", vtCallNumber, ADODB::DataTypeEnum::adInteger, ADODB::ParameterDirectionEnum::adParamInput, sizeof ( long ));

            _variant_t vtCallStartTime ( dCallStartTime, VT_DATE );
            adoStoredProcedure.AddParameter( "pi_call_start_time", vtCallStartTime, ADODB::DataTypeEnum::adDate, ADODB::ParameterDirectionEnum::adParamInput, sizeof ( double ));

            _bstr_t bstrCentralAuditor( m_SerialNumber );
            _variant_t vtCentralAuditor ( bstrCentralAuditor );
            adoStoredProcedure.AddParameter( "pi_centralauditor", vtCentralAuditor, ADODB::DataTypeEnum::adBSTR, ADODB::ParameterDirectionEnum::adParamInput, bstrCentralAuditor.length());

            _variant_t vtToHost ( chTransmit );
            adoStoredProcedure.AddParameter( "pi_tohost", vtToHost, ADODB::DataTypeEnum::adTinyInt, ADODB::ParameterDirectionEnum::adParamInput, sizeof ( chTransmit ));


            char chRetransmit = 0;
            if ( Retransmit == true )
            {
                chRetransmit = 1;
            }
            _variant_t vtRetransmit ( chRetransmit );
            adoStoredProcedure.AddParameter( "pi_retransmit", vtRetransmit, ADODB::DataTypeEnum::adTinyInt, ADODB::ParameterDirectionEnum::adParamInput, sizeof ( chRetransmit ));


            _bstr_t bstrCommand( m_szCurrentCommand );
            _variant_t vtCommand ( bstrCommand );
            adoStoredProcedure.AddParameter( "pi_command", vtCommand, ADODB::DataTypeEnum::adBSTR, ADODB::ParameterDirectionEnum::adParamInput, bstrCommand.length());

            variantBlob vtBuffer ( Transmission, TransmissionLength );
            adoStoredProcedure.AddParameter( "pi_transmission_data", vtBuffer, ADODB::DataTypeEnum::adVarBinary, ADODB::ParameterDirectionEnum::adParamInput, TransmissionLength );

            //m_EventTrace.Event ( CEventTrace::Details, "CProtelHost::Database_CommunicationsData -->g_pAdoConnection->ExecuteNonQuery" );
            m_padoConnection->ExecuteNonQuery( adoStoredProcedure, false );
            //m_EventTrace.Event ( CEventTrace::Details, "CProtelHost::Database_CommunicationsData <--g_pAdoConnection->ExecuteNonQuery" );
            return true;
        }
        catch ( _com_error &comError )
        {
            m_EventTrace.Event ( CEventTrace::Information, "CProtelHost::Database_CommunicationsData <--> ERROR: %s", CErrorMessage::ReturnComErrorMessage ( comError ));
        }
        return false;
    }

    void Database_UpdateCentralAuditor ( void )
    {
        /**************************************************************************************************************
         * This is called from Process_I_Response. The called procedure updates the existing row in the               *
         * COMM_SERVER_CALL table where CALLNUMBER matches. It also updates the CENTRALAUDITOR field in any existing  *
         * rows in the COMM_SERVER_DETAILS and COMM_SERVER_LOG tables where CALLNUMBER matches.                       *
         **************************************************************************************************************/
        try
        {
            //procedure central_auditor (
            //  pi_CALLNUMBER           IN       INTEGER,
            //  pi_CENTRALAUDITOR       IN       VARCHAR2  DEFAULT NULL
            //  , pi_callstarttime    in timestamp  DEFAULT NULL,
            //  pi_SIMID                IN       VARCHAR2,
            //  pi_CARDREADERID         IN       VARCHAR2,
            //  pi_CARDREADERREVISION   IN       VARCHAR2,
            //  pi_CARDREADER_FW_VERSION     IN       VARCHAR2,
            //  pi_cardreader_cfg_ver in      integer default null,
            //  po_CALLFLAG             OUT      INTEGER

            CAdoStoredProcedure adoStoredProcedure ( "PKG_COMM_SERVER.CENTRAL_AUDITOR" );


            _variant_t vtCallNumber (( long ) CallNumber, VT_I4 );
            adoStoredProcedure.AddParameter( "pi_CALLNUMBER", vtCallNumber, ADODB::DataTypeEnum::adInteger, ADODB::ParameterDirectionEnum::adParamInput, sizeof ( long ));

            _bstr_t bstrCentralAuditor( m_SerialNumber );
            _variant_t vtCentralAuditor ( bstrCentralAuditor );
            adoStoredProcedure.AddParameter( "pi_CENTRALAUDITOR", vtCentralAuditor, ADODB::DataTypeEnum::adBSTR, ADODB::ParameterDirectionEnum::adParamInput, bstrCentralAuditor.length());
            //char m_SerialNumber [ 64 ];
            //char m_CellModemSimmID [ 64 ];
            //char m_CardReaderID [ 64 ];
            //char m_CardReaderRevision [ 64 ];
            //char m_CardReaderFirmwareVersion [ 64 ];
            //char m_CardReaderConfigVersion [ 64 ];

            _variant_t vtCallStartTime ( dCallStartTime, VT_DATE );
            adoStoredProcedure.AddParameter( "pi_callstarttime", vtCallStartTime, ADODB::DataTypeEnum::adDate, ADODB::ParameterDirectionEnum::adParamInput, sizeof ( double ));

            _bstr_t bstrCellModemSimmID( m_CellModemSimmID );
            _variant_t vtCellModemSimmID ( bstrCellModemSimmID );
            adoStoredProcedure.AddParameter( "pi_SIMID", vtCellModemSimmID, ADODB::DataTypeEnum::adBSTR, ADODB::ParameterDirectionEnum::adParamInput, bstrCellModemSimmID.length());

            _bstr_t bstrCardReaderID( m_CardReaderID );
            _variant_t vtCardReaderID ( bstrCardReaderID );
            adoStoredProcedure.AddParameter( "pi_CARDREADERID", vtCardReaderID, ADODB::DataTypeEnum::adBSTR, ADODB::ParameterDirectionEnum::adParamInput, bstrCardReaderID.length());

            _bstr_t bstrCardReaderRevision( m_CardReaderRevision );
            _variant_t vtCardReaderRevision ( bstrCardReaderRevision );
            adoStoredProcedure.AddParameter( "pi_CARDREADERREVISION", vtCardReaderRevision, ADODB::DataTypeEnum::adBSTR, ADODB::ParameterDirectionEnum::adParamInput, bstrCardReaderRevision.length());

            _bstr_t bstrCardReaderFirmwareVersion( m_CardReaderFirmwareVersion );
            _variant_t vtCardReaderFirmwareVersion ( bstrCardReaderFirmwareVersion );
            adoStoredProcedure.AddParameter( "pi_CARDREADER_FW_VERSION", vtCardReaderFirmwareVersion, ADODB::DataTypeEnum::adBSTR, ADODB::ParameterDirectionEnum::adParamInput, bstrCardReaderFirmwareVersion.length());

            long CardReaderConfigVersion = atol ( m_CardReaderConfigVersion );
            _variant_t vtCardReaderConfigVersion (( long ) CardReaderConfigVersion, VT_I4 );
            adoStoredProcedure.AddParameter( "pi_cardreader_cfg_ver", vtCardReaderConfigVersion, ADODB::DataTypeEnum::adBSTR, ADODB::ParameterDirectionEnum::adParamInput, sizeof ( long ));

            _variant_t vtCallFlag (( short ) 0, VT_I2 );
            adoStoredProcedure.AddParameter( "po_CALLFLAG", vtCallFlag, ADODB::DataTypeEnum::adInteger, ADODB::ParameterDirectionEnum::adParamOutput, sizeof ( short ));

            //m_EventTrace.Event ( CEventTrace::Details, "CProtelHost::Database_UpdateCentralAuditor -->g_pAdoConnection->ExecuteNonQuery" );
            if (m_padoConnection->ExecuteNonQuery( adoStoredProcedure, false ) == false)
			{
				Transmit_A_Command(false);		// see bug 3010
			}
            //m_EventTrace.Event ( CEventTrace::Details, "CProtelHost::Database_UpdateCentralAuditor <--g_pAdoConnection->ExecuteNonQuery" );

            vtCallFlag = adoStoredProcedure.GetParameter("po_CALLFLAG");
            protelCallFlag = (ProtelCallFlag)vtCallFlag.iVal;
        }
        catch ( _com_error &comError )
        {
            m_EventTrace.Event ( CEventTrace::Information, "CProtelHost::Database_UpdateCentralAuditor <--> ERROR: %s", CErrorMessage::ReturnComErrorMessage ( comError ));
        }
    }

    void ProcessApplicationLayerError ( BYTE ErrorCode )                                      // used to do nothing!!!!
    {
        /**************************************************************************************************************
         * This is used by Process_E_Response (where the remote master signals an application layer error).           *
         *                                                                                                            *
         * At present, it just ends the connection.                                                                   *
         **************************************************************************************************************/
        ApplicationLayerError error = ( ApplicationLayerError ) ErrorCode;          // but this doesn't do anything!!!!
        Transmit_A_Command(false);                    // wjs I sure it was supposed to do this here he must have forgot
    }

    void CleanupProtelDevices ( void )
    {
        /**************************************************************************************************************
         * This deletes any devices in m_pProtelDevices[].                                                            *
         **************************************************************************************************************/
        for ( int nLoop = 0; nLoop < ( sizeof ( m_pProtelDevices ) / sizeof ( m_pProtelDevices[ 0 ] )); nLoop++ )

        {
            if ( m_pProtelDevices[ nLoop ] != NULL )
            {
                delete m_pProtelDevices[ nLoop ];
                m_pProtelDevices[ nLoop ] = NULL;
            }
        }
    }

protected:
    void CancelTimer ( HANDLE& hTimer )
    {
        /**************************************************************************************************************
         * This sets timer *hTimer inactive (so it won't become signalled until after it is set again).               *
         **************************************************************************************************************/
#if 1
        CancelWaitableTimer ( hTimer );
#else
        // MAXLONG is defined as 0x7fffffff
        // so this resets the timer to not signal until
        // 2147483.647 seconds from now (596 hours [3.5 weeks] from now)
        LARGE_INTEGER li = { 0 };
        SetWaitableTimer ( m_hTimer, &li, MAXLONG, NULL, NULL, FALSE);
#endif
    }

    void SetTimeoutTimer ( HANDLE& hTimer, int Seconds )
    {
        /**************************************************************************************************************
         * This sets timer *hTimer to become signalled in Seconds seconds from now (unless it is cancelled before     *
         * then).                                                                                                     *
         **************************************************************************************************************/
#if 1
        __int64 i64DueTime;
        LARGE_INTEGER liDueTime;

        // Create a negative 64-bit integer that will be used to
        // signal the timer Seconds seconds from now.

        i64DueTime = Seconds * -1;
        i64DueTime *= _SECOND;

        // Copy the relative time into a LARGE_INTEGER.
        liDueTime.LowPart  = (DWORD) ( i64DueTime & 0xFFFFFFFF );
        liDueTime.HighPart = (LONG)  ( i64DueTime >> 32 );
        BOOL bSuccess = SetWaitableTimer(
            hTimer,                                                                                // [in] timer handle
            &liDueTime,                          // [in] time until signalled in 100 ns intervals, negative == relative
            0,                                                                     // period [in]. 0 - signal once only
            NULL,                                                        // pointer to optional completion routine [in]
            NULL,                                            // pointer to struct passed to the completion routine [in]
            TRUE );                                           // [in] TRUE - restore suspended system when time expires
#else
        LARGE_INTEGER li = { 0 };
        SetWaitableTimer ( hTimer, &li, Seconds * 1000, NULL, NULL, FALSE);
#endif
    }

    virtual char* GetPort ( void )
    {
        /**************************************************************************************************************
         * This is overridden in ProtelSerial or ProtelSocket to return the human-readable port name (e.g. "\\.\COM3" *
         * or "192.168.3.2:38308").                                                                                   *
         **************************************************************************************************************/
        return "?????";
    }

    virtual char* GetDevice ( void )
    {
        /**************************************************************************************************************
         * This is overridden in ProtelSerial or ProtelSocket to return the human-readable device name (e.g.          *
         * "Standard 1200 bps Modem" or "TCP/IP").                                                                    *
         **************************************************************************************************************/
        return "?????";
    }

    virtual int GetWaitSeconds ( void )
    {
        /**************************************************************************************************************
         * This is overridden in ProtelSerial or ProtelSocket to return the timeout to use if a response to a command *
         * isn't received: 10 and 90 seconds respectively.                                                            *
         **************************************************************************************************************/
        return 1;
    }

    virtual int CommsErrsLimit ( void )
    {
        /**************************************************************************************************************
         * This is overridden in ProtelSerial or ProtelSocket to return the error limit that causes the call to be    *
         * aborted if a valid response isn't received (9 and 0 respectively).                                         *
         **************************************************************************************************************/
        return 1;
    }

/*	char* corrected_serial ( bool have_simm  )
	{
		/********************************************************************************************************
		 * If the esn is available then use the esn to get the sn fromn the database
		 * if the only the corrupt sn exist then  use the valid part of the sn to find the 
		 * closest match then check call in  date/time for further resolution
		 ********************************************************************************************************/

	  /*  Environment *env = Environment::createEnvironment();

		OracleConnection conn = new OracleConnection( profileValues.GetConnectionString());
		Connection *conn1 = env->createConnection(
		conn.Open();

		OracleCommand cmd = conn.CreateCommand();
        
		cmd.CommandText = "select SIMMID from simms_tab where esn=" + m_CellModemSimmID ;

		OracleDataReader dr = cmd.ExecuteReader();


		char *simm = dr.GetString();
		
		cmd.CommandText = "select monitorid from AVW2_DATA_V where simmid=" + simm;
		OracleDataReader dr = cmd.ExecuteReader();
		char * valid_sn = dr.GetString(0);
*/
		
/*		cmd.CommandText = "select MONITORID from AVW2_DATA_V where simmid=:v1'" +
    timestampCol.ToString("dd MMM yyyy hh:mm:sstt") + "', '" +
    timestampTZCol.ToString("dd MMM yyyy hh:mm:sstt") + "', '" +
    timestampLTZCol.ToString("dd MMM yyyy hh:mm:sstt") + "')";
*/

/*where upper(monitorid) like upper( '%'||lv_compareString||'%')
		creater sql to return sn nearly = corrupt sn
	}*/
 };
