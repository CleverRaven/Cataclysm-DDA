#ifndef _VEHICLE_H_
#define _VEHICLE_H_

#include "tileray.h"
#include "color.h"
#include "item.h"
#include "line.h"
#include "veh_type.h"
#include <vector>
#include <string>
#include <iosfwd>

class map;
class player;

//How far to scatter parts from a vehicle when the part is destroyed (+/-)
#define SCATTER_DISTANCE 3

// The size (width) of a single game tile in mm, 1 meter is canon
#define TILE_SIZE_MM 3000
#define TILE_SIZE_M 3.0d

// The length of a single game turn in second, 6000 msec is canon
#define TURN_TIME_MSEC 1000
#define TURN_TIME_S 1.0d

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

struct veh_collision
{
    veh_coll_type type;       // What we're colliding with
    int x, y;                 // Coordinates of impact point
    int precalc_x, precalc_y; // Coordinates of vehicle part impacted
    float mass;               // For non-vehicles
    float elasticity;         // For non-vehicles
    int density;              // For non-vehicles
    void* target;             // Pointer to vehicle, mon, or npc
    std::string target_name;
    veh_collision() : type(veh_coll_nothing), x(0), y(0), mass(0.0f), elasticity(0.3f), density(15), target(NULL), target_name("") {};
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

// Used in calculate_forces and div physical properties resulting from generating thrust
// These values are used when accelerating/braking and in the examine screen
struct vehicle_forces
{
    int eng_pwr_cur, eng_pwr_max, eng_alt_pwr;
    bool is_thrusting;               // If engine is used to provide thrust
    bool valid_wheel_config;         // Cached, if it has enough wheels to acc/brake
    double distance_traveled;        // How far the vehicle traveled
    double downforce_newton;         // Force pushing vehicle towards the ground 
    double drag_newton;              // Force slowing vehicle from wind res
    double engine_newton;            // Force used to actually drive wheels
    double engine_newton_max;        // How much the engine can work
    double engine_watt_max;          // Available power
    double engine_watt_average;      // How much power engine actually used
    double ground_res_coeff;         // Rolling resistance and debris
    double ground_res_newton;        // Force slowing vehicle from rolling/debris
    double kinetic_end;              // Initial kinetic energy
    double kinetic_start;            // Kinetic energy after calculations
    double newton_total;             // Sum of all the forces working on the vehicle
    double newton_average;           // Average sum of all the forces
    double rolling_res_coeff;        // Wheel resistance
    double time_taken;               // Time it took to move
    double tire_friction_coeff;      // How well the tires grip the ground
    double user_applied_newton;      // Force actually driving wheels
    double velocity_average;
    double velocity_end;
    double velocity_start;
    double wheel_newton_available;   // Grip available for thrust/steering
    double wheel_newton_max;         // Total grip
};


/**
 * Structure, describing vehicle part (ie, wheel, seat)
 */
struct vehicle_part : public JsonSerializer, public JsonDeserializer
{
    vehicle_part() : id("null"), iid(0), mount_dx(0), mount_dy(0), hp(0),
      blood(0), bigness(0), inside(false), removed(false), flags(0), passenger_id(0), amount(0)
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
    bool removed;           // TRUE if this part is removed. The part won't disappear until the end of the turn
                            // so our indices can remain consistent.
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
 *     steering  | delta (in degrees) player is trying to steer it to
 *     Direction will also be modified by collisions, rough terrain, and skidding.
 * - While in cruise control, the vehicle tries to maintain value set by thrust().
 *   Otherwise, thrust must be set every turn with thrust() or the vehicle
 *   assumes player has let go of the gas (brake) pedal. The thrust is not
 *   actually applied until do_turn_actions() is called by map.
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
    bool has_structural_part(int dx, int dy);
    void open_or_close(int part_index, bool opening);
    bool is_connected(vehicle_part &to, vehicle_part &from, vehicle_part &excluded);
    void add_missing_frames();

    // Finds a single collision with vehicle, monster/NPC/player or terrain obstacle.
    // Return veh_collision, which has type, mass, and target.
    veh_collision get_point_collision (int x, int y);

    // Processes a single veh_collision, assigning damage and changing velocity/dir.
    void process_collision (veh_collision coll);

    // Calculate and deal damage to a single point from collition
    bool apply_damage_from_collision_to_point(int frame, veh_collision coll);

    // direct damage to part (armor protection and internals are not counted)
    // returns damage bypassed
    int damage_direct (int p, int dmg, int type = 1);

    // get vpart powerinfo for part number, accounting for variable-sized parts and hps.
    int part_power( int index, bool at_full_hp = false );

    // get vpart epowerinfo for part number, in watts
    int part_epower (int index);

    //Refresh all caches and re-locate all parts
    void refresh();

    // Do stuff like clean up blood and produce smoke from broken parts. Returns false if nothing needs doing.
    bool do_environmental_effects();

public:
    vehicle (std::string type_id = "null", int veh_init_fuel = -1, int veh_init_status = -1);
    ~vehicle ();

// check if given player controls this vehicle
    bool player_in_control (player *p);

// init parts state for randomly generated vehicle
    void init_state(int veh_init_fuel, int veh_init_status);

// damages all parts of a vehicle by a random amount
    void smash();

    // Runs engines at idle for alternator power, spend epower, charges batteries
    // Returns idle engine load in joules. 
    double generate_all_power();

// load and init vehicle data from stream. This implies valid save data!
    void load_legacy(std::ifstream &stin);
    void load (std::ifstream &stin);

// Save vehicle data to stream
    void save (std::ofstream &stout);

    using JsonSerializer::serialize;
    void serialize(JsonOut &jsout) const;
    using JsonDeserializer::deserialize;
    void deserialize(JsonIn &jsin);

    // convert epower (watts) to power.
    int epower_to_power (int epower);

    // convert power to epower (watts).
    int power_to_epower (int power);

    // Calculates combined power of all engines, returns current power as if engines were on
    int engine_power( int *r_power_max = 0, int *r_epower = 0, int *r_alt_power = 0 );

// Operate vehicle
    void use_controls();

// Start the vehicle's engine, if there are any
    void start_engine();

// Honk the vehicle's horn, if there are any
    void honk_horn();

// get vpart type info for part number (part at given vector index)
    vpart_info& part_info (int index, bool include_removed = false);

// check if certain part can be mounted at certain position (not accounting frame direction)
    bool can_mount (int dx, int dy, std::string id);

// check if certain part can be unmounted
    bool can_unmount (int p);

// install a new part to vehicle (force to skip possibility check)
    int install_part (int dx, int dy, std::string id, int hp = -1, bool force = false);

    bool remove_part (int p);
    void part_removal_cleanup ();

    void break_part_into_pieces (int p, int x, int y, bool scatter = false);

// Generate the corresponding item from a vehicle part.
// Still needs to be removed.
    item item_from_part( int part );

// translate item health to part health
    void get_part_properties_from_item (int partnum, item& i);
// translate part health to item health (very lossy.)
    void give_part_properties_to_item (int partnum, item& i);

// returns the list of indeces of parts at certain position (not accounting frame direction)
    const std::vector<int> parts_at_relative (const int dx, const int dy, bool use_cache = true);

// returns index of part, inner to given, with certain flag, or -1
    int part_with_feature (int p, const std::string &f, bool unbroken = true);
    int part_with_feature (int p, const vpart_bitflags &f, bool unbroken = true);

    /**
     *  Return the index of the next part to open at `p`'s location
     *
     *  The next part to open is the first unopened part in the reversed list of
     *  parts at part `p`'s coordinates.
     *
     *  @param outside If true, give parts that can be opened from outside only
     *  @return part index or -1 if no part
     */
    int next_part_to_open (int p, bool outside = false);

    /**
     *  Return the index of the next part to close at `p`
     *
     *  The next part to open is the first opened part in the list of
     *  parts at part `p`'s coordinates. Returns -1 for no more to close.
     *
     *  @param outside If true, give parts that can be closed from outside only
     *  @return part index or -1 if no part
     */
    int next_part_to_close (int p, bool outside = false);

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
    int index_of_part(vehicle_part *part, bool check_removed = false);

// get symbol for map
    char part_sym (int p);
    std::string part_id_string(int p, char &part_mod);


// Vehicle parts description
    int print_part_desc (WINDOW *win, int y1, int width, int p, int hl = -1);

// Vehicle fuel indicator. Should probably rename to print_fuel_indicators and make a print_fuel_indicator(..., FUEL_TYPE);
    void print_fuel_indicator (void *w, int y, int x, bool fullsize = false,
                               bool verbose = false, bool desc = false);

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

// Checks how much certain fuel left in tanks.
    int fuel_left (const ammotype & ftype);
    int fuel_capacity (const ammotype & ftype);

    // refill fuel tank(s) with given type of fuel
    // returns amount of leftover fuel
    int refill (const ammotype & ftype, int amount);

    // drains a fuel type (e.g. for the kitchen unit)
    // returns amount actually drained, does not engage reactor
    int drain (const ammotype & ftype, int amount);

// fuel consumption of vehicle engines of given type, in one-hundreth of fuel
    int basic_consumption (const ammotype & ftype);

    void consume_fuel( double load );

// get color for map
    nc_color part_color (int p);

    void charge_battery (int amount);

    int discharge_battery (int amount);

// get the total mass of vehicle, including cargo and passengers
    int total_mass ();

// get center of mass of vehicle; coordinates are precalc_dx[0] and precalc_dy[0]
    void center_of_mass(int &x, int &y);

// Get combined epower of solar panels
    int solar_epower ();

    // Change the facing direction of a headlight
    bool change_headlight_direction(int p);

    // Calculates air_resistance and downforce
    void calculate_air_resistance();

    // Calculate the forces currently acting on the vehicle and store them in struct vf
    // v is velocity to calculate at. returns net force of newtons acting on vehicle
    double calculate_forces( vehicle_forces *vf, double v, double fdir );

    // Sum up the forces working over a single tile where fdir=engine thrust/braking,
    // reiterate with different time values until the correct time is found,
    // store them in vf, and return calculated vehicle velocity
    double calculate_time_to_move_one_tile( vehicle_forces *vf, double fdir );

    // Calculate factors and coefficiencies from current tiles/temperature/grip etc,
    // store them in vf, and then figure out what parameters (time/thrust) to use
    // in order to move exactly 1 tile length and end up at the desired velocity.
    // Also used by veh_interact to display stuff
    double calculate_movement( vehicle_forces *vf, double engine_output, double target_speed = 0.0, int tile_move_cost = 2 );

    // Generate smoke from a part, either at front or back of vehicle depending on velocity.
    void spew_smoke( double joules, int part );

    // Loop through engines and generate noise and smoke for each one
    void noise_and_smoke( double load, double time = 6.0 );

// Calculate area covered by wheels and, optionally count number of wheels
    float wheels_area (int *cnt = 0);

// calculate if it can move using its wheels configuration
    bool valid_wheel_config ();

// Vehicle loses control and starts to skid
    void start_skid (int turn_deg = 0);
// idle fuel consumption
    void idle ();

// thrust (1) or brake (-1) vehicle
    void thrust (int thd);

// depending on skid vectors, chance to recover.
    void possibly_recover_from_skid ();

//forward component of velocity.
    float forward_velocity();

// cruise control
    void cruise_thrust (int amount);

// turn vehicle left (negative) or right (positive), degrees
    void turn (int deg);

    // Handles all collisions between this vehicle and other stuff at the given delta.
    bool collision (int dx, int dy, bool just_detect);

// Slow down vehicle, returns true if vehicle stopped completely
    bool slow_down (int vel_dec);

// Accelerate, spend fuel, turn etc. But don't move, map does that for us.
    void do_turn_actions();

// Move through current maptile. Handle friction/wheel damage etc. Called by map. Return false if failed.
    bool drive_one_tile ();

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

// reduces velocity to 0
    void stop ();

    void refresh_insides ();

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

    //Shifts the coordinates of all parts and moves the vehicle in the opposite direction.
    void shift_parts(const int dx, const int dy);
    bool shift_if_needed();

    void leak_fuel (int p);

    // Cycle through available turret modes
    void cycle_turret_mode();

    // fire the turret which is part p
    bool fire_turret( int p, bool burst = true );

    // internal procedure of turret firing
    bool fire_turret_internal (int p, it_gun &gun, it_ammo &ammo, long charges,
                               const std::string &firing_sound = "");

    // opens/closes doors or multipart doors
    void open(int part_index);
    void close(int part_index);

    /**
     *  Opens everything that can be opened on the same tile as `p`
     */
    void open_all_at(int p);

    // upgrades/refilling/etc. see veh_interact.cpp
    void interact ();

    // return a vector w/ 'direction' & 'magnitude', in its own sense of the words.
    rl_vec2d velo_vec();
    //normalized vectors, from tilerays face & move
    rl_vec2d face_vec();
    rl_vec2d move_vec();

    // cached values, should in theory be correct, only recalculated occasionally in refresh()
    int cached_mass;                // Total mass, becomes inaccurate if player removes cargo, parts fall off etc, in kg
    float drag_coeff;               // Cd * A, includes skin friction, form drag, and interference drag, dimensionless
    float downforce;                // Cl * A, in m^2

    double turn_delta_per_tile;     // Used for turning (or spinning out of control). Equals degress to turn every tile.
    double target_turn_delta;       // To keep track of turning within a single game turn.

    int player_thrust;              // Direction to go, from -100 to 100 in percentage of max. Can mean either acccelerating or braking

    // config values
    std::string name;   // vehicle name
    std::string type;           // vehicle type
    std::vector<vehicle_part> parts;   // Parts which occupy different tiles
    int removed_part_count;            // Subtract from parts.size() to get the real part count.
    std::map<point, std::vector<int> > relative_parts;    // parts_at_relative(x,y) is used alot (to put it mildly)
    std::vector<int> lights;           // List of light part indices
    std::vector<int> alternators;      // List of alternator indices
    std::vector<int> fuel;             // List of fuel tank indices
    std::vector<int> engines;          // List of engine indices
    std::vector<int> reactors;         // List of reactor indices
    std::vector<int> solar_panels;     // List of solar panel indices
    std::vector<int> wheelcache;
    std::vector<vehicle_item_spawn> item_spawns; //Possible starting items
    std::set<std::string> tags;        // Properties of the vehicle

    // temp values
    int smx, smy;   // submap coords. WARNING: must ALWAYS correspond to sumbap coords in grid, or i'm out
    bool insides_dirty; // if true, then parts' "inside" flags are outdated and need refreshing
    int init_veh_fuel;
    int init_veh_status;
    float alternator_load;

    // save values
    int posx, posy;
    int levx,levy;       // vehicle map coordinates.
    tileray face;       // frame direction
    tileray move;       // direction we are moving
    int velocity;       // vehicle current velocity, mph * 100
    int cruise_velocity; // velocity vehicle's cruise control trying to acheive
    bool cruise_on;     // cruise control on/off
    bool reactor_on;    // reactor on/off
    bool engine_on;     // engine on/off
    bool has_pedals;
    bool lights_on;     // lights on/off
    bool tracking_on;        // vehicle tracking on/off
    int om_id;          // id of the om_vehicle struct corresponding to this vehicle
    bool overhead_lights_on; //circle lights on/off
    bool fridge_on;     //fridge on/off
    bool recharger_on;  //recharger on/off
    int turn_dir;       // direction, to wich vehicle is turning (player control). will rotate frame on next move
    bool skidding;      // skidding mode
    int last_turn;      // amount of last turning (for calculate skidding due to handbrake)
    //int moves;
    float of_turn;      // goes from ~1 to ~0 while proceeding every turn
    float of_turn_carry;// leftover from prev. turn
    int turret_mode;    // turret firing mode: 0 = off, 1 = burst fire
    int lights_epower;   // total power of components with LIGHT or CONE_LIGHT flag
    int overhead_epower;   // total power of components with CIRCLE_LIGHT flag
    int tracking_epower; // total power consumed by tracking devices (why would you use more than one?)
    int fridge_epower; // total power consumed by fridges
    int recharger_epower; // total power consumed by rechargers
    bool check_environmental_effects; // True if it has bloody or smoking parts
};

#endif
