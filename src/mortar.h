#pragma once
#ifndef CATA_SRC_MORTAR_H
#define CATA_SRC_MORTAR_H

#include <string>
#include <string_view>
#include <vector>

#include "calendar.h"
#include "coordinates.h"
#include "type_id.h"

class JsonObject;
class item;

class mortar_type
{
        friend class DynamicDataLoader;
    public:
        mortar_type_id id;
        bool was_loaded = false;

        static void load_mortar_type( const JsonObject &jo, const std::string &src );
        static void finalize_all();
        static void check_consistency();
        static void reset();
        static const std::vector<mortar_type> &get_all();
        static const mortar_type *from_furniture( const furn_str_id &furn );
        static bool is_mortar_round( const item &it );

        void load( const JsonObject &jo, std::string_view src );

        const furn_str_id &furniture() const;
        const ammotype &ammo() const;
        int range() const;
        time_duration player_flight_time() const;
        time_duration npc_fire_message_delay() const;
        time_duration npc_impact_delay() const;
        time_duration npc_impact_message_delay() const;
        double baseline_cep() const;

        double minimum_cep( int launcher_skill ) const;
        double repeat_cep_multiplier( int launcher_skill ) const;
        double player_cep( double aim_deviation, int distance, int launcher_skill ) const;
        tripoint_abs_ms apply_dispersion( const tripoint_abs_ms &target,
                                          const tripoint_abs_ms &axis_from,
                                          const tripoint_abs_ms &axis_to,
                                          double cep, int launcher_skill,
                                          double *minor_cep = nullptr ) const;

    private:
        furn_str_id furniture_;
        ammotype ammo_;
        int range_ = 0;
        time_duration player_flight_time_ = 0_seconds;
        time_duration npc_fire_message_delay_ = 0_seconds;
        time_duration npc_impact_delay_ = 0_seconds;
        time_duration npc_impact_message_delay_ = 0_seconds;
        double cep_baseline_ = 100.0;
        double cep_min_base_ = 20.0;
        double cep_min_skill_scale_ = 1.0;
        double cep_min_floor_ = 5.0;
        double axis_ratio_baseline_ = 4.0;
        double axis_ratio_final_base_ = 2.5;
        double axis_ratio_skill_scale_ = 0.1;
        double axis_ratio_floor_ = 1.2;

        double axis_ratio( double cep, int launcher_skill ) const;
};

#endif // CATA_SRC_MORTAR_H
