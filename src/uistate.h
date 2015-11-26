#ifndef UISTATE_H
#define UISTATE_H

#include <typeinfo>
#include <list>

#include "json.h"
#include "enums.h"

#include <map>
#include <vector>
#include <string>

class item;

/*
  centralized depot for trivial ui data such as sorting, string_input_popup history, etc.
  To use this, see the ****notes**** below
*/
class uistatedata : public JsonSerializer, public JsonDeserializer
{
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
        int iexamine_atm_selected = 0;
        std::array<int, 2> adv_inv_sort = {{1, 1}};
        std::array<int, 2> adv_inv_area = {{5, 0}};
        std::array<int, 2> adv_inv_index = {{0, 0}};
        std::array<bool, 2> adv_inv_in_vehicle = {{false, false}};
        std::array<std::string, 2> adv_inv_filter = {{"", ""}};
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

        int ags_pay_gas_selected_pump = 0;

        bool editmap_nsa_viewmode = false;      // true: ignore LOS and lighting
        bool overmap_blinking = true;           // toggles active blinking of overlays.
        bool overmap_show_overlays = false;     // whether overlays are shown or not.
        bool debug_ranged;
        tripoint adv_inv_last_coords = {-999, -999, -999};
        int last_inv_start = -2;
        int last_inv_sel = -2;
        int list_item_mon = -1;
        int list_item_sort = 0;
        std::string list_item_filter;
        std::string list_item_downvote;
        std::string list_item_priority;
        bool list_item_filter_active = false;
        bool list_item_downvote_active = false;
        bool list_item_priority_active = false;
        bool list_item_init = false;

        /* to save input history and make accessible via 'up', you don't need to edit this file, just run:
           output = string_input_popup(str, int, str, str, std::string("set_a_unique_identifier_here") );
        */

        std::map<std::string, std::vector<std::string>> input_history;

        std::map<std::string, std::string> lastreload; // last typeid used when reloading ammotype

        // internal stuff
        bool _testing_save = true; // internal: whine on json errors. set false if no complaints in 2 weeks.
        bool _really_testing_save = false; // internal: spammy

        std::vector<std::string>& gethistory(std::string id)
        {
            return input_history[id];
        }

        // nice little convenience function for serializing an array, regardless of amount. :^)
        template <typename T>
        void serialize_array(JsonOut &json, std::string name, T &data) const
        {
            json.member(name);
            json.start_array();
            for(const auto &d : data) {
                json.write(d);
            }
            json.end_array();
        }

        using JsonSerializer::serialize;
        void serialize(JsonOut &json) const override
        {
            const unsigned int input_history_save_max = 25;
            json.start_object();

            /**** if you want to save whatever so it's whatever when the game is started next, declare here and.... ****/
            serialize_array(json, "adv_inv_sort", adv_inv_sort);
            serialize_array(json, "adv_inv_area", adv_inv_area);
            serialize_array(json, "adv_inv_index", adv_inv_index);
            serialize_array(json, "adv_inv_in_vehicle", adv_inv_in_vehicle);
            serialize_array(json, "adv_inv_filter", adv_inv_filter);
            // non array stuffs
            json.member("adv_inv_src", adv_inv_src);
            json.member("adv_inv_dest", adv_inv_dest);
            json.member("adv_inv_last_popup_dest", adv_inv_last_popup_dest);
            json.member("adv_inv_container_location", adv_inv_container_location);
            json.member("adv_inv_container_index", adv_inv_container_index);
            json.member("adv_inv_container_in_vehicle", adv_inv_container_in_vehicle);
            json.member("adv_inv_container_type", adv_inv_container_type);
            json.member("adv_inv_container_content_type", adv_inv_container_content_type);
            json.member("editmap_nsa_viewmode", editmap_nsa_viewmode);
            json.member("overmap_blinking", overmap_blinking);
            json.member("overmap_show_overlays", overmap_show_overlays);
            json.member("list_item_mon", list_item_mon);
            json.member("list_item_sort", list_item_sort);
            json.member("list_item_filter_active", list_item_filter_active);
            json.member("list_item_downvote_active", list_item_downvote_active);
            json.member("list_item_priority_active", list_item_priority_active);

            json.member("input_history");
            json.start_object();
            for( auto& e : input_history ) {
                json.member( e.first );
                const std::vector<std::string>& history = e.second;
                json.start_array();
                int save_start = 0;
                if ( history.size() > input_history_save_max) {
                    save_start = history.size() - input_history_save_max;
                }
                for (std::vector<std::string>::const_iterator hit = history.begin() + save_start;
                     hit != history.end(); ++hit ) {
                    json.write(*hit);
                }
                json.end_array();
            }
            json.end_object(); // input_history

            json.end_object();
        };

        void deserialize(JsonIn &jsin) override
        {
            JsonObject jo = jsin.get_object();
            /**** here ****/
            if(jo.has_array("adv_inv_sort")) {
                auto tmp = jo.get_int_array("adv_inv_sort");
                std::move(tmp.begin(), tmp.end(), adv_inv_sort.begin());
            } else {
                jo.read("adv_inv_leftsort", adv_inv_sort[left]);
                jo.read("adv_inv_rightsort", adv_inv_sort[right]);
            }
            // pane area selected
            if(jo.has_array("adv_inv_area")) {
                auto tmp = jo.get_int_array("adv_inv_area");
                std::move(tmp.begin(), tmp.end(), adv_inv_area.begin());
            } else {
                jo.read("adv_inv_leftarea", adv_inv_area[left]);
                jo.read("adv_inv_rightarea", adv_inv_area[right]);
            }
            // pane current index
            if(jo.has_array("adv_inv_index")) {
                auto tmp = jo.get_int_array("adv_inv_index");
                std::move(tmp.begin(), tmp.end(), adv_inv_index.begin());
            } else {
                jo.read("adv_inv_leftindex", adv_inv_index[left]);
                jo.read("adv_inv_rightindex", adv_inv_index[right]);
            }
            // viewing vehicle cargo
            if(jo.has_array("adv_inv_in_vehicle")) {
                JsonArray ja = jo.get_array("adv_inv_in_vehicle");
                for(size_t i = 0; ja.has_more(); ++i) {
                    adv_inv_in_vehicle[i] = ja.next_bool();
                }
            }
            // filter strings
            if(jo.has_array("adv_inv_filter")) {
                auto tmp = jo.get_string_array("adv_inv_filter");
                std::move(tmp.begin(), tmp.end(), adv_inv_filter.begin());
            } else {
                jo.read("adv_inv_leftfilter", adv_inv_filter[left]);
                jo.read("adv_inv_rightfilter", adv_inv_filter[right]);
            }
            // the rest
            jo.read("adv_inv_src", adv_inv_src);
            jo.read("adv_inv_dest", adv_inv_dest);
            jo.read("adv_inv_last_popup_dest", adv_inv_last_popup_dest);
            jo.read("adv_inv_container_location", adv_inv_container_location);
            jo.read("adv_inv_container_index", adv_inv_container_index);
            jo.read("adv_inv_container_in_vehicle", adv_inv_container_in_vehicle);
            jo.read("adv_inv_container_type", adv_inv_container_type);
            jo.read("adv_inv_container_content_type", adv_inv_container_content_type);
            jo.read("overmap_blinking", overmap_blinking);
            jo.read("overmap_show_overlays", overmap_show_overlays);
            jo.read("list_item_mon", list_item_mon);
            jo.read("list_item_sort", list_item_sort);
            jo.read("list_item_filter_active", list_item_filter_active);
            jo.read("list_item_downvote_active", list_item_downvote_active);
            jo.read("list_item_priority_active", list_item_priority_active);

            JsonObject inhist = jo.get_object("input_history");
            std::set<std::string> inhist_members = inhist.get_member_names();
            for (std::set<std::string>::iterator it = inhist_members.begin();
                 it != inhist_members.end(); ++it) {
                JsonArray ja = inhist.get_array(*it);
                std::vector<std::string>& v = gethistory(*it);
                v.clear();
                while (ja.has_more()) {
                    v.push_back(ja.next_string());
                }
            }
            // fetch list_item settings from input_history
            if ( !gethistory("item_filter").empty() ) {
                list_item_filter = gethistory("item_filter").back();
            }
            if ( !gethistory("list_item_downvote").empty() ) {
                list_item_downvote = gethistory("list_item_downvote").back();
            }
            if ( !gethistory("list_item_priority").empty() ) {
                list_item_priority = gethistory("list_item_priority").back();
            }
        };
};
extern uistatedata uistate;

#endif
