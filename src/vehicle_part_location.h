#pragma once
#ifndef CATA_SRC_VEHICLE_PART_LOCATION_H
#define CATA_SRC_VEHICLE_PART_LOCATION_H

#include <string>
#include <string_view>

#include "translation.h"
#include "type_id.h"

class JsonObject;

class vpart_location
{
    public:
        bool was_loaded = false;
        vpart_location_id id;

        translation name;
        translation description;
        // Highest Z is rendered, potentially excluding roof. -1 never renders.
        int z_order = 0;
        // Display order in vehicle interact display, lowest first
        int list_order = 5;

        static void load_vehicle_part_locations( const JsonObject &jo, const std::string &src );
        static void reset();
        static void finalize_all();
        static void check_all();
        void load( const JsonObject &jo, const std::string_view &src );
        void check() const;
};

#endif // CATA_SRC_VEHICLE_PART_LOCATION_H
