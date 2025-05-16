#pragma once
#ifndef CATA_TESTS_CATA_GENERATORS_H
#define CATA_TESTS_CATA_GENERATORS_H

// Some Catch2 Generators for generating our data types

#include "cata_catch.h"
#include "map_scale_constants.h"

struct point;
struct tripoint;

Catch::Generators::GeneratorWrapper<point> random_points( int low = -1000, int high = 1000 );

Catch::Generators::GeneratorWrapper<tripoint> random_tripoints(
    int low = -1000, int high = 1000, int zlow = -OVERMAP_DEPTH, int zhigh = OVERMAP_HEIGHT );

#endif // CATA_TESTS_CATA_GENERATORS_H
