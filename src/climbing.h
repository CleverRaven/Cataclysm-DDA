#pragma once
#ifndef CATA_SRC_CLIMBING_H
#define CATA_SRC_CLIMBING_H

#include <iosfwd>
#include <new>
#include <optional>
#include <set>
#include <vector>
#include <unordered_map>

#include "calendar.h"
#include "type_id.h"

class JsonObject;

/**
* A "Climbing Aid" as defined here is
*/

class climbing_aid
{
    public:
        enum class category {
            special = 0,
            ter_furn,
            veh,
            item,
            character,
            mutation,
            last
        };

        struct condition {
            category    cat; // Called "type" in json
            std::string flag;
            int         uses_item = 0;
            int         range = 1;

            void deserialize( const JsonObject &jo );
        };

        struct climb_cost {
            int pain = 0;
            int damage = 0;
            int kcal = 0;
            int thirst = 0;

            void deserialize( const JsonObject &jo );
        };

    public:
        static void load_climbing_aid( const JsonObject &jo, const std::string &src );
        static void finalize();
        static void check_consistency();
        static void reset();
        void load( const JsonObject &jo, std::string_view );


        using lookup = std::vector<std::unordered_multimap<std::string, const climbing_aid *> >;
        using condition_list = std::vector<condition>;

        using aid_list = std::vector<const climbing_aid *>;


        static const std::vector<climbing_aid> &get_all();
        // static void check_climbing_aid_consistency(); TODO
        bool was_loaded = false;


        static aid_list list( const condition_list &cond );
        static aid_list list_all( const condition_list &cond );

        static const climbing_aid &get_default();
        static const climbing_aid &get_safest( const condition_list &cond, bool no_deploy = true );


        // Identity of the climbing aid.
        climbing_aid_id id;
        std::vector<std::pair<climbing_aid_id, mod_id>> src;

        // Every climbing aid is based on one primary requirement without which it can't be used.
        condition base_condition;


        int slip_chance_mod = 0;

        // Rule for using this aid to climb down.
        struct down_t {
            bool was_loaded = false;

            // Set max_height to 0 to disallow climbing down with this aid.
            int         max_height = 1;

            std::string menu_text;
            std::string menu_cant;
            int         menu_hotkey = 0;

            std::string confirm_text;

            std::string msg_before;
            std::string msg_after;

            climb_cost  cost;
            furn_str_id deploy_furn;

            bool enabled() const noexcept {
                return max_height >= 0;
            }

            bool deploys_furniture() const noexcept {
                return !deploy_furn.is_empty();
            }

            void deserialize( const JsonObject &jo );
        } down;

};
#endif // CATA_SRC_DISEASE_H

