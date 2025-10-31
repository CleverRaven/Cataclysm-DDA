#pragma once
#ifndef CATA_SRC_WOUND_H
#define CATA_SRC_WOUND_H

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "calendar.h"
#include "translation.h"
#include "type_id.h"

class JsonObject;
class JsonOut;

enum class bp_type;

class wound_type
{
    public:
        bool was_loaded = false;
        wound_type_id id;

        static void load_wounds( const JsonObject &jo, const std::string &src );
        void load( const JsonObject &jo, const std::string_view &src );
        void reset();
        static void check_consistency();
        void check() const;
        void deserialize( const JsonObject &jo );
        static void finalize_all();
        void finalize();
        static const std::vector<wound_type> &get_all();

        bool allowed_on_bodypart( bodypart_str_id bp_id ) const;
        std::string get_name() const;
        std::string get_description() const;
        int evaluate_pain() const;
        time_duration evaluate_healing_time() const;


        // what damage types can apply this wound
        std::vector<damage_type_id> damage_types;
        // how much damage an attack should deal to apply this wound, lower and upper bound
        std::pair<int, int> damage_required;
        // general weight of this wound comparing to another wounds, smaller = lesser chance to got one
        int weight;

        // this wound can or cannot be applied to this bodyparts
        json_character_flag whitelist_bp_with_flag;
        json_character_flag blacklist_bp_with_flag;

        std::vector<bp_type> whitelist_body_part_types;
        std::vector<bp_type> blacklist_body_part_types;

    private:
        translation name_;
        translation description_;

        // if applied, how long it may take for it to heal
        std::pair<time_duration, time_duration> healing_time_;
        // if applied, how much pain it might provide to you
        std::pair<int, int> pain_;
};

class wound
{
    public:
        wound() = default;
        explicit wound( wound_type_id wd );

        wound_type_id type;

        void serialize( JsonOut &jsout ) const;
        void deserialize( const JsonObject &jsin );
        // returns the wound healing progress divided by healing time
        float healing_percentage() const;
        // return how much pain this specific wound gives you, reduced depending on how long the wound presented.
        int get_pain() const;
        // exposes healing_time
        time_duration get_healing_time() const;
        // update the wound healing progress, if it's healed, return true
        bool update_wound( time_duration time_passed );
    private:
        // how long it takes for this wound to heal. Can be adjusted by wound fix.
        time_duration healing_time;
        // how much time passed since this wound was applied
        time_duration healing_progress;
        // how much pain this specific wound provide.
        int pain;
};

#endif // CATA_SRC_WOUND_H
