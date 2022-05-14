#pragma once

#include <random>

class CStringArray
{
    private:
        vector<WCHAR *> elements;
        std::mutex mtx;

        static int PathCompare( const void * a, const void * b )
        {
            WCHAR *pa = *(WCHAR **) a;
            WCHAR *pb = *(WCHAR **) b;

            return ( wcscmp( pa, pb ) );
        } //PathCompare

    public:
        CStringArray()
        {
        }

        ~CStringArray()
        {
            Clear();
        }

        size_t Count() { return elements.size(); }
        WCHAR ** Array() { return elements.data(); }
        WCHAR * Get( size_t i ) { return elements[ i ]; }

        void Sort()
        {
            qsort( elements.data(), elements.size(), sizeof( WCHAR *), PathCompare );
        } //Sort

        PWCHAR & operator[] ( size_t i ) { return elements[ i ]; }

        void Clear()
        {
            for ( size_t i = 0; i < elements.size(); i++ )
            {
                delete elements[ i ];
                elements[ i ] = NULL;
            }

            elements.resize( 0 );
        }

        void Randomize()
        {
            if ( elements.size() <= 1 )
                return;

            std::random_device rd;
            std::mt19937 gen( rd() );
            std::uniform_int_distribution<> distrib( 0, (int) elements.size() - 1 );

            for ( size_t i = 0; i < elements.size() * 2; i++ )
            {
                int a = distrib( gen );
                int b = distrib( gen );

                swap( elements[ a ], elements[ b ] );
            }
        } //Randomize

        void Add( WCHAR * pwc )
        {
            size_t len = 1 + wcslen( pwc );
            WCHAR * p = new WCHAR[ len ];
            wcscpy_s( p, len, pwc );

            lock_guard<mutex> lock( mtx );

            elements.push_back( p );
        }
}; //CStringArray


