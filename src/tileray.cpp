#include "tileray.h"
#include "game_constants.h"
#include <math.h>
#include <stdlib.h>

static const int sx[4] = { 1, -1, -1, 1 };
static const int sy[4] = { 1, 1, -1, -1 };

tileray::tileray (): deltax(0), deltay(0),  leftover(0), direction(0),
    last_dx(0), last_dy(0), steps(0), infinite (false)
{
}

tileray::tileray (int adx, int ady)
{
    init (adx, ady);
}

tileray::tileray (int adir): direction (adir)
{
    init (adir);
}

void tileray::init (int adx, int ady)
{
    deltax = adx;
    deltay = ady;
    leftover = 0;
    if (!adx && !ady) {
        direction = 0;
    } else {
        direction = (int) (atan2 ((double)deltay, (double)deltax) * 180.0 / M_PI);
        if (direction < 0) {
            direction += 360;
        }
    }
    last_dx = 0;
    last_dy = 0;
    steps = 0;
    infinite = false;
}

void tileray::init (int adir)
{
    deltax = 0;
    deltay = 0;
    leftover = 0;
    // Clamp adir to the range [0, 359]
    direction = (adir < 0 ? 360 - ((-adir) % 360) : adir % 360);
    last_dx = 0;
    last_dy = 0;
    deltax = abs((int) (cos ((float) direction * M_PI / 180.0) * 100));
    deltay = abs((int) (sin ((float) direction * M_PI / 180.0) * 100));
    steps = 0;
    infinite = true;
}

int tileray::dx () const
{
    return last_dx;
}

int tileray::dy () const
{
    return last_dy;
}

int tileray::dir () const
{
    return direction;
}

int tileray::dir4 () const
{
    if (direction >= 45 && direction <= 135) {
        return 1;
    } else if (direction > 135 && direction < 225) {
        return 2;
    } else if (direction >= 225 && direction <= 315) {
        return 3;
    } else {
        return 0;
    }
}

int tileray::dir8 () const
{
    int oct = 0;
    int dir = direction;
    if (dir < 23 || dir > 337) {
        return 0;
    }
    while (dir > 22) {
        dir -= 45;
        oct += 1;
    }
    return oct;
}

long tileray::dir_symbol (long sym)
{
    switch (sym) {
    // output.cpp special_symbol() converts yubn to corners, hj to lines, c to cross
    case 'j':
        return (long[]){'h','\\','j','/','h','\\','j','/'}[dir8()];
    case 'h':
        return (long[]){'j','/','h','\\','j','/','h','\\'}[dir8()];
    case 'y':
        return (long[]){'u','>','n','v','b','<','y','^'}[dir8()];
    case 'u':
        return (long[]){'n','v','b','<','y','^','u','>'}[dir8()];
    case 'n':
        return (long[]){'b','<','y','^','u','>','n','v'}[dir8()];
    case 'b':
        return (long[]){'y','^','u','>','n','v','b','<'}[dir8()];
    case '^':
        return (long[]){'>','n','v','b','<','y','^','u'}[dir8()];
    // [ not rotated to ] because they might represent different items
    case '[':
        return "-\\[/-\\[/"[dir8()];
    case ']':
        return "-\\]/-\\]/"[dir8()];
    case '|':
        return "-\\|/-\\|/"[dir8()];
    case '-':
        return "|/-\\|/-\\"[dir8()];
    case '=':
        switch (dir4()) {
        default:
        case 0:
            return 'H';
        case 1:
            return '=';
        case 2:
            return 'H';
        case 3:
            return '=';
        }
    case 'H':
        switch (dir4()) {
        default:
        case 0:
            return '=';
        case 1:
            return 'H';
        case 2:
            return '=';
        case 3:
            return 'H';
        }
    case '\\':
        return "/-\\|/-\\|"[dir8()];
    case '/':
        return "\\|/-\\|/-"[dir8()];
    default:
        ;
    }
    return sym;
}

int tileray::ortho_dx (int od)
{
    int quadr = (direction / 90) % 4;
    od *= -sy[quadr];
    return mostly_vertical() ? od : 0;
}

int tileray::ortho_dy (int od)
{
    int quadr = (direction / 90) % 4;
    od *= sx[quadr];
    return mostly_vertical() ? 0 : od;
}

bool tileray::mostly_vertical ()
{
    return abs(deltax) <= abs(deltay);
}

void tileray::advance (int num)
{
    last_dx = last_dy = 0;
    int ax = abs(deltax);
    int ay = abs(deltay);
    int anum = abs (num);
    for (int i = 0; i < anum; i++) {
        if (mostly_vertical ()) {
            // mostly vertical line
            leftover += ax;
            if (leftover >= ay) {
                last_dx++;
                leftover -= ay;
            }
            last_dy++;
        } else {
            // mostly horizontal line
            leftover += ay;
            if (leftover >= ax) {
                last_dy++;
                leftover -= ax;
            }
            last_dx++;
        }
        steps++;
    }

    // offset calculated for 0-90 deg quadrant, we need to adjust if direction is other
    int quadr = (direction / 90) % 4;
    last_dx *= sx[quadr];
    last_dy *= sy[quadr];
    if (num < 0) {
        last_dx = -last_dx;
        last_dy = -last_dy;
    }
}

bool tileray::end ()
{
    if (infinite) {
        return true;
    }
    return mostly_vertical() ? steps >= abs(deltay) - 1 : steps >= abs(deltax) - 1;
}


