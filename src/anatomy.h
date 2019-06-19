#pragma once
#ifndef ANATOMY_H
#define ANATOMY_H

#include <vector>
#include <string>

#include "bodypart.h"
#include "string_id.h"

class anatomy;
class JsonObject;

using anatomy_id = string_id<anatomy>;

/**
 * A structure that contains body parts.
 * Keeps caches for weighted selections.
 */
class anatomy
{
    private:
        std::vector<bodypart_ids> unloaded_bps;
        std::vector<bodypart_id> cached_bps;
        /** Sum of chances to hit a body part randomly, without aiming. */
        float size_sum = 0.0f;

        // TODO: get_better_name_for_function
        bodypart_ids get_part_with_cumulative_hit_size( float size ) const;

    public:
        anatomy_id id;
        bool was_loaded = false;

        anatomy() = default;
        anatomy( const anatomy & ) = default;

        /** Returns a random body_part token. main_parts_only will limit it to arms, legs, torso, and head. */
        bodypart_id random_body_part() const;
        /** Returns a random body part dependent on attacker's relative size and hit roll. */
        bodypart_id select_body_part( int size_diff, int hit_roll ) const;

        void add_body_part( const bodypart_ids &new_bp );
        // TODO: remove_body_part

        void load( JsonObject &jo, const std::string &src );
        void finalize();
        void check() const;

        static void load_anatomy( JsonObject &jo, const std::string &src );

        static void reset();
        static void finalize_all();
        static void check_consistency();
};

extern anatomy_id human_anatomy;

#endif
