#pragma once

#include <winbase.h>
#include <chrono>

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
                _InlineInterlockedAdd64( &sum, duration );
            }

            return duration;
        }

        ~CTimed()
        {
            Complete();
        }
};


