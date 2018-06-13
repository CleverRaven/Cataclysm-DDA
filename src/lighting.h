#pragma once
#ifndef LIGHTING_H
#define LIGHTING_H

#include <vector>

class Lighting
{
    public:
        typedef std::vector< std::vector<int> > vvInt;
        typedef std::vector< std::vector<double> > vvDouble;

        Lighting( const int w, const int h );

        void recalculateLighting( const vvInt &objects, const double startIntensity );
        double getLight( const int x, const int y ) const;

        enum en_types {
            floor = 0,
            wall,
            lightsource
        };

    private:
        class T
        {
            public:
                double b, i, l;
                bool d;

                T() {};

                T( double b, double i ) {
                    this->b = b;
                    this->l = 1;
                    this->i = i;
                    this->d = false;
                }

                T &operator=( T *other ) {
                    this->b = other->b;
                    this->l = other->l;
                    this->i = other->i;
                    this->d = other->d;

                    return *this;
                }
        };

        int w, h;
        vvDouble light;
        std::vector<T> tmp, tmp2, tmpNew;

        int tmp2IntN;
        int tmpNewIntN;

        void applyLight( const T &t, const int x, const int y, const double td );
        void cutInterval( T &t, const int x );
        int merge();
        int merge1( T &t, int n );
        static double max( const double a, const double b );
        static double min( const double a, const double b );
};

/*
*  ix   iy
*  0     0    transpose
*  0     1    ccw
*  1     0    cw
*  1     1    transpose other axis
* */
template<typename T>
void rotate90( T &dst, const T &src, bool ix, bool iy )
{
    //assuming square
    int w = src.size();
    for( int x = 0; x < w; x++ ) {
        for( int y = 0; y < w; y++ ) {
            dst[iy ? w - 1 - y : y][ix ? w - 1 - x : x] = src[x][y];
        }
    }
}

template<typename T>
void rotate180( T &dst, const T &src )
{
    //assuming square
    int w = src.size();
    for( int x = 0; x < w; x++ ) {
        for( int y = 0; y < w; y++ ) {
            dst[w - 1 - x][w - 1 - y] = src[x][y];
        }
    }
}

/* angle:
    0 - copy as is
    1 - 90º cw
    2 - 180º
    3 - 90º ccw
* */
template<typename T>
void rotate( T &dst, const T &src, int angle )
{
    switch( angle ) {
        case 0: {
            dst = src;
        }
        case 1:
            rotate90( dst, src, true, false );
        case 2:
            rotate180( dst, src );
        case 3:
            rotate90( dst, src, false, true );
    }
}

#endif
