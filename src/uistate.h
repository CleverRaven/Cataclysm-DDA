#pragma once
#ifndef UISTATE_H
#define UISTATE_H

#include <list>
#include <map>
#include <string>
#include <vector>

#include "enums.h"
#include "omdata.h"
#include "type_id.h"

class item;

struct comestible_inv_save_state {
    public:
        int sort_idx = -1;
        std::string filter;
        int area_idx = 13; //A+I+W

        /**
        When we sort by weight and item is consumed it might jump down the list, so we want to keep track of that item with pointer
        When item is fully consumed we want to keep list at the same index
        */
        int selected_idx = 0;
        item *selected_itm = nullptr;

        int bio = -1;
        bool show_food = true;

        bool in_vehicle = false;
        int exit_code = 0;
        bool food_filter = true;

        int container_location = -1;
        int container_index = 0;
        itype_id container_type = "null";
        itype_id container_content_type = "null";
};

/*
  centralized depot for trivial ui data such as sorting, string_input_popup history, etc.
  To use this, see the ****notes**** below
*/
// There is only one game instance, so losing a few bytes of memory
// due to padding is not much of a concern.
// NOLINTNEXTLINE(clang-analyzer-optin.performance.Padding)
class uistatedata
{
        /**** this will set a default value on startup, however to save, see below ****/
    private:
        // not needed for compilation, but keeps syntax plugins happy
        using itype_id = std::string;
        enum side { left  = 0, right = 1, NUM_PANES = 2 };
    public:
        int ags_pay_gas_selected_pump = 0;

        int wishitem_selected = 0;
        int wishmutate_selected = 0;
        int wishmonster_selected = 0;
        int iexamine_atm_selected = 0;
        std::array<int, 2> adv_inv_sort = {{1, 1}};
        std::array<int, 2> adv_inv_area = {{5, 0}};
        std::array<int, 2> adv_inv_index = {{0, 0}};
        std::array<bool, 2> adv_inv_in_vehicle = {{false, false}};
        std::array<std::string, 2> adv_inv_filter = {{"", ""}};
        std::array<int, 2> adv_inv_default_areas = {{11, 0}}; //left: All, right: Inventory
        int adv_inv_src = left;
        int adv_inv_dest = right;
        int adv_inv_last_popup_dest = 0;
        int adv_inv_container_location = -1;
        int adv_inv_container_index = 0;
        int adv_inv_exit_code = 0;
        itype_id adv_inv_container_type = "null";
        itype_id adv_inv_container_content_type = "null";
        int adv_inv_re_enter_move_all = 0;
        int adv_inv_aim_all_location = 1;
        std::map<int, std::list<item>> adv_inv_veh_items, adv_inv_map_items;
        bool adv_inv_container_in_vehicle = false;

        comestible_inv_save_state comestible_save;

        bool editmap_nsa_viewmode = false;      // true: ignore LOS and lighting
        bool overmap_blinking = true;           // toggles active blinking of overlays.
        bool overmap_show_overlays = false;     // whether overlays are shown or not.
        bool overmap_show_map_notes = true;
        bool overmap_show_land_use_codes = false; // toggle land use code sym/color for terrain
        bool overmap_show_city_labels = true;
        bool overmap_show_hordes = true;
        bool overmap_show_forest_trails = true;

        bool debug_ranged;
        tripoint adv_inv_last_coords = {-999, -999, -999};
        int last_inv_start = -2;
        int last_inv_sel = -2;

        // V Menu Stuff
        int list_item_sort = 0;
        std::string list_item_filter;
        std::string list_item_downvote;
        std::string list_item_priority;
        bool vmenu_show_items = true; // false implies show monsters
        bool list_item_filter_active = false;
        bool list_item_downvote_active = false;
        bool list_item_priority_active = false;
        bool list_item_init = false;

        // construction menu selections
        std::string construction_filter;
        std::string last_construction;

        // overmap editor selections
        const oter_t *place_terrain = nullptr;
        const overmap_special *place_special = nullptr;
        om_direction::type omedit_rotation = om_direction::type::none;

        std::set<recipe_id> hidden_recipes;
        std::set<recipe_id> favorite_recipes;
        std::vector<recipe_id> recent_recipes;

        /* to save input history and make accessible via 'up', you don't need to edit this file, just run:
           output = string_input_popup(str, int, str, str, std::string("set_a_unique_identifier_here") );
        */

        std::map<std::string, std::vector<std::string>> input_history;

        std::map<ammotype, itype_id> lastreload; // id of ammo last used when reloading ammotype

        // internal stuff
        bool _testing_save = true; // internal: whine on json errors. set false if no complaints in 2 weeks.
        bool _really_testing_save = false; // internal: spammy

        std::vector<std::string> &gethistory( const std::string &id ) {
            return input_history[id];
        }

        // nice little convenience function for serializing an array, regardless of amount. :^)
        template<typename JsonStream, typename T>
        void serialize_array( JsonStream &json, std::string name, T &data ) const {
            json.member( name );
            json.start_array();
            for( const auto &d : data ) {
                json.write( d );
            }
            json.end_array();
        }

        template<typename JsonStream>
        void serialize( JsonStream &json ) const {
            const unsigned int input_history_save_max = 25;
            json.start_object();

            /**** if you want to save whatever so it's whatever when the game is started next, declare here and.... ****/
            serialize_array( json, "adv_inv_sort", adv_inv_sort );
            serialize_array( json, "adv_inv_area", adv_inv_area );
            serialize_array( json, "adv_inv_index", adv_inv_index );
            serialize_array( json, "adv_inv_in_vehicle", adv_inv_in_vehicle );
            serialize_array( json, "adv_inv_filter", adv_inv_filter );
            serialize_array( json, "adv_inv_default_areas", adv_inv_default_areas );
            // non array stuffs
            json.member( "adv_inv_src", adv_inv_src );
            json.member( "adv_inv_dest", adv_inv_dest );
            json.member( "adv_inv_last_popup_dest", adv_inv_last_popup_dest );
            json.member( "adv_inv_container_location", adv_inv_container_location );
            json.member( "adv_inv_container_index", adv_inv_container_index );
            json.member( "adv_inv_container_in_vehicle", adv_inv_container_in_vehicle );
            json.member( "adv_inv_container_type", adv_inv_container_type );
            json.member( "adv_inv_container_content_type", adv_inv_container_content_type );
            json.member( "editmap_nsa_viewmode", editmap_nsa_viewmode );
            json.member( "overmap_blinking", overmap_blinking );
            json.member( "overmap_show_overlays", overmap_show_overlays );
            json.member( "overmap_show_map_notes", overmap_show_map_notes );
            json.member( "overmap_show_land_use_codes", overmap_show_land_use_codes );
            json.member( "overmap_show_city_labels", overmap_show_city_labels );
            json.member( "overmap_show_hordes", overmap_show_hordes );
            json.member( "overmap_show_forest_trails", overmap_show_forest_trails );
            json.member( "vmenu_show_items", vmenu_show_items );
            json.member( "list_item_sort", list_item_sort );
            json.member( "list_item_filter_active", list_item_filter_active );
            json.member( "list_item_downvote_active", list_item_downvote_active );
            json.member( "list_item_priority_active", list_item_priority_active );
            json.member( "hidden_recipes", hidden_recipes );
            json.member( "favorite_recipes", favorite_recipes );
            json.member( "recent_recipes", recent_recipes );

            json.member( "input_history" );
            json.start_object();
            for( auto &e : input_history ) {
                json.member( e.first );
                const std::vector<std::string> &history = e.second;
                json.start_array();
                int save_start = 0;
                if( history.size() > input_history_save_max ) {
                    save_start = history.size() - input_history_save_max;
                }
                for( std::vector<std::string>::const_iterator hit = history.begin() + save_start;
                     hit != history.end(); ++hit ) {
                    json.write( *hit );
                }
                json.end_array();
            }
            json.end_object(); // input_history

            json.end_object();
        }

        template<typename JsonStream>
        void deserialize( JsonStream &jsin ) {
            auto jo = jsin.get_object();
            /**** here ****/
            if( jo.has_array( "adv_inv_sort" ) ) {
                auto tmp = jo.get_int_array( "adv_inv_sort" );
                std::move( tmp.begin(), tmp.end(), adv_inv_sort.begin() );
            } else {
                jo.read( "adv_inv_leftsort", adv_inv_sort[left] );
                jo.read( "adv_inv_rightsort", adv_inv_sort[right] );
            }
            // pane area selected
            if( jo.has_array( "adv_inv_area" ) ) {
                auto tmp = jo.get_int_array( "adv_inv_area" );
                std::move( tmp.begin(), tmp.end(), adv_inv_area.begin() );
            } else {
                jo.read( "adv_inv_leftarea", adv_inv_area[left] );
                jo.read( "adv_inv_rightarea", adv_inv_area[right] );
            }
            // pane current index
            if( jo.has_array( "adv_inv_index" ) ) {
                auto tmp = jo.get_int_array( "adv_inv_index" );
                std::move( tmp.begin(), tmp.end(), adv_inv_index.begin() );
            } else {
                jo.read( "adv_inv_leftindex", adv_inv_index[left] );
                jo.read( "adv_inv_rightindex", adv_inv_index[right] );
            }
            // viewing vehicle cargo
            if( jo.has_array( "adv_inv_in_vehicle" ) ) {
                auto ja = jo.get_array( "adv_inv_in_vehicle" );
                for( size_t i = 0; ja.has_more(); ++i ) {
                    adv_inv_in_vehicle[i] = ja.next_bool();
                }
            }
            // filter strings
            if( jo.has_array( "adv_inv_filter" ) ) {
                auto tmp = jo.get_string_array( "adv_inv_filter" );
                std::move( tmp.begin(), tmp.end(), adv_inv_filter.begin() );
            } else {
                jo.read( "adv_inv_leftfilter", adv_inv_filter[left] );
                jo.read( "adv_inv_rightfilter", adv_inv_filter[right] );
            }
            // default areas
            if( jo.has_array( "adv_inv_deafult_areas" ) ) {
                auto tmp = jo.get_int_array( "adv_inv_deafult_areas" );
                std::move( tmp.begin(), tmp.end(), adv_inv_default_areas.begin() );
            }
            // the rest
            jo.read( "adv_inv_src", adv_inv_src );
            jo.read( "adv_inv_dest", adv_inv_dest );
            jo.read( "adv_inv_last_popup_dest", adv_inv_last_popup_dest );
            jo.read( "adv_inv_container_location", adv_inv_container_location );
            jo.read( "adv_inv_container_index", adv_inv_container_index );
            jo.read( "adv_inv_container_in_vehicle", adv_inv_container_in_vehicle );
            jo.read( "adv_inv_container_type", adv_inv_container_type );
            jo.read( "adv_inv_container_content_type", adv_inv_container_content_type );
            jo.read( "overmap_blinking", overmap_blinking );
            jo.read( "overmap_show_overlays", overmap_show_overlays );
            jo.read( "overmap_show_map_notes", overmap_show_map_notes );
            jo.read( "overmap_show_land_use_codes", overmap_show_land_use_codes );
            jo.read( "overmap_show_city_labels", overmap_show_city_labels );
            jo.read( "overmap_show_hordes", overmap_show_hordes );
            jo.read( "overmap_show_forest_trails", overmap_show_forest_trails );
            jo.read( "hidden_recipes", hidden_recipes );
            jo.read( "favorite_recipes", favorite_recipes );
            jo.read( "recent_recipes", recent_recipes );

            if( !jo.read( "vmenu_show_items", vmenu_show_items ) ) {
                // This is an old save: 1 means view items, 2 means view monsters,
                // -1 means uninitialized
                vmenu_show_items = jo.get_int( "list_item_mon", -1 ) != 2;
            }

            jo.read( "list_item_sort", list_item_sort );
            jo.read( "list_item_filter_active", list_item_filter_active );
            jo.read( "list_item_downvote_active", list_item_downvote_active );
            jo.read( "list_item_priority_active", list_item_priority_active );

            auto inhist = jo.get_object( "input_history" );
            std::set<std::string> inhist_members = inhist.get_member_names();
            for( const auto &inhist_member : inhist_members ) {
                auto ja = inhist.get_array( inhist_member );
                std::vector<std::string> &v = gethistory( inhist_member );
                v.clear();
                while( ja.has_more() ) {
                    v.push_back( ja.next_string() );
                }
            }
            // fetch list_item settings from input_history
            if( !gethistory( "item_filter" ).empty() ) {
                list_item_filter = gethistory( "item_filter" ).back();
            }
            if( !gethistory( "list_item_downvote" ).empty() ) {
                list_item_downvote = gethistory( "list_item_downvote" ).back();
            }
            if( !gethistory( "list_item_priority" ).empty() ) {
                list_item_priority = gethistory( "list_item_priority" ).back();
            }
        }
};

extern uistatedata uistate;
#endif
