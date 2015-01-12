#ifndef OVERMAP_H
#define OVERMAP_H

#include "enums.h"
#include "string.h"
#include "omdata.h"
#include "mapdata.h"
#include "mongroup.h"
#include "output.h"
#include "debug.h"
#include "cursesdef.h"
#include "name.h"
#include "input.h"
#include "json.h"
#include <vector>
#include <iosfwd>
#include <string>
#include <stdlib.h>
#include <array>

class overmapbuffer;
class npc;

#define OVERMAP_DEPTH 10
#define OVERMAP_HEIGHT 10
#define OVERMAP_LAYERS (1 + OVERMAP_DEPTH + OVERMAP_HEIGHT)

// base oters: exactly what's defined in json before things are split up into blah_east or roadtype_ns, etc
extern std::unordered_map<std::string, oter_t> obasetermap;

// Likelihood to pick a specific overmap terrain.
struct oter_weight {
    std::string ot_sid;
    int ot_iid;
    int weight;
};

// Local class for picking overmap terrain from a weighted list.
struct oter_weight_list {
    oter_weight_list() : total_weight(0) { };

    void add_item(std::string id, int weight) {
        oter_weight new_weight = { id, -1, weight };
        items.push_back(new_weight);
        total_weight += weight;
    }

    void setup() { // populate iid's for faster generation and sanity check.
        for(std::vector<oter_weight>::iterator item_it = items.begin();
            item_it != items.end(); ++item_it ) {
            if ( item_it->ot_iid == -1 ) {
                std::unordered_map<std::string, oter_t>::const_iterator it = obasetermap.find(item_it->ot_sid);
                if ( it == obasetermap.end() ) {
                    debugmsg("Bad oter_weight_list entry in region settings: overmap_terrain '%s' not found.", item_it->ot_sid.c_str() );
                    item_it->ot_iid = 0;
                } else {
                    item_it->ot_iid = it->second.loadid;
                }
            }
        }
    }

    size_t pick_ent() {
        int picked = rng(0, total_weight);
        int accumulated_weight = 0;
        size_t i;
        for(i=0; i<items.size(); i++) {
            accumulated_weight += items[i].weight;
            if(accumulated_weight >= picked) {
                break;
            }
        }
        return i;
    }

    std::string pickstr() {
        return items[ pick_ent() ].ot_sid;
    }

    int pick() {
        return items[ pick_ent() ].ot_iid;
    }

private:
    int total_weight;
    std::vector<oter_weight> items;
};



enum city_gen_type {
   CITY_GEN_RADIAL, // default/small/oldschol; shops constitute core, then parks, then residential
   CITY_GEN_INVALID, // reserved; big multi-overmap cities will have a more complex zoning pattern
};

struct city_settings {
   city_gen_type zoning_type;
   int shop_radius; // this is not a cut and dry % but rather an inverse voodoo number; rng(0,99) > VOODOO * distance / citysize;
   int park_radius; // in theory, adjusting these can make a town with a few shops and alot of parks + houses......by increasing shop_radius
   oter_weight_list shops;
   oter_weight_list parks;
   city_settings() : zoning_type(CITY_GEN_RADIAL), shop_radius(80), park_radius(130) { }
};
/*
todo: add relevent vars to regional_settings struct
#define STREETCHANCE 2
#define NUM_FOREST 250
#define TOP_HIWAY_DIST 999
#define MIN_ANT_SIZE 8
#define MAX_ANT_SIZE 20
#define MIN_GOO_SIZE 1
#define MAX_GOO_SIZE 2
#define MIN_RIFT_SIZE 6
#define MAX_RIFT_SIZE 16
#define SETTLE_DICE 2
#define SETTLE_SIDES 2
#define HIVECHANCE 180 //Chance that any given forest will be a hive
*/
/*
      "plant_coverage": {                   "//": "biome settings for builtin field mapgen",
          "percent_coverage": 0.833,        "//": "% of tiles that have a plant: one_in(120)",
          "default": "t_shrub",             "//": "default plant",
          "other": {
              "t_shrub_blueberry": 0.25,
              "t_shrub_strawberry": 0.25,
              "f_mutpoppy": 0.1
          },                                "//": "% of plants that aren't default",
          "boost_chance": 0.833,            "//": "% of fields with a boosted chance for plants",
          "boosted_percent_coverage": 2.5,  "//": "for the above: % of tiles that have a plant",
          "boosted_other_multiplier": 100,  "//": "for the above: multiplier for 'other' percentages"
      },

*/

/*
 * template for random bushes and such.
 * supports occasional boost to a single ter/furn type (clustered blueberry bushes for example)
 * json: double percentages (region statistics)and str ids, runtime int % * 1mil and int ids
 */
struct groundcover_extra { // todo; make into something more generic for other stuff (maybe)
   int mpercent_coverage; // % coverage where this is applied (*10000)
   std::string default_ter_str;
   int default_ter;
   std::map<std::string, double> percent_str;
   std::map<std::string, double> boosted_percent_str;
   std::map<int, ter_furn_id> weightlist;
   std::map<int, ter_furn_id> boosted_weightlist;
   int boost_chance;
   int boosted_mpercent_coverage;
   int boosted_other_mpercent;
   ter_furn_id pick( bool boosted = false ) const;
   void setup();
   groundcover_extra() : mpercent_coverage(0), default_ter_str(""), default_ter(0), boost_chance(0), boosted_mpercent_coverage(0), boosted_other_mpercent(1.0) {};
};

/*
 * Spationally relevent overmap and mapgen variables grouped into a set of suggested defaults;
 * eventually region mapping will modify as required and allow for transitions of biomes / demographics in a smoooth fashion
 */
class regional_settings {
  public:
   std::string id;            //
   std::string default_oter;  // 'field'
   id_or_id default_groundcover; // ie, 'grass_or_dirt'
   sid_or_sid * default_groundcover_str;
   int num_forests;           // amount of forest groupings per overmap
   int forest_size_min;       // size range of a forest group
   int forest_size_max;       // size range of a forest group
   int house_basement_chance; // (one in) Varies by region due to watertable
   int swamp_maxsize;         // SWAMPINESS: Affects the size of a swamp
   int swamp_river_influence; // voodoo number limiting spread of river through swamp
   int swamp_spread_chance;   // SWAMPCHANCE: (one in, every forest*forest size) chance of swamp extending past forest
   city_settings city_spec;   // put what where in a city of what kind
   groundcover_extra field_coverage;
   groundcover_extra forest_coverage;
   regional_settings() :
       id("null"),
       default_oter("field"),
       default_groundcover(0,0,0),
       default_groundcover_str(NULL),
       num_forests(250),
       forest_size_min(15),
       forest_size_max(40),
       house_basement_chance(2),
       swamp_maxsize(4),          // SWAMPINESS // Affects the size of a swamp
       swamp_river_influence(5),  //
       swamp_spread_chance(8500)  // SWAMPCHANCE // Chance that a swamp will spawn instead of forest
   {
       //default_groundcover_str = new sid_or_sid("t_grass", 4, "t_dirt");
   };
   void setup();
};


struct city {
 int x;
 int y;
 int s;
 std::string name;
 city(int X = -1, int Y = -1, int S = -1) : x (X), y (Y), s (S)
 {
     name = Name::get(nameIsTownName);
 }
};

struct om_note {
 int x;
 int y;
 int num;
 std::string text;
 om_note(int X = -1, int Y = -1, int N = -1, std::string T = "") :
         x (X), y (Y), num (N), text (T) {}
};

struct om_vehicle {
 int x; // overmap x coordinate of tracked vehicle
 int y; // overmap y coordinate
 std::string name;
};

enum radio_type {
    MESSAGE_BROADCAST,
    WEATHER_RADIO
};

#define RADIO_MIN_STRENGTH 80
#define RADIO_MAX_STRENGTH 200

struct radio_tower {
 int x;
 int y;
 int strength;
 radio_type type;
 std::string message;
 int frequency;
radio_tower(int X = -1, int Y = -1, int S = -1, std::string M = "",
            radio_type T = MESSAGE_BROADCAST) :
    x (X), y (Y), strength (S), type (T), message (M) {frequency = rand();}
};

struct map_layer {
    oter_id terrain[OMAPX][OMAPY];
    bool visible[OMAPX][OMAPY];
    bool explored[OMAPX][OMAPY];
    std::vector<om_note> notes;
};

struct node
{
 int x;
 int y;
 int d;
 int p;

 node(int xp, int yp, int dir, int pri) {x = xp; y = yp; d = dir; p = pri;}
 bool operator< (const node &n) const { return this->p > n.p; }
};

class overmap
{
 public:
    overmap(const overmap&) = default;
    overmap(overmap &&) = default;
    overmap(int x, int y);
    ~overmap();

    overmap& operator=(overmap const&) = default;

    point const& pos() const { return loc; }

    void save() const;

    /**
     * @return The (local) overmap terrain coordinates of a randomly
     * chosen place on the overmap with the specific overmap terrain.
     * Returns @ref invalid_tripoint if no suitable place has been found.
     */
    tripoint find_random_omt( const std::string &omt_base_type ) const;

    void process_mongroups(); // Makes them die out, maybe more
    void move_hordes();
    void signal_hordes( const int x, const int y, const int sig_power);

    /**
     * Return a vector containing the absolute coordinates of
     * every matching terrain on the current z level of the current overmap.
     * @returns A vector of terrain coordinates (absolute overmap terrain
     * coordinates), or empty vector if no matching terrain is found.
     */
    std::vector<point> find_terrain(const std::string &term, int zlevel);
    int closest_city(point p);
    point random_house_in_city(int city_id);
    int dist_from_city(point p);

    oter_id& ter(const int x, const int y, const int z);
    const oter_id get_ter(const int x, const int y, const int z) const;
    bool&   seen(int x, int y, int z);
    bool&   explored(int x, int y, int z);
    bool is_safe(int x, int y, int z); // true if monsters_at is empty, or only woodland
    bool is_road_or_highway(int x, int y, int z);
    bool is_explored(int const x, int const y, int const z) const;

    bool has_note(int const x, int const y, int const z) const;
    std::string const& note(int const x, int const y, int const z) const;
    void add_note(int const x, int const y, int const z, std::string const& message);
    void delete_note(int const x, int const y, int const z) { add_note(x, y, z, ""); }

    /**
     * Display a list of all notes on this z-level. Let the user choose
     * one or none of them.
     * @returns The location of the chosen note (absolute overmap terrain
     * coordinates), or invalid_point if the user did not choose a note.
     */
    static point display_notes(int z);
    /**
     * Dummy value, used to indicate that a point returned by a function
     * is invalid.
     */
    static const point invalid_point;
    static const tripoint invalid_tripoint;
    /**
     * Return a vector containing the absolute coordinates of
     * every matching note on the current z level of the current overmap.
     * @returns A vector of note coordinates (absolute overmap terrain
     * coordinates), or empty vector if no matching notes are found.
     */
     std::vector<point> find_notes(int const z, std::string const& text);
    /**
     * Interactive point choosing; used as the map screen.
     * The map is initially center at the players position.
     * @returns The absolute coordinates of the chosen point or
     * @ref invalid_point if canceled with escape (or similar key).
     */
    static tripoint draw_overmap();
    /**
     * Same as @ref draw_overmap() but starts at select if set.
     * Otherwise on players location.
     */
    static tripoint draw_overmap(const tripoint& center,
                                 bool debug_mongroup = false,
                                 const tripoint& select = tripoint(-1, -1, -1),
                                 const int iZoneIndex = -1);
    /**
     * Same as above but start at z-level z instead of players
     * current z-level, x and y are taken from the players position.
     */
    static tripoint draw_overmap(int z);

  /** Get the x coordinate of the left border of this overmap. */
  int get_left_border();

  /** Get the x coordinate of the right border of this overmap. */
  int get_right_border();

  /** Get the y coordinate of the top border of this overmap. */
  int get_top_border();

  /** Get the y coordinate of the bottom border of this overmap. */
  int get_bottom_border();

  const regional_settings& get_settings(const int x, const int y, const int z) {
     (void)x;
     (void)y;
     (void)z; // todo

     return settings;
  }
    void clear_mon_groups() { zg.clear(); }
private:
    std::multimap<tripoint, mongroup> zg;
public:
  // TODO: make private
  std::vector<radio_tower> radios;
  std::vector<npc *> npcs;
  std::map<int, om_vehicle> vehicles;
  std::vector<city> cities;
  std::vector<city> roads_out;

 private:
  friend class overmapbuffer;
  point loc;

    std::array<map_layer, OVERMAP_LAYERS> layer;

  oter_id nullret;
  bool nullbool;
  std::string nullstr;

    /**
     * When monsters despawn during map-shifting they will be added here.
     * map::spawn_monsters will load them and place them into the reality bubble
     * (adding it to the creature tracker and putting it onto the map).
     * This stores each submap worth of monsters in a different bucket of the multimap.
     */
    std::unordered_multimap<tripoint, monster> monster_map;
    regional_settings settings;

  // Initialise
  void init_layers();
  // open existing overmap, or generate a new one
  void open();
  // parse data in an opened overmap file
  void unserialize(std::ifstream & fin, std::string const & plrfilename, std::string const & terfilename);
  // parse data in an old overmap file
  bool unserialize_legacy(std::ifstream & fin, std::string const & plrfilename, std::string const & terfilename);

  void generate(const overmap* north, const overmap* east, const overmap* south, const overmap* west);
  bool generate_sub(int const z);

  /**
   * Draws the overmap terrain.
   * @param w The window to draw in.
   * @param center The global overmap terrain coordinate of the center
   * of the view. The z-component is used to determine the z-level.
   * @param orig The global overmap terrain coordinates of the player.
   * It will be marked specially.
   * @param debug_monstergroups Displays monster groups on the overmap.
   */
  static void draw(WINDOW *w, WINDOW *wbar, const tripoint &center,
            const tripoint &orig, bool blink, bool showExplored,
            input_context* inp_ctxt, bool debug_monstergroups = false,
            const int iZoneIndex = -1);

  // Overall terrain
  void place_river(point pa, point pb);
  void place_forest();
  // City Building
  void place_cities();
  void put_buildings(int x, int y, int dir, city town);
  void make_road(int cx, int cy, int cs, int dir, city town);
  bool build_lab(int x, int y, int z, int s);
  bool build_ice_lab(int x, int y, int z, int s);
  void build_anthill(int x, int y, int z, int s);
  void build_tunnel(int x, int y, int z, int s, int dir);
  bool build_slimepit(int x, int y, int z, int s);
  void build_mine(int x, int y, int z, int s);
  void place_rifts(int const z);
  // Connection highways
  void place_hiways(std::vector<city> cities, int z, const std::string &base);
  void place_subways(std::vector<point> stations);
  void make_hiway(int x1, int y1, int x2, int y2, int z, const std::string &base);
  void building_on_hiway(int x, int y, int dir);
  // Polishing
  bool check_ot_type(const std::string &otype, int x, int y, int z) const;
  bool check_ot_type_road(const std::string &otype, int x, int y, int z);
  bool is_road(int x, int y, int z);
  void polish(const int z, const std::string &terrain_type="all");
  void good_road(const std::string &base, int x, int y, int z);
  void good_river(int x, int y, int z);
  oter_id rotate(const oter_id &oter, int dir);
  bool allowed_terrain(tripoint p, int width, int height, std::list<std::string> allowed);
  bool allowed_terrain(tripoint p, std::list<tripoint>, std::list<std::string> allowed, std::list<std::string> disallowed);
  bool allow_special(tripoint p, overmap_special special, int &rotate);
  // Monsters, radios, etc.
  void place_specials();
  void place_special(overmap_special special, tripoint p, int rotation);
  void place_mongroups();
  void place_radios();

    void add_mon_group(const mongroup &group);
    // not available because *every* overmap needs location, so use the other constructor.
    overmap() = delete;
};

// TODO: readd the stream operators
//std::ostream & operator<<(std::ostream &, const overmap *);
//std::ostream & operator<<(std::ostream &, const overmap &);
//std::ostream & operator<<(std::ostream &, const city &);

extern std::unordered_map<std::string,oter_t> otermap;
extern std::vector<oter_t> oterlist;
//extern const regional_settings default_region_settings;
extern std::unordered_map<std::string, regional_settings> region_settings_map;

void load_overmap_terrain(JsonObject &jo);
void reset_overmap_terrain();
void load_region_settings(JsonObject &jo);
void reset_region_settings();

void finalize_overmap_terrain();

bool is_river(const oter_id &ter);
bool is_ot_type(const std::string &otype, const oter_id &oter);
map_extras& get_extras(const std::string &name);

#endif
