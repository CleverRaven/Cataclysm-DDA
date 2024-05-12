#pragma once
#ifndef CATA_SRC_LINE_COORDINATES_H
#define CATA_SRC_LINE_H
// Cannot include this in line.h since that would require a mutual header dependency C can't handle.
// The purpose of this header is to provide operations on the derived point types. Note that the
// implementations are still located in line.cpp.

#include <vector>

#include "coordinates.h"
#include "line.h"

// t and t2 decide which Bresenham line is used.
std::vector<tripoint_bub_ms> line_to( const tripoint_bub_ms &loc1, const tripoint_bub_ms &loc2,
                                      int t = 0, int t2 = 0 );
// sqrt(dX^2 + dY^2)

#endif // CATA_SRC_LINE_COORDINATES_H
