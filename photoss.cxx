#ifndef UNICODE
#define UNICODE
#endif

// Super-simple screen saver that iterates through images under a folder.
// Uses WIC & GDI+ for simplicity of loading and scaling images.
// To install, copy photoss.exe to %windir%\system32\photoss.scr
// To run the exe stand-alone, pass -s
// To run the exe and bring up the settings dialog, pass no arguments
// April 27, 2021

#include <windows.h>
#include <scrnsave.h>
#include <gdiplus.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <wincodec.h>

#include <stdio.h>
#include <ppl.h>

#include <mutex>
#include <chrono>

using namespace std;
using namespace Gdiplus;
using namespace concurrency;
using namespace std::chrono;

#include <djltrace.hxx>
#include <djlres.hxx>
#include <djlsav.hxx>
#include <djlenum.hxx>
#include <djl_strm.hxx>
#include <djlimagedata.hxx>
#include <djl_wic2gdi.hxx>

#include "photoss.h"

#pragma comment(lib, "Gdiplus.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "scrnsave.lib" )
#pragma comment(lib, "shlwapi.lib" )
#pragma comment(lib, "windowscodecs.lib" )

#define TIMER_ID_DELAY 1
#define TIMER_ID_BLANK 2
#define REGISTRY_APP_NAME L"SOFTWARE\\photoss"
#define REGISTRY_PHOTO_PATH L"PhotoPath"
#define REGISTRY_PHOTO_DELAY L"PhotoDelay"
#define REGISTRY_PHOTO_BLANK_DELAY L"BlankDelay"
#define REGISTRY_PHOTO_SHOWCAPTUREDATE L"PhotoShowCaptureDate"

CDJLTrace tracer;

Bitmap * g_pCurrentBitmap = NULL;
BYTE * g_pCurrentBitmapBuffer = NULL;
int g_currentBitmapIndex = 0;
WCHAR g_awcPhotoPath[ MAX_PATH + 2 ] = { 0 };
CStringArray * g_pImagePaths = NULL;
char g_acPhotoDateTime[ 25 ] = { 0 };
const int g_validDelays[] = { 1, 5, 15, 30, 60, 600 };  // seconds between photo changes
const int g_validBlanks[] = { 5, 15, 30, 60, 120 };     // minutes until the display goes blank
int photoDelay = g_validDelays[ 1 ];
int blankDelay = g_validBlanks[ 1 ];
bool g_showCaptureDate = true;                          // also controls whether current date is shown
bool g_blankMode = false;                               // show a blank screen (plus perhaps current date)
IWICImagingFactory *g_pIWICFactory = NULL;
RECT g_AppRect;
CImageData g_ImageData;
CWic2Gdi * g_pWic2Gdi = 0;

long long timeCreate = 0;
long long timeDraw = 0;
long long timeBLT = 0;

class StartupDPIAwareness
{
    public:
        StartupDPIAwareness()
        {
            SetProcessDpiAwarenessContext( DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 );
        }
};

StartupDPIAwareness MakeAware;

// Canon's WIC plugin doesn't work, apparently, so .hif doesn't work.
// Neither do .CR3 files with .hif embedded previews. JPG embedded previews work fine.
//    L".hif",
// I haven't yet added the embedded code from pv.exe here.
//    L".flac",

const WCHAR * imageExtensions[] =
{
    L"3fr",
    L"arw",
    L"bmp",
    L"cr2",
    L"cr3",
    L"dng",
//    L"flac",
    L"gif",
    L"heic",   
//    L"hif",
    L"jfif",
    L"jpeg",
    L"jpg",
    L"nef",
    L"orf",
    L"png",
    L"raf",
    L"rw2",
    L"tif",
    L"tiff",
};

void LoadPhotoPath()
{
    g_awcPhotoPath[ 0 ] = 0;
    BOOL ok = CDJLRegistry::readStringFromRegistry( HKEY_CURRENT_USER, REGISTRY_APP_NAME, REGISTRY_PHOTO_PATH, g_awcPhotoPath, sizeof( g_awcPhotoPath ) );

    if ( !ok )
    {
        PWSTR path = NULL;
        HRESULT hr = SHGetKnownFolderPath( FOLDERID_Pictures, 0, NULL, &path );
        if ( S_OK == hr )
        {
            wcscpy( g_awcPhotoPath, path );
            CoTaskMemFree( path );
        }
    }

    WCHAR awcPhotoDelay[ 10 ];
    awcPhotoDelay[ 0 ] = 0;
    ok = CDJLRegistry::readStringFromRegistry( HKEY_CURRENT_USER, REGISTRY_APP_NAME, REGISTRY_PHOTO_DELAY, awcPhotoDelay, sizeof( awcPhotoDelay ) );

    tracer.Trace( "read delay %ws from registry, ok %d\n", awcPhotoDelay, ok );

    if ( ok )
    {
        swscanf_s( awcPhotoDelay, L"%d", & photoDelay );

        bool found = false;

        for ( int d = 0; d < _countof( g_validDelays ); d++ )
        {
            if ( g_validDelays[ d ] == photoDelay )
            {
                found = true;
                break;
            }
        }

        if ( !found )
            photoDelay = g_validDelays[ 0 ];

        tracer.Trace( "photodelay found: %d, final value %d\n", found, photoDelay );
    }

    WCHAR awcg_showCaptureDate[ 10 ];
    awcg_showCaptureDate[ 0 ] = 0;
    ok = CDJLRegistry::readStringFromRegistry( HKEY_CURRENT_USER, REGISTRY_APP_NAME, REGISTRY_PHOTO_SHOWCAPTUREDATE, awcg_showCaptureDate, sizeof( awcg_showCaptureDate ) );

    if ( ok )
        g_showCaptureDate = !wcsicmp( awcg_showCaptureDate, L"yes" );

    WCHAR awcBlankDelay[ 10 ];
    awcBlankDelay[ 0 ] = 0;
    ok = CDJLRegistry::readStringFromRegistry( HKEY_CURRENT_USER, REGISTRY_APP_NAME, REGISTRY_PHOTO_BLANK_DELAY, awcBlankDelay, sizeof( awcBlankDelay ) );

    tracer.Trace( "read blank delay %ws from registry, ok %d\n", awcBlankDelay, ok );

    if ( ok )
    {
        swscanf_s( awcBlankDelay, L"%d", & blankDelay );

        bool found = false;

        for ( int d = 0; d < _countof( g_validBlanks ); d++ )
        {
            if ( g_validBlanks[ d ] == blankDelay )
            {
                found = true;
                break;
            }
        }

        if ( !found )
            blankDelay = g_validBlanks[ 0 ];

        tracer.Trace( "blankdelay found: %d, final value %d\n", found, blankDelay );
    }
} //LoadPhotoPath

bool LoadNextImageInternal( bool forward )
{
    tracer.Trace( "LoadNextImage, count of images %d, current %d, forward %d\n", g_pImagePaths->Count(), g_currentBitmapIndex, forward );

    if ( 0 == g_pImagePaths->Count() )
        return true;

    if ( forward )
    {
        g_currentBitmapIndex++;

        if ( g_currentBitmapIndex >= g_pImagePaths->Count() )
            g_currentBitmapIndex = 0;
    }
    else
    {
        if ( 0 == g_currentBitmapIndex )
            g_currentBitmapIndex = g_pImagePaths->Count() - 1;
        else
            g_currentBitmapIndex--;
    }

    int targetW = 0;
    int targetH = 0;

    if ( 0 != g_AppRect.right && 0 != g_AppRect.bottom )
    {
        targetW = g_AppRect.right;
        targetH = g_AppRect.bottom;

        double arDisplay = (double) g_AppRect.right / (double) g_AppRect.bottom;
        
        // super-minimal multi-mon support. If displays are apparently side by side, only use the left one.

        if ( arDisplay > 2.0 )
            targetW /= 2;
    }

    tracer.Trace( "loading image index %d, %ws\n", g_currentBitmapIndex, g_pImagePaths->Get( g_currentBitmapIndex ) );

    // Note: loading JPGs and other simple formats through GDIPlus works, but use WIC to get iPhone HEIC and other formats
    //Bitmap * pBitmap = new Bitmap( g_pImagePaths->Get( g_currentBitmapIndex ), FALSE );

    BYTE *pBitmapBuffer = NULL;
    int availableW, availableH;
    Bitmap * pBitmap = g_pWic2Gdi->GDIPBitmapFromWIC( g_pImagePaths->Get( g_currentBitmapIndex ), 0, &pBitmapBuffer, targetW, targetH, &availableW, &availableH );

    if ( NULL != pBitmap )
    {
        if ( ( 0 == pBitmap->GetWidth() ) || ( 0 == pBitmap->GetHeight() ) )
        {
            tracer.Trace( "  image has w %d, h %d, so it'll be skipped\n", pBitmap->GetWidth(), pBitmap->GetHeight() );
            delete pBitmap;
            delete pBitmapBuffer;
            return false;
        }

        if ( NULL != g_pCurrentBitmap )
        {
            delete g_pCurrentBitmap;
            delete g_pCurrentBitmapBuffer;
        }

        g_pCurrentBitmap = pBitmap;
        pBitmap = NULL;
        g_pCurrentBitmapBuffer = pBitmapBuffer;
        pBitmapBuffer = NULL;

        g_acPhotoDateTime[ 0 ] = 0;

        if ( g_showCaptureDate )
            g_ImageData.FindDateTime( g_pImagePaths->Get( g_currentBitmapIndex ), g_acPhotoDateTime, _countof( g_acPhotoDateTime ) );
    }
    else
        return false;

    return true;
} //LoadNextImageInternal

void LoadNextImage( bool forward = true )
{
    // Skip over files that can't be loaded or have a 0 width or height.

    int start = g_currentBitmapIndex;

    do
    {
        bool worked = LoadNextImageInternal( forward );

        if ( worked )
            return;

        // avoid infinite loop if none of the imags can be loaded (e.g. all are .cr3)

        if ( g_currentBitmapIndex == start )
            return;
    } while ( true );
} //LoadNextImage

void PutPathTextInClipboard( const WCHAR * pwcPath )
{
    size_t len = wcslen( pwcPath );
    size_t bytes = ( len + 1 ) * sizeof WCHAR;

    LPTSTR pstr = (LPTSTR) GlobalAlloc( GMEM_FIXED, bytes );
    if ( 0 != pstr )
    {
        // Windows takes ownership of pstr once it's in the clipboard

        wcscpy_s( pstr, 1 + len, pwcPath );
        SetClipboardData( CF_UNICODETEXT, pstr );
    }
} //PutPathTextInClipboard

void PutPathInClipboard( const WCHAR * pwcPath )
{
    size_t len = wcslen( pwcPath );

    // save room for two trailing null characters

    size_t bytes = sizeof( DROPFILES ) + ( ( len + 2 ) * sizeof( WCHAR ) );

    DROPFILES * df = (DROPFILES *) GlobalAlloc( GMEM_FIXED, bytes );
    ZeroMemory( df, bytes );
    df->pFiles = sizeof( DROPFILES );
    df->fWide = TRUE;
    WCHAR * ptr = (LPWSTR) ( df + 1 );
    wcscpy_s( ptr, len + 1, pwcPath );

    // Windows takes ownership of the buffer

    SetClipboardData( CF_HDROP, df );
} //PutPathInClipboard

void CopyCommand( HWND hwnd )
{
    tracer.Trace( "copying path into clipboard\n" );

    if ( !g_blankMode && ( 0 != g_pImagePaths ) && ( 0 != g_pImagePaths->Count() ) )
    {
        if ( OpenClipboard( hwnd ) )
        {
            EmptyClipboard();

            const WCHAR * pwcPath = g_pImagePaths->Get( g_currentBitmapIndex );
            PutPathTextInClipboard( pwcPath );
            PutPathInClipboard( pwcPath );

            CloseClipboard();
        }
    }
} //CopyCommand

__forceinline void WordToWC( WCHAR * pwc, WORD x )
{
    if ( x > 99 )
        x = 0;

    pwc[0] = ( x / 10 ) + L'0';
    pwc[1] = ( x % 10 ) + L'0';
} //WordToWC

// else the header redefines as W version but the lib links to the non-W version.
#undef ScreenSaverProc

extern "C" LRESULT WINAPI ScreenSaverProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    static bool firstEraseBackground = true;
    static bool iterationPaused = false;
    static ULONG_PTR gdiplusToken = 0;
    static HBRUSH brushBlack = 0;
    static HFONT fontText = 0;
    static int fontHeight = 30;

    tracer.Trace( "message %#x, wparam %#x\n", message, wParam );

    switch ( message )
    {
        case WM_CREATE:
        {
            tracer.Enable( false, L"d:\\photoss.txt" );

            brushBlack = CreateSolidBrush( RGB( 0, 0, 0 ) );

            GetClientRect( hWnd, &g_AppRect );
            fontHeight = g_AppRect.bottom / 50;
            tracer.Trace( "font height: %d\n", fontHeight );
            fontText = CreateFont( fontHeight, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_OUTLINE_PRECIS,
                                   CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, VARIABLE_PITCH, L"Tahoma" );

            LoadPhotoPath();
            tracer.Trace( "wm_create, g_awcPhotoPath %ws\n", g_awcPhotoPath );

            HRESULT hr = CoInitializeEx( NULL, COINIT_MULTITHREADED );
            if ( FAILED( hr ) )
                return 0;
        
            hr = CoCreateInstance( CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS( &g_pIWICFactory ) );
            if ( FAILED( hr ) )
            {
                tracer.Trace( "can't initialize WIC, error %#x\n", hr );
                return 0;
            }

            g_pWic2Gdi = new CWic2Gdi();
            if ( !g_pWic2Gdi->Ok() )
                return 0;

            GdiplusStartupInput si;
            Status gdiStatus = GdiplusStartup( &gdiplusToken, &si, NULL );
        
            if ( Status::Ok != gdiStatus )
                return 0;

            g_pImagePaths = new CStringArray();
            CEnumFolder enumFolder( true, g_pImagePaths, (WCHAR **) imageExtensions, _countof( imageExtensions ) );
            enumFolder.Enumerate( g_awcPhotoPath, L"*" );

            tracer.Trace( "found %d files\n", g_pImagePaths->Count() );

            g_pImagePaths->Randomize();
            LoadNextImage( true );

            SetTimer( hWnd, TIMER_ID_DELAY, 1000 * photoDelay, NULL );
            SetTimer( hWnd, TIMER_ID_BLANK, 60 * 1000 * blankDelay, NULL );

            SetProcessWorkingSetSize( GetCurrentProcess, ~0, ~0 );
            return 0;
        }

        case WM_DESTROY:
        {
            KillTimer( hWnd, TIMER_ID_DELAY );
            KillTimer( hWnd, TIMER_ID_BLANK );

            delete g_pImagePaths;
            g_pImagePaths = NULL;

            delete g_pCurrentBitmap;
            delete g_pCurrentBitmapBuffer;
            g_pCurrentBitmap = NULL;
            g_pCurrentBitmapBuffer = NULL;

            if ( 0 != gdiplusToken )
            {
                GdiplusShutdown( gdiplusToken );
                gdiplusToken = 0;
            }

            delete g_pWic2Gdi;
            g_pWic2Gdi = NULL;

            CoUninitialize();

            DeleteObject( brushBlack );
            return 0;
        }

        case WM_TIMER:
        {
            if ( !iterationPaused )
            {
                if ( TIMER_ID_BLANK == wParam && !g_blankMode )
                {
                    g_blankMode = true;

                    if ( g_pCurrentBitmap )
                    {
                        delete g_pCurrentBitmap;
                        g_pCurrentBitmap = 0;
                    }

                    if ( g_pCurrentBitmapBuffer )
                    {
                        delete g_pCurrentBitmapBuffer;
                        g_pCurrentBitmapBuffer = 0;
                    }

                    InvalidateRect( hWnd, NULL, TRUE );
                }

                if ( TIMER_ID_DELAY == wParam )
                {
                    if ( !g_blankMode )
                        LoadNextImage( true );

                    InvalidateRect( hWnd, NULL, TRUE );
                }
            }
            return 0;
        }

        case WM_CHAR:
        {
            tracer.Trace( "wm_char, wparam %#x\n", wParam );
            return 1;
        }

        case WM_KEYDOWN:
        {
            tracer.Trace( "wm_keydown, wparam %#x\n", wParam );

            if ( g_blankMode || ( 0 == g_pImagePaths->Count() ) )
                break;

            if ( VK_CONTROL == wParam )
            {
                // ignore just the control key being pressed. Don't exit the screensaver
                return 0;
            }
            else if ( ( 0x43 == wParam ) && ( GetKeyState( VK_CONTROL ) & 0x8000 ) ) // ^c for copy
            {
                CopyCommand( hWnd );
                return 0;
            }
            else if ( VK_LEFT == wParam || VK_RIGHT == wParam )
            {
                iterationPaused = true;
                LoadNextImage( VK_RIGHT == wParam );
                InvalidateRect( hWnd, NULL, TRUE );
                return 0;
            }
            else if ( VK_UP == wParam || VK_DOWN == wParam )
            {
                iterationPaused = false;
                return 0;
            }

            break;
        }

        case WM_ERASEBKGND:
        {
            // lie about erasing the background since otherwise the window will be black for a long time
            // when WM_PAINT has to load an image from the network.

            if ( firstEraseBackground )
            {
                firstEraseBackground = FALSE;
                break;
            }

            return 1;
        }

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint( hWnd, &ps );
    
            RECT rect;
            GetClientRect( hWnd, &rect );

            if ( 0 != rect.right && 0 != rect.bottom )
            {
                double arDisplay = (double) rect.right / (double) rect.bottom;
                
                // super-minimal multi-mon support. If displays are apparently side by side, only use the left one.

                if ( arDisplay > 2.0 )
                {
                    rect.right /= 2;
                    arDisplay /= 2;
                }

                if ( 0 != g_pCurrentBitmap )
                {
                    int bw = g_pCurrentBitmap->GetWidth();
                    int bh = g_pCurrentBitmap->GetHeight();

                    tracer.Trace( "in wm_paint. image w %d, h %d\n", bw, bh );

                    if ( 0 != bw && 0 != bh )
                    {
                        double arBitmap = (double) bw / (double) bh;

                        int toLeft = 0;
                        int toTop = 0;
                        int toRight = rect.right;
                        int toBottom = rect.bottom;
                        int targetWidth = rect.right;
                        int targetHeight = rect.bottom;

                        if ( arBitmap > arDisplay )
                        {
                            // bitmap is wider than the screen

                            double ratio = (double) rect.right / (double) bw;
                            targetHeight = (int) ( ratio * (double) bh );
                            toTop = ( rect.bottom - targetHeight ) / 2;
                            toBottom = targetHeight;
                        }
                        else
                        {
                            // bitmap is more narrow than the screen

                            double ratio = (double) rect.bottom / (double) bh;
                            targetWidth = (int) ( ratio * (double) bw );
                            toLeft = ( rect.right - targetWidth ) / 2;
                            toRight = targetWidth; 
                        }

                        tracer.Trace( "arBitmap %lf, arScreen %lf, targetHeight %d, targetWidth %d, toLeft %d, toTop %d, toRight %d, toBottom %d\n",
                                      arBitmap, arDisplay, targetHeight, targetWidth, toLeft, toTop, toRight, toBottom );

                        // DrawImage into the display is visibly slow -- you can see drawing from top to bottom.
                        // Instead, stretch the image to an offscreen backing bitmap then BLT that to the display.
                        // Not much point in caching the offscreen hdc and bitmap since DrawImage() is where the time goes by a factor of 50:
                        //     PID 2064 -- bitmap create 294,529,000, draw 10,930,507,700, blt 57,153,200
                        
                        
                        {
                            high_resolution_clock::time_point tA = high_resolution_clock::now();
                            HDC hdcBack = CreateCompatibleDC( hdc );
                            HBITMAP bmpBack = CreateCompatibleBitmap( hdc, rect.right, rect.bottom );
                            high_resolution_clock::time_point tB = high_resolution_clock::now();
                            timeCreate += duration_cast<std::chrono::nanoseconds>( tB - tA ).count();

                            HBITMAP bmpOld = (HBITMAP) SelectObject( hdcBack, bmpBack );
                            FillRect( hdcBack, &rect, brushBlack );

                            {
                                unique_ptr<Graphics> gDCBack( Graphics::FromHDC( hdcBack ) );
                                gDCBack->SetCompositingMode( CompositingMode::CompositingModeSourceCopy );

                                gDCBack->SetCompositingQuality( CompositingQuality::CompositingQualityHighQuality );
                                gDCBack->SetInterpolationMode( InterpolationMode::InterpolationModeHighQualityBicubic );
                                gDCBack->SetPixelOffsetMode( PixelOffsetMode::PixelOffsetModeHighQuality );

                                // No need to SetWrapMode( Tile ) since only one image is being drawn on the Graphics object.

                                Rect rectTo( toLeft, toTop, targetWidth, targetHeight );
                                high_resolution_clock::time_point tC = high_resolution_clock::now();
                                Status x = gDCBack->DrawImage( g_pCurrentBitmap, rectTo, 0, 0, bw, bh, UnitPixel, NULL, NULL );
                                high_resolution_clock::time_point tD = high_resolution_clock::now();
                                timeDraw += duration_cast<std::chrono::nanoseconds>( tD - tC ).count();
                            }
    
                            int len = strlen( g_acPhotoDateTime );
                            if ( 0 != len )
                            {
                                HFONT fontOld = (HFONT) SelectObject( hdcBack, fontText );
                                COLORREF crOldBk = SetBkColor( hdcBack, RGB( 0, 0, 0 ) );
                                COLORREF crOldText = SetTextColor( hdcBack, RGB( 140, 140, 100 ) );
                                UINT taOld = SetTextAlign( hdcBack, TA_RIGHT );
        
                                RECT rectDateTime = { 0, 0, fontHeight * 10, fontHeight };
                                ExtTextOutA( hdcBack, rect.right, rect.bottom - fontHeight, 0, &rectDateTime, g_acPhotoDateTime, len, NULL );

                                WCHAR awcCurrentTime[ 6 ] = { L'h', L'h', L':', L'm', L'm', 0 };
                                const int currentTimeLen = _countof( awcCurrentTime ) - 1;
                                SYSTEMTIME lt = {};
                                GetLocalTime( &lt );
                                WordToWC( awcCurrentTime,     lt.wHour );
                                WordToWC( awcCurrentTime + 3, lt.wMinute );
                                ExtTextOut( hdcBack, rect.right, rect.top, 0, &rectDateTime, awcCurrentTime, currentTimeLen, NULL );

                                SetTextAlign( hdcBack, taOld );
                                SetTextColor( hdcBack, crOldText );
                                SetBkColor( hdcBack, crOldBk );
                                SelectObject( hdcBack, fontOld );
                            }
    
                            high_resolution_clock::time_point tE = high_resolution_clock::now();
                            BOOL bltOK = BitBlt( hdc, 0, 0, rect.right, rect.bottom, hdcBack, 0, 0, SRCCOPY );
                            high_resolution_clock::time_point tF = high_resolution_clock::now();
                            timeBLT += duration_cast<std::chrono::nanoseconds>( tF - tE ).count();
    
                            SelectObject( hdcBack, bmpOld );
                            DeleteObject( bmpBack );
                            DeleteObject( hdcBack );

                            tracer.Trace( "bitmap create %lld, draw %lld, blt %lld\n", timeCreate, timeDraw, timeBLT );
                        }
                    }
                }
                else if ( g_showCaptureDate )
                {
                    // in blank mode; show the current time in a random location.

                    FillRect( hdc, &rect, brushBlack );

                    HFONT fontOld = (HFONT) SelectObject( hdc, fontText );
                    COLORREF crOldBk = SetBkColor( hdc, RGB( 0, 0, 0 ) );
                    COLORREF crOldText = SetTextColor( hdc, RGB( 140, 140, 100 ) );
                    UINT taOld = SetTextAlign( hdc, TA_RIGHT );
        
                    RECT rectDateTime = { 0, 0, fontHeight * 10, fontHeight };

                    WCHAR awcCurrentTime[ 6 ] = { L'h', L'h', L':', L'm', L'm', 0 };
                    const int currentTimeLen = _countof( awcCurrentTime ) - 1;
                    SYSTEMTIME lt = {};
                    GetLocalTime( &lt );
                    WordToWC( awcCurrentTime,     lt.wHour );
                    WordToWC( awcCurrentTime + 3, lt.wMinute );

                    int effectiveWidth = rect.right - rect.left - rectDateTime.right;
                    int effectiveHeight = rect.bottom - rect.top - rectDateTime.bottom;
                    int xoffset = rect.right - ( rand() % effectiveWidth );
                    int yoffset = rect.top + ( rand() % effectiveHeight );

                    ExtTextOut( hdc, xoffset, yoffset, 0, &rectDateTime, awcCurrentTime, currentTimeLen, NULL );

                    SetTextAlign( hdc, taOld );
                    SetTextColor( hdc, crOldText );
                    SetBkColor( hdc, crOldBk );
                    SelectObject( hdc, fontOld );
                }
            }

            EndPaint( hWnd, &ps );
            return 0;
        }
    }
  
    return DefScreenSaverProc( hWnd, message, wParam, lParam );
} //ScreenSaverProc

extern "C" BOOL WINAPI ScreenSaverConfigureDialog( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
    WCHAR awcItem[ 10 ];

    switch ( message ) 
    {
        case WM_INITDIALOG:
        {
            LoadPhotoPath();
            tracer.Trace( "initdialog, path %ws delay %d\n", g_awcPhotoPath, photoDelay );
            SetDlgItemText( hDlg, ID_PATHFIELD, g_awcPhotoPath );
            int delaySelection = 0;

            for ( int i = 0; i < _countof( g_validDelays ); i++ )
            {
                swprintf_s( awcItem, _countof( awcItem ), L"%d", g_validDelays[ i ] );
                if ( g_validDelays[ i ] == photoDelay )
                    delaySelection  = i;

                SendDlgItemMessage( hDlg, ID_DELAY_SECONDS, CB_ADDSTRING, 0, (LPARAM) awcItem );
            }

            int blankSelection = 0;

            for ( int i = 0; i < _countof( g_validBlanks ); i++ )
            {
                swprintf_s( awcItem, _countof( awcItem ), L"%d", g_validBlanks[ i ] );
                if ( g_validBlanks[ i ] == blankDelay )
                    blankSelection  = i;

                SendDlgItemMessage( hDlg, ID_BLANK_MINUTES, CB_ADDSTRING, 0, (LPARAM) awcItem );
            }

            SendDlgItemMessage( hDlg, ID_DELAY_SECONDS, CB_SETCURSEL, delaySelection, 0 );
            SendDlgItemMessage( hDlg, ID_SHOWCAPTUREDATE, BM_SETCHECK, g_showCaptureDate ? BST_CHECKED : BST_UNCHECKED, 0 );
            SendDlgItemMessage( hDlg, ID_BLANK_MINUTES, CB_SETCURSEL, blankSelection, 0 );
            return TRUE;
        }

        case WM_COMMAND:
        {
           switch( LOWORD( wParam ) ) 
           { 
                case IDOK:
                {
                    tracer.Trace( "IDOK top, path %ws\n", g_awcPhotoPath );
                    GetDlgItemText( hDlg, ID_PATHFIELD, g_awcPhotoPath, _countof( g_awcPhotoPath ) );
                    CDJLRegistry::writeStringToRegistry( HKEY_CURRENT_USER, REGISTRY_APP_NAME, REGISTRY_PHOTO_PATH, g_awcPhotoPath );
                    tracer.Trace( "IDOK bottom, path %ws\n", g_awcPhotoPath );

                    int delaySel = (int) SendDlgItemMessage( hDlg, ID_DELAY_SECONDS, CB_GETCURSEL, 0, 0 );

                    if ( delaySel >=0 && delaySel < _countof( g_validDelays ) )
                    {
                        SendDlgItemMessage( hDlg, ID_DELAY_SECONDS, CB_GETLBTEXT, delaySel, (LPARAM) awcItem );
                        CDJLRegistry::writeStringToRegistry( HKEY_CURRENT_USER, REGISTRY_APP_NAME, REGISTRY_PHOTO_DELAY, awcItem );
                    }

                    int showCD = (int) SendDlgItemMessage( hDlg, ID_SHOWCAPTUREDATE, BM_GETCHECK, 0, 0 );
                    CDJLRegistry::writeStringToRegistry( HKEY_CURRENT_USER, REGISTRY_APP_NAME, REGISTRY_PHOTO_SHOWCAPTUREDATE, ( BST_CHECKED == showCD ) ? L"yes" : L"no" );
        
                    int blankSel = (int) SendDlgItemMessage( hDlg, ID_BLANK_MINUTES, CB_GETCURSEL, 0, 0 );

                    if ( blankSel >=0 && blankSel < _countof( g_validBlanks ) )
                    {
                        SendDlgItemMessage( hDlg, ID_BLANK_MINUTES, CB_GETLBTEXT, blankSel, (LPARAM) awcItem );
                        CDJLRegistry::writeStringToRegistry( HKEY_CURRENT_USER, REGISTRY_APP_NAME, REGISTRY_PHOTO_BLANK_DELAY, awcItem );
                    }

                    EndDialog( hDlg, 0 );
                    return TRUE; 
                }

                case IDCANCEL:
                {
                    tracer.Trace( "IDCANCEL path %ws\n", g_awcPhotoPath );
                    EndDialog( hDlg, 0 );
                    return TRUE;
                }
            }
        }
    }    

    return FALSE; 
} //ScreenSaverConfigureDialog

BOOL WINAPI RegisterDialogClasses( HANDLE hInst )
{
    return TRUE;
} //RegisterDialogClasses

