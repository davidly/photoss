#pragma once

#include <winbase.h>
#include <chrono>
#include <winnt.h>

using namespace std;
using namespace std::chrono;

class CTimed
{
    private:
        long long & sum;
        high_resolution_clock::time_point tStart;
        bool active;

    public:
        static const long long NanoPerMilli() { return 1000000; }

        CTimed( long long & s ) : sum( s ), active( true )
        {
            tStart = high_resolution_clock::now();
        }

        long long Complete()
        {
            long long duration = 0;

            if ( active )
            {
                active = false;

                high_resolution_clock::time_point tEnd = high_resolution_clock::now();
                duration = duration_cast<std::chrono::nanoseconds>( tEnd - tStart ).count();

#if defined( _M_IX86 ) || defined( _M_X64 )
                _InlineInterlockedAdd64( &sum, duration );
#else
                _InterlockedAdd64( &sum, duration );
#endif
            }

            return duration;
        }

        ~CTimed()
        {
            Complete();
        }
};


