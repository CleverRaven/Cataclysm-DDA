#pragma once
#ifndef CATA_SRC_UISTATE_H
#define CATA_SRC_UISTATE_H

#include <list>
#include <map>
#include <string>
#include <vector>

#include "enums.h"
#include "optional.h"
#include "omdata.h"
#include "type_id.h"

class item;

struct advanced_inv_pane_save_state {
    public:
        int sort_idx = 1;
        std::string filter;
        int area_idx = 11;
        int selected_idx = 0;

        bool in_vehicle = false;

        template<typename JsonStream>
        void serialize( JsonStream &json, const std::string &prefix ) const {
            json.member( prefix + "sort_idx", sort_idx );
            json.member( prefix + "filter", filter );
            json.member( prefix + "area_idx", area_idx );
            json.member( prefix + "selected_idx", selected_idx );
            json.member( prefix + "in_vehicle", in_vehicle );
        }

        void deserialize( const JsonObject &jo, const std::string &prefix ) {
            jo.read( prefix + "sort_idx", sort_idx );
            jo.read( prefix + "filter", filter );
            jo.read( prefix + "area_idx", area_idx );
            jo.read( prefix + "selected_idx", selected_idx );
            jo.read( prefix + "in_vehicle", in_vehicle );
        }
};

struct advanced_inv_save_state {
    public:
        int exit_code = 0;
        int re_enter_move_all = 0;
        int aim_all_location = 1;

        bool active_left = true;
        int last_popup_dest = 0;

        int saved_area = 11;
        int saved_area_right = 0;
        advanced_inv_pane_save_state pane;
        advanced_inv_pane_save_state pane_right;

        template<typename JsonStream>
        void serialize( JsonStream &json, const std::string &prefix ) const {
            json.member( prefix + "active_left", active_left );
            json.member( prefix + "last_popup_dest", last_popup_dest );

            json.member( prefix + "saved_area", saved_area );
            json.member( prefix + "saved_area_right", saved_area_right );
            pane.serialize( json, prefix + "pane_" );
            pane_right.serialize( json, prefix + "pane_right_" );
        }

        void deserialize( JsonObject &jo, const std::string &prefix ) {
            jo.read( prefix + "active_left", active_left );
            jo.read( prefix + "last_popup_dest", last_popup_dest );

            jo.read( prefix + "saved_area", saved_area );
            jo.read( prefix + "saved_area_right", saved_area_right );
            pane.area_idx = saved_area;
            pane_right.area_idx = saved_area_right;
            pane.deserialize( jo, prefix + "pane_" );
            pane_right.deserialize( jo, prefix + "pane_right_" );
        }
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
        enum side { left = 0, right = 1, NUM_PANES = 2 };
    public:
        int ags_pay_gas_selected_pump = 0;

        int wishitem_selected = 0;
        int wishmutate_selected = 0;
        int wishmonster_selected = 0;
        int iexamine_atm_selected = 0;

        int adv_inv_container_location = -1;
        int adv_inv_container_index = 0;
        itype_id adv_inv_container_type = itype_id::NULL_ID();
        itype_id adv_inv_container_content_type = itype_id::NULL_ID();
        bool adv_inv_container_in_vehicle = false;

        advanced_inv_save_state transfer_save;

        bool editmap_nsa_viewmode = false;      // true: ignore LOS and lighting
        bool overmap_blinking = true;           // toggles active blinking of overlays.
        bool overmap_show_overlays = false;     // whether overlays are shown or not.
        bool overmap_show_map_notes = true;
        bool overmap_show_land_use_codes = false; // toggle land use code sym/color for terrain
        bool overmap_show_city_labels = true;
        bool overmap_show_hordes = true;
        bool overmap_show_forest_trails = true;

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
        cata::optional<std::string> last_construction;
        construction_category_id construction_tab = construction_category_id::NULL_ID();

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

            transfer_save.serialize( json, "transfer_save_" );

            /**** if you want to save whatever so it's whatever when the game is started next, declare here and.... ****/
            // non array stuffs
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

            transfer_save.deserialize( jo, "transfer_save_" );
            // the rest
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

            for( const JsonMember member : jo.get_object( "input_history" ) ) {
                std::vector<std::string> &v = gethistory( member.name() );
                v.clear();
                for( const std::string line : member.get_array() ) {
                    v.push_back( line );
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

#endif // CATA_SRC_UISTATE_H
