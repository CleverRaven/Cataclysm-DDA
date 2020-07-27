#pragma once
#ifndef CATA_SRC_ANATOMY_H
#define CATA_SRC_ANATOMY_H

#include <algorithm>
#include <string>
#include <vector>

#include "bodypart.h"
#include "string_id.h"

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
        bool was_loaded = false;

        anatomy() = default;
        anatomy( const anatomy & ) = default;
        anatomy &operator=( const anatomy & ) = default;

        /** Returns a random body_part token. main_parts_only will limit it to arms, legs, torso, and head. */
        bodypart_id random_body_part() const;
        /** Returns a random body part dependent on attacker's relative size and hit roll. */
        bodypart_id select_body_part( int size_diff, int hit_roll ) const;

        std::vector<bodypart_id> get_bodyparts() const;
        void add_body_part( const bodypart_str_id &new_bp );
        // TODO: remove_body_part

        void load( const JsonObject &jo, const std::string &src );
        void finalize();
        void check() const;

        static void load_anatomy( const JsonObject &jo, const std::string &src );

        static void reset();
        static void finalize_all();
        static void check_consistency();
};

#endif // CATA_SRC_ANATOMY_H
