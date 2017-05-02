/**********************************************************************************************************************
 *                                       This file contains the CHexDump class.                                       *
 *                                                                                                                    *
 * This provides methods which produce ASCII representations of vectors of arbitrary bytes.                           *
 *                                                                                                                    *
 * WARNING: These methods don't look to be threadsafe.                                                                *
 *                                                                                                                    *
 *                                         Copyright (c) Protel Inc. 2009-2010                                        *
 **********************************************************************************************************************/
#pragma once

class CHexDump
 {
public:
    static char* Output ( BYTE* pData, int DataLength )
    {
        /**************************************************************************************************************
         * This returns an ASCII representation of DataLength bytes in pData formatted in lines representing 16 bytes *
         * each, terminated with CRLF and concatenated together. Each line shows each of the 16 bytes (or less on the *
         * last line) as 2 digit hex, then ASCII for each of the bytes using a dot (.) for nonprintable values.       *
         **************************************************************************************************************/

        static char szReturnBuffer [ 4096 ];                                            // concatenated formatted lines
        char szDebugBuffer [ 256 ];                                                                   // formatted line
        char szDebug0 [ 16 ];                                                                      // each value in hex
        char szDebug1 [ 64 ];                                                    // hex values (e.g. 00 01 41 42 43 ff)
        char szDebug2 [ 17 ];                                                    // printable chars or .s (e.g. ..ABC.)

        ZeroMemory ( szReturnBuffer, sizeof ( szReturnBuffer ));

        for ( int nLoop = 0; nLoop < DataLength; nLoop += 16 )
        {
            /*
             * We process up to 16 bytes from pData: these will produce one line in szDebugBuffer.
             */
            ZeroMemory ( szDebugBuffer, sizeof ( szDebugBuffer ));
            ZeroMemory ( szDebug1, sizeof ( szDebug1 ));
            ZeroMemory ( szDebug2, sizeof ( szDebug2 ));
            for ( int nLoop2 = 0; nLoop2 < 16; nLoop2++ )
            {
                if (( nLoop + nLoop2 ) < DataLength )
                {
                    /*
                     * We haven't passed the end of the data. We append a . to szDebug2, then change it to the ASCII
                     * character corresponding to the byte if it is printable.
                     */
                    szDebug2 [ nLoop2 ] = '.';
                    if ( isprint ( *( pData + nLoop + nLoop2 )))
                    {
                        szDebug2 [ nLoop2 ] = *( pData + nLoop + nLoop2 );
                    }

                    /*
                     * We create a 2 byte ASCII hex representation of the byte plus a space and append to szDebug1.
                     */
                    StringCchPrintf ( szDebug0, sizeof ( szDebug0 ), "%02x ", *( pData + nLoop + nLoop2 ));
                    StringCbCat ( szDebug1, sizeof ( szDebug1 ), szDebug0 );
                }
            }

            /*
             * We pad szDebug1 to 48 bytes (it could have been shorter if we passed the end of pData).
             */
            size_t nLength = 0;
            StringCchLength( szDebug1, sizeof ( szDebug1 ), &nLength );
            while ( nLength < 48 )
            {
                *( szDebug1 + nLength ) = ' ';
                nLength++;
            }

            /*
             * We append the ASCII hex and code strings plus an end-of-line to the return buffer.
             */
            StringCbCat ( szDebugBuffer, sizeof ( szDebugBuffer ), szDebug1 );
            StringCbCat ( szDebugBuffer, sizeof ( szDebugBuffer ), szDebug2 );
            StringCbCat ( szDebugBuffer, sizeof ( szDebugBuffer ), "\r\n" );
            StringCbCat ( szReturnBuffer, sizeof ( szReturnBuffer ), szDebugBuffer );
        }

        szReturnBuffer[sizeof szReturnBuffer - 1] = '\0';                                           // added for safety
        return szReturnBuffer;
    }

    static char* GetHexString ( BYTE* pData, int DataLength )
    {
        /**************************************************************************************************************
         * This returns an ASCII representation of DataLength bytes in pData. Each byte is shown as a two digit hex   *
         * value plus a space.                                                                                        *
         **************************************************************************************************************/
        static char szReturnBuffer [ 4096 ];
        ZeroMemory ( szReturnBuffer, sizeof ( szReturnBuffer ));
        char szDebugBuffer [ 256 ];

        for ( int nLoop = 0; nLoop < DataLength; nLoop++ )
        {
            StringCchPrintf ( szDebugBuffer, sizeof ( szDebugBuffer ), "%02x ", *( pData + nLoop ));
            StringCbCat ( szReturnBuffer, sizeof ( szReturnBuffer ), szDebugBuffer );
        }
        return szReturnBuffer;
    }

    static char* GetAsciiString ( BYTE* pData, int DataLength )
    {
        /**************************************************************************************************************
         * This returns the DataLength bytes in pData with non-printable values replaced by a dot (.).                *
         **************************************************************************************************************/
        static char szReturnBuffer [ 4096 ];
        ZeroMemory ( szReturnBuffer, sizeof ( szReturnBuffer ));
        CopyMemory( szReturnBuffer, pData, DataLength );

        for ( int nLoop = 0; nLoop < DataLength; nLoop++ )
        {
            if ( *( szReturnBuffer + nLoop ) < 0x20 || *( szReturnBuffer + nLoop ) > 0x7e )
            {
                *( szReturnBuffer + nLoop ) = '.';
            }
        }
        return szReturnBuffer;
    }
 };
