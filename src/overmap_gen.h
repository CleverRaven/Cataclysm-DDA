#pragma once
#ifndef OVERMAP_GEN_H
#define OVERMAP_GEN_H

#include "omdata.h"

class overmap;
class tripoint;

class overmap_gen
{
    public:
        static void build_city( overmap &om, const tripoint &loc, int size );
        static void city_building_line( overmap &om, const city &new_city, const tripoint &origin,
                                        om_direction::type length_dir, int length,
                                        om_direction::type width_dir, int width );
};

#endif