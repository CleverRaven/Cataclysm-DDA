#pragma once
#ifndef OVERMAP_H
#define OVERMAP_H

#include "omdata.h"
#include "overmap_types.h"
#include "mapdata.h"
#include "weighted_list.h"
#include "game_constants.h"
#include "monster.h"
#include "weather_gen.h"

#include <array>
#include <iosfwd>
#include <list>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

class input_context;
class JsonObject;
class npc;
class overmapbuffer;

struct mongroup;

struct oter_weight {
    inline bool operator ==(const oter_weight &other) const {
        return id == other.id;
    }

    string_id<oter_type_t> id;
};

struct city_settings {
   int shop_radius = 80;  // this is not a cut and dry % but rather an inverse voodoo number; rng(0,99) > VOODOO * distance / citysize;
   int park_radius = 130; // in theory, adjusting these can make a town with a few shops and alot of parks + houses......by increasing shop_radius
   weighted_int_list<oter_weight> shops;
   weighted_int_list<oter_weight> parks;

    oter_id pick_shop() const {
        return shops.pick()->id->get_first();
    }

    oter_id pick_park() const {
        return parks.pick()->id->get_first();
    }
};

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
    oter_str_id default_oter; // 'field'

    id_or_id<ter_t> default_groundcover; // ie, 'grass_or_dirt'
    std::shared_ptr<sid_or_sid> default_groundcover_str;

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

    weather_generator weather;

    std::unordered_map<std::string, map_extras> region_extras;

    regional_settings() : id("null"), default_oter("field"), default_groundcover(t_null, 0, t_null) { }
    void setup();
};


struct city {
    // in overmap terrain coordinates
    int x;
    int y;
    int s;
    std::string name;
    city(int X = -1, int Y = -1, int S = -1);

    operator bool() const {
        return s >= 0;
    }

    int get_distance_from( const tripoint &p ) const;
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
    void clear();

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
    const oter_id get_ter( const tripoint &p ) const;
    bool&   seen(int x, int y, int z);
    bool&   explored(int x, int y, int z);
    bool is_explored(int const x, int const y, int const z) const;

    bool has_note(int x, int y, int z) const;
    std::string const& note(int x, int y, int z) const;
    void add_note(int x, int y, int z, std::string message);
    void delete_note(int x, int y, int z);

    /**
     * Getter for overmap scents.
     * @returns a reference to a scent_trace from the requested location.
     */
    const scent_trace &scent_at( const tripoint &loc ) const;
    /**
     * Setter for overmap scents, stores the provided scent at the provided location.
     */
    void set_scent( const tripoint &loc, scent_trace &new_scent );

    /**
     * @returns Whether @param loc is within desired bounds of the overmap
     * @param clearance Minimal distance from the edges of the overmap
     */
    static bool inbounds( const tripoint &loc, int clearance = 0 );
    static bool inbounds( int x, int y, int z, int clearance = 0 ); /// @todo This one should be obsoleted
    /**
     * Display a list of all notes on this z-level. Let the user choose
     * one or none of them.
     * @returns The location of the chosen note (absolute overmap terrain
     * coordinates), or invalid_point if the user did not choose a note.
     */
    static point display_notes(int z);
    /**
     * Dummy value, used to indicate that a point returned by a function is invalid.
     */
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
     * invalid_point if canceled with escape (or similar key).
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
     * Draw overmap like with @ref draw_overmap() and display scent traces.
     */
    static tripoint draw_scents();
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

    /** Returns the (0, 0) corner of the overmap in the global coordinates. */
    point global_base_point() const;

  // @todo Should depend on coords
  const regional_settings& get_settings() const
  {
     return settings;
  }
    void clear_mon_groups();
private:
    std::multimap<tripoint, mongroup> zg;
public:
    /** Unit test enablers to check if a given mongroup is present. */
    bool mongroup_check(const mongroup &candidate) const;
    bool monster_check(const std::pair<tripoint, monster> &candidate) const;

    void add_npc( npc &who );
    // TODO: make private
  std::vector<radio_tower> radios;
  std::vector<npc *> npcs;
  std::map<int, om_vehicle> vehicles;
  std::vector<city> cities;
  std::vector<city> roads_out;

    std::vector<const overmap_special *> unplaced_mandatory_specials;

 private:
    friend class overmapbuffer;

    bool nullbool = false;
    point loc{ 0, 0 };

    std::array<map_layer, OVERMAP_LAYERS> layer;
    std::unordered_map<tripoint, scent_trace> scents;

    /**
     * When monsters despawn during map-shifting they will be added here.
     * map::spawn_monsters will load them and place them into the reality bubble
     * (adding it to the creature tracker and putting it onto the map).
     * This stores each submap worth of monsters in a different bucket of the multimap.
     */
    std::unordered_multimap<tripoint, monster> monster_map;
    regional_settings settings;

    // "Valid" map is one that has all mandatory specials
    // "Limited" map is one where all specials are placed only in allowed places
    enum class overmap_valid : int {
        // Invalid map, with no limits
        invalid = 0,
        // Valid map, but some parts are without limits
        unlimited,
        // Perfectly valid map
        valid
    };

    // Overmaps less valid than this will trigger the query
    overmap_valid minimum_validity;
    // The validity of this overmap, changed by actually generating it
    overmap_valid current_validity;

    void set_validity_from_settings();

  // Initialise
  void init_layers();
  // open existing overmap, or generate a new one
  void open();
 public:
  // parse data in an opened overmap file
  void unserialize(std::istream &fin);
  // Parse per-player overmap view data.
  void unserialize_view(std::istream &fin);
  // Save data in an opened overmap file
  void serialize(std::ostream &fin) const;
  // Save per-player overmap view data.
  void serialize_view(std::ostream &fin) const;
  // parse data in an old overmap file
  void unserialize_legacy(std::istream &fin);
  void unserialize_view_legacy(std::istream &fin);
 private:
  void generate(const overmap* north, const overmap* east, const overmap* south, const overmap* west);
  // Controls error handling in generation
  void generate_outer(const overmap* north, const overmap* east, const overmap* south, const overmap* west);
  bool generate_sub(int const z);

    const city &get_nearest_city( const tripoint &p ) const;

    void signal_hordes( const tripoint &p, int sig_power );
    void process_mongroups();
    void move_hordes();

    static bool obsolete_terrain( const std::string &ter );
    void convert_terrain( const std::unordered_map<tripoint, std::string> &needs_conversion );

    // drawing relevant data, e.g. what to draw.
    struct draw_data_t {
        // draw monster groups on the overmap.
        bool debug_mongroup = false;
        // draw weather, e.g. clouds etc.
        bool debug_weather = false;
        // draw editor.
        bool debug_editor = false;
        // draw scent traces.
        bool debug_scent = false;
        // draw zone location.
        tripoint select = tripoint(-1, -1, -1);
        int iZoneIndex = -1;
    };
    static tripoint draw_overmap(const tripoint& center, const draw_data_t &data);
  /**
   * Draws the overmap terrain.
   * @param w The window to draw map in.
   * @param wbar Window containing status bar
   * @param center The global overmap terrain coordinate of the center
   * of the view. The z-component is used to determine the z-level.
   * @param orig The global overmap terrain coordinates of the player.
   * It will be marked specially.
   * @param blink Whether blinking is enabled
   * @param showExplored Whether display of explored territory is enabled
   * @param inp_ctxt Input context in this screen
   * @param data Various other drawing flags, largely regarding debug information
   */
  static void draw(WINDOW *w, WINDOW *wbar, const tripoint &center,
            const tripoint &orig, bool blink, bool showExplored,
            input_context* inp_ctxt, const draw_data_t &data);

    oter_id random_shop() const;
    oter_id random_park() const;
    oter_id random_house() const;

  // Overall terrain
  void place_river(point pa, point pb);
  void place_forest();
  // City Building
  void place_cities();
  void put_building( int x, int y, om_direction::type dir, const city &town );

  void build_city_street( int cx, int cy, int cs, om_direction::type dir, const city &town );
  bool build_lab(int x, int y, int z, int s, bool ice = false);
  void build_anthill(int x, int y, int z, int s);
  void build_tunnel( int x, int y, int z, int s, om_direction::type dir );
  bool build_slimepit(int x, int y, int z, int s);
  void build_mine(int x, int y, int z, int s);
  void place_rifts(int const z);
    // Connection laying
    void build_connection( const point &source, const point &dest, int z, const int_id<oter_type_t> &type_id );
    void connect_closest_points( const std::vector<point> &points, int z, const int_id<oter_type_t> &type_id );
  // Polishing
  bool check_ot_type(const std::string &otype, int x, int y, int z) const;
  void polish(const int z, const std::string &terrain_type="all");
  void chip_rock(int x, int y, int z);

  oter_id good_connection( const oter_t &oter, const tripoint &p );
  void good_river(int x, int y, int z);

  // Returns a vector of enabled overmap specials.
  std::vector<const overmap_special *> get_enabled_specials() const;
  // Returns a vector of permuted coordinates of overmap sectors.
  // Each sector consists of 12x12 small maps. Coordinates of the sectors are in range [0, 15], [0, 15].
  // Check OMAPX, OMAPY, and OMSPEC_FREQ to learn actual values.
  std::vector<point> get_sectors() const;

  om_direction::type random_special_rotation( const overmap_special &special, const tripoint &p ) const;
  void place_special( const overmap_special &special, const tripoint &p, om_direction::type dir, const city &cit );
  // Monsters, radios, etc.
  void place_specials();
  /**
   * One pass of placing specials - by default there are 3 (mandatory, mandatory without city distance, optional)
   * @param to_place vector of pairs [special, count] to place in this pass. Placed specials are removed/deducted from this.
   * @param sectors sectors in which placement is possible. Taken sectors will be removed from this vector.
   * @param check_city_distance If false, the city distance limits of specials are not respected.
   */
  void place_specials_pass( std::vector<std::pair<const overmap_special *, int>> &to_place,
                            std::vector<point> &sectors, bool check_city_distance );
  /**
   * As @ref place_specials_pass, but for only one sector at a time.
   */
  bool place_special_attempt( std::vector<std::pair<const overmap_special *, int>> &candidates,
                              const point &sector, bool check_city_distance );
  void place_mongroups();
  void place_radios();

    void add_mon_group(const mongroup &group);

    void load_monster_groups( JsonIn &jo );
    void load_legacy_monstergroups( JsonIn &jo );
    void save_monster_groups( JsonOut &jo ) const;
};

// TODO: readd the stream operators
//std::ostream & operator<<(std::ostream &, const overmap *);
//std::ostream & operator<<(std::ostream &, const overmap &);
//std::ostream & operator<<(std::ostream &, const city &);

//extern const regional_settings default_region_settings;
typedef std::unordered_map<std::string, regional_settings> t_regional_settings_map;
typedef t_regional_settings_map::const_iterator t_regional_settings_map_citr;
extern t_regional_settings_map region_settings_map;

void load_region_settings(JsonObject &jo);
void reset_region_settings();
void load_region_overlay(JsonObject &jo);
void apply_region_overlay(JsonObject &jo, regional_settings &region);

namespace overmap_terrains
{

void load( JsonObject &jo, const std::string &src );
void check_consistency();
void finalize();
void reset();

size_t count();

}

bool is_river(const oter_id &ter);
bool is_ot_type(const std::string &otype, const oter_id &oter);

#endif
