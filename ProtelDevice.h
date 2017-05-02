/**********************************************************************************************************************
 *                                      This file contains the CProtelDevice class.                                   *
 *                                                                                                                    *
 * This class represents an individual device that is controlled by a Master Auditor (ProtelHost).                    *
 *                                                                                                                    *
 *                                          Copyright (c) Protel Inc. 2009-2010                                       *
 **********************************************************************************************************************/
#pragma once
#include "ProtelHost.h"
#include "AuditDevice.h"
#include "AdoStoredProcedure.h"
#include "AdoRecordset.h"
#include "ProfileValues.h"

#define MAX_TRANSMIT    250
#define FIVEHUNDREDTWELVE	512		// added to fix crash problem when a config file is downloaded wjs-6/14/2011

class CProtelDevice 
 {
protected:
    char m_szCentralAuditor [ 32 ];        // serial number of the master auditor (e.g. AVW200) controlling this device
    double m_dCallStartTime;                                          // start time of connection to the master auditor
    int m_nCallNumber;                                               // call number of connection to the master auditor
    char m_szSerialNumber [ 32 ];                                                       // serial number of this device
    BYTE* m_pbFirmware;                              // points to entire firmware image (binary) obtained from database
    long m_nFirmwareLength;                                                                 // length of firmware image
    BYTE* m_pbConfiguration;                    // points to entire configuration image (binary) obtained from database
    long m_nConfigurationLength;                                                       // length of configuration image
	long new_m_nConfigurationLength;										// m_nConfigurationLength set to zero and I know not where
    bool m_bTransmitFirmware;                // set TRUE by GetFirmwareOrConfiguration if a firmware download is needed
    bool m_bTransmitConfiguration;      // set TRUE by GetFirmwareOrConfiguration if a configuration download is needed
    //int m_nTransmitFreeVend;
    int m_nFreeBeeeControllerflag;		// WJS 10///12////2010 flag >= 0 implies freebeedownload
    int m_nFreeBeeController;                                  // set by GetFreeBee but doesn't seem to do anything!!!!
    char m_szFreeBeeSerialNumber [ 32 ];                       // set by GetFreeBee but doesn't seem to do anything!!!!
    AuditDevice m_AuditDevice;                         // the device's status as read by the master using the N command
	char m_freeBeeAuditDeviceSN	[ 8 ];				// wjs sn of audit device with freebee to activate
	int	m_indxOfFBAuditDevice;					//wjs index of audit device needing free bee in cproteldevice array
    CAdoConnection* m_padoConnection;                                                            // database connection
    BYTE m_bTransmitBuffer [ FIVEHUNDREDTWELVE ];                  // holds chunk returned by GetNextFirmware or GetNextConfiguration
    long m_nCurrentConfigurationOffset;                                   // current position in configuration download
    long m_nCurrentFirmwareOffset;                                       // current position in firmware image download
    int m_nPacketNumber;                                        // number of last configuration or firmware packet sent
    bool m_bFirmwareDuplicate;           // TRUE if an earlier device controlled by the same host has the same firmware
    bool m_bConfigurationDuplicate;        // TRUE if an earlier device controlled by the same host has the same config
    BYTE m_bFirmwareDuplicates [ 256 ];                  // later devices controlled by the host with the same firmware
    BYTE m_bConfigurationDuplicates [ 256 ];               // later devices controlled by the host with the same config
    BYTE m_bNewConfiguration [ 256 ];               // WJS this config data is for xmit_config call 
    bool m_bFirmwareHasBeenDownloaded;       // TRUE if the device firmware has been updated (it needs reconfiguration)



public:
    CProtelDevice(CAdoConnection* padoConnection) :  m_padoConnection(padoConnection)
    {
        /**************************************************************************************************************
         * CONSTRUCTOR. This initialises the class members.                                                           *
         **************************************************************************************************************/
        m_bFirmwareHasBeenDownloaded = false;
        ZeroMemory ( m_szSerialNumber, sizeof ( m_szSerialNumber ));
        m_pbFirmware = NULL;                // pointer to firmware to be downloaded - set by GetFirmwareOrConfiguration
        m_nFirmwareLength = 0;                                                              // length of firmware image
        m_pbConfiguration = NULL;      // pointer to configuration to be downloaded - set by GetFirmwareOrConfiguration
        m_nConfigurationLength = 0;                                                     // length of configuration data
        m_bTransmitFirmware = false;
        m_bTransmitConfiguration = false;
        m_nFreeBeeeControllerflag = -1;					// default is no freebee download
        m_nFreeBeeController = -1;
        ZeroMemory ( m_szFreeBeeSerialNumber, sizeof ( m_szFreeBeeSerialNumber ));
        ZeroMemory ( m_bTransmitBuffer, sizeof ( m_bTransmitBuffer ));
        m_nCurrentConfigurationOffset = 0;
        m_nCurrentFirmwareOffset = 0;
        m_nPacketNumber = 0;
        m_bFirmwareDuplicate = false;
        m_bConfigurationDuplicate = false;
		ZeroMemory ( m_freeBeeAuditDeviceSN, sizeof ( m_freeBeeAuditDeviceSN ));		// wjs holds sn of audit device assigned to free bee 
        ZeroMemory ( m_bFirmwareDuplicates, sizeof ( m_bFirmwareDuplicates ));
        ZeroMemory ( m_bConfigurationDuplicates, sizeof ( m_bConfigurationDuplicates ));
        ZeroMemory ( m_bNewConfiguration, sizeof ( m_bNewConfiguration ));
    }
	virtual ~CProtelDevice(void)
    {
        /**************************************************************************************************************
         * DESTRUCTOR. This deletes any firmware or configuration data that may exist.                                *
         **************************************************************************************************************/
        if ( m_pbFirmware != NULL )
        {
            delete [] m_pbFirmware;
            m_pbFirmware = NULL;
        }
        if ( m_pbConfiguration != NULL )
        {
            delete [] m_pbConfiguration;
            m_pbConfiguration = NULL;
        }
//    _CrtDumpMemoryLeaks();
    }


	char* GetSerialNumber ( void )
    {
        /**************************************************************************************************************
         * This returns the serial number of the device.                                                              *
         **************************************************************************************************************/
        return m_szSerialNumber;
    }

    void SetCentralAuditor ( char* pszCentralAuditor )
    {
        /**************************************************************************************************************
         * ProtelHost uses this when processing the I command response to set the serial number of the central        *
         * auditor (e.g. AVW200) controlling this device.                                                             *
         **************************************************************************************************************/
        StringCbCopy ( m_szCentralAuditor, sizeof ( m_szCentralAuditor ), pszCentralAuditor );
    }

    void SetCallStartTime ( double CallStartTime )
    {
        /**************************************************************************************************************
         * ProtelHost uses this when processing the I command response to set the call start time for this device.    *
         **************************************************************************************************************/
        m_dCallStartTime = CallStartTime;
    }

    void SetCallNumber ( int CallNumber )
    {
        /**************************************************************************************************************
         * ProtelHost uses this when processing the I command response to set the call number for this device.        *
         **************************************************************************************************************/
        m_nCallNumber = CallNumber;
    }

    int GetCallNumber ( void )
    {
        /**************************************************************************************************************
         * ProtelHost uses this when processing the V command response to get the call number for this device.        *
         **************************************************************************************************************/
        return m_nCallNumber;
    }


    BYTE* GetAuditDevice ( void )
	{
		return	( BYTE* ) &m_AuditDevice;
	}

    bool SetAuditDevice ( BYTE* pAuditDevice )
    {
        /**************************************************************************************************************
         * ProtelHost uses this when processing the N command response. It places data relating to this device in     *
         * m_AuditDevice, extracts the serial number of the device and records details of the device in the           *
         * COMM_SERVER_AUDITORS database table.                                                                       *
         **************************************************************************************************************/
        MoveMemory ( &m_AuditDevice, pAuditDevice, sizeof ( m_AuditDevice ));
        ZeroMemory ( m_szSerialNumber, sizeof ( m_szSerialNumber ));
        MoveMemory ( m_szSerialNumber, m_AuditDevice.szSerialNumber, sizeof ( m_AuditDevice.szSerialNumber ));


        char chArray [ 64 ];

        try
        {
            CAdoStoredProcedure adoStoredProcedure ( "PKG_COMM_SERVER.AUDITORS" );
#if 0
            PROCEDURE AUDITORS (
                pi_callnumber            in    number,
                centralauditor             IN   VARCHAR2  DEFAULT NULL,
                callstarttime              IN   TIMESTAMP  DEFAULT NULL,
                auditor                    IN   VARCHAR2,
                address                    IN   integer,
                firmware_version           IN   VARCHAR2,
                firmware_monitor           IN   VARCHAR2,
                firmware_version_rev       IN   VARCHAR2,
                config_file_version        IN   VARCHAR2,
                connection_type            IN   integer,
                status                     IN   INTEGER,
                remote_central             IN   INTEGER,
                display_unit               IN   INTEGER,
                needs_configuration_file   IN   INTEGER
                );
#endif

            _variant_t vtCallNumber (( long ) m_nCallNumber, VT_I4 );
            adoStoredProcedure.AddParameter( "pi_callnumber", vtCallNumber, ADODB::DataTypeEnum::adInteger, ADODB::ParameterDirectionEnum::adParamInput, sizeof ( long ));

            //centralauditor            IN  VARCHAR2,
            _bstr_t bstrCentralAuditor( m_szCentralAuditor );
            _variant_t vtCentralAuditor ( bstrCentralAuditor );
            adoStoredProcedure.AddParameter( "centralauditor", vtCentralAuditor, ADODB::DataTypeEnum::adBSTR, ADODB::ParameterDirectionEnum::adParamInput, bstrCentralAuditor.length());

            //callstarttime             IN  TIMESTAMP,
            _variant_t vtCallStartTime ( m_dCallStartTime, VT_DATE );
            adoStoredProcedure.AddParameter( "callstarttime", vtCallStartTime, ADODB::DataTypeEnum::adDate, ADODB::ParameterDirectionEnum::adParamInput, sizeof ( double ));

            //auditor                   IN  VARCHAR2,
            _bstr_t bstrAuditor( m_szSerialNumber );
            _variant_t vtAuditor ( bstrAuditor );
            adoStoredProcedure.AddParameter( "auditor", vtAuditor, ADODB::DataTypeEnum::adBSTR, ADODB::ParameterDirectionEnum::adParamInput, bstrAuditor.length());

            //address                   IN  SMALLINT,
            _variant_t vtAddress (( BYTE ) m_AuditDevice.Address );
            adoStoredProcedure.AddParameter( "address", vtAddress, ADODB::DataTypeEnum::adSmallInt, ADODB::ParameterDirectionEnum::adParamInput, sizeof ( m_AuditDevice.Address ));

            //firmware_version          IN  VARCHAR2,
            ZeroMemory ( chArray, sizeof ( chArray ));
            CopyMemory ( chArray, &m_AuditDevice.FirmwareVersionLevel, sizeof ( m_AuditDevice.FirmwareVersionLevel ));
            _bstr_t bstrFirmwareVersionLevel( chArray );
            _variant_t vtFirmwareVersionLevel ( bstrFirmwareVersionLevel );
            adoStoredProcedure.AddParameter( "firmware_version", vtFirmwareVersionLevel, ADODB::DataTypeEnum::adBSTR, ADODB::ParameterDirectionEnum::adParamInput, bstrFirmwareVersionLevel.length());

            //firmware_monitor          IN  VARCHAR2,
            ZeroMemory ( chArray, sizeof ( chArray ));
            CopyMemory ( chArray, &m_AuditDevice.FirmwareMonitorType, sizeof ( m_AuditDevice.FirmwareMonitorType ));
            _bstr_t bstrFirmwareMonitorType( chArray );
            _variant_t vtFirmwareMonitorType ( bstrFirmwareMonitorType );
            adoStoredProcedure.AddParameter( "firmware_monitor", vtFirmwareMonitorType, ADODB::DataTypeEnum::adBSTR, ADODB::ParameterDirectionEnum::adParamInput, bstrFirmwareMonitorType.length());

            //firmware_version_rev      IN  VARCHAR2,
            ZeroMemory ( chArray, sizeof ( chArray ));
            CopyMemory ( chArray, &m_AuditDevice.FirmwareVersionRev, sizeof ( m_AuditDevice.FirmwareVersionRev ));
            _bstr_t bstrFirmwareVersionRev( chArray );
            _variant_t vtFirmwareVersionRev ( bstrFirmwareVersionRev );
            adoStoredProcedure.AddParameter( "firmware_version_rev", vtFirmwareVersionRev, ADODB::DataTypeEnum::adBSTR, ADODB::ParameterDirectionEnum::adParamInput, bstrFirmwareVersionRev.length());

            //config_file_version       IN  VARCHAR2,
            ZeroMemory ( chArray, sizeof ( chArray ));
            CopyMemory ( chArray, &m_AuditDevice.ConfigFileVersion, sizeof ( m_AuditDevice.ConfigFileVersion ));
            _bstr_t bstrConfigFileVersion( chArray );
            _variant_t vtConfigFileVersion ( bstrConfigFileVersion );
            adoStoredProcedure.AddParameter( "config_file_version", vtConfigFileVersion, ADODB::DataTypeEnum::adBSTR, ADODB::ParameterDirectionEnum::adParamInput, bstrConfigFileVersion.length());

            //connection_type           IN  integer,
            _variant_t vtConnectionType (( BYTE ) m_AuditDevice.ConnectionType );
            adoStoredProcedure.AddParameter( "connection_type", vtConnectionType, ADODB::DataTypeEnum::adSmallInt, ADODB::ParameterDirectionEnum::adParamInput, sizeof ( m_AuditDevice.ConnectionType ));

            //status                    IN  INTEGER,
            _variant_t vtStatusByte (( BYTE ) m_AuditDevice.StatusByte );
            adoStoredProcedure.AddParameter( "status", vtStatusByte, ADODB::DataTypeEnum::adSmallInt, ADODB::ParameterDirectionEnum::adParamInput, sizeof ( m_AuditDevice.StatusByte ));

            //remote_central            IN  INTEGER,
            BYTE RemoteCentral = 0;
            if (( m_AuditDevice.StatusByte & 0x01 ) != 0 )
            {
                RemoteCentral = 1;
            }
            _variant_t vtRemoteCentral (( BYTE ) RemoteCentral );
            adoStoredProcedure.AddParameter( "remote_central", vtRemoteCentral, ADODB::DataTypeEnum::adSmallInt, ADODB::ParameterDirectionEnum::adParamInput, sizeof ( RemoteCentral ));

            //display_unit              IN  INTEGER,
            BYTE DisplayUnit = 0;
            if (( m_AuditDevice.StatusByte & 0x02 ) != 0 )
            {
                DisplayUnit = 1;
            }
            _variant_t vtDisplayUnit (( BYTE  ) DisplayUnit );
            adoStoredProcedure.AddParameter( "display_unit", vtDisplayUnit, ADODB::DataTypeEnum::adSmallInt, ADODB::ParameterDirectionEnum::adParamInput, sizeof ( DisplayUnit ));

            //needs_configuration_file  IN  INTEGER
            BYTE NeedsConfigurationFile = 0;
            if (( m_AuditDevice.StatusByte & 0x80 ) != 0 )
            {
                NeedsConfigurationFile = 1;
            }
            _variant_t vtNeedsConfigurationFile (( BYTE  ) NeedsConfigurationFile );
            adoStoredProcedure.AddParameter( "needs_configuration_file", vtNeedsConfigurationFile, ADODB::DataTypeEnum::adSmallInt, ADODB::ParameterDirectionEnum::adParamInput, sizeof ( NeedsConfigurationFile ));

            //eventTrace.Event ( CEventTrace::Details, "CProtelHost::SetAuditDevice -->g_pAdoConnection->ExecuteNonQuery" );
			int checkit = -1;

            checkit = m_padoConnection->ExecuteNonQuery( adoStoredProcedure, false );                                    // do it
			if (checkit <= 0 )
			{
//		PKG_COMM_SERVER.addSysLogRecAutonomous ( pi_text varchar2 )
				return false;
			}
			return true;
            //eventTrace.Event ( CEventTrace::Details, "CProtelHost::SetAuditDevice <--g_pAdoConnection->ExecuteNonQuery" );
        }
        catch ( _com_error &comError )
        {
            CEventTrace eventTrace;                                                // records events - see EventTrace.h
            eventTrace.Event ( CEventTrace::Information, "CProtelDevice::SetAuditDevice <--> ERROR: %s", CErrorMessage::ReturnComErrorMessage ( comError ));
			return false;
			//CloseDevice(0);		// call close device with 0 => failed call
        }
    }

    BYTE GetAddress ( void )
    {
        /**************************************************************************************************************
         * This returns the address of the device (i.e. the address that the master auditor uses to communicate with  *
         * it).                                                                                                       *
         **************************************************************************************************************/
        return m_AuditDevice.Address;
    }

    bool GetNeedsConfiguration ( void )
    {
        /**************************************************************************************************************
         * This returns TRUE if the status byte of the device indicates that it needs configuration.                  *
         **************************************************************************************************************/
        // Bit 7 =1 = Auditor Needs Configuration file
		// wjs 4/18/2011 Need to accomodate config download when a firmware download has taken
		// place
        if (( m_AuditDevice.StatusByte & 0x80 ) == 0x80 )
        {
            return true;
        }
        return false;
    }

    bool GetNeedsFirmware ( void )
    {
        /**************************************************************************************************************
         * This returns TRUE if m_bTransmitFirmware indicates that it needs Firmware.                  *
         **************************************************************************************************************/
		// wjs 4/18/2011 Need to accomodate config download when a firmware download has taken
		// place
        return m_bTransmitFirmware;
    }
	bool GetHasDownloadFirmware (void )
	{
        /**************************************************************************************************************
         * This returns TRUE if m_bFirmwareHasBeenDownloaded indicates that Firmware has been downloaded.                  *
         **************************************************************************************************************/
        return m_bFirmwareHasBeenDownloaded;
	}

	int GetFreeBeeControllerflag ( void  )
    {
        return m_nFreeBeeeControllerflag;		// changed from -1 to 'm_nFreeBeeeController' because freebee not downloading wjs 10-07-2010
    }											// m_nFreeBeeeControllerflag set to -1 or controler id

	int GetFreeBeeController ( void  )
    {
        return m_nFreeBeeeControllerflag;		// changed from -1 to 'm_nFreeBeeeController' because freebee not downloading wjs 10-07-2010
    }

    char* GetFreeBeeSerialNumber ( void  )
    {
		return m_szFreeBeeSerialNumber;	// changed from NULL to 'm_szFreeBeeSerialNumber' because freebee not downloading wjs 10-07-2010
    }

    __int64 GetFirmwareChecksum ( void )
    {
        /**************************************************************************************************************
         * This returns the checksum of firmware waiting to be downloaded to the device. (ProtelHost uses this when   *
         * looking for devices controlled by the same master auditor which are due to receive identical firmware.)    *
         **************************************************************************************************************/
        __int64 Total = 0;

        for ( long Offset = 0; Offset < m_nFirmwareLength; Offset++ )
        {
            Total += (BYTE)*( m_pbFirmware + Offset );
        }
        return Total;
    }

    __int64 GetConfigurationChecksum ( void )
    {
        /**************************************************************************************************************
         * This returns the checksum of a configuration waiting to be downloaded to the device. (ProtelHost uses this *
         * when looking for devices controlled by the same master auditor which are due to receive an identical       *
         * configuration.)                                                                                            *
         **************************************************************************************************************/
        __int64 Total = 0;

        for ( long Offset = 0; Offset < m_nConfigurationLength; Offset++ )
        {
            Total += (BYTE)*( m_pbConfiguration + Offset );
        }
        return Total;
    }

    long GetFirmwareLength ( void )
    {
        /**************************************************************************************************************
         * This returns the length in bytes of firmware waiting to be downloaded to the device.                       *
         **************************************************************************************************************/
        return m_nFirmwareLength;
    }

    long GetConfigurationLength ( void )
    {
        /**************************************************************************************************************
         * This returns the length in bytes of a configuration waiting to be downloaded to the device.                *
         **************************************************************************************************************/
        return m_nConfigurationLength;
    }

    void SetNewConfigurationLength ( long new_value )
    {
        /**************************************************************************************************************
         * This sets the length in bytes of a configuration waiting to be downloaded to the device.                *
         **************************************************************************************************************/
        new_m_nConfigurationLength = new_value;
    }

    long GetNewConfigurationLength ( void )
    {
        /**************************************************************************************************************
         * This sets the length in bytes of a configuration waiting to be downloaded to the device.                *
         **************************************************************************************************************/
        return new_m_nConfigurationLength;
    }

	void SetConfigurationDuplicate ( bool bDuplicate )
    {
        /**************************************************************************************************************
         * ProtelHost uses this to record that this device is to receive its configuration as a duplicate of one sent *
         * to an earlier device.                                                                                      *
         **************************************************************************************************************/
        m_bConfigurationDuplicate = bDuplicate;
    }

    bool GetConfigurationDuplicate ( void )
    {
        /**************************************************************************************************************
         * This returns TRUE if the device is to receive or has received its configuration as a duplicate of one sent *
         * to an earlier device.                                                                                      *
         **************************************************************************************************************/
        return m_bConfigurationDuplicate;
    }

    void AddConfigurationDuplicate ( BYTE DuplicateAddress )
    {
        /**************************************************************************************************************
         * ProtelHost uses this to add the address of a later device that is to receive an identical configuration to *
         * this one. (GetNextDownload prepends a list of these addresses to the configuration.)                       *
         **************************************************************************************************************/
        BYTE Offset = m_bConfigurationDuplicates[ 0 ];
        m_bConfigurationDuplicates[ Offset + 1 ] = DuplicateAddress;
        m_bConfigurationDuplicates[ 0 ] = Offset + 1;
    }
	
	void setNewConfig ( BYTE * config_dat, long len_config )
	{
		memcpy(&m_bNewConfiguration, config_dat, len_config);
	}

	BYTE* getNewConfig ( void )
	{
		return (BYTE*) &m_bNewConfiguration;
	}

    void SetFirmwareDuplicate ( bool bDuplicate )
    {
        /**************************************************************************************************************
         * ProtelHost uses this to record that this device is to receive firmware as a duplicate of that sent to an   *
         * earlier device.                                                                                            *
         **************************************************************************************************************/
        m_bFirmwareDuplicate = bDuplicate;
    }

    bool GetFirmwareDuplicate ( void )
    {
        /**************************************************************************************************************
         * This returns TRUE if the device is to receive or has received firmware as a duplicate of that sent to an   *
         * earlier device.                                                                                            *
         **************************************************************************************************************/
        return m_bFirmwareDuplicate;
    }

    void AddFirmwareDuplicate ( BYTE DuplicateAddress )
    {
        /**************************************************************************************************************
         * ProtelHost uses this to add the address of a later device that is to receive identical firmware to this    *
         * one. (GetNextDownload prepends a list of these addresses to the configuration.)                            *
         **************************************************************************************************************/
        BYTE Offset = m_bFirmwareDuplicates[ 0 ];
        m_bFirmwareDuplicates[ Offset + 1 ] = DuplicateAddress;
        m_bFirmwareDuplicates[ 0 ] = Offset + 1;
    }

    void DontDownloadFirmware ( void )
    {
        /**************************************************************************************************************
         * ProtelHost uses this to delete any pending firmware download to the device if there is an earlier device   *
         * requiring a download of different firmware. (Only one firmware download is allowed during a connection.)   *
         **************************************************************************************************************/
        m_bTransmitFirmware = false;
        m_nFirmwareLength = 0;
        if ( m_pbFirmware != NULL )
        {
            delete [] m_pbFirmware;
            m_pbFirmware = NULL;
        }
    }

    // This allows the class to be used like a C# class that has properties
    __declspec(property(get = GetSerialNumber)) char* SerialNumber;
    __declspec(property(get = GetAuditDevice,put = SetAuditDevice)) BYTE* AuditDevice;
    __declspec(property(get = GetAddress)) BYTE Address;
    __declspec(property(get = GetNeedsConfiguration)) bool NeedsConfiguration;
    __declspec(property(get = GetNeedsFirmware)) bool NeedsFirmware;
    __declspec(property(get = GetHasDownloadFirmware)) bool HasDownloadFirmware;
    __declspec(property(get = GetFreeBeeController)) int FreeBeeController;
    __declspec(property(get = GetFreeBeeControllerflag)) int FreeBeeControllerflag;
    __declspec(property(get = GetFreeBeeSerialNumber,put = SetFreeBeeSerialNumber)) char* FreeBeeSerialNumber;
    __declspec(property(put = SetCentralAuditor)) char* CentralAuditor;
    __declspec(property(put = SetCallStartTime)) double CallStartTime;
    __declspec(property(get = GetCallNumber, put = SetCallNumber)) int CallNumber;
    __declspec(property(get = GetFirmwareChecksum)) __int64 FirmwareChecksum;
    __declspec(property(get = GetConfigurationChecksum)) __int64 ConfigurationChecksum;
    __declspec(property(get = GetIndexFreeBeeAuditDeviceidx,put = SetIndexFreeBeeAuditDeviceidx)) int FreeBeeAuditDeviceidx;
    __declspec(property(get = GetFirmwareLength)) long FirmwareLength;
    __declspec(property(get = GetConfigurationLength)) long ConfigurationLength;
    __declspec(property(get = GetNewConfigurationLength, put = SetNewConfigurationLength)) long NewConfigurationLength;
    __declspec(property(get = GetConfigurationDuplicate,put = SetConfigurationDuplicate)) bool ConfigurationDuplicate;
    __declspec(property(get = getNewConfig,put = setNewConfig)) BYTE* NewConfiguration;
    __declspec(property(get = GetFirmwareDuplicate,put = SetFirmwareDuplicate)) bool FirmwareDuplicate;


	int GetIndexFreeBeeAuditDeviceidx ( void )
	{
		return m_indxOfFBAuditDevice;
	}
	void SetIndexFreeBeeAuditDeviceidx (int indxFBAD )
	{
		m_indxOfFBAuditDevice = indxFBAD;
	}
	
	void GetConfiguration ( void )
    {
        /**************************************************************************************************************
         * This is called from ProtelHost for each device to check if it needs reconfiguration.                       *
         **************************************************************************************************************/
        GetFirmwareOrConfiguration( true );
    }

    void GetSecondConfiguration ( void )
    {
        /**************************************************************************************************************
         * This is called from ProtelHost for each device after new firmware has been downloaded. It checks if the    *
         * device received the firmware (including as a duplicate) and, if so, may mark to reconfigure it.            *
         **************************************************************************************************************/
        m_bTransmitConfiguration = false;
        ZeroMemory ( m_bConfigurationDuplicates, sizeof ( m_bConfigurationDuplicates ));
        if ( m_bFirmwareHasBeenDownloaded == true || m_bFirmwareDuplicate == true )
        {
            GetFirmwareOrConfiguration( true );                                                // needs reconfiguration
        }
    }

    void GetFirmware ( void )
    {
        /**************************************************************************************************************
         * This is called from ProtelHost for each device to check if it needs new firmware.                          *
         **************************************************************************************************************/
        GetFirmwareOrConfiguration( false );
    }


    int GetFreeBee ( void )
    {
        /**************************************************************************************************************
         * This sets m_nFreeBeeController (target_controller) and m_szFreeBeeSerialNumber (freebeeid) from            *
         * MACHINES_CS_TAB and FREEBEE_TAB2 or perhaps FREEBEE_MV database tables where monitorid equals our serial   *
         * number.                                                                                                    *
         **************************************************************************************************************/
        CAdoStoredProcedure getFreeBeeAssignment ( "PKG_COMM_SERVER.getFreeBeeAssignment2" );
        //procedure getFreeBeeAssignment (							getFreeBeeAssignment
                    //pi_callnumber in integer
                    //, pi_centralAuditor in varchar2 default null	CentralAuditor
                    //, pi_callstarttime in timestamp default null
                    //, pi_auditor in varchar2
                    //, po_ctrl_number out integer                    
                    //, po_freebeeid out varchar2	
                    //, po_perform_cmd  out integer --boolean, 0=FALSE/DO_NOT_TRY, <>0=Proceed with command
        //);



        _variant_t vtCallNumber (( long ) CallNumber, VT_I4 );
        getFreeBeeAssignment.AddParameter( "pi_callnumber", vtCallNumber, ADODB::DataTypeEnum::adInteger, ADODB::ParameterDirectionEnum::adParamInput, sizeof ( long ));

        _bstr_t bstrCentralAuditor( m_szCentralAuditor );
        _variant_t vtCentralAuditor ( bstrCentralAuditor );
        getFreeBeeAssignment.AddParameter( "pi_centralAuditor", vtCentralAuditor, ADODB::DataTypeEnum::adBSTR, ADODB::ParameterDirectionEnum::adParamInput, bstrCentralAuditor.length());

        _variant_t vtCallStartTime ( m_dCallStartTime, VT_DATE );
        getFreeBeeAssignment.AddParameter( "pi_callstarttime", vtCallStartTime, ADODB::DataTypeEnum::adDate, ADODB::ParameterDirectionEnum::adParamInput, sizeof ( double ));

        _bstr_t bstrSerialNumber( SerialNumber );
        _variant_t vtAuditor ( bstrSerialNumber );
        getFreeBeeAssignment.AddParameter( "pi_auditor", vtAuditor, ADODB::DataTypeEnum::adBSTR, ADODB::ParameterDirectionEnum::adParamInput, bstrSerialNumber.length());

        _variant_t vtControllerNumber (( long ) 0 );
        getFreeBeeAssignment.AddParameter( "po_ctrl_number", vtControllerNumber, ADODB::adInteger, ADODB::adParamInputOutput, sizeof ( long ));

        _variant_t vtFreeBeeID ( _bstr_t ( "" ));
        getFreeBeeAssignment.AddParameter("po_freebeeid", vtFreeBeeID, ADODB::adBSTR, ADODB::adParamOutput, 64 );

        _variant_t vtIsAssigned (( long ) 0 );
        getFreeBeeAssignment.AddParameter( "po_perform_cmd", vtIsAssigned, ADODB::adInteger, ADODB::adParamInputOutput, sizeof ( long ));

		// try catch goes here
        m_padoConnection->ExecuteNonQuery( getFreeBeeAssignment, false );         // do query

        ZeroMemory ( m_szFreeBeeSerialNumber, sizeof ( m_szFreeBeeSerialNumber ));
        vtIsAssigned = getFreeBeeAssignment.GetParameter("po_perform_cmd");
        if (( long ) vtIsAssigned == 0 )
        {
            m_nFreeBeeeControllerflag  = -1;	// no freebee download
        }
        else
        {
            vtControllerNumber = getFreeBeeAssignment.GetParameter("po_ctrl_number");
            vtFreeBeeID = getFreeBeeAssignment.GetParameter("po_freebeeid");
            m_nFreeBeeeControllerflag = ( long ) vtControllerNumber;
            _bstr_t bstrFreeBeeID (( _bstr_t ) vtFreeBeeID );
            StringCbCopy ( m_szFreeBeeSerialNumber, sizeof ( m_szFreeBeeSerialNumber ), ( const char* ) bstrFreeBeeID );
        }
		return m_nFreeBeeeControllerflag;
    }

    BYTE* GetNextFirmware ( long& FirmwareLength )
    {
        /**************************************************************************************************************
         * This returns the next chunk of the firmware download for the device and sets ConfigurationLength to its    *
         * length (it is don't care on entry). If/when there is nothing to send, it returns NULL.                     *
         *                                                                                                            *
         * NOTE: Once the download is complete, it returns NULL after using the FIRMWARE_XMIT database procedure to   *
         * clear PENDING_FW in MACHINES_PENDING_DWNLD_TAB and set SUCCESS in COMM_SERVER_FW_DOWNLOADS.                *
         **************************************************************************************************************/
        if ( m_bTransmitFirmware == true )
        {
            BYTE* pByte = GetNextDownload ( m_pbFirmware, m_nCurrentFirmwareOffset, m_nFirmwareLength, m_bFirmwareDuplicate, m_bFirmwareDuplicates, "PKG_COMM_SERVER.FIRMWARE_XMIT", FirmwareLength );
            if ( pByte == NULL )
            {
                // Tell the Database to download the configuration for the newly downloaded firmware!
                m_bFirmwareHasBeenDownloaded = true;
            }
            return pByte;
        }
        return NULL;
    }

    BYTE* GetNextConfiguration ( long& ConfigurationLength )
    {
        /**************************************************************************************************************
         * This returns the next chunk of the configuration download for the device and sets ConfigurationLength to   *
         * its length (it is don't care on entry). If/when there is nothing to send, it returns NULL.                 *
         *                                                                                                            *
         * NOTE: Once the download is complete, it returns NULL after using the CONFIG_XMIT database procedure to set *
         * SUCCESS in COMM_SERVER_DOWNLOADS but this DOESN'T clear PENDING_CR in MACHINES_PENDING_DWNLD_TAB!!!!       *
         **************************************************************************************************************/
        //if ( m_bTransmitConfiguration == true || m_bFirmwareHasBeenDownloaded == true )
        //{
			//if ( m_bFirmwareHasBeenDownloaded == true )		
			//{
			//	m_nCurrentConfigurationOffset = -1;		// needed to force a download iff fw download happened
			//}
            return GetNextDownload ( m_pbConfiguration, m_nCurrentConfigurationOffset, m_nConfigurationLength, m_bConfigurationDuplicate, m_bConfigurationDuplicates, "PKG_COMM_SERVER.CONFIG_XMIT", ConfigurationLength );
        //}
        //return NULL;
    }

private:

    BYTE* GetNextDownload ( BYTE* pBuffer, long& nCurrentOffset, long& nDownloadLength, bool bDuplicate, BYTE bDuplicates[], char* pszStoredProcedure, long& ConfigurationLength )
    {
        /**************************************************************************************************************
         * This gets the next packet of a configuration or firmware download. If it is called with bDuplicate TRUE or *
         * after the download is complete, it saves details in the database. Parameters are:                          *
         *                                                                                                            *
         *  pBuffer - pointer to binary image of entire configuration or firmware to be downloaded                    *
         *  nCurrentOffset - current download position in pBuffer                                                     *
         *  nDownloadLength - total length of image in pBuffer                                                        *
         *  bDuplicate - TRUE if the same config or firmware was already downloaded to an earlier device              *
         *  bDuplicates - addresses of devices controlled by the same host to receive the firmware or config          *
         *  pszStoredProcedure - to access database when recording completion of download                             *
         *  ConfigurationLength - (don't care on entry) length of returned packet                                     *
         *                                                                                                            *
         * Return value is m_bTransmitBuffer (holding the packet) or NULL if there is nothing to send.                *
         * ConfigurationLength is set and nCurrentOffset advanced on return - other parameters appear to be           *
         * unchanged.                                                                                                 *
         **************************************************************************************************************/
        //ConfigurationLength = ( nDownloadLength - nCurrentOffset );//MAX_TRANSMIT;
        //if  ( ConfigurationLength > MAX_TRANSMIT )
        //{
        //  ConfigurationLength = MAX_TRANSMIT;
        //}

        if ( bDuplicate || nCurrentOffset >= nDownloadLength )
        {
			//nDownloadLength = GetNewConfigurationLength();
                    CEventTrace eventTrace;
                    eventTrace.BeginXML ( CEventTrace::Details );
                    eventTrace.XML ( CEventTrace::Details, "CProtelDevice::1GetNextDownload", pszStoredProcedure );
                    eventTrace.XML ( CEventTrace::Details, "Transmit", "Completed" );
                    eventTrace.EndXML ( CEventTrace::Details );
            if ( bDuplicate || nDownloadLength > 0 )
            {
                /*
                 * We have finished sending the configuration or firmware or the device is a duplicate device (where
                 * we will have sent the configuration or firmware to it along with an earlier device). We update the
                 * database (see comments for GetNextFirmware or GetNextConfiguration).
                 */
                try
                {
                    //pi_callnumber            in    number,
                    //pi_CENTRALAUDITOR   IN   VARCHAR2 default null,
                    //pi_CALLSTARTTIME    IN   TIMESTAMP default null,
                    //pi_AUDITOR          IN   VARCHAR2,
                    //CEventTrace eventTrace;
                    //eventTrace.BeginXML ( CEventTrace::Details );
                    //eventTrace.XML ( CEventTrace::Details, "CProtelDevice::2GetNextDownload", pszStoredProcedure );
                    //eventTrace.XML ( CEventTrace::Details, "Transmit", "Completed" );
                    //eventTrace.EndXML ( CEventTrace::Details );
                    CAdoStoredProcedure adoStoredProcedure ( pszStoredProcedure );

                    _variant_t vtCallNumber (( long ) m_nCallNumber, VT_I4 );
					long temcn = vtCallNumber.operator long();
                    adoStoredProcedure.AddParameter( "pi_CALLNUMBER", vtCallNumber, ADODB::DataTypeEnum::adInteger, ADODB::ParameterDirectionEnum::adParamInput, sizeof ( long ));

					//eventTrace.BeginXML ( CEventTrace::Details );
     //               eventTrace.XML ( CEventTrace::Details, "CProtelDevice::2.5GetNextDownload", pszStoredProcedure );
					//CEventTrace m_EventTrace;                                                      // records events - see EventTrace.h
					//m_EventTrace.Event( CEventTrace::Information, "void CProtelDevice::0GetNextDownload vtCallNumber [%d](%d)",temcn,1);
					//eventTrace.XML ( CEventTrace::Details, "vtCallNumber []", "test1");
     //               eventTrace.XML ( CEventTrace::Details, "Transmit", "Completed" );
     //               eventTrace.EndXML ( CEventTrace::Details );
			
                    _bstr_t bstrCentralAuditor( m_szCentralAuditor );
                    _variant_t vtCentralAuditor ( bstrCentralAuditor );
                    adoStoredProcedure.AddParameter( "pi_CENTRALAUDITOR", vtCentralAuditor, ADODB::DataTypeEnum::adBSTR, ADODB::ParameterDirectionEnum::adParamInput, bstrCentralAuditor.length());

					char * temca = vtCentralAuditor.operator _bstr_t();


     //               eventTrace.BeginXML ( CEventTrace::Details );
     //               eventTrace.XML ( CEventTrace::Details, "CProtelDevice::2.5GetNextDownload", pszStoredProcedure );
					//m_EventTrace.Event( CEventTrace::Information, "void CProtelDevice::0GetNextDownload vtCentralAuditor [%10s](%d)",m_szCentralAuditor,1);
					//eventTrace.XML ( CEventTrace::Details, "vtCentralAuditor [ ]", "test1" );
     //               eventTrace.XML ( CEventTrace::Details, "Transmit", "Completed" );
     //               eventTrace.EndXML ( CEventTrace::Details );
                    _variant_t vtCallStartTime ( m_dCallStartTime, VT_DATE );
                    adoStoredProcedure.AddParameter( "pi_CALLSTARTTIME", vtCallStartTime, ADODB::DataTypeEnum::adDate, ADODB::ParameterDirectionEnum::adParamInput, sizeof ( double ));

					double temct = vtCallStartTime.operator double();
     //               eventTrace.BeginXML ( CEventTrace::Details );
     //               eventTrace.XML ( CEventTrace::Details, "CProtelDevice::2.5GetNextDownload", pszStoredProcedure );
					//m_EventTrace.Event( CEventTrace::Information, "void CProtelDevice::0GetNextDownload vtCallStartTime [%c](%d)",m_dCallStartTime,1);
					//m_EventTrace.Event( CEventTrace::Information, "void CProtelDevice::0.0GetNextDownload vtCallStartTime [%c](%d)",temct,1);
					//eventTrace.XML ( CEventTrace::Details, "vtCallStartTime [  ]", "test2" );
     //               eventTrace.XML ( CEventTrace::Details, "Transmit", "Completed" );
     //               eventTrace.EndXML ( CEventTrace::Details );
                    _bstr_t bstrAuditor( m_szSerialNumber );
                    _variant_t vtAuditor ( bstrAuditor );
                    adoStoredProcedure.AddParameter( "pi_AUDITOR", vtAuditor, ADODB::DataTypeEnum::adBSTR, ADODB::ParameterDirectionEnum::adParamInput, bstrAuditor.length());

					//if (pBuffer = NULL)
					//	return NULL;
                    //variantBlob vtPayload ( pBuffer, sizeof(&pBuffer) ); // save downloaded data back to the database - ridiculous!!!!
     //               eventTrace.BeginXML ( CEventTrace::Details );
     //               eventTrace.XML ( CEventTrace::Details, "CProtelDevice::2.7GetNextDownload", pszStoredProcedure );
					//m_EventTrace.Event( CEventTrace::Information, "void CProtelDevice::0GetNextDownload vtAuditor [%s](%d)",m_szSerialNumber,1);
					//eventTrace.XML ( CEventTrace::Details, "vtAuditor [  ]", "test1" );
     //               eventTrace.XML ( CEventTrace::Details, "Transmit", "Completed" );
     //               eventTrace.EndXML ( CEventTrace::Details );


                    variantBlob vtPayload ( m_bTransmitBuffer, FIVEHUNDREDTWELVE ); // save downloaded data back to the database - ridiculous!!!!
                    adoStoredProcedure.AddParameter( "pi_DOWNLOAD_DATA", vtPayload, ADODB::DataTypeEnum::adVarBinary, ADODB::ParameterDirectionEnum::adParamInput, FIVEHUNDREDTWELVE );

					//BYTE* getit = getNewConfig();
					//variantBlob vtPayload ( getit, m_nConfigurationLength ); // save downloaded config back to the database - ridiculous!!!!
					//adoStoredProcedure.AddParameter( "pi_DOWNLOAD_DATA", vtPayload, ADODB::DataTypeEnum::adVarBinary, ADODB::ParameterDirectionEnum::adParamInput, m_nConfigurationLength );

                    //char szLength [ 32 ];
                    //_ltoa_s( nDownloadLength, szLength, sizeof ( szLength ), 10 );
                    //CEventTrace eventTrace;
     //               eventTrace.BeginXML ( CEventTrace::Details );
     //               eventTrace.XML ( CEventTrace::Details, "CProtelDevice::2.8GetNextDownload", pszStoredProcedure );
					//eventTrace.XML ( CEventTrace::Details, "vtPayload", "test1");
					//m_EventTrace.Event( CEventTrace::Information, "void CProtelDevice::0GetNextDownload sizeof ( m_bTransmitBuffer ) [%d](%d)",sizeof ( m_bTransmitBuffer ),1);
     //               eventTrace.XML ( CEventTrace::Details, "Transmit", "Completed" );
     //               eventTrace.EndXML ( CEventTrace::Details );
                    m_padoConnection->ExecuteNonQuery( adoStoredProcedure, false, true );
                    eventTrace.BeginXML ( CEventTrace::Details );
                    eventTrace.XML ( CEventTrace::Details, "CProtelDevice::3GetNextDownload", pszStoredProcedure );
                    eventTrace.XML ( CEventTrace::Details, "Transmit", "Completed" );
                    eventTrace.EndXML ( CEventTrace::Details );
                }
                catch ( _com_error &comError )
                {
                    CEventTrace eventTrace;                                        // records events - see EventTrace.h
                    eventTrace.Event ( CEventTrace::Information, "CProtelDevice::GetNextDownload <--> ERROR: [%s]", CErrorMessage::ReturnComErrorMessage ( comError ));
                }
            }
            ConfigurationLength = 0;
            return NULL;
        }
#if 0
        bool bLastPacket = false;
        int CopyLength = ConfigurationLength - 2;
        if (( nCurrentOffset + CopyLength ) > nDownloadLength )
        {
            CopyLength = nDownloadLength - nCurrentOffset;
            bLastPacket = true;
        }
#else
        /*
         * We need to send something. We get the amount of data remaining to send and set the buffer position for the
         * first data byte (after the packet number which we will set later).
         */
        int CopyLength = ( nDownloadLength - nCurrentOffset );
		//SetNewConfigurationLength( nDownloadLength );			// wjs config dwn load
        int nTransmitBufferOffset = 2;
        //MAX_TRANSMIT;
#endif
                    CEventTrace eventTrace;                                        // records events - see EventTrace.h
		eventTrace.Event ( CEventTrace::Details, "CProtelHost::4GetNextDownload -->" );
        ZeroMemory ( m_bTransmitBuffer, sizeof ( m_bTransmitBuffer ));
        if ( nCurrentOffset == 0 )                       // this is the first chunk - we prepare the slave address list
        {
            /*
             * This is the first chunk. We insert the slave address list.
             */
            m_nPacketNumber = 0;
            m_bTransmitBuffer [ 2 ] = bDuplicates[ 0 ] + 1;
            m_bTransmitBuffer [ 3 ] = m_AuditDevice.Address;
            nTransmitBufferOffset = 4;
            for ( int nLoop = 0; nLoop < bDuplicates[ 0 ]; nLoop++ )
            {
                m_bTransmitBuffer [ nTransmitBufferOffset++ ] = bDuplicates[ nLoop + 1 ];
            }
        }

        if (( CopyLength + nTransmitBufferOffset ) > MAX_TRANSMIT )
        {
            /*
             * We can't send all the data so we shorten it. !!!!What if the address list itself is too long?
             */
            CopyLength = MAX_TRANSMIT - nTransmitBufferOffset;
        }

        /*
         * We copy the data chunk to the buffer, advance our position and determine the length of the packet.
         */
        CopyMemory ( m_bTransmitBuffer + nTransmitBufferOffset, pBuffer + nCurrentOffset, CopyLength );
        nCurrentOffset += CopyLength;
        ConfigurationLength = nTransmitBufferOffset + CopyLength;                                   // this is returned
        /*
         * We advance the packet number and set the first two bytes of the packet.
         */
        m_nPacketNumber++;
        if ( nCurrentOffset < nDownloadLength )                                                  // not the last packet
        {
            m_bTransmitBuffer [ 0 ] = HIBYTE( m_nPacketNumber );
            m_bTransmitBuffer [ 1 ] = LOBYTE( m_nPacketNumber );
        }
        else                                                                                         // the last packet
        {
            m_bTransmitBuffer [ 0 ] = 0xff;
            m_bTransmitBuffer [ 1 ] = 0xff;
        }
//printf ( "%d\t%d\t%d\t%d\n", ConfigurationLength, m_nPacketNumber, nCurrentOffset, CopyLength );
        return m_bTransmitBuffer;
    }


    void GetFirmwareOrConfiguration ( bool bConfiguration )
    {
        /**************************************************************************************************************
         * This is called by GetConfiguration or GetSecondConfiguration with bConfiguration TRUE or by GetFirmware.   *
         * with it FALSE. It checks the database to see if a configuration or firmware (as selected by                *
         * bConfiguration) needs to be sent to the device. If so, it sets up m_pbConfiguration or m_pbFirmware and    *
         * sets m_bTransmitConfiguration or m_bTransmitFirmware TRUE. This causes GetNextConfiguration or             *
         * GetNextFirmware to return data when called.                                                                *
         *                                                                                                            *
         * NOTE: When a configuration is fetched from the database, this method prepends it with its length (2 bytes, *
         * MS first). It doesn't do this with firmware. !!!!Is this right?                                            *
         **************************************************************************************************************/
        char szBlobFieldName [ 64 ];
        char szOracleProcedureName [ 128 ];
        ZeroMemory ( szBlobFieldName, sizeof ( szBlobFieldName ));
        ZeroMemory ( szOracleProcedureName, sizeof ( szOracleProcedureName ));
        if ( bConfiguration == true )
        {
            /*
             * Request for configuration. We delete any existing configuration image and prepare to check/get the
             * configuration using the applicable database stored procedure.
             */
            if ( m_pbConfiguration != NULL )
            {
                delete [] m_pbConfiguration;
                m_pbConfiguration = NULL;
            }
            m_bTransmitConfiguration = false;
            m_nConfigurationLength = 0;
            m_nCurrentConfigurationOffset = 0;
            StringCbCopy ( szBlobFieldName, sizeof ( szBlobFieldName ), "po_image" );
            StringCbCopy ( szOracleProcedureName, sizeof ( szOracleProcedureName ), "PKG_COMM_SERVER.getDownloadConfig" );
            //procedure getDownloadConfig (
            //                    pi_callnumber in integer
            //                    , pi_centralAuditor in varchar2 default null
            //                    , pi_auditor in varchar2
            //                    , pi_callstarttime in timestamp default null
            //                    , po_image in out blob
            //                    , pio_doDownload in out integer
            //);
        } 
        else
        {
            /*
             * Request for firmware. We delete any existing firmware image and prepare to check/get the firmware
             * using the applicable database stored procedure.
             */
            if ( m_pbFirmware != NULL )
            {
                delete [] m_pbFirmware;
                m_pbFirmware = NULL;
            }
            m_bTransmitFirmware = false;
            m_nFirmwareLength = 0;
            m_nCurrentFirmwareOffset = 0;
            StringCbCopy ( szBlobFieldName, sizeof ( szBlobFieldName ), "po_firmware" );
            StringCbCopy ( szOracleProcedureName, sizeof ( szOracleProcedureName ), "PKG_COMM_SERVER.getDownloadFirmware" );
            //procedure getDownloadFirmware (
            //                    pi_callnumber in integer
            //                    , pi_centralAuditor in varchar2 default null
            //                    , pi_auditor in varchar2
            //                    , pi_callstarttime in timestamp default null
            //                    , po_firmware in out blob
            //                    , pio_doDownload in out integer
            //);
        }

        /*
         * We prepare the database query, providing details of the call, central auditor and connected device. We
         * also provide a blob parameter with the appropriate name for any returned configuration or firmware.
         */
        CAdoStoredProcedure adoDownload ( szOracleProcedureName );

        _variant_t vtCallNumber (( long ) m_nCallNumber, VT_I4 );
        adoDownload.AddParameter( "pi_callnumber", vtCallNumber, ADODB::DataTypeEnum::adInteger, ADODB::ParameterDirectionEnum::adParamInput, sizeof ( long ));

        _bstr_t bstrCentralAuditor( m_szCentralAuditor );
        _variant_t vtCentralAuditor ( bstrCentralAuditor );
        adoDownload.AddParameter( "pi_centralAuditor", vtCentralAuditor, ADODB::DataTypeEnum::adBSTR, ADODB::ParameterDirectionEnum::adParamInput, bstrCentralAuditor.length());

        _bstr_t bstrSerialNumber( SerialNumber );               // connected device (can be the central auditor itself)
        _variant_t vtAuditor ( bstrSerialNumber );
        adoDownload.AddParameter( "pi_auditor", vtAuditor, ADODB::DataTypeEnum::adBSTR, ADODB::ParameterDirectionEnum::adParamInput, bstrSerialNumber.length());

        _variant_t vtCallStartTime ( m_dCallStartTime, VT_DATE );
        adoDownload.AddParameter( "pi_callstarttime", vtCallStartTime, ADODB::DataTypeEnum::adDate, ADODB::ParameterDirectionEnum::adParamInput, sizeof ( double ));

        {
            int nBytes = 1;                                        // must be nonzero - otherwise vtBlob would be empty
            BYTE* pszBlob = new BYTE [ nBytes ];
            ZeroMemory ( pszBlob, nBytes );

            variantBlob vtBlob ( pszBlob, nBytes );
            adoDownload.AddParameter( szBlobFieldName, vtBlob, ADODB::DataTypeEnum::adLongVarBinary, ADODB::ParameterDirectionEnum::adParamInputOutput, 262144 );
            delete [] pszBlob;
        }

        /*
         * We add a parameter indicating if we or the device itself know it needs configuration. The query may also
         * determine reconfiguration is required because it has changed - no corresponding check for firmware!!!!
         */
        long NeedsDownload = 0;
        if (( m_AuditDevice.StatusByte & 0x80 ) != 0 && bConfiguration == true )
        {
            NeedsDownload = 1;                                      // we got the configuration and the device needs it
        }
        if ( m_bFirmwareHasBeenDownloaded == true && bConfiguration == true )
        {
            NeedsDownload = 1;                                // we got the configuration and have updated the firmware
        }
        _variant_t vtdoDownload (( long ) NeedsDownload );
        adoDownload.AddParameter( "pio_doDownload", vtdoDownload, ADODB::adInteger, ADODB::adParamInputOutput, sizeof ( long ));

        /*
         * We do the query, allowing messages and blob operations. The stored procedure gets the record relating to
         * this device and call from the COMM_SERVER_AUDITORS table (inserted during the call according to the N
         * command response).
         *
         * FOR CONFIGURATION:
         *
         * If the device is a card reader (ADDRESS from COMM_SERVER_AUDITORS is 200 or more), PENDING_CR and
         * CR_CFG_NUM where CARDREADERID matches in the MACHINES_PENDING_DWNLD_TAB table are obtained. Otherwise,
         * PENDING_CFG where MONITOR ID matches in the same table is obtained. If pio_doDownload, PENDING_CR or
         * PENDING_CFG is non-zero, configuration download is required.
         *
         * Otherwise in the case of a card reader, its current configuration version is built from the
         * COMM_SERVER_AUDITORS table entry as a concatenation of the third digit of FIRMWARE_VERSION plus
         * FIRMWARE_MONITOR, FIRMWARE_VERSION_REV and CONFIG_FILE_VERSION. Unless this matches CR_CFG_NUM,
         * configuration download is required.
         *
         * If configuration download is required, the configuration is obtained from the MACHINES_IMAGE_CS_TAB and
         * pio_do_Download is returned non-zero. Note: for a card reader, IMAGETEMPLATEID is always 99.
         *
         * FOR FIRMWARE:
         *
         * PENDING_FW and PENDING_FIRMWAREID where MONITORID matches in the MACHINES_PENDING_DWNLD_TAB table are
         * obtained. If PENDING_FW is zero (or there is no match), pioDoDownload is returned as zero. Otherwise, the
         * firmware is obtained from the FIRMWARE_LIBRARY_TAB where FIRMWAREID matches PENDING_FIRMWAREID and
         * pioDoDownload is returned non-zero. (See comments for GetNextFirmware regarding how PENDING_FW is
         * cleared.)
         */
		int ret_dwnload;
		try
		{
			ret_dwnload = m_padoConnection->ExecuteNonQuery( adoDownload, false, true );
		} 
		catch (_com_error &comError)
		{
            CEventTrace eventTrace;                                                // records events - see EventTrace.h
            eventTrace.Event ( CEventTrace::Information, "CProtelDevice::GetFirmwareOrConfiguration <--> ERROR: [%s]", CErrorMessage::ReturnComErrorMessage ( comError ));
			return;
		}
        _variant_t vtDoDownload = adoDownload.GetParameter("pio_doDownload");
        if (( long ) vtDoDownload == 0 )                                                        // no download required
        {
#ifdef  _DEBUG
            printf_s ( "Monitor: %s--NO %s RETURNED (pio_doDownload=0) (Call#%d)\n", SerialNumber, ( bConfiguration == true ) ? "Configuration" : "Firmware", m_nCallNumber );
#endif
        }
        else                                        // download is required - get the image from the returned parameter
        {
            _variant_t vtReturnedBlob = adoDownload.GetParameter( szBlobFieldName );

            long nBlobLength = 0;
            void* pBlobPointer = NULL;
            HRESULT hr=::SafeArrayAccessData ( vtReturnedBlob.parray, &pBlobPointer );
            if ( SUCCEEDED ( hr ))
            {
                hr = SafeArrayGetUBound ( vtReturnedBlob.parray, 1, &nBlobLength );
                if ( nBlobLength > 0 )
                {
                    nBlobLength++;    // Necessary because SafeArrayGetUBound returns the Offset of the highest member!
                    if ( bConfiguration == true )
                    {
                        /*
                         * Configuration download is required and we have the image. We set it up and store it, set
                         * its length and mark to send it.
                         */
                        m_bTransmitConfiguration = true;
                        m_nConfigurationLength = nBlobLength + 2;
						//SetNewConfigurationLength( m_nConfigurationLength );	// bug fixing
                        m_pbConfiguration = new unsigned char [ m_nConfigurationLength ];
                        memcpy ( m_pbConfiguration + 2, pBlobPointer, nBlobLength );
                        short shortConfigurationLength = ( short ) nBlobLength;
                        *( m_pbConfiguration + 0 ) = HIBYTE ( shortConfigurationLength );
                        *( m_pbConfiguration + 1 ) = LOBYTE ( shortConfigurationLength );
                    }
                    else
                    {
                        /*
                         * Firmware download is required and we have the image. We store it, set its length and mark
                         * to send it.
                         */
                        m_bTransmitFirmware = true;
                        m_nFirmwareLength = nBlobLength;
                        m_pbFirmware = new unsigned char [ m_nFirmwareLength ];
                        memcpy ( m_pbFirmware, pBlobPointer, nBlobLength );
                    }
                }
                hr = SafeArrayUnaccessData ( vtReturnedBlob.parray );
            }
#ifdef  _DEBUG
            printf_s ( "Monitor: %s--%s TO DOWNLOAD (%d bytes) (Call#%d)\n", SerialNumber, ( bConfiguration == true ) ? "Configuration" : "Firmware", nBlobLength, m_nCallNumber );
#endif
            if (( bConfiguration == true && m_bTransmitConfiguration == true ) || ( bConfiguration == false && m_bTransmitFirmware == true ))
            {
                CEventTrace eventTrace;                                            // records events - see EventTrace.h
                char szText [ 32 ];
                BYTE* pszBlob = NULL;
                long nBlobLen = 0;
                if ( bConfiguration == true )
                {
                    StringCbCopy ( szText, sizeof ( szText ), "Configuration" );
                    pszBlob = m_pbConfiguration;
                    nBlobLen = m_nConfigurationLength;
                }
                else
                {
                    StringCbCopy ( szText, sizeof ( szText ), "Firmware" );
                    pszBlob = m_pbFirmware;
                    nBlobLen = m_nFirmwareLength;
                }
                char szLength [ 32 ];
                _ltoa_s( nBlobLen, szLength, sizeof ( szLength ), 10 );
                eventTrace.BeginXML ( CEventTrace::Details );
                eventTrace.XML ( CEventTrace::Details, szText, SerialNumber );
                eventTrace.XML ( CEventTrace::Details, "Length", szLength );
                if ( bConfiguration == true )
                {
                    eventTrace.XML ( CEventTrace::Details, "ASCII", CHexDump::GetAsciiString( pszBlob, nBlobLen ));
                    eventTrace.XML ( CEventTrace::Details, "HEX", CHexDump::GetHexString( pszBlob, nBlobLen ));
                }
                eventTrace.EndXML ( CEventTrace::Details );

            }
        }
    }

public:
    void UpdateDatabaseForFirmware ( void )
    {
        /**************************************************************************************************************
         * ProtelHost uses this to update the database for this device when it has received firmware as a duplicate   *
         * of that sent to an earlier device.                                                                         *
         **************************************************************************************************************/
        m_bFirmwareHasBeenDownloaded = true;
        try
        {
            //procedure firmware_xmit
            //pi_callnumber in integer,
            //pi_CENTRALAUDITOR   IN   VARCHAR2 default null,
            //pi_CALLSTARTTIME    IN   TIMESTAMP  default null,
            //pi_AUDITOR          IN   VARCHAR2,
            //pi_DOWNLOAD_DATA    IN   BLOB
            CAdoStoredProcedure adoStoredProcedure ( "PKG_COMM_SERVER.FIRMWARE_XMIT" );

            _variant_t vtCallNumber (( long ) m_nCallNumber, VT_I4 );
            adoStoredProcedure.AddParameter( "pi_CALLNUMBER", vtCallNumber, ADODB::DataTypeEnum::adInteger, ADODB::ParameterDirectionEnum::adParamInput, sizeof ( long ));

            _bstr_t bstrCentralAuditor( m_szCentralAuditor );
            _variant_t vtCentralAuditor ( bstrCentralAuditor );
            adoStoredProcedure.AddParameter( "pi_CENTRALAUDITOR", vtCentralAuditor, ADODB::DataTypeEnum::adBSTR, ADODB::ParameterDirectionEnum::adParamInput, bstrCentralAuditor.length());

            _variant_t vtCallStartTime ( m_dCallStartTime, VT_DATE );
            adoStoredProcedure.AddParameter( "pi_CALLSTARTTIME", vtCallStartTime, ADODB::DataTypeEnum::adDate, ADODB::ParameterDirectionEnum::adParamInput, sizeof ( double ));

            _bstr_t bstrAuditor( m_szSerialNumber );
            _variant_t vtAuditor ( bstrAuditor );
            adoStoredProcedure.AddParameter( "pi_AUDITOR", vtAuditor, ADODB::DataTypeEnum::adBSTR, ADODB::ParameterDirectionEnum::adParamInput, bstrAuditor.length());

            variantBlob vtPayload ( m_pbFirmware, m_nFirmwareLength ); // save downloaded firmware back to the database - ridiculous!!!!
            adoStoredProcedure.AddParameter( "pi_DOWNLOAD_DATA", vtPayload, ADODB::DataTypeEnum::adVarBinary, ADODB::ParameterDirectionEnum::adParamInput, m_nFirmwareLength );
            m_padoConnection->ExecuteNonQuery( adoStoredProcedure, false, true );
        }
        catch ( _com_error &comError )
        {
}
    }

    void UpdateDatabaseForConfig ( void )
    {
        /**************************************************************************************************************
         * This may be intended to be used by ProtelHost to update the database for this device when it has received  *
         * a configuration as a duplicate of one sent to an earlier device. However, it doesn't seem to be being      *
         * used!!!!                                                                                                   *
         **************************************************************************************************************/
        try
        {
            //procedure CONFIG_XMIT
            //pi_callnumber in integer,
            //pi_CENTRALAUDITOR   IN   VARCHAR2 default null,
            //pi_CALLSTARTTIME    IN   TIMESTAMP  default null,
            //pi_AUDITOR          IN   VARCHAR2,
            //pi_DOWNLOAD_DATA    IN   BLOB
            CAdoStoredProcedure adoStoredProcedure ( "PKG_COMM_SERVER.CONFIG_XMIT" );

            _variant_t vtCallNumber (( long ) m_nCallNumber, VT_I4 );
            adoStoredProcedure.AddParameter( "pi_CALLNUMBER", vtCallNumber, ADODB::DataTypeEnum::adInteger, ADODB::ParameterDirectionEnum::adParamInput, sizeof ( long ));

            _bstr_t bstrCentralAuditor( m_szCentralAuditor );
            _variant_t vtCentralAuditor ( bstrCentralAuditor );
            adoStoredProcedure.AddParameter( "pi_CENTRALAUDITOR", vtCentralAuditor, ADODB::DataTypeEnum::adBSTR, ADODB::ParameterDirectionEnum::adParamInput, bstrCentralAuditor.length());

            _variant_t vtCallStartTime ( m_dCallStartTime, VT_DATE );
            adoStoredProcedure.AddParameter( "pi_CALLSTARTTIME", vtCallStartTime, ADODB::DataTypeEnum::adDate, ADODB::ParameterDirectionEnum::adParamInput, sizeof ( double ));

            _bstr_t bstrAuditor( m_szSerialNumber );
            _variant_t vtAuditor ( bstrAuditor );
            adoStoredProcedure.AddParameter( "pi_AUDITOR", vtAuditor, ADODB::DataTypeEnum::adBSTR, ADODB::ParameterDirectionEnum::adParamInput, bstrAuditor.length());

            variantBlob vtPayload ( m_pbConfiguration, m_nConfigurationLength ); // save downloaded config back to the database - ridiculous!!!!
            adoStoredProcedure.AddParameter( "pi_DOWNLOAD_DATA", vtPayload, ADODB::DataTypeEnum::adVarBinary, ADODB::ParameterDirectionEnum::adParamInput, m_nConfigurationLength );
            m_padoConnection->ExecuteNonQuery( adoStoredProcedure, false, true );                              // do it
        }
        catch ( _com_error &comError )
        {
//		PKG_COMM_SERVER.addSysLogRecAutonomous ( pi_text varchar2 )
            CEventTrace eventTrace;                                                // records events - see EventTrace.h
            eventTrace.Event ( CEventTrace::Information, "CProtelDevice::UpdateDatabaseForConfig <--> ERROR: [%s]", CErrorMessage::ReturnComErrorMessage ( comError ));
        }
    }
 };
