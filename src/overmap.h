#ifndef OVERMAP_H
#define OVERMAP_H

#include "omdata.h"
#include "mapdata.h"
#include "weighted_list.h"
#include "game_constants.h"
#include "monster.h"
#include <vector>
#include <iosfwd>
#include <string>
#include <array>
#include <map>
#include <unordered_map>

class overmapbuffer;
class npc;
struct mongroup;
class JsonObject;
class input_context;

// base oters: exactly what's defined in json before things are split up into blah_east or roadtype_ns, etc
extern std::unordered_map<std::string, oter_t> obasetermap;

struct oter_weight {
    inline bool operator ==(const oter_weight &other) const {
        return ot_sid == other.ot_sid;
    }

    std::string ot_sid;
    int ot_iid;
};

enum city_gen_type {
   CITY_GEN_RADIAL, // default/small/oldschol; shops constitute core, then parks, then residential
   CITY_GEN_INVALID, // reserved; big multi-overmap cities will have a more complex zoning pattern
};

struct city_settings {
   city_gen_type zoning_type;
   int shop_radius; // this is not a cut and dry % but rather an inverse voodoo number; rng(0,99) > VOODOO * distance / citysize;
   int park_radius; // in theory, adjusting these can make a town with a few shops and alot of parks + houses......by increasing shop_radius
   weighted_int_list<oter_weight> shops;
   weighted_int_list<oter_weight> parks;
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
struct groundcover_extra {
    // TODO: make into something more generic for other stuff (maybe)
    std::string                   default_ter_str;
    std::map<std::string, double> percent_str;
    std::map<std::string, double> boosted_percent_str;
    std::map<int, ter_furn_id>    weightlist;
    std::map<int, ter_furn_id>    boosted_weightlist;
    ter_id default_ter               = t_null;
    int mpercent_coverage         = 0; // % coverage where this is applied (*10000)
    int boost_chance              = 0;
    int boosted_mpercent_coverage = 0;
    int boosted_other_mpercent    = 1;

    ter_furn_id pick( bool boosted = false ) const;
    void setup();
    groundcover_extra() = default;
};

struct sid_or_sid;
/*
 * Spationally relevent overmap and mapgen variables grouped into a set of suggested defaults;
 * eventually region mapping will modify as required and allow for transitions of biomes / demographics in a smoooth fashion
 */
struct regional_settings {
    std::string id;           //
    std::string default_oter; // 'field'

    id_or_id<ter_t> default_groundcover; // ie, 'grass_or_dirt'
    sid_or_sid *default_groundcover_str = nullptr;

    int num_forests           = 250;  // amount of forest groupings per overmap
    int forest_size_min       = 15;   // size range of a forest group
    int forest_size_max       = 40;   // size range of a forest group
    int house_basement_chance = 2;    // (one in) Varies by region due to watertable
    int swamp_maxsize         = 4;    // SWAMPINESS: Affects the size of a swamp
    int swamp_river_influence = 5;    // voodoo number limiting spread of river through swamp
    int swamp_spread_chance   = 8500; // SWAMPCHANCE: (one in, every forest*forest size) chance of swamp extending past forest

    city_settings     city_spec;      // put what where in a city of what kind
    groundcover_extra field_coverage;
    groundcover_extra forest_coverage;

    std::unordered_map<std::string, map_extras> region_extras;

    regional_settings() : id("null"), default_oter("field"), default_groundcover(t_null, 0, t_null) { }
    void setup();
    static void setup_oter(oter_weight &oter);
};


struct city {
 // in overmap terrain coordinates
 int x;
 int y;
 int s;
 std::string name;
 city(int X = -1, int Y = -1, int S = -1);
};

struct om_note {
    std::string text;
    int         x;
    int         y;
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

extern std::map<enum radio_type, std::string> radio_type_names;

#define RADIO_MIN_STRENGTH 80
#define RADIO_MAX_STRENGTH 200

struct radio_tower {
 // local (to the containing overmap) submap coordinates
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

class overmap
{
 public:
    overmap(const overmap&) = default;
    overmap(overmap &&) = default;
    overmap(int x, int y);
    // Argument-less constructor bypasses trying to load matching file, only used for unit testing.
    overmap();
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
    /**
     * Return a vector containing the absolute coordinates of
     * every matching terrain on the current z level of the current overmap.
     * @returns A vector of terrain coordinates (absolute overmap terrain
     * coordinates), or empty vector if no matching terrain is found.
     */
    std::vector<point> find_terrain(const std::string &term, int zlevel);

    oter_id& ter(const int x, const int y, const int z);
    const oter_id get_ter(const int x, const int y, const int z) const;
    bool&   seen(int x, int y, int z);
    bool&   explored(int x, int y, int z);
    bool is_road_or_highway(int x, int y, int z);
    bool is_explored(int const x, int const y, int const z) const;

    bool has_note(int x, int y, int z) const;
    std::string const& note(int x, int y, int z) const;
    void add_note(int x, int y, int z, std::string message);
    void delete_note(int x, int y, int z);

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
     * Draw overmap like with @ref draw_overmap() and display hordes.
     */
    static tripoint draw_hordes();
    /**
     * Draw overmap like with @ref draw_overmap() and display the weather.
     */
    static tripoint draw_weather();
    /**
     * Draw overmap like with @ref draw_overmap() and display the given zone.
     */
    static tripoint draw_zones( tripoint const &center, tripoint const &select, int const iZoneIndex );
    /**
     * Same as @ref draw_overmap() but starts at select if set.
     * Otherwise on players location.
     */
    /**
     * Same as above but start at z-level z instead of players
     * current z-level, x and y are taken from the players position.
     */
    static tripoint draw_overmap(int z);

    static tripoint draw_editor();

    static oter_id rotate(const oter_id &oter, int dir);

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
    void clear_mon_groups();
private:
    std::multimap<tripoint, mongroup> zg;
public:
    /** Unit test enablers to check if a given mongroup is present. */
    bool mongroup_check(const mongroup &candidate) const;
    int num_mongroups() const;
    bool monster_check(const std::pair<tripoint, monster> &candidate) const;
    int num_monsters() const;
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
 public:
  // parse data in an opened overmap file
  void unserialize(std::ifstream &fin);
  // Parse per-player overmap view data.
  void unserialize_view(std::ifstream &fin);
  // Save data in an opened overmap file
  void serialize(std::ofstream &fin) const;
  // Save per-player overmap view data.
  void serialize_view(std::ofstream &fin) const;
  // parse data in an old overmap file
  void unserialize_legacy(std::ifstream &fin);
  void unserialize_view_legacy(std::ifstream &fin);
 private:
  void generate(const overmap* north, const overmap* east, const overmap* south, const overmap* west);
  bool generate_sub(int const z);

    int dist_from_city( const tripoint &p );
    void signal_hordes( const tripoint &p, int sig_power );
    void process_mongroups();
    void move_hordes();

    static bool obsolete_terrain( const std::string &ter );
    void convert_terrain( const std::unordered_map<tripoint, std::string> &needs_conversion );

    // drawing relevant data, e.g. what to draw
    struct draw_data_t {
        // draw monster groups on the overmap
        bool debug_mongroup = false;
        // draw weather, e.g. clouds etc.
        bool debug_weather = false;
        // draw editor
        bool debug_editor = false;
        // draw zone location
        tripoint select = tripoint(-1, -1, -1);
        int iZoneIndex = -1;
    };
    static tripoint draw_overmap(const tripoint& center, const draw_data_t &data);
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
            input_context* inp_ctxt, const draw_data_t &data);

  // Overall terrain
  void place_river(point pa, point pb);
  void place_forest();
  // City Building
  void place_cities();
  void put_buildings(int x, int y, int dir, city town);
  void make_road(int cx, int cy, int cs, int dir, city town);
  bool build_lab(int x, int y, int z, int s, bool ice = false);
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
  void chip_rock(int x, int y, int z);
  void good_road(const std::string &base, int x, int y, int z);
  void good_river(int x, int y, int z);
  bool allowed_terrain( const tripoint& p, int width, int height, const std::list<std::string>& allowed );
  bool allowed_terrain( const tripoint& p, const std::list<tripoint>& rotated_points,
                        const std::list<std::string>& allowed, const std::list<std::string>& disallowed );
  bool allow_special(const overmap_special& special, const tripoint& p, int &rotate);
  // Monsters, radios, etc.
  void place_specials();
  void place_special(const overmap_special& special, const tripoint& p, int rotation);
  void place_mongroups();
  void place_radios();

    void add_mon_group(const mongroup &group);
};

// TODO: readd the stream operators
//std::ostream & operator<<(std::ostream &, const overmap *);
//std::ostream & operator<<(std::ostream &, const overmap &);
//std::ostream & operator<<(std::ostream &, const city &);

extern std::unordered_map<std::string,oter_t> otermap;
extern std::vector<oter_t> oterlist;
//extern const regional_settings default_region_settings;
typedef std::unordered_map<std::string, regional_settings> t_regional_settings_map;
typedef t_regional_settings_map::const_iterator t_regional_settings_map_citr;
extern t_regional_settings_map region_settings_map;

void load_overmap_terrain(JsonObject &jo);
void reset_overmap_terrain();
void load_region_settings(JsonObject &jo);
void reset_region_settings();
void load_region_overlay(JsonObject &jo);
void apply_region_overlay(JsonObject &jo, regional_settings &region);

void finalize_overmap_terrain();

bool is_river(const oter_id &ter);
bool is_ot_type(const std::string &otype, const oter_id &oter);

inline tripoint rotate_tripoint( tripoint p, int rotations );

#endif
