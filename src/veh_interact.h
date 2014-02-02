#ifndef _VEH_INTERACT_H_
#define _VEH_INTERACT_H_

#include <vector>
#include "output.h"
#include "inventory.h"

#define DUCT_TAPE_USED 100
#define CIRC_SAW_USED 20

enum sel_types {
  SEL_NULL, SEL_JACK
};

/** Represents possible return values from the cant_do function. */
enum task_reason {
    UNKNOWN_TASK = -1, //No such task
    CAN_DO, //Task can be done
    CANT_REFILL, // All fuel tanks are broken or player don't have properly fuel
    INVALID_TARGET, //No valid target ie can't "change tire" if no tire present
    LACK_TOOLS, //Player doesn't have all the tools they need
    NOT_FREE, //Part is attached to something else and can't be unmounted
    LACK_SKILL, //Player doesn't have high enough mechanics skill
    LOW_MORALE // Player has too low morale (for operations that require it)
};

class vehicle;

class veh_interact
{
public:
    int ddx;
    int ddy;
    struct vpart_info *sel_vpart_info;
    struct vehicle_part *sel_vehicle_part;
    char sel_cmd; //Command currently being run by the player
    int sel_type;
private:
    int cpart;
    int page_size;
    bool vertical_menu;
    WINDOW *w_grid;
    WINDOW *w_mode;
    WINDOW *w_msg;
    WINDOW *w_disp;
    WINDOW *w_parts;
    WINDOW *w_stats;
    WINDOW *w_list;
    WINDOW *w_name;

    int mode_h;
    int mode_w;
    int msg_h;
    int msg_w;
    int disp_h;
    int disp_w;
    int parts_h;
    int parts_w;
    int stats_h;
    int stats_w;
    int list_h;
    int list_w;
    int name_h;
    int name_w;

    vehicle *veh;
    bool has_wrench;
    bool has_welder;
    bool has_goggles;
    bool has_duct_tape;
    bool has_hacksaw;
    bool has_jack;
    bool has_siphon;
    bool has_wheel;
    inventory crafting_inv;

    int part_at(int dx, int dy);
    void move_cursor(int dx, int dy);
    task_reason cant_do(char mode);

    void do_install(task_reason reason);
    void do_repair(task_reason reason);
    void do_refill(task_reason reason);
    void do_remove(task_reason reason);
    void do_rename(task_reason reason);
    void do_siphon(task_reason reason);
    void do_tirechange(task_reason reason);
    void do_drain(task_reason reason);

    void display_grid();
    void display_veh();
    void display_stats();
    void display_name();
    void display_mode(char mode);
    void display_list(int pos, std::vector<vpart_info> list);
    size_t display_esc (WINDOW *w);

    void countDurability();
    nc_color getDurabilityColor(const int& dur);
    std::string getDurabilityDescription(const int& dur);

    int durabilityPercent;
    std::string totalDurabilityText;
    std::string worstDurabilityText;
    nc_color totalDurabilityColor;
    nc_color worstDurabilityColor;

    /** Store the most damaged part's index, or -1 if they're all healthy. */
    int mostDamagedPart;

    /* true if current selected square has part with "FUEL_TANK flag and 
     * they are not full. Otherwise will be false.
     */
    bool has_ptank;

    /* Vector of all vpart TYPES that can be mounted in the current square.
     * Can be converted to a vector<vpart_info>.
     * Updated whenever the cursor moves. */
    std::vector<vpart_info> can_mount;

    /* Vector of all wheel types. Used for changing wheels, so it only needs
     * to be built once. */
    std::vector<vpart_info> wheel_types;

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

    /* Refers to the fuel tanks (if any) in the currently selected square. */
    std::vector<vehicle_part*> ptanks;

    /* Refers to the wheel (if any) in the currently selected square. */
    struct vehicle_part *wheel;

    /* called by exec() */
    void cache_tool_availability();
    void allocate_windows();
    void do_main_loop();
    void deallocate_windows();

public:
    veh_interact ();
    void exec(vehicle *v);
};

void complete_vehicle ();

#endif
