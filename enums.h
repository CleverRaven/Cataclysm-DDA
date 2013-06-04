#ifndef _ENUMS_H_
#define _ENUMS_H_

#ifndef sgn
#define sgn(x) (((x) < 0) ? -1 : 1)
#endif

enum phase_id {
PNULL, SOLID, LIQUID, GAS, PLASMA
};

enum damage_type
{
    DNULL = 0,
    BASH,
    CUT,
    ACID,
    ELECTRICITY,
    FIRE,
    NUM_DAM_TYPES
};

struct point {
 int x;
 int y;
 point(int X = 0, int Y = 0) : x (X), y (Y) {}
 point(const point &p) : x (p.x), y (p.y) {}
 ~point(){}
};

struct tripoint {
 int x;
 int y;
 int z;
 tripoint(int X = 0, int Y = 0, int Z = 0) : x (X), y (Y), z (Z) {}
 tripoint(const tripoint &p) : x (p.x), y (p.y), z (p.z) {}
 ~tripoint(){}
};

#endif
