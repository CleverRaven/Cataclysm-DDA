#ifndef _VEHICLE_H_
#define _VEHICLE_H_

#include "tileray.h"
#include "color.h"
#include "item.h"
#include "line.h"
#include "veh_type.h"
#include <vector>
#include <string>
#include <fstream>

class map;
class player;
class game;

//collision factor for vehicle-vehicle collision; delta_v in mph
float get_collision_factor(float delta_v);

//How far to scatter parts from a vehicle when the part is destroyed (+/-)
#define SCATTER_DISTANCE 3

#define num_fuel_types 5
extern const ammotype fuel_types[num_fuel_types];
#define k_mvel 200 //adjust this to balance collision damage

// 0 - nothing, 1 - monster/player/npc, 2 - vehicle,
// 3 - thin_obstacle, 4 - bashable, 5 - destructible, 6 - other
enum veh_coll_type {
 veh_coll_nothing = 0,
 veh_coll_body,
 veh_coll_veh,
 veh_coll_thin_obstacle,
 veh_coll_bashable,
 veh_coll_destructable,
 veh_coll_other,

 num_veh_coll_types
};

struct veh_collision {
 //int veh?
 int part;
 veh_coll_type type;
 int imp; // impulse

 void* target;  //vehicle
 int target_part; //veh partnum
 std::string target_name;
 veh_collision() : part(0), type(veh_coll_nothing), imp(0), target(NULL), target_part(0), target_name("") {};
};

struct vehicle_item_spawn
{
    int x, y;
    int chance;
    std::vector<std::string> item_ids;
    std::vector<std::string> item_groups;
};

struct vehicle_prototype
{
    std::string id, name;
    std::vector<std::pair<point, std::string> > parts;
    std::vector<vehicle_item_spawn> item_spawns;
};


/**
 * Structure, describing vehicle part (ie, wheel, seat)
 */
struct vehicle_part : public JsonSerializer, public JsonDeserializer
{
    vehicle_part() : id("null"), iid(0), mount_dx(0), mount_dy(0), hp(0),
      blood(0), bigness(0), inside(false), flags(0), passenger_id(0), amount(0)
    {
        precalc_dx[0] = precalc_dx[1] = -1;
        precalc_dy[0] = precalc_dy[1] = -1;
    }
    bool has_flag( int flag ) { return flag & flags; }
    int set_flag( int flag ) { return flags |= flag; }
    int remove_flag( int flag ) { return flags &= ~flag; }

    static const int passenger_flag = 1;

    std::string id;         // id in map of parts (vehicle_part_types key)
    int iid;                // same as above, for lookup via int
    int mount_dx;           // mount point on the forward/backward axis
    int mount_dy;           // mount point on the left/right axis
    int precalc_dx[2];      // mount_dx translated to face.dir [0] and turn_dir [1]
    int precalc_dy[2];      // mount_dy translated to face.dir [0] and turn_dir [1]
    int hp;                 // current durability, if 0, then broken
    int blood;              // how much blood covers part (in turns).
    int bigness;            // size of engine, wheel radius, translates to item properties.
    bool inside;            // if tile provides cover. WARNING: do not read it directly, use vehicle::is_inside() instead
    int flags;
    int passenger_id;       // carrying passenger
    union
    {
        int amount;         // amount of fuel for tank/charge in battery
        int open;           // door is open
        int direction;      // direction the part is facing
    };
    std::vector<item> items;// inventory

    bool setid(const std::string str) {
        std::map<std::string, vpart_info>::const_iterator vpit = vehicle_part_types.find(str);
        if ( vpit == vehicle_part_types.end() ) {
            return false;
        }
        id = str;
        iid = vpit->second.loadid;
        return true;
    }

    // json saving/loading
    using JsonSerializer::serialize;
    void serialize(JsonOut &jsout) const;
    using JsonDeserializer::deserialize;
    void deserialize(JsonIn &jsin);
};

/**
 * A vehicle as a whole with all its components.
 *
 * This object can occupy multiple tiles, the objects actually visible
 * on the map are of type `vehicle_part`.
 *
 * Facts you need to know about implementation:
 * - Vehicles belong to map. There's `std::vector<vehicle>`
 *   for each submap in grid. When requesting a reference
 *   to vehicle, keep in mind it can be invalidated
 *   by functions such as `map::displace_vehicle()`.
 * - To check if there's any vehicle at given map tile,
 *   call `map::veh_at()`, and check vehicle type (`veh_null`
 *   means there's no vehicle there).
 * - Vehicle consists of parts (represented by vector). Parts have some
 *   constant info: see veh_type.h, `vpart_info` structure and
 *   vpart_list array -- that is accessible through `part_info()` method.
 *   The second part is variable info, see `vehicle_part` structure.
 * - Parts are mounted at some point relative to vehicle position (or starting part)
 *   (`0, 0` in mount coords). There can be more than one part at
 *   given mount coords, and they are mounted in different slots.
 *   Check tileray.h file to see a picture of coordinate axes.
 * - Vehicle can be rotated to arbitrary degree. This means that
 *   mount coords are rotated to match vehicle's face direction before
 *   their actual positions are known. For optimization purposes
 *   mount coords are precalculated for current vehicle face direction
 *   and stored in `precalc_*[0]`. `precalc_*[1]` stores mount coords for
 *   next move (vehicle can move and turn). Method `map::displace_vehicle()`
 *   assigns `precalc[1]` to `precalc[0]`. At any time (except
 *   `map::vehmove()` innermost cycle) you can get actual part coords
 *   relative to vehicle's position by reading `precalc_*[0]`.
 * - Vehicle keeps track of 3 directions:
 *     Direction | Meaning
 *     --------- | -------
 *     face      | where it's facing currently
 *     move      | where it's moving, it's different from face if it's skidding
 *     turn_dir  | where it will turn at next move, if it won't stop due to collision
 * - Some methods take `part` or `p` parameter. This is the index of a part in
 *   the parts list.
 * - Driver doesn't know what vehicle he drives.
 *   There's only player::in_vehicle flag which
 *   indicates that he is inside vehicle. To figure
 *   out what, you need to ask a map if there's a vehicle
 *   at driver/passenger position.
 * - To keep info consistent, always use
 *   `map::board_vehicle()` and `map::unboard_vehicle()` for
 *   boarding/unboarding player.
 * - To add new predesigned vehicle, add an entry to data/raw/vehicles.json
 *   similar to the existing ones. Keep in mind, that positive x coordinate points
 *   forwards, negative x is back, positive y is to the right, and
 *   negative y to the left:
 *
 *       orthogonal dir left (-Y)
 *            ^
 *       -X ------->  +X (forward)
 *            v
 *       orthogonal dir right (+Y)
 *
 *   When adding parts, function checks possibility to install part at given
 *   coords. If it shows debug messages that it can't add parts, when you start
 *   the game, you did something wrong.
 *   There are a few rules:
 *   1. Every mount point (tile) must begin with a part in the 'structure'
 *      location, usually a frame.
 *   2. No part can stack with itself.
 *   3. No part can stack with another part in the same location, unless that
 *      part is so small as to have no particular location (such as headlights).
 *   If you can't understand why installation fails, try to assemble your
 *   vehicle in game first.
 */
class vehicle : public JsonSerializer, public JsonDeserializer
{
private:
    game *g;

    bool has_structural_part(int dx, int dy);
    void open_or_close(int part_index, bool opening);
    bool is_connected(vehicle_part &to, vehicle_part &from, vehicle_part &excluded);
    void add_missing_frames();

    // direct damage to part (armor protection and internals are not counted)
    // returns damage bypassed
    int damage_direct (int p, int dmg, int type = 1);

    // get vpart powerinfo for part number, accounting for variable-sized parts.
    int part_power (int index);

public:
    vehicle (game *ag=0, std::string type_id = "null", int veh_init_fuel = -1, int veh_init_status = -1);
    ~vehicle ();

// check if given player controls this vehicle
    bool player_in_control (player *p);

// init parts state for randomly generated vehicle
    void init_state(game* g, int veh_init_fuel, int veh_init_status);

// damages all parts of a vehicle by a random amount
    void smash();

// load and init vehicle data from stream. This implies valid save data!
    void load_legacy(std::ifstream &stin);
    void load (std::ifstream &stin);

// Save vehicle data to stream
    void save (std::ofstream &stout);

    using JsonSerializer::serialize;
    void serialize(JsonOut &jsout) const;
    using JsonDeserializer::deserialize;
    void deserialize(JsonIn &jsin);

// Operate vehicle
    void use_controls();

// Honk the vehicle's horn, if there are any
    void honk_horn();

// get vpart type info for part number (part at given vector index)
    vpart_info& part_info (int index);

// check if certain part can be mounted at certain position (not accounting frame direction)
    bool can_mount (int dx, int dy, std::string id);

// check if certain part can be unmounted
    bool can_unmount (int p);

// install a new part to vehicle (force to skip possibility check)
    int install_part (int dx, int dy, std::string id, int hp = -1, bool force = false);

    void remove_part (int p);

    void break_part_into_pieces (int p, int x, int y, bool scatter = false);

// Generate the corresponding item from a vehicle part.
// Still needs to be removed.
    item item_from_part( int part );

// translate item health to part health
    void get_part_properties_from_item (game* g, int partnum, item& i);
// translate part health to item health (very lossy.)
    void give_part_properties_to_item (game* g, int partnum, item& i);

// returns the list of indeces of parts at certain position (not accounting frame direction)
    const std::vector<int> parts_at_relative (const int dx, const int dy, bool use_cache = true);

// returns index of part, inner to given, with certain flag, or -1
    int part_with_feature (int p, const std::string &f, bool unbroken = true);
    int part_with_feature (int p, const vpart_bitflags &f, bool unbroken = true);
// returns indices of all parts in the vehicle with the given flag
    std::vector<int> all_parts_with_feature(const std::string &feature, bool unbroken = true);
    std::vector<int> all_parts_with_feature(const vpart_bitflags &f, bool unbroken = true);

// returns indices of all parts in the given location slot
    std::vector<int> all_parts_at_location(const std::string &location);

// returns true if given flag is present for given part index
    bool part_flag (int p, const std::string &f);
    bool part_flag (int p, const vpart_bitflags &f);

// Translate seat-relative mount coords into tile coords
    void coord_translate (int reldx, int reldy, int &dx, int &dy);

// Translate seat-relative mount coords into tile coords using given face direction
    void coord_translate (int dir, int reldx, int reldy, int &dx, int &dy);

// Seek a vehicle part which obstructs tile with given coords relative to vehicle position
    int part_at (int dx, int dy);
    int global_part_at (int x, int y);
    int part_displayed_at(int local_x, int local_y);

// Given a part, finds its index in the vehicle
    int index_of_part(vehicle_part *part);

// get symbol for map
    char part_sym (int p);
    std::string part_id_string(int p, char &part_mod);

// get color for map
    nc_color part_color (int p);

// Vehicle parts description
    int print_part_desc (WINDOW *win, int y1, int width, int p, int hl = -1);

// Vehicle fuel indicator. Should probably rename to print_fuel_indicators and make a print_fuel_indicator(..., FUEL_TYPE);
    void print_fuel_indicator (void *w, int y, int x, bool fullsize = false, bool verbose = false);

// Precalculate mount points for (idir=0) - current direction or (idir=1) - next turn direction
    void precalc_mounts (int idir, int dir);

// get a list of part indeces where is a passenger inside
    std::vector<int> boarded_parts();
    int free_seat();

// get passenger at part p
    player *get_passenger (int p);

// get global coords for vehicle
    int global_x ();
    int global_y ();

// get omap coordinate for vehicle
    int omap_x ();
    int omap_y ();

// update map coordinates of the vehicle
    void update_map_x(int x);
    void update_map_y(int y);

// Checks how much certain fuel left in tanks. If for_engine == true that means
// ftype == "battery" is also takes in account "plutonium" fuel (electric motors can use both)
    int fuel_left (const ammotype & ftype, bool for_engine = false);
    int fuel_capacity (const ammotype & ftype);

    // refill fuel tank(s) with given type of fuel
    // returns amount of leftover fuel
    int refill (const ammotype & ftype, int amount);

    // drains a fuel type (e.g. for the kitchen unit)
    // returns amount actually drained, does not engage reactor
    int drain (const ammotype & ftype, int amount);

// fuel consumption of vehicle engines of given type, in one-hundreth of fuel
    int basic_consumption (const ammotype & ftype);

    void consume_fuel ();

    void power_parts ();

    void charge_battery (int amount);

// get the total mass of vehicle, including cargo and passengers
    int total_mass ();

// get center of mass of vehicle; coordinates are precalc_dx[0] and precalc_dy[0]
    void center_of_mass(int &x, int &y);

// Get combined power of all engines. If fueled == true, then only engines which
// vehicle have fuel for are accounted
    int total_power (bool fueled = true);

// Get combined power of solar panels
    int solar_power ();

// Get acceleration gained by combined power of all engines. If fueled == true, then only engines which
// vehicle have fuel for are accounted
    int acceleration (bool fueled = true);

// Get maximum velocity gained by combined power of all engines. If fueled == true, then only engines which
// vehicle have fuel for are accounted
    int max_velocity (bool fueled = true);

// Get safe velocity gained by combined power of all engines. If fueled == true, then only engines which
// vehicle have fuel for are accounted
    int safe_velocity (bool fueled = true);

    int noise (bool fueled = true, bool gas_only = false);

// Calculate area covered by wheels and, optionally count number of wheels
    float wheels_area (int *cnt = 0);

// Combined coefficient of aerodynamic and wheel friction resistance of vehicle, 0-1.0.
// 1.0 means it's ideal form and have no resistance at all. 0 -- it won't move
    float k_dynamics ();

// Coefficient of mass, 0-1.0.
// 1.0 means mass won't slow vehicle at all, 0 - it won't move
    float k_mass ();

// strain of engine(s) if it works higher that safe speed (0-1.0)
    float strain ();

// calculate if it can move using its wheels configuration
    bool valid_wheel_config ();

// idle fuel consumption and battery charge
    void idle ();

// thrust (1) or brake (-1) vehicle
    void thrust (int thd);

// depending on skid vectors, chance to recover.
    void possibly_recover_from_skid();

//forward component of velocity.
    float forward_velocity();

// cruise control
    void cruise_thrust (int amount);

// turn vehicle left (negative) or right (positive), degrees
    void turn (int deg);

    bool collision( std::vector<veh_collision> &veh_veh_colls,
                    std::vector<veh_collision> &veh_misc_colls, int dx, int dy,
                    bool &can_move, int &imp, bool just_detect = false );

// handle given part collision with vehicle, monster/NPC/player or terrain obstacle
// return collision, which has type, impulse, part, & target.
    veh_collision part_collision (int part, int x, int y, bool just_detect);

// Process the trap beneath
    void handle_trap (int x, int y, int part);

    int max_volume(int part); // stub for per-vpart limit
    int free_volume(int part);
    int stored_volume(int part);
    bool is_full(const int part, const int addvolume = -1, const int addnumber = -1 );

// add item to part's cargo. if false, then there's no cargo at this part or cargo is full(*)
// *: "full" means more than 1024 items, or max_volume(part) volume (500 for now)
    bool add_item (int part, item itm);

// remove item from part's cargo
    void remove_item (int part, int itemdex);

// Generates starting items in the car, should only be called when placed on the map
    void place_spawn_items();

    void gain_moves (int mp);

// reduces velocity to 0
    void stop ();

    void find_horns ();

    void find_power ();

    void find_fuel_tanks ();

    void find_parts();

    void find_exhaust ();

    void refresh_insides ();

    bool pedals();

    bool is_inside (int p);

    void unboard_all ();

    // damage types:
    // 0 - piercing
    // 1 - bashing (damage applied if it passes certain treshold)
    // 2 - incendiary
    // damage individual part. bash means damage
    // must exceed certain threshold to be substracted from hp
    // (a lot light collisions will not destroy parts)
    // returns damage bypassed
    int damage (int p, int dmg, int type = 1, bool aimed = true);

    // damage all parts (like shake from strong collision), range from dmg1 to dmg2
    void damage_all (int dmg1, int dmg2, int type, const point &impact);

    void leak_fuel (int p);

    // fire the turret which is part p
    void fire_turret (int p, bool burst = true);

    // internal procedure of turret firing
    bool fire_turret_internal (int p, it_gun &gun, it_ammo &ammo, int charges,
                               const std::string &firing_sound = "");

    // opens/closes doors or multipart doors
    void open(int part_index);
    void close(int part_index);

    // upgrades/refilling/etc. see veh_interact.cpp
    void interact ();

    // return a vector w/ 'direction' & 'magnitude', in its own sense of the words.
    rl_vec2d velo_vec();
    //normalized vectors, from tilerays face & move
    rl_vec2d face_vec();
    rl_vec2d move_vec();

    // config values
    std::string name;   // vehicle name
    std::string type;           // vehicle type
    std::vector<vehicle_part> parts;   // Parts which occupy different tiles
    std::map<point, std::vector<int> > relative_parts;    // parts_at_relative(x,y) is used alot (to put it mildly)
    std::vector<int> horns;            // List of horn part indices
    std::vector<int> lights;           // List of light part indices
    std::vector<int> fuel;             // List of fuel tank indices
    std::vector<int> wheelcache;
    std::vector<vehicle_item_spawn> item_spawns; //Possible starting items
    std::set<std::string> tags;        // Properties of the vehicle
    int exhaust_dx;
    int exhaust_dy;

    // temp values
    int smx, smy;   // submap coords. WARNING: must ALWAYS correspond to sumbap coords in grid, or i'm out
    bool insides_dirty; // if true, then parts' "inside" flags are outdated and need refreshing
    bool parts_dirty;   //
    int init_veh_fuel;
    int init_veh_status;

    // save values
    int posx, posy;
    int levx,levy;       // vehicle map coordinates.
    tileray face;       // frame direction
    tileray move;       // direction we are moving
    int velocity;       // vehicle current velocity, mph * 100
    int cruise_velocity; // velocity vehicle's cruise control trying to acheive
    bool cruise_on;     // cruise control on/off
    bool engine_on;     // engine on/off
    bool has_pedals;
    bool lights_on;     // lights on/off
    bool tracking_on;        // vehicle tracking on/off
    int om_id;          // id of the om_vehicle struct corresponding to this vehicle
    bool overhead_lights_on; //circle lights on/off
    bool fridge_on;     //fridge on/off
    int turn_dir;       // direction, to wich vehicle is turning (player control). will rotate frame on next move
    bool skidding;      // skidding mode
    int last_turn;      // amount of last turning (for calculate skidding due to handbrake)
    //int moves;
    float of_turn;      // goes from ~1 to ~0 while proceeding every turn
    float of_turn_carry;// leftover from prev. turn
    int turret_mode;    // turret firing mode: 0 = off, 1 = burst fire
    int lights_power;   // total power of components with LIGHT or CONE_LIGHT flag
    int overhead_power;   // total power of components with CIRCLE_LIGHT flag
    int tracking_power; // total power consumed by tracking devices (why would you use more than one?)
    int fridge_power; // total power consumed by fridges
};

#endif
