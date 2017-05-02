#pragma once

#include "AdoConnection.h"
#include "EventTrace.h"
#include "ProtelDevice.h"
#include "variantBlob.h"

#define _SECOND 10000000
//#define __WAIT_TIME__	4

class CProtelHost
{
protected:
	enum ProtelCallFlag
	{
		ProcessNormally,
		HangupImmediately,
	};

	enum ReasonPinging
	{
		NotPinging,
		ConfigurationFile,
		FirmwareFile,
		RequestedDEXRead,
	};

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

	enum LinkLayerError : unsigned char
	{
		// Bad Checksum
		BadChecksum = 0x00,
		// Bad Length
		BadLength = 0x01,
	};

	HANDLE m_hShutDown;
	HANDLE m_hTimer;
	int m_nMessageBufferOffset;
	BYTE m_szMessageBuffer [ 4096 ];
	BYTE m_szPayload [ 4096 ];
	BYTE m_transmitBuffer [ 4096 ];
	CAdoConnection* m_padoConnection;

	int m_nLastTransmission;
	BYTE m_LastTransmission [ 4096 ];
	int m_nRetransmitCount;

	CEventTrace m_EventTrace;

	char m_SerialNumber [ 64 ];
	char m_CellModemSimmID [ 64 ];
	char m_CardReaderID [ 64 ];
	char m_CardReaderRevision [ 64 ];
	char m_CardReaderFirmwareVersion [ 64 ];
	char m_CardReaderConfigVersion [ 64 ];
	bool RamFull;
	bool HaveDexFiles;
	bool BatteryMode;
	int CallReason;
	int CallNumber;
	double dCallStartTime;
	char m_szCurrentCommand [ 2 ];

	CProtelDevice* m_pProtelDevices [ 32 ];

	int m_nCurrentAuditDevice;
	int m_nAuditDevices;
	long m_nConfigurationLength;
	BYTE *m_pConfiguration;
	int m_nDexFileRemoteAddress;
	char m_ActiveSerialNumber [ 64 ];
	bool m_NormalShutdown;

	ProtelCallFlag protelCallFlag;

	bool Download2ndConfiguration;
	int Download2ndDevice;

	ReasonPinging m_nReasonPinging;

public:
	CProtelHost(HANDLE hShutDown) :
	  Closed ( false ),
		  m_hShutDown ( hShutDown ),
		  m_nLastTransmission ( 0 ),
		  m_nMessageBufferOffset ( 0 ),
		  m_NormalShutdown ( true ),
		  Download2ndConfiguration ( false ),
		  Download2ndDevice ( -1 ),
		  CallNumber ( 0 ),
		  dCallStartTime (( double ) 0 ),
		  m_nReasonPinging ( ReasonPinging::NotPinging ),
		  m_padoConnection ( NULL )
	  {
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

		  m_EventTrace.Event( CEventTrace::Details, "CProtelHost::CProtelHost(void)" );
		  Initialize();
	  }

	  virtual ~CProtelHost(void)
	  {
		  if ( m_hTimer != NULL )
		  {
			  CloseHandle ( m_hTimer );
			  m_hTimer = NULL;
		  }
		  CleanupProtelDevices();
		  if ( m_padoConnection != NULL )
		  {
			  delete m_padoConnection;
			  m_padoConnection = NULL;
		  }
	  }

	  virtual void Send(LPBYTE pszBuffer, int BufferLength)
	  {
		  throw "Not Implemented";
	  }

	  virtual void Shutdown ( void )
	  {
		  throw "Not Implemented";
	  }

	  bool Closed;

protected:
	void MessageReceived ( BYTE* pBuffer, int bytesRead )
	{
		m_nRetransmitCount = 0;
		m_EventTrace.HexDump( CEventTrace::Details, pBuffer, bytesRead );
		Database_CommunicationsData ( false, false, pBuffer, bytesRead );
		MoveMemory ( m_szMessageBuffer + m_nMessageBufferOffset, pBuffer, bytesRead );
		m_nMessageBufferOffset += bytesRead;
		if ( m_szMessageBuffer[0] == (BYTE) 'T' )
		{
			if ( m_szMessageBuffer[1] <= ( m_nMessageBufferOffset - 3 ))
			{
				if (IsValidChecksum( m_szMessageBuffer, /*m_nMessageBufferOffset*/ m_szMessageBuffer[ 1 ] + 3 ))
				{
					//CancelWaitableTimer( m_hTimer );
					CancelTimer( m_hTimer );
					Database_Dialog ( false, m_szMessageBuffer, m_szMessageBuffer[1]+3 );
					int nPayloadLength = m_nMessageBufferOffset - 4;
					ZeroMemory ( m_szPayload, sizeof ( m_szPayload ));
					MoveMemory ( m_szPayload, m_szMessageBuffer + 3, m_nMessageBufferOffset - 4 );
					switch ( m_szMessageBuffer[2] )
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
			}
		}
	}

	bool IsValidChecksum ( BYTE* Buffer, int BufferLength )
	{
		unsigned char Checksum = CalculateChecksum( Buffer, BufferLength );
		if (Checksum == *( Buffer + BufferLength - 1 ))
		{
			return true;
		}
		return false;
	}

	unsigned char CalculateChecksum ( BYTE* Buffer, int Length )
	{
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
		ZeroMemory ( m_szMessageBuffer, sizeof ( m_szMessageBuffer ));
		m_nMessageBufferOffset = 0;
	}

	virtual void Initialize ( void )
	{
		m_nMessageBufferOffset = 0;
		ZeroMemory ( m_szMessageBuffer, sizeof ( m_szMessageBuffer ));
		ZeroMemory ( m_szPayload, sizeof ( m_szPayload ));
		ZeroMemory ( m_transmitBuffer, sizeof ( m_transmitBuffer ));

		m_nLastTransmission = 0;
		ZeroMemory ( m_LastTransmission, sizeof ( m_LastTransmission ));
		m_nRetransmitCount = 0;

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
		Download2ndDevice = -1;
		m_nReasonPinging = ReasonPinging::NotPinging;
		m_NormalShutdown = true;
		protelCallFlag = ProtelCallFlag::ProcessNormally;

		{
			if ( m_padoConnection != NULL )
			{
				delete m_padoConnection;
				m_padoConnection = NULL;
			}
			CProfileValues profileValues;
			m_padoConnection = new CAdoConnection;
			m_padoConnection->ConnectionStringOpen( profileValues.GetConnectionString());
		}

		//CancelWaitableTimer( m_hTimer );
		CancelTimer( m_hTimer );


		SYSTEMTIME CallStartTime;
		GetSystemTime ( &CallStartTime );
		SystemTimeToVariantTime ( &CallStartTime, &dCallStartTime );

		m_EventTrace.Event( CEventTrace::Details, "CProtelHost::Initialize ( void )" );

		m_nCurrentAuditDevice = 0;
		m_nAuditDevices = 0;
		m_nDexFileRemoteAddress = 0;

		CleanupProtelDevices();
		for ( int nLoop = 0; nLoop < ( sizeof ( m_pProtelDevices ) / sizeof ( m_pProtelDevices[ 0 ] )); nLoop++ )
		{
			m_pProtelDevices[ nLoop ] = new CProtelDevice ( m_padoConnection );
		}
	}

	bool Transmit_A_Command ( bool NormalShutdown )
	{
		m_NormalShutdown = NormalShutdown;
		BYTE ShutdownFlag = ( BYTE ) NormalShutdown;
		Transmit( 'A', &ShutdownFlag, sizeof ( ShutdownFlag ));
		return true;
	}

	void Process_A_Response ( void )
	{
		CloseDevice();
	}

	bool Transmit_C_Command ( void )
	{
		BYTE* pBuffer = NULL;
		long nLength = 0;
		if ( Download2ndConfiguration == false )
		{
			while ( pBuffer == NULL )
			{
				pBuffer = m_pProtelDevices [ m_nCurrentAuditDevice ]->GetNextConfiguration( nLength );
				if ( pBuffer == NULL )
				{
					if ( m_nReasonPinging == ReasonPinging::ConfigurationFile )
					{
						Transmit_Z_Command();
						return true;
					}
					m_nCurrentAuditDevice++;
					if ( m_nCurrentAuditDevice >= m_nAuditDevices )
					{
						m_nCurrentAuditDevice = 0;
						Transmit_O_Command();
						return true;
					}
				}
			}
		}
		else
		{
			pBuffer = m_pProtelDevices [ Download2ndDevice ]->GetNextConfiguration( nLength );
			if ( pBuffer == NULL )
			{
				if ( m_nReasonPinging == ReasonPinging::ConfigurationFile )
				{
					Transmit_Z_Command();
					return true;
				}
				Transmit_A_Command(true);
				return true;
			}
		}
		Transmit( 'C', pBuffer, nLength );
		return true;
	}

	void Process_C_Response ( void )
	{
		Transmit_C_Command();
	}

	void Transmit_D_Command ( void )
	{
		Transmit( 'D', NULL, 0 );
	}

	void Process_D_Response ( void )
	{
		m_nCurrentAuditDevice = 0;
		Transmit_V_Command();
	}

	void Transmit_I_Command ( void )
	{
		Transmit ( 'I', NULL, 0 );
	}

	void Process_I_Response ( void )
	{
		// Response from Master auditor-
		// The response from the Master Auditor will be a repeat of the command and the
		// 8 bytes of the serial number. If the connection is cellular, the serial
		// number will be followed by a colon as a field separator and then a 20
		// byte SIM ID of the cellular modem.   
		// T | 0x1e | I | Monitor Serial Number | : | Cell Modem SIM ID |  Checksum

		//--Monitor Serial # : SIM ID : Card Reader Serial # : --card reader revision -- : Card Reader Firmware Version : Card Reader Config #
		//--RMC79260:89310380106023138946:RFX02005::AC0000045A:1106

		//Monitor Serial #
		BYTE* pszColon = ( BYTE* )StrChr (( LPCTSTR ) m_szPayload, ':' );
		if ( pszColon != NULL )
		{
			*( pszColon ) = '\0';
			StringCbCopy ( m_SerialNumber, sizeof ( m_SerialNumber ), ( LPCTSTR )m_szPayload );
			pszColon++;
			BYTE* pszCellModemSimmID = pszColon;
			pszColon = ( BYTE* )StrChr (( LPCTSTR ) pszCellModemSimmID, ':' );
			if ( pszColon != NULL )
			{
				*( pszColon ) = '\0';
				// SIM ID
				StringCbCopy ( m_CellModemSimmID, sizeof ( m_CellModemSimmID ), ( LPCTSTR )pszCellModemSimmID );
				pszColon++;
				BYTE* pszCardReaderID = pszColon;
				pszColon = ( BYTE* )StrChr (( LPCTSTR ) pszCardReaderID, ':' );
				if ( pszColon != NULL )
				{
					*( pszColon ) = '\0';
					// Card Reader Serial #
					StringCbCopy ( m_CardReaderID, sizeof ( m_CardReaderID ), ( LPCTSTR )pszCardReaderID );
					pszColon++;
					BYTE* pszCardReaderRevision = pszColon;
					pszColon = ( BYTE* )StrChr (( LPCTSTR ) pszCardReaderRevision, ':' );
					if ( pszColon != NULL )
					{
						*( pszColon ) = '\0';
						// card reader revision
						StringCbCopy ( m_CardReaderRevision, sizeof ( m_CardReaderRevision ), ( LPCTSTR )pszCardReaderRevision );
						pszColon++;
						BYTE* pszCardReaderFirmwareVersion = pszColon;
						pszColon = ( BYTE* )StrChr (( LPCTSTR ) pszCardReaderFirmwareVersion, ':' );
						if ( pszColon != NULL )
						{
							*( pszColon ) = '\0';
							// Card Reader Firmware Version
							StringCbCopy ( m_CardReaderFirmwareVersion, sizeof ( m_CardReaderFirmwareVersion ), ( LPCTSTR )pszCardReaderFirmwareVersion );
							pszColon++;
							BYTE* pszCardReaderConfigVersion = pszColon;
							pszColon = ( BYTE* )StrChr (( LPCTSTR ) pszCardReaderConfigVersion, ':' );
							if ( pszColon != NULL )
							{
								*( pszColon ) = '\0';
							}
							// Card Reader Config #
							StringCbCopy ( m_CardReaderConfigVersion, sizeof ( m_CardReaderConfigVersion ), ( LPCTSTR )pszCardReaderConfigVersion );
						}
					}
				}
			}
		}
		else
		{
			//Monitor Serial #
			StringCbCopy ( m_SerialNumber, sizeof ( m_SerialNumber ), ( LPCTSTR )m_szPayload );
		}
		Database_UpdateCentralAuditor();
		for ( int nLoop = 0; nLoop < ( sizeof ( m_pProtelDevices ) / sizeof ( m_pProtelDevices[ 0 ] )); nLoop++ )
		{
			m_pProtelDevices[ nLoop ]->CentralAuditor = m_SerialNumber;
			m_pProtelDevices[ nLoop ]->CallStartTime = dCallStartTime;
			m_pProtelDevices[ nLoop ]->CallNumber = CallNumber;
		}
		if ( protelCallFlag == ProtelCallFlag::HangupImmediately )
		{
			Transmit_A_Command ( true );
		}
		else
		{
			Transmit_S_Command();
		}
	}

	void Transmit_N_Command ( int DeviceNumber )
	{
		BYTE DeviceNumberBytes[ 2 ];
		DeviceNumberBytes[ 0 ] = (( DeviceNumber & 0xff00 ) >> 8 );
		DeviceNumberBytes[ 1 ] = ( DeviceNumber & 0xff );
		Transmit( 'N', DeviceNumberBytes, sizeof ( DeviceNumberBytes ));
	}

	void Process_N_Response ( int nPayloadLength )
	{
		int Offset = 2;
		while ( Offset < nPayloadLength )
		{
			m_pProtelDevices[ m_nAuditDevices ]->AuditDevice = ( m_szPayload + Offset );
			m_nAuditDevices++;
			Offset += sizeof ( AuditDevice );
		}

		if ( m_szPayload[ 0 ] == 0xff && m_szPayload[ 1 ] == 0xff )
		{
			for ( int nAuditDevice = 0; nAuditDevice < m_nAuditDevices; nAuditDevice++ )
			{
				m_pProtelDevices[ nAuditDevice ]->GetConfiguration();
				m_pProtelDevices[ nAuditDevice ]->GetFirmware();
				m_pProtelDevices[ nAuditDevice ]->GetFreeBee();
			}
			for ( int nAuditDevice = 0; nAuditDevice < m_nAuditDevices; nAuditDevice++ )
			{
				__int64 FirmwareCheckSum = m_pProtelDevices[ nAuditDevice ]->FirmwareChecksum;
				__int64 ConfigurationChecksum = m_pProtelDevices[ nAuditDevice ]->ConfigurationChecksum;
				for ( int nAuditDevice2 = ( nAuditDevice + 1 ); nAuditDevice2 < m_nAuditDevices; nAuditDevice2++ )
				{
					if ( m_pProtelDevices[ nAuditDevice ]->FirmwareLength == m_pProtelDevices[ nAuditDevice ]->FirmwareLength )
					{
						if ( m_pProtelDevices[ nAuditDevice2 ]->FirmwareChecksum == FirmwareCheckSum )
						{
							m_pProtelDevices[ nAuditDevice2 ]->FirmwareDuplicate = true;
							m_pProtelDevices[ nAuditDevice ]->AddFirmwareDuplicate( m_pProtelDevices[ nAuditDevice2 ]->Address );
						}
					}
					if ( m_pProtelDevices[ nAuditDevice ]->ConfigurationLength == m_pProtelDevices[ nAuditDevice ]->ConfigurationLength )
					{
						if ( m_pProtelDevices[ nAuditDevice2 ]->ConfigurationChecksum == ConfigurationChecksum )
						{
							m_pProtelDevices[ nAuditDevice2 ]->ConfigurationDuplicate = true;
							m_pProtelDevices[ nAuditDevice ]->AddConfigurationDuplicate( m_pProtelDevices[ nAuditDevice2 ]->Address );
						}
					}
				}
			}
			if( RamFull == true || HaveDexFiles == true )
			{
				ZeroMemory ( m_ActiveSerialNumber, sizeof ( m_ActiveSerialNumber ));
				MoveMemory ( m_ActiveSerialNumber, m_SerialNumber, lstrlen ( m_SerialNumber ));
				m_nDexFileRemoteAddress = 0;
				Transmit_U_Command( 1 );
			}
			else
			{
				Transmit_A_Command(true);
			}
		}
		else
		{
			int LastPacketNumber = ( m_szPayload[ 0 ] * 256 ) + m_szPayload[ 1 ];
			Transmit_N_Command( LastPacketNumber + 1 );
		}
	}

	bool Transmit_O_Command ( void )
	{
		BYTE* pBuffer = NULL;
		long nLength = 0;
		while ( pBuffer == NULL )
		{
			pBuffer = m_pProtelDevices [ m_nCurrentAuditDevice ]->GetNextFirmware( nLength );
			if ( pBuffer == NULL )
			{
#if FALSE
				m_nCurrentAuditDevice++;
				if ( m_nCurrentAuditDevice >= m_nAuditDevices )
				{
					m_nCurrentAuditDevice = 0;
					//Transmit_C_Command();
					Transmit_A_Command(true);
					return true;
				}
#else
				m_pProtelDevices[ m_nCurrentAuditDevice ]->GetConfiguration();
				Download2ndConfiguration = true;
				Download2ndDevice = m_nCurrentAuditDevice;
				m_nReasonPinging = ReasonPinging::ConfigurationFile;
				Transmit_C_Command();
				return true;
#endif
			}
		}
		Transmit( 'O', pBuffer, nLength );
		return true;
	}

	void Process_O_Response ( void )
	{
		Transmit_O_Command();
	}

	void Transmit_R_Command ( int Address )
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

	bool Transmit_S_Command ( void )
	{
		RamFull = false;
		HaveDexFiles = false;
		BatteryMode = false;
		CallReason = 0x00;	// 00 = Undefined

		// This command requests the 2 status bytes and sends the date and time to the
		// Master Auditor.
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
		case 0:	// Sunday = 0
			payload[10] = ProtelDayOfTheWeek::Sunday;
			break;
		case 1:	// Monday = 1
			payload[10] = ProtelDayOfTheWeek::Monday;
			break;
		case 2:	// Tuesday = 2
			payload[10] = ProtelDayOfTheWeek::Tuesday;
			break;
		case 3:	// Wednesday = 3
			payload[10] = ProtelDayOfTheWeek::Wednesday;
			break;
		case 4:	// Thursday = 4
			payload[10] = ProtelDayOfTheWeek::Thursday;
			break;
		case 5:	// Friday = 5
			payload[10] = ProtelDayOfTheWeek::Friday;
			break;
		case 6:	// Saturday = 6
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

		CallReason = ( m_szPayload[ 1 ] & 0x03 );
		Database_UpdateCallStatus();
		// Pass a '1' to initiate the conversation with the master auditor!
		Transmit_N_Command( 1 );
	}

	void Transmit_U_Command ( int PacketNumber )
	{
		m_nReasonPinging = ReasonPinging::FirmwareFile;
		BYTE PacketNumberBytes[ 2 ];
		PacketNumberBytes[ 0 ] = HIBYTE ( PacketNumber );//(( PacketNumber & 0xff00 ) >> 8 );
		PacketNumberBytes[ 1 ] = LOBYTE ( PacketNumber );//( PacketNumber & 0xff );
		Transmit( 'U', PacketNumberBytes, sizeof ( PacketNumberBytes ));
	}

	void Process_U_Response ( int nPayloadLength )
	{
		int LastPacketNumber = ( m_szPayload[ 0 ] * 256 ) + m_szPayload[ 1 ];
		Database_DexData ( m_ActiveSerialNumber, CallNumber, LastPacketNumber, m_szPayload + 2, nPayloadLength - 2 );
		if ( m_szPayload[ 0 ] == 0xff && m_szPayload[ 1 ] == 0xff )
		{
			Transmit_D_Command();
		}
		else
		{
			int LastPacketNumber = ( m_szPayload[ 0 ] * 256 ) + m_szPayload[ 1 ];
			Transmit_U_Command( LastPacketNumber + 1 );
		}
	}

	bool Transmit_V_Command ( void )
	{
		while ( m_nCurrentAuditDevice < m_nAuditDevices )
		{
			if ( m_pProtelDevices [ m_nCurrentAuditDevice ]->FreeBeeController < 1 )
			{
				m_nCurrentAuditDevice++;
				continue;
			}
			BYTE szPayload [ 256 ];
			ZeroMemory ( szPayload, sizeof ( szPayload ));
			szPayload [ 0 ] = ( BYTE ) m_pProtelDevices [ m_nCurrentAuditDevice ]->FreeBeeController;
			StringCbCat (( LPTSTR ) szPayload + 1, sizeof ( szPayload ) - 1, m_pProtelDevices [ m_nCurrentAuditDevice ]->FreeBeeSerialNumber );
			Transmit( 'V', szPayload, lstrlen (( LPTSTR )( szPayload + 1 ) + 1 ));
			return true;
		}
		m_nCurrentAuditDevice = 0;
		m_nReasonPinging = ReasonPinging::ConfigurationFile;
		Transmit_C_Command();
		return true;
	}

	void Process_V_Response ( void )
	{
		m_nCurrentAuditDevice++;
		Transmit_V_Command();
	}

	void Transmit_Z_Command ( void )
	{
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
		//	0x00 = Idle, still processing
		//	0x01 = Task in progress
		//	0x02 = Task complete, request to report results

		if ( m_szPayload [ 0 ] == 0x01 ) // Task in progress
		{
			SleepEx(1000,TRUE);	// Pause for 1 second before continuing -- don't overwhelm remote!
			Transmit_Z_Command();
			return;
		}

		if ( m_szPayload [ 0 ] == 0x02 ) // Task completed
		{
			if ( nPayloadLength > 2 )
			{
				int nFailureCount = ( int ) m_szPayload [ 1 ];
				int nOffset = -1;
				for ( int nLoop = 0; nLoop < nFailureCount; nLoop++ )
				{
					nOffset = m_szPayload [ nLoop + 1 ];
					m_pProtelDevices [ nOffset ]->SerialNumber;
				}
			}
		}

		switch ( m_nReasonPinging )
		{
		case ReasonPinging::ConfigurationFile:
			m_nReasonPinging = ReasonPinging::NotPinging;
			Transmit_C_Command();
			break;
		case ReasonPinging::FirmwareFile:
			m_nReasonPinging = ReasonPinging::NotPinging;
			Transmit_O_Command();
			break;
		case ReasonPinging::RequestedDEXRead:
			m_nReasonPinging = ReasonPinging::NotPinging;
			Transmit_U_Command( 1 );
			break;
		}

#if FALSE
		if ( m_szPayload [ 0 ] == 0x00 ||	// Idle, still processing
			m_szPayload [ 0 ] == 0x01 )	// Task in progress
		{
			SleepEx(1000,TRUE);	// Pause for 1 second before continuing -- don't overwhelm remote!
			Transmit_Z_Command();
		}
		else if ( m_szPayload [ 0 ] == 0x02 )	// = Task complete, request to report results
		{
			switch ( m_nReasonPinging )
			{
			case ReasonPinging::ConfigurationFile:
				m_nReasonPinging = ReasonPinging::NotPinging;
				Transmit_C_Command();
				break;
			case ReasonPinging::FirmwareFile:
				m_nReasonPinging = ReasonPinging::NotPinging;
				Transmit_O_Command();
				break;
			case ReasonPinging::RequestedDEXRead:
				m_nReasonPinging = ReasonPinging::NotPinging;
				Transmit_U_Command( 1 );
				break;
			}
		}
#endif
	}

	void Retransmit ( void )
	{
		if ( m_nLastTransmission > 0 )
		{
			m_nRetransmitCount++;
			m_EventTrace.Event( CEventTrace::Information, "void CProtelHost::Retransmit ( void ) [%d](%d)", m_nLastTransmission, GetTickCount());
			Database_CommunicationsData ( true, true, m_LastTransmission, m_nLastTransmission );

			ResetReceivedBuffer();

			Send ( m_LastTransmission, m_nLastTransmission );

			//SetTimeoutTimer( m_hTimer, __WAIT_TIME__ );
			SetTimeoutTimer( m_hTimer, GetWaitSeconds());
		}
	}

	void Transmit ( char command, BYTE* Payload, int PayloadLength)
	{
		m_szCurrentCommand [ 0 ] = command;
		m_szCurrentCommand [ 1 ] = '\0';
		ResetReceivedBuffer();
		ZeroMemory ( m_transmitBuffer, sizeof ( m_transmitBuffer ));

		if (Payload != NULL && PayloadLength > 0)
		{
			MoveMemory ( m_transmitBuffer + 3, Payload, PayloadLength );
		}

		m_transmitBuffer[0] = (BYTE)'T';
		m_transmitBuffer[1] = (BYTE)PayloadLength + 1;
		m_transmitBuffer[2] = (BYTE)command;

		BYTE Checksum = CalculateChecksum ( m_transmitBuffer, PayloadLength + 4 );
		m_transmitBuffer [ PayloadLength + 3 ] = Checksum;


		Database_Dialog ( true, m_transmitBuffer, PayloadLength + 4 );
		m_nLastTransmission = PayloadLength + 4;
		ZeroMemory ( m_LastTransmission, sizeof ( m_LastTransmission ));
		CopyMemory ( m_LastTransmission, m_transmitBuffer, m_nLastTransmission );
		Database_CommunicationsData ( true, false, m_LastTransmission, m_nLastTransmission );

		m_EventTrace.Event( CEventTrace::Details, "void CProtelHost::Transmit ( '%c', BYTE*, %d )", command, PayloadLength );
		m_EventTrace.HexDump( CEventTrace::Information, m_transmitBuffer, PayloadLength + 4 );
		Send ( m_transmitBuffer, PayloadLength + 4 );

		//SetTimeoutTimer( m_hTimer, __WAIT_TIME__ );
		SetTimeoutTimer( m_hTimer, GetWaitSeconds());
	}

	virtual void CloseDevice ( void )
	{
		CancelTimer( m_hTimer );
		Database_FinishCall();
		if ( m_padoConnection != NULL )
		{
			delete m_padoConnection;
			m_padoConnection = NULL;
		}
	}

	bool Database_AddNewCall ( void )
	{
		bool bReturn = false;

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

			_variant_t vtCallStartTime ( dCallStartTime, VT_DATE );
			adoStoredProcedure.AddParameter( "pi_CALL_START_TIME", vtCallStartTime, ADODB::DataTypeEnum::adDate, ADODB::ParameterDirectionEnum::adParamInput, sizeof ( double ));

			int DeviceType = 0;
			if ( lstrcmpi ( GetDevice(), "TCP/IP" ) == 0 )
			{
				DeviceType = 1;	// TCP/IP
			}
			else
			{
				DeviceType = 2;	// Modem
			}

			_variant_t vtDeviceType (( long ) DeviceType, VT_I4 );
			adoStoredProcedure.AddParameter("pi_device_type", vtCallNumber, ADODB::DataTypeEnum::adInteger, ADODB::ParameterDirectionEnum::adParamInput, sizeof(long));

			_bstr_t bstrDevice( GetDevice());
			_variant_t vtDevice ( bstrDevice );
			adoStoredProcedure.AddParameter( "pi_device_desc", vtDevice, ADODB::DataTypeEnum::adBSTR, ADODB::ParameterDirectionEnum::adParamInput, bstrDevice.length());

			_bstr_t bstrPort( GetPort());
			_variant_t vtPort ( bstrPort );
			adoStoredProcedure.AddParameter( "pi_port", vtPort, ADODB::DataTypeEnum::adBSTR, ADODB::ParameterDirectionEnum::adParamInput, bstrPort.length());



			m_EventTrace.Event ( CEventTrace::Details, "CProtelHost::Database_AddNewCall -->g_pAdoConnection->ExecuteNonQuery" );
			m_padoConnection->ExecuteNonQuery( adoStoredProcedure, false );
			m_EventTrace.Event ( CEventTrace::Details, "CProtelHost::Database_AddNewCall <--g_pAdoConnection->ExecuteNonQuery" );

			_variant_t vtReturnedCallNumber = adoStoredProcedure.GetParameter("po_call_number");
			CallNumber = ( long ) vtReturnedCallNumber;
			char szCallNumber [ 64 ];
			_itoa_s( CallNumber, szCallNumber, sizeof ( szCallNumber ), 10 );
			m_EventTrace.Identifier( szCallNumber, GetDevice(), GetPort());
			bReturn = true;
		}
		catch ( _com_error &comError )
		{
			m_EventTrace.Event ( CEventTrace::Information, "CProtelHost::Database_AddNewCall <--> ERROR: %s", CErrorMessage::ReturnComErrorMessage ( comError ));
		}
		return bReturn;
	}

	void Database_FinishCall ( void )
	{
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
			m_nRetransmitCount = 0;
			//CancelWaitableTimer( m_hTimer );
			CancelTimer( m_hTimer );
			m_EventTrace.Event( CEventTrace::Details, "CAdoStoredProcedure adoStoredProcedure ( PKG_COMM_SERVER.FINISH );" );
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
			if ( m_NormalShutdown == true )
			{
				nNormalShutdown = 1;
			}
			_variant_t vtSuccess (( short ) nNormalShutdown, VT_I2 );
			adoStoredProcedure.AddParameter( "pi_success", vtSuccess, ADODB::DataTypeEnum::adInteger, ADODB::ParameterDirectionEnum::adParamInput, sizeof ( short ));

			_bstr_t bstrErrorMessage( "" );
			_variant_t vtErrorMessage ( bstrErrorMessage );
			adoStoredProcedure.AddParameter( "pi_errormsg", vtErrorMessage, ADODB::DataTypeEnum::adBSTR, ADODB::ParameterDirectionEnum::adParamInput, bstrErrorMessage.length());

			m_EventTrace.Event ( CEventTrace::Details, "CProtelHost::Database_FinishCall -->g_pAdoConnection->ExecuteNonQuery" );
			m_padoConnection->ExecuteNonQuery( adoStoredProcedure, false );
			m_EventTrace.Event ( CEventTrace::Details, "CProtelHost::Database_FinishCall <--g_pAdoConnection->ExecuteNonQuery" );
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
		try
		{
			CAdoStoredProcedure adoStoredProcedure ( "PKG_COMM_SERVER.DEX" );


			//pi_callnumber            in    number,
			//pi_call_start_time   in timestamp,
			//pi_centralauditor    in varchar2 default null, --added by ESH
			//pi_serial_number     in varchar2 default null,
			//pi_sequence          in integer,                  
			//pi_dex_data          in blob,
			//pi_lastdexrecord     in smallint   );

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

			_variant_t vtSequence (( short ) nSequence, VT_I2 );
			adoStoredProcedure.AddParameter( "pi_sequence", vtSequence, ADODB::DataTypeEnum::adInteger, ADODB::ParameterDirectionEnum::adParamInput, sizeof ( short ));

			variantBlob vtPayload ( pPayload, nPayloadLength );
			adoStoredProcedure.AddParameter( "pi_dex_data", vtPayload, ADODB::DataTypeEnum::adVarBinary, ADODB::ParameterDirectionEnum::adParamInput, nPayloadLength );

			char chLastDexRecord = 0;
			if ( nSequence == 0xffff )
			{
				chLastDexRecord = 1;
			}
			_variant_t vtLastDexRecord ( chLastDexRecord );
			adoStoredProcedure.AddParameter( "pi_lastdexrecord", vtLastDexRecord, ADODB::DataTypeEnum::adTinyInt, ADODB::ParameterDirectionEnum::adParamInput, sizeof ( chLastDexRecord ));

			m_EventTrace.Event ( CEventTrace::Details, "CProtelHost::Database_DexData -->g_pAdoConnection->ExecuteNonQuery" );
			m_padoConnection->ExecuteNonQuery( adoStoredProcedure, false );
			m_EventTrace.Event ( CEventTrace::Details, "CProtelHost::Database_DexData <--g_pAdoConnection->ExecuteNonQuery" );
		}
		catch ( _com_error &comError )
		{
			m_EventTrace.Event ( CEventTrace::Information, "CProtelHost::Database_DexData <--> ERROR: %s", CErrorMessage::ReturnComErrorMessage ( comError ));
		}
	}

	void Database_UpdateCallStatus ( void )
	{
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

			m_EventTrace.Event ( CEventTrace::Details, "CProtelHost::Database_UpdateCallStatus -->g_pAdoConnection->ExecuteNonQuery" );
			m_padoConnection->ExecuteNonQuery( adoStoredProcedure, false );
			m_EventTrace.Event ( CEventTrace::Details, "CProtelHost::Database_UpdateCallStatus <--g_pAdoConnection->ExecuteNonQuery" );
		}
		catch ( _com_error &comError )
		{
			m_EventTrace.Event ( CEventTrace::Information, "CProtelHost::Database_UpdateCallStatus <--> ERROR: %s", CErrorMessage::ReturnComErrorMessage ( comError ));
		}
	}

	bool Database_Dialog ( bool Transmit, BYTE* Transmission, int TransmissionLength )
	{
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

			m_EventTrace.Event ( CEventTrace::Details, "CProtelHost::Database_Dialog -->g_pAdoConnection->ExecuteNonQuery" );
			m_padoConnection->ExecuteNonQuery( adoStoredProcedure, false );
			m_EventTrace.Event ( CEventTrace::Details, "CProtelHost::Database_Dialog <--g_pAdoConnection->ExecuteNonQuery" );
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

			m_EventTrace.Event ( CEventTrace::Details, "CProtelHost::Database_CommunicationsData -->g_pAdoConnection->ExecuteNonQuery" );
			m_padoConnection->ExecuteNonQuery( adoStoredProcedure, false );
			m_EventTrace.Event ( CEventTrace::Details, "CProtelHost::Database_CommunicationsData <--g_pAdoConnection->ExecuteNonQuery" );
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
		try
		{
			//procedure central_auditor (      
			//	pi_CALLNUMBER           IN       INTEGER,
			//	pi_CENTRALAUDITOR       IN       VARCHAR2  DEFAULT NULL
			//	, pi_callstarttime    in timestamp  DEFAULT NULL,
			//	pi_SIMID                IN       VARCHAR2,
			//	pi_CARDREADERID         IN       VARCHAR2,
			//	pi_CARDREADERREVISION   IN       VARCHAR2,
			//	pi_CARDREADER_FW_VERSION     IN       VARCHAR2,
			//	pi_cardreader_cfg_ver in      integer default null,
			//	po_CALLFLAG             OUT      INTEGER

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

			m_EventTrace.Event ( CEventTrace::Details, "CProtelHost::Database_UpdateCentralAuditor -->g_pAdoConnection->ExecuteNonQuery" );
			m_padoConnection->ExecuteNonQuery( adoStoredProcedure, false );
			m_EventTrace.Event ( CEventTrace::Details, "CProtelHost::Database_UpdateCentralAuditor <--g_pAdoConnection->ExecuteNonQuery" );

			vtCallFlag = adoStoredProcedure.GetParameter("po_CALLFLAG");
			protelCallFlag = (ProtelCallFlag)vtCallFlag.iVal;
		}
		catch ( _com_error &comError )
		{
			m_EventTrace.Event ( CEventTrace::Information, "CProtelHost::Database_UpdateCentralAuditor <--> ERROR: %s", CErrorMessage::ReturnComErrorMessage ( comError ));
		}
	}

	void ProcessApplicationLayerError ( BYTE ErrorCode )
	{
		ApplicationLayerError error = ( ApplicationLayerError ) ErrorCode;
	}

	void CleanupProtelDevices ( void )
	{
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
#if TRUE
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
#if TRUE
		__int64 i64DueTime;
		LARGE_INTEGER liDueTime;

		// Create a negative 64-bit integer that will be used to
		// signal the timer 10 seconds from now.

		i64DueTime = Seconds * -1;
		i64DueTime *= _SECOND;

		// Copy the relative time into a LARGE_INTEGER.
		liDueTime.LowPart  = (DWORD) ( i64DueTime & 0xFFFFFFFF );
		liDueTime.HighPart = (LONG)  ( i64DueTime >> 32 );
		BOOL bSuccess = SetWaitableTimer( hTimer, &liDueTime, 0, NULL, NULL, TRUE );
#else
		LARGE_INTEGER li = { 0 };
		SetWaitableTimer ( hTimer, &li, Seconds * 1000, NULL, NULL, FALSE);
#endif
	}

	virtual char* GetPort ( void )
	{
		return "?????";
	}

	virtual char* GetDevice ( void )
	{
		return "?????";
	}

	virtual int GetWaitSeconds ( void )
	{
		return 1;
	}

	virtual int GetRetryCount ( void )
	{
		return 1;
	}
};
