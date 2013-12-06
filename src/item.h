#ifndef _ITEM_H_
#define _ITEM_H_

#include <string>
#include <vector>
#include "itype.h"
#include "mtype.h"

class game;
class player;
class npc;
struct itype;

// Thresholds for radiation dosage for the radiation film badge.
const int rad_dosage_thresholds[] = { 0, 30, 60, 120, 240, 500};
const std::string rad_threshold_colors[] = { "green", "blue", "yellow", "orange", "red", "black"};

struct light_emission {
  unsigned short luminance;
  short width;
  short direction;
};
extern light_emission nolight;

struct iteminfo{
 public:
  std::string sType; //Itemtype
  std::string sName; //Main item text
  std::string sFmt; //Text between main item and value
  std::string sValue; //Set to "-999" if no compare value is present
  double dValue; //Stores double value of sValue for value comparisons
  bool is_int; //Sets if sValue should be treated as int or single decimal double
  std::string sPlus; //number +
  bool bNewLine; //New line at the end
  bool bLowerIsBetter; //Lower values are better (red <-> green)

  //Inputs are: ItemType, main text, text between main text and value, value, if the value should be an int instead of a double, text after number, if there should be a newline after this item, if lower values are better
  iteminfo(std::string sIn0, std::string sIn1, std::string sIn2 = "", double dIn0 = -999, bool bIn0 = true, std::string sIn3 = "", bool bIn1 = true, bool bIn2 = false) {
    sType = sIn0;
    sName = sIn1;
    sFmt = sIn2;
    is_int = bIn0;
    dValue = dIn0;
    std::stringstream convert;
    if (bIn0 == true) {
    int dIn0i = int(dIn0);
    convert << dIn0i;
    } else {
    convert.precision(1);
    convert << std::fixed << dIn0;
    }
    sValue = convert.str();
    sPlus = sIn3;
    bNewLine = bIn1;
    bLowerIsBetter = bIn2;
  }
};

enum LIQUID_FILL_ERROR {L_ERR_NONE, L_ERR_NO_MIX, L_ERR_NOT_CONTAINER, L_ERR_NOT_WATERTIGHT,
    L_ERR_NOT_SEALED, L_ERR_FULL};

class item : public JsonSerializer, public JsonDeserializer
{
public:
 item();
 item(itype* it, unsigned int turn);
 item(itype* it, unsigned int turn, char let);
 void make_corpse(itype* it, mtype* mt, unsigned int turn); // Corpse
 item(std::string itemdata, game *g);
 item(JsonObject &jo);
 virtual ~item();
 void init();
 void make(itype* it);
 void clear(); // cleanup that's required to re-use an item variable

// returns the default container of this item, with this item in it
 item in_its_container(std::map<std::string, itype*> *itypes);

    nc_color color(player *u) const;
    nc_color color_in_inventory();
    std::string tname(bool with_prefix = true); // item name (includes damage, freshness, etc)
    std::string display_name(); // name for display (includes charges, etc)
    void use();
    bool burn(int amount = 1); // Returns true if destroyed

// Firearm specifics
 int reload_time(player &u);
 int clip_size();
 int dispersion();
 int gun_damage(bool with_ammo = true);
 int gun_pierce(bool with_ammo = true);
 int noise() const;
 int burst_size();
 int recoil(bool with_ammo = true);
 int range(player *p = NULL);
 ammotype ammo_type() const;
 char pick_reload_ammo(player &u, bool interactive);
 bool reload(player &u, char invlet);
 void next_mode();

    using JsonSerializer::serialize;
    // give the option not to save recursively, but recurse by default
    void serialize(JsonOut &jsout) const { serialize(jsout, true); }
    virtual void serialize(JsonOut &jsout, bool save_contents) const;
    using JsonDeserializer::deserialize;
    // easy deserialization from JsonObject
    virtual void deserialize(JsonObject &jo);
    void deserialize(JsonIn &jsin) {
        JsonObject jo = jsin.get_object();
        deserialize(jo);
    }

 std::string save_info() const; // Formatted for save files
 //
 void load_legacy(std::stringstream & dump);
 void load_info(std::string data);
 //std::string info(bool showtext = false); // Formatted for human viewing
 std::string info(bool showtext = false);
 std::string info(bool showtext, std::vector<iteminfo> *dump, game *g = NULL, bool debug = false);
 char symbol() const;
 nc_color color() const;
 int price() const;

 bool invlet_is_okay();
 bool stacks_with(item rhs);
 void put_in(item payload);
 void add_rain_to_container(bool acid, int charges = 1);

 int weight() const;

 int precise_unit_volume() const;
 int volume(bool unit_value=false, bool precise_value=false) const;
 int volume_contained();
 int attack_time();
 int damage_bash();
 int damage_cut() const;
 bool has_flag(std::string f) const;
 bool has_quality(std::string quality_id) const;
 bool has_quality(std::string quality_id, int quality_value) const;
 bool has_technique(std::string t);
 int has_gunmod(itype_id type);
 item* active_gunmod();
 item const* inspect_active_gunmod() const;
 bool goes_bad();
 bool count_by_charges() const;
 int max_charges() const;
 bool craft_has_charges();
 int num_charges();
 bool rotten(game *g);
 bool ready_to_revive(game *g); // used for corpses
// light emission, determined by type->light_emission (LIGHT_???) tag (circular),
// overridden by light.* struct (shaped)
 bool getlight(float & luminance, int & width, int & direction, bool calculate_dimming = true) const;
// for quick iterative loops
 int getlight_emit(bool calculate_dimming = true) const;
// Our value as a weapon, given particular skills
 int  weapon_value(player *p) const;
// As above, but discounts its use as a ranged weapon
 int  melee_value (player *p);
// how resistant item is to bashing and cutting damage
 int bash_resist() const;
 int cut_resist() const;
 // elemental resistances
 int acid_resist() const;
 bool is_two_handed(player *u);
 bool made_of(std::string mat_ident) const;
 std::string get_material(int m) const;
 bool made_of(phase_id phase) const;
 bool conductive() const; // Electricity
 bool flammable() const;

 bool destroyed_at_zero_charges();
// Most of the is_whatever() functions call the same function in our itype
 bool is_null() const; // True if type is NULL, or points to the null item (id == 0)
 bool is_food(player const*u) const;// Some non-food items are food to certain players
 bool is_food_container(player const*u) const;  // Ditto
 bool is_food() const;                // Ignoring the ability to eat batteries, etc.
 bool is_food_container() const;      // Ignoring the ability to eat batteries, etc.
 bool is_ammo_container() const;
 bool is_drink() const;
 bool is_weap() const;
 bool is_bashing_weapon() const;
 bool is_cutting_weapon() const;
 bool is_gun() const;
 bool is_silent() const;
 bool is_gunmod() const;
 bool is_bionic() const;
 bool is_ammo() const;
 bool is_armor() const;
 bool is_book() const;
 bool is_container() const;
 bool is_watertight_container() const;
 int is_funnel_container(int bigger_than) const;

 bool is_tool() const;
 bool is_software() const;
 bool is_macguffin() const;
 bool is_stationary() const;
 bool is_other() const; // Doesn't belong in other categories
 bool is_var_veh_part() const;
 bool is_artifact() const;

 int get_remaining_capacity_for_liquid(const item &liquid, LIQUID_FILL_ERROR &error) const;

 bool operator<(const item& other) const;

 itype_id typeId() const;
 itype* type;
 mtype*   corpse;
 it_ammo* curammo;

 std::vector<item> contents;

 std::string name;
 char invlet;           // Inventory letter
 int charges;
 bool active;           // If true, it has active effects to be processed
 int fridge;           // The turn we entered a fridge.
 signed char damage;    // How much damage it's sustained; generally, max is 5
 int burnt;             // How badly we're burnt
 int bday;              // The turn on which it was created
 int owned;             // UID of NPC owner; 0 = player, -1 = unowned
 light_emission light;
 union{
   int poison;          // How badly poisoned is it?
   int bigness;         // engine power, wheel size
   int frequency;       // Radio frequency
   int note;            // Associated dynamic text snippet.
   int irridation;      // Tracks radiation dosage.
 };
 std::string mode;    // Mode of operation, can be changed by the player.
 std::set<std::string> item_tags; // generic item specific flags
 unsigned item_counter; // generic counter to be used with item flags
 int mission_id; // Refers to a mission in game's master list
 int player_id; // Only give a mission to the right player!
 std::map<std::string, std::string> item_vars;
 static itype * nullitem();

 item clone();
private:
 int sort_rank() const;
 static itype * nullitem_m;
};

std::ostream & operator<<(std::ostream &, const item &);
std::ostream & operator<<(std::ostream &, const item *);

class map_item_stack
{
    private:
        class item_group
        {
            public:
                int x;
                int y;
                int count;

                //only expected to be used for things like lists and vectors
                item_group() {
                    x = 0;
                    y = 0;
                    count = 0;
                }

                item_group(const int arg_x, const int arg_y, const int arg_count) {
                    x = arg_x;
                    y = arg_y;
                    count = arg_count;
                }

                ~item_group() {};
        };
    public:
        item example; //an example item for showing stats, etc.
        std::vector<item_group> vIG;
        int totalcount;

        //only expected to be used for things like lists and vectors
        map_item_stack() {
            example = item();
            vIG.push_back(item_group());
            totalcount = 0;
        }

        map_item_stack(const item it, const int arg_x, const int arg_y) {
            example = it;
            vIG.push_back(item_group(arg_x, arg_y, 1));
            totalcount = 1;
        }

        ~map_item_stack() {};

        void addNewPos(const int arg_x, const int arg_y) {
            vIG.push_back(item_group(arg_x, arg_y, 1));
            totalcount++;
        }

        void incCount() {
            const int iVGsize = vIG.size();
            if (iVGsize > 0) {
                vIG[iVGsize-1].count++;
            }
            totalcount++;
        }
};

//this is an attempt for functional programming
bool is_edible(item i, player const*u);

//the assigned numbers are a result of legacy stuff in compare_split_screen_popup(),
//it would be better long-term to rewrite stuff so that we don't need that hack
enum hint_rating {
 HINT_CANT = 0, //meant to display as gray
 HINT_IFFY = 1, //meant to display as red
 HINT_GOOD = -999 // meant to display as green
};

#endif
