#pragma once
#ifndef FIELD_TYPE_H
#define FIELD_TYPE_H

#include <algorithm>
#include <vector>

#include "calendar.h"
#include "catacharset.h"
#include "color.h"
#include "enums.h"
#include "type_id.h"

class JsonObject;
struct maptile;
struct tripoint;

enum phase_id : int;

struct field_intensity_level {
    std::string name;
    uint32_t symbol = PERCENT_SIGN_UNICODE;
    nc_color color = c_white;
    bool dangerous = false;
    bool transparent = true;
    int move_cost = 0;
};

struct field_type {
    public:
        void load( JsonObject &jo, const std::string &src );
        void check() const;

    public:
        // Used by generic_factory
        field_type_str_id id;
        bool was_loaded = false;

    public:
        int legacy_enum_id = -1;

        std::vector<field_intensity_level> intensity_levels;

        int priority = 0;
        time_duration half_life = 0_days;
        phase_id phase = phase_id::PNULL;
        bool accelerated_decay = false;
        bool do_item = true;
        bool is_draw_field = false;

    public:
        std::string get_name( int level = 0 ) const {
            return intensity_levels[level].name;
        }
        std::string get_symbol( int level = 0 ) const {
            return utf32_to_utf8( intensity_levels[level].symbol );
        }
        nc_color get_color( int level = 0 ) const {
            return intensity_levels[level].color;
        }
        int get_move_cost( int level = 0 ) const {
            return intensity_levels[level].move_cost;
        }
        bool get_dangerous( int level = 0 ) const {
            return intensity_levels[level].dangerous;
        }
        bool get_transparent( int level = 0 ) const {
            return intensity_levels[level].transparent;
        }
        bool is_dangerous() const {
            return std::any_of( intensity_levels.begin(), intensity_levels.end(),
            []( const field_intensity_level & elem ) {
                return elem.dangerous;
            } );
        }

        static size_t count();
};

namespace field_types
{

void load( JsonObject &jo, const std::string &src );
void finalize_all();
void check_consistency();
void reset();

const std::vector<field_type> &get_all();
void set_field_type_ids();
field_type get_field_type_by_legacy_enum( int legacy_enum_id );

}

extern field_type_id x_fd_null,
       x_fd_blood,
       x_fd_bile,
       x_fd_gibs_flesh,
       x_fd_gibs_veggy,
       x_fd_web,
       x_fd_slime,
       x_fd_acid,
       x_fd_sap,
       x_fd_sludge,
       x_fd_fire,
       x_fd_rubble,
       x_fd_smoke,
       x_fd_toxic_gas,
       x_fd_tear_gas,
       x_fd_nuke_gas,
       x_fd_gas_vent,
       x_fd_fire_vent,
       x_fd_flame_burst,
       x_fd_electricity,
       x_fd_fatigue,
       x_fd_push_items,
       x_fd_shock_vent,
       x_fd_acid_vent,
       x_fd_plasma,
       x_fd_laser,
       x_fd_spotlight,
       x_fd_dazzling,
       x_fd_blood_veggy,
       x_fd_blood_insect,
       x_fd_blood_invertebrate,
       x_fd_gibs_insect,
       x_fd_gibs_invertebrate,
       x_fd_cigsmoke,
       x_fd_weedsmoke,
       x_fd_cracksmoke,
       x_fd_methsmoke,
       x_fd_bees,
       x_fd_incendiary,
       x_fd_relax_gas,
       x_fd_fungal_haze,
       x_fd_cold_air1,
       x_fd_cold_air2,
       x_fd_cold_air3,
       x_fd_cold_air4,
       x_fd_hot_air1,
       x_fd_hot_air2,
       x_fd_hot_air3,
       x_fd_hot_air4,
       x_fd_fungicidal_gas,
       x_fd_smoke_vent
       ;

#endif
