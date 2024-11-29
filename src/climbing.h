#pragma once
#ifndef CATA_SRC_CLIMBING_H
#define CATA_SRC_CLIMBING_H

#include <algorithm>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "point.h"
#include "translation.h"
#include "type_id.h"

class Character;
class JsonObject;

/**
* A "Climbing Aid" as defined here is any one trait, mutation, tool, furniture terrain, technique
* or any other affordance that facilitates movement up, down or across the environment.
* Availability of climbing aids is subject to various conditions; exactly one is used at a time.
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
            trait,
            last
        };

        struct condition {
            category    cat; // Called "type" in json
            std::string flag;
            int         uses_item = 0;
            int         range = 1;

            std::string category_string() const noexcept;

            void deserialize( const JsonObject &jo );
        };

        struct climb_cost {
            int pain = 0;
            int damage = 0;
            int kcal = 0;
            int thirst = 0;

            void deserialize( const JsonObject &jo );
        };

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


        static condition_list detect_conditions( Character &you, const tripoint &examp );

        static aid_list list( const condition_list &cond );
        static aid_list list_all( const condition_list &cond );

        static const climbing_aid &get_default();
        static const climbing_aid &get_safest( const condition_list &cond, bool no_deploy = true );


        /**
        * Scans downwards from a tile to assess what lies between it and solid ground.
        */
        class fall_scan
        {
            public:
                explicit fall_scan( const tripoint &examp );

                tripoint examp; // Initial position of scan (usually a ledge / open air tile)
                int height; // Z-levels to "ground" based on here.valid_move
                int height_until_creature; // Z-levels below that are free of creatures
                int height_until_furniture; // Z-levels below that are free of furniture
                int height_until_vehicle; // Z-levels below that are free of furniture

                // fall_scan_t is "truthy" if falling is possible at all.
                explicit operator bool() const {
                    return height != 0;
                }

                tripoint pos_top() const {
                    return examp;
                }
                tripoint pos_bottom() const {
                    tripoint ret = examp;
                    ret.z -= height;
                    return ret;
                }
                tripoint pos_just_below() const {
                    tripoint ret = examp;
                    ret.z -= 1;
                    return ret;
                }
                tripoint pos_furniture_or_floor() const {
                    tripoint ret = examp;
                    ret.z -= std::min( height, height_until_furniture + 1 );
                    return ret;
                }

                bool furn_just_below() const {
                    return height > 0 && height_until_furniture == 0;
                }
                bool furn_below() const {
                    return height > 0 && height_until_furniture != height;
                }
                bool veh_just_below() const {
                    return height > 0 && height_until_vehicle == 0;
                }
                bool veh_below() const {
                    return height > 0 && height_until_vehicle != height;
                }
                bool crea_just_below() const {
                    return height > 0 && height_until_creature == 0;
                }
                bool crea_below() const {
                    return height > 0 && height_until_creature != height;
                }
        };


        // Identity of the climbing aid, and index in get_all() list.
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

            // Set to number of Z-levels that will be trivial to climb back up.
            //    Usually set to zero or equal to max_height.
            int         easy_climb_back_up = 0;

            // Whether this aid can be used to climb down partway to ground.  Default true.
            //   Note that climbing down partway usually means the player will fall...
            bool        allow_remaining_height = true;

            // Menu text.  Menus normally display all deploy aids and the safest non-deploy aid.
            translation menu_text;
            translation menu_cant;
            int         menu_hotkey = 0;

            translation confirm_text;

            translation msg_before;
            translation msg_after;

            // Physical cost of using the aid and furniture created (these often go together).
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
#endif // CATA_SRC_CLIMBING_H

