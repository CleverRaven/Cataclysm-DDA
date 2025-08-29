#pragma once
#ifndef CATA_SRC_VEH_APPLIANCE_H
#define CATA_SRC_VEH_APPLIANCE_H

#include <functional>
#include <optional>
#include <vector>

#include "coordinates.h"
#include "cursesdef.h"
#include "input_context.h"
#include "item.h"
#include "memory_fast.h"
#include "player_activity.h"
#include "point.h"
#include "type_id.h"
#include "uilist.h"

class Character;
class map;
class ui_adaptor;
class vehicle;

vpart_id vpart_appliance_from_item( const itype_id &item_id );
bool place_appliance( map &here, const tripoint_bub_ms &p, const vpart_id &vpart,
                      const Character &owner, const std::optional<item> &base = std::nullopt );

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
        static player_activity run( map &here, vehicle &veh, const point_rel_ms &p = point_rel_ms::zero );

    private:
        explicit veh_app_interact( vehicle &veh, const point_rel_ms &p );
        ~veh_app_interact() = default;

        // The player's point of interaction on the appliance.
        point_rel_ms a_point = point_rel_ms::zero;
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
        void init_ui_windows( map &here );
        /**
         * Draws the upper portion, representing the appliance's usage info.
        */
        void draw_info( map &here );
        /**
         * Populates the list of available actions for this appliance. The action display
         * names are stored as uilist entries in imenu, and the associated functions
         * are stored in app_actions.
        */
        void populate_app_actions( map &here );
        /**
         * Initializes the ui_adaptor and callbacks responsible for drawing w_border and w_info.
        */
        shared_ptr_fast<ui_adaptor> create_or_get_ui_adaptor( map &here );
        /**
         * Checks whether the current appliance can be refilled (excludes batteries).
         * @returns True if the appliance contains a part that can be refilled.
        */
        bool can_refill( );
        /**
         * Checks whether the player can siphon liquids from the appliance.
         * Requires an item with HOSE quality and a part containing liquid.
         * @returns True if a liquid can be siphoned from the appliance.
        */
        bool can_siphon();
        /**
         * Function associated with the "REFILL" action.
         * Checks all appliance parts for a watertight container to refill. If multiple
         * parts are eligible, the player is prompted to select one. A refill activity
         * is created and assigned to 'act' to be assigned to the player once run returns.
        */
        void refill( );
        /**
         * Function associated with the "SIPHON" action.
         * Checks all appliance parts for a liquid containing part that can be siphoned.
         * If multiple parts are eligible, the player is prompted to select one.
         * A liquid handling activity is assigned to the player to process the transfer.
        */
        void siphon( map &here );
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
        void remove( map &here );
        /**
         * Checks whether the part has any items linked to it so it can tell the player
         * to disconnect those first. This prevents players from doing this by accident.
         * @returns True if there aren't any tow cable parts or items linked to the mount point.
        */
        bool can_disconnect();
        /**
         * Function associated with the "DISCONNECT_GRID" action.
         * Removes appliance from a power grid, allowing it to be moved individually.
        */
        void disconnect( map &here );
        /**
        * Function associated with the "PLUG" action.
        * Connects the power cable to selected tile.
        */
        void plug( map &here );
        /**
        * Function associated with the "HIDE" action.
        * Hides the selected tiles sprite.
        */
        void hide();
        /**
         * The main loop of the appliance UI. Redraws windows, checks for input, and
         * performs selected actions. The loop exits once an activity is assigned
         * (either directly to the player or to 'act'), or when QUIT input is received.
        */
        void app_loop( map &here );
};

#endif // CATA_SRC_VEH_APPLIANCE_H
