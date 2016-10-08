#ifndef VEH_INTERACT_H
#define VEH_INTERACT_H

#include "inventory.h"
#include "input.h"
#include "color.h"
#include "cursesdef.h" // WINDOW
#include "string_id.h"
#include "int_id.h"
#include "requirements.h"
#include "player_activity.h"

#include <string>
#include <vector>
#include <map>
#include <sstream>

class vpart_info;
using vpart_id = int_id<vpart_info>;
using vpart_str_id = string_id<vpart_info>;

/** Represents possible return values from the cant_do function. */
enum task_reason {
    UNKNOWN_TASK = -1, //No such task
    CAN_DO, //Task can be done
    INVALID_TARGET, //No valid target ie can't "change tire" if no tire present
    LACK_TOOLS, //Player doesn't have all the tools they need
    NOT_FREE, //Part is attached to something else and can't be unmounted
    LACK_SKILL, //Player doesn't have high enough mechanics skill
    MOVING_VEHICLE, // vehicle is moving, no modifications allowed
    LOW_MORALE // Player has too low morale (for operations that require it)
};

class vehicle;
struct vehicle_part;

class veh_interact
{
    public:
        static player_activity run( vehicle &veh, int x, int y );

        /** Prompt for a suitable tank that can be used to contain @param liquid */
        static item_location select_tank( const vehicle &veh, const item &liquid );

        static void complete_vehicle();

    private:
        veh_interact( vehicle &veh, int x = 0, int y = 0 );
        ~veh_interact();

        item_location target;

        int ddx = 0;
        int ddy = 0;
        const vpart_info *sel_vpart_info = nullptr;
        char sel_cmd = ' '; //Command currently being run by the player

        const vehicle_part *sel_vehicle_part = nullptr;

        int cpart = -1;
        int page_size;
        int fuel_index = 0; /** Starting index of where to start printing fuels from */
        WINDOW *w_grid;
        WINDOW *w_mode;
        WINDOW *w_msg;
        WINDOW *w_disp;
        WINDOW *w_parts;
        WINDOW *w_stats;
        WINDOW *w_list;
        WINDOW *w_details;
        WINDOW *w_name;

        vehicle *veh;
        bool has_wrench;
        bool has_jack;
        bool has_wheel;
        inventory crafting_inv;
        input_context main_context;

        int max_lift; // maximum level of available lifting equipment (if any)
        int max_jack; // maximum level of available jacking equipment (if any)

        player_activity serialize_activity();

        void set_title( std::string msg, ... ) const;

        /** Format list of requirements returning true if all are met */
        bool format_reqs( std::ostringstream &msg, const requirement_data &reqs,
                          const std::map<skill_id, int> &skills, int moves ) const;

        int part_at( int dx, int dy );
        void move_cursor( int dx, int dy );
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
        bool move_in_list( int &pos, const std::string &action, const int size,
                           const int header = 0 ) const;
        void move_fuel_cursor( int delta );

        void do_install();
        void do_repair();
        void do_mend();
        void do_refill();
        void do_remove();
        void do_rename();
        void do_siphon();
        void do_tirechange();
        void do_relabel();

        void display_grid();
        void display_veh();
        void display_stats();
        void display_name();
        void display_mode();
        void display_list( size_t pos, std::vector<const vpart_info *> list, const int header = 0 );
        void display_details( const vpart_info *part );
        size_t display_esc( WINDOW *w );

        /**
          * Display overview of parts
          * @param enable used to determine if a part can be selected
          * @param action callback run when a part is selected
          */
        void overview( std::function<bool( const vehicle_part &pt )> enable = {},
                       std::function<void( const vehicle_part &pt )> action = {} );

        void countDurability();

        std::string totalDurabilityText;
        nc_color totalDurabilityColor;

        /** Store the most damaged part's index, or -1 if they're all healthy. */
        int mostDamagedPart = -1;

        //do_remove supporting operation, writes requirements to ui
        bool can_remove_part( int idx );
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
        void deallocate_windows();
};

#endif
