#pragma once

//
// Hard-coded crop factors for various cameras.
// The list of cameras is not exhaustive by any stretch.
//

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <float.h>
#include <eh.h>
#include <math.h>
#include <assert.h>

#include <memory>
#include <vector>

#include "djltrace.hxx"

//#define GenerateSortedTable

class CCropFactor
{
    private:

        struct CropFactor
        {
            const char * pcCamera;
            double       cropFactor;

            CropFactor( const char * camera = 0, double crop = 0.0 ) : pcCamera( camera ), cropFactor( crop ) {}
        };
        
        const double width_PhaseOne =       53.9;
        const double height_PhaseOne =      40.4;
        const double width_Fuji_Medium =    43.811610;
        const double height_Fuji_Medium =   32.858708;
        const double width_FF =             36.0;
        const double height_FF =            24.0;
        const double width_APSC =           23.6;    // Ricoh is 23.5. Some Nikon APSC cameras are bigger. There is no standard. Fuji is this.
        const double height_APSC =          15.6;
        const double width_NIKON_APSC =     23.7;
        const double height_NIKON_APSC =    15.5;
        const double width_CANON_APSC =     22.2;
        const double height_CANON_APSC =    14.8;
        const double width_43 =             17.3;
        const double height_43 =            13.0;
        const double width_ONE_INCH =       13.2;
        const double height_ONE_INCH =       8.8;
        const double width_OOOPS =           7.44;   // 1/1.7  One Over One Point Seven
        const double height_OOOPS =          5.58;
        const double width_SD1100 =          5.75;   // 1/2.5 4x3
        const double height_SD1100 =         4.32;
        const double width_SD780 =           6.17;   // 1/2.3 4x3
        const double height_SD780 =          4.55;
        const double width_Quarter =         3.2;
        const double height_Quarter =        2.4;
        const double width_iPhone4 =         4.54;   // 1/3.2
        const double height_iPhone4 =        3.42;
        const double width_SD10 =            5.744;  // 4x3 1/2.5 38mm equiv. (5.744 x 4.308)
        const double height_SD10 =           4.308;
        const double width_DMC_ZS1 =         5.744;
        const double height_DMC_ZS1 =        4.308;
        const double width_S70 =             7.144;  // 1/1.8 (7.144 x 5.358)
        const double height_S70 =            5.358;
        const double width_DC210 =           5.25;   // 6.59 crop 4x3
        const double height_DC210 =          3.94;
        const double width_Third =           4.8;
        const double height_Third =          3.6;
        const double width_Two33 =           6.08;
        const double height_Two33 =          4.56;
        const double width_TwoPoint7 =       5.312;  // 1/2.7
        const double height_TwoPoint7 =      3.984;
        const double width_ThreePoint2 =     4.54;   // 1/3.2
        const double height_ThreePoint2 =    3.42;
        const double width_OneFive =         8.64;   // 1/1.5
        const double height_OneFive =        6.0;
        const double width_CANON_APSH =     28.7;    // also seen: 28.9 x 18.6. wikipedia: 1.255 for the 1D, 1.28 for the EOS-1D Mk III and 1.29 1d mk iv
        const double height_CANON_APSH =    19.1;
        const double width_Samsung_Tablet =  3.6;   // this one, anyways: https://www.devicespecifications.com/en/model/cbe82c7b
        const double height_Samsung_Tablet = 2.7;
        const double width_SONY_EX1 =        6.56;    // sony 16x9. no definitive source, but this seems accurate for a 1/2" sensor.
        const double height_SONY_EX1 =       3.69;
        
        static double sqr( double d ) { return d * d; }
        
        const double diagonal_PhaseOne =       sqrt( sqr( width_PhaseOne ) + sqr( height_PhaseOne ) );
        const double diagonal_Fuji_Medium =    sqrt( sqr( width_Fuji_Medium ) + sqr( height_Fuji_Medium ) );
        const double diagonal_FF =             sqrt( sqr( width_FF ) + sqr( height_FF ) );
        const double diagonal_APSC =           sqrt( sqr( width_APSC ) + sqr( height_APSC ) );
        const double diagonal_NIKON_APSC =     sqrt( sqr( width_NIKON_APSC ) + sqr( height_NIKON_APSC ) );
        const double diagonal_CANON_APSC =     sqrt( sqr( width_CANON_APSC ) + sqr( height_CANON_APSC ) );
        const double diagonal_43 =             sqrt( sqr( width_43 ) + sqr( height_43 ) );
        const double diagonal_ONE_INCH =       sqrt( sqr( width_ONE_INCH ) + sqr( height_ONE_INCH ) );
        const double diagonal_OOOPS =          sqrt( sqr( width_OOOPS ) + sqr( height_OOOPS ) );
        const double diagonal_SD1100 =         sqrt( sqr( width_SD1100 ) + sqr( height_SD1100 ) );
        const double diagonal_SD780 =          sqrt( sqr( width_SD780 ) + sqr( height_SD780 ) );
        const double diagonal_Quarter =        sqrt( sqr( width_Quarter ) + sqr( height_Quarter ) );
        const double diagonal_iPhone4 =        sqrt( sqr( width_iPhone4 ) + sqr( height_iPhone4 ) );
        const double diagonal_SD10 =           sqrt( sqr( width_SD10 ) + sqr( height_SD10 ) );
        const double diagonal_DMC_ZS1 =        sqrt( sqr( width_DMC_ZS1 ) + sqr( height_DMC_ZS1 ) );
        const double diagonal_S70 =            sqrt( sqr( width_S70 ) + sqr( height_S70 ) );
        const double diagonal_DC210 =          sqrt( sqr( width_DC210 ) + sqr( height_DC210 ) );
        const double diagonal_Third =          sqrt( sqr( width_Third ) + sqr( height_Third ) );
        const double diagonal_Two33 =          sqrt( sqr( width_Two33 ) + sqr( height_Two33 ) );
        const double diagonal_TwoPoint7 =      sqrt( sqr( width_TwoPoint7 ) + sqr( height_TwoPoint7 ) );
        const double diagonal_Kodak_V530 =     10.2;
        const double diagonal_ThreePoint2 =    sqrt( sqr( width_ThreePoint2 ) + sqr( height_ThreePoint2 ) );
        const double diagonal_OneFive =        sqrt( sqr( width_OneFive ) + sqr( height_OneFive ) );
        const double diagonal_CANON_APSH =     sqrt( sqr( width_CANON_APSH ) + sqr( height_CANON_APSH ) );
        const double diagonal_Samsung_Tablet = sqrt( sqr( width_Samsung_Tablet ) + sqr( height_Samsung_Tablet ) );
        const double diagonal_SONY_EX1 =       sqrt( sqr( width_SONY_EX1 ) + sqr( height_SONY_EX1 ) );
        
        const double crop_PhaseOne =       diagonal_FF / diagonal_PhaseOne;
        const double crop_Fuji_Medium =    diagonal_FF / diagonal_Fuji_Medium;
        const double crop_APSC =           diagonal_FF / diagonal_APSC;
        const double crop_NIKON_APSC =     diagonal_FF / diagonal_NIKON_APSC;
        const double crop_CANON_APSC =     diagonal_FF / diagonal_CANON_APSC;
        const double crop_43 =             diagonal_FF / diagonal_43;
        const double crop_ONE_INCH =       diagonal_FF / diagonal_ONE_INCH;
        const double crop_OOOPS =          diagonal_FF / diagonal_OOOPS;
        const double crop_SD1100 =         diagonal_FF / diagonal_SD1100;
        const double crop_SD780 =          diagonal_FF / diagonal_SD780;
        const double crop_Quarter =        diagonal_FF / diagonal_Quarter;
        const double crop_iPhone4 =        diagonal_FF / diagonal_iPhone4;
        const double crop_SD10 =           diagonal_FF / diagonal_SD10;
        const double crop_DMC_ZS1 =        diagonal_FF / diagonal_DMC_ZS1;
        const double crop_S70 =            diagonal_FF / diagonal_S70;
        const double crop_DC210 =          diagonal_FF / diagonal_DC210;
        const double crop_Third =          diagonal_FF / diagonal_Third;
        const double crop_Two33 =          diagonal_FF / diagonal_Two33;
        const double crop_TwoPoint7 =      diagonal_FF / diagonal_TwoPoint7;
        const double crop_Kodak_V530 =     diagonal_FF / diagonal_Kodak_V530;
        const double crop_ThreePoint2 =    diagonal_FF / diagonal_ThreePoint2;
        const double crop_OneFive =        diagonal_FF / diagonal_OneFive;
        const double crop_CANON_APSH =     diagonal_FF / diagonal_CANON_APSH;
        const double crop_Samsung_Tablet = diagonal_FF / diagonal_Samsung_Tablet;
        const double crop_SONY_EX1 =       diagonal_FF / diagonal_SONY_EX1;

        vector<CropFactor> cameras;
        
        static int CameraEntryCompare( const void * a, const void * b )
        {
            CropFactor *pa = (CropFactor *) a;
            CropFactor *pb = (CropFactor *) b;
        
            return ( strcmp( pa->pcCamera, pb->pcCamera ) );
        } //CameraEntryCompare
        
    public:

        CCropFactor()
        {

#ifdef GenerateSortedTable // Generate the table, then paste that into the else section below

            // add them all to a vector so they can be sorted.

            cameras.push_back( CropFactor( "ADR6410LVW", 7.0 ) );  // just a guess for an HTC droid phone
            cameras.push_back( CropFactor( "AE-1", 1.0 ) );        // this is a canon film camera, but somehow some images are stamped with it
            cameras.push_back( CropFactor( "C3000Z", 7.0 ) );      // Olympus camera. This is a guess since I couldn't find info
            cameras.push_back( CropFactor( "C5060WZ", crop_S70 ) );// Olympus
            cameras.push_back( CropFactor( "C6902", 5.6 ) );
            cameras.push_back( CropFactor( "COOLPIX P7100", crop_OOOPS ) );
            cameras.push_back( CropFactor( "CYBERSHOT", 7.0 ) );   // random guess. no idea
            cameras.push_back( CropFactor( "CanoScan 8800F", 1.0 ) );               // not really, but it doesn't matter
            cameras.push_back( CropFactor( "Canon EOS 10D", crop_CANON_APSC ) );
            cameras.push_back( CropFactor( "Canon EOS 1300D", crop_CANON_APSC ) );
            cameras.push_back( CropFactor( "Canon EOS 20D", crop_CANON_APSC ) );
            cameras.push_back( CropFactor( "Canon EOS 30D", crop_CANON_APSC ) );
            cameras.push_back( CropFactor( "Canon EOS 40D", crop_CANON_APSC ) );
            cameras.push_back( CropFactor( "Canon EOS 50D", crop_CANON_APSC ) );
            cameras.push_back( CropFactor( "Canon EOS 5D Mark II", 1.0 ) );
            cameras.push_back( CropFactor( "Canon EOS 5D Mark III", 1.0 ) );
            cameras.push_back( CropFactor( "Canon EOS 5D Mark IV", 1.0 ) );
            cameras.push_back( CropFactor( "Canon EOS 5D", 1.0 ) );
            cameras.push_back( CropFactor( "Canon EOS 5DS R", 1.0 ) );
            cameras.push_back( CropFactor( "Canon EOS 5DS", 1.0 ) );
            cameras.push_back( CropFactor( "Canon EOS 60D", crop_CANON_APSC ) );
            cameras.push_back( CropFactor( "Canon EOS 6D Mark II", 1.0 ) );
            cameras.push_back( CropFactor( "Canon EOS 6D", 1.0 ) );
            cameras.push_back( CropFactor( "Canon EOS 700D", crop_CANON_APSC ) );
            cameras.push_back( CropFactor( "Canon EOS 70D", crop_CANON_APSC ) );
            cameras.push_back( CropFactor( "Canon EOS 77D", crop_CANON_APSC ) );
            cameras.push_back( CropFactor( "Canon EOS 7D Mark II", crop_CANON_APSC ) );
            cameras.push_back( CropFactor( "Canon EOS 7D", crop_CANON_APSC ) );
            cameras.push_back( CropFactor( "Canon EOS 80D", crop_CANON_APSC ) );
            cameras.push_back( CropFactor( "Canon EOS 90D", crop_CANON_APSC ) );
            cameras.push_back( CropFactor( "Canon EOS D30", crop_CANON_APSC ) );
            cameras.push_back( CropFactor( "Canon EOS D60", crop_CANON_APSC ) );
            cameras.push_back( CropFactor( "Canon EOS DIGITAL REBEL XT", crop_CANON_APSC ) );
            cameras.push_back( CropFactor( "Canon EOS DIGITAL REBEL", crop_CANON_APSC ) );
            cameras.push_back( CropFactor( "Canon EOS M", crop_CANON_APSC ) );
            cameras.push_back( CropFactor( "Canon EOS M100", crop_CANON_APSC ) );
            cameras.push_back( CropFactor( "Canon EOS M2", crop_CANON_APSC ) );
            cameras.push_back( CropFactor( "Canon EOS M3", crop_CANON_APSC ) );
            cameras.push_back( CropFactor( "Canon EOS M5", crop_CANON_APSC ) );
            cameras.push_back( CropFactor( "Canon EOS M50", crop_CANON_APSC ) );
            cameras.push_back( CropFactor( "Canon EOS M6 Mark II", crop_CANON_APSC ) );
            cameras.push_back( CropFactor( "Canon EOS M6", crop_CANON_APSC ) );
            cameras.push_back( CropFactor( "Canon EOS R5", 1.0 ) );
            cameras.push_back( CropFactor( "Canon EOS REBEL T2i", crop_CANON_APSC ) );
            cameras.push_back( CropFactor( "Canon EOS REBEL T3", crop_CANON_APSC ) );
            cameras.push_back( CropFactor( "Canon EOS REBEL T3i", crop_CANON_APSC ) );
            cameras.push_back( CropFactor( "Canon EOS REBEL T4i", crop_CANON_APSC ) );
            cameras.push_back( CropFactor( "Canon EOS REBEL T5", crop_CANON_APSC ) );
            cameras.push_back( CropFactor( "Canon EOS REBEL T6", crop_CANON_APSC ) );
            cameras.push_back( CropFactor( "Canon EOS RP", 1.0 ) );
            cameras.push_back( CropFactor( "Canon EOS Rebel T6", crop_CANON_APSC ) );
            cameras.push_back( CropFactor( "Canon EOS-1D X", 1.0 ) );
            cameras.push_back( CropFactor( "Canon EOS-1D Mark II N", crop_CANON_APSH ) );
            cameras.push_back( CropFactor( "Canon EOS-1D Mark II", crop_CANON_APSH ) );
            cameras.push_back( CropFactor( "Canon EOS-1D Mark III", crop_CANON_APSH ) );
            cameras.push_back( CropFactor( "Canon EOS-1D Mark IV", crop_CANON_APSH ) );
            cameras.push_back( CropFactor( "Canon EOS-1D", crop_CANON_APSH ) );
            cameras.push_back( CropFactor( "Canon EOS-1DS", 1.0 ) );
            cameras.push_back( CropFactor( "Canon EOS-1Ds Mark II", 1.0 ) );
            cameras.push_back( CropFactor( "Canon EOS-1Ds Mark III", 1.0 ) );
            cameras.push_back( CropFactor( "Canon EOS-1Ds Mark IV", 1.0 ) );
            cameras.push_back( CropFactor( "Canon PowerShot A20", crop_SD780 ) );         // 1/2.3
            cameras.push_back( CropFactor( "Canon PowerShot A430", crop_Third ) );
            cameras.push_back( CropFactor( "Canon PowerShot A520", crop_S70 ) );
            cameras.push_back( CropFactor( "Canon PowerShot A60", crop_TwoPoint7 ) );     // 1/2.7 5.312x3.984
            cameras.push_back( CropFactor( "Canon PowerShot A610", crop_S70 ) );
            cameras.push_back( CropFactor( "Canon PowerShot A620", crop_S70 ) );
            cameras.push_back( CropFactor( "Canon PowerShot A80", crop_S70 ) );           // 1/1.8
            cameras.push_back( CropFactor( "Canon PowerShot A85", crop_TwoPoint7 ) );
            cameras.push_back( CropFactor( "Canon PowerShot G5", crop_S70 ) );
            cameras.push_back( CropFactor( "Canon PowerShot G5 X Mark II", crop_ONE_INCH ) );
            cameras.push_back( CropFactor( "Canon PowerShot G7 X Mark II", crop_ONE_INCH ) );
            cameras.push_back( CropFactor( "Canon PowerShot G7 X", crop_ONE_INCH ) );
            cameras.push_back( CropFactor( "Canon PowerShot G9 X Mark II", crop_ONE_INCH ) );
            cameras.push_back( CropFactor( "Canon PowerShot G9", crop_OOOPS ) );
            cameras.push_back( CropFactor( "Canon PowerShot S100", crop_OOOPS ) );
            cameras.push_back( CropFactor( "Canon PowerShot S2 IS", crop_SD1100 ) );
            cameras.push_back( CropFactor( "Canon PowerShot S200", crop_OOOPS ) );
            cameras.push_back( CropFactor( "Canon PowerShot S70", crop_S70 ) );
            cameras.push_back( CropFactor( "Canon PowerShot S95", crop_OOOPS ) );
            cameras.push_back( CropFactor( "Canon PowerShot SD10", crop_SD10 ) ); 
            cameras.push_back( CropFactor( "Canon PowerShot SD1100 IS", crop_SD1100 ) );
            cameras.push_back( CropFactor( "Canon PowerShot SD450", crop_SD1100 ) );
            cameras.push_back( CropFactor( "Canon PowerShot SD550", crop_S70 ) );
            cameras.push_back( CropFactor( "Canon PowerShot SD780 IS", crop_SD780 ) );
            cameras.push_back( CropFactor( "Canon PowerShot SD790 IS", crop_SD780 ) );           // same as SD780
            cameras.push_back( CropFactor( "Canon PowerShot SX110 IS", crop_SD1100 ) );          // 1/2.5"
            cameras.push_back( CropFactor( "Canon PowerShot SX530 HS", crop_SD780 ) );
            cameras.push_back( CropFactor( "Canon PowerShot SX600 HS", crop_SD780 ) );           // 1/2.3
            cameras.push_back( CropFactor( "Canon VIXIA HF10", crop_ThreePoint2 ) ); // 1/3.2
            cameras.push_back( CropFactor( "DC-S1R", 1.0 ) );                   // Panasonic L mount
            cameras.push_back( CropFactor( "DC-ZS200", crop_ONE_INCH ) );
            cameras.push_back( CropFactor( "DC210 Zoom (V03.10)", crop_DC210 ) );   // Kodak camera
            cameras.push_back( CropFactor( "DCR-TRV30", crop_Quarter ) );       // 1/4
            cameras.push_back( CropFactor( "DMC-CM1", crop_ONE_INCH ) );
            cameras.push_back( CropFactor( "DMC-FS3", crop_DMC_ZS1 ) );
            cameras.push_back( CropFactor( "DMC-FX8", crop_DMC_ZS1 ) );
            cameras.push_back( CropFactor( "DMC-FZ5", crop_SD1100 ) );
            cameras.push_back( CropFactor( "DMC-G3", crop_43 ) );
            cameras.push_back( CropFactor( "DMC-GF1", crop_43 ) );
            cameras.push_back( CropFactor( "DMC-GF2", crop_43 ) );
            cameras.push_back( CropFactor( "DMC-GM1", crop_43 ) );
            cameras.push_back( CropFactor( "DMC-GX7", crop_43 ) );
            cameras.push_back( CropFactor( "DMC-LX100", crop_43 ) );            // 4/3
            cameras.push_back( CropFactor( "DMC-TS2", crop_Two33 ) );
            cameras.push_back( CropFactor( "DMC-TS3", crop_Two33 ) );           // not precise -- it's actually 6.13x4.6
            cameras.push_back( CropFactor( "DMC-ZS1", crop_DMC_ZS1 ) );         // 1/2.5
            cameras.push_back( CropFactor( "DMC-ZS100", crop_ONE_INCH ) );
            cameras.push_back( CropFactor( "DMC-ZS7", crop_Two33 ) );           // 1/2.33
            cameras.push_back( CropFactor( "DSC-N1", crop_S70 ) );              // close enough. actually 1/1.8 7.144x5.358
            cameras.push_back( CropFactor( "DSC-P52", crop_TwoPoint7 ) );       // 5.33x4
            cameras.push_back( CropFactor( "DSC-RX1", 1.0 ) );
            cameras.push_back( CropFactor( "DSC-RX100", crop_ONE_INCH ) );      // 1
            cameras.push_back( CropFactor( "DSC-W120", crop_SD1100 ) );
            cameras.push_back( CropFactor( "DSC-W560", 5.6 ) );                 // 1/2.5
            cameras.push_back( CropFactor( "DSC-W80", crop_DMC_ZS1 ) );         // 1/2.5 5.8x4.3 (zs1 is close enough)
            cameras.push_back( CropFactor( "DSC-W800", crop_SD780 ) );          // 6.17x4.55 1/2.3
            cameras.push_back( CropFactor( "DSC-WX1", 5.623 ) );                // 1/2.4
            cameras.push_back( CropFactor( "DSC-WX300", crop_SD780 ) );
            cameras.push_back( CropFactor( "DV 5700", 7.0 ) );                  // guess. Aiptek. No focal length info anyway
            cameras.push_back( CropFactor( "DiMAGE 7", 3.9 ) );
            cameras.push_back( CropFactor( "Digital Link", 1.0 ) );             // these files don't have a focal length anyway; 3rd-party film scanner
            cameras.push_back( CropFactor( "E-M10MarkII", crop_43 ) );
            cameras.push_back( CropFactor( "E-PM2", crop_43 ) );
            cameras.push_back( CropFactor( "E3100", crop_TwoPoint7 ) );         // 1/2.7 nikon coolpix 
            cameras.push_back( CropFactor( "E5200", crop_S70 ) );               // nikon coolpix
            cameras.push_back( CropFactor( "E5400", crop_S70 ) );               // nikon 1/1.8
            cameras.push_back( CropFactor( "E6653", 5.6 ) );
            cameras.push_back( CropFactor( "E990", crop_S70 ) );                // nikon coolpix
            cameras.push_back( CropFactor( "Electro 35 GSN", 1.0 ) );           // This is a Yashica film camera, but somehow images are stamped with it
            cameras.push_back( CropFactor( "EOS 5D Mark II", 1.0 ) );
            cameras.push_back( CropFactor( "Epson Stylus NX420", 1.0 ) );       // scanner
            cameras.push_back( CropFactor( "EX1", crop_SONY_EX1 ) );            // sony pmw-ex1. three 1/2 inch
            cameras.push_back( CropFactor( "FinePix F900EXR", 5.326 ) );        // 1/2
            cameras.push_back( CropFactor( "FinePix S3Pro", crop_APSC ) );      // fujifilm with a nikon f mount!
            cameras.push_back( CropFactor( "FinePix4900ZOOM", crop_OOOPS ) );   // 1/1.7
            cameras.push_back( CropFactor( "FinePixS2Pro", crop_APSC ) );
            cameras.push_back( CropFactor( "FrontRow Wear", 5.08 ) );           // 1 x 1/5" sensor to mm
            cameras.push_back( CropFactor( "G8141", crop_SD780 ) );             // Sony phone. 1/2.3"
            cameras.push_back( CropFactor( "GFX 100", crop_Fuji_Medium ) );     // Fujifilm-style Medium format 4x3
            cameras.push_back( CropFactor( "GFX 50R", crop_Fuji_Medium ) );     // Fujifilm-style Medium format 4x3
            cameras.push_back( CropFactor( "GFX 50S", crop_Fuji_Medium ) );     // Fujifilm-style Medium format 4x3
            cameras.push_back( CropFactor( "GFX100S", crop_Fuji_Medium ) );     // Fujifilm-style Medium format 4x3
            cameras.push_back( CropFactor( "GR II", crop_APSC ) );
            cameras.push_back( CropFactor( "H1A1000", 7.0 ) );                  // red hydrogen one. This is just a guess
            cameras.push_back( CropFactor( "H8166", crop_Two33 ) );             // sony phone
            cameras.push_back( CropFactor( "HD7", crop_Two33 ) );               // HTC phone, no data online about sensor size, so guess
            cameras.push_back( CropFactor( "HDR-SR1", crop_Third ) );           // 1/3
            cameras.push_back( CropFactor( "HERO6 Black", crop_SD780 ) );       // 1/2.3
            cameras.push_back( CropFactor( "HP Scanjet 4800", 1.0 ) );          // this is a lie, but it doesn't matter
            cameras.push_back( CropFactor( "HTC Touch Diamond P370", 10.0 ) );  // really, unknown sensor size. But these files don't have focal length anyway
            cameras.push_back( CropFactor( "HTC Touch Diamond P3700", 10.0 ) ); // really, unknown sensor size. But these files don't have focal length anyway
            cameras.push_back( CropFactor( "HTC-8900", 7.68 ) );                // this is an educated guess
            cameras.push_back( CropFactor( "Hewlett-Packard PSC 750 Scanner", 1.0 ) ); // not really, but it doesn't matter
            cameras.push_back( CropFactor( "HP PhotoSmart C945 (V01.60)", 1.0 ) ); // not really, but it doesn't matter
            cameras.push_back( CropFactor( "id313", crop_Two33 ) );             // low-end nokia. no data online, so this is a guess
            cameras.push_back( CropFactor( "ILCE-7", 1.0 ) );                   // sony full-frame
            cameras.push_back( CropFactor( "ILCE-7M3", 1.0 ) );                 // sony full-frame
            cameras.push_back( CropFactor( "ILCE-7RM2", 1.0 ) );                // sony a7r ii full-frame
            cameras.push_back( CropFactor( "ILCE-7S", 1.0 ) );                  // sony full-frame
            cameras.push_back( CropFactor( "KODAK DC240 ZOOM DIGITAL CAMERA", crop_DC210 ) );
            cameras.push_back( CropFactor( "KODAK EASYSHARE V1003 ZOOM DIGITAL CAMERA", crop_S70 ) );   // 1/1.8
            cameras.push_back( CropFactor( "KODAK V530 ZOOM DIGITAL CAMERA", crop_Kodak_V530 ) ); // 1/2.5
            cameras.push_back( CropFactor( "Kodak CLAS Digital Film Scanner / HR200", 1.0 ) ); // doesn't matter
            cameras.push_back( CropFactor( "L16", 1.0 ) );                      // not really; multiple sensors
            cameras.push_back( CropFactor( "LEICA M MONOCHROM (Typ 246)", 1.0 ) );
            cameras.push_back( CropFactor( "LEICA M10", 1.0 ) );
            cameras.push_back( CropFactor( "LEICA SL (Typ 601)", 1.0 ) );
            cameras.push_back( CropFactor( "LEICA SL2-S", 1.0 ) );
            cameras.push_back( CropFactor( "LEICA Q (Typ 116)", 1.0 ) );
            cameras.push_back( CropFactor( "LEICA Q2 MONO", 1.0 ) );
            cameras.push_back( CropFactor( "LEICA Q2", 1.0 ) );
            cameras.push_back( CropFactor( "LEICA X-U (Typ 113)", crop_APSC ) );
            cameras.push_back( CropFactor( "LS-5000", 1.0 ) );                  // it's a scanner
            cameras.push_back( CropFactor( "LS-9000", 1.0 ) );                  // it's a scanner
            cameras.push_back( CropFactor( "Lumia 1020", crop_OneFive ) );      // 1/1.5
            cameras.push_back( CropFactor( "Lumia 520", 7.68 ) );               // ?
            cameras.push_back( CropFactor( "Lumia 830", 7.68 ) );               // ?
            cameras.push_back( CropFactor( "Lumia 920", 7.68 ) );               // 1/3.2
            cameras.push_back( CropFactor( "Lumia 950 XL", 5.623 ) );           // 1/2.4
            cameras.push_back( CropFactor( "Lumia 950", 5.623 ) );              // 1/2.4
            cameras.push_back( CropFactor( "MHS-PM1", crop_SD1100 ) );          // 1/2.5"
            cameras.push_back( CropFactor( "MX880 series", 1.0 ) );             // it's a scanner
            cameras.push_back( CropFactor( "Nexus 9", crop_Quarter ) );
            cameras.push_back( CropFactor( "NEX-3N", crop_APSC ) );             // sony apsc
            cameras.push_back( CropFactor( "NEX-5N", crop_APSC ) );             // sony apsc
            cameras.push_back( CropFactor( "NIKON D100", crop_NIKON_APSC ) );
            cameras.push_back( CropFactor( "NIKON D300", crop_NIKON_APSC ) );
            cameras.push_back( CropFactor( "NIKON D3100", crop_NIKON_APSC ) );
            cameras.push_back( CropFactor( "NIKON D3400", crop_NIKON_APSC ) );
            cameras.push_back( CropFactor( "NIKON D4", 1.0 ) );
            cameras.push_back( CropFactor( "NIKON D40", crop_NIKON_APSC ) );
            cameras.push_back( CropFactor( "NIKON D5100", crop_NIKON_APSC ) );
            cameras.push_back( CropFactor( "NIKON D5200", crop_NIKON_APSC ) );
            cameras.push_back( CropFactor( "NIKON D600", 1.0 ) );
            cameras.push_back( CropFactor( "NIKON D610", 1.0 ) );
            cameras.push_back( CropFactor( "NIKON D70", crop_NIKON_APSC ) );
            cameras.push_back( CropFactor( "NIKON D700", 1.0 ) );
            cameras.push_back( CropFactor( "NIKON D70s", crop_NIKON_APSC ) );
            cameras.push_back( CropFactor( "NIKON D80", crop_NIKON_APSC ) );
            cameras.push_back( CropFactor( "NIKON D800", 1.0 ) );
            cameras.push_back( CropFactor( "NIKON D810", 1.0 ) );
            cameras.push_back( CropFactor( "NIKON Z 6", 1.0 ) );
            cameras.push_back( CropFactor( "NIKON Z 7_2", 1.0 ) );
            cameras.push_back( CropFactor( "Nexus 7", 9.44 ) );                 // I saw it on the internet
            cameras.push_back( CropFactor( "Nikon SUPER COOLSCAN 5000 ED", 1.0 ) ); // these files don't have a focal length anyway
            cameras.push_back( CropFactor( "u1030SW,S1030SW", crop_Two33 ) );   // 2.33
            cameras.push_back( CropFactor( "OpticFilm 8100", 1.0 ) );           // film scanner
            cameras.push_back( CropFactor( "P 65+", crop_PhaseOne ) );          // real medium format
            cameras.push_back( CropFactor( "PENTAX K10D", crop_APSC ) );
            cameras.push_back( CropFactor( "PENTAX K-3 Mark III", crop_APSC ) );
            cameras.push_back( CropFactor( "PENTAX K-r", crop_APSC ) );
            cameras.push_back( CropFactor( "Panasonic DMC-GF2", crop_43 ) );
            cameras.push_back( CropFactor( "Perfection V30/V300", 1.0 ) );      // scanner
            cameras.push_back( CropFactor( "Perfection V39", 1.0 ) );           // scanner
            cameras.push_back( CropFactor( "PM23300", 7.68 ) );                 // this is an educated guess for this HTC phone
            cameras.push_back( CropFactor( "PowerShot S95", crop_OOOPS ) );     // libraw strikes again
            cameras.push_back( CropFactor( "QCAM-AA", 7.68 ) );                 // google pixel phone? random guess
            cameras.push_back( CropFactor( "QSS-32_33", 1.0 ) );                // it's a scanner
            cameras.push_back( CropFactor( "RICOH GR III", crop_APSC ) );
            cameras.push_back( CropFactor( "RICOH GR IIIx", crop_APSC ) );
            cameras.push_back( CropFactor( "RICOH THETA S", ( 7.3 / 1.3 ) ) );
            cameras.push_back( CropFactor( "RICOH THETA Z1", ( 7.3 / 1.3 ) ) );
            cameras.push_back( CropFactor( "SGH-I917", 7.68 ) );                // unknown, this is a guess
            cameras.push_back( CropFactor( "SGH-i937", 7.68 ) );                // unknown, this is a guess
            cameras.push_back( CropFactor( "SIGMA fp L", 1.0 ) );
            cameras.push_back( CropFactor( "SPH-L710", 7.68 ) );                // unknown, this is a guess
            cameras.push_back( CropFactor( "ScanJet 8200", 1.0 ) );             // scanner
            cameras.push_back( CropFactor( "SM-G930F", crop_Samsung_Tablet ) );
            cameras.push_back( CropFactor( "SM-G965U1", crop_Samsung_Tablet ) );
            cameras.push_back( CropFactor( "SM-G975F", crop_Samsung_Tablet ) );
            cameras.push_back( CropFactor( "SM-T700", crop_Samsung_Tablet ) );
            cameras.push_back( CropFactor( "SM-T713", crop_Samsung_Tablet ) );
            cameras.push_back( CropFactor( "Sinarback eVolution 75, Sinar p3 / f3", 1.0 ) ); // scanner
            cameras.push_back( CropFactor( "TS3100 series", 1.0 ) );            // it's a scanner
            cameras.push_back( CropFactor( "USB 2.0 Camera", 7.0 ) );                // no idea
            cameras.push_back( CropFactor( "VAIO Camera Capture Utility", 1.0 ) );   // screen capture
            cameras.push_back( CropFactor( "X-A7", crop_APSC ) );
            cameras.push_back( CropFactor( "X-E3", crop_APSC ) );
            cameras.push_back( CropFactor( "X-M1", crop_APSC ) );
            cameras.push_back( CropFactor( "X-Pro1", crop_APSC ) );
            cameras.push_back( CropFactor( "X-Pro2", crop_APSC ) );
            cameras.push_back( CropFactor( "X-S10", crop_APSC ) );
            cameras.push_back( CropFactor( "X-T2", crop_APSC ) );
            cameras.push_back( CropFactor( "X100F", crop_APSC ) );
            cameras.push_back( CropFactor( "X100S", crop_APSC ) );
            cameras.push_back( CropFactor( "X100T", crop_APSC ) );
            cameras.push_back( CropFactor( "X100V", crop_APSC ) );
            cameras.push_back( CropFactor( "X1D II 50C", crop_Fuji_Medium ) );
            cameras.push_back( CropFactor( "XP-420 Series", 1.0 ) );            // it's a scanner
            cameras.push_back( CropFactor( "XP-420", 1.0 ) );                   // it's a scanner
            cameras.push_back( CropFactor( "X-U (Typ 113)", crop_APSC ) );      // LibRaw does this to this Leica camera
            cameras.push_back( CropFactor( "XZ-1", 4.414995 ) );                // 1/1.63"  8.07 x 5.56
            cameras.push_back( CropFactor( "ZN5", 7.0 ) );                      // Motorola phone. value is a guess
            cameras.push_back( CropFactor( "iPAQ rx3000", 9.0 ) );              // random guess
            cameras.push_back( CropFactor( "iPad (6th generation)", crop_iPhone4 ) );
            cameras.push_back( CropFactor( "iPad mini (5th generation)", crop_iPhone4 ) );
            cameras.push_back( CropFactor( "iPad mini 2", crop_iPhone4 ) );
            cameras.push_back( CropFactor( "iPhone 11", 7.0 ) );
            cameras.push_back( CropFactor( "iPhone 11 Pro", 7.0 ) );
            cameras.push_back( CropFactor( "iPhone 12", 7.0 ) );
            cameras.push_back( CropFactor( "iPhone 12 Pro", 7.0 ) );
            cameras.push_back( CropFactor( "iPhone 12 Pro Max", 7.0 ) );
            cameras.push_back( CropFactor( "iPhone 3", crop_iPhone4 ) );
            cameras.push_back( CropFactor( "iPhone 3G", crop_iPhone4 ) );
            cameras.push_back( CropFactor( "iPhone 3GS", crop_iPhone4 ) );
            cameras.push_back( CropFactor( "iPhone 4", crop_iPhone4 ) );
            cameras.push_back( CropFactor( "iPhone 4S", crop_iPhone4 ) );
            cameras.push_back( CropFactor( "iPhone 5", crop_iPhone4 ) );
            cameras.push_back( CropFactor( "iPhone 5s", crop_iPhone4 ) );
            cameras.push_back( CropFactor( "iPhone 6", crop_iPhone4 ) );
            cameras.push_back( CropFactor( "iPhone 6s", crop_iPhone4 ) );
            cameras.push_back( CropFactor( "iPhone 7", crop_iPhone4 ) );
            cameras.push_back( CropFactor( "iPhone 8", crop_iPhone4 ) );
            cameras.push_back( CropFactor( "iPhone 8 Plus", crop_iPhone4 ) );
            cameras.push_back( CropFactor( "iPhone X", crop_iPhone4 ) );
            cameras.push_back( CropFactor( "iPhone XR", crop_iPhone4 ) );
            cameras.push_back( CropFactor( "iPhone XS", crop_iPhone4 ) );
            cameras.push_back( CropFactor( "iPhone XS Max", crop_iPhone4 ) );
            cameras.push_back( CropFactor( "iPhone", crop_iPhone4 ) );
            cameras.push_back( CropFactor( "iPod touch", crop_iPhone4 ) );

            qsort( cameras.data(), cameras.size(), sizeof( CropFactor ), CameraEntryCompare );

            printf( "            cameras.resize( %Iu );\n", cameras.size() );

            for ( int i = 0; i < cameras.size(); i++ )
                printf( "            cameras[%d].pcCamera = \"%s\"; cameras[%d].cropFactor = %lf;\n", i, cameras[i].pcCamera, i, cameras[i].cropFactor );

            for ( int i = 0; i < ( cameras.size() - 1 ); i++ )
            {
                int c = strcmp( cameras[i].pcCamera, cameras[i+1].pcCamera );
                if ( c >= 0 )
                    printf( "bad crop entries %s, %s\n", cameras[i].pcCamera, cameras[i+1].pcCamera );
                assert( c < 0 );
            }
        
#else // the part below is generated by the code above; don't edit manually

            cameras.resize( 281 );
            cameras[0].pcCamera = "ADR6410LVW"; cameras[0].cropFactor = 7.000000;
            cameras[1].pcCamera = "AE-1"; cameras[1].cropFactor = 1.000000;
            cameras[2].pcCamera = "C3000Z"; cameras[2].cropFactor = 7.000000;
            cameras[3].pcCamera = "C5060WZ"; cameras[3].cropFactor = 4.845086;
            cameras[4].pcCamera = "C6902"; cameras[4].cropFactor = 5.600000;
            cameras[5].pcCamera = "COOLPIX P7100"; cameras[5].cropFactor = 4.652324;
            cameras[6].pcCamera = "CYBERSHOT"; cameras[6].cropFactor = 7.000000;
            cameras[7].pcCamera = "CanoScan 8800F"; cameras[7].cropFactor = 1.000000;
            cameras[8].pcCamera = "Canon EOS 10D"; cameras[8].cropFactor = 1.621622;
            cameras[9].pcCamera = "Canon EOS 1300D"; cameras[9].cropFactor = 1.621622;
            cameras[10].pcCamera = "Canon EOS 20D"; cameras[10].cropFactor = 1.621622;
            cameras[11].pcCamera = "Canon EOS 30D"; cameras[11].cropFactor = 1.621622;
            cameras[12].pcCamera = "Canon EOS 40D"; cameras[12].cropFactor = 1.621622;
            cameras[13].pcCamera = "Canon EOS 50D"; cameras[13].cropFactor = 1.621622;
            cameras[14].pcCamera = "Canon EOS 5D"; cameras[14].cropFactor = 1.000000;
            cameras[15].pcCamera = "Canon EOS 5D Mark II"; cameras[15].cropFactor = 1.000000;
            cameras[16].pcCamera = "Canon EOS 5D Mark III"; cameras[16].cropFactor = 1.000000;
            cameras[17].pcCamera = "Canon EOS 5D Mark IV"; cameras[17].cropFactor = 1.000000;
            cameras[18].pcCamera = "Canon EOS 5DS"; cameras[18].cropFactor = 1.000000;
            cameras[19].pcCamera = "Canon EOS 5DS R"; cameras[19].cropFactor = 1.000000;
            cameras[20].pcCamera = "Canon EOS 60D"; cameras[20].cropFactor = 1.621622;
            cameras[21].pcCamera = "Canon EOS 6D"; cameras[21].cropFactor = 1.000000;
            cameras[22].pcCamera = "Canon EOS 6D Mark II"; cameras[22].cropFactor = 1.000000;
            cameras[23].pcCamera = "Canon EOS 700D"; cameras[23].cropFactor = 1.621622;
            cameras[24].pcCamera = "Canon EOS 70D"; cameras[24].cropFactor = 1.621622;
            cameras[25].pcCamera = "Canon EOS 77D"; cameras[25].cropFactor = 1.621622;
            cameras[26].pcCamera = "Canon EOS 7D"; cameras[26].cropFactor = 1.621622;
            cameras[27].pcCamera = "Canon EOS 7D Mark II"; cameras[27].cropFactor = 1.621622;
            cameras[28].pcCamera = "Canon EOS 80D"; cameras[28].cropFactor = 1.621622;
            cameras[29].pcCamera = "Canon EOS 90D"; cameras[29].cropFactor = 1.621622;
            cameras[30].pcCamera = "Canon EOS D30"; cameras[30].cropFactor = 1.621622;
            cameras[31].pcCamera = "Canon EOS D60"; cameras[31].cropFactor = 1.621622;
            cameras[32].pcCamera = "Canon EOS DIGITAL REBEL"; cameras[32].cropFactor = 1.621622;
            cameras[33].pcCamera = "Canon EOS DIGITAL REBEL XT"; cameras[33].cropFactor = 1.621622;
            cameras[34].pcCamera = "Canon EOS M"; cameras[34].cropFactor = 1.621622;
            cameras[35].pcCamera = "Canon EOS M100"; cameras[35].cropFactor = 1.621622;
            cameras[36].pcCamera = "Canon EOS M2"; cameras[36].cropFactor = 1.621622;
            cameras[37].pcCamera = "Canon EOS M3"; cameras[37].cropFactor = 1.621622;
            cameras[38].pcCamera = "Canon EOS M5"; cameras[38].cropFactor = 1.621622;
            cameras[39].pcCamera = "Canon EOS M50"; cameras[39].cropFactor = 1.621622;
            cameras[40].pcCamera = "Canon EOS M6"; cameras[40].cropFactor = 1.621622;
            cameras[41].pcCamera = "Canon EOS M6 Mark II"; cameras[41].cropFactor = 1.621622;
            cameras[42].pcCamera = "Canon EOS R5"; cameras[42].cropFactor = 1.000000;
            cameras[43].pcCamera = "Canon EOS REBEL T2i"; cameras[43].cropFactor = 1.621622;
            cameras[44].pcCamera = "Canon EOS REBEL T3"; cameras[44].cropFactor = 1.621622;
            cameras[45].pcCamera = "Canon EOS REBEL T3i"; cameras[45].cropFactor = 1.621622;
            cameras[46].pcCamera = "Canon EOS REBEL T4i"; cameras[46].cropFactor = 1.621622;
            cameras[47].pcCamera = "Canon EOS REBEL T5"; cameras[47].cropFactor = 1.621622;
            cameras[48].pcCamera = "Canon EOS REBEL T6"; cameras[48].cropFactor = 1.621622;
            cameras[49].pcCamera = "Canon EOS RP"; cameras[49].cropFactor = 1.000000;
            cameras[50].pcCamera = "Canon EOS Rebel T6"; cameras[50].cropFactor = 1.621622;
            cameras[51].pcCamera = "Canon EOS-1D"; cameras[51].cropFactor = 1.255028;
            cameras[52].pcCamera = "Canon EOS-1D Mark II"; cameras[52].cropFactor = 1.255028;
            cameras[53].pcCamera = "Canon EOS-1D Mark II N"; cameras[53].cropFactor = 1.255028;
            cameras[54].pcCamera = "Canon EOS-1D Mark III"; cameras[54].cropFactor = 1.255028;
            cameras[55].pcCamera = "Canon EOS-1D Mark IV"; cameras[55].cropFactor = 1.255028;
            cameras[56].pcCamera = "Canon EOS-1D X"; cameras[56].cropFactor = 1.000000;
            cameras[57].pcCamera = "Canon EOS-1DS"; cameras[57].cropFactor = 1.000000;
            cameras[58].pcCamera = "Canon EOS-1Ds Mark II"; cameras[58].cropFactor = 1.000000;
            cameras[59].pcCamera = "Canon EOS-1Ds Mark III"; cameras[59].cropFactor = 1.000000;
            cameras[60].pcCamera = "Canon EOS-1Ds Mark IV"; cameras[60].cropFactor = 1.000000;
            cameras[61].pcCamera = "Canon PowerShot A20"; cameras[61].cropFactor = 5.643778;
            cameras[62].pcCamera = "Canon PowerShot A430"; cameras[62].cropFactor = 7.211103;
            cameras[63].pcCamera = "Canon PowerShot A520"; cameras[63].cropFactor = 4.845086;
            cameras[64].pcCamera = "Canon PowerShot A60"; cameras[64].cropFactor = 6.516057;
            cameras[65].pcCamera = "Canon PowerShot A610"; cameras[65].cropFactor = 4.845086;
            cameras[66].pcCamera = "Canon PowerShot A620"; cameras[66].cropFactor = 4.845086;
            cameras[67].pcCamera = "Canon PowerShot A80"; cameras[67].cropFactor = 4.845086;
            cameras[68].pcCamera = "Canon PowerShot A85"; cameras[68].cropFactor = 6.516057;
            cameras[69].pcCamera = "Canon PowerShot G5"; cameras[69].cropFactor = 4.845086;
            cameras[70].pcCamera = "Canon PowerShot G5 X Mark II"; cameras[70].cropFactor = 2.727273;
            cameras[71].pcCamera = "Canon PowerShot G7 X"; cameras[71].cropFactor = 2.727273;
            cameras[72].pcCamera = "Canon PowerShot G7 X Mark II"; cameras[72].cropFactor = 2.727273;
            cameras[73].pcCamera = "Canon PowerShot G9"; cameras[73].cropFactor = 4.652324;
            cameras[74].pcCamera = "Canon PowerShot G9 X Mark II"; cameras[74].cropFactor = 2.727273;
            cameras[75].pcCamera = "Canon PowerShot S100"; cameras[75].cropFactor = 4.652324;
            cameras[76].pcCamera = "Canon PowerShot S2 IS"; cameras[76].cropFactor = 6.015934;
            cameras[77].pcCamera = "Canon PowerShot S200"; cameras[77].cropFactor = 4.652324;
            cameras[78].pcCamera = "Canon PowerShot S70"; cameras[78].cropFactor = 4.845086;
            cameras[79].pcCamera = "Canon PowerShot S95"; cameras[79].cropFactor = 4.652324;
            cameras[80].pcCamera = "Canon PowerShot SD10"; cameras[80].cropFactor = 6.025991;
            cameras[81].pcCamera = "Canon PowerShot SD1100 IS"; cameras[81].cropFactor = 6.015934;
            cameras[82].pcCamera = "Canon PowerShot SD450"; cameras[82].cropFactor = 6.015934;
            cameras[83].pcCamera = "Canon PowerShot SD550"; cameras[83].cropFactor = 4.845086;
            cameras[84].pcCamera = "Canon PowerShot SD780 IS"; cameras[84].cropFactor = 5.643778;
            cameras[85].pcCamera = "Canon PowerShot SD790 IS"; cameras[85].cropFactor = 5.643778;
            cameras[86].pcCamera = "Canon PowerShot SX110 IS"; cameras[86].cropFactor = 6.015934;
            cameras[87].pcCamera = "Canon PowerShot SX530 HS"; cameras[87].cropFactor = 5.643778;
            cameras[88].pcCamera = "Canon PowerShot SX600 HS"; cameras[88].cropFactor = 5.643778;
            cameras[89].pcCamera = "Canon VIXIA HF10"; cameras[89].cropFactor = 7.611984;
            cameras[90].pcCamera = "DC-S1R"; cameras[90].cropFactor = 1.000000;
            cameras[91].pcCamera = "DC-ZS200"; cameras[91].cropFactor = 2.727273;
            cameras[92].pcCamera = "DC210 Zoom (V03.10)"; cameras[92].cropFactor = 6.591501;
            cameras[93].pcCamera = "DCR-TRV30"; cameras[93].cropFactor = 10.816654;
            cameras[94].pcCamera = "DMC-CM1"; cameras[94].cropFactor = 2.727273;
            cameras[95].pcCamera = "DMC-FS3"; cameras[95].cropFactor = 6.025991;
            cameras[96].pcCamera = "DMC-FX8"; cameras[96].cropFactor = 6.025991;
            cameras[97].pcCamera = "DMC-FZ5"; cameras[97].cropFactor = 6.015934;
            cameras[98].pcCamera = "DMC-G3"; cameras[98].cropFactor = 1.999381;
            cameras[99].pcCamera = "DMC-GF1"; cameras[99].cropFactor = 1.999381;
            cameras[100].pcCamera = "DMC-GF2"; cameras[100].cropFactor = 1.999381;
            cameras[101].pcCamera = "DMC-GM1"; cameras[101].cropFactor = 1.999381;
            cameras[102].pcCamera = "DMC-GX7"; cameras[102].cropFactor = 1.999381;
            cameras[103].pcCamera = "DMC-LX100"; cameras[103].cropFactor = 1.999381;
            cameras[104].pcCamera = "DMC-TS2"; cameras[104].cropFactor = 5.692976;
            cameras[105].pcCamera = "DMC-TS3"; cameras[105].cropFactor = 5.692976;
            cameras[106].pcCamera = "DMC-ZS1"; cameras[106].cropFactor = 6.025991;
            cameras[107].pcCamera = "DMC-ZS100"; cameras[107].cropFactor = 2.727273;
            cameras[108].pcCamera = "DMC-ZS7"; cameras[108].cropFactor = 5.692976;
            cameras[109].pcCamera = "DSC-N1"; cameras[109].cropFactor = 4.845086;
            cameras[110].pcCamera = "DSC-P52"; cameras[110].cropFactor = 6.516057;
            cameras[111].pcCamera = "DSC-RX1"; cameras[111].cropFactor = 1.000000;
            cameras[112].pcCamera = "DSC-RX100"; cameras[112].cropFactor = 2.727273;
            cameras[113].pcCamera = "DSC-W120"; cameras[113].cropFactor = 6.015934;
            cameras[114].pcCamera = "DSC-W560"; cameras[114].cropFactor = 5.600000;
            cameras[115].pcCamera = "DSC-W80"; cameras[115].cropFactor = 6.025991;
            cameras[116].pcCamera = "DSC-W800"; cameras[116].cropFactor = 5.643778;
            cameras[117].pcCamera = "DSC-WX1"; cameras[117].cropFactor = 5.623000;
            cameras[118].pcCamera = "DSC-WX300"; cameras[118].cropFactor = 5.643778;
            cameras[119].pcCamera = "DV 5700"; cameras[119].cropFactor = 7.000000;
            cameras[120].pcCamera = "DiMAGE 7"; cameras[120].cropFactor = 3.900000;
            cameras[121].pcCamera = "Digital Link"; cameras[121].cropFactor = 1.000000;
            cameras[122].pcCamera = "E-M10MarkII"; cameras[122].cropFactor = 1.999381;
            cameras[123].pcCamera = "E-PM2"; cameras[123].cropFactor = 1.999381;
            cameras[124].pcCamera = "E3100"; cameras[124].cropFactor = 6.516057;
            cameras[125].pcCamera = "E5200"; cameras[125].cropFactor = 4.845086;
            cameras[126].pcCamera = "E5400"; cameras[126].cropFactor = 4.845086;
            cameras[127].pcCamera = "E6653"; cameras[127].cropFactor = 5.600000;
            cameras[128].pcCamera = "E990"; cameras[128].cropFactor = 4.845086;
            cameras[129].pcCamera = "EOS 5D Mark II"; cameras[129].cropFactor = 1.000000;
            cameras[130].pcCamera = "EX1"; cameras[130].cropFactor = 5.748494;
            cameras[131].pcCamera = "Electro 35 GSN"; cameras[131].cropFactor = 1.000000;
            cameras[132].pcCamera = "Epson Stylus NX420"; cameras[132].cropFactor = 1.000000;
            cameras[133].pcCamera = "FinePix F900EXR"; cameras[133].cropFactor = 5.326000;
            cameras[134].pcCamera = "FinePix S3Pro"; cameras[134].cropFactor = 1.529400;
            cameras[135].pcCamera = "FinePix4900ZOOM"; cameras[135].cropFactor = 4.652324;
            cameras[136].pcCamera = "FinePixS2Pro"; cameras[136].cropFactor = 1.529400;
            cameras[137].pcCamera = "FrontRow Wear"; cameras[137].cropFactor = 5.080000;
            cameras[138].pcCamera = "G8141"; cameras[138].cropFactor = 5.643778;
            cameras[139].pcCamera = "GFX 100"; cameras[139].cropFactor = 0.790048;
            cameras[140].pcCamera = "GFX 50R"; cameras[140].cropFactor = 0.790048;
            cameras[141].pcCamera = "GFX 50S"; cameras[141].cropFactor = 0.790048;
            cameras[142].pcCamera = "GFX100S"; cameras[142].cropFactor = 0.790048;
            cameras[143].pcCamera = "GR II"; cameras[143].cropFactor = 1.529400;
            cameras[144].pcCamera = "H1A1000"; cameras[144].cropFactor = 7.000000;
            cameras[145].pcCamera = "H8166"; cameras[145].cropFactor = 5.692976;
            cameras[146].pcCamera = "HD7"; cameras[146].cropFactor = 5.692976;
            cameras[147].pcCamera = "HDR-SR1"; cameras[147].cropFactor = 7.211103;
            cameras[148].pcCamera = "HERO6 Black"; cameras[148].cropFactor = 5.643778;
            cameras[149].pcCamera = "HP PhotoSmart C945 (V01.60)"; cameras[149].cropFactor = 1.000000;
            cameras[150].pcCamera = "HP Scanjet 4800"; cameras[150].cropFactor = 1.000000;
            cameras[151].pcCamera = "HTC Touch Diamond P370"; cameras[151].cropFactor = 10.000000;
            cameras[152].pcCamera = "HTC Touch Diamond P3700"; cameras[152].cropFactor = 10.000000;
            cameras[153].pcCamera = "HTC-8900"; cameras[153].cropFactor = 7.680000;
            cameras[154].pcCamera = "Hewlett-Packard PSC 750 Scanner"; cameras[154].cropFactor = 1.000000;
            cameras[155].pcCamera = "ILCE-7"; cameras[155].cropFactor = 1.000000;
            cameras[156].pcCamera = "ILCE-7M3"; cameras[156].cropFactor = 1.000000;
            cameras[157].pcCamera = "ILCE-7RM2"; cameras[157].cropFactor = 1.000000;
            cameras[158].pcCamera = "ILCE-7S"; cameras[158].cropFactor = 1.000000;
            cameras[159].pcCamera = "KODAK DC240 ZOOM DIGITAL CAMERA"; cameras[159].cropFactor = 6.591501;
            cameras[160].pcCamera = "KODAK EASYSHARE V1003 ZOOM DIGITAL CAMERA"; cameras[160].cropFactor = 4.845086;
            cameras[161].pcCamera = "KODAK V530 ZOOM DIGITAL CAMERA"; cameras[161].cropFactor = 4.241825;
            cameras[162].pcCamera = "Kodak CLAS Digital Film Scanner / HR200"; cameras[162].cropFactor = 1.000000;
            cameras[163].pcCamera = "L16"; cameras[163].cropFactor = 1.000000;
            cameras[164].pcCamera = "LEICA M MONOCHROM (Typ 246)"; cameras[164].cropFactor = 1.000000;
            cameras[165].pcCamera = "LEICA M10"; cameras[165].cropFactor = 1.000000;
            cameras[166].pcCamera = "LEICA Q (Typ 116)"; cameras[166].cropFactor = 1.000000;
            cameras[167].pcCamera = "LEICA Q2"; cameras[167].cropFactor = 1.000000;
            cameras[168].pcCamera = "LEICA Q2 MONO"; cameras[168].cropFactor = 1.000000;
            cameras[169].pcCamera = "LEICA SL (Typ 601)"; cameras[169].cropFactor = 1.000000;
            cameras[170].pcCamera = "LEICA SL2-S"; cameras[170].cropFactor = 1.000000;
            cameras[171].pcCamera = "LEICA X-U (Typ 113)"; cameras[171].cropFactor = 1.529400;
            cameras[172].pcCamera = "LS-5000"; cameras[172].cropFactor = 1.000000;
            cameras[173].pcCamera = "LS-9000"; cameras[173].cropFactor = 1.000000;
            cameras[174].pcCamera = "Lumia 1020"; cameras[174].cropFactor = 4.113183;
            cameras[175].pcCamera = "Lumia 520"; cameras[175].cropFactor = 7.680000;
            cameras[176].pcCamera = "Lumia 830"; cameras[176].cropFactor = 7.680000;
            cameras[177].pcCamera = "Lumia 920"; cameras[177].cropFactor = 7.680000;
            cameras[178].pcCamera = "Lumia 950"; cameras[178].cropFactor = 5.623000;
            cameras[179].pcCamera = "Lumia 950 XL"; cameras[179].cropFactor = 5.623000;
            cameras[180].pcCamera = "MHS-PM1"; cameras[180].cropFactor = 6.015934;
            cameras[181].pcCamera = "MX880 series"; cameras[181].cropFactor = 1.000000;
            cameras[182].pcCamera = "NEX-3N"; cameras[182].cropFactor = 1.529400;
            cameras[183].pcCamera = "NEX-5N"; cameras[183].cropFactor = 1.529400;
            cameras[184].pcCamera = "NIKON D100"; cameras[184].cropFactor = 1.527854;
            cameras[185].pcCamera = "NIKON D300"; cameras[185].cropFactor = 1.527854;
            cameras[186].pcCamera = "NIKON D3100"; cameras[186].cropFactor = 1.527854;
            cameras[187].pcCamera = "NIKON D3400"; cameras[187].cropFactor = 1.527854;
            cameras[188].pcCamera = "NIKON D4"; cameras[188].cropFactor = 1.000000;
            cameras[189].pcCamera = "NIKON D40"; cameras[189].cropFactor = 1.527854;
            cameras[190].pcCamera = "NIKON D5100"; cameras[190].cropFactor = 1.527854;
            cameras[191].pcCamera = "NIKON D5200"; cameras[191].cropFactor = 1.527854;
            cameras[192].pcCamera = "NIKON D600"; cameras[192].cropFactor = 1.000000;
            cameras[193].pcCamera = "NIKON D610"; cameras[193].cropFactor = 1.000000;
            cameras[194].pcCamera = "NIKON D70"; cameras[194].cropFactor = 1.527854;
            cameras[195].pcCamera = "NIKON D700"; cameras[195].cropFactor = 1.000000;
            cameras[196].pcCamera = "NIKON D70s"; cameras[196].cropFactor = 1.527854;
            cameras[197].pcCamera = "NIKON D80"; cameras[197].cropFactor = 1.527854;
            cameras[198].pcCamera = "NIKON D800"; cameras[198].cropFactor = 1.000000;
            cameras[199].pcCamera = "NIKON D810"; cameras[199].cropFactor = 1.000000;
            cameras[200].pcCamera = "NIKON Z 6"; cameras[200].cropFactor = 1.000000;
            cameras[201].pcCamera = "NIKON Z 7_2"; cameras[201].cropFactor = 1.000000;
            cameras[202].pcCamera = "Nexus 7"; cameras[202].cropFactor = 9.440000;
            cameras[203].pcCamera = "Nexus 9"; cameras[203].cropFactor = 10.816654;
            cameras[204].pcCamera = "Nikon SUPER COOLSCAN 5000 ED"; cameras[204].cropFactor = 1.000000;
            cameras[205].pcCamera = "OpticFilm 8100"; cameras[205].cropFactor = 1.000000;
            cameras[206].pcCamera = "P 65+"; cameras[206].cropFactor = 0.642319;
            cameras[207].pcCamera = "PENTAX K-3 Mark III"; cameras[207].cropFactor = 1.529400;
            cameras[208].pcCamera = "PENTAX K-r"; cameras[208].cropFactor = 1.529400;
            cameras[209].pcCamera = "PENTAX K10D"; cameras[209].cropFactor = 1.529400;
            cameras[210].pcCamera = "PM23300"; cameras[210].cropFactor = 7.680000;
            cameras[211].pcCamera = "Panasonic DMC-GF2"; cameras[211].cropFactor = 1.999381;
            cameras[212].pcCamera = "Perfection V30/V300"; cameras[212].cropFactor = 1.000000;
            cameras[213].pcCamera = "Perfection V39"; cameras[213].cropFactor = 1.000000;
            cameras[214].pcCamera = "PowerShot S95"; cameras[214].cropFactor = 4.652324;
            cameras[215].pcCamera = "QCAM-AA"; cameras[215].cropFactor = 7.680000;
            cameras[216].pcCamera = "QSS-32_33"; cameras[216].cropFactor = 1.000000;
            cameras[217].pcCamera = "RICOH GR III"; cameras[217].cropFactor = 1.529400;
            cameras[218].pcCamera = "RICOH GR IIIx"; cameras[218].cropFactor = 1.529400;
            cameras[219].pcCamera = "RICOH THETA S"; cameras[219].cropFactor = 5.615385;
            cameras[220].pcCamera = "RICOH THETA Z1"; cameras[220].cropFactor = 5.615385;
            cameras[221].pcCamera = "SGH-I917"; cameras[221].cropFactor = 7.680000;
            cameras[222].pcCamera = "SGH-i937"; cameras[222].cropFactor = 7.680000;
            cameras[223].pcCamera = "SIGMA fp L"; cameras[223].cropFactor = 1.000000;
            cameras[224].pcCamera = "SM-G930F"; cameras[224].cropFactor = 9.614803;
            cameras[225].pcCamera = "SM-G965U1"; cameras[225].cropFactor = 9.614803;
            cameras[226].pcCamera = "SM-G975F"; cameras[226].cropFactor = 9.614803;
            cameras[227].pcCamera = "SM-T700"; cameras[227].cropFactor = 9.614803;
            cameras[228].pcCamera = "SM-T713"; cameras[228].cropFactor = 9.614803;
            cameras[229].pcCamera = "SPH-L710"; cameras[229].cropFactor = 7.680000;
            cameras[230].pcCamera = "ScanJet 8200"; cameras[230].cropFactor = 1.000000;
            cameras[231].pcCamera = "Sinarback eVolution 75, Sinar p3 / f3"; cameras[231].cropFactor = 1.000000;
            cameras[232].pcCamera = "TS3100 series"; cameras[232].cropFactor = 1.000000;
            cameras[233].pcCamera = "USB 2.0 Camera"; cameras[233].cropFactor = 7.000000;
            cameras[234].pcCamera = "VAIO Camera Capture Utility"; cameras[234].cropFactor = 1.000000;
            cameras[235].pcCamera = "X-A7"; cameras[235].cropFactor = 1.529400;
            cameras[236].pcCamera = "X-E3"; cameras[236].cropFactor = 1.529400;
            cameras[237].pcCamera = "X-M1"; cameras[237].cropFactor = 1.529400;
            cameras[238].pcCamera = "X-Pro1"; cameras[238].cropFactor = 1.529400;
            cameras[239].pcCamera = "X-Pro2"; cameras[239].cropFactor = 1.529400;
            cameras[240].pcCamera = "X-S10"; cameras[240].cropFactor = 1.529400;
            cameras[241].pcCamera = "X-T2"; cameras[241].cropFactor = 1.529400;
            cameras[242].pcCamera = "X-U (Typ 113)"; cameras[242].cropFactor = 1.529400;
            cameras[243].pcCamera = "X100F"; cameras[243].cropFactor = 1.529400;
            cameras[244].pcCamera = "X100S"; cameras[244].cropFactor = 1.529400;
            cameras[245].pcCamera = "X100T"; cameras[245].cropFactor = 1.529400;
            cameras[246].pcCamera = "X100V"; cameras[246].cropFactor = 1.529400;
            cameras[247].pcCamera = "X1D II 50C"; cameras[247].cropFactor = 0.790048;
            cameras[248].pcCamera = "XP-420"; cameras[248].cropFactor = 1.000000;
            cameras[249].pcCamera = "XP-420 Series"; cameras[249].cropFactor = 1.000000;
            cameras[250].pcCamera = "XZ-1"; cameras[250].cropFactor = 4.414995;
            cameras[251].pcCamera = "ZN5"; cameras[251].cropFactor = 7.000000;
            cameras[252].pcCamera = "iPAQ rx3000"; cameras[252].cropFactor = 9.000000;
            cameras[253].pcCamera = "iPad (6th generation)"; cameras[253].cropFactor = 7.611984;
            cameras[254].pcCamera = "iPad mini (5th generation)"; cameras[254].cropFactor = 7.611984;
            cameras[255].pcCamera = "iPad mini 2"; cameras[255].cropFactor = 7.611984;
            cameras[256].pcCamera = "iPhone"; cameras[256].cropFactor = 7.611984;
            cameras[257].pcCamera = "iPhone 11"; cameras[257].cropFactor = 7.000000;
            cameras[258].pcCamera = "iPhone 11 Pro"; cameras[258].cropFactor = 7.000000;
            cameras[259].pcCamera = "iPhone 12"; cameras[259].cropFactor = 7.000000;
            cameras[260].pcCamera = "iPhone 12 Pro"; cameras[260].cropFactor = 7.000000;
            cameras[261].pcCamera = "iPhone 12 Pro Max"; cameras[261].cropFactor = 7.000000;
            cameras[262].pcCamera = "iPhone 3"; cameras[262].cropFactor = 7.611984;
            cameras[263].pcCamera = "iPhone 3G"; cameras[263].cropFactor = 7.611984;
            cameras[264].pcCamera = "iPhone 3GS"; cameras[264].cropFactor = 7.611984;
            cameras[265].pcCamera = "iPhone 4"; cameras[265].cropFactor = 7.611984;
            cameras[266].pcCamera = "iPhone 4S"; cameras[266].cropFactor = 7.611984;
            cameras[267].pcCamera = "iPhone 5"; cameras[267].cropFactor = 7.611984;
            cameras[268].pcCamera = "iPhone 5s"; cameras[268].cropFactor = 7.611984;
            cameras[269].pcCamera = "iPhone 6"; cameras[269].cropFactor = 7.611984;
            cameras[270].pcCamera = "iPhone 6s"; cameras[270].cropFactor = 7.611984;
            cameras[271].pcCamera = "iPhone 7"; cameras[271].cropFactor = 7.611984;
            cameras[272].pcCamera = "iPhone 8"; cameras[272].cropFactor = 7.611984;
            cameras[273].pcCamera = "iPhone 8 Plus"; cameras[273].cropFactor = 7.611984;
            cameras[274].pcCamera = "iPhone X"; cameras[274].cropFactor = 7.611984;
            cameras[275].pcCamera = "iPhone XR"; cameras[275].cropFactor = 7.611984;
            cameras[276].pcCamera = "iPhone XS"; cameras[276].cropFactor = 7.611984;
            cameras[277].pcCamera = "iPhone XS Max"; cameras[277].cropFactor = 7.611984;
            cameras[278].pcCamera = "iPod touch"; cameras[278].cropFactor = 7.611984;
            cameras[279].pcCamera = "id313"; cameras[279].cropFactor = 5.692976;
            cameras[280].pcCamera = "u1030SW,S1030SW"; cameras[280].cropFactor = 5.692976;

#endif

            //tracer.Trace( "initialized CropFactor object\n" );
        } //CCropFactor
    
        double GetCropFactor( char * pcCameraModel )
        {
            CropFactor search = { pcCameraModel, 0.0 };
            double result = DBL_MAX;
        
            CropFactor * pFactor = (CropFactor *) bsearch( &search, cameras.data(), cameras.size(), sizeof( CropFactor ), CameraEntryCompare );
        
            if ( 0 != pFactor )
                result = pFactor->cropFactor;
            else
            {
                if ( 0 != *pcCameraModel )
                {
                    char acAddCanon[ 100 ];
                    size_t l = strlen( pcCameraModel );
                    if ( ( l - 7 ) < _countof( acAddCanon ) )
                    {
                        // LibRaw strips "Canon " from the start of Canon camera models

                        strcpy( acAddCanon, "Canon " );
                        strcpy( acAddCanon + 6, pcCameraModel );
                        CropFactor search2 = { acAddCanon, 0.0 };

                        pFactor = (CropFactor *) bsearch( &search2, cameras.data(), cameras.size(), sizeof( CropFactor ), CameraEntryCompare );
        
                        if ( 0 != pFactor )
                            result = pFactor->cropFactor;
                    }
                }
            }

            //tracer.Trace( "crop factor lookup for %s: %lf\n", pcCameraModel, result );

            if ( DBL_MAX == result )
                tracer.Trace( "crop factor can't find camera model '%s'\n", pcCameraModel );

            return result;
        } //GetCropFactor
}; //CCropFactor

