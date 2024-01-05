#pragma once

//
// Stream over a file or subset of a file
//

class CStream
{
    private:
        __int64 length;
        __int64 offset;
        __int64 embedOffset;
        HANDLE hFile;
        bool handleOwned;
        bool seekCalled;
        bool forWrite;

    public:
        CStream()
        {
            length = 0;
            offset = 0;
            embedOffset = 0;
            hFile = INVALID_HANDLE_VALUE;
            handleOwned = false;
            seekCalled = false;
            forWrite = false;
        } //CStream

        CStream( WCHAR const * pwcFile, bool write = false )
        {
            embedOffset = 0;
            length = 0;
            offset = 0;
            seekCalled = false;
            handleOwned = true;
            forWrite = write;

            if ( forWrite )
                hFile = CreateFile( pwcFile, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, CREATE_ALWAYS, 0, 0 );
            else
            {
                hFile = CreateFile( pwcFile, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, 0 );

                if ( INVALID_HANDLE_VALUE != hFile )
                {
                    LARGE_INTEGER liSize;
                    GetFileSizeEx( hFile, &liSize );
                    length = liSize.QuadPart;
                }
            }
        } //CStream

        CStream( HANDLE h )
        {
            embedOffset = 0;
            length = 0;
            offset = 0;
            seekCalled = true; // don't trust where this handle has been
            handleOwned = false;
            hFile = h;
            forWrite = false;

            LARGE_INTEGER liSize;
            BOOL ok = GetFileSizeEx( hFile, &liSize );
            if ( ok )
                length = liSize.QuadPart;
        } //CStream

        CStream( WCHAR const * pwcFile, __int64 embeddedOffset, __int64 embeddedLength )
        {
            if ( embeddedOffset < 0 || embeddedLength < 0 )
            {
                embeddedOffset = 0;
                embeddedLength = 0;
            }

            embedOffset = embeddedOffset;
            length = embeddedLength;
            offset = 0;
            seekCalled = true; // need to get to virtual 0 on first read
            handleOwned = true;
            forWrite = false;
            hFile = CreateFile( pwcFile, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0 );

            if ( INVALID_HANDLE_VALUE == hFile )
                length = 0;
            else
            {
                LARGE_INTEGER liSize;
                BOOL ok = GetFileSizeEx( hFile, &liSize );
                if ( ok )
                {
                    if ( embedOffset > liSize.QuadPart )
                    {
                        embedOffset = 0;
                        length = 0;
                    }
                    else
                    {
                        length = __min( liSize.QuadPart - embeddedOffset, length );
                    }
                }
                else
                {
                    length = 0;
                    embedOffset = 0;
                }
             }
        } //CStream

        void CloseFile()
        {
            if ( handleOwned && INVALID_HANDLE_VALUE != hFile )
            {
                CloseHandle( hFile );
                hFile = INVALID_HANDLE_VALUE;
            }
        } //CloseFile

        ~CStream()
        {
            CloseFile();
        }

        ULONG Read( void *pv, ULONG cb )
        {
            if ( 0 == length )
                return 0;

            if ( seekCalled )
            {
                LARGE_INTEGER li;
                li.QuadPart = offset + embedOffset;
                SetFilePointerEx( hFile, li, NULL, FILE_BEGIN );
                seekCalled = false;
            }

            if ( ( offset + cb ) > length )
            {
                if ( length > offset )
                    cb = __min( cb, (ULONG) ( length - offset ) );
                else
                    cb = 0;
            }

            DWORD dwRead = 0;
            BOOL ok = ReadFile( hFile, pv, cb, &dwRead, NULL );

            if ( ok )
                offset += cb;
            else
                cb = 0;

            return cb;
        } //Read

        bool Seek( __int64 location )
        {
            if ( location < 0 || location > length )
                return false;

            if ( offset != location )
            {
                offset = location;
                seekCalled = true;
            }

            return true;
        } //Seek

        bool Ok() { return ( INVALID_HANDLE_VALUE != hFile ); }
        __int64 Tell() { return offset; }
        __int64 Length() { return length; }
        bool AtEOF() { return ( offset >= length ); }

        void GetBytes( __int64 seek_offset, void * pData, int byteCount )
        {
            memset( pData, 0, byteCount );

            if ( Seek( seek_offset ) )
                Read( pData, byteCount );
        } //GetBytes

        ULONG Write( void *pv, ULONG cb )
        {
            if ( seekCalled )
            {
                LARGE_INTEGER li;
                li.QuadPart = offset + embedOffset;
                SetFilePointerEx( hFile, li, NULL, FILE_BEGIN );
                seekCalled = false;
            }

            DWORD dwWritten = 0;
            BOOL ok = WriteFile( hFile, pv, cb, &dwWritten, NULL );

            if ( ok )
            {
                offset += dwWritten;

                if ( offset > length )
                    length = offset;
            }
            else
                cb = 0;

            return cb;
        } //Write
};

