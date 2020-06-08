#pragma once
#ifndef CATA_SRC_VEH_INTERACT_H
#define CATA_SRC_VEH_INTERACT_H

#include <cstddef>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "color.h"
#include "cursesdef.h"
#include "input.h"
#include "inventory.h"
#include "item_location.h"
#include "memory_fast.h"
#include "player_activity.h"
#include "point.h"
#include "type_id.h"

class player;
class vpart_info;
struct requirement_data;

/** Represents possible return values from the cant_do function. */
enum task_reason {
    UNKNOWN_TASK = -1, //No such task
    CAN_DO, //Task can be done
    INVALID_TARGET, //No valid target i.e. can't "change tire" if no tire present
    LACK_TOOLS, //Player doesn't have all the tools they need
    NOT_FREE, //Part is attached to something else and can't be unmounted
    LACK_SKILL, //Player doesn't have high enough mechanics skill
    MOVING_VEHICLE, // vehicle is moving, no modifications allowed
    LOW_MORALE, // Player has too low morale (for operations that require it)
    LOW_LIGHT // Player cannot see enough to work (for operations that require it)
};

class ui_adaptor;
class vehicle;
struct vehicle_part;

// For marking 'leaking' tanks/reactors/batteries
const std::string leak_marker = "<color_red>*</color>";

class veh_interact
{
        using part_selector = std::function<bool( const vehicle_part &pt )>;

    public:
        static player_activity run( vehicle &veh, const point &p );

        /** Prompt for a part matching the selector function */
        static vehicle_part &select_part( const vehicle &veh, const part_selector &sel,
                                          const std::string &title = std::string() );

        static void complete_vehicle( player &p );

    private:
        veh_interact( vehicle &veh, const point &p = point_zero );
        ~veh_interact();

        item_location target;

        point dd = point_zero;
        /* starting offset for vehicle parts description display and max offset for scrolling */
        int start_at = 0;
        int start_limit = 0;
        /* starting offset for the overview and the max offset for scrolling */
        int overview_offset = 0;
        int overview_limit = 0;

        const vpart_info *sel_vpart_info = nullptr;
        //Command currently being run by the player
        char sel_cmd = ' ';

        const vehicle_part *sel_vehicle_part = nullptr;

        int cpart = -1;
        int page_size;
        int fuel_index = 0; /** Starting index of where to start printing fuels from */
        // height of the stats window
        const int stats_h = 8;
        catacurses::window w_border;
        catacurses::window w_mode;
        catacurses::window w_msg;
        catacurses::window w_disp;
        catacurses::window w_parts;
        catacurses::window w_stats;
        catacurses::window w_list;
        catacurses::window w_details;
        catacurses::window w_name;

        weak_ptr_fast<ui_adaptor> ui;

        cata::optional<std::string> title;
        cata::optional<std::string> msg;

        int highlight_part = -1;

        struct install_info_t;
        std::unique_ptr<install_info_t> install_info;

        vehicle *veh;
        inventory crafting_inv;
        input_context main_context;

        // maximum level of available lifting equipment (if any)
        int max_lift;
        // maximum level of available jacking equipment (if any)
        int max_jack;

        shared_ptr_fast<ui_adaptor> create_or_get_ui_adaptor();

        player_activity serialize_activity();

        /** Format list of requirements returning true if all are met */
        bool format_reqs( std::string &msg, const requirement_data &reqs,
                          const std::map<skill_id, int> &skills, int moves ) const;

        int part_at( const point &d );
        void move_cursor( const point &d, int dstart_at = 0 );
        task_reason cant_do( char mode );
        bool can_potentially_install( const vpart_info &vpart );
        /** Move index (parameter pos) according to input action:
         * (up or down, single step or whole page).
         * @param pos index to change.
         * @param action input action (taken from input_context::handle_input)
         * @param size size of the list to scroll, used to wrap the cursor around.
         * @param header number of lines reserved for list header.
         * @return false if the action is not a move action, the index is not changed in this case.
         */
        bool move_in_list( int &pos, const std::string &action, int size,
                           int header = 0 ) const;
        void move_fuel_cursor( int delta );

        /**
         * @name Task handlers
         *
         * One function for each specific task
         * @warning presently functions may mutate local state
         * @param msg failure message to display (if any)
         */
        /*@{*/
        void do_install();
        void do_repair();
        void do_mend();
        void do_refill();
        void do_remove();
        void do_rename();
        void do_siphon();
        // Returns true if exiting the screen
        bool do_unload();
        void do_assign_crew();
        void do_relabel();
        /*@}*/

        void display_grid();
        void display_veh();
        void display_stats() const;
        void display_name();
        void display_mode();
        void display_list( size_t pos, const std::vector<const vpart_info *> &list, int header = 0 );
        void display_details( const vpart_info *part );
        size_t display_esc( const catacurses::window &win );

        struct part_option {
            part_option( const std::string &key, vehicle_part *part, char hotkey,
                         std::function<void( const vehicle_part &pt, const catacurses::window &w, int y )> details ) :
                key( key ), part( part ), hotkey( hotkey ), details( details ) {}

            part_option( const std::string &key, vehicle_part *part, char hotkey,
                         std::function<void( const vehicle_part &pt, const catacurses::window &w, int y )> details,
                         std::function<void( const vehicle_part &pt )> message ) :
                key( key ), part( part ), hotkey( hotkey ), details( details ), message( message ) {}

            std::string key;
            vehicle_part *part;

            /** Can @param action be run for this entry? */
            char hotkey;

            /** Writes any extra details for this entry */
            std::function<void( const vehicle_part &pt, const catacurses::window &w, int y )> details;

            /** Writes to message window when part is selected */
            std::function<void( const vehicle_part &pt )> message;
        };
        std::vector<part_option> overview_opts;
        std::map<std::string, std::function<void( const catacurses::window &, int )>> overview_headers;
        using overview_enable_t = std::function<bool( const vehicle_part &pt )>;
        using overview_action_t = std::function<void( vehicle_part &pt )>;
        overview_enable_t overview_enable;
        overview_action_t overview_action;
        int overview_pos = -1;

        void calc_overview();
        void display_overview();
        /**
         * Display overview of parts, optionally with interactive selection of one part
         *
         * @param enable used to determine parts of interest. If \p action also present, these
                         parts are the ones that can be selected. Otherwise, these are the parts
                         that will be highlighted
         * @param action callback when part is selected.
         */
        void overview( const overview_enable_t &enable = {},
                       const overview_action_t &action = {} );
        void move_overview_line( int );

        void count_durability();

        std::string total_durability_text;
        nc_color total_durability_color;

        /** Returns the most damaged part's index, or -1 if they're all healthy. */
        vehicle_part *get_most_damaged_part() const;

        /** Returns the index of the part that needs repair the most.
         * This may not be mostDamagedPart since not all parts can be repaired
         * If there are no damaged parts this returns -1 */
        vehicle_part *get_most_repariable_part() const;

        //do_remove supporting operation, writes requirements to ui
        bool can_remove_part( int idx, const player &p );
        //do install support, writes requirements to ui
        bool can_install_part();
        //true if trying to install foot crank with electric engines for example
        //writes failure to ui
        bool is_drive_conflict();

        /* Vector of all vpart TYPES that can be mounted in the current square.
         * Can be converted to a vector<vpart_info>.
         * Updated whenever the cursor moves. */
        std::vector<const vpart_info *> can_mount;

        /* Maps part names to vparts representing different shapes of a part.
         * Used to slim down installable parts list. Only built once. */
        std::map< std::string, std::vector<const vpart_info *> > vpart_shapes;

        /* Vector of all wheel types. Used for changing wheels, so it only needs
         * to be built once. */
        std::vector<const vpart_info *> wheel_types;

        /* Vector of vparts in the current square that can be repaired. Strictly a
         * subset of parts_here.
         * Can probably be removed entirely, otherwise is a vector<vehicle_part>.
         * Updated whenever parts_here is updated.
         */
        std::vector<int> need_repair;

        /* Vector of all vparts that exist on the vehicle in the current square.
         * Can be converted to a vector<vehicle_part>.
         * Updated whenever the cursor moves. */
        std::vector<int> parts_here;

        /* Refers to the wheel (if any) in the currently selected square. */
        struct vehicle_part *wheel;

        /* called by exec() */
        void cache_tool_availability();
        void allocate_windows();
        void do_main_loop();

        void cache_tool_availability_update_lifting( const tripoint &world_cursor_pos );

        /** Returns true if the vehicle has a jack powerful enough to lift itself installed */
        bool can_self_jack();
};

#endif // CATA_SRC_VEH_INTERACT_H
