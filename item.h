#ifndef _ITEM_H_
#define _ITEM_H_

#include <string>
#include <vector>
#include "itype.h"
#include "mtype.h"
//#include "npc.h"

class player;
class npc;

class item
{
public:
 item();
 item(itype* it, unsigned int turn);
 item(itype* it, unsigned int turn, char let);
 void make_corpse(itype* it, mtype* mt, unsigned int turn);	// Corpse
 item(std::string itemdata, game *g);
 ~item();
 void make(itype* it);

// returns the default container of this item, with this item in it
 item in_its_container(std::vector<itype*> *itypes);

 nc_color color(player *u);
 nc_color color_in_inventory(player *u);
 std::string tname(game *g = NULL); // g needed for rotten-test
 void use(player &u);
 bool burn(int amount = 1); // Returns true if destroyed

// Firearm specifics
 int reload_time(player &u);
 int clip_size();
 int accuracy();
 int gun_damage(bool with_ammo = true);
 int noise();
 int burst_size();
 int recoil(bool with_ammo = true);
 int range(player *p = NULL);
 ammotype ammo_type();
 int pick_reload_ammo(player &u, bool interactive);
 bool reload(player &u, int index);

 std::string save_info();	// Formatted for save files
 void load_info(std::string data, game *g);
 std::string info(bool showtext = false);	// Formatted for human viewing
 char symbol();
 nc_color color();
 int price();

 bool invlet_is_okay();
 bool stacks_with(item rhs);
 void put_in(item payload);

 int weight();
 int volume();
 int volume_contained();
 int attack_time();
 int damage_bash();
 int damage_cut();
 bool has_flag(item_flag f);
 bool has_technique(technique_id t, player *p = NULL);
 std::vector<technique_id> techniques();
 bool goes_bad();
 bool count_by_charges();
 bool craft_has_charges();
 bool rotten(game *g);

// Our value as a weapon, given particular skills
 int  weapon_value(int skills[num_skill_types]);
// As above, but discounts its use as a ranged weapon
 int  melee_value (int skills[num_skill_types]);
// Returns the data associated with tech, if we are an it_style
 style_move style_data(technique_id tech);
 bool is_two_handed(player *u);
 bool made_of(material mat);
 bool conductive(); // Electricity
 bool destroyed_at_zero_charges();
// Most of the is_whatever() functions call the same function in our itype
 bool is_null(); // True if type is NULL, or points to the null item (id == 0)
 bool is_food(player *u);// Some non-food items are food to certain players
 bool is_food_container(player *u);  // Ditto
 bool is_food();                // Ignoring the ability to eat batteries, etc.
 bool is_food_container();      // Ignoring the ability to eat batteries, etc.
 bool is_drink();
 bool is_weap();
 bool is_bashing_weapon();
 bool is_cutting_weapon();
 bool is_gun();
 bool is_gunmod();
 bool is_bionic();
 bool is_ammo();
 bool is_armor();
 bool is_book();
 bool is_container();
 bool is_tool();
 bool is_software();
 bool is_macguffin();
 bool is_style();
 bool is_other(); // Doesn't belong in other categories
 bool is_artifact();

 int typeId();

 itype*   type;
 mtype*   corpse;
 it_ammo* curammo;

 std::vector<item> contents;

 std::string name;
 char invlet;           // Inventory letter
 int charges;
 bool active;           // If true, it has active effects to be processed
 signed char damage;    // How much damage it's sustained; generally, max is 5
 char burnt;		// How badly we're burnt
 unsigned int bday;     // The turn on which it was created
 int owned;		// UID of NPC owner; 0 = player, -1 = unowned
 int poison;		// How badly poisoned is it?

 int mission_id;// Refers to a mission in game's master list
 int player_id;	// Only give a mission to the right player!

 static itype * nullitem();

private:
 static itype * nullitem_m;
};

std::ostream & operator<<(std::ostream &, const item &);
std::ostream & operator<<(std::ostream &, const item *);

#endif
