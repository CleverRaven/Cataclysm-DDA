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

struct mortar_error {
    double range = 0.0;
    double deflection = 0.0;
};

struct mortar_location_error {
    double range = 0.0;
    double deflection = 0.0;
};

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
        static int minimum_launcher_skill();
        static double skill_accuracy_multiplier( int launcher_skill );
        static double effective_ballistic_multiplier( double raw_multiplier );

        void load( const JsonObject &jo, std::string_view src );

        const furn_str_id &furniture() const;
        const ammotype &ammo() const;
        int range() const;
        time_duration player_flight_time() const;
        time_duration npc_fire_message_delay() const;
        time_duration npc_impact_delay() const;
        time_duration npc_impact_message_delay() const;

        double minimum_range_error( int distance ) const;
        double minimum_deflection_error( int distance ) const;
        mortar_error minimum_error( int distance ) const;
        int minimum_target_distance( int target_distance, double ballistic_multiplier ) const;
        double repeat_cep_multiplier( int launcher_skill ) const;
        tripoint_abs_ms apply_dispersion( const tripoint_abs_ms &target,
                                          const tripoint_abs_ms &axis_from,
                                          const tripoint_abs_ms &axis_to,
                                          const mortar_error &error,
                                          double *deflection_error = nullptr ) const;
        tripoint_abs_ms apply_location_error( const tripoint_abs_ms &target,
                                              const tripoint_abs_ms &axis_from,
                                              const tripoint_abs_ms &axis_to,
                                              const mortar_location_error &error ) const;
        mortar_error project_location_error( const tripoint_abs_ms &axis_from,
                                             const tripoint_abs_ms &axis_to,
                                             const tripoint_abs_ms &location_axis_from,
                                             const tripoint_abs_ms &location_axis_to,
                                             const mortar_location_error &location_error ) const;
        bool point_in_probable_impact_area( const tripoint_abs_ms &target,
                                            const tripoint_abs_ms &axis_from,
                                            const tripoint_abs_ms &axis_to,
                                            const mortar_error &error,
                                            const tripoint_abs_ms &location_axis_from,
                                            const tripoint_abs_ms &location_axis_to,
                                            const mortar_location_error &location_error,
                                            const tripoint_abs_ms &point,
                                            double scale ) const;

    private:
        furn_str_id furniture_;
        ammotype ammo_;
        int range_ = 0;
        time_duration player_flight_time_ = 0_seconds;
        time_duration npc_fire_message_delay_ = 0_seconds;
        time_duration npc_impact_delay_ = 0_seconds;
        time_duration npc_impact_message_delay_ = 0_seconds;
        double range_error_ratio_ = 0.015;
        double deflection_error_mils_ = 2.0;
};

#endif // CATA_SRC_MORTAR_H
