#pragma once
#ifndef CATA_SRC_CITY_H
#define CATA_SRC_CITY_H

#include <string>
#include <string_view>
#include <vector>

#include "coordinates.h"
#include "type_id.h"

class JsonObject;
template <typename T> class generic_factory;

struct city {
    void load( const JsonObject &, std::string_view );
    void check() const;
    static void load_city( const JsonObject &, const std::string & );
    static void finalize();
    static void check_consistency();
    static const std::vector<city> &get_all();
    static void reset();

    city_id id;
    bool was_loaded = false;

    int database_id = 0;
    // location of the city (in overmap coordinates)
    point_abs_om pos_om;
    // location of the city (in overmap terrain coordinates)
    point_om_omt pos;
    // original population
    int population = 0;
    int size = -1;
    std::string name;

    explicit city( const point_om_omt &P = point_om_omt(), int S = -1 );

    explicit operator bool() const {
        return size >= 0;
    }

    bool operator==( const city &rhs ) const {
        return id == rhs.id ||
               database_id == rhs.database_id ||
               ( pos_om == rhs.pos_om && pos == rhs.pos ) ;
    }

    int get_distance_from( const tripoint_om_omt &p ) const;
};

generic_factory<city> &get_city_factory();

#endif // CATA_SRC_CITY_H