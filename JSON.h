#pragma once

class JSON
{
protected:
	char szBuffer [ 10240 ];

public:

	JSON(void)
	{
		ZeroMemory ( szBuffer, sizeof ( szBuffer ));
	}

	~JSON(void)
	{
	}

	void Begin()
	{
		ZeroMemory ( szBuffer, sizeof ( szBuffer ));
		StringCbCopy ( szBuffer, sizeof ( szBuffer ), "{ " );
	}

	void End()
	{
		StringCbCat ( szBuffer, sizeof ( szBuffer ), " }" );
	}

	void NameValue ( char* Name, char* Value )
	{
		size_t Offset;
		StringCbLength( szBuffer, sizeof ( szBuffer ), &Offset );
		if ( Offset > 2 )
		{
			szBuffer [ Offset ] = ',';
			Offset++;
		}
		StringCchPrintf ( szBuffer + Offset, sizeof ( szBuffer ) - Offset, " \"%s\": \"", Name );
		StringCbLength( Value, STRSAFE_MAX_CCH, &Offset );
		CopyValue (( BYTE* ) Value, Offset );
		StringCbLength( szBuffer, sizeof ( szBuffer ), &Offset );
		StringCchCat ( szBuffer + Offset, sizeof ( szBuffer ) - Offset, "\"" );
	}

	void NameValue ( char* Name, BYTE* Value, int Length )
	{
		size_t Offset;
		StringCbLength( szBuffer, sizeof ( szBuffer ), &Offset );
		if ( Offset > 2 )
		{
			szBuffer [ Offset ] = ',';
			Offset++;
		}
		StringCchPrintf ( szBuffer + Offset, sizeof ( szBuffer ) - Offset, " \"%s\": \"", Name );
		CopyValue ( Value, Length );
		StringCbLength( szBuffer, sizeof ( szBuffer ), &Offset );
		StringCchCat ( szBuffer + Offset, sizeof ( szBuffer ) - Offset, "\"" );
	}

	void NameValue ( char* Name, int Value )
	{
		size_t Offset;
		StringCbLength( szBuffer, sizeof ( szBuffer ), &Offset );
		if ( Offset > 2 )
		{
			szBuffer [ Offset ] = ',';
			Offset++;
		}
		StringCchPrintf ( szBuffer + Offset, sizeof ( szBuffer ) - Offset, " \"%s\": %d", Name, Value );
	}

	void NameValue ( char* Name, long Value )
	{
		size_t Offset;
		StringCbLength( szBuffer, sizeof ( szBuffer ), &Offset );
		if ( Offset > 2 )
		{
			szBuffer [ Offset ] = ',';
			Offset++;
		}
		StringCchPrintf ( szBuffer + Offset, sizeof ( szBuffer ) - Offset, " \"%s\": %ld", Name, Value );
	}

	void NameValue ( char* Name, bool Value )
	{
		size_t Offset;
		StringCbLength( szBuffer, sizeof ( szBuffer ), &Offset );
		if ( Offset > 2 )
		{
			szBuffer [ Offset ] = ',';
			Offset++;
		}
		StringCchPrintf ( szBuffer + Offset, sizeof ( szBuffer ) - Offset, " \"%s\": %s", Name, Value == true ? "true" : "false" );
	}

	void NameValue ( char* Name, SYSTEMTIME& Value )
	{
		size_t Offset;
		StringCbLength( szBuffer, sizeof ( szBuffer ), &Offset );
		if ( Offset > 2 )
		{
			szBuffer [ Offset ] = ',';
			Offset++;
		}
		StringCchPrintf ( szBuffer + Offset, sizeof ( szBuffer ) - Offset, " \"%s\": \"%02d/%02d/%04d %02d:%02d:%02d.%03d\"", Name, Value.wMonth, Value.wDay, Value.wYear, Value.wHour, Value.wMinute, Value.wSecond, Value.wMilliseconds );
	}

	char* GetBuffer()
	{
		return szBuffer;
	}

	__declspec(property(get = GetBuffer)) char* Buffer;

protected:
	void CopyValue ( BYTE* pBuffer, int Length )
	{
		size_t Offset;
		StringCbLength( szBuffer, sizeof ( szBuffer ), &Offset );
		for ( int BufferOffset = 0; BufferOffset < Length; BufferOffset++ )
		{
			if ( pBuffer[ BufferOffset ] == '\\' ||
					  pBuffer[ BufferOffset ] == '"' ||
					  pBuffer[ BufferOffset ] == '/' )
			{
				szBuffer [ Offset ] = '\\';
				Offset++;
				szBuffer [ Offset ] = pBuffer[ BufferOffset ];
				Offset++;
				szBuffer [ Offset ] = '\0';
				//Offset++;
			}
			else if ( isprint( pBuffer[ BufferOffset ] ))
			{
				szBuffer [ Offset ] = pBuffer[ BufferOffset ];
				Offset++;
				//szBuffer [ Offset ] = '\0';
			}
			else if ( pBuffer[ BufferOffset ] == '\t' )
			{
				szBuffer [ Offset ] = '\\';
				Offset++;
				szBuffer [ Offset ] = 't';
				Offset++;
				szBuffer [ Offset ] = '\0';
				//Offset++;
			}
			else if ( pBuffer[ BufferOffset ] == '\f' )
			{
				szBuffer [ Offset ] = '\\';
				Offset++;
				szBuffer [ Offset ] = 'f';
				Offset++;
				szBuffer [ Offset ] = '\0';
				//Offset++;
			}
			else if ( pBuffer[ BufferOffset ] == '\n' )
			{
				szBuffer [ Offset ] = '\\';
				Offset++;
				szBuffer [ Offset ] = 'n';
				Offset++;
				szBuffer [ Offset ] = '\0';
				//Offset++;
			}
			else if ( pBuffer[ BufferOffset ] == '\r' )
			{
				szBuffer [ Offset ] = '\\';
				Offset++;
				szBuffer [ Offset ] = 'r';
				Offset++;
				szBuffer [ Offset ] = '\0';
				//Offset++;
			}
			else if ( pBuffer[ BufferOffset ] == '\b' )
			{
				szBuffer [ Offset ] = '\\';
				Offset++;
				szBuffer [ Offset ] = 'b';
				Offset++;
				szBuffer [ Offset ] = '\0';
				//Offset++;
			}
			else
			{
				char szHexBuffer [ 16 ];
				StringCchPrintf ( szHexBuffer, sizeof ( szHexBuffer ), "%04x", pBuffer[ BufferOffset ] );
				szBuffer [ Offset ] = '\\';
				Offset++;
				szBuffer [ Offset ] = szHexBuffer [ 0 ];
				Offset++;
				szBuffer [ Offset ] = szHexBuffer [ 1 ];
				Offset++;
				szBuffer [ Offset ] = szHexBuffer [ 2 ];
				Offset++;
				szBuffer [ Offset ] = szHexBuffer [ 3 ];
				Offset++;
				szBuffer [ Offset ] = '\0';
				//Offset++;
			}
		}
	}

};
