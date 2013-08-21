#ifndef _VEH_INTERACT_H_
#define _VEH_INTERACT_H_

#include <vector>
#include "output.h"
#include "inventory.h"

enum sel_types {
  SEL_NULL, SEL_JACK
};

class vehicle;
class game;

class veh_interact
{
public:
    int cx;
    int cy;
    int ddx;
    int ddy;
    int sel_part;
    char sel_cmd;
    int sel_type;
private:
    int cpart;
    int page_size;
    WINDOW *w_grid;
    WINDOW *w_mode;
    WINDOW *w_msg;
    WINDOW *w_disp;
    WINDOW *w_parts;
    WINDOW *w_stats;
    WINDOW *w_list;

    int winw1;
    int winw2;
    int winh1;
    int winh2;
    int winw12;
    int winw3;
    int winh3;
    int winh23;
    int winx1;
    int winx2;
    int winy1;
    int winy2;

    vehicle *veh;
    game *g;
    int ex, ey;
    bool has_wrench;
    bool has_welder;
    bool has_hacksaw;
    bool has_jack;
    bool has_siphon;
    bool has_wheel;
    inventory crafting_inv;

    int part_at (int dx, int dy);
    void move_cursor (int dx, int dy);
    int cant_do (char mode);

    void do_install(int reason);
    void do_repair(int reason);
    void do_refill(int reason);
    void do_remove(int reason);
    void do_rename(int reason);
    void do_siphon(int reason);
    void do_tirechange(int reason);
    void do_drain(int reason);

    void display_veh ();
    void display_stats ();
    void display_mode (char mode);
    void display_list (int pos);

    std::vector<int> can_mount;
    std::vector<bool> has_mats;
    std::vector<int> need_repair;
    std::vector<int> parts_here;
    int ptank;
    int wheel;
    bool has_fuel;
public:
    veh_interact ();
    void exec (game *gm, vehicle *v, int x, int y);
};

void complete_vehicle (game *g);

#endif
