#ifndef _TILERAY_H_
#define _TILERAY_H_

// Class for calculating tile coords
// of a point that moves along the ray with given
// direction (dir) or delta tile coords (dx, dy).
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
class tileray
{
private:
    int deltax;     // ray delta x
    int deltay;     // ray delta y
    int leftover;   // counter to shift coords
    int direction;  // ray direction
    int last_dx;    // dx of last advance
    int last_dy;    // dy of last advance
    int steps;      // how many steps we advanced so far
    bool infinite;  // ray is infinite (end will always return true)
public:
    tileray ();
    tileray (int adx, int ady);
    tileray (int adir);

    void init (int adx, int ady);   // init ray with dx,dy
    void init (int adir);           // init ray with direction

    int dx ();      // return dx of last advance (-1 to 1)
    int dy ();      // return dy of last advance (-1 to 1)
    int dir ();     // return direction of ray (degrees)
    int dir4 ();    // return 4-sided direction (0 = east, 1 = south, 2 = west, 3 = north)
    char dir_symbol (char sym); // convert certain symbols from north-facing variant into current dir facing
    int ortho_dx (int od); // return dx for point at "od" distance in orthogonal direction
    int ortho_dy (int od); // return dy for point at "od" distance in orthogonal direction
    bool mostly_vertical (); // return if ray is mostly vertical

    void advance (int num = 1); // move to the next tile (calculate last dx, dy)
    bool end ();     // do we reach the end of (dx,dy) defined ray?
};

#endif
