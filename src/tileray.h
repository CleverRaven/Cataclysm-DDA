#pragma once
#ifndef TILERAY_H
#define TILERAY_H

#include "point.h"

// Class for calculating tile coordinates
// of a point that moves along the ray with given
// direction (dir) or delta tile coordinates (dx, dy).
// Advance method will move to the next tile
// along ray
// Direction is angle in degrees from positive x-axis to positive y-axis:
//
//       | 270         orthogonal dir left (-)
// 180   |     0         ^
//   ----+----> X    -------> X (forward dir, 0 degrees)
//       |               v
//       V 90          orthogonal dir right (+)
//       Y
//
// note to future developers: tilerays can't be cached at the tileray level,
// because tileray values depend on leftover, and thus tileray.advance(1)
// changes depending on previous calls to advance.

class tileray
{
    private:
        point delta;    // ray delta
        int leftover;   // counter to shift coordinates
        point abs_d;    // absolute value of delta
        int direction;  // ray direction
        point last_d;   // delta of last advance
        int steps;      // how many steps we advanced so far
        bool infinite;  // ray is infinite (end will always return true)
    public:
        tileray();
        tileray( int adx, int ady );
        tileray( int adir );

        void init( int adx, int ady );  // init ray with dx,dy
        void init( int adir );          // init ray with direction

        int dx() const;       // return dx of last advance (-1 to 1)
        int dy() const;       // return dy of last advance (-1 to 1)
        int dir() const;      // return direction of ray (degrees)
        int dir4() const;     // return 4-sided direction (0 = east, 1 = south, 2 = west, 3 = north)
        int dir8() const;     // return 8-sided direction (0 = east, 1 = southeast, 2 = south ...)
        // convert certain symbols from north-facing variant into current dir facing
        int dir_symbol( int sym ) const;
        // return dx for point at "od" distance in orthogonal direction
        int ortho_dx( int od ) const;
        // return dy for point at "od" distance in orthogonal direction
        int ortho_dy( int od ) const;
        bool mostly_vertical() const;  // return if ray is mostly vertical

        void advance( int num = 1 ); // move to the next tile (calculate last dx, dy)
        void clear_advance(); // clear steps, leftover, last_dx, and last_dy
        bool end();      // do we reach the end of (dx,dy) defined ray?
};

#endif
