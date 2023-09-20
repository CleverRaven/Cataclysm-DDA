#pragma once
#ifndef CATA_SRC_ADVANCED_INV_H
#define CATA_SRC_ADVANCED_INV_H

#include <array>
#include <iosfwd>

#include "activity_actor_definitions.h"
#include "advanced_inv_area.h"
#include "advanced_inv_pane.h"
#include "cursesdef.h"
#include "string_input_popup.h"
#include "ui_manager.h"

class advanced_inv_listitem;
class input_context;
class item;
struct advanced_inv_save_state;

void create_advanced_inv();

/**
 * Cancels ongoing move all action.
 * TODO: Make this not needed.
 */
void cancel_aim_processing();

class advanced_inventory
{
    public:
        advanced_inventory();
        ~advanced_inventory();

        void display();

        /**
         * Converts from screen relative location to game-space relative location
         * for control rotation in isometric mode.
        */
        aim_location screen_relative_location( aim_location area );
        std::string get_location_key( aim_location area );

        advanced_inv_area &get_one_square( const aim_location &loc ) {
            return squares[loc];
        }
    private:
        /**
         * Refers to the two panes, used as index into @ref panes.
         */
        enum side {
            left  = 0,
            right = 1,
            NUM_PANES = 2
        };
        static constexpr int head_height = 5;
        bool move_all_items_and_waiting_to_quit = false;

        std::unique_ptr<ui_adaptor> ui;
        std::unique_ptr<string_input_popup> spopup;

        // swap the panes and windows via std::swap()
        void swap_panes();

        // minimap that displays things around character
        catacurses::window minimap;
        catacurses::window mm_border;
        const int minimap_width  = 3;
        const int minimap_height = 3;
        void draw_minimap();
        void refresh_minimap();
        char get_minimap_sym( side p ) const;

        bool inCategoryMode = false;

        int linesPerPage = 0;
        int w_height = 0;
        int w_width = 0;

        int headstart = 0;
        int colstart = 0;

        bool recalc = false;
        bool always_recalc = false;
        /**
         * Which panels is active (item moved from there).
         */
        side src;
        /**
         * Which panel is the destination (items want to go to there).
         */
        side dest;
        /**
         * True if (and only if) the filter of the active panel is currently
         * being edited.
         */
        bool filter_edit = false;
        /**
         * Two panels (left and right) showing the items, use a value of @ref side
         * as index.
         */
        std::array<advanced_inventory_pane, NUM_PANES> panes;
        static const advanced_inventory_pane null_pane;
        std::array<advanced_inv_area, NUM_AIM_LOCATIONS> squares;

        catacurses::window head;

        bool exit = false;

        advanced_inv_save_state *save_state;

        /**
         * registers all the ctxt for display()
         */
        input_context register_ctxt() const;
        /**
         *  a smaller chunk of display()
         */
        void start_activity( aim_location destarea, aim_location srcarea,
                             advanced_inv_listitem *sitem, int &amount_to_move,
                             bool from_vehicle, bool to_vehicle ) const;

        /**
         * returns whether the display loop exits or not
         */
        bool action_move_item( advanced_inv_listitem *sitem,
                               advanced_inventory_pane &dpane, const advanced_inventory_pane &spane,
                               const std::string &action );

        void action_examine( advanced_inv_listitem *sitem, advanced_inventory_pane &spane );

        // store/load settings (such as index, filter, etc)
        void save_settings( bool only_panes );
        void load_settings();
        // Adds an auto-resumed activity that reopens AIM. If this is called
        // before assigning an item-moving activity, AIM is reopened when the
        // item-moving activity finishes. This function should only be called
        // when AIM is going to be automatically closed due to pending item-moving
        // activity, otherwise the player will need to close AIM multiple times.
        void do_return_entry();
        // returns true if currently processing a routine
        // (such as `MOVE_ALL_ITEMS' with `AIM_ALL' source)
        bool is_processing() const;

        static std::string get_sortname( advanced_inv_sortby sortby );
        void print_items( side p, bool active );
        void recalc_pane( side p );
        void redraw_pane( side p );
        void redraw_sidebar();

        bool move_all_items();
        /**
        * Fills drop_or_stash_item_info lists with the current contents of the active pane, for use with move_all_items.
        * @param player_character Reference to the player character.
        * @param spane Reference to the active AIM pane.
        * @param item_list Reference to the list to fill with items.
        * @param fav_list Reference to the list to fill with favorited items.
        * @param filter_buckets Whether to skip over containers that would spill in certain locations.
        * @return a string description of why some items were skipped, if any.
        */
        std::string fill_lists_with_pane_items( side &p, Character &player_character,
                                                std::vector<drop_or_stash_item_info> &item_list,
                                                std::vector<drop_or_stash_item_info> &fav_list, bool filter_buckets );
        // Returns the x coordinate where the header started. The header is
        // displayed right of it, everything left of it is till free.
        int print_header( advanced_inventory_pane &pane, aim_location sel );
        void init();
        /**
         * Translate an action ident from the input context to an aim_location.
         * @param action Action ident to translate
         * @param ret If the action ident referred to a location, its id is stored
         * here. Only valid when the function returns true.
         * @return true if the action did refer to an location (which has been
         * stored in ret), false otherwise.
         */
        bool get_square( const std::string &action, aim_location &ret );
        void change_square( aim_location changeSquare, advanced_inventory_pane &dpane,
                            advanced_inventory_pane &spane );
        /**
         * Show the sort-by menu and change the sorting of this pane accordingly.
         * @return whether the sort order was actually changed.
         */
        bool show_sort_menu( advanced_inventory_pane &pane );
        /**
         * Checks whether one can put items into the supplied location.
         * If the supplied location is AIM_ALL, query for the actual location
         * (stores the result in def) and check that destination.
         * @return false if one can not put items in the destination, true otherwise.
         * The result true also indicates the def is not AIM_ALL (because the
         * actual location has been queried).
         */
        bool query_destination( aim_location &def );
        /**
         * Setup how many items/charges (if counted by charges) should be moved.
         * @param destarea Where to move to. This must not be AIM_ALL.
         * @param sitem The source item, it must contain a valid reference to an item!
         * @param action The action we are querying
         * @param amount The input value is ignored, contains the amount that should
         *      be moved. Only valid if this returns true.
         * @return false if nothing should/can be moved. True only if there can and
         *      should be moved. A return value of true indicates that amount now contains
         *      a valid item count to be moved.
         */
        bool query_charges( aim_location destarea, const advanced_inv_listitem &sitem,
                            const std::string &action, int &amount );
};

#endif // CATA_SRC_ADVANCED_INV_H
