/**********************************************************************************************************************
 *                                    This file contains the CProfileValues class.                                    *
 *                                                                                                                    *
 * This class gets profile (option) values from a .INI file associated with the executable file of the current        *
 * process.                                                                                                           *
 *                                                                                                                    *
 *                                         Copyright (c) Protel Inc. 2009-2010                                        *
 **********************************************************************************************************************/
#pragma once

class CProfileValues
 {
private:

    enum SectionKey                                                      // used to select which item to get and return
    {
        Socket_Listen_Port,                                                                                        // 0
        Socket_Retry,                                                                                              // 1
        Socket_Use,                                                                                                // 2
        Modem_Retry,                                                                                               // 3
        Modem_Use,                                                                                                 // 4
        debug_port,                                                                                                // 5
        debug_level,                                                                                               // 6
        database_file,                                                                                             // 7
        database_connection,                                                                                       // 8
        database_maxsize,                                                                                       // 8
        heartbeat_minutes,                                                                                         // 9
        manualpoll_seconds,                                                                                       // 10
		commserver_version,																						   // 11
    };
    char szFileName [ 1024 ];                                                   // path and name of profile (.INI) file
    char szValue [ 4096 ];                                                                           // returned string

    int GetIntegerValue ( int WhichOne )
    {
        /**************************************************************************************************************
1>ProtelCommunications.obj : error LNK2020: unresolved token (0A00025E) "protected: static int CProtelHost::maxretranAcmd" (?maxretranAcmd@CProtelHost@@1HA)
         * This gets  profile string value, converts it to an integer and returns it.                                *
         **************************************************************************************************************/
        return atoi ( GetStringValue ( WhichOne ));
    }

    char* GetStringValue ( int WhichOne )
    {
        /**************************************************************************************************************
         * This uses the standard MS GetPrivateprofileString function to return a named string from a named section   *
         * in a .INI file associated with the executable file of the current process. SectionKey enumeration WhichOne *
         * selects the section and string names from hard coded lists below.                                          *
         *                                                                                                            *
         * If the file, section or string is not found in the profile file, a default value hard coded below is       *
         * returned and is also written to the file (which is created if it doesn't exist).                           *
         *                                                                                                            *
         * If WhichOne is database_file, the default value is a .LOG file associated with the executable file of the  *
         * current process.                                                                                           *
         **************************************************************************************************************/
        //HANDLE hIOMutex = CreateMutex ( NULL, FALSE, "CProfileValues.WaitMutex" );
        //WaitForSingleObject( hIOMutex, INFINITE );
        char* pszSectionName[] =                                                            // hard-coded section names
        {
            "Socket",                                                                              //Socket_Listen_Port
            "Socket",                                                                                    //Socket_Retry
            "Socket",                                                                                      //Socket_Use
            "Modem",                                                                                      //Modem_Retry
            "Modem",                                                                                        //Modem_Use
            "debug",                                                                                       //debug_port
            "debug",                                                                                      //debug_level
            "database",                                                                                 //database_file
            "database",                                                                           //database_connection
            "database",                                                                           //database_connection
            "heartbeat",                                                                            //heartbeat_minutes
            "manualpoll",                                                                          //manualpoll_seconds
			"commserver"																		   // comm server version #
        };
        char* pszKeyName[] =                                                           // hard-coded key (string) names
        {
            "Listen Port",                                                                         //Socket_Listen_Port
            "Retry",                                                                                     //Socket_Retry
            "Use",                                                                                         //Socket_Use
            "Retry",                                                                                      //Modem_Retry
            "Use",                                                                                          //Modem_Use
            "port",                                                                                        //debug_port
            "level",                                                                                      //debug_level
            "file",                                                                                     //database_file
            "connection",                                                                         //database_connection
            "maxsize",                                                                         //database_connection
            "minutes",                                                                              //heartbeat_minutes
            "seconds",                                                                             //manualpoll_seconds
			"version"
        };
        char* pszDefaultValue[] =                                                          // hard-coded default values
        {
            "2172",                                                                                //Socket_Listen_Port
            "3",                                                                                         //Socket_Retry
            "1",                                                                                           //Socket_Use
            "3",                                                                                          //Modem_Retry
            "1",                                                                                            //Modem_Use
            "5000",                                                                                        //debug_port
            "2",                                                                                          //debug_level
            "Protelcommunications.log",                                                                   //database_file - isn't actually used
            "Provider=OraOLEDB.Oracle.1;Password=IVS2_CS;Persist Security Info=True;User ID=IVS2_CS;Data Source=pvend2",
			//		database connect string
			"100000",						                       //max log file size                                                                  //database_connection
            "5",                                                                                    //heartbeat_minutes
            "60",                                           // changed to 60 (1 min) from 300 (5 min) manualpoll_seconds
			"2.0.0.100"									// default value
        };
        ZeroMemory ( szValue, sizeof ( szValue ));
        int ReturnedLength = GetPrivateProfileString(
            pszSectionName[ WhichOne ],                              // lpAppName [in] - section name to search for key
            pszKeyName[ WhichOne ],                                // lpKeyName [in] - key whose data is to be returned
            NULL,                                       // lpDefault [in] - NULL = return empty string if key not found
            szValue,                                                       // lpReturnedString [out] - retrieved string
            sizeof ( szValue ),                                 // nsize [in] -- size of lpReturnedString in characters
            GetIniFileName());                                              // lpFileName [in] - name of file to search

        if ( ReturnedLength < 1 )                                                                    // no string found
        {
            StringCbCopy ( szValue, sizeof ( szValue ), pszDefaultValue[ WhichOne ] );
            if ( WhichOne == database_file )
            {
                ZeroMemory ( szValue, sizeof ( szValue ));
                GetModuleFileName( NULL, szValue, sizeof ( szValue ));               // executable file of this process
                PathRemoveExtension ( szValue );
                PathAddExtension ( szValue, ".LOG" );
            }
            WritePrivateProfileString( pszSectionName[ WhichOne ], pszKeyName[ WhichOne ], szValue, GetIniFileName());
        }
        //ReleaseMutex( hIOMutex);
        return szValue;
    }

public:
    char* GetIniFileName(void)
    {
        /**************************************************************************************************************
         * This is used by GetStringValue() above. It gets the name of the file containing the executable of this     *
         * process, changes its extension to .INI and returns it.                                                     *
         **************************************************************************************************************/
        ZeroMemory ( szFileName, sizeof ( szFileName ));
        GetModuleFileName( NULL, szFileName, sizeof ( szFileName ));
        PathRemoveExtension ( szFileName );
        PathAddExtension ( szFileName, ".INI" );
        if ( PathFileExists ( szFileName ) == FALSE )
        {
        }
        return szFileName;
    }
    //__declspec(property(get = GetIniFileName)) char* IniFileName;


    /*
     * The following are used to get specific profile (option) values.
     */
    int GetListenPortNumber ( void )                                        // CSocketListener listens on returned port
    {
        return GetIntegerValue ( Socket_Listen_Port );
    }

    int GetModemRetryCount ( void )                                                                           // unused
    {
        return GetIntegerValue ( Modem_Retry );
    }

    bool GetUseModems ( void )                                        // if true returned, CApplication sets up modems.
    {
        int Value = GetIntegerValue ( Modem_Use );
        if ( Value <= 0 )
        {
            return false;
        }
        return true;
    }

    int GetSocketRetryCount ( void )                                                                          // unused
    {
        return GetIntegerValue ( Socket_Retry );
    }

    bool GetUseSockets ( void )                                // if true returned, CApplication starts CSocketListener
    {
        int Value = GetIntegerValue ( Socket_Use );
        if ( Value <= 0 )
        {
            return false;
        }
        return true;
    }

    int GetDebugPort ( void )                 // EventTrace::Broadcast sends data to this port at LAN broadcast address
    {
        return GetIntegerValue ( debug_port );
    }

    int GetDebugLevel ( void )                                             // sets minimum level recorded by EventTrace
    {
        return GetIntegerValue ( debug_level );
    }

    char* GetLogFile ( void )                                                 // sets log file path/name for EventTrace
    {
        return GetStringValue ( database_file );
    }

    int GetMaxFileSize ( void )                                    // CMonitor records heartbeat when this period elapses
    {
        return GetIntegerValue ( database_maxsize );
    }

    char* GetConnectionString ( void )                                 // used all over to open connections to database
    {
        return GetStringValue ( database_connection );
    }

    int GetHeartbeat ( void )                                    // CMonitor records heartbeat when this period elapses
    {
        return GetIntegerValue ( heartbeat_minutes );
    }

    int GetManualPolling ( void )                          // Application tries a polling call when this period elapses
    {
        return GetIntegerValue ( manualpoll_seconds );
    }

    char*  GetCommServer ( void )                          // Application tries a polling call when this period elapses
    {
        return GetStringValue ( commserver_version );
    }

	CProfileValues()
    {
        /**************************************************************************************************************
         * CONSTRUCTOR                                                                                                *
         **************************************************************************************************************/
    }
 };
