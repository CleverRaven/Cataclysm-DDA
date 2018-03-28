#pragma once
#ifndef UISTATE_H
#define UISTATE_H

#include <typeinfo>
#include <list>

#include "enums.h"
#include "omdata.h"
#include "pimpl.h"

#include <map>
#include <vector>
#include <string>

class ammunition_type;
using ammotype = string_id<ammunition_type>;

class item;
class JsonIn;
class JsonOut;
class JsonSerDes;

class crafting_uistatedata;
class editmap_uistatedata;
class game_uistatedata;
class iexamine_uistatedata;
class overmap_uistatedata;

/*
  centralized depot for trivial ui data such as sorting, string_input_popup history, etc.
  To use this, see the ****notes**** below
*/
class uistatedata
{
    protected:
        std::vector<std::unique_ptr<JsonSerDes>> modules;
    public:
        uistatedata();
    public:
        pimpl<crafting_uistatedata> crafting;
        pimpl<editmap_uistatedata> editmap;
        pimpl<game_uistatedata> game;
        pimpl<iexamine_uistatedata> iexamine;
        pimpl<overmap_uistatedata> overmap;
        /**** this will set a default value on startup, however to save, see below ****/
    private:
        // not needed for compilation, but keeps syntax plugins happy
        typedef std::string itype_id;
        enum side { left  = 0, right = 1, NUM_PANES = 2 };
    public:
        /**** declare your variable here. It can be anything, really *****/
        int wishitem_selected = 0;
        int wishmutate_selected = 0;
        int wishmonster_selected = 0;
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
        bool adv_inv_container_in_vehicle = 0;
        int adv_inv_exit_code = 0;
        itype_id adv_inv_container_type = "null";
        itype_id adv_inv_container_content_type = "null";
        int adv_inv_re_enter_move_all = 0;
        int adv_inv_aim_all_location = 1;
        std::map<int, std::list<item>> adv_inv_veh_items, adv_inv_map_items;

        bool debug_ranged;
        tripoint adv_inv_last_coords = {-999, -999, -999};
        int last_inv_start = -2;
        int last_inv_sel = -2;

        // construction menu selections
        std::string construction_filter;
        std::string last_construction;

        std::map<ammotype, itype_id> lastreload; // id of ammo last used when reloading ammotype

        // internal stuff
        bool _testing_save = true; // internal: whine on json errors. set false if no complaints in 2 weeks.
        bool _really_testing_save = false; // internal: spammy

    protected:
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
        void _serialize( JsonStream &json ) const {
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
        };

        template<typename JsonStream>
        void _deserialize( JsonStream &jsin ) {
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
        };

    public:
        void serialize( JsonOut &json ) const;
        void deserialize( JsonIn &jsin );
};
extern uistatedata uistate;

#endif
