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

 std::string tname();
 void use(player &u);

// Firearm specifics
 int reload_time(player &u);
 int clip_size();
 int accuracy();
 int gun_damage();
 int noise();
 int burst_size();
 int recoil();
 ammotype ammo();
 int pick_reload_ammo(player &u, bool interactive);
 bool reload(player &u, int index);

 std::string save_info();	// Formatted for save files
 void load_info(std::string data, game *g);
 std::string info(bool showtext = false);	// Formatted for human viewing
 char symbol();
 nc_color color();
 int price();

 void put_in(item payload);

 int weight();
 int volume();
 int volume_contained();
 int attack_time();

 int  weapon_value(int skills[num_skill_types]);
 bool is_two_handed(player *u);
 bool made_of(material mat);
 bool conductive();
 bool is_food(player *u);// Some non-food items are food to certain players
 bool is_food_container(player *u);	// Ditto
 bool is_food();		// Ignoring the ability to eat batteries, etc.
 bool is_food_container();	// Ignoring the ability to eat batteries, etc.
 bool is_drink();
 bool is_weap();
 bool is_bashing_weapon();
 bool is_cutting_weapon();
 bool is_gun();
 bool is_gunmod();;
 bool is_ammo();
 bool is_armor();
 bool is_book();
 bool is_container();
 bool is_tool();
 virtual bool is_artifact()  { return false; }

 itype* type;
 mtype* corpse;
 it_ammo* curammo;

 std::vector<item> contents;

 std::string name;
 char invlet;
 int charges;
 bool active;		// If true, it has active effects to be processed
 signed char damage;	// How much damage it's sustained; generally, max is 5
 unsigned int bday;	// The turn on which it was created
 bool owned;		// If true, NPCs will shout at you for picking it up
};


#endif
