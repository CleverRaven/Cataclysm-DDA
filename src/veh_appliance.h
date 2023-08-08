#pragma once
#ifndef CATA_SRC_VEH_APPLIANCE_H
#define CATA_SRC_VEH_APPLIANCE_H

#include "input.h"
#include "player_activity.h"

class vehicle;
class ui_adaptor;
struct point;

vpart_id vpart_appliance_from_item( const itype_id &item_id );
void place_appliance( const tripoint &p, const vpart_id &vpart,
                      const std::optional<item> &base = std::nullopt );

/**
 * Appliance interaction UI. Works similarly to veh_interact, but has
 * a much simpler design. The intended usage is:
 *
 *   player_activity act = veh_app_interact::run( veh );
 *   if( act ) {
 *       player_character.moves = 0;
 *       player_character.assign_activity( act );
 *   }
 *
 * The UI contains 2 parts: an upper portion showing usage info about
 * the appliance, and a lower portion listing actions that can be
 * performed on the appliance.
*/
class veh_app_interact
{
    public:
        /**
         * Starts and manages the appliance interaction UI.
         *
         * @param veh The vehicle representing the appliance
         * @param p The point of interaction on the vehicle (Default = (0,0))
         * @returns An activity to assign to the player (ACT_VEHICLE),
         * or a null activity if no further action is required.
        */
        static player_activity run( vehicle &veh, const point &p = point_zero );

    private:
        explicit veh_app_interact( vehicle &veh, const point &p );
        ~veh_app_interact() = default;

        // The player's point of interaction on the appliance.
        point a_point = point_zero;
        // The vehicle representing the appliance.
        vehicle *veh;
        // An input context to contain registered actions from the APP_INTERACT category.
        // Input is not handled by this context, instead the registered actions are passed
        // to uilist entries in imenu.
        input_context ctxt;
        weak_ptr_fast<ui_adaptor> ui;
        // Activity to be returned from run(), or a null activity if
        // no further action is required.
        player_activity act;
        // Functions corresponding to the actions listed in imenu.entries.
        std::vector<std::function<void()>> app_actions;
        // Curses window to represent the whole drawing area of the interaction UI.
        catacurses::window w_border;
        // Curses window to represent the upper portion that displays
        // the appliance's usage info.
        catacurses::window w_info;
        // Input menu in the lower portion. Handles input and draws the list
        // of possible actions. The menu's internal window is modified by
        // init_ui_windows() to integrate with the rest of the UI.
        uilist imenu;

        /**
         * Sets up the window parameters to fit the amount of text to be displayed.
         * The setup calls populate_app_actions() to initialize the content.
        */
        void init_ui_windows();
        /**
         * Draws the upper portion, representing the appliance's usage info.
        */
        void draw_info();
        /**
         * Populates the list of available actions for this appliance. The action display
         * names are stored as uilist entries in imenu, and the associated functions
         * are stored in app_actions.
        */
        void populate_app_actions();
        /**
         * Initializes the ui_adaptor and callbacks responsible for drawing w_border and w_info.
        */
        shared_ptr_fast<ui_adaptor> create_or_get_ui_adaptor();
        /**
         * Checks whether the current appliance can be refilled (excludes batteries).
         * @returns True if the appliance contains a part that can be refilled.
        */
        bool can_refill();
        /**
         * Checks whether the player can siphon liquids from the appliance.
         * Requires an item with HOSE quality and a part containing liquid.
         * @returns True if a liquid can be siphoned from the appliance.
        */
        bool can_siphon();
        /**
         * Checks whether the current appliance has any power connections that
         * can be disconnected by the player.
         * @returns True if the appliance can be unplugged.
        */
        bool can_unplug();
        /**
         * Function associated with the "REFILL" action.
         * Checks all appliance parts for a watertight container to refill. If multiple
         * parts are eligible, the player is prompted to select one. A refill activity
         * is created and assigned to 'act' to be assigned to the player once run returns.
        */
        void refill();
        /**
         * Function associated with the "SIPHON" action.
         * Checks all appliance parts for a liquid containing part that can be siphoned.
         * If multiple parts are eligible, the player is prompted to select one.
         * A liquid handling activity is assigned to the player to process the transfer.
        */
        void siphon();
        /**
         * Function associated with the "RENAME" action.
         * Prompts the player to choose a new name for this appliance. The name is
         * reflected in the UI's title.
        */
        void rename();
        /**
         * Function associated with the "REMOVE" action.
         * Turns the installed appliance into its base item.
        */
        void remove();
        /**
        * Function associated with the "PLUG" action.
        * Connects the power cable to selected tile.
        */
        void plug();
        /**
         * The main loop of the appliance UI. Redraws windows, checks for input, and
         * performs selected actions. The loop exits once an activity is assigned
         * (either directly to the player or to 'act'), or when QUIT input is received.
        */
        void app_loop();
};

#endif // CATA_SRC_VEH_APPLIANCE_H
