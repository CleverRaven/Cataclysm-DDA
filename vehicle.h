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
 veh_collision(){};
};


/**
 * Structure, describing vehicle part (ie, wheel, seat)
 */
struct vehicle_part
{
    vehicle_part() : id(vp_null), mount_dx(0), mount_dy(0), hp(0),
      blood(0), bigness(0), inside(false), flags(0), passenger_id(0)
    {
        precalc_dx[0] = precalc_dx[1] = -1;
        precalc_dy[0] = precalc_dy[1] = -1;
    }
    bool has_flag( int flag ) { return flag & flags; }
    int set_flag( int flag ) { return flags |= flag; }
    int remove_flag( int flag ) { return flags &= ~flag; }

    static const int passenger_flag = 1;

    vpart_id id;            // id in list of parts (vpart_list index)
    int mount_dx;           // mount point on the forward/backward axis
    int mount_dy;           // mount point on the left/right axis
    int precalc_dx[2];      // mount_dx translated to face.dir [0] and turn_dir [1]
    int precalc_dy[2];      // mount_dy translated to face.dir [0] and turn_dir [1]
    int hp;                 // current durability, if 0, then broken
    int blood;              // how much blood covers part (in turns). only useful for external
    int bigness;            // size of engine, wheel radius, translates to item properties.
    bool inside;            // if tile provides cover. WARNING: do not read it directly, use vehicle::is_inside() instead
    int flags;
    int passenger_id;       // carrying passenger
    union
    {
        int amount;         // amount of fuel for tank
        int open;           // door is open
        int direction;      // direction the part is facing
    };
    std::vector<item> items;// inventory

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
 *   given mount coords. First one is considered external,
 *   others are internal (or, as special case, "over" -- like trunk)
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
 * - Some methods take `part` or `p` parameter. Some of them
 *   assume that's external part number, and all internal parts
 *   at this mount point are affected. There is separate
 *   vector in which a list of external parts is stored,
 *   it must correspond to actual list of external parts
 *   (assure this if you add/remove parts programmatically).
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
 *   There are a few rules: some parts are external, so one should be the first part
 *   at given mount point (tile). They require some part in neighbouring tile (with the "MOUNT_POINT" flag) to
 *   be mounted to. Other parts are internal or placed over. They can only be installed on top
 *   of external part. Some functional parts can be only in single instance per tile, i. e.,
 *   no two engines at one mount point.
 *   If you can't understand why installation fails, try to assemble your vehicle in game first.
 */
class vehicle
{
private:
    game *g;

    bool can_stack_vpart_flag(std::string vpart_flag);

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
    void load (std::ifstream &stin);

// Save vehicle data to stream
    void save (std::ofstream &stout);

    void json_load( picojson::value & parsed, game * g );

    picojson::value json_save( bool save_contents = true );
// Operate vehicle
    void use_controls();

// get vpart type info for part number (part at given vector index)
    vpart_info& part_info (int index);

// get vpart powerinfo for part number, accounting for variable-sized parts.
    int part_power (int index);

// check if certain part can be mounted at certain position (not accounting frame direction)
    bool can_mount (int dx, int dy, vpart_id id);

// check if certain external part can be unmounted
    bool can_unmount (int p);

// install a new part to a vehicle - initialization only
    bool install_part (int dx, int dy, std::string vpart_info_id);

// install a new part to vehicle (force to skip possibility check)
    int install_part (int dx, int dy, vpart_id id, int hp = -1, bool force = false);

    void remove_part (int p);

// Generate the corresponding item from a vehicle part.
// Still needs to be removed.
    item item_from_part( int part );

// translate item health to part health
    void get_part_properties_from_item (game* g, int partnum, item& i);
// translate part health to item health (very lossy.)
    void give_part_properties_to_item (game* g, int partnum, item& i);

// returns the list of indeces of parts at certain position (not accounting frame direction)
    std::vector<int> parts_at_relative (int dx, int dy);

// returns the list of indeces of parts inside (or over) given
    std::vector<int> internal_parts (int p);

// returns index of part, inner to given, with certain flag, or -1
    int part_with_feature (int p, const std::string &f, bool unbroken = true);

// returns true if given flag is present for given part index
    bool part_flag (int p, const std::string &f);

// Translate seat-relative mount coords into tile coords
    void coord_translate (int reldx, int reldy, int &dx, int &dy);

// Translate seat-relative mount coords into tile coords using given face direction
    void coord_translate (int dir, int reldx, int reldy, int &dx, int &dy);

// Seek a vehicle part which obstructs tile with given coords relative to vehicle position
    int part_at (int dx, int dy);
    int global_part_at (int x, int y);

// get symbol for map
    char part_sym (int p);

// get color for map
    nc_color part_color (int p);

// Vehicle parts description
    void print_part_desc (void *w, int y1, int width, int p, int hl = -1);

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

// Checks how much certain fuel left in tanks. If for_engine == true that means
// ftype == "battery" is also takes in account "plutonium" fuel (electric motors can use both)
    int fuel_left (ammotype ftype, bool for_engine = false);
    int fuel_capacity (ammotype ftype);

    // refill fuel tank(s) with given type of fuel
    // returns amount of leftover fuel
    int refill (ammotype ftype, int amount);

    // drains a fuel type (e.g. for the kitchen unit)
    // returns amount actually drained, does not engage reactor
    int drain (ammotype ftype, int amount);

// fuel consumption of vehicle engines of given type, in one-hundreth of fuel
    int basic_consumption (ammotype ftype);

    void consume_fuel ();

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

    bool collision( std::vector<veh_collision> &veh_veh_colls, int dx, int dy,
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

    void gain_moves (int mp);

// reduces velocity to 0
    void stop ();

    void find_external_parts ();

    void find_exhaust ();

    void refresh_insides ();

    bool is_inside (int p);

    void unboard_all ();

    // damage types:
    // 0 - piercing
    // 1 - bashing (damage applied if it passes certain treshold)
    // 2 - incendiary
    // damage individual external part. bash means damage
    // must exceed certain threshold to be substracted from hp
    // (a lot light collisions will not destroy parts)
    // returns damage bypassed
    int damage (int p, int dmg, int type = 1, bool aimed = true);

    // damage all parts (like shake from strong collision), range from dmg1 to dmg2
    void damage_all (int dmg1, int dmg2, int type = 1);

    // direct damage to part (armor protection and internals are not counted)
    // returns damage bypassed
    int damage_direct (int p, int dmg, int type = 1);

    void leak_fuel (int p);

    // fire the turret which is part p
    void fire_turret (int p, bool burst = true);

    // internal procedure of turret firing
    bool fire_turret_internal (int p, it_gun &gun, it_ammo &ammo, int charges);

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
    std::vector<int> external_parts;   // List of external parts indeces
    std::set<std::string> tags;        // Properties of the vehicle
    int exhaust_dx;
    int exhaust_dy;

    // temp values
    int smx, smy;   // submap coords. WARNING: must ALWAYS correspond to sumbap coords in grid, or i'm out
    bool insides_dirty; // if true, then parts' "inside" flags are outdated and need refreshing
    int init_veh_fuel;
    int init_veh_status;

    // save values
    int posx, posy;
    tileray face;       // frame direction
    tileray move;       // direction we are moving
    int velocity;       // vehicle current velocity, mph * 100
    int cruise_velocity; // velocity vehicle's cruise control trying to acheive
    bool cruise_on;     // cruise control on/off
    bool lights_on;     // lights on/off
    int turn_dir;       // direction, to wich vehicle is turning (player control). will rotate frame on next move
    bool skidding;      // skidding mode
    int last_turn;      // amount of last turning (for calculate skidding due to handbrake)
    //int moves;
    float of_turn;      // goes from ~1 to ~0 while proceeding every turn
    float of_turn_carry;// leftover from prev. turn
    int turret_mode;    // turret firing mode: 0 = off, 1 = burst fire
};

#endif
