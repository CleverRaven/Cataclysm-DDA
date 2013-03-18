#ifndef _ITEM_H_
#define _ITEM_H_

#include <string>
#include <vector>
#include "itype.h"
#include "mtype.h"
//#include "npc.h"

class player;
class npc;

struct iteminfo{
 public:
  std::string sType; //Itemtype
  std::string sName; //Main item text
  std::string sPre; //Text between main item and value
  int iValue; //Set to -999 if no compare value is present
  std::string sPost; //Text after value
  bool bNewLine; //New line at the end
  bool bLowerIsBetter; //Lower values are better (red <-> green)

  iteminfo(std::string sIn0, std::string sIn1, std::string sIn2 = "", int iIn0 = -999, std::string sIn3 = "", bool bIn0 = true, bool bIn1 = false) {
   sType = sIn0;
   sName = sIn1;
   sPre = sIn2;
   iValue = iIn0;
   sPost = sIn3;
   bNewLine = bIn0;
   bLowerIsBetter = bIn1;
  }
};

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
 void next_mode();

 std::string save_info();	// Formatted for save files
 void load_info(std::string data, game *g);
 //std::string info(bool showtext = false);	// Formatted for human viewing
 std::string info(bool showtext = false);
 std::string info(bool showtext, std::vector<iteminfo> *dump);
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
 int has_gunmod(int type);
 item* active_gunmod();
 std::vector<technique_id> techniques();
 bool goes_bad();
 bool count_by_charges();
 bool craft_has_charges();
 int num_charges();
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
 bool is_ammo_container();
 bool is_drink();
 bool is_weap();
 bool is_bashing_weapon();
 bool is_cutting_weapon();
 bool is_gun();
 bool is_silent();
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
 bool is_var_veh_part();
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
 char burnt;	         // How badly we're burnt
 unsigned int bday;     // The turn on which it was created
 int owned;	            // UID of NPC owner; 0 = player, -1 = unowned
 union{
   int poison;	         // How badly poisoned is it?
   int bigness;         // engine power, wheel size
 };
 int mode;              // Mode of operation, can be changed by the player.

 int mission_id;// Refers to a mission in game's master list
 int player_id;	// Only give a mission to the right player!

 static itype * nullitem();

private:
 static itype * nullitem_m;
};

std::ostream & operator<<(std::ostream &, const item &);
std::ostream & operator<<(std::ostream &, const item *);

#endif
