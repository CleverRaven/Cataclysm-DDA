#ifndef _OVERMAP_H_
#define _OVERMAP_H_
#include "enums.h"
#include "string.h"
#include "omdata.h"
#include "mongroup.h"
#include "settlement.h"
#include "output.h"
#include <vector>
#include <iosfwd>
#include <string>
#include <stdlib.h>
#include "cursesdef.h"
#include "name.h"

class npc;
struct settlement;

#define OVERMAP_DEPTH 10
#define OVERMAP_HEIGHT 0
#define OVERMAP_LAYERS (1 + OVERMAP_DEPTH + OVERMAP_HEIGHT)

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
 std::vector<om_note> notes;

 map_layer() : terrain(), visible(), notes() {}
};

class overmap
{
 public:
  overmap();
  overmap(overmap const&);
  overmap(game *g, int x, int y);
  ~overmap();

  overmap& operator=(overmap const&);

  point const& pos() const { return loc; }

  void save();
  void make_tutorial();
  void first_house(int &x, int &y);

  void process_mongroups(); // Makes them die out, maybe more

/* Returns the closest point of terrain type [type, type + type_range)
 * Use type_range of 4, for instance, to match all gun stores (4 rotations).
 * dist is set to the distance between the two points.
 * You can give dist a value, which will be used as the maximum distance; a
 *  value of 0 will search the entire overmap.
 * Use must_be_seen=true if only terrain seen by the player should be searched.
 * If no such tile can be found, (-1, -1) is returned.
 */
  // TODO: make this 3d
  point find_closest(point origin, oter_id type, int type_range,
                     int &dist, bool must_be_seen);
  std::vector<point> find_all(tripoint origin, oter_id type, int type_range,
                              int &dist, bool must_be_seen);
  std::vector<point> find_terrain(std::string term, int cursx, int cursy, int zlevel);
  int closest_city(point p);
  point random_house_in_city(int city_id);
  int dist_from_city(point p);
// Interactive point choosing; used as the map screen
  point draw_overmap(game *g, int const z);

  bool ter_in_type_range(int x, int y, int z, oter_id type, int type_range);
  oter_id& ter(int x, int y, int z);
  bool&   seen(int x, int y, int z);
  std::vector<mongroup*> monsters_at(int x, int y, int z);
  bool is_safe(int x, int y, int z); // true if monsters_at is empty, or only woodland

  bool has_note(int const x, int const y, int const z) const;
  std::string const& note(int const x, int const y, int const z) const;
  void add_note(int const x, int const y, int const z, std::string const& message);
  void delete_note(int const x, int const y, int const z) { add_note(x, y, z, ""); }
  point display_notes(game* g, int const z) const;

  point find_note(int const x, int const y, int const z, std::string const& text) const;
  void remove_npc(int npc_id);

  // TODO: make private
  std::vector<mongroup> zg;
  std::vector<radio_tower> radios;
  std::vector<npc *> npcs;
  std::vector<city> cities;
  std::vector<city> roads_out;
  std::vector<settlement> towns;

 private:
  point loc;
  std::string prefix;
  std::string name;

  //map_layer layer[OVERMAP_LAYERS];
  map_layer *layer;

  oter_id nullret;
  bool nullbool;
  std::string nullstr;

  // Initialise
  void init_layers();
  void open(game *g);
  void generate(game *g, overmap* north, overmap* east, overmap* south,
                overmap* west);
  bool generate_sub(int const z);

  //Drawing
  void draw(WINDOW *w, game *g, int z, int &cursx, int &cursy,
            int &origx, int &origy, char &ch, bool blink,
            overmap &hori, overmap &vert, overmap &diag);
  // Overall terrain
  void place_river(point pa, point pb);
  void place_forest();
  // City Building
  void place_cities();
  void put_buildings(int x, int y, int dir, city town);
  void make_road(int cx, int cy, int cs, int dir, city town);
  bool build_lab(int x, int y, int z, int s);
  void build_anthill(int x, int y, int z, int s);
  void build_tunnel(int x, int y, int z, int s, int dir);
  bool build_slimepit(int x, int y, int z, int s);
  void build_mine(int x, int y, int z, int s);
  void place_rifts(int const z);
  // Connection highways
  void place_hiways(std::vector<city> cities, int z, oter_id base);
  void place_subways(std::vector<point> stations);
  void make_hiway(int x1, int y1, int x2, int y2, int z, oter_id base);
  void building_on_hiway(int x, int y, int dir);
  // Polishing
  bool is_road(oter_id base, int x, int y, int z); // Dependant on road type
  bool is_road(int x, int y, int z);
  void polish(int z, oter_id min = ot_null, oter_id max = ot_tutorial);
  void good_road(oter_id base, int x, int y, int z);
  void good_river(int x, int y, int z);
  // Monsters, radios, etc.
  void place_specials();
  void place_special(overmap_special special, tripoint p);
  void place_mongroups();
  void place_radios();
  // File I/O

  std::string terrain_filename(int const x, int const y) const;
  std::string player_filename(int const x, int const y) const;

  // Map helper function.
  bool has_npc(game *g, int const x, int const y, int const z) const;
  void print_npcs(game *g, WINDOW *w, int const x, int const y, int const z);
};

// TODO: readd the stream operators
//std::ostream & operator<<(std::ostream &, const overmap *);
//std::ostream & operator<<(std::ostream &, const overmap &);
//std::ostream & operator<<(std::ostream &, const city &);

#endif
