#pragma once

//
// Enumerate the filesystem to build a list of paths matching a criteria
//

#include <windows.h>
#include <windowsx.h>

#include <djlsav.hxx>
#include <djl_pa.hxx>
#include <djltrace.hxx>
#include <ppl.h>

using namespace concurrency;

class CEnumFolder
{
    private:
        bool recurse;
        CStringArray * resultStrings;
        CPathArray * resultPaths;
        const WCHAR * const * extensions;
        int extensionCount;

        bool HasValidExtension( const WCHAR * pwc )
        {
            if ( 0 == extensionCount )
                return true;

            const WCHAR * pext = wcsrchr( pwc, L'.' );
            if ( NULL == pext )
                return false;

            pext++;

            for ( int i = 0; i < extensionCount; i++ )
            {
                int c = wcscmp( pext, extensions[ i ] );

                if ( 0 == c )
                    return true;

                if ( c < 0 )
                    break;
             }

            return false;
        }

    public:
        // recurse:      true to recurse into folders
        // pPathArray:   files found
        // aExtensions:  a sorted list of valid file extensions not including a period. May be NULL.
        // cExtensions:  count of extensions in the array. may be 0.

        CEnumFolder( bool recurseFolders, CPathArray * pPathArray, const WCHAR * const * aExtensions, int cExtensions )
        {
            recurse = recurseFolders;
            resultStrings = NULL;
            resultPaths = pPathArray;
            extensions = aExtensions;
            extensionCount = cExtensions;
        }

        CEnumFolder( bool recurseFolders, CStringArray * pStringArray, const WCHAR * const * aExtensions, int cExtensions )
        {
            recurse = recurseFolders;
            resultStrings = pStringArray;
            resultPaths = NULL;
            extensions = aExtensions;
            extensionCount = cExtensions;
        }

        // pwcFolder:   the root of the enumeration, e.g. C:\users
        // pwcFileSpec: a wildcard string like "*", "*.jpg", or "??.jpg". Can be NULL for "*"

        void Enumerate( const WCHAR * pwcFolder, const WCHAR * pwcFileSpec )
        {
            size_t len = wcslen( pwcFolder );
            if ( 0 == len )
                return;

            WCHAR *pwcSpec = ( 0 == pwcFileSpec ) ? L"*" : pwcFileSpec;
            size_t specLen = wcslen( pwcSpec );

            WCHAR awc[ MAX_PATH ];

            if ( ( len + 1 + specLen ) >= _countof( awc ) )
            {
                tracer.Trace( "skipping very long enumerate path %ws\n", pwcFolder );
                return;
            }

            wcscpy_s( awc, len + 1, pwcFolder );
            if ( L'\\' != awc[ len - 1 ] )
            {
                awc[ len++ ] = L'\\';
                awc[ len ] = 0;
            }

            wcscpy_s( awc + len, specLen + 1, pwcSpec );

            bool allFiles = ( !wcscmp( pwcSpec, L"*" ) || !wcscmp( pwcSpec, L"*.*" ) );

            CStringArray aDirs;
            WIN32_FIND_DATA fd;
            HANDLE hFile = FindFirstFileEx( awc, FindExInfoBasic, &fd, FindExSearchNameMatch, 0, FIND_FIRST_EX_LARGE_FETCH | FIND_FIRST_EX_ON_DISK_ENTRIES_ONLY );

            if ( INVALID_HANDLE_VALUE != hFile )
            {
                do
                {
                    if ( wcscmp( fd.cFileName, L"." ) && wcscmp( fd.cFileName, L".." ) )
                    {
                        _wcslwr( fd.cFileName );
                        size_t namelen = wcslen( fd.cFileName );
        
                        if ( ( namelen + len ) < _countof( awc ) )
                        {
                            wcscpy_s( awc + len, namelen + 1, fd.cFileName );

                            if ( fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
                            {
                                if ( recurse && allFiles )
                                    aDirs.Add( awc );
                            }
                            else if ( HasValidExtension( fd.cFileName ) )
                            {
                                if ( 0 != resultPaths )
                                    resultPaths->Add( awc, fd.ftCreationTime, fd.ftLastWriteTime );
                                if ( 0 != resultStrings )
                                    resultStrings->Add( awc );
                            }
                        }
                        else
                        {
                            tracer.Trace( "skipping very long path %ws and file %ws\n", awc, fd.cFileName );
                        }
                    }
                } while ( FindNextFile( hFile, &fd ) );
        
                FindClose( hFile );
            }

            if ( recurse )
            {
                // If the filespec didn't include all files, look for folders here

                if ( !allFiles )
                {
                    wcscpy_s( awc + len, 2, L"*" );
                    hFile = FindFirstFileEx( awc, FindExInfoBasic, &fd, FindExSearchLimitToDirectories, 0, FIND_FIRST_EX_LARGE_FETCH );
                
                    if ( INVALID_HANDLE_VALUE != hFile )
                    {
                        do
                        {
                            if ( ( 0 != ( fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) ) && // required because the flag above is just a hint
                                 ( 0 != wcscmp( fd.cFileName, L".") ) &&
                                 ( 0 != wcscmp( fd.cFileName, L"..") ) )
                            {
                                size_t fileLen = wcslen( fd.cFileName );
                
                                if ( ( len + fileLen + 2 ) >= _countof( awc ) )
                                {
                                    tracer.Trace( "skipping very long path %ws and directory %ws\n", awc, fd.cFileName );
                                    continue;
                                }

                                wcscpy_s( awc + len, fileLen + 1, fd.cFileName );
                                wcscpy_s( awc + len + fileLen, 2, L"\\" );

                                aDirs.Add( awc );
                            }
                        } while ( FindNextFile( hFile, &fd ) );
                
                        FindClose( hFile );
                    }
                }

                parallel_for( 0, (int) aDirs.Count(), [&] ( int i )
                {
                    Enumerate( aDirs[ i ], pwcFileSpec );
                } );
            }
        }
};

