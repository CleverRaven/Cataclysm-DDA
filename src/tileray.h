#pragma once
#ifndef CATA_SRC_TILERAY_H
#define CATA_SRC_TILERAY_H

#include <string>

#include "coordinates.h"
#include "units.h"

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
//
// This class is used with bubble and relative coordinates. Fundamentally the
// difference is whether the point it's relative to is the reality map corner
// or something else, so both usages are supported.
class tileray
{
    private:
        point_rel_ms delta;            // ray delta
        int leftover = 0;       // counter to shift coordinates
        point_rel_ms abs_d;            // absolute value of delta
        units::angle direction = 0_degrees; // ray direction
        point_rel_ms last_d;           // delta of last advance
        int steps = 0;          // how many steps we advanced so far
        bool infinite = false;  // ray is infinite (end will always return true)
    public:
        tileray();
        explicit tileray( const point_rel_ms &ad );
        explicit tileray( const point_bub_ms &ad );
        explicit tileray( units::angle adir );

        void init( const point_rel_ms &ad );   // init ray with ad
        void init( const point_bub_ms &ad ); // init ray with ad
        void init( const units::angle &adir ); // init ray with direction

        int dx() const;       // return dx of last advance (-1 to 1)
        int dy() const;       // return dy of last advance (-1 to 1)
        units::angle dir() const;      // return direction of ray
        int quadrant() const;

        /** convert to a string representation of the azimuth from north, in integer degrees */
        std::string to_string_azimuth_from_north() const;

        // return dx for point at "od" distance in orthogonal direction
        int ortho_dx( int od ) const;
        // return dy for point at "od" distance in orthogonal direction
        int ortho_dy( int od ) const;
        bool mostly_vertical() const;  // return if ray is mostly vertical

        void advance( int num = 1 ); // move to the next tile (calculate last dx, dy)
        void clear_advance(); // clear steps, leftover, last_dx, and last_dy
        int get_steps() const;  // how many steps we advanced
};

#endif // CATA_SRC_TILERAY_H
