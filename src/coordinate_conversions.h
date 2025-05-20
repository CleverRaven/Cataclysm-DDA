#pragma once
#ifndef CATA_SRC_COORDINATE_CONVERSIONS_H
#define CATA_SRC_COORDINATE_CONVERSIONS_H

#include "point.h"

/**
 * This file defines legacy coordinate conversion functions.  We should be
 * migrating to the new functions defined in coordinates.h.
 *
 * For documentation on coordinate systems in general see
 * doc/c++/POINTS_COORDINATES.md.
 *

 * Functions ending with _remain return the translated coordinates and
 * store the remainder in the parameters.
 */

// Submap to memory map region.
point sm_to_mmr_remain( int &x, int &y );

#endif // CATA_SRC_COORDINATE_CONVERSIONS_H
