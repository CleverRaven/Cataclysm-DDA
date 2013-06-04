#include <vector>
#include "game.h"

void game::init_traps()
{
int id = -1;
// Arguments for new trap (in order):
// id, Trap_Key, Name, Symbol, Color, Visibility, Avoidance,
// Difficulty, Action on Player, Action on Monster, Keys for Items to be dropped
id++;
std::vector<std::string> keys;
traps.push_back(new trap(id, "NONE", "none", c_white, '?',  20, 0,
    0, &trapfunc::none, &trapfuncm::none, keys));;

keys.clear();
id++;
keys.push_back("bubblewrap");
traps.push_back(new trap(id, "BUBBLEWRAP", "bubblewrap", c_ltcyan, '_',  0, 8,
    0, &trapfunc::bubble, &trapfuncm::bubble, keys));;

keys.clear();
id++;
keys.push_back("cot");
traps.push_back(new trap(id, "COT", "cot", c_green, '#',  0, 0,
    0, &trapfunc::none, &trapfuncm::cot, keys));;

keys.clear();
id++;
keys.push_back("brazier");
traps.push_back(new trap(id, "BRAZIER", "brazier", c_red, '#',  0, 0,
    0, &trapfunc::none, &trapfuncm::none, keys));;

keys.clear();
id++;
keys.push_back("funnel");
traps.push_back(new trap(id, "FUNNEL", "funnel", c_yellow, 'V',  0, 0,
    0, &trapfunc::none, &trapfuncm::none, keys));;

keys.clear();
id++;
keys.push_back("rollmat");
traps.push_back(new trap(id, "ROLLMAT", "roll mat", c_blue, '#',  0, 0,
    0, &trapfunc::none, &trapfuncm::none, keys));;

keys.clear();
id++;
keys.push_back("beartrap");
traps.push_back(new trap(id, "BEARTRAP", "bear trap", c_blue, '^',  2, 7,
    3, &trapfunc::beartrap, &trapfuncm::beartrap, keys));;

keys.clear();
id++;
keys.push_back("beartrap");
traps.push_back(new trap(id, "BEARTRAP_BURIED", "buried bear trap", c_blue, '^',  9, 8,
    4, &trapfunc::beartrap, &trapfuncm::beartrap, keys));;

// Name Symbol Color Vis Avd Diff

keys.clear();
id++;
keys.push_back("stick");
keys.push_back("string_36");
traps.push_back(new trap(id, "SNARE", "rabbit snare", c_brown, '\\', 5, 10,
    2, &trapfunc::snare, &trapfuncm::snare, keys));;

keys.clear();
id++;
keys.push_back("board_trap");
traps.push_back(new trap(id, "NAILBOARD", "spiked board", c_ltgray, '_',  1, 6,
    0, &trapfunc::board, &trapfuncm::board, keys));;

keys.clear();
id++;
keys.push_back("string_36");
traps.push_back(new trap(id, "TRIPWIRE", "tripwire", c_ltred, '^',  6, 4,
    3, &trapfunc::tripwire, &trapfuncm::tripwire, keys));;

keys.clear();
id++;
keys.push_back("string_36");
keys.push_back("crossbow");
keys.push_back("bolt_steel");
traps.push_back(new trap(id, "CROSSBOW_TRAP", "crossbow trap", c_green, '^',  5, 4,
    5, &trapfunc::crossbow, &trapfuncm::crossbow, keys));;

keys.clear();
id++;
keys.push_back("string_36");
keys.push_back("shotgun_sawn");
traps.push_back(new trap(id, "SHOTGUN_2", "shotgun trap", c_red, '^',  4, 5,
    6, &trapfunc::shotgun, &trapfuncm::shotgun, keys));;

keys.clear();
id++;
keys.push_back("string_36");
keys.push_back("shotgun_sawn");
traps.push_back(new trap(id, "SHOTGUN_1", "shotgun trap", c_red, '^',  4, 5,
    6, &trapfunc::shotgun, &trapfuncm::shotgun, keys));;

keys.clear();
id++;
keys.push_back("motor");
keys.push_back("blade");
keys.push_back("blade");
traps.push_back(new trap(id, "ENGINE", "spinning blade engine", c_ltred, '_',  0, 0,
    2, &trapfunc::none, &trapfuncm::none, keys));;

keys.clear();
id++;
keys.push_back("null");
traps.push_back(new trap(id, "BLADE", "spinning blade", c_cyan, '\\', 0, 4,
    99, &trapfunc::blade, &trapfuncm::blade, keys));;

keys.clear();
id++;
keys.push_back("light_snare_kit");
traps.push_back(new trap(id, "LIGHT_SNARE", "light snare trap", c_brown, '^', 5, 10,
    2, &trapfunc::snare_light, &trapfuncm::snare_light, keys));


keys.clear();
id++;
keys.push_back("heavy_snare_kit");
traps.push_back(new trap(id, "HEAVY_SNARE", "heavy snare trap", c_brown, '^', 3, 10,
    4, &trapfunc::snare_heavy, &trapfuncm::snare_heavy, keys));

keys.clear();
id++;
keys.push_back("landmine");
traps.push_back(new trap(id, "LANDMINE", "land mine", c_red, '^',  1, 14,
    10, &trapfunc::landmine, &trapfuncm::landmine, keys));;

keys.clear();
id++;
keys.push_back("landmine");
traps.push_back(new trap(id, "LANDMINE_BURIED", "buried land mine", c_red, '_',  10, 14,
    10, &trapfunc::landmine, &trapfuncm::landmine, keys));;

keys.clear();
id++;
keys.push_back("null"); //WHY????? Why does this drop a null? T_T
traps.push_back(new trap(id, "TELEPAD", "teleport pad", c_magenta, '_',  0, 15,
    20, &trapfunc::telepad, &trapfuncm::telepad, keys));;

keys.clear();
id++;
keys.push_back("null"); //Still why????
traps.push_back(new trap(id, "GOO", "goo pit", c_dkgray, '_',  0, 15,
    15, &trapfunc::goo, &trapfuncm::goo, keys));;

keys.clear();
id++;
keys.push_back("null"); //Whatever, I don't even care.
traps.push_back(new trap(id, "DISSECTOR", "dissector", c_cyan, '7',  2, 20,
    99, &trapfunc::dissector, &trapfuncm::dissector, keys));;

keys.clear();
id++;
keys.push_back("null"); //...
traps.push_back(new trap(id, "SINKHOLE", "sinkhole", c_brown, '_',  10, 14,
    99, &trapfunc::sinkhole, &trapfuncm::sinkhole, keys));;

keys.clear();
id++;
keys.push_back("null"); //T_T
traps.push_back(new trap(id, "PIT", "pit", c_brown, '0',  0, 8,
    99, &trapfunc::pit, &trapfuncm::pit, keys));;

keys.clear();
id++;
keys.push_back("null"); //o_o
traps.push_back(new trap(id, "SPIKE_PIT", "spiked pit", c_blue, '0',  0, 8,
    99, &trapfunc::pit_spikes, &trapfuncm::pit_spikes, keys));;

keys.clear();
id++;
keys.push_back("null");
traps.push_back(new trap(id, "", "lava", c_red, '~',  0, 99,
    99, &trapfunc::lava, &trapfuncm::lava, keys));;

keys.clear();
id++;
keys.push_back("null");
// The '%' symbol makes the portal cycle through ~*0&
traps.push_back(new trap(id, "", "shimmering portal", c_magenta, '%',  0, 30,
    99, &trapfunc::portal, &trapfuncm::portal, keys));;

keys.clear();
id++;
keys.push_back("null");
traps.push_back(new trap(id, "", "ledge", c_black, ' ',  0, 99,
    99, &trapfunc::ledge, &trapfuncm::ledge, keys));;

keys.clear();
id++;
keys.push_back("null");
traps.push_back(new trap(id, "", "boobytrap", c_ltcyan, '^',  5, 4,
    7, &trapfunc::boobytrap, &trapfuncm::boobytrap, keys));;

keys.clear();
id++;
keys.push_back("null");
traps.push_back(new trap(id, "", "raised tile", c_ltgray, '^',  9, 20,
    99, &trapfunc::temple_flood,&trapfuncm::none, keys));;

keys.clear();
id++;
keys.push_back("null");
// Toggles through states of RGB walls
traps.push_back(new trap(id, "", "", c_white, '^',  99, 99,
    99, &trapfunc::temple_toggle, &trapfuncm::none, keys));;

keys.clear();
id++;
keys.push_back("null");
// Glow attack
traps.push_back(new trap(id, "", "", c_white, '^',  99, 99,
    99, &trapfunc::glow, &trapfuncm::glow, keys));;

keys.clear();
id++;
keys.push_back("null");
// Hum attack
traps.push_back(new trap(id, "", "", c_white, '^',  99, 99,
    99, &trapfunc::hum, &trapfuncm::hum, keys));;

keys.clear();
id++;
keys.push_back("null");
// Shadow spawn
traps.push_back(new trap(id, "", "", c_white, '^',  99, 99,
    99, &trapfunc::shadow, &trapfuncm::none, keys));;

keys.clear();
id++;
keys.push_back("null");
// Drain attack
traps.push_back(new trap(id, "", "", c_white, '^',  99, 99,
    99, &trapfunc::drain, &trapfuncm::drain, keys));;

keys.clear();
id++;
keys.push_back("null");
// Snake spawn / hisssss
traps.push_back(new trap(id, "", "", c_white, '^',  99, 99,
    99, &trapfunc::snake, &trapfuncm::snake, keys));;
}
