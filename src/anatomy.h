#pragma once
#ifndef CATA_SRC_ANATOMY_H
#define CATA_SRC_ANATOMY_H

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "bodypart.h"
#include "type_id.h"

class Creature;
class JsonObject;
class anatomy;

using anatomy_id = string_id<anatomy>;

/**
 * A structure that contains body parts.
 * Keeps caches for weighted selections.
 */
class anatomy
{
    private:
        std::vector<bodypart_str_id> unloaded_bps;
        std::vector<bodypart_id> cached_bps;
        /** Sum of chances to hit a body part randomly, without aiming. */
        float size_sum = 0.0f;

        // TODO: get_better_name_for_function
        bodypart_str_id get_part_with_cumulative_hit_size( float size ) const;

    public:
        anatomy_id id;
        std::vector<std::pair<anatomy_id, mod_id>> src;
        bool was_loaded = false;

        anatomy() = default;
        anatomy( const anatomy & ) = default;
        anatomy &operator=( const anatomy & ) = default;
        explicit anatomy( const std::vector<bodypart_id> &parts );

        /** Returns a random body_part token. main_parts_only will limit it to arms, legs, torso, and head. */
        bodypart_id random_body_part() const;
        // Returns a random bodypart determined by the attacks hitsize/limb restrictions
        bodypart_id select_body_part( int min_hit, int max_hit, bool can_attack_high, int hit_roll ) const;
        bodypart_id select_blocking_part( const Creature *blocker, bool arm, bool leg,
                                          bool nonstandard ) const;
        std::vector<bodypart_id> get_all_eligable_parts( int min_hit, int max_hit,
                bool can_attack_high ) const;
        // Find the body part with the biggest hitsize - we will treat this as the center of mass for targeting
        bodypart_id get_max_hitsize_bodypart() const;
        // Based on the value provided (which is between range_min and range_max),
        // select an appropriate body part to hit with a projectile attack
        bodypart_id select_body_part_projectile_attack( double range_min, double range_max,
                double value ) const;

        std::vector<bodypart_id> get_bodyparts() const;
        float get_size_ratio( const anatomy_id &base ) const;
        float get_hit_size_sum() const;
        float get_organic_size_sum() const;
        float get_base_hit_size_sum( const anatomy_id &base ) const;
        void add_body_part( const bodypart_str_id &new_bp );
        // TODO: remove_body_part

        void load( const JsonObject &jo, std::string_view src );
        void finalize();
        void check() const;

        static void load_anatomy( const JsonObject &jo, const std::string &src );

        static void reset();
        static void finalize_all();
        static void check_consistency();
};

#endif // CATA_SRC_ANATOMY_H
