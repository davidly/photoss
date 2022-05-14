#pragma once

// Loads files using WIC and produces GDI+ bitmaps.
// Assumes COM has been initialized already.
// Usage:
//      CWic2Gdi wic2gdi;
//      Bitmap * pbitmap = wic2gdi.GDIBitmapFromWIC( ... );
//

#include <windows.h>
#include <windowsx.h>
#include <gdiplus.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <wincodec.h>

#include <djltrace.hxx>

class CWic2Gdi
{
    private:

        IWICImagingFactory * pIWICFactory;

        template <typename T> static inline void SafeRelease( T *&p )
        {
            if ( NULL != p )
            {
                p->Release();
                p = NULL;
            }
        } //SafeRelease

        static UINT RoundUpTo4( UINT x )
        {
            UINT remainder = x % 4;
            if ( 0 == remainder )
                return x;
            return x + 4 - remainder;
        } //RoundUpTo4

        static HRESULT CreateBitmapFromBitmapSource( IWICBitmapSource *pBitmapSource, Bitmap ** ppBitmap, byte **ppBuffer,
                                                     WICPixelFormatGUID & wicPixelFormat, DWORD gdipPixelFormat )
        {
            *ppBuffer = NULL;
            WICPixelFormatGUID pixelFormat;
            HRESULT hr = pBitmapSource->GetPixelFormat( &pixelFormat );
            if ( FAILED( hr ) )
            {
                tracer.Trace( "can't read pixelformat in CreateBitmapFromBitmapSource: %#x\n", hr );
                return hr;
            }

            if ( wicPixelFormat != pixelFormat )
            {
                tracer.Trace( "  wic pixel format isn't as expected: %#x, 32bgr is %#x\n", pixelFormat, GUID_WICPixelFormat32bppBGR );
                return E_FAIL;
            }
        
            UINT width = 0;
            UINT height = 0;
            if ( SUCCEEDED( hr ) )
                hr = pBitmapSource->GetSize( &width, &height ); 
        
            if ( SUCCEEDED( hr ) )
            {
                UINT cbStride = 4 * width;

                if ( GUID_WICPixelFormat24bppRGB == wicPixelFormat || GUID_WICPixelFormat24bppBGR == wicPixelFormat )
                    cbStride = RoundUpTo4( 3 * width );

                UINT cbBufferSize = cbStride * height;
                BYTE *pbBuffer  = new BYTE[ cbBufferSize ];
        
                // The WIC plugin decoder is invoked in CopyPixels(), which means many failure modes are inevitable.
                // For example, Canon .HIF files fail at CopyPixels(). The transforms happen here as well.

                hr = pBitmapSource->CopyPixels( NULL, cbStride, cbBufferSize, pbBuffer );
        
                if ( SUCCEEDED( hr ) )
                {
                    *ppBitmap = new Gdiplus::Bitmap( width, height, cbStride, gdipPixelFormat, pbBuffer );
                    *ppBuffer = pbBuffer;
                    pbBuffer = NULL;
                }
                else
                {
                    tracer.Trace( "  CreateBitmapFromBitmapSource failed in CopyPixels; likely a codec failure, hr %#x\n", hr );
                    delete pbBuffer;
                    pbBuffer = NULL;
                }
        
                //tracer.Trace( "  gdi+ buffer in bitmap: %p\n", pbBuffer );
            }
        
            return hr;
        } //CreateBitmapFromBitmapSource
        
        HRESULT ScaleAndConvertBitmapToTarget( IWICBitmapSource * pIn, IWICBitmapSource ** ppOut, int targetW, int targetH, WICPixelFormatGUID & targetPixelFormat )
        {
            *ppOut = NULL;
        
            WICPixelFormatGUID pixelFormat;
            HRESULT hr = pIn->GetPixelFormat( &pixelFormat );

            if ( SUCCEEDED( hr ) && targetPixelFormat == pixelFormat && 0 == targetW && 0 == targetH )
            {
                tracer.Trace( "  no need to convert wic pixel format\n" );
                return S_FALSE;
            }

            if ( FAILED( hr ) )
                return hr;
        
            IWICBitmapScaler *pScaler = NULL;
        
            if ( 0 != targetW && 0 != targetH )
            {
                UINT width = 0;
                UINT height = 0;
                pIn->GetSize( &width, &height );
        
                UINT w, h;
                AdjustSizeToFit( width, height, targetW, targetH, w, h );
        
                //tracer.Trace( "img %d %d, target %d %d, final %d %d\n", width, height, targetW, targetH, w, h );
            
                hr = pIWICFactory->CreateBitmapScaler( &pScaler );
        
                if (SUCCEEDED(hr))
                    hr = pScaler->Initialize( pIn, w, h, WICBitmapInterpolationModeHighQualityCubic );
            }
        
            if (SUCCEEDED(hr))
            {
                IWICFormatConverter *pConverter = NULL;
                hr = pIWICFactory->CreateFormatConverter( &pConverter );
        
                if (SUCCEEDED(hr))
                {
                    hr = pConverter->Initialize( pScaler ? pScaler : pIn, targetPixelFormat, WICBitmapDitherTypeNone, NULL, 0.f, WICBitmapPaletteTypeCustom );
        
                    if (SUCCEEDED(hr))
                        hr = pConverter->QueryInterface( IID_PPV_ARGS( ppOut ) );
                }
        
                SafeRelease( pConverter );
            }
        
            SafeRelease( pScaler );
        
            return hr;
        } //ScaleAndConvertBitmapToTarget
        
        static void AdjustSizeToFit( UINT wImg, UINT hImg, int targetW, int targetH, UINT & wOut, UINT & hOut )
        {
            wOut = wImg;
            hOut = hImg;
        
            // Scale the image, sometimes smaller and sometimes larger
            
            double winAR = (double) targetW / (double) targetH;
            double imgAR = (double) wImg / (double) hImg;
            
            if ( winAR > imgAR )
            {
                hOut = targetH;
                wOut = (UINT) round( (double) targetH / (double) hImg * (double) wImg );
            }
            else
            {
                wOut = targetW;
                hOut = (UINT) round( (double) targetW / (double) wImg * (double) hImg );
            }
        } //AdjustSizeToFit

        static Bitmap * ResizeBitmapQuality( Bitmap * pb, int targetW, int targetH, PixelFormat pf )
        {
            Rect rectT( 0, 0, targetW, targetH );
        
            Bitmap * newBitmap = new Bitmap( targetW, targetH, pf );
            unique_ptr<Graphics> g( Graphics::FromImage( newBitmap ) );
        
            g->SetCompositingMode( CompositingMode::CompositingModeSourceCopy );
            g->SetCompositingQuality( CompositingQuality::CompositingQualityHighQuality );
            g->SetInterpolationMode( InterpolationMode::InterpolationModeHighQualityBicubic );
            g->SetPixelOffsetMode( PixelOffsetMode::PixelOffsetModeHighQuality );
        
            g->DrawImage( pb, rectT, 0, 0, pb->GetWidth(), pb->GetHeight(), UnitPixel, NULL, NULL );
        
            return newBitmap;
        } //ResizeBitmapQuality

        static WCHAR const * ExpectedOrientationName( GUID & containerFormat )
        {
            // JPG and JFIF store Orientation under /app1/
        
            if ( ! memcmp( & containerFormat, & GUID_ContainerFormatJpeg, sizeof GUID ) )
                return L"/app1/ifd/{ushort=274}";
        
            // TIFF and many raw formats store Orientation here.
        
            return L"/ifd/{ushort=274}";
        } //ExpectedOrientationName
    
        static void ExifRotate( Image & img, int val, bool flipY )
        {
            if ( 0 != val || flipY )
            {
                RotateFlipType rot = RotateNoneFlipNone;
            
                if ( val == 3 || val == 4 )
                    rot = Rotate180FlipNone;
                else if ( val == 5 || val == 6 )
                    rot = Rotate90FlipNone;
                else if ( val == 7 || val == 8 )
                    rot = Rotate270FlipNone;
            
                if ( val == 2 || val == 4 || val == 5 || val == 7 )
                    rot = (RotateFlipType) ( (int) rot | (int) RotateNoneFlipX );
        
                if ( flipY )
                    rot = (RotateFlipType) ( (int) rot | (int) RotateNoneFlipY );
        
                if ( rot != RotateNoneFlipNone )
                    img.RotateFlip( rot );
            }
        } //ExifRotate

        static int StrideInBytes(int width, int bitsPerPixel)
        {
            // Not sure if it's documented anywhere, but Windows seems to use 4-byte-aligned strides.
            // Stride is the number of bytes per row of an image. The last bytes to get to 4-byte alignment are unused.
            // Based on Internet searches, some other platforms use 8-byte-aligned strides.
            // I don't know how to query the runtime environment to tell what the default stride alignment is. Assume 4.
        
            int bytesPerPixel = bitsPerPixel / 8;
        
            if (0 != (bitsPerPixel % 8))
                bytesPerPixel++;
        
            const int AlignmentForStride = 4;
        
            return (((width * bytesPerPixel) + (AlignmentForStride - 1)) / AlignmentForStride) * AlignmentForStride;
        } //StrideInBytes

        static Bitmap * Rotate90( Bitmap & before )
        {
            Bitmap * after = new Bitmap( before.GetHeight(), before.GetWidth(), PixelFormat24bppRGB );
        
            Rect rectAfter( 0, 0, after->GetWidth(), after->GetHeight() );
            BitmapData bdAfter;
            after->LockBits( &rectAfter, ImageLockModeWrite, PixelFormat24bppRGB, &bdAfter );
            int strideAfter = abs( bdAfter.Stride );
        
            if ( strideAfter != StrideInBytes( after->GetWidth(), 24 ) )
                wprintf( L"stride After not expected\n" );
        
            Rect rectBefore( 0, 0, before.GetWidth(), before.GetHeight() );
            BitmapData bdBefore;
        
            // The native pixel format is 24bppRGB. Reading anything else is much slower.
        
            before.LockBits( &rectBefore, ImageLockModeRead, PixelFormat24bppRGB, &bdBefore );
            int strideBefore = abs( bdBefore.Stride );
        
            if ( strideBefore != StrideInBytes( before.GetWidth(), 24 ) )
                wprintf( L"stride Before not expected\n" );
        
            const int blockSize = 128; // in pixels, which are 3 bytes long. 64 and 256 are each a little slower
            int afterHorRem = after->GetWidth() % blockSize;
            int afterVerRem = after->GetHeight() % blockSize;
            int afterBlocksHor = ( after->GetWidth() / blockSize ) + ( ( 0 == afterHorRem ) ? 0 : 1 );
            int afterBlocksVer = ( after->GetHeight() / blockSize ) + ( ( 0 == afterVerRem ) ? 0 : 1 );
            int afterLastH = afterBlocksHor - 1;
            int afterLastV = afterBlocksVer - 1;
            byte *pBefore = (byte *) bdBefore.Scan0;
            byte *pAfter = (byte *) bdAfter.Scan0;
            int bhm1 = before.GetHeight() - 1;
        
            //for ( int x = 0; x < afterBlocksHor; x++ )
            parallel_for( 0, afterBlocksHor, [&] ( int x )
            {
                int xp = ( ( x == afterLastH ) && ( 0 != afterHorRem ) ) ? afterHorRem : blockSize;
                int xBlock = x * blockSize;
        
                for ( int y = 0; y < afterBlocksVer; y++ )
                {
                    int yp = ( ( y == afterLastV ) && ( 0 != afterVerRem ) ) ? afterVerRem : blockSize;
                    int yp3 = yp * 3;
                    int yBlock = y * blockSize;
                    int yoA = yBlock * strideAfter;
                    byte * pAfterBase = pAfter + yoA;
                    byte * pBeforeBase = pBefore + 3 * yBlock;
        
                    //wprintf( L"  xp: %d, yp: %d\n", xp, yp );
        
                    for ( int xc = 0; xc < xp; xc++ )
                    {
                        int xoA = xBlock + xc;
                        byte * pa = pAfterBase + ( 3 * xoA );
                        int yoB = ( bhm1 - xoA ) * strideBefore;
                        byte * pb = pBeforeBase + yoB;
                        byte * pbend = pb + yp3;
        
                        do
                        {
                            // This code gets generated inline. Note the potentially unaligned word copy, but it's fast
                            //    movzx   eax,word ptr [rdi]
                            //    mov     word ptr [rsi],ax
                            //    movzx   eax,byte ptr [rdi+2]
                            //    mov     byte ptr [rsi+2],al
                            //    add     rdi,3
                            //    mov     qword ptr [rsp+0B8h],rdi
                            //    add     rsi,rbx
                            //    mov     qword ptr [rsp+0C0h],rsi
                            //    cmp     rdi,r15
                            //    jb      cv!Rotate90+0x540 (00007ff7`787fb350)
        
                            memcpy( pa, pb, 3 );
                            pb += 3;
                            pa += strideAfter;
                        } while ( pb < pbend );
                    }
                }
            } );
        
            before.UnlockBits( &bdBefore );
            after->UnlockBits( &bdAfter );
        
            return after;
        } //Rotate90

    public:

        static Bitmap * ResizeGDIPBitmap( Bitmap * pb, int targetW, int targetH, PixelFormat pf )
        {
            UINT w, h;
            AdjustSizeToFit( pb->GetWidth(), pb->GetHeight(), targetW, targetH, w, h );
        
            return ResizeBitmapQuality( pb, w, h, pf );
        } //ResizeGDIPBitmap

        // pwcPath: path of input file or NULL to use pStream
        // pStream: stream of input or NULL to use pwcPath
        // ppBuffer: returns a byte array of the bits held in the returned bitmap or NULL if not needed
        // targetW / targetH: size of the intended window, so the image can be rescaled or 0 to indicate no scaling
        // availableWidth / availableHeight: full original dimensions of the bitmap
        // gdipPixelFormat: pixel format of the GDI+ bitmap created.

        Bitmap * GDIPBitmapFromWIC( WCHAR * pwcPath, IStream * pStream, byte **ppBuffer, int targetW, int targetH,
                                    int * availableWidth, int * availableHeight, DWORD gdipPixelFormat = PixelFormat32bppRGB )
        {
        
            //tracer.Trace( "opening %ws\n", pwcPath );

            // Note: GDIPlus and WIC color formats apparently flip R and B

            WICPixelFormatGUID wicPixelFormat;

            if ( PixelFormat24bppRGB == gdipPixelFormat )
                wicPixelFormat = GUID_WICPixelFormat24bppBGR;
            else if ( PixelFormat32bppRGB == gdipPixelFormat )
                wicPixelFormat = GUID_WICPixelFormat32bppBGR;
            else
            {
                tracer.Trace( "unsupported GDI+ PixelFormat %#x\n", gdipPixelFormat );
                return 0;
            }
        
            *ppBuffer = NULL;
            IWICBitmapDecoder *pDecoder = NULL;
            HRESULT hr = S_OK;
        
            //tracer.Trace( "  GDIPBitmapFromWIC path %p, stream %p\n", pwcPath, pStream );
        
            if ( NULL != pwcPath )
                hr = pIWICFactory->CreateDecoderFromFilename( pwcPath, NULL, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &pDecoder );
            else
                hr = pIWICFactory->CreateDecoderFromStream( pStream, NULL, WICDecodeMetadataCacheOnDemand, &pDecoder );
        
            if (FAILED(hr))
                tracer.Trace( "  hr from createdecoderfrom filename/stream: %#x for %ws\n", hr, pwcPath );

            WCHAR const * orientationName = 0;
        
            if ( SUCCEEDED( hr ) )
            {
                GUID containerFormat = {0};
                hr = pDecoder->GetContainerFormat( &containerFormat );
                if ( FAILED( hr ) )
                    tracer.Trace( "hr from GetContainerFormat %#x\n", hr );
                else
                    orientationName = ExpectedOrientationName( containerFormat );
            }
        
            IWICBitmapFrameDecode *pFrame = NULL;
            if (SUCCEEDED(hr))
                hr = pDecoder->GetFrame( 0, &pFrame );

            IWICMetadataQueryReader *pReader = NULL;
            if (SUCCEEDED(hr))
                hr = pFrame->GetMetadataQueryReader( &pReader );

            int orientation = 0;

            if ( SUCCEEDED( hr ) )
            {
                PROPVARIANT value;
                PropVariantInit( &value );
                hr = pReader->GetMetadataByName( orientationName, &value );

                if ( SUCCEEDED( hr ) && ( 0x12 == value.vt ) )
                    orientation = value.iVal;
                else if ( WINCODEC_ERR_PROPERTYNOTFOUND == hr )
                    hr = S_OK;
                else
                    tracer.Trace( "  failure. hr from Get Orientation %#x\n", hr );
         
                SafeRelease( pReader );
            }

            IWICBitmapSource *pBitmapSource = NULL;
            if ( SUCCEEDED( hr ) )
                hr = pFrame->QueryInterface( IID_IWICBitmapSource, reinterpret_cast<void **>( &pBitmapSource ) );

            #if false

                // Don't do the rotate here. The performance an be terrible. From msft documentation:
                // IWICBitmapFipRotator requests data on a per-pixel basis, while WIC codecs provide data on a per-scanline basis.
                // This causes the fliprotator object to exhibit n behavior if there is no buffering. This occurs because each pixel in the 
                // transformed image requires an entire scanline to be decoded in the file. It is recommended that you buffer the image using
                // IWICBitmap, or flip/rotate the image using Direct2D.

                if ( SUCCEEDED( hr ) )
                {
                    if ( ( orientation >= 3 ) && ( orientation <= 8 ) )
                    {
                        IWICBitmapFlipRotator *pRotator;
                        hr = pIWICFactory->CreateBitmapFlipRotator( & pRotator );
                        if (SUCCEEDED(hr))
                        {
                            WICBitmapTransformOptions wbto = WICBitmapTransformRotate0;
                     
                            if ( orientation == 3 || orientation == 4 )
                                wbto = WICBitmapTransformRotate180;
                            else if ( orientation == 5 || orientation == 6 )
                                wbto = WICBitmapTransformRotate90;
                            else if ( orientation == 7 || orientation == 8 )
                                wbto = WICBitmapTransformRotate270;
                        
                            //if ( val == 2 || val == 4 || val == 5 || val == 7 )
                            //    rot = (RotateFlipType) ( (int) rot | (int) RotateNoneFlipX );
                 
                            if ( WICBitmapTransformRotate0 != wbto )
                            {
                                hr = pRotator->Initialize( pBitmapSource, wbto );
                                tracer.Trace( "  rotated wic bitmap exif orientation %d, wic transform %d, hr %#x\n", orientation, wbto, hr );
                     
                                if ( SUCCEEDED( hr ) )
                                {
                                    SafeRelease( pBitmapSource );
                                    pBitmapSource = pRotator;
                                    pRotator = NULL;
                                }
                            }
    
                            SafeRelease( pRotator );
                        }
                    }
                }
    
            #else

                if ( orientation >= 5 && orientation <= 8 )
                {
                    int tmp = targetW;
                    targetW = targetH;
                    targetH = tmp;
                }

            #endif
        
            UINT width, height;
            if ( SUCCEEDED( hr ) )
            {
                hr = pBitmapSource->GetSize( &width, &height );
                if (SUCCEEDED(hr))
                {
                    *availableWidth = (int) width;
                    *availableHeight = (int) height;
                }
            }
        
            IWICBitmapSource *pConverted = NULL;
            if ( SUCCEEDED( hr ) )
                hr = ScaleAndConvertBitmapToTarget( pBitmapSource, &pConverted, targetW, targetH, wicPixelFormat );
        
            if ( S_FALSE == hr )
            {
                // no need to convert -- use the original bitmap
        
                pConverted = pBitmapSource;
                pBitmapSource = NULL;
                hr = S_OK;
            }
        
            SafeRelease( pBitmapSource );
        
            Bitmap * pBitmap = 0;
            if ( SUCCEEDED( hr ) )
                hr = CreateBitmapFromBitmapSource( pConverted, & pBitmap, ppBuffer, wicPixelFormat, gdipPixelFormat );
        
            SafeRelease( pConverted );
            SafeRelease( pDecoder );
            SafeRelease( pFrame );

            // Instead of rotating in the WIC pipeline above, do it here.

            if ( pBitmap && orientation )
            {
                if ( ( 6 == orientation ) && ( PixelFormat24bppRGB == gdipPixelFormat ) )
                {
                    // 4.7 times faster than ExifRotate

                    Bitmap * pRotated = Rotate90( *pBitmap );
                    delete pBitmap;
                    pBitmap = pRotated;
                }
                else
                {
                    ExifRotate( *pBitmap, orientation, FALSE );
                }
            }

            return pBitmap;
        } //GDIPBitmapFromWIC

        CWic2Gdi()
        {
            pIWICFactory = 0;

            HRESULT hr = CoCreateInstance( CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS( &pIWICFactory ) );

            if ( FAILED( hr ) )
                tracer.Trace( "unable to create WIC %#x\n", hr );
        }

        bool Ok() { return ( 0 != pIWICFactory ); }

        void ShutdownWic()
        {
            SafeRelease( pIWICFactory );
        } //ShutdownWic

        ~CWic2Gdi()
        {
            ShutdownWic();
        }
}; //CWic2Gdi

