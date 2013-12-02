#ifndef _INVENTORY_H_
#define _INVENTORY_H_

#include "item.h"
#include "artifact.h"

#include <string>
#include <vector>
#include <list>

class game;
class map;

const extern std::string inv_chars;

typedef std::list< std::list<item> > invstack;
typedef std::vector< std::list<item>* > invslice;

class inventory
{
 public:
  invslice slice(int start, int length);
  invslice slice(const std::list<item>* stack, int length);
  // returns an inventory instance containing only the chosen items
  // chosen is an invlet-count mapping
  inventory subset(std::map<char, int> chosen) const;
  std::list<item>& stack_by_letter(char ch);
  std::list<item> const_stack(int i) const;
  int size() const;
  int num_items() const;
  bool is_sorted() const;

  inventory& operator=  (inventory &rhs);
  inventory& operator=  (const inventory &rhs);
  inventory& operator+= (const inventory &rhs);
  inventory& operator+= (const item &rhs);
  inventory& operator+= (const std::list<item> &rhs);
  inventory  operator+  (const inventory &rhs);
  inventory  operator+  (const item &rhs);
  inventory  operator+  (const std::list<item> &rhs);

  inventory filter_by_activation(player& u);
  inventory filter_by_category(item_cat cat, const player& u) const;
  inventory filter_by_capacity_for_liquid(const item &liquid) const;

  void unsort(); // flags the inventory as unsorted
  void sort();
  void clear();
  void add_stack(std::list<item> newits);
  void clone_stack(const std::list<item> &rhs);
  void push_back(std::list<item> newits);
  char get_invlet_for_item( std::string item_type );
  item& add_item (item newit, bool keep_invlet = false, bool assign_invlet = true); //returns a ref to the added item
  void add_item_by_type(itype_id type, int count = 1, int charges = -1);
  void add_item_keep_invlet(item newit);
  void push_back(item newit);

/* Check all items for proper stacking, rearranging as needed
 * game pointer is not necessary, but if supplied, will ensure no overlap with
 * the player's worn items / weapon
 */
  void restack(player *p = NULL);

  void form_from_map(game *g, point origin, int distance, bool assign_invlet = true, std::string filter_flag = "", bool filter_inclusive = true);

  std::list<item> remove_stack_by_letter(char ch);
  std::list<item> remove_partial_stack(char ch, int amount);
  item  remove_item(item *it);
  item  remove_item_by_type(itype_id type);
  item  remove_item_by_letter(char ch);
  item  remove_item_by_charges(char ch, int quantity); // charged items, not stacks
  std::vector<item>  remove_mission_items(int mission_id);
  item& item_by_letter(char ch);
  item& item_by_type(itype_id type);
  item& item_or_container(itype_id type); // returns an item, or a container of it

  std::vector<item*> all_items_by_type(itype_id type);
  std::vector<item*> all_ammo(ammotype type);
  std::vector<item*> all_items_with_flag( const std::string flag );

// Below, "amount" refers to quantity
//        "charges" refers to charges
  int  amount_of (itype_id it) const;
  int  charges_of(itype_id it) const;

  std::list<item> use_amount (itype_id it, int quantity, bool use_container = false);
  std::list<item> use_charges(itype_id it, int quantity);

  bool has_amount (itype_id it, int quantity) const;
  bool has_charges(itype_id it, int quantity) const;
  bool has_flag(std::string flag) const; //Inventory item has flag
  bool has_item(item *it) const; // Looks for a specific item
  bool has_items_with_quality(std::string id, int level, int amount) const;
  bool has_gun_for_ammo(ammotype type) const;
  bool has_active_item(itype_id) const;

  bool has_mission_item(int mission_id) const;
  int butcher_factor() const;
  bool has_artifact_with(art_effect_passive effect) const;
  bool has_liquid(itype_id type) const;
  item& watertight_container();

  // NPC/AI functions
  int worst_item_value(npc* p) const;
  bool has_enough_painkiller(int pain) const;
  item& most_appropriate_painkiller(int pain);
  item& best_for_melee(player *p);
  item& most_loaded_gun();

  void rust_iron_items();

  int weight() const;
  int volume() const;
  int max_active_item_charges(itype_id id) const;

  void dump(std::vector<item*>& dest); // dumps contents into dest (does not delete contents)

  // vector rather than list because it's NOT an item stack
  std::vector<item*> active_items();

  void load_invlet_cache( std::ifstream &fin ); // see savegame_legacy.cpp

  void json_load_invcache(JsonIn &jsin);
  void json_load_items(JsonIn &jsin);

  void json_save_invcache(JsonOut &jsout) const;
  void json_save_items(JsonOut &jsout) const;

  item nullitem;
  std::list<item> nullstack;

 private:
  // For each item ID, store a set of "favorite" inventory letters.
  std::map<std::string, std::vector<char> > invlet_cache;
  void update_cache_with_item(item& newit);

  item remove_item(invstack::iterator iter);
  void assign_empty_invlet(item &it);
  invstack items;
  bool sorted;
};

void init_inventory_categories(); // inventory_ui.cpp

#endif
