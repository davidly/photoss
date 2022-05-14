#pragma once

//
// This code extracts a subset of metadata in image files. It's not intended to be exhaustive;
// it just pulls out the data absolutely required by calling apps.
// Single-threaded runtime is:  51% in CreateFile, 30% in ReadFile,   5% in CloseHandle, 1.7% in GetFileSizeEx,    1.6% in SetFilePointerEx
// Multi-threaded runtime is:   48% in ReadFile,   28% in CreateFile, 4% in CloseHandle, 1.0% in SetFilePointerEx, 0.6% in GetFileSizeEx.
//
// This code reduces the calls to ReadFile at the expense of some clarity.

#include <windows.h>
#include <shlwapi.h>

#include <stdlib.h>
#include <stdio.h>
#include <io.h>
#include <limits.h>
#include <float.h>
#include <eh.h>
#include <math.h>
#include <sys\stat.h>
#include <assert.h>

#include <string>
#include <memory>
#include <mutex>

#include "djltrace.hxx"
#include "djl_strm.hxx"
#include "djl_crop.hxx"

using namespace std;

/*
   1 = BYTE An 8-bit unsigned integer.,
   2 = ASCII An 8-bit byte containing one 7-bit ASCII code. The final byte is terminated with NULL.,
   3 = SHORT A 16-bit (2-byte) unsigned integer,
   4 = LONG A 32-bit (4-byte) unsigned integer,
   5 = RATIONAL Two LONGs. The first LONG is the numerator and the second LONG expresses the denominator.,
   6 = no official definition, but used as an unsigned BYTE
   7 = UNDEFINED An 8-bit byte that can take any value depending on the field definition,
   8 = no official definition. Nokia uses it as an integer to represent ISO. Use as type 4.
   9 = SLONG A 32-bit (4-byte) signed integer (2's complement notation),
  10 = SRATIONAL Two SLONGs. The first SLONG is the numerator and the second SLONG is the denominator.
  11 = No official definition. Sigma uses it for FLOATs (4 bytes)
  12 = No official definition. Apple uses it for double floats (8 bytes)
  13 = IFD pointer (Olympus ORF uses this)
*/

class CImageData
{
private:
    struct TwoDWORDs
    {
        DWORD dw1;
        DWORD dw2;

        void Endian( bool littleEndian )
        {
            if ( !littleEndian )
            {
                dw1= _byteswap_ulong( dw1 );
                dw2 = _byteswap_ulong( dw2 );
            }
        }
    };

    struct IFDHeader
    {
        WORD id;
        WORD type;
        DWORD count;
        DWORD offset;

        void Endian( bool littleEndian )
        {
            if ( !littleEndian )
            {
                id = _byteswap_ushort( id );
                type = _byteswap_ushort( type );
                count = _byteswap_ulong( count );
                offset = _byteswap_ulong( offset );
            }

            offset = AdjustOffset( littleEndian );
        }

        private:

            DWORD AdjustOffset( bool littleEndian )
            {
                if ( 1 != count )
                    return offset;
            
                if ( littleEndian )
                {
                    if ( 1 == type || 6 == type )
                        return offset & 0xff;
            
                    if ( 3 == type || 8 == type )
                        return offset & 0xffff;
                }
                else
                {
                    // The DWORD has already been swapped, but to interpret it as a 1 or 2 byte quantity,
                    // that must be shifted as well.
    
                    if ( 1 == type || 6 == type )
                        return ( offset >> 24 );
            
                    if ( 3 == type || 8 == type )
                        return ( offset >> 16 );
                }
            
                return offset;
            } //AdjustOffset
    };
    
    std::mutex g_mtx;
    CCropFactor g_factor;
    CStream * g_pStream = NULL;
    const double InvalidCoordinate = 1000.0;
    static const WORD MaxIFDHeaders = 200; // assume anything more than this is a corrupt or badly parsed file.
                                           // panasonic makernotes sometimes have 133 entries.
    
    WCHAR g_awcPath[ MAX_PATH + 1 ];
    FILETIME g_ftWrite;
    
    DWORD g_Heif_Exif_ItemID                = 0xffffffff;
    __int64 g_Heif_Exif_Offset              = 0;
    __int64 g_Heif_Exif_Length              = 0;
    __int64 g_Canon_CR3_Exif_IFD0           = 0;
    __int64 g_Canon_CR3_Exif_Exif_IFD       = 0;
    __int64 g_Canon_CR3_Exif_Makernotes_IFD = 0;
    __int64 g_Canon_CR3_Exif_GPS_IFD        = 0;
    __int64 g_Canon_CR3_Embedded_JPG_Length = 0;
    
    __int64 g_Embedded_Image_Offset = 0;
    __int64 g_Embedded_Image_Length = 0;
    int g_Embedded_Image_Width = 0;
    int g_Embedded_Image_Height = 0;
    int g_Orientation_Value = -1;
    int g_Orientation_Value2 = -1;

    // Offsets for writes into the file
    __int64 g_Orientation_Offset = 0;
    __int64 g_Orientation_Offset2 = 0;
    __int64 g_Orientation_Type = 0;
    __int64 g_Orientation_Type2 = 0;
    bool g_Orientation_LittleEndian = false;
    
    char g_acDateTimeOriginal[ 100 ];
    char g_acDateTime[ 100 ];
    int g_ImageWidth;
    int g_ImageHeight;
    int g_ISO;
    int g_ExposureNum;
    int g_ExposureDen;
    int g_ApertureNum;
    int g_ApertureDen;
    int g_ExposureProgram;
    int g_ExposureMode;
    int g_FocalLengthNum;
    int g_FocalLengthDen;
    int g_FocalLengthIn35mmFilm;
    int g_ComputedSensorWidth;
    int g_ComputedSensorHeight;
    double g_Latitude;
    double g_Longitude;
    char g_acLensMake[ 100 ];
    char g_acLensModel[ 100 ];
    char g_acLensSerialNumber[ 100 ];
    char g_acMake[ 100 ];
    char g_acModel[ 100 ];
    char g_acSerialNumber[ 100 ];
    bool g_holdsAdobeEditsInXMP;
    
    WORD FixEndianWORD( WORD w, bool littleEndian )
    {
        if ( !littleEndian )
            w = _byteswap_ushort( w );

        return w;
    } //FixEndianWORD
    
    unsigned long long GetULONGLONG( __int64 offset, bool littleEndian )
    {
        unsigned long long ull = 0;

        if ( g_pStream->Seek( offset ) )
        {
            g_pStream->Read( &ull, sizeof ull );
    
            if ( !littleEndian )
                ull = _byteswap_uint64( ull );
        }
    
        return ull;
    } //GetULONGULONG

    DWORD GetDWORD( __int64 offset, bool littleEndian )
    {
        DWORD dw = 0;     // Note: some files are malformed and point to reads beyond the EOF. Return 0 in these cases

        if ( g_pStream->Seek( offset ) )
        {
            g_pStream->Read( &dw, sizeof dw );
    
            if ( !littleEndian )
                dw = _byteswap_ulong( dw );
        }
    
        return dw;
    } //GetDWORD
    
    WORD GetWORD( __int64 offset, bool littleEndian )
    {
        WORD w = 0;

        if ( g_pStream->Seek( offset ) )
        {
            g_pStream->Read( &w, sizeof w );
    
            if ( !littleEndian )
                w = _byteswap_ushort( w );
        }
    
        return w;
    } //GetWORD
    
    byte GetBYTE( __int64 offset )
    {
        byte b = 0;

        if ( g_pStream->Seek( offset ) )
            g_pStream->Read( &b, sizeof b );
    
        return b;
    } //GetBYTE

    void GetBytes( __int64 offset, void * pData, int byteCount )
    {
        memset( pData, 0, byteCount );

        if ( g_pStream->Seek( offset ) )
            g_pStream->Read( pData, byteCount );
    } //GetBytes

    bool GetIFDHeaders( __int64 offset, IFDHeader * pHeader, WORD numHeaders, bool littleEndian )
    {
        if ( 0 == numHeaders )
            return true;

        bool ok = true;
        int cb = sizeof IFDHeader * numHeaders;

        GetBytes( offset, pHeader, cb );
        for ( WORD i = 0; i < numHeaders; i++ )
            pHeader[i].Endian( littleEndian );

        for ( WORD i = 0; i < numHeaders; i++ )
        {
            // validate type info, because if it's wrong we're likely parsing the file incorrectly.
            // Note the Panasonic LX100, S1R, zs100, & zs200 write 0x100 to the type's second byte, so mask it off.
            // Not all Panasonic RAW files do this -- GF1 for example.

            if ( !strcmp( g_acMake, "Panasonic" ) && ( 0x100 == ( 0xff00 & pHeader[i].type ) ) )
                pHeader[i].type &= 0xff;

            if ( pHeader[i].type > 13 )
            {
                tracer.Trace( "record %d has invalid type %#x make %s, model %s, path %ws\n", i, pHeader[i].type, g_acMake, g_acModel, g_awcPath );
                ok = false;
                break;
            }
        }

        return ok;
    } //GetIFDHeaders
    
    int GetTwoDWORDs( __int64 offset, TwoDWORDs * pb, bool littleEndian )
    {
        GetBytes( offset, pb, sizeof TwoDWORDs );
        pb->Endian( littleEndian );
        return sizeof TwoDWORDs;
    } //GetTwoDWORDs
    
    void GetString( __int64 offset, char * pcOutput, int outputSize, int maxBytes )
    {
        if ( outputSize <= 0 )
            return;

        *pcOutput = 0;

        if ( maxBytes < 0 )
            maxBytes = 0;

        if ( g_pStream->Seek( offset ) )
        {
            int maxlen = __min( outputSize - 1, maxBytes );
            g_pStream->Read( pcOutput, maxlen );
    
            // In case the string wasn't already null terminated because it wasn't stored with a null or the
            // count of bytes didn't include the null termination, add it now. This may add a second null,
            // which is OK.
    
            pcOutput[ maxlen ] = 0;
    
            // These strings sometimes have trailing spaces. Remove them.
        
            size_t len = strlen( pcOutput );
            len--;
        
            while ( len >= 0 )
            {
                if ( ' ' == pcOutput[ len ] )
                    pcOutput[ len ] = 0;
                else
                    break;
        
                len--;
            }
        }
    } //GetString
    
    bool IsIntType( WORD tagType )
    {
        return ( 1 == tagType || 3 == tagType || 4 == tagType || 6 == tagType || 8 == tagType || 9 == tagType );
    } //IsIntType

    bool IsPerhapsJPG( unsigned long long x )
    {
        return ( 0xd8ff == ( x & 0xffff ) );
    } //IsPerhapsJPG
    
    bool IsPerhapsPNG( unsigned long long x )
    {
        return ( 0x5089 == ( x & 0xffff ) );
    } //IsPerhapsPNG
    
    bool IsPerhapsAnImageHeader( unsigned long long x )
    {
        return ( ( 0xd8ff == ( x & 0xffff ) ) ||    // jpg
                 ( 0x5089 == ( x & 0xffff ) ) ||    // png
                 ( 0x4d42 == ( x & 0xffff ) ) ||    // bmp
                 ( 7079746620000000 == x ) );       // hif, heic, etc.
    } //IsPerhapsAnImageHeader

    bool IsPerhapsAnImage( __int64 offset, __int64 headerBase )
    {
        unsigned long long x = GetULONGLONG( offset + headerBase, true );

        return IsPerhapsAnImageHeader( x );
    } //IsPerhapsAnImage

    void EnumerateGPSTags( int depth, __int64 IFDOffset, __int64 headerBase, bool littleEndian )
    {
        if ( 0xffffffff == IFDOffset )
            return;
    
        char acBuffer[ 10 ];
        bool latNeg = false;
        bool lonNeg = false;
        vector<IFDHeader> aHeaders( MaxIFDHeaders );
    
        while ( 0 != IFDOffset ) 
        {
            WORD NumTags = GetWORD( IFDOffset + headerBase, littleEndian );
            IFDOffset += 2;

            // the file is problematic if this is true

            if ( NumTags > MaxIFDHeaders )
                break;
        
            if ( !GetIFDHeaders( IFDOffset + headerBase, aHeaders.data(), NumTags, littleEndian ) )
                break;
        
            for ( int i = 0; i < NumTags; i++ )
            {
                IFDHeader & head = aHeaders[ i ];
                IFDOffset += sizeof IFDHeader;

                if ( 1 == head.id && 2 == head.type )
                {
                    __int64 stringOffset = ( head.count <= 4 ) ? ( IFDOffset - 4 ) : head.offset;
                    GetString( stringOffset + headerBase, acBuffer, _countof( acBuffer ), head.count );

                    if ( 'S' == toupper( acBuffer[0] ) )
                        latNeg = true;
                }
                else if ( 2 == head.id && ( ( 10 == head.type ) || ( 5 == head.type ) ) && 3 == head.count )
                {
                    LONG num1 = GetDWORD( (__int64) head.offset +      headerBase, littleEndian );
                    LONG den1 = GetDWORD( (__int64) head.offset +  4 + headerBase, littleEndian );
                    double d1 = (double) num1 / (double) den1;
    
                    LONG num2 = GetDWORD( (__int64) head.offset +  8 + headerBase, littleEndian );
                    LONG den2 = GetDWORD( (__int64) head.offset + 12 + headerBase, littleEndian );
                    double d2 = (double) num2 / (double) den2;
    
                    LONG num3 = GetDWORD( (__int64) head.offset + 16 + headerBase, littleEndian );
                    LONG den3 = GetDWORD( (__int64) head.offset + 20 + headerBase, littleEndian );
                    double d3 = (double) num3 / (double) den3;
    
                    g_Latitude = d1 + ( d2 / 60.0 ) + ( d3 / 3600.0 );
                }
                else if ( 3 == head.id && 2 == head.type )
                {
                    __int64 stringOffset = ( head.count <= 4 ) ? ( IFDOffset - 4 ) : head.offset;
                    GetString( stringOffset + headerBase, acBuffer, _countof( acBuffer ), head.count );

                    if ( 'W' == toupper( acBuffer[0] ) )
                        lonNeg = true;
                }
                else if ( 4 == head.id && ( ( 10 == head.type ) || ( 5 == head.type ) ) && 3 == head.count )
                {
                    LONG num1 = GetDWORD( (__int64) head.offset +      headerBase, littleEndian );
                    LONG den1 = GetDWORD( (__int64) head.offset +  4 + headerBase, littleEndian );
                    double d1 = (double) num1 / (double) den1;
    
                    LONG num2 = GetDWORD( (__int64) head.offset +  8 + headerBase, littleEndian );
                    LONG den2 = GetDWORD( (__int64) head.offset + 12 + headerBase, littleEndian );
                    double d2 = (double) num2 / (double) den2;
    
                    LONG num3 = GetDWORD( (__int64) head.offset + 16 + headerBase, littleEndian );
                    LONG den3 = GetDWORD( (__int64) head.offset + 20 + headerBase, littleEndian );
                    double d3 = (double) num3 / (double) den3;
    
                    g_Longitude = d1 + ( d2 / 60.0 ) + ( d3 / 3600.0 );
                }
            }
    
            IFDOffset = GetDWORD( IFDOffset + headerBase, littleEndian );
    
            if ( 0xffffffff == IFDOffset )
                break;
        }
    
        if ( latNeg )
            g_Latitude = -g_Latitude;
    
        if ( lonNeg )
            g_Longitude = -g_Longitude;
    } //EnumerateGPSTags
    
    void EnumerateNikonPreviewIFD( int depth, __int64 IFDOffset, __int64 headerBase, bool littleEndian )
    {
        vector<IFDHeader> aHeaders( MaxIFDHeaders );
        __int64 provisionalOffset = 0;

        while ( 0 != IFDOffset ) 
        {
            provisionalOffset = 0;

            WORD NumTags = GetWORD( IFDOffset + headerBase, littleEndian );
            IFDOffset += 2;
    
            if ( NumTags > MaxIFDHeaders )
                break;

            if ( !GetIFDHeaders( IFDOffset + headerBase, aHeaders.data(), NumTags, littleEndian ) )
                break;
        
            for ( int i = 0; i < NumTags; i++ )
            {
                IFDHeader & head = aHeaders[ i ];
                IFDOffset += sizeof IFDHeader;

                if ( 0x201 == head.id && 4 == head.type )
                {
                    if ( 0xffffffff != head.offset )
                        provisionalOffset = head.offset + headerBase;
                }
                else if ( 0x202 == head.id && 4 == head.type )
                {
                    if ( 0 != provisionalOffset && 0 != head.offset && 0xfffffffff != head.offset && head.offset > g_Embedded_Image_Length )
                    {
                        g_Embedded_Image_Offset = provisionalOffset;
                        g_Embedded_Image_Length = head.offset;
                    }
                }
            }
    
            IFDOffset = GetDWORD( IFDOffset + headerBase, littleEndian );
        }
    } //EnumerateNikonPreviewIFD
    
    void EnumerateNikonMakernotes( int depth, __int64 IFDOffset, __int64 headerBase, bool littleEndian )
    {
        // https://www.exiv2.org/tags-nikon.html
    
        __int64 originalNikonMakernotesOffset = IFDOffset - 8; // the -8 here is just from trial and error. But it works.
        vector<IFDHeader> aHeaders( MaxIFDHeaders );
    
        while ( 0 != IFDOffset ) 
        {
            WORD NumTags = GetWORD( IFDOffset + headerBase, littleEndian );
            IFDOffset += 2;
    
            if ( NumTags > MaxIFDHeaders )
                break;
        
            if ( !GetIFDHeaders( IFDOffset + headerBase, aHeaders.data(), NumTags, littleEndian ) )
                break;

            for ( int i = 0; i < NumTags; i++ )
            {
                IFDHeader & head = aHeaders[ i ];
                IFDOffset += sizeof IFDHeader;

                if ( 2 == head.id && 3 == head.type )
                {
                    g_ISO = head.offset;
                }
                else if ( 17 == head.id && 4 == head.type && 1 == head.count )
                {
                    // Nikon Preview IFD
    
                    // This "original - 8" in originalNikonMakernotesOffset is clearly a hack. But it woks on images from the D300, D70, and D100
                    // Note it's needed to correctly compute both the preview IFD start and the embedded JPG preview start
    
                    EnumerateNikonPreviewIFD( depth + 1, head.offset, originalNikonMakernotesOffset + headerBase, littleEndian );
                }
            }
    
            IFDOffset = GetDWORD( IFDOffset + headerBase, littleEndian );
        }
    } //EnumerateNikonMakernotes
    
    void EnumerateOlympusCameraSettingsIFD( int depth, __int64 IFDOffset, __int64 headerBase, bool littleEndian )
    {
        __int64 originalIFDOffset = IFDOffset;
        bool previewIsValid = false;
        vector<IFDHeader> aHeaders( MaxIFDHeaders );
    
        while ( 0 != IFDOffset ) 
        {
            WORD NumTags = GetWORD( IFDOffset + headerBase, littleEndian );
            IFDOffset += 2;
    
            if ( NumTags > MaxIFDHeaders )
                break;

            if ( !GetIFDHeaders( IFDOffset + headerBase, aHeaders.data(), NumTags, littleEndian ) )
                break;
        
            for ( int i = 0; i < NumTags; i++ )
            {
                IFDHeader & head = aHeaders[ i ];
                IFDOffset += sizeof IFDHeader;

                if ( 256 == head.id && 4 == head.type )
                {
                    previewIsValid = ( 0 != head.offset );  // literally the value is named PreviewImageValid
                }
                else if ( 257 == head.id && 4 == head.type )
                {
                    if ( previewIsValid )
                        g_Embedded_Image_Offset = head.offset + headerBase;
                }
                else if ( 258 == head.id && 4 == head.type )
                {
                    if ( previewIsValid )
                        g_Embedded_Image_Length = head.offset;
                }
            }
    
            IFDOffset = GetDWORD( IFDOffset + headerBase, littleEndian );
        }
    } //EnumerateOlympusCameraSettingsIFD
    
    void EnumerateFujifilmMakernotes( int depth, __int64 IFDOffset, __int64 headerBase, bool littleEndian )
    {
        // https://www.exiv2.org/tags-fujifilm.html
    
        vector<IFDHeader> aHeaders( MaxIFDHeaders );
    
        // In Fujifilm files, the base is not relative to the prior base; it's relative to the IFD start.
    
        DWORD tagHeaderBase = IFDOffset - 12;
    
        while ( 0 != IFDOffset ) 
        {
            WORD NumTags = GetWORD( IFDOffset + headerBase, littleEndian );
            IFDOffset += 2;
    
            if ( NumTags > MaxIFDHeaders )
                break;
        
            if ( !GetIFDHeaders( IFDOffset + headerBase, aHeaders.data(), NumTags, littleEndian ) )
                break;
    
            for ( int i = 0; i < NumTags; i++ )
            {
                IFDHeader & head = aHeaders[ i ];
                IFDOffset += sizeof IFDHeader;
    
                if ( 16 == head.id )
                {
                    ULONG stringOffset = ( head.count <= 4 ) ? ( IFDOffset - 4 ) : head.offset;
                    GetString( stringOffset + tagHeaderBase + headerBase, g_acSerialNumber, _countof( g_acSerialNumber ), head.count );
                    //tracer.Trace( "fujifilm makernote (alternate) Serial #: %s\n", g_acSerialNumber );
                }
            }
    
            IFDOffset = GetDWORD( IFDOffset + headerBase, littleEndian );
        }
    } //EnumerateFujifilmMakernotes

    static void DetectGarbage( char * pc )
    {
        char * pcIn = pc;
        while ( 0 != *pcIn )
        {
            if ( *pcIn < ' ' )
            {
                *pc = 0;
                return;
            }
            pcIn++;
        }
    } //DetectGarbage

    void EnumeratePanasonicMakernotes( int depth, __int64 IFDOffset, __int64 headerBase, bool littleEndian )
    {
        vector<IFDHeader> aHeaders( MaxIFDHeaders );
    
        while ( 0 != IFDOffset ) 
        {
            WORD NumTags = GetWORD( IFDOffset + headerBase, littleEndian );
            IFDOffset += 2;
    
            if ( NumTags > MaxIFDHeaders )
                break;
        
            if ( !GetIFDHeaders( IFDOffset + headerBase, aHeaders.data(), NumTags, littleEndian ) )
                break;
    
            // Note: Photomatix Pro 5.0.1 (64-bit) generates .tif files where these 3 strings are garbage.
            // Detect this case and make the strings empty.

            for ( int i = 0; i < NumTags; i++ )
            {
                IFDHeader & head = aHeaders[ i ];
                IFDOffset += sizeof IFDHeader;
                
                if ( 37 == head.id && 7 == head.type && 16 == head.count )
                {
                    // treat this as if it's a string even though it's a type 7 

                    ULONG stringOffset = ( head.count <= 4 ) ? ( IFDOffset - 4 ) : head.offset;
                    GetString( stringOffset + headerBase, g_acSerialNumber, _countof( g_acSerialNumber ), head.count );
                    DetectGarbage( g_acSerialNumber );
                }
                else if ( 81 == head.id && 2 == head.type )
                {
                    ULONG stringOffset = ( head.count <= 4 ) ? ( IFDOffset - 4 ) : head.offset;
                    GetString( stringOffset + headerBase, g_acLensModel, _countof( g_acLensModel ), head.count );
                    DetectGarbage( g_acLensModel );
                }
                else if ( 82 == head.id && 2 == head.type )
                {
                    ULONG stringOffset = ( head.count <= 4 ) ? ( IFDOffset - 4 ) : head.offset;
                    GetString( stringOffset + headerBase, g_acLensSerialNumber, _countof( g_acLensSerialNumber ), head.count );
                    DetectGarbage( g_acLensSerialNumber );
                }
            }
    
            IFDOffset = GetDWORD( IFDOffset + headerBase, littleEndian );
        }
    } //EnumeratePanasonicMakernotes

    void EnumerateMakernotes( int depth, __int64 IFDOffset, __int64 headerBase, bool littleEndian )
    {
        __int64 originalIFDOffset = IFDOffset;
    
        bool isLeica = false;
        bool isOlympus = false;
        bool isPentax = false;
        bool isNikon = false;
        bool isFujifilm = false;
        bool isRicoh = false;
        bool isRicohTheta = false;
        bool isEastmanKodak = false;
        bool isPanasonic = false;
        bool isApple = false;
        bool isSony = false;
        bool isCanon = false;
    
        // Note: Canon and Sony cameras have no manufacturer string filler before the IFD begins
    
        if ( !strcmp( g_acMake, "NIKON CORPORATION" ) )
        {
            IFDOffset += 10;
            WORD endian = GetWORD( IFDOffset + headerBase, littleEndian );
            littleEndian = ( 0x4d4d != endian );
    
            // https://www.exiv2.org/tags-nikon.html     Format 3 for D100
    
            IFDOffset += 8;
            isNikon = true;
    
            EnumerateNikonMakernotes( depth, IFDOffset, headerBase, littleEndian );
            return;
        }
        if ( !strcmp( g_acMake, "Nikon" ) )
        {
            IFDOffset += 10;
            WORD endian = GetWORD( IFDOffset + headerBase, littleEndian );
            littleEndian = ( 0x4d4d != endian );
    
            // https://www.exiv2.org/tags-nikon.html     Format 3 for D100
    
            IFDOffset += 8;
            isNikon = true;
    
            EnumerateNikonMakernotes( depth, IFDOffset, headerBase, littleEndian );
            return;
        }
        if ( !strcmp( g_acMake, "NIKON" ) )
        {
            IFDOffset += 10;
            WORD endian = GetWORD( IFDOffset + headerBase, littleEndian );
            littleEndian = ( 0x4d4d != endian );
    
            // https://www.exiv2.org/tags-nikon.html     Format 3 for D100
    
            IFDOffset += 8;
            isNikon = true;
    
            EnumerateNikonMakernotes( depth, IFDOffset, headerBase, littleEndian );
            return;
        }
        else if ( !strcmp( g_acMake, "LEICA CAMERA AG" ) )
        {
            IFDOffset += 8;
            isLeica = true;
        }
        else if ( !strcmp( g_acMake, "RICOH IMAGING COMPANY, LTD." ) )
        {
            // GR III, etc.
    
            IFDOffset += 8;
            isRicoh = true;
    
            if ( !strcmp( g_acModel, "PENTAX K-3 Mark III" ) )
                IFDOffset += 2;
        }
        else if ( !strcmp( g_acMake, "RICOH" ) )
        {
            // THETA, etc.
    
            IFDOffset += 8;
            isRicohTheta = true;
        }
        else if ( !strcmp( g_acMake, "PENTAX" ) )
        {
            IFDOffset += 6;
            isPentax = true;
        }
        else if ( !strcmp( g_acMake, "OLYMPUS IMAGING CORP." ) )
        {
            IFDOffset += 12;
            isOlympus = true;
        }
        else if ( !strcmp( g_acMake, "OLYMPUS CORPORATION" ) )
        {
            IFDOffset += 12;
            isOlympus = true;
        }
        else if ( !strcmp( g_acMake, "Eastman Kodak Company" ) )
        {
            isEastmanKodak = true;
            return; // apparently unparsable
        }
        else if ( !strcmp( g_acMake, "FUJIFILM" ) )
        {
            IFDOffset += 12;
            isFujifilm = true;
    
            EnumerateFujifilmMakernotes( depth, IFDOffset, headerBase, littleEndian );
            return;
        }
        else if ( !strcmp( g_acMake, "Panasonic" ) )
        {
            IFDOffset += 12;
            isPanasonic = true;

            EnumeratePanasonicMakernotes( depth, IFDOffset, headerBase, littleEndian );
            return;
        }
        else if ( !strcmp( g_acMake, "Apple" ) )
        {
            if ( !strcmp( g_acModel, "iPhone 12" ) )
            {
                IFDOffset += 14;
                littleEndian = false;
            }
            else
                IFDOffset += 14;
    
            isApple = true;
        }
        else if ( !strcmp( g_acMake, "SONY" ) ) // real camera
        {
            isSony = true;
        }
        else if ( !strcmp( g_acMake, "Sony" ) ) // cellphone
        {
            IFDOffset += 12;
            isSony = true;
        }
        else if ( !strcmp( g_acMake, "CANON" ) )
        {
            isCanon = true;
        }
        else if ( !strcmp( g_acMake, "" ) )
        {
            if ( !strcmp( g_acModel, "DMC-GM1" ) )
            {
                IFDOffset += 12;
                isPanasonic = true;
            }
        }
    
        vector<IFDHeader> aHeaders( MaxIFDHeaders );

        while ( 0 != IFDOffset ) 
        {
            WORD NumTags = GetWORD( IFDOffset + headerBase, littleEndian );
            IFDOffset += 2;
    
            // the file is problematic if this is true

            if ( NumTags > MaxIFDHeaders )
                break;
        
            if ( !GetIFDHeaders( IFDOffset + headerBase, aHeaders.data(), NumTags, littleEndian ) )
                break;
        
            for ( int i = 0; i < NumTags; i++ )
            {
                IFDHeader & head = aHeaders[ i ];
                IFDOffset += sizeof IFDHeader;

                if ( 5 == head.id && 7 == head.type && isRicohTheta )
                {
                    if ( head.count < ( _countof( g_acSerialNumber ) - 1 ) )
                    {
                        GetBytes( head.offset + headerBase, g_acSerialNumber, head.count );
                        g_acSerialNumber[ head.count ] = 0;
                    }
                }
                else if ( 12 == head.id && 4 == head.type && 1 == head.count && isCanon )
                {
                    if ( 0 == g_acSerialNumber )
                    {
                        sprintf_s( g_acSerialNumber, _countof( g_acSerialNumber ), "%d", head.offset );
                        //tracer.Trace( "canon makernote serial number: %s\n", g_acSerialNumber );
                    }
                }
                else if ( 224 == head.id && 17 == head.count )
                {
                    struct SensorData
                    {
                        short blank1, width, height, blank2, blank3, lborder, tborder, rborder, bborder;
                    };

                    SensorData sd;
                    GetBytes( head.offset + headerBase, &sd, sizeof sd );

                    short sensorWidth = FixEndianWORD( sd.width, littleEndian );
                    short sensorHeight = FixEndianWORD( sd.height, littleEndian );
    
                    short leftBorder = FixEndianWORD( sd.lborder, littleEndian );
                    short topBorder = FixEndianWORD( sd.tborder, littleEndian );
                    short rightBorder = FixEndianWORD( sd.rborder, littleEndian );
                    short bottomBorder = FixEndianWORD( sd.bborder, littleEndian );
                }
                else if ( 553 == head.id && isRicoh )
                {
                    if ( 2 == head.type )
                    {
                        ULONG stringOffset = ( head.count <= 4 ) ? ( IFDOffset - 4 ) : head.offset;
    
                        // Ricoh assumes the base is originalIFDOffset

                        GetString( originalIFDOffset + stringOffset + headerBase, g_acSerialNumber, _countof( g_acSerialNumber ), head.count );
                    }
                }
                else if ( 1280 == head.id && isLeica )
                {
                    //char acInternalSerialNumber[ 100 ];
                    //ULONG stringOffset = ( head.count <= 4 ) ? ( IFDOffset - 4 ) : head.offset;
                    //GetString( stringOffset + headerBase, acInternalSerialNumber, _countof( acInternalSerialNumber ), head.count );
                    //tracer.Trace( "leica makernote InternalSerialNumber count %d: %s\n", head.count, acInternalSerialNumber );
                }
                else if ( 8224 == head.id && 13 == head.type && isOlympus )
                {
                    EnumerateOlympusCameraSettingsIFD( depth + 1, head.offset, originalIFDOffset + headerBase, littleEndian );
                }
            }
    
            IFDOffset = GetDWORD( IFDOffset + headerBase, littleEndian );
        }
    } //EnumerateMakernotes
    
    void EnumerateExifTags( int depth, __int64 IFDOffset, __int64 headerBase, bool littleEndian )
    {
        DWORD XResolutionNum = 0;
        DWORD XResolutionDen = 0;
        DWORD YResolutionNum = 0;
        DWORD YResolutionDen = 0;
        DWORD sensorSizeUnit = 0; // 2==inch, 3==centimeter
        double focalLength = 0.0;
        DWORD pixelWidth = 0;
        DWORD pixelHeight = 0;
        vector<IFDHeader> aHeaders( MaxIFDHeaders );
    
        while ( 0 != IFDOffset ) 
        {
            WORD NumTags = GetWORD( IFDOffset + headerBase, littleEndian );
            IFDOffset += 2;

            // the file is problematic if this is true

            if ( NumTags > MaxIFDHeaders )
                break;
        
            if ( !GetIFDHeaders( IFDOffset + headerBase, aHeaders.data(), NumTags, littleEndian ) )
                break;
        
            for ( int i = 0; i < NumTags; i++ )
            {
                IFDHeader & head = aHeaders[ i ];
                IFDOffset += sizeof IFDHeader;

                if ( 33434 == head.id && 5 == head.type )
                {
                    TwoDWORDs td;
                    GetTwoDWORDs( head.offset + headerBase, &td, littleEndian );
                    g_ExposureNum = td.dw1;
                    g_ExposureDen = td.dw2;
                }
                else if ( 33434 == head.id && 4 == head.type )
                {
                    g_ExposureNum = 1;
                    g_ExposureDen = head.offset;
                }
                else if ( 33437 == head.id && 5 == head.type )
                {
                    TwoDWORDs td;
                    GetTwoDWORDs( head.offset + headerBase, &td, littleEndian );
                    g_ApertureNum = td.dw1;
                    g_ApertureDen = td.dw2;
                }
                else if ( 34850 == head.id )
                    g_ExposureProgram = head.offset;
                else if ( 34855 == head.id )
                    g_ISO = head.offset;
                else if ( 36867 == head.id && 2 == head.type )
                {
                    __int64 stringOffset = ( head.count <= 4 ) ? ( IFDOffset - 4 ) : head.offset;
                    GetString( stringOffset + headerBase, g_acDateTimeOriginal, _countof( g_acDateTimeOriginal ), head.count );
                }
                else if ( 37386 == head.id && 5 == head.type )
                {
                    TwoDWORDs td;
                    GetTwoDWORDs( head.offset + headerBase, &td, littleEndian );
                    g_FocalLengthNum = td.dw1; 
                    g_FocalLengthDen = td.dw2; 
                }
                else if ( 37500 == head.id )
                {
                    EnumerateMakernotes( depth + 1, head.offset, headerBase, littleEndian );
                }
                else if ( 40962 == head.id )
                {
                    pixelWidth = head.offset;
    
                    if ( (int) head.offset > g_ImageWidth )
                        g_ImageWidth = head.offset;
                }
                else if ( 40963 == head.id )
                {
                    pixelHeight = head.offset;
    
                    if ( (int) head.offset > g_ImageHeight )
                        g_ImageHeight = head.offset;
                }
                else if ( 41486 == head.id )
                {
                    TwoDWORDs td;
                    GetTwoDWORDs( head.offset + headerBase, &td, littleEndian );
                    XResolutionNum = td.dw1;
                    XResolutionDen = td.dw2;
                }
                else if ( 41487 == head.id )
                {
                    TwoDWORDs td;
                    GetTwoDWORDs( head.offset + headerBase, &td, littleEndian );
                    YResolutionNum = td.dw1;
                    YResolutionDen = td.dw2;
                }
                else if ( 41488 == head.id )
                {
                    sensorSizeUnit = head.offset;
                }
                else if ( 41986 == head.id )
                {
                    g_ExposureMode = head.offset;
                }
                else if ( 41989 == head.id )
                {
                    g_FocalLengthIn35mmFilm = head.offset;
                }
                else if ( 42033 == head.id && 2 == head.type )
                {
                    __int64 stringOffset = ( head.count <= 4 ) ? ( IFDOffset - 4 ) : head.offset;
                    GetString( stringOffset + headerBase, g_acSerialNumber, _countof( g_acSerialNumber ), head.count );
                    //tracer.Trace( "exif Body Serial Number: %s\n", g_acSerialNumber );
                }
                else if ( 42035 == head.id && 2 == head.type )
                {
                    __int64 stringOffset = ( head.count <= 4 ) ? ( IFDOffset - 4 ) : head.offset;
                    GetString( stringOffset + headerBase, g_acLensMake, _countof( g_acLensMake ), head.count );
                }
                else if ( 42036 == head.id && 2 == head.type )
                {
                    __int64 stringOffset = ( head.count <= 4 ) ? ( IFDOffset - 4 ) : head.offset;
                    GetString( stringOffset + headerBase, g_acLensModel, _countof( g_acLensModel ), head.count );
                }
                else if ( 42037 == head.id && 2 == head.type )
                {
                    __int64 stringOffset = ( head.count <= 4 ) ? ( IFDOffset - 4 ) : head.offset;
                    GetString( stringOffset + headerBase, g_acLensSerialNumber, _countof( g_acLensSerialNumber ), head.count );
                    //tracer.Trace( "exif Lens Serial Number: %s\n", g_acLensSerialNumber );
                }
            }
    
            IFDOffset = GetDWORD( IFDOffset + headerBase, littleEndian );
        }
    
        if ( 0 != XResolutionNum && 0 != XResolutionDen && 0 != YResolutionNum && 0 != YResolutionDen && 0 != sensorSizeUnit &&
             0 != pixelWidth && 0 != pixelHeight)
        {
            //tracer.Trace( "XResolutionNum %d, XResolutionDen %d, YResoltuionNum %d, YResolutionDen %d\n", XResolutionNum, XResolutionDen, YResolutionNum, YResolutionDen );
            //tracer.Trace( "pixel dimension: %d %d\n", pixelWidth, pixelHeight );
    
            double resolutionX = (double) XResolutionNum / (double) XResolutionDen;
            double resolutionY = (double) YResolutionNum / (double) YResolutionDen;
    
            //tracer.Trace( "ppu X %lf, Y %lf, sensor size unit %d\n", resolutionX, resolutionY, sensorSizeUnit );
    
            double unitsX = (double) pixelWidth / resolutionX;
            double unitsY = (double) pixelHeight / resolutionY;
    
            double sensorSizeXmm = 0.0;
            double sensorSizeYmm = 0.0;
    
            if ( 2 == sensorSizeUnit )
            {
                // inches
    
                sensorSizeXmm = unitsX * 25.4;
                sensorSizeYmm = unitsY * 25.4;
            }
            else if ( 3 == sensorSizeUnit )
            {
                sensorSizeXmm = unitsX * 10.0;
                sensorSizeYmm = unitsY * 10.0;
            }
            else // 4==mm 5==um perhaps?
            {
            }
    
            //tracer.Trace( "exif (Computed) sensor WxH:         %lf, %lf\n", sensorSizeXmm, sensorSizeYmm );
    
            g_ComputedSensorWidth = (int) round( sensorSizeXmm );
            g_ComputedSensorHeight = (int) round( sensorSizeYmm );
        }
    } //EnumerateExifTags

    void EnumerateGenericIFD( int depth, __int64 IFDOffset, __int64 headerBase, bool littleEndian )
    {
        __int64 provisionalJPGOffset = 0;
        __int64 provisionalJPGFromRAWOffset = 0;
        int currentIFD = 0;
        bool likelyRAW = false;
        vector<IFDHeader> aHeaders( MaxIFDHeaders );
    
        while ( 0 != IFDOffset ) 
        {
            provisionalJPGOffset = 0;
            provisionalJPGFromRAWOffset = 0;
    
            WORD NumTags = GetWORD( IFDOffset + headerBase, littleEndian );
            IFDOffset += 2;
    
            // the file is problematic if this is true

            if ( NumTags > MaxIFDHeaders )
                break;
        
            if ( !GetIFDHeaders( IFDOffset + headerBase, aHeaders.data(), NumTags, littleEndian ) )
                break;
        
            for ( int i = 0; i < NumTags; i++ )
            {
                IFDHeader & head = aHeaders[ i ];
                IFDOffset += sizeof IFDHeader;

                //tracer.Trace( "genericifd head.id %d\n", head.id );
    
                if ( 254 == head.id && 4 == head.type )
                {
                    likelyRAW = ( 0 == ( 1 & head.offset ) );
                }
                else if ( 256 == head.id && IsIntType( head.type ) )
                {
                    if ( (int) head.offset > g_ImageWidth )
                        g_ImageWidth = head.offset;
                }
                else if ( 257 == head.id && IsIntType( head.type ) )
                {
                    if ( (int) head.offset > g_ImageHeight )
                        g_ImageHeight = head.offset;
                }
                else if ( 273 == head.id && IsIntType( head.type ) )
                {
                    if ( 0 != head.offset && 0xffffffff != head.offset && IsPerhapsAnImage( head.offset, headerBase ) && !likelyRAW )
                        provisionalJPGOffset = head.offset + headerBase;
                }
                else if ( 279 == head.id && IsIntType( head.type ) )
                {
                    if ( 0 != provisionalJPGOffset && 0 != head.offset && 0xffffffff != head.offset && !likelyRAW && head.offset > g_Embedded_Image_Length )
                    {
                        g_Embedded_Image_Length = head.offset;
                        g_Embedded_Image_Offset = provisionalJPGOffset;
                    }
                }
                else if ( 513 == head.id && IsIntType( head.type ) )
                {
                    if ( 0 != head.offset && 0xffffffff != head.offset && IsPerhapsAnImage( head.offset, headerBase ) && !likelyRAW )
                        provisionalJPGFromRAWOffset = head.offset + headerBase;
    
                }
                else if ( 514 == head.id && IsIntType( head.type ) )
                {
                    if ( 0 != head.offset && 0xffffffff != head.offset && 0 != provisionalJPGFromRAWOffset && ( head.offset > g_Embedded_Image_Length ) )
                    {
                        g_Embedded_Image_Length = head.offset;
                        g_Embedded_Image_Offset = provisionalJPGFromRAWOffset;
                    }
                }
            }
    
            IFDOffset = GetDWORD( IFDOffset + headerBase, littleEndian );
            currentIFD++;
        }
    } //EnumerateGenericIFD
    
    void GetPanasonicIFD0Tag( int depth, WORD tagID, WORD tagType, DWORD tagCount, DWORD tagOffset, __int64 headerBase, bool littleEndian, __int64 IFDOffset )
    {
        if ( 2 == tagID )
        {
            if ( (int) tagOffset > g_ImageWidth )
                g_ImageWidth = tagOffset;
        }
        else if ( 3 == tagID )
        {
            if ( (int) tagOffset > g_ImageHeight )
                g_ImageHeight = tagOffset;
        }
        else if ( 23 == tagID )
            g_ISO = tagOffset;
        else if ( 46 == tagID )
        {
            g_Embedded_Image_Offset = tagOffset;
            g_Embedded_Image_Length = tagCount;
        }
    } //GetPanasonicIFD0Tag
    
    void EnumerateFlac()
    {
        // Data is big-endian!
    
        ULONG offset = 4;
    
        do
        {
            ULONG blockHeader = GetDWORD( offset, false );
            offset += 4;
    
            bool lastBlock = ( 0 != ( 0x80000000 & blockHeader ) );
            BYTE blockType = ( ( blockHeader >> 24 ) & 0x7f );
            ULONG length = blockHeader & 0xffffff;
    
            // protect against malformed files
    
            if ( 0 == length )
                break;
    
            if ( 6 == blockType )
            {
                // Picture
    
                DWORD o = offset;
    
                DWORD pictureType = GetDWORD( o, false );
    
                o += 4;
    
                DWORD mimeTypeBytes = GetDWORD( o, false );
                if ( mimeTypeBytes > 1000 )
                    return;
    
                o += 4;
                o += mimeTypeBytes;
    
                DWORD descriptionBytes = GetDWORD( o, false );
                if ( descriptionBytes > 1000 )
                    return;
    
                o += 4;
                o += descriptionBytes;
    
                DWORD width = GetDWORD( o, false );
                g_ImageWidth = width;
                o += 4;
    
                DWORD height = GetDWORD( o, false );
                g_ImageHeight = height;
                o += 4;
    
                DWORD bpp = GetDWORD( o, false );
                o += 4;
    
                DWORD indexedColors = GetDWORD( o, false );
                o += 4;
    
                DWORD imageLength = GetDWORD( o, false );
                o += 4;
    
                if ( ( imageLength > 2 ) && IsPerhapsAnImage( o, 0 ) )
                {
                    g_Embedded_Image_Offset = o;
                    g_Embedded_Image_Length = imageLength;

                    // we've got the image and that's all this function does. return now.
                    return;
                }
            }
    
            offset += length;
    
            if ( lastBlock )
                break;
        } while ( true );
    } //EnumerateFlac
    
    class HeifStream
    {
        private:
            __int64 offset;
            __int64 length;
            CStream * pStream;
    
        public:
            HeifStream( CStream *ps, __int64 o, __int64 l )
            {
                offset = o;
                length = l;
                pStream = ps;
            }
    
            __int64 Length() { return length; }
            __int64 Offset() { return offset; }
            CStream * Stream() { return pStream; }
    
            DWORD GetDWORD( __int64 & streamOffset, bool littleEndian )
            {
                DWORD dw = 0;

                if ( pStream->Seek( offset + streamOffset ) )
                {
                    pStream->Read( &dw, sizeof dw );
    
                    if ( !littleEndian )
                        dw = _byteswap_ulong( dw );
    
                    streamOffset += sizeof dw;
                }
    
                return dw;
            } //GetDWORD
    
            WORD GetWORD( __int64 & streamOffset, bool littleEndian )
            {
                WORD w = 0;

                if ( pStream->Seek( offset + streamOffset ) )
                {
                    pStream->Read( &w, sizeof w );
            
                    if ( !littleEndian )
                        w = _byteswap_ushort( w );
    
                    streamOffset += sizeof w;
                }
            
                return w;
            } //GetWORD
    
            byte GetBYTE( __int64 & streamOffset )
            {
                byte b = 0;

                if ( pStream->Seek( offset + streamOffset ) )
                {
                    pStream->Read( &b, sizeof b );
    
                    streamOffset += sizeof b;
                }
    
                return b;
            } //GetBYTE
    }; //HeifStream
    
    // Heif and CR3 use ISO Base Media File Format ISO/IEC 14496-12. This function walks those files and pulls out data including Exif offsets
    
    void EnumerateBoxes( HeifStream & hs, DWORD depth )
    {
        //tracer.Trace( "EnumerateBoxes at depth %d\n", depth );
    
        __int64 offset = 0;
        DWORD box = 0;
        int indent = depth * 2;
    
        do
        {
            if ( ( 0 == hs.Length() ) || ( offset >= hs.Length() ) )
                break;
    
            __int64 boxOffset = offset;
            ULONGLONG boxLen = hs.GetDWORD( offset, false );
    
            if ( 0 == boxLen )
                break;
    
            DWORD dwTag = hs.GetDWORD( offset, false );
            char tag[ 5 ];
            tag[ 3 ] = dwTag & 0xff;
            tag[ 2 ] = ( dwTag >> 8 ) & 0xff;
            tag[ 1 ] = ( dwTag >> 16 ) & 0xff;
            tag[ 0 ] = ( dwTag >> 24 ) & 0xff;
            tag[ 4 ] = 0;
    
            if ( 1 == boxLen )
            {
                ULONGLONG high = hs.GetDWORD( offset, false );
                ULONGLONG low = hs.GetDWORD( offset, false );
    
                boxLen = ( high << 32 ) | low;
            }
    
            if ( 0 == boxLen )
                break;
    
            //tracer.Trace( "  tag %s\n", tag );
    
            if ( !strcmp( tag, "ftyp" ) )
            {
                char brand[ 5 ];
                for ( int i = 0; i < 4; i++ )
                    brand[ i ] = (char) hs.GetBYTE( offset );
                brand[ 4 ] = 0;
    
                DWORD version = hs.GetDWORD( offset, false );
            }
            else if ( !strcmp( tag, "meta" ) )
            {
                DWORD data = hs.GetDWORD( offset, true );
    
                HeifStream hsChild( hs.Stream(), hs.Offset() + offset, boxLen - ( offset - boxOffset ) );
    
                EnumerateBoxes( hsChild, depth + 1 );
            }
            else if ( !strcmp( tag, "iinf" ) )
            {
                DWORD data = hs.GetDWORD( offset, false );
                BYTE version = (BYTE) ( ( data >> 24 ) & 0xff );
                DWORD flags = data & 0x00ffffff;
    
                DWORD entriesSize = ( version > 0 ) ? 4 : 2;
                DWORD entries = 0;
    
                if ( 2 == entriesSize )
                    entries = hs.GetWORD( offset, false );
                else
                    entries = hs.GetDWORD( offset, false );
    
                if ( entries > 0 )
                {
                    HeifStream hsChild( hs.Stream(), hs.Offset() + offset, boxLen - ( offset - boxOffset ) );
    
                    EnumerateBoxes( hsChild, depth + 1 );
                }
            }
            else if ( !strcmp( tag, "infe" ) )
            {
                DWORD data = hs.GetDWORD( offset, false );
                BYTE version = (BYTE) ( ( data >> 24 ) & 0xff );
                DWORD flags = data & 0x00ffffff;
    
                if ( version <= 1 )
                {
                    WORD itemID = hs.GetWORD( offset, false );
                    WORD itemProtection = hs.GetWORD( offset, false );
                }
                else if ( version >= 2 )
                {
                    bool hiddenItem = ( 0 != (flags & 0x1 ) );
    
                    ULONG itemID = 0;
    
                    if ( 2 == version )
                        itemID = hs.GetWORD( offset, false );
                    else
                        itemID = hs.GetDWORD( offset, false );
    
                    WORD protectionIndex = hs.GetWORD( offset, false );
    
                    char itemType[ 5 ];
                    for ( int i = 0; i < 4; i++ )
                        itemType[ i ] = hs.GetBYTE( offset );
                    itemType[ 4 ] = 0;
    
                    if ( !strcmp( itemType, "Exif" ) )
                        g_Heif_Exif_ItemID = itemID;
                }
            }
            else if ( !strcmp( tag, "iloc" ) )
            {
                DWORD data = hs.GetDWORD( offset, false );
                BYTE version = (BYTE) ( ( data >> 24 ) & 0xff );
                DWORD flags = data & 0x00ffffff;
    
                WORD values4 = hs.GetWORD( offset, false );
                int offsetSize = ( values4 >> 12 ) & 0xf;
                int lengthSize = ( values4 >> 8 ) & 0xf;
                int baseOffsetSize = ( values4 >> 4 ) & 0xf;
                int indexSize = 0;
    
                if ( version > 1 )
                    indexSize = ( values4 & 0xf );
    
                int itemCount = 0;
    
                if ( version < 2 )
                    itemCount = hs.GetWORD( offset, false );
                else
                    itemCount = hs.GetDWORD( offset, false );
    
                for ( int i = 0; i < itemCount; i++ )
                {
                    int itemID = 0;
    
                    if ( version < 2 )
                        itemID = hs.GetWORD( offset, false );
                    else
                        itemID = hs.GetWORD( offset, false );
    
                    BYTE constructionMethod = 0;
    
                    if ( version >= 1 )
                    {
                        values4 = hs.GetWORD( offset, false );
    
                        constructionMethod = ( values4 & 0xf );
                    }
    
                    WORD dataReferenceIndex = hs.GetWORD( offset, false );
    
                    __int64 baseOffset = 0;
    
                    if ( 4 == baseOffsetSize )
                    {
                        baseOffset = hs.GetDWORD( offset, false );
                    }
                    else if ( 8 == baseOffsetSize )
                    {
                        __int64 high = hs.GetDWORD( offset, false );
                        __int64 low = hs.GetDWORD( offset, false );
    
                        baseOffset = ( high << 32 ) | low;
                    }
    
                    WORD extentCount = hs.GetWORD( offset, false );
    
                    for ( int e = 0; e < extentCount; e++ )
                    {
                        __int64 extentIndex = 0;
                        __int64 extentOffset = 0;
                        __int64 extentLength = 0;
    
                        if ( version > 1 && indexSize > 0 )
                        {
                            if ( 4 == indexSize )
                            {
                                extentIndex = hs.GetDWORD( offset, false );
                            }
                            else if ( 8 == indexSize )
                            {
                                __int64 high = hs.GetDWORD( offset, false );
                                __int64 low = hs.GetDWORD( offset, false );
    
                                extentIndex = ( high << 32 ) | low;
                            }
                        }
    
                        if ( 4 == offsetSize )
                        {
                            extentOffset = hs.GetDWORD( offset, false );
                        }
                        else if ( 8 == offsetSize )
                        {
                            __int64 high = hs.GetDWORD( offset, false );
                            __int64 low = hs.GetDWORD( offset, false );
    
                            extentOffset = ( high << 32 ) | low;
                        }
    
                        if ( 4 == lengthSize )
                        {
                            extentLength = hs.GetDWORD( offset, false );
                        }
                        else if ( 8 == lengthSize )
                        {
                            __int64 high = hs.GetDWORD( offset, false );
                            __int64 low = hs.GetDWORD( offset, false );
    
                            extentLength = ( high << 32 ) | low;
                        }
    
                        if ( itemID == g_Heif_Exif_ItemID )
                        {
                            g_Heif_Exif_Offset = extentOffset;
                            g_Heif_Exif_Length = extentLength;
                        }
                    }
                }
            }
            else if ( !strcmp( tag, "hvcC" ) )
            {
                BYTE version = hs.GetBYTE( offset );
                BYTE byteInfo = hs.GetBYTE( offset );
                BYTE profileSpace = ( byteInfo >> 6 ) & 3;
                bool tierFlag = 0 != ( ( byteInfo >> 4 ) & 0x1 );
                BYTE  profileIDC = byteInfo & 0x1f;
    
                DWORD profileCompatibilityFlags = hs.GetDWORD( offset, false );
    
                for ( int i = 0; i < 6; i++ )
                {
                    BYTE b = hs.GetBYTE( offset );
                }
    
                BYTE generalLevelIDC = hs.GetBYTE( offset );
                WORD minSpacialSegmentationIDC = hs.GetWORD( offset, false ) & 0xfff;
                BYTE parallelismType = hs.GetBYTE( offset ) & 0x3;
                BYTE chromaFormat = hs.GetBYTE( offset ) & 0x3;
                BYTE bitDepthLuma = ( hs.GetBYTE( offset ) & 0x7 ) + 8;
                BYTE bitDepthChroma = ( hs.GetBYTE( offset ) & 0x7 ) + 8;
                WORD avgFrameRate = hs.GetWORD( offset, false );
            }
            else if ( !strcmp( tag, "iprp" ) )
            {
                HeifStream hsChild( hs.Stream(), hs.Offset() + offset, boxLen - ( offset - boxOffset ) );
    
                EnumerateBoxes( hsChild, depth + 1 );
            }
            else if ( !strcmp( tag, "ipco" ) )
            {
                HeifStream hsChild( hs.Stream(), hs.Offset() + offset, boxLen - ( offset - boxOffset ) );
    
                EnumerateBoxes( hsChild, depth + 1 );
            }
            else if ( !strcmp( tag, "moov" ) ) // Canon CR3 format
            {
                HeifStream hsChild( hs.Stream(), hs.Offset() + offset, boxLen - ( offset - boxOffset ) );
    
                EnumerateBoxes( hsChild, depth + 1 );
            }
            else if ( !strcmp( tag, "uuid" ) )
            {
                char acGUID[ 33 ];
                acGUID[ 32 ] = 0;
    
                for ( size_t i = 0; i < 16; i++ )
                {
                    BYTE x = hs.GetBYTE( offset );
                    sprintf_s( acGUID + i * 2, 3, "%02x", x );
                }
    
                if ( !strcmp( "85c0b687820f11e08111f4ce462b6a48", acGUID ) )
                {
                    // Canon CR3, will contain CNCV, CCTP, CTBO, CMT1, CMD2, CMD3, CMT4, etc.
    
                    HeifStream hsChild( hs.Stream(), hs.Offset() + offset, boxLen - ( offset - boxOffset ) );
    
                    EnumerateBoxes( hsChild, depth + 1 );
                }
    
                if ( !strcmp( "eaf42b5e1c984b88b9fbb7dc406e4d16", acGUID ) )
                {
                    // preview data -- a reduced-resolution jpg
    
                    HeifStream hsChild( hs.Stream(), hs.Offset() + offset, boxLen - ( offset - boxOffset ) );
    
                    EnumerateBoxes( hsChild, depth + 1 );
                }
            }
            else if ( !strcmp( tag, "CMT1" ) )
            {
                g_Canon_CR3_Exif_IFD0 = hs.Offset() + offset;
            }
            else if ( !strcmp( tag, "CMT2" ) )
            {
                g_Canon_CR3_Exif_Exif_IFD = hs.Offset() + offset;
            }
            else if ( !strcmp( tag, "CMT3" ) )
            {
                g_Canon_CR3_Exif_Makernotes_IFD = hs.Offset() + offset;
            }
            else if ( !strcmp( tag, "CMT4" ) )
            {
                g_Canon_CR3_Exif_GPS_IFD = hs.Offset() + offset;
            }
            else if ( !strcmp( tag, "PRVW" ) )
            {
                DWORD unk = hs.GetDWORD( offset, false );
                WORD unkW = hs.GetWORD( offset, false );
                WORD width = hs.GetWORD( offset, false );
                WORD height = hs.GetWORD( offset, false );
                unkW = hs.GetWORD( offset, false );
                DWORD length = hs.GetDWORD( offset, false );
    
                // This should work per https://github.com/exiftool/canon_cr3, but it doesn't exist
    
                g_Embedded_Image_Length = length;
                g_Embedded_Image_Offset = offset + hs.Offset();
            }
            else if ( !strcmp( tag, "mdat" ) ) // Canon .CR3 main data
            {
                __int64 jpgOffset = hs.Offset() + offset;
                DWORD head = hs.GetDWORD( offset, false );
    
                if ( ( 0xffd8ffdb == head ) && // looks like JPG
                     ( 0 != g_Canon_CR3_Embedded_JPG_Length ) )
                {
                    g_Embedded_Image_Length = g_Canon_CR3_Embedded_JPG_Length;
                    g_Embedded_Image_Offset = jpgOffset;
                }
            }
            else if ( !strcmp( tag, "trak" ) ) // Canon .CR3 metadata
            {
                HeifStream hsChild( hs.Stream(), hs.Offset() + offset, boxLen - ( offset - boxOffset ) );
                EnumerateBoxes( hsChild, depth + 1 );
            }
            else if ( !strcmp( tag, "mdia" ) ) // Canon .CR3 metadata
            {
                HeifStream hsChild( hs.Stream(), hs.Offset() + offset, boxLen - ( offset - boxOffset ) );
                EnumerateBoxes( hsChild, depth + 1 );
            }
            else if ( !strcmp( tag, "minf" ) ) // Canon .CR3 metadata
            {
                HeifStream hsChild( hs.Stream(), hs.Offset() + offset, boxLen - ( offset - boxOffset ) );
                EnumerateBoxes( hsChild, depth + 1 );
            }
            else if ( !strcmp( tag, "stbl" ) ) // Canon .CR3 metadata
            {
                HeifStream hsChild( hs.Stream(), hs.Offset() + offset, boxLen - ( offset - boxOffset ) );
                EnumerateBoxes( hsChild, depth + 1 );
            }
            else if ( !strcmp( tag, "stsz" ) ) // Canon .CR3 metadata
            {
                // There is no documentation, but it appears that the 1st of 4 instances of
                // the stsz tag has the full-resolution embedded JPG length at i==3 in this
                // loop. I mean, it worked for one file.
    
                for ( int i = 0; i < 5; i++ )
                {
                    DWORD len = hs.GetDWORD( offset, false );
    
                    if ( ( 3 == i ) && ( 0 == g_Canon_CR3_Embedded_JPG_Length ) )
                        g_Canon_CR3_Embedded_JPG_Length = len;
                }
            }
    
            box++;
            offset = boxOffset + boxLen;
        } while ( true );
    
        //tracer.Trace( "falling out of EnumerateBoxes at depth %d\n", depth );
    } //EnumerateBoxes
    
    void EnumerateHeif( CStream * pStream )
    {
        __int64 length = pStream->Length();
        HeifStream hs( pStream, 0, length );
    
        EnumerateBoxes( hs, 0 );
    } //EnumerateHeif
    
    void EnumerateIFD0( int depth, __int64 IFDOffset, __int64 headerBase, bool littleEndian, WCHAR const * pwcExt )
    {
        int currentIFD = 0;
        __int64 provisionalJPGOffset = 0;
        __int64 provisionalEmbeddedJPGOffset = 0;
        bool likelyRAW = false;
        int lastBitsPerSample = 0;
        vector<IFDHeader> aHeaders( MaxIFDHeaders );

        while ( 0 != IFDOffset ) 
        {
            provisionalJPGOffset = 0;
            provisionalEmbeddedJPGOffset = 0;
    
            WORD NumTags = GetWORD( IFDOffset + headerBase, littleEndian );
            IFDOffset += 2;
        
            if ( NumTags > MaxIFDHeaders )
                break;

            if ( !GetIFDHeaders( IFDOffset + headerBase, aHeaders.data(), NumTags, littleEndian ) )
                break;

            for ( int i = 0; i < NumTags; i++ )
            {
                IFDHeader & head = aHeaders[ i ];
                IFDOffset += sizeof IFDHeader;

                if ( ( !_wcsicmp( pwcExt, L".rw2" ) ) && ( ( head.id < 254 ) || ( head.id >= 280 && head.id <= 290 ) ) )
                {
                    GetPanasonicIFD0Tag( depth, head.id, head.type, head.count, head.offset, headerBase, littleEndian, IFDOffset );
                    continue;
                }
    
                //tracer.Trace( "ifd0 tag # %d, head.id %d\n", i, head.id );
    
                if ( 254 == head.id && IsIntType( head.type ) )
                {
                    likelyRAW = ( 0 == ( 1 & head.offset ) );
                }
                else if ( 256 == head.id && IsIntType( head.type ) )
                {
                    if ( (int) head.offset > g_ImageWidth )
                        g_ImageWidth = head.offset;
                }
                else if ( 257 == head.id && IsIntType( head.type ) )
                {
                    if ( (int) head.offset > g_ImageHeight )
                        g_ImageHeight = head.offset;
                }
                else if ( 258 == head.id && 3 == head.type && 3 == head.count )
                {
                    // read the first one
                    lastBitsPerSample = GetWORD( head.offset + headerBase, littleEndian );
                }
                else if ( 258 == head.id && 3 == head.type && 1 == head.count )
                {
                    lastBitsPerSample = head.offset;
                }
                else if ( 271 == head.id  && 2 == head.type )
                {
                    __int64 stringOffset = ( head.count <= 4 ) ? ( IFDOffset - 4 ) : head.offset;
                    GetString( stringOffset + headerBase, g_acMake, _countof( g_acMake ), head.count );
                }
                else if ( 272 == head.id && 2 == head.type )
                {
                    __int64 stringOffset = ( head.count <= 4 ) ? ( IFDOffset - 4 ) : head.offset;
                    GetString( stringOffset + headerBase, g_acModel, _countof( g_acModel ), head.count );
                }
                else if ( 273 == head.id && IsIntType( head.type ) )
                {
                    if ( 0 != head.offset && 0xffffffff != head.offset && !likelyRAW && IsPerhapsAnImage( head.offset, headerBase ) )
                        provisionalJPGOffset = head.offset + headerBase;
                }
                else if ( 274 == head.id && IsIntType( head.type ) )
                {
                    if ( -1 == g_Orientation_Value )
                    {
                        g_Orientation_Value = head.offset;
                        g_Orientation_Offset = headerBase + IFDOffset - 4;
                        g_Orientation_Type = head.type;
                        g_Orientation_LittleEndian = littleEndian;
                    }
                    else
                    {
                        g_Orientation_Value2 = head.offset;
                        g_Orientation_Offset2 = headerBase + IFDOffset - 4;
                        g_Orientation_Type2 = head.type;
                    }
                }
                else if ( 279 == head.id && IsIntType( head.type ) )
                {
                    if ( ( lastBitsPerSample != 16 ) && 0 != provisionalJPGOffset && 0 != head.offset && 0xffffffff != head.offset && !likelyRAW && ( head.offset > g_Embedded_Image_Length ) )
                    {
                        g_Embedded_Image_Length = head.offset;
                        g_Embedded_Image_Offset = provisionalJPGOffset;
                    }
                }
                else if ( 306 == head.id && 2 == head.type )
                {
                    __int64 stringOffset = ( head.count <= 4 ) ? ( IFDOffset - 4 ) : head.offset;
                    GetString( stringOffset + headerBase, g_acDateTime, _countof( g_acDateTime ), head.count );
                }
                else if ( 330 == head.id && 4 == head.type )
                {
                    if ( 1 == head.count )
                        EnumerateGenericIFD( depth + 1, head.offset, headerBase, littleEndian );
                    else
                    {
                        for ( size_t i = 0; i < head.count; i++ )
                        {
                            DWORD oIFD = GetDWORD( ( i * 4 ) + head.offset + headerBase, littleEndian );
                            EnumerateGenericIFD( depth + 1, oIFD, headerBase, littleEndian );
                        }
                    }
                }
                else if ( 513 == head.id && IsIntType( head.type ) )
                {
                    if ( 0 != head.offset && 0xffffffff != head.offset && IsPerhapsAnImage( head.offset, headerBase ) && !likelyRAW )
                        provisionalEmbeddedJPGOffset = head.offset + headerBase;
    
                }
                else if ( 514 == head.id && IsIntType( head.type ) )
                {
                    if ( 0 != head.offset && 0xffffffff != head.offset && 0 != provisionalEmbeddedJPGOffset && ( head.offset > g_Embedded_Image_Length ) )
                    {
                        g_Embedded_Image_Length = head.offset;
                        g_Embedded_Image_Offset = provisionalEmbeddedJPGOffset;
                    }
                }
                else if ( 700 == head.id )
                {
                    // XMP Data. Adobe products update (and move and resize) this tag to include edits for DNG, TIFF, and JPG files.
                    // The data is there instead of in .xmp files, as it is for other RAW formats.

                    if ( head.count > 4 && head.count < 65536 )
                    {
                        unique_ptr<byte> bytes( new byte[ head.count + 1 ] );
                        bytes.get()[ head.count ] = 0; // ensure it'll be null-terminated
                        GetBytes( head.offset + headerBase, bytes.get(), head.count );
                        if ( strstr( (char *) bytes.get(), "Adobe XMP Core" ) )
                            g_holdsAdobeEditsInXMP = true;
                    }
                }
                else if ( 34665 == head.id )
                {
                    EnumerateExifTags( depth + 1, head.offset, headerBase, littleEndian );
                }
                else if ( 34853 == head.id )
                {
                    EnumerateGPSTags( depth + 1, head.offset, headerBase, littleEndian );
                }
                else if ( 41989 == head.id && IsIntType( head.type ) )
                {
                    // This is generally in ExifTags, but for Nikon it can sometimes be here in IFD0

                    g_FocalLengthIn35mmFilm = head.offset;
                }
                else if ( 42037 == head.id && 2 == head.type )
                {
                    __int64 stringOffset = ( head.count <= 4 ) ? ( IFDOffset - 4 ) : head.offset;
                    GetString( stringOffset + headerBase, g_acLensSerialNumber, _countof( g_acLensSerialNumber ), head.count );
                    //tracer.Trace( "IFD0 Lens Serial Number: %s\n", g_acLensSerialNumber );
                }
                else if ( 50735 == head.id && 2 == head.type )
                {
                     __int64 stringOffset = ( head.count <= 4 ) ? ( IFDOffset - 4 ) : head.offset;
                    GetString( stringOffset + headerBase, g_acSerialNumber, _countof( g_acSerialNumber ), head.count );
                    //tracer.Trace( "IFD0 Body Serial Number: %s\n", g_acSerialNumber );
                }
                else if ( 50740 == head.id && IsIntType( head.type ) )
                {
                    // Sony and Ricoh Makernotes (in addition to makernotes stored in Exif IFD)
    
                    EnumerateMakernotes( depth + 1, head.offset, headerBase, littleEndian );
                }
            }
    
            IFDOffset = GetDWORD( IFDOffset + headerBase, littleEndian );
    
            currentIFD++;
        }
    } //EnumerateIFD0
    
    int ParseOldJpg( bool embedded = false )
    {
        int exifOffset = 0;

        const BYTE MARKER_SOI   = 0xd8;          // start of image
        const BYTE MARKER_APP0  = 0xe0;          // JFIF application segment
        const BYTE MARKER_APP1  = 0xe1;          // Exif or adobe segment
        const BYTE MARKER_DQT   = 0xdb;          // Quantization table
        const BYTE MARKER_SOF0  = 0xc0;          // start of frame baseline
        const BYTE MARKER_SOF2  = 0xc2;          // start of frame progressive
        const BYTE MARKER_DHT   = 0xc4;          // Define Huffman table
        const BYTE MARKER_JPG   = 0xc8;          // JPG extensions
        const BYTE MARKER_DAC   = 0xcc;          // Define arithmetic coding conditioning(s)
        const BYTE MARKER_SOF15 = 0xcf;          // last of the SOF? marker
        const BYTE MARKER_SOS   = 0xda;          // start of scan
        const BYTE MARKER_EOI   = 0xd9;          // end of image
        const BYTE MARKER_COM   = 0xfe;          // comment
    
        DWORD offset = 2;

        #pragma pack(push, 1)
        struct JpgRecord
        {
            BYTE marker;
            BYTE segment;
            WORD length;

            // items below are just for MARKER_SOF* segments

            BYTE precision;
            WORD height;
            WORD width;
            BYTE components;

            void FixEndian( bool littleEndian )
            {
                if ( !littleEndian )
                    length = _byteswap_ushort( length );
            }

            void FixSOFEndian( bool littleEndian )
            {
                if ( !littleEndian )
                {
                    height = _byteswap_ushort( height );
                    width = _byteswap_ushort( width );
                }
            }
        };
        #pragma pack(pop)
    
        do
        {
            JpgRecord record;
            GetBytes( offset, &record, sizeof record );
            record.FixEndian( false );

            if ( 0xff != record.marker )
                return exifOffset;
    
            if ( MARKER_EOI == record.segment )
                return exifOffset;
    
            if ( 0xff == record.segment )
            {
                offset += 2;
                continue;
            }
    
            if ( MARKER_DHT == record.segment || MARKER_JPG == record.segment || MARKER_DAC == record.segment )
            {
                // These 3 fall in the range of the valid SOF? segments, so watch for them and ignore
            }
            else if ( record.segment >= MARKER_SOF0 && record.segment <= MARKER_SOF15 )
            {
                record.FixSOFEndian( false );

                if ( embedded )
                {
                    g_Embedded_Image_Width = record.width;
                    g_Embedded_Image_Height = record.height;
                }
                else
                {
                    if ( record.width > g_ImageWidth )
                        g_ImageWidth = record.width;

                    if ( record.height > g_ImageHeight )
                        g_ImageHeight = record.height;
                }
            }
            else if ( MARKER_SOS == record.segment )
            {
                break;
            }
            else if ( MARKER_APP1 == record.segment )
            {
                char app1Header[ 5 ];
                GetBytes( (__int64) offset + 4, app1Header, 4 );
                app1Header[4] = 0;
    
                if ( !stricmp( app1Header, "exif" ) )
                {
                    // just return the exifoffset so it can be parsed later

                    exifOffset = offset + 8;
                }
            }
    
            if ( 0 == record.length )
                break;
    
            // note: this will not work for many segments, where additional math is required to find lengths.
            // Luckily, the 0xc0 Start Of Frame 0 SOF0 segment generally comes early
    
            offset += ( record.length + 2 );
        } while (true);
    
        return exifOffset;
    } //ParseOldJpg
    
    // https://en.wikipedia.org/wiki/Portable_Network_Graphics
    // http://www.libpng.org/pub/png/spec/1.2/PNG-Chunks.html
    // PNG files are big-endian. 8-byte header then recordsof:
    //    4: length
    //    4: type
    //    length: data
    //    4: crc

    struct SLenType
    {
        DWORD len;
        DWORD type;
    };
    
    void ParsePNG( bool embedded = false )
    {
        DWORD offset = 8;
    
        do
        {
            bool seekOK = g_pStream->Seek( offset );
    
            if ( !seekOK || g_pStream->AtEOF() )
                return;

            SLenType lenType;
            GetBytes( offset, &lenType, sizeof lenType );
       
            if ( lenType.len > ( 128 * 1024 * 1024 ) )
                return;
    
            // likely a malformed file

            if ( 0 == lenType.type )
                break;
    
            char acChunkType[ 5 ];
            memcpy( acChunkType, &lenType.type, 4 );
            acChunkType[ 4 ] = 0;
    
            if ( 0x49454e44 == lenType.type ) // IEND  end of chunks
            {
                return;
            }
            else if ( 0x49484452 == lenType.type ) // IHDR   header
            {
                if ( embedded )
                {
                    g_Embedded_Image_Width = GetDWORD( (__int64) offset + 8, false );
                    g_Embedded_Image_Height = GetDWORD( (__int64) offset + 12, false );
                }
                else
                {
                    g_ImageWidth = GetDWORD( (__int64) offset + 8, false );
                    g_ImageHeight = GetDWORD( (__int64) offset + 12, false );
                }

                // BUGMAGNET: ALERT
                // Since these are the only two fields required from PNG files, break out
                // of the loop after we get them. If any other data is required, remove
                // this break!

                break;
            }
    
            offset += ( 12 + lenType.len );
        } while( true );
    } //ParsePNG
    
    bool isMP3Frame( char const * frameID, char const * name, const char * name2 = 0 )
    {
        if ( * (DWORD *) frameID == * (DWORD *) name )
            return true;
    
        if ( NULL == name2 )
            return false;
    
        return ( * (DWORD *) frameID == * (DWORD *) name2 );
    } //isMP3Frame
    
    bool isValidMP3Frame( char const * pc )
    {
        for ( int i = 0; i < 4; i++ )
        {
            char c = pc[i];
    
            if ( 0 == c )
            {
                if ( 0 == i )
                    return false;
                else
                    continue;
            }
    
            if ( ( c > 'Z' || c < 'A' ) && ( c > '9' || c < '0' ) )
                return false;
        }
    
        return true;
    } //isValidMP3Frame

    void ParseMP3()
    {
        #pragma pack(push, 1)
    
        __int64 len = g_pStream->Length();
        if ( len < 128 )
            return;
    
        struct ID3v2Header
        {
            char id[ 3 ];
            byte ver[ 2 ];
            byte flags;
            DWORD size;
        };
    
        ID3v2Header start;
        memset( &start, 0, sizeof start );
        GetBytes( 0, &start, sizeof start );
        start.size = _byteswap_ulong( start.size );
        start.size = ( ( start.size & 0x7f000000 ) >> 3 ) | ( ( start.size & 0x7f0000 ) >> 2 ) | ( ( start.size & 0x7f00 ) >> 1 ) | ( start.size & 0x7f );
    
        __int64 firstFrameOffset = sizeof start;
    
        if ( 'I' != start.id[0] || 'D' != start.id[1] || '3' != start.id[2] )
        {
            tracer.Trace( "not an mp3 file that can hold an image (there is no ID3 header)\n" );
            return;
        }

        __int64 frameOffset = firstFrameOffset;
    
        struct ID3v23FrameHeader
        {
            char id[4];
            DWORD size;
            WORD flags;
        };
    
        struct ID3v22FrameHeader
        {
            char id[3];
            byte size[3];
        };
    
        while ( frameOffset < ( start.size + firstFrameOffset ) )
        {
            ID3v23FrameHeader frameHeader;
            memset( &frameHeader, 0, sizeof frameHeader );
            int frameHeaderSize = sizeof frameHeader;
    
            if ( 2 == start.ver[0] )
            {
                ID3v22FrameHeader frame2;
                memset( &frame2, 0, sizeof frame2 );
                GetBytes( frameOffset, &frame2, sizeof frame2 );
    
                frameHeader.id[0] = frame2.id[0];
                frameHeader.id[1] = frame2.id[1];
                frameHeader.id[2] = frame2.id[2];
                frameHeader.id[3] = 0;
    
                frameHeader.size = frame2.size[2] | ( frame2.size[1] << 8 ) | ( frame2.size[0] << 16 );
                frameHeader.flags = 0;
    
                frameHeaderSize = sizeof frame2;
            }
            else if ( start.ver[0] >= 3 )
            {
                GetBytes( frameOffset, &frameHeader, sizeof frameHeader );
                frameHeader.size = _byteswap_ulong( frameHeader.size );
            }
            else
            {
                tracer.Trace( "unrecognized or invalid MP3 ID3v2 header version %d\n", 0xff & start.ver[0] );
                break;
            }
    
            if ( 0 == frameHeader.size )
                break;
    
            if ( frameHeader.size > 1024 * 1024 * 100 )
            {
                // The largest I've seen is Jonathan Coulton 29MB. It's possible we'll not load valid images
                // Malformed files can be gigabytes

                tracer.Trace( "invalid frame size is too large: %u == %#x\n", frameHeader.size, frameHeader.size );
                break;
            }

            if ( !isValidMP3Frame( frameHeader.id ) )
            {
                tracer.Trace( "invalid frame id %c%c%c%c == %#x %#x %#x %#x\n", frameHeader.id[0], frameHeader.id[1], frameHeader.id[2], frameHeader.id[3],
                              0xff & frameHeader.id[0], 0xff & frameHeader.id[1], 0xff & frameHeader.id[2], 0xff & frameHeader.id[3] );
                break;
            }
    
            if ( isMP3Frame( frameHeader.id, "APIC", "PIC" ) )
            {
                // Embedded image (or a link to an image)
                // byte:                           encoding
                // null-terminated ascii:          mime type. iTunes writes JPG or PNG with no null-termination
                // byte:                           picture type
                // null-terminated encoded string: description
                // byte array:                     the image
                // 

                int o = frameOffset + frameHeaderSize;
                int oFrameData = o;

                // Every MP3 in my collection had far less than 100 bytes of data prior to the image itself.
                // I'm using 200 in case there are really odd MP3s out there

                byte apicdata[ 200 ];
                GetBytes( o, &apicdata, sizeof apicdata );

                int datao = 0;
                byte encoding = apicdata[ datao++ ];
    
                if ( 0 != encoding && 1 != encoding && 3 != encoding )
                {
                    // 2 == UTF-16 isn't handled because I can't find a file like that to test
    
                    tracer.Trace( "invalid encoding for APIC image text %d\n", 0xff & encoding );
                    return;
                }
    
                // mimetype encoding is always ascii

                char acMimeType[ 40 ];
                int i = 0;

                do
                {
                    if ( i >= ( _countof( acMimeType ) - 1 ) )
                    {
                        tracer.Trace( "invalid APIC mime type; it may not be null-terminated\n" );
                        return;
                    }

                    char c = apicdata[ datao++ ];
                    acMimeType[ i++ ] = c;
                    if ( 0 == c )
                        break;
                } while ( true );

                acMimeType[i] = 0;
    
                // iTunes writes the mimetype as "JPG" or "PNG" with no null-termination sometimes.
                // Hunt for the image start then backup o so it points at the picture type.

                bool isJPG = !strcmp( acMimeType, "JPG" );
                bool isPNG = !strcmp( acMimeType, "PNG" );
    
                if ( isJPG || isPNG )
                {
                    int to = datao;
    
                    for ( int z = 0; z < 20; z++ )
                    {
                        WORD w = apicdata[ z + datao ] | ( apicdata[ z + datao + 1 ] << 8 );
    
                        if ( ( ( 0xd8ff == w ) && isJPG ) || ( ( 0x5089 == w ) && isPNG ) )
                        {
                            datao = to + z - 2;
                            break;
                        }
                    }
                }

                bool knownMimeType = ( isJPG ||
                                       isPNG ||
                                       !strcmp( acMimeType, "image/jpg" ) ||   // lots of files have this non-standard mimetype
                                       !strcmp( acMimeType, "image/jpeg" ) ||
                                       !strcmp( acMimeType, "image/png" ) ||
                                       !strcmp( acMimeType, "image/bmp" ) );

               if ( !knownMimeType )
               {
                   tracer.Trace( "invalid or unknown APIC mimetype: %s\n", acMimeType );
                   return;
               }

                byte pictureType = apicdata[ datao++ ];
    
                i = 0;
                bool foundEndOfString = false;
    
                if ( 0 == encoding || 3 == encoding )
                {
                    do
                    {
                        WCHAR wc = apicdata[ datao++ ];
    
                        if ( datao >= sizeof( apicdata ) )
                            break;

                        if ( 0 == wc )
                        {
                            foundEndOfString = true;
                            break;
                        }
                    } while ( true );
                }
                else if ( 1 == encoding )
                {
                    do
                    {
                        WCHAR wc = apicdata[ datao ] | ( apicdata[ datao + 1 ] << 8 );
                        datao += 2;
    
                        if ( datao >= sizeof( apicdata ) )
                            break;

                        if ( 0 == wc )
                        {
                            foundEndOfString = true;
                            break;
                        }
                    } while( true );
                }

                if ( !foundEndOfString )
                {
                    tracer.Trace( "invalid description string for APIC picture\n" );
                    return;
                }

                if ( datao >= sizeof( apicdata ) )
                {
                    tracer.Trace( "walked past end of APIC data, invalid datao: %d\n", datao );
                    return;
                }

                o = oFrameData + datao;

                if ( o < ( oFrameData + frameHeader.size ) )
                {
                    int imageSize = frameHeader.size - ( o - oFrameData );
                    //tracer.Trace( "  image size:        %d and offset %d\n", imageSize, o );
    
                    // Sometimes there are multiple embedded images. Use the first with a reasonable size.
    
                    if ( 0 == g_Embedded_Image_Offset && 0 == g_Embedded_Image_Length )
                    {
                        if ( imageSize > 1000 )
                        {
                            bool isAnImage = IsPerhapsAnImage( o, 0 );
    
                            if ( isAnImage )
                            {
                                g_Embedded_Image_Offset = o;
                                g_Embedded_Image_Length = imageSize;

                                // Return from here; no need to continue iterating MP3 tags

                                return;
                            }
                            else
                                tracer.Trace( "invalid image header: apparently not an image in the MP3 APIC picture field\n" );
                        }
                        else
                            tracer.Trace( "ignoring an APIC image because it's too small: %d bytes\n", imageSize );
                    }
                    else
                        tracer.Trace( "ignoring an APIC image because we already have one\n" );
                }
                else
                    tracer.Trace( "invalid APIC image offset %d is beyond size of the frame %d. strings inside malformed?\n", o, frameHeader.size );
            }
            else
            {
                //tracer.Trace( "skipped frame id %c%c%c%c == %#x %#x %#x %#x\n", frameHeader.id[0], frameHeader.id[1], frameHeader.id[2], frameHeader.id[3],
                //              0xff & frameHeader.id[0], 0xff & frameHeader.id[1], 0xff & frameHeader.id[2], 0xff & frameHeader.id[3] );
            }
    
            frameOffset += ( frameHeaderSize + frameHeader.size );
    
            if ( frameOffset >= g_pStream->Length() )
            {
                tracer.Trace( "invalid MP3 frame offset is beyond the end of the file %#I64x\n", frameOffset );
                break;
            }
    
            // the >= case will be caught in the while loop above; no need to break

            if ( frameOffset > ( start.size + firstFrameOffset ) )
                tracer.Trace( "invalid MP3 frame offset %I64d is beyond the metadata size in the header + firstFrame %d\n", frameOffset, start.size + firstFrameOffset );
        }
    
        #pragma pack(pop)
    } //ParseMP3

    const WCHAR * FindExtension( const WCHAR * pwcPath )
    {
        size_t len = wcslen( pwcPath );
        WCHAR const * pwcEnd = pwcPath + len - 1;
    
        while ( pwcEnd >= pwcPath )
        {
            if ( L'.' == *pwcEnd )
                return pwcEnd;
    
            pwcEnd--;
        }
    
        return pwcPath + len;
    } //FindExtension

    void EnumerateImageData( HANDLE hFile, const WCHAR * pwc )
    {
        g_pStream = new CStream( hFile );
        unique_ptr<CStream> stream( g_pStream );
    
        if ( !g_pStream->Ok() )
        {
            g_pStream = NULL;
            return;
        }

        bool isOuterFileJPG = false;
        WCHAR const * pwcExt = FindExtension( pwc );
        __int64 heifOffsetBase = 0;
    
        if ( !_wcsicmp( pwcExt, L".heic" ) ||          // Apple iOS photos
             !_wcsicmp( pwcExt, L".hif" ) )            // Canon HEIF photos
        {
            // enumeration of the heif file is just to find the EXIF data offset, reflected in the g_Heif_Exif_* variables
    
            EnumerateHeif( g_pStream );
    
            if ( 0 == g_Heif_Exif_Offset )
            {
                g_pStream = NULL;
                return;
            }
    
            DWORD o = GetDWORD( g_Heif_Exif_Offset, false );
    
            heifOffsetBase = o + g_Heif_Exif_Offset + 4;
            bool seekOK = g_pStream->Seek( heifOffsetBase );

            if ( !seekOK )
            {
                tracer.Trace( "heif offset base looks wrong\n" );
                g_pStream = NULL;
                return;
            }
        }
        else if ( !_wcsicmp( pwcExt, L".cr3" ) )        // Canon's newer RAW format
        {
            // enumeration of the heif file is just to find the EXIF data offset, reflected in the g_Canon_CR3_* variables
            // Heif and CR3 use ISO Base Media File Format ISO/IEC 14496-12
    
            EnumerateHeif( g_pStream );
    
            if ( 0 == g_Canon_CR3_Exif_IFD0 )
            {
                g_pStream = NULL;
                return;
            }
    
            heifOffsetBase = g_Canon_CR3_Exif_IFD0;
            bool seekOK = g_pStream->Seek( heifOffsetBase );

            if ( !seekOK )
            {
                tracer.Trace( "heif-CR3 offset base looks wrong\n" );
                g_pStream = NULL;
                return;
            }
        }
    
        DWORD header = 0;
        ULONG bytesread = g_pStream->Read( &header, sizeof header );
        if ( 0 == bytesread )
        {
            tracer.Trace( "can't read from the file\n" );
            g_pStream = NULL;
            return;
        }
    
        bool parsingEmbeddedImage = false;

        if ( 0x43614c66 == header )
        {
            EnumerateFlac();

            if ( 0 != g_Embedded_Image_Offset && 0 != g_Embedded_Image_Length )
            {
                CStream * embeddedImage = new CStream( pwc, g_Embedded_Image_Offset, g_Embedded_Image_Length );
    
                embeddedImage->Read( &header, sizeof header );
                stream.reset( embeddedImage );
                g_pStream = embeddedImage;
                parsingEmbeddedImage = true; 
            }
            else
            {
                g_pStream = NULL;
                return;
            }
        }
        else if ( 0x03334449 == header || 0x02334449 == header || 0x04334449 == header || 0x90fbff == ( header & 0xffffff ) )
        {
            ParseMP3();
    
            if ( 0 != g_Embedded_Image_Offset && 0 != g_Embedded_Image_Length )
            {
                CStream * embeddedImage = new CStream( pwc, g_Embedded_Image_Offset, g_Embedded_Image_Length );
    
                embeddedImage->Read( &header, sizeof header );
                stream.reset( embeddedImage );
                g_pStream = embeddedImage;
                parsingEmbeddedImage = true; 
            }
            else
            {
                g_pStream = NULL;
                return;
            }
        }
    
        if ( ( 0xd8ff     != ( header & 0xffff ) ) &&   // JPG
             ( 0x002a4949 != header ) &&                // CR2 canon and standard TIF and DNG
             ( 0x2a004d4d != header ) &&                // NEF nikon (big endian!)
             ( 0x4f524949 != header ) &&                // ORF olympus
             ( 0x00554949 != header ) &&                // RW2 panasonic
             ( 0x494a5546 != header ) &&                // RAF fujifilm
             ( 0x474e5089 != header ) )                 // PNG
        {
            // BMP: 0x4d42
    
            g_pStream = NULL;
            return;
        }
    
        bool littleEndian = true;
        __int64 startingOffset = 4;
        __int64 headerBase = 0;
        __int64 exifHeaderOffset = 12;
    
        if ( 0x474e5089 == header ) // PNG
        {
            DWORD nextFour = GetDWORD( 4, false );
    
            if ( 0x0d0a1a0a != nextFour )
            {
                g_pStream = NULL;
                return;
            }
    
            ParsePNG();

            g_pStream = NULL;
            return;
        }
        else if ( 0xd8ff == ( header & 0xffff ) ) 
        {
            // special handling for JPG files

            if ( !parsingEmbeddedImage )
                isOuterFileJPG = true;

            int exifMaybe = ParseOldJpg();

            if ( 0 == exifMaybe )
            {
                g_pStream = NULL;
                return;
            }
    
            int saveMaybe = exifMaybe;
            exifMaybe += 5;  // Get past "Exif."
            DWORD maybe = GetDWORD( exifMaybe, littleEndian );
    
            if ( ( 0x002a4949 != maybe ) && ( 0x2a004d4d != maybe ) )
            {
                exifMaybe = 12; // It's just very common
                maybe = GetDWORD( exifMaybe, littleEndian );
            }
    
            if ( ( 0x002a4949 != maybe ) && ( 0x2a004d4d != maybe ) )
            {
                exifMaybe = saveMaybe + 2; // JFIF files are like this sometimes
                maybe = GetDWORD( exifMaybe, littleEndian );
            }

            if ( ( 0x002a4949 == maybe ) || ( 0x2a004d4d == maybe ) )
            {
                exifHeaderOffset = exifMaybe;
                headerBase = exifHeaderOffset;
                startingOffset = exifHeaderOffset + 4;
                header = maybe;
            }
        }
        else if ( 0x494a5546 == header )
        {
            // RAF files aren't like TIFF files. They have their own format which isn't documented and this app can't parse.
            // But RAF files have an embedded JPG with full properties, so show those.
            // https://libopenraw.freedesktop.org/formats/raf/
    
            DWORD jpgOffset = GetDWORD( 84, false );
            DWORD jpgLength = GetDWORD( 88, false );
            DWORD jpgSig = GetDWORD( jpgOffset, true );
    
            if ( 0xd8ff != ( jpgSig & 0xffff ) )
            {
                g_pStream = NULL;
                return;
            }
    
            DWORD exifSig = GetDWORD( jpgOffset + exifHeaderOffset, true );
    
            if ( 0x002a4949 != exifSig )
            {
                g_pStream = NULL;
                return;
            }
    
            header = exifSig;
            headerBase = jpgOffset + exifHeaderOffset;
            startingOffset = headerBase + 4;
    
            g_Embedded_Image_Offset = jpgOffset;
            g_Embedded_Image_Length = jpgLength;
            parsingEmbeddedImage = true;
        }
        else if ( 0x2a004d4d == header )
        {
            // Apple iPhone .heic files
    
            if ( 0 != heifOffsetBase )
            {
                headerBase = heifOffsetBase;
                startingOffset += heifOffsetBase;
            }
        }
        else if ( 0x002a4949 == header )
        {
            // Canon mirrorless .hif files
    
            if ( 0 != heifOffsetBase )
            {
                headerBase = heifOffsetBase;
                startingOffset += heifOffsetBase;
            }
        }
        
        if ( 0x4d4d == ( header & 0xffff ) ) // NEF
            littleEndian = false;
    
        DWORD IFDOffset = GetDWORD( startingOffset, littleEndian );
    
        EnumerateIFD0( 0, IFDOffset, headerBase, littleEndian, pwcExt );
    
        if ( ( 0 != g_Embedded_Image_Offset ) && ( 0 != g_Embedded_Image_Length ) && !wcsicmp( pwcExt, L".rw2" )  )
        {
            // Panasonic raw files sometimes have embedded JPGs with metadata not in the actual RW2 file.
            // Specifically, Serial Number, Lens Model, and Lens Serial Number can only be retrieved in this way.
    
            g_pStream = new CStream( pwc, g_Embedded_Image_Offset, g_Embedded_Image_Length );
            stream.reset( g_pStream );
    
            if ( !g_pStream->Ok() )
                return;
    
            int exifMaybe = ParseOldJpg( true );
            if ( 0 != exifMaybe )
            {
                int saveMaybe = exifMaybe;
                exifMaybe += 5;  // Get past "Exif."
                DWORD maybe = GetDWORD( exifMaybe, littleEndian );
    
                if ( ( 0x002a4949 != maybe ) && ( 0x2a004d4d != maybe ) )
                {
                    exifMaybe = 12; // It's just very common
                    maybe = GetDWORD( exifMaybe, littleEndian );
                }
    
                if ( ( 0x002a4949 != maybe ) && ( 0x2a004d4d != maybe ) )
                {
                    exifMaybe = saveMaybe + 2; // JFIF files are like this sometimes
                    maybe = GetDWORD( exifMaybe, littleEndian );
                }
    
                if ( ( 0x002a4949 == maybe ) || ( 0x2a004d4d == maybe ) )
                {
                    exifHeaderOffset = exifMaybe;
                    headerBase = exifHeaderOffset;
                    startingOffset = exifHeaderOffset + 4;
                    header = maybe;
                    littleEndian = ( 0x4949 == ( header & 0xffff ) );
    
                    DWORD IFDOffset = GetDWORD( startingOffset, littleEndian );
                    EnumerateIFD0( 0, IFDOffset, headerBase, littleEndian, pwcExt );
                }
            }
        }

        if ( 0 != g_Canon_CR3_Exif_Exif_IFD )
        {
            WORD endian = GetWORD( g_Canon_CR3_Exif_Exif_IFD, littleEndian );
    
            EnumerateExifTags( 0, 8, g_Canon_CR3_Exif_Exif_IFD, ( 0x4949 == endian ) );
        }
    
        if ( 0 != g_Canon_CR3_Exif_Makernotes_IFD )
        {
            WORD endian = GetWORD( g_Canon_CR3_Exif_Makernotes_IFD, littleEndian );
    
            EnumerateMakernotes( 0, 8, g_Canon_CR3_Exif_Makernotes_IFD, ( 0x4949 == endian ) );
        }
    
        if ( 0 != g_Canon_CR3_Exif_GPS_IFD  )
        {
            WORD endian = GetWORD( g_Canon_CR3_Exif_GPS_IFD, littleEndian );
    
            EnumerateGPSTags( 0, 8, g_Canon_CR3_Exif_GPS_IFD, ( 0x4949 == endian ) );
        }

        // If there is an embedded file, load and treat it as if it's the main image.
        // Sometimes JPGs have embedded smaller JPGs. Ignore them.

        if ( !isOuterFileJPG && 0 != g_Embedded_Image_Offset && 0 != g_Embedded_Image_Length )
        {
            if ( parsingEmbeddedImage )
            {
                g_Embedded_Image_Width = g_ImageWidth;
                g_Embedded_Image_Height = g_ImageHeight;
            }
            else
            {
                CStream * embeddedImage = new CStream( pwc, g_Embedded_Image_Offset, g_Embedded_Image_Length );
                unsigned long long head;
                embeddedImage->Read( &head, sizeof head );
                stream.reset( embeddedImage );
                g_pStream = embeddedImage;

                // At this point, we just want the width and height of the embedded JPG/PNG

                if ( IsPerhapsJPG( head ) )
                    ParseOldJpg( true );
                else if ( IsPerhapsPNG( head ) )
                    ParsePNG( true );
                else
                    tracer.Trace( "skipping embedded image with unexpected header %#llx in %ws\n", head, pwc );

                //tracer.Trace( "embedded width %d, embedded height %d, full width %d, full height %d\n",
                //              g_Embedded_Image_Width, g_Embedded_Image_Height, g_ImageWidth, g_ImageHeight );
            }
        }

        g_pStream = NULL;
    } //EnumerateImageData
    
    const char * ExifExposureMode( DWORD x )
    {
        if ( 0 == x )
            return "auto";
        if ( 1 == x )
            return "manual";
        if ( 2 == x )
            return "auto bracket";
    
        return "unknown";
    } //ExifExposureMode
    
    const char * ExifExposureProgram( DWORD x )
    {
        if ( 1 == x )
            return "manual";
        if ( 2 == x )
            return "program AE";
        if ( 3 == x )
            return "aperture-priority AE";
        if ( 4 == x )
            return "shutter speed priority AE";
        if ( 5 == x )
            return "creative (slow speed)";
        if ( 6 == x )
            return "action (high speed)";
        if ( 7 == x )
            return "portrait";
        if ( 8 == x )
            return "landscape";
        if ( 9 == x )
            return "bulb";
    
        return "unknown";
    } //ExifExposureProgram
    
    void InitializeGlobals()
    {
        g_pStream = NULL;
        g_Heif_Exif_ItemID = 0xffffffff;
        g_Heif_Exif_Offset = 0;
        g_Heif_Exif_Length = 0;
        g_Canon_CR3_Exif_IFD0 = 0;
        g_Canon_CR3_Exif_Exif_IFD = 0;
        g_Canon_CR3_Exif_Makernotes_IFD = 0;
        g_Canon_CR3_Exif_GPS_IFD = 0;
        g_Canon_CR3_Embedded_JPG_Length = 0;
    
        g_Embedded_Image_Offset = 0;
        g_Embedded_Image_Length = 0;
        g_Embedded_Image_Width = 0;
        g_Embedded_Image_Height = 0;

        g_Orientation_Value = -1;
        g_Orientation_Offset = 0;
        g_Orientation_Type = 0;
        g_Orientation_Value2 = -1;
        g_Orientation_Offset2 = 0;
        g_Orientation_Type2 = 0;
        g_Orientation_LittleEndian = false;
    
        g_acDateTimeOriginal[ 0 ] = 0;
        g_acDateTime[ 0 ] = 0;
        g_ImageWidth = -1;
        g_ImageHeight = -1;
        g_ISO = -1;
        g_ExposureNum = -1;
        g_ExposureDen = -1;
        g_ApertureNum = -1;
        g_ApertureDen = -1;
        g_ExposureProgram = -1;
        g_ExposureMode = -1;
        g_FocalLengthNum = -1;
        g_FocalLengthDen = -1;
        g_FocalLengthIn35mmFilm = -1;
        g_ComputedSensorWidth = -1;
        g_ComputedSensorHeight = -1;
        g_Longitude = InvalidCoordinate;
        g_Latitude = InvalidCoordinate;
        g_acLensMake[ 0 ] = 0;
        g_acLensModel[ 0 ] = 0;
        g_acLensSerialNumber[ 0 ] = 0;
        g_acMake[ 0 ] = 0;
        g_acModel[ 0 ] = 0;
        g_acSerialNumber[ 0 ] = 0;
        g_holdsAdobeEditsInXMP = false;
    } //InitializeGlobals
    
    void UpdateCache( const WCHAR * pwcPath )
    {
        // protect against multiple threads updating Image Data at the same time.
        // note that this doesn't help if they are opening different files since the globals will be trashed.
        // apps that need that protection should have separate objects or their own mutex.

        lock_guard<mutex> lock( g_mtx );

        bool cached = false;
        HANDLE hFile = INVALID_HANDLE_VALUE;

#if HANDLE_FILE_CHANGES
        FILETIME ftCreate, ftAccess, ftWrite;
#endif
    
        if ( !_wcsicmp( pwcPath, g_awcPath ) )
        {
#if HANDLE_FILE_CHANGES
            hFile = CreateFile( pwcPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL );
    
            if ( INVALID_HANDLE_VALUE == hFile )
            {
                InitializeGlobals();
                return;
            }
    
            FILETIME ftCreate, ftAccess, ftWrite;
            GetFileTime( hFile, &ftCreate, &ftAccess, &ftWrite );
    
            if ( !memcmp( &ftWrite, &g_ftWrite, sizeof ftWrite ) )
#endif
                cached = true;
        }
    
        if ( !cached )
        {
            InitializeGlobals();
    
            if ( INVALID_HANDLE_VALUE == hFile )
                hFile = CreateFile( pwcPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL );
    
            if ( INVALID_HANDLE_VALUE != hFile )
            {
                wcscpy_s( g_awcPath, _countof( g_awcPath ), pwcPath );

#if HANDLE_FILE_CHANGES
                FILETIME ftCreate, ftAccess, ftWrite;
                GetFileTime( hFile, &ftCreate, &ftAccess, &g_ftWrite );
#endif
    
                EnumerateImageData( hFile, pwcPath );
            }
        }
    
        if ( INVALID_HANDLE_VALUE != hFile )
            CloseHandle( hFile );

        //tracer.Trace( "metadata cached: %d for file %ws\n", cached, pwcPath );
    } //UpdateCache
    
    bool SubstantiallyDifferentResolution( int a, int b )
    {
        // no data to compare
    
        if ( 0 == a || 0 == b )
            return false;
    
        double x = a;
        double y = b;
        double diff = x - y;
        double d = fabs( diff );
    
        return ( ( d / a * 100.0 ) > 5.0 );
    } //SubstantiallyDifferentResolution
    
    bool SameFocalLength( double a, double b )
    {
        double diff = fabs( a - b );
    
    //    tracer.Trace( "samefl, a %lf, b %lf\n", a, b );
    
        return ( ( diff / a * 100.0 ) < 5.0 );
    } //SameFocualLength
    
    static double sqr( double d ) { return d * d; }
    
    bool validFLVal( int x )
    {
        return ( -1 != x && 0 != x );
    } //validFLVal
    
    bool validFLVal( double x )
    {
        return ( DBL_MAX != x && 0.0 != x );
    } //validFLVal
    
    double GetComputedCropFactor()
    {
        double diagonalFF = sqrt( sqr( 36.0 ) + sqr( 24.0 ) );
    
        if ( -1 != g_ComputedSensorWidth && 0 != g_ComputedSensorWidth && -1 != g_ComputedSensorHeight && 0 != g_ComputedSensorHeight )
        {
            double diagonal = sqrt( sqr( g_ComputedSensorWidth ) + sqr( g_ComputedSensorHeight ) );
            return diagonalFF / diagonal;
        }
    
        return DBL_MAX;
    } //GetComputedCropFactor

public:

    double FindFocalLength( const WCHAR * pwcPath, double &focalLength, int & flIn35mmFilm, double &flGuess, double &flComputed, char * pcModel, int modelLen )
    {
        UpdateCache( pwcPath );

        double flBestGuess = 0.0;
        focalLength = 0.0;
        flIn35mmFilm = 0;
        flGuess = 0.0;
        flComputed = 0.0;
        flBestGuess = 0.0;
        strcpy_s( pcModel, modelLen, g_acModel );

        double cropGuess = g_factor.GetCropFactor( g_acModel );
        double cropComputed = GetComputedCropFactor();
        bool validFL = validFLVal( g_FocalLengthNum ) && validFLVal( g_FocalLengthDen );
        bool validCropGuess = validFLVal( cropGuess );
        bool validCropComputed = validFLVal( cropComputed );
        bool valid35mmFilm = validFLVal( g_FocalLengthIn35mmFilm );

        if ( valid35mmFilm )
        {
            flIn35mmFilm = g_FocalLengthIn35mmFilm;
            flBestGuess = flIn35mmFilm;
        }

        if ( validFL )
        {
            focalLength = (double) g_FocalLengthNum / (double) g_FocalLengthDen;

            if ( 0.0 == flBestGuess )
                flBestGuess = focalLength;

            if ( validCropGuess )
            {
                flGuess = focalLength * cropGuess;

                if ( !valid35mmFilm )
                    flBestGuess = flGuess;
            }

            if ( validCropComputed )
            {
                flComputed = focalLength * cropComputed;

                if ( !valid35mmFilm && !validCropGuess )
                    flBestGuess = flComputed;
            }
        }

        return flBestGuess;
    } //FindFocalLength

    bool FindDateTime( const WCHAR * pwcPath, char * pcDateTime, int buflen )
    {
        UpdateCache( pwcPath );
    
        char * p = NULL;
    
        if ( 0 != g_acDateTimeOriginal[ 0 ] )
            p = g_acDateTimeOriginal;
        else if ( 0 != g_acDateTime[ 0 ] )
            p = g_acDateTime;
        else
        {
            if ( buflen > 0 )
            {
                *pcDateTime = 0;
                return true;
            }
            else
                return false;
        }
    
        if ( strlen( p ) >= buflen )
            return false;
    
        strcpy_s( pcDateTime, buflen, p );
    
        return ( 0 != *pcDateTime );
    } //FindDateTime
    
    bool GetInterestingMetadata( const WCHAR * pwcPath, char * pc, int buflen, int previewWidth, int previewHeight )
    {
        UpdateCache( pwcPath );
    
        *pc = 0;
        char * current = pc;
        char * past = pc + buflen;
    
        if ( 0 != g_acDateTimeOriginal[ 0 ] )
            current += sprintf_s( current, past - current, "%s\n", g_acDateTimeOriginal );
        else if ( 0 != g_acDateTime[ 0 ] )
            current += sprintf_s( current, past - current, "%s\n", g_acDateTime );

        if ( ( -1 != g_ImageWidth ) && ( -1 != g_ImageHeight ) )
        {
            int w = g_ImageWidth;
            int h = g_ImageHeight;
    
            if ( g_Orientation_Value >= 5 && g_Orientation_Value <= 8 )
            {
                w = g_ImageHeight;
                h = g_ImageWidth;
            }
    
            if ( SubstantiallyDifferentResolution( w, previewWidth ) )
                current += sprintf_s( current, past - current, "%d x %d (%d x %d)\n", w, h, previewWidth, previewHeight );
            else
                current += sprintf_s( current, past - current, "%d x %d\n", w, h );
        }
        else
            current += sprintf_s( current, past - current, "%d x %d\n", previewWidth, previewHeight ); // BMP, PNG, and any other non-supported formats
    
        if ( -1 != g_ISO )
            current += sprintf_s( current, past - current, "ISO %d\n", g_ISO );
    
        if ( -1 != g_ExposureNum )
        {
            if ( 0 != g_ExposureNum && 1 != g_ExposureNum )
            {
                g_ExposureDen = (int) round( (double) g_ExposureDen / (double) g_ExposureNum );
                g_ExposureNum = 1;
            }
    
            if ( 0 == g_ExposureDen || 1 == g_ExposureDen )
                current += sprintf_s( current, past - current, "%d sec\n", g_ExposureNum );
            else
                current += sprintf_s( current, past - current, "%d/%d sec\n", g_ExposureNum, g_ExposureDen );
        }
    
        if ( -1 != g_ApertureNum && -1 != g_ApertureDen && 0 != g_ApertureDen )
            current += sprintf_s( current, past - current, "f / %.1lf\n", (double) g_ApertureNum / (double) g_ApertureDen );
    
        // Try to find both the focal length and effective focal length (if it's different / not full frame)
    
        {
            double cropGuess = g_factor.GetCropFactor( g_acModel );
            double cropComputed = GetComputedCropFactor();
            bool validFL = validFLVal( g_FocalLengthNum ) && validFLVal( g_FocalLengthDen );
            bool validCropGuess = validFLVal( cropGuess );
            bool validCropComputed = validFLVal( cropComputed );
            bool valid35mmFilm = validFLVal( g_FocalLengthIn35mmFilm );
        
            //tracer.Trace( "cropGuess %lf, cropComputed %lf\n", cropGuess, cropComputed );
        
            if ( validFL )
            {
                double focalLength = (double) g_FocalLengthNum / (double) g_FocalLengthDen;
        
                if ( valid35mmFilm )
                {
                    double fl = (double) g_FocalLengthIn35mmFilm;
                    if ( SameFocalLength( fl, focalLength ) )
                        current += sprintf_s( current, past - current, "%.1lfmm\n", focalLength );
                    else
                        current += sprintf_s( current, past - current, "%.1fmm (%dmm equivalent)\n", focalLength, g_FocalLengthIn35mmFilm );
                }
                else if ( validCropGuess )
                {
                    double fl = focalLength * cropGuess;
                    if ( SameFocalLength( fl, focalLength ) )
                        current += sprintf_s( current, past - current, "%.1lfmm\n", focalLength );
                    else
                        current += sprintf_s( current, past - current, "%.1fmm (approx. %.1lfmm)\n", focalLength, fl );
                }
                else if ( validCropComputed )
                {
                    double fl = focalLength * cropComputed;
                    if ( SameFocalLength( fl, focalLength ) )
                        current += sprintf_s( current, past - current, "%.1lfmm\n", focalLength );
                    else
                        current += sprintf_s( current, past - current, "%.1fmm (computed %.1lfmm)\n", focalLength, fl );
                }
                else
                    current += sprintf_s( current, past - current, "%.1lfmm\n", focalLength );
            }
            else if ( valid35mmFilm )
                current += sprintf_s( current, past - current, " %dmm equivalent\n", g_FocalLengthIn35mmFilm );
        }
    
        if ( -1 != g_ExposureProgram )
            current += sprintf_s( current, past - current, "%s\n", ExifExposureProgram( g_ExposureProgram ) );
        else if ( -1 != g_ExposureMode )
            current += sprintf_s( current, past - current, "%s\n", ExifExposureMode( g_ExposureMode ) );
    
        if ( 0 != g_acMake[0] || 0 != g_acModel[ 0 ] )
            current += sprintf_s( current, past - current, "%s%s%s\n", g_acMake, ( 0 == g_acMake[0] ) ? "" : " ", g_acModel );
    
        if ( 0 != g_acLensMake[0] || 0 != g_acLensModel[ 0 ] )
            current += sprintf_s( current, past - current, "%s%s%s\n", g_acLensMake, ( 0 == g_acLensMake[0] ) ? "" : " ", g_acLensModel );
    
        if ( ( InvalidCoordinate != fabs( g_Latitude ) && InvalidCoordinate != fabs( g_Longitude ) ) )
            current += sprintf_s( current, past - current, "%.7lf, %.7lf\n", g_Latitude, g_Longitude );
    
    //    if ( -1 != g_ComputedSensorWidth && -1 != g_ComputedSensorHeight )
    //        current += sprintf_s( current, past - current, "sensor %dx%dmm\n", g_ComputedSensorWidth, g_ComputedSensorHeight );
    
        // remove the trailing newline
    
        if ( ( 0 != *pc ) && ( '\n' == * ( current - 1 ) ) )
            *( current - 1 ) = 0;
    
        return ( 0 != ( *pc ) );
    } //GetInterestingMetadata
    
    bool GetCameraInfo( const WCHAR * pwcPath, char * pcMake, int makeLen, char * pcModel, int modelLen )
    {
        UpdateCache( pwcPath );
    
        *pcMake = 0;
        *pcModel = 0;
    
        if ( 0 != g_acMake[0] )
            strcpy_s( pcMake, makeLen, g_acMake );
    
        if ( 0 != g_acModel[0] )
            strcpy_s( pcModel, modelLen, g_acModel );
    
        return ( 0 != *pcMake || 0 != *pcModel );
    } //GetCameraInfo

    bool GetSerialNumbers( const WCHAR * pwcPath, char * pcMake, int makeLen, char * pcModel, int modelLen, char * pcSerialNumber, int serialNumberLen,
                           char * pcLensMake, int lensMakeLen, char * pcLensModel, int lensModelLen, char * pcLensSerialNumber, int lensSerialNumberLen )
    {
        UpdateCache( pwcPath );

        *pcMake = 0;
        *pcModel = 0;
        *pcSerialNumber = 0;
        *pcLensMake = 0;
        *pcLensModel = 0;
        *pcLensSerialNumber = 0;

        if ( 0 != g_acMake[0] )
            strcpy_s( pcMake, makeLen, g_acMake );
    
        if ( 0 != g_acModel[0] )
            strcpy_s( pcModel, modelLen, g_acModel );
    
        if ( 0 != g_acSerialNumber[0] )
            strcpy_s( pcSerialNumber, serialNumberLen, g_acSerialNumber );
    
        if ( 0 != g_acLensMake[0] )
            strcpy_s( pcLensMake, lensMakeLen, g_acLensMake );
    
        if ( 0 != g_acLensModel[0] )
            strcpy_s( pcLensModel, lensModelLen, g_acLensModel );
    
        if ( 0 != g_acLensSerialNumber[0] )
            strcpy_s( pcLensSerialNumber, lensSerialNumberLen, g_acLensSerialNumber );
    
        return ( 0 != *pcSerialNumber || 0 != *pcLensSerialNumber );
    } //GetSerialNumbers
    
    bool FindEmbeddedImage( const WCHAR * pwcPath, long long * pOffset, long long * pLength, int * orientationValue,
                            int * pWidth, int * pHeight, int * pFullWidth, int * pFullHeight )
    {
        UpdateCache( pwcPath );
    
        // Note that the embedded image has no orientation/rotate value. Use orientation from the outer RAW file
    
        *pOffset = g_Embedded_Image_Offset;
        *pLength = g_Embedded_Image_Length;
        *orientationValue = g_Orientation_Value;
        *pWidth = g_Embedded_Image_Width;
        *pHeight = g_Embedded_Image_Height;
        *pFullWidth = g_ImageWidth;
        *pFullHeight = g_ImageHeight;
    
        if ( ( 0 == g_Embedded_Image_Offset ) || ( 0 == g_Embedded_Image_Length ) )
            return false;
    
        return true;
    } //FindEmbeddedJPG
    
    bool GetGPSLocation( const WCHAR * pwcPath, double * pLatitude, double * pLongitude )
    {
        UpdateCache( pwcPath );
    
        if ( ( InvalidCoordinate == fabs( g_Latitude ) && InvalidCoordinate == fabs( g_Longitude ) ) )
            return false;
    
        *pLatitude = g_Latitude;
        *pLongitude = g_Longitude;
    
        return true;
    } //GetGPSLocation

    bool GetOrientation( const WCHAR * pwcPath, int * orientation )
    {
        *orientation = 1; // default

        UpdateCache( pwcPath );

        if ( -1 == g_Orientation_Value )
        {
            tracer.Trace( "orientation value is -1, so assuming it isn't set in the file, so can't rotate because there is nothing to update\n" );
            return false;
        }

        return true;
    } //GetOrientation

    bool HoldsAdobeEditsInXMP( const WCHAR * pwcPath )
    {
        UpdateCache( pwcPath );

        return g_holdsAdobeEditsInXMP;
    } //HoldsAdobeEditsInXMP

    bool RotateImage( const WCHAR * pwcPath, bool rotateRight )
    {
        UpdateCache( pwcPath );

        if ( -1 == g_Orientation_Value )
        {
            tracer.Trace( "orientation value is -1, so assuming it isn't set in the file, so can't rotate because there is nothing to update\n" );
            return false;
        }

        if ( g_Orientation_Value > 8 || g_Orientation_Value < 1 )
        {
            tracer.Trace( "overriding illegal orientation value %d with a default of 1 == horizontal (normal)\n", g_Orientation_Value );
            g_Orientation_Value = 1;
        }

        if ( 1 != g_Orientation_Value && 6 != g_Orientation_Value && 3 != g_Orientation_Value && 8 != g_Orientation_Value )
        {
            tracer.Trace( "orientation vaue isn't 1, 6, 3, or 8, so rotate can't be performed: %d\n", g_Orientation_Value );
            return false;
        }

        if ( 0 == g_Orientation_Offset )
        {
            tracer.Trace( "orientation offset is 0, which can't be correct\n" );
            return false;
        }

        if ( 3 != g_Orientation_Type )
        {
            tracer.Trace( "orientation data type isn't 3 (short) as expected: %d\n", g_Orientation_Type );
            return false;
        }

        HANDLE hFile = CreateFile( pwcPath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL );

        if ( INVALID_HANDLE_VALUE == hFile )
        {
            tracer.Trace( "can't open file for write to update orientation, error %d\n", GetLastError() );
            return false;
        }

        // 1 --> 6 --> 3 --> 8 --> 1 ...
        WORD o = g_Orientation_Value;

        if ( rotateRight )
        {
            if ( 1 == o )
                o = 6;
            else if ( 8 == o )
                o = 1;
            else if ( 3 == o )
                o = 8;
            else
                o = 3;
        }
        else
        {
            if ( 1 == o )
                o = 8;
            else if ( 8 == o )
                o = 3;
            else if ( 3 == o )
                o = 6;
            else
                o = 1;
        }

        tracer.Trace( "updating orientation value %d with %d at file offset %lld\n", g_Orientation_Value, o, g_Orientation_Offset );

        LARGE_INTEGER li;
        li.QuadPart = g_Orientation_Offset;
        bool ok = SetFilePointerEx( hFile, li, NULL, FILE_BEGIN );

        if ( ok )
        {
            DWORD written = 0;
            WORD oToWrite = g_Orientation_LittleEndian ? o : _byteswap_ushort( o );
            ok = WriteFile( hFile, &oToWrite, sizeof oToWrite, &written, NULL );

            if ( ok )
                g_Orientation_Value = o;
            else
                tracer.Trace( "can't write orientation to file, error %d\n", GetLastError() );
        }
        else
        {
            tracer.Trace( "can't set file pointer to update orientation, error %d\n", GetLastError() );
        }

        // Sometimes (Panasonic RAWs written by Lightroom) the orientation is stored twice,
        // in IFD0 and IFD1 (the second record of IFD0). Update both.
        // Different apps look at different values, so the behavior is otherwise unpredictable.

        if ( -1 != g_Orientation_Value2 && 0 != g_Orientation_Offset2 )
        {
            li.QuadPart = g_Orientation_Offset2;
            ok = SetFilePointerEx( hFile, li, NULL, FILE_BEGIN );

            if ( ok )
            {
                DWORD written = 0;
                WORD oToWrite = g_Orientation_LittleEndian ? o : _byteswap_ushort( o );
                ok = WriteFile( hFile, &oToWrite, sizeof oToWrite, &written, NULL );

                if ( !ok )
                    tracer.Trace( "can't write orientation2 to file, error %d\n", GetLastError() );
            }
            else
            {
                tracer.Trace( "can't set file pointer to update orientation2, error %d\n", GetLastError() );
            }
        }

        CloseHandle( hFile );

        return ok;
    } //RotateImage

    void PurgeCache()
    {
        InitializeGlobals();
        g_awcPath[ 0 ] = 0;
    }
    
    CImageData()
    {
        InitializeGlobals();
    }

    ~CImageData()
    {
    }
};
