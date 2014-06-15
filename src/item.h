#ifndef _ITEM_H_
#define _ITEM_H_

#include <climits>
#include <string>
#include <vector>
#include <list>
#include "itype.h"
#include "mtype.h"

class game;
class player;
class npc;
struct itype;
class material_type;

// Thresholds for radiation dosage for the radiation film badge.
const int rad_dosage_thresholds[] = { 0, 30, 60, 120, 240, 500};
const std::string rad_threshold_colors[] = { _("green"), _("blue"), _("yellow"),
                                             _("orange"), _("red"), _("black")};

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
    bool bDrawName; //If false then compares sName, but don't print sName.

    // Inputs are: ItemType, main text, text between main text and value, value,
    // if the value should be an int instead of a double, text after number,
    // if there should be a newline after this item, if lower values are better
    iteminfo(std::string Type, std::string Name, std::string Fmt = "", double Value = -999,
             bool _is_int = true, std::string Plus = "", bool NewLine = true,
             bool LowerIsBetter = false, bool DrawName = true);
};

enum LIQUID_FILL_ERROR {L_ERR_NONE, L_ERR_NO_MIX, L_ERR_NOT_CONTAINER, L_ERR_NOT_WATERTIGHT,
    L_ERR_NOT_SEALED, L_ERR_FULL};

enum layer_level {
    UNDERWEAR = 0,
    REGULAR_LAYER,
    OUTER_LAYER,
    BELTED_LAYER,
    MAX_CLOTHING_LAYER
};

class item : public JsonSerializer, public JsonDeserializer
{
public:
 item();
 item(const std::string new_type, unsigned int turn, bool rand = true );
 void make_corpse(const std::string new_type, mtype* mt, unsigned int turn); // Corpse
 item(std::string itemdata);
 item(JsonObject &jo);
 virtual ~item();
 void init();
 void make( const std::string new_type );
 void clear(); // cleanup that's required to re-use an item variable

 // returns the default container of this item, with this item in it
 item in_its_container();

    nc_color color(player *u) const;
    nc_color color_in_inventory();
    std::string tname(unsigned int quantity = 1, bool with_prefix = true); // item name (includes damage, freshness, etc)
    std::string display_name(unsigned int quantity = 1); // name for display (includes charges, etc)
    void use();
    bool burn(int amount = 1); // Returns true if destroyed

 // Returns the category of this item.
 const item_category &get_category() const;

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
 int pick_reload_ammo(player &u, bool interactive);
 bool reload(player &u, int pos);
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
 std::string info(bool showtext, std::vector<iteminfo> *dump, bool debug = false);
 char symbol() const;
 nc_color color() const;
 int price() const;

    /**
     * Return the butcher factor, always positive, but lower is better.
     * If the item can not be used for butcherin it return INT_MAX.
     */
    int butcher_factor() const;

    /**
     * Returns true if this item is of the specific type, or
     * if this functions returns true for any of its contents.
     */
    bool is_of_type_or_contains_it(const std::string &type_id) const;
    /**
     * Returns true if this item is ammo and has the specifi ammo type,
     * or if this functions returns true for any of its contents.
     * This does not check type->id, but it_ammo::type.
     */
    bool is_of_ammo_type_or_contains_it(const ammotype &ammo_type_id) const;

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

 /**
  * Count the amount of items of type 'it' including this item,
  * and any of its contents (recursively).
  * @param it The type id, only items with the same id are counted.
  * @param used_as_tool If false all items with the PSEUDO flag are ignore
  * (not counted).
  */
 int amount_of(const itype_id &it, bool used_as_tool) const;
 /**
  * Count all the charges of items of the type 'it' including this item,
  * and any of its contents (recursively).
  * @param it The type id, only items with the same id are counted.
  */
 long charges_of(const itype_id &it) const;
 /**
  * Consume a specific amount of charges from items of a specific type.
  * This includes this item, and any of its contents (recursively).
  * @param it The type id, only items of this type are considered.
  * @param quantity The number of charges that should be consumed.
  * It will be changed for each used charge. After calling this function it
  * may be at 0 which means all requested charges have been consumed.
  * @param used All used charges are put into this list, the caller may need it.
  * @return Whether this item should be deleted (in which case it returns true).
  * Some items (those that are counted by charges) must be destroyed when
  * their charges reach 0.
  * This is usually does not apply to tools.
  * Also if this function is called on a container and the function erase charges
  * from its contents the container should not be deleted - it returns false in
  * that case.
  * The caller *must* check the return value and remove the item from wherever
  * it is stored when the function returns true.
  * Note that the item itself has no way of knowing where it is stored and can
  * therefor not delete itself.
  */
 bool use_charges(const itype_id &it, long &quantity, std::list<item> &used);
 /**
  * Consume a specific amount of items of a specific type.
  * This includes this item, and any of its contents (recursively).
  * @see item::use_charges - this is similar for items, not charges.
  * @param use_container If the contents of an item are used, also use the
  * container it was in.
  */
 bool use_amount(const itype_id &it, int &quantity, bool use_container, std::list<item> &used);

 bool has_flag(const std::string &f) const;
 bool contains_with_flag (std::string f) const;
 bool has_quality(std::string quality_id) const;
 bool has_quality(std::string quality_id, int quality_value) const;
 bool has_technique(std::string t);
 int has_gunmod(itype_id type);
 item* active_gunmod();
 item const* inspect_active_gunmod() const;
 bool goes_bad();
 bool count_by_charges() const;
 long max_charges() const;
 bool craft_has_charges();
 long num_charges();
 bool rotten();
 void calc_rot();
 int brewing_time();
 bool ready_to_revive(); // used for corpses
 void detonate(point p) const;
 bool can_revive();      // test if item is a corpse and can be revived
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
 // Never returns NULL
 const material_type *get_material(int m) const;
 bool made_of(phase_id phase) const;
 bool conductive() const; // Electricity
 bool flammable() const;

 // umber of mods that can still be installed into the given
 // mod location, for non-guns it returns always 0
 int get_free_mod_locations(const std::string &location) const;

 bool destroyed_at_zero_charges();
// Most of the is_whatever() functions call the same function in our itype
 bool is_null() const; // True if type is NULL, or points to the null item (id == 0)
 bool is_food(player const*u) const;// Some non-food items are food to certain players
 bool is_food_container(player const*u) const;  // Ditto
 bool is_food() const;                // Ignoring the ability to eat batteries, etc.
 bool is_food_container() const;      // Ignoring the ability to eat batteries, etc.
 bool is_corpse() const;
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
 bool is_funnel_container(unsigned int &bigger_than) const;

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
 long charges;
 bool active;           // If true, it has active effects to be processed
 int fridge;            // The turn we entered a fridge.
 int rot;               // decay; same as turn-bday at 65 degrees, but doubles/halves every 18 degrees. can be negative (start game fridges)
 int last_rot_check;    // last turn we calculated rot
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
 typedef std::vector<item> t_item_vector;
 t_item_vector components;

 item clone(bool rand = true);

 int add_ammo_to_quiver(player *u, bool isAutoPickup);
 int max_charges_from_flag(std::string flagName);
private:
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

// Commonly used convenience functions that match an item to one of the 3 common types of locators:
// invlet (char), type_id (itype_id, a typedef of string), or position (int).
// The item's position is optional, if not passed in we expect the item to fail position match.
bool item_matches_locator(const item& it, const itype_id& id, int item_pos = INT_MIN);
bool item_matches_locator(const item& it, int locator_pos, int item_pos = INT_MIN);
bool item_matches_locator(const item& it, char invlet, int item_pos = INT_MIN);

//this is an attempt for functional programming
bool is_edible(item i, player const*u);

//the assigned numbers are a result of legacy stuff in draw_item_info(),
//it would be better long-term to rewrite stuff so that we don't need that hack
enum hint_rating {
 HINT_CANT = 0, //meant to display as gray
 HINT_IFFY = 1, //meant to display as red
 HINT_GOOD = -999 // meant to display as green
};

#endif
