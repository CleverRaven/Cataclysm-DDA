#ifndef VEHICLE_H
#define VEHICLE_H

#include "calendar.h"
#include "tileray.h"
#include "color.h"
#include "damage.h"
#include "item.h"
#include "line.h"
#include "item_stack.h"
#include "active_item_cache.h"
#include "string_id.h"
#include "int_id.h"

#include <vector>
#include <array>
#include <map>
#include <list>
#include <string>
#include <iosfwd>

class map;
class player;
class vehicle;
struct vpart_info;
enum vpart_bitflags : int;
using vpart_id = int_id<vpart_info>;
using vpart_str_id = string_id<vpart_info>;
struct vehicle_prototype;
using vproto_id = string_id<vehicle_prototype>;

//collision factor for vehicle-vehicle collision; delta_v in mph
float get_collision_factor(float delta_v);

//How far to scatter parts from a vehicle when the part is destroyed (+/-)
constexpr int SCATTER_DISTANCE = 3;
constexpr int k_mvel = 200; //adjust this to balance collision damage

struct fuel_type {
    /** Id of the item type that represents the fuel. It may not be valid for certain pseudo
     * fuel types like muscle. */
    itype_id id;
    /** Color when displaying information about. */
    nc_color color;
    /** See @ref vehicle::consume_fuel */
    int coeff;
    /** Factor is used when transforming from item charges to fuel amount. */
    int charges_to_amount_factor;
};

const std::array<fuel_type, 7> &get_fuel_types();
int fuel_charges_to_amount_factor( const itype_id &ftype );

enum veh_coll_type : int {
    veh_coll_nothing,  // 0 - nothing,
    veh_coll_body,     // 1 - monster/player/npc
    veh_coll_veh,      // 2 - vehicle
    veh_coll_bashable, // 3 - bashable
    veh_coll_other,    // 4 - other
    num_veh_coll_types
};

// Saved to file as int, so values should not change
enum turret_mode_type : int {
    turret_mode_off = 0,
    turret_mode_autotarget = 1,
    turret_mode_manual = 2
};

// Describes turret's ability to fire (possibly at a particular target)
enum turret_fire_ability {
    turret_all_ok,
    turret_wont_aim,
    turret_is_off,
    turret_out_of_range,
    turret_no_ammo
};

struct veh_collision {
    //int veh?
    int           part        = 0;
    veh_coll_type type        = veh_coll_nothing;
    int           imp         = 0; // impulse
    void         *target      = nullptr;  //vehicle
    int           target_part = 0; //veh partnum
    std::string   target_name;

    veh_collision() = default;
};

class vehicle_stack : public item_stack {
private:
    std::list<item> *mystack;
    point location;
    vehicle *myorigin;
    int part_num;
public:
vehicle_stack( std::list<item> *newstack, point newloc, vehicle *neworigin, int part ) :
    mystack(newstack), location(newloc), myorigin(neworigin), part_num(part) {};
    size_t size() const override;
    bool empty() const override;
    std::list<item>::iterator erase( std::list<item>::iterator it ) override;
    void push_back( const item &newitem ) override;
    void insert_at( std::list<item>::iterator index, const item &newitem ) override;
    std::list<item>::iterator begin();
    std::list<item>::iterator end();
    std::list<item>::const_iterator begin() const;
    std::list<item>::const_iterator end() const;
    std::list<item>::reverse_iterator rbegin();
    std::list<item>::reverse_iterator rend();
    std::list<item>::const_reverse_iterator rbegin() const;
    std::list<item>::const_reverse_iterator rend() const;
    item &front() override;
    item &operator[]( size_t index ) override;
};

/**
 * Structure, describing vehicle part (ie, wheel, seat)
 */
struct vehicle_part : public JsonSerializer, public JsonDeserializer
{
    friend vehicle;
    enum : int { passenger_flag = 1 };

    vehicle_part( int dx = 0, int dy = 0 );
    vehicle_part( const vpart_str_id &sid, int dx = 0, int dy = 0, const item *it = nullptr );

    bool has_flag(int const flag) const noexcept { return flag & flags; }
    int  set_flag(int const flag)       noexcept { return flags |= flag; }
    int  remove_flag(int const flag)    noexcept { return flags &= ~flag; }

private:
    vpart_id id;         // id in map of parts (vehicle_part_types key)
public:
    point mount;                  // mount point: x is on the forward/backward axis, y is on the left/right axis
    std::array<point, 2> precalc; // mount translated to face.dir [0] and turn_dir [1]
    int hp           = 0;         // current durability, if 0, then broken
    int blood        = 0;         // how much blood covers part (in turns).
    int bigness      = 0;         // size of engine, wheel radius, translates to item properties.
    bool inside      = false;     // if tile provides cover. WARNING: do not read it directly, use vehicle::is_inside() instead
    bool removed     = false;     // true if this part is removed. The part won't disappear until the end of the turn
                                  // so our indices can remain consistent.
    bool enabled     = true;      //
    int flags        = 0;         //
    int passenger_id = 0;         // carrying passenger

    union {
        int amount;    // amount of fuel for tank/charge in battery
        int open;      // door is open
        int direction; // direction the part is facing
        int mode;      // turret mode
    };

    // Coordinates for some kind of target; jumper cables and turrets use this
    // Two coord pairs are stored: actual target point, and target vehicle center.
    // Both cases use absolute coordinates (relative to world origin)
    std::pair<tripoint, tripoint> target;
private:
    std::list<item> items; // inventory
public:
    void set_id( const vpart_str_id & str );
    const vpart_str_id &get_id() const;
    const vpart_info &info() const;

    // json saving/loading
    using JsonSerializer::serialize;
    void serialize(JsonOut &jsout) const override;
    using JsonDeserializer::deserialize;
    void deserialize(JsonIn &jsin) override;

    /**
     * Generate the corresponding item from this vehicle part. It includes
     * the hp (item damage), fuel charges (battery or liquids), bigness
     * aspect, ...
     */
    item properties_to_item() const;
    /**
     * Set members of this vehicle part from properties of the item.
     * It includes hp, fuel, bigness, ...
     */
    void properties_from_item( const item &used_item );
};

/**
 * Struct used for storing labels
 * (easier to json opposed to a std::map<point, std::string>)
 */
struct label : public JsonSerializer, public JsonDeserializer {
    label() = default;
    label(int const x, int const y) : x(x), y(y) {}
    label(const int x, const int y, std::string text) : x(x), y(y), text(std::move(text)) {}

    int         x = 0;
    int         y = 0;
    std::string text;

    // these are stored in a set
    bool operator<(const label &rhs) const noexcept {
        return (x != rhs.x) ? (x < rhs.x) : (y < rhs.y);
    }

    // json saving/loading
    using JsonSerializer::serialize;
    void serialize(JsonOut &jsout) const override;
    using JsonDeserializer::deserialize;
    void deserialize(JsonIn &jsin) override;
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
 *   and stored in `precalc[0]`. `precalc[1]` stores mount coords for
 *   next move (vehicle can move and turn). Method `map::displace_vehicle()`
 *   assigns `precalc[1]` to `precalc[0]`. At any time (except
 *   `map::vehmove()` innermost cycle) you can get actual part coords
 *   relative to vehicle's position by reading `precalc[0]`.
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
    bool has_structural_part(int dx, int dy) const;
    void open_or_close(int part_index, bool opening);
    bool is_connected(vehicle_part const &to, vehicle_part const &from, vehicle_part const &excluded) const;
    void add_missing_frames();

    // direct damage to part (armor protection and internals are not counted)
    // returns damage bypassed
    int damage_direct( int p, int dmg, damage_type type = DT_TRUE );
    //damages vehicle controls and security system
    void smash_security_system();
    // get vpart powerinfo for part number, accounting for variable-sized parts and hps.
    int part_power( int index, bool at_full_hp = false ) const;

    // get vpart epowerinfo for part number.
    int part_epower (int index) const;

    // convert epower (watts) to power.
    static int epower_to_power (int epower);

    // convert power to epower (watts).
    static int power_to_epower (int power);

    //Refresh all caches and re-locate all parts
    void refresh();

    // Do stuff like clean up blood and produce smoke from broken parts. Returns false if nothing needs doing.
    bool do_environmental_effects();

    int total_folded_volume() const;

    // Gets the fuel color for a given fuel
    nc_color get_fuel_color ( const itype_id &fuel_type ) const;

    // Whether a fuel indicator should be printed
    bool should_print_fuel_indicator (itype_id fuelType, bool fullsize) const;

    // Vehical fuel indicator (by fuel)
    void print_fuel_indicator (void *w, int y, int x, itype_id fuelType,
                               bool verbose = false, bool desc = false) const;

    // Calculate how long it takes to attempt to start an engine
    int engine_start_time( const int e );

    // How much does the temperature effect the engine starting (0.0 - 1.0)
    double engine_cold_factor( const int e );

    /**
     * Find a possibly off-map vehicle. If necessary, loads up its submap through
     * the global MAPBUFFER and pulls it from there. For this reason, you should only
     * give it the coordinates of the origin tile of a target vehicle.
     * @param where Location of the other vehicle's origin tile.
     */
    static vehicle* find_vehicle( const tripoint &where );

    /**
     * Traverses the graph of connected vehicles, starting from start_veh, and continuing
     * along all vehicles connected by some kind of POWER_TRANSFER part.
     * @param start_vehicle The vehicle to start traversing from. NB: the start_vehicle is
     * assumed to have been already visited!
     * @param amount An amount of power to traverse with. This is passed back to the visitor,
     * and reset to the visitor's return value at each step.
     * @param visitor A function(vehicle* veh, int amount, int loss) returning int. The function
     * may do whatever it desires, and may be a lambda (including a capturing lambda).
     * NB: returning 0 from a visitor will stop traversal immediately!
     * @return The last visitor's return value.
     */
    template <typename Func, typename Vehicle>
    static int traverse_vehicle_graph(Vehicle *start_veh, int amount, Func visitor);
public:
    vehicle(const vproto_id &type_id, int veh_init_fuel = -1, int veh_init_status = -1);
    vehicle();
    ~vehicle ();

    // check if given player controls this vehicle
    bool player_in_control(player const &p) const;
    // check if player controls this vehicle remotely
    bool remote_controlled(player const &p) const;

    // init parts state for randomly generated vehicle
    void init_state(int veh_init_fuel, int veh_init_status);

    // damages all parts of a vehicle by a random amount
    void smash();

    // load and init vehicle data from stream. This implies valid save data!
    void load (std::ifstream &stin);

    // Save vehicle data to stream
    void save (std::ofstream &stout);

    using JsonSerializer::serialize;
    void serialize(JsonOut &jsout) const override;
    using JsonDeserializer::deserialize;
    void deserialize(JsonIn &jsin) override;

    // Operate vehicle
    void use_controls(const tripoint &pos);

    // Fold up the vehicle
    bool fold_up();

    // Attempt to start an engine
    bool start_engine( const int e );

    // Attempt to start the vehicle's active engines
    void start_engines( const bool take_control = false );

    // Engine backfire, making a loud noise
    void backfire( const int e );

    // Honk the vehicle's horn, if there are any
    void honk_horn();
    void beeper_sound();
    void play_music();
    void play_chimes();
    void operate_planter();
    // get vpart type info for part number (part at given vector index)
    const vpart_info& part_info (int index, bool include_removed = false) const;

    // check if certain part can be mounted at certain position (not accounting frame direction)
    bool can_mount (int dx, int dy, const vpart_str_id &id) const;

    // check if certain part can be unmounted
    bool can_unmount (int p) const;

    // install a new part to vehicle (force to skip possibility check)
    int install_part (int dx, int dy, const vpart_str_id &id, int hp = -1, bool force = false);
    // Install a copy of the given part, skips possibility check
    int install_part (int dx, int dy, const vehicle_part &part);
    // install an item to vehicle as a vehicle part.
    int install_part (int dx, int dy, const vpart_str_id &id, const item &item_used);

    bool remove_part (int p);
    void part_removal_cleanup ();

    /**
     * Remove a part from a targeted remote vehicle. Useful for, e.g. power cables that have
     * a vehicle part on both sides.
     */
    void remove_remote_part(int part_num);

    std::string const& get_label(int x, int y) const;
    void set_label(int x, int y, std::string text);

    void break_part_into_pieces (int p, int x, int y, bool scatter = false);

    // returns the list of indeces of parts at certain position (not accounting frame direction)
    std::vector<int> parts_at_relative (int dx, int dy, bool use_cache = true) const;

    // returns index of part, inner to given, with certain flag, or -1
    int part_with_feature (int p, const std::string &f, bool unbroken = true) const;
    int part_with_feature (int p, vpart_bitflags f, bool unbroken = true) const;

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
    std::vector<int> all_parts_with_feature(const std::string &feature, bool unbroken = true) const;
    std::vector<int> all_parts_with_feature(vpart_bitflags f, bool unbroken = true) const;

    // returns indices of all parts in the given location slot
    std::vector<int> all_parts_at_location(const std::string &location) const;

    // returns true if given flag is present for given part index
    bool part_flag (int p, const std::string &f) const;
    bool part_flag (int p, vpart_bitflags f) const;

    // Returns the obstacle that shares location with this part (useful in some map code)
    // Open doors don't count as obstacles, but closed do
    // Broken parts are also never obstacles
    int obstacle_at_part( int p ) const;

    // Translate seat-relative mount coords into tile coords
    void coord_translate (int reldx, int reldy, int &dx, int &dy) const;

    // Translate seat-relative mount coords into tile coords using given face direction
    void coord_translate (int dir, int reldx, int reldy, int &dx, int &dy) const;

    // Seek a vehicle part which obstructs tile with given coords relative to vehicle position
    int part_at( int dx, int dy ) const;
    int global_part_at( int x, int y ) const;
    int global_part_at( const tripoint &p ) const;
    int part_displayed_at( int local_x, int local_y ) const;
    int roof_at_part( int p ) const;

    // Given a part, finds its index in the vehicle
    int index_of_part(const vehicle_part *part, bool check_removed = false) const;

    // get symbol for map
    char part_sym( int p, bool exact = false ) const;
    const vpart_str_id &part_id_string(int p, char &part_mod) const;

    // get color for map
    nc_color part_color( int p, bool exact = false ) const;

    // Vehicle parts description
    int print_part_desc (WINDOW *win, int y1, int width, int p, int hl = -1) const;

    // Get all printable fuel types
    std::vector< itype_id > get_printable_fuel_types (bool fullsize) const;

    // Vehicle fuel indicators (all of them)
    void print_fuel_indicators (void *w, int y, int x, int startIndex = 0, bool fullsize = false,
                               bool verbose = false, bool desc = false, bool isHorizontal = false) const;

    // Precalculate mount points for (idir=0) - current direction or (idir=1) - next turn direction
    void precalc_mounts (int idir, int dir);

    // get a list of part indeces where is a passenger inside
    std::vector<int> boarded_parts() const;

    // get passenger at part p
    player *get_passenger (int p) const;

    /**
     * Get the coordinates (in map squares) of this vehicle, it's the same
     * coordinate system that player::posx uses.
     * Global apparently means relative to the currently loaded map (game::m).
     * This implies:
     * <code>g->m.veh_at(this->global_x(), this->global_y()) == this;</code>
     */
    int global_x() const;
    int global_y() const;
    point global_pos() const;
    tripoint global_pos3() const;
    /**
     * Really global absolute coordinates in map squares.
     * This includes the overmap, the submap, and the map square.
     */
    point real_global_pos() const;
    tripoint real_global_pos3() const;
    /**
     * All the fuels that are in all the tanks in the vehicle, nicely summed up.
     * Note that empty tanks don't count at all. The value is the amout as it would be
     * reported by @ref fuel_left, it is always greater than 0. The key is the fuel item type.
     */
    std::map<itype_id, long> fuels_left() const;

    // Checks how much certain fuel left in tanks.
    int fuel_left (const itype_id &ftype, bool recurse = false) const;
    int fuel_capacity (const itype_id &ftype) const;

    // refill fuel tank(s) with given type of fuel
    // returns amount of leftover fuel
    int refill (const itype_id &ftype, int amount);

    // drains a fuel type (e.g. for the kitchen unit)
    // returns amount actually drained, does not engage reactor
    int drain (const itype_id &ftype, int amount);

    // fuel consumption of vehicle engines of given type, in one-hundreth of fuel
    int basic_consumption (const itype_id &ftype) const;

    void consume_fuel( double load );

    void power_parts();

    /**
     * Try to charge our (and, optionally, connected vehicles') batteries by the given amount.
     * @return amount of charge left over.
     */
    int charge_battery (int amount, bool recurse = true);

    /**
     * Try to discharge our (and, optionally, connected vehicles') batteries by the given amount.
     * @return amount of request unfulfilled (0 if totally successful).
     */
    int discharge_battery (int amount, bool recurse = true);

    // get the total mass of vehicle, including cargo and passengers
    int total_mass () const;

    // get center of mass of vehicle; coordinates are precalc[0]
    void center_of_mass(int &x, int &y) const;

    // Get combined power of all engines. If fueled == true, then only engines which
    // vehicle have fuel for are accounted
    int total_power (bool fueled = true) const;

    // Get acceleration gained by combined power of all engines. If fueled == true, then only engines which
    // vehicle have fuel for are accounted
    int acceleration (bool fueled = true) const;

    // Get maximum velocity gained by combined power of all engines. If fueled == true, then only engines which
    // vehicle have fuel for are accounted
    int max_velocity (bool fueled = true) const;

    // Get safe velocity gained by combined power of all engines. If fueled == true, then only engines which
    // vehicle have fuel for are accounted
    int safe_velocity (bool fueled = true) const;

    // Generate smoke from a part, either at front or back of vehicle depending on velocity.
    void spew_smoke( double joules, int part );

    // Loop through engines and generate noise and smoke for each one
    void noise_and_smoke( double load, double time = 6.0 );

    // Calculate area covered by wheels and, optionally count number of wheels
    float wheels_area (int *cnt = 0) const;

    // Combined coefficient of aerodynamic and wheel friction resistance of vehicle, 0-1.0.
    // 1.0 means it's ideal form and have no resistance at all. 0 -- it won't move
    float k_dynamics () const;

    // Components of the dynamic coefficient
    float k_friction () const;
    float k_aerodynamics () const;

    // Coefficient of mass, 0-1.0.
    // 1.0 means mass won't slow vehicle at all, 0 - it won't move
    float k_mass () const;

    // Extra drag on the vehicle from components other than wheels.
    float drag() const;

    // strain of engine(s) if it works higher that safe speed (0-1.0)
    float strain () const;

    // calculate if it can move using its wheels configuration
    bool sufficient_wheel_config() const;
    bool balanced_wheel_config() const;
    bool valid_wheel_config() const;

    // idle fuel consumption
    void idle(bool on_map = true);
    // continuous processing for running vehicle alarms
    void alarm();
    // leak from broken tanks
    void slow_leak();

    // thrust (1) or brake (-1) vehicle
    void thrust (int thd);

    // depending on skid vectors, chance to recover.
    void possibly_recover_from_skid();

    //forward component of velocity.
    float forward_velocity() const;

    // cruise control
    void cruise_thrust (int amount);

    // turn vehicle left (negative) or right (positive), degrees
    void turn (int deg);

    // Returns if any collision occured
    bool collision( std::vector<veh_collision> &colls,
                    const tripoint &dp,
                    bool just_detect, bool bash_floor = false );

    // Handle given part collision with vehicle, monster/NPC/player or terrain obstacle
    // Returns collision, which has type, impulse, part, & target.
    veh_collision part_collision( int part, const tripoint &p,
                                  bool just_detect, bool bash_floor );

    // Process the trap beneath
    void handle_trap( const tripoint &p, int part );

    int max_volume(int part) const; // stub for per-vpart limit
    int free_volume(int part) const;
    int stored_volume(int part) const;
    bool is_full(int part, int addvolume = -1, int addnumber = -1) const;

    // add item to part's cargo. if false, then there's no cargo at this part or cargo is full(*)
    // *: "full" means more than 1024 items, or max_volume(part) volume (500 for now)
    bool add_item( int part, item itm );
    // Position specific item insertion that skips a bunch of safety checks
    // since it should only ever be used by item processing code.
    bool add_item_at( int part, std::list<item>::iterator index, item itm );

    // remove item from part's cargo
    bool remove_item( int part, int itemdex );
    bool remove_item( int part, const item *it );
    std::list<item>::iterator remove_item (int part, std::list<item>::iterator it);

    vehicle_stack get_items( int part ) const;
    vehicle_stack get_items( int part );

    // Generates starting items in the car, should only be called when placed on the map
    void place_spawn_items();

    void gain_moves();

    // reduces velocity to 0
    void stop ();

    void refresh_insides ();

    bool is_inside (int p) const;

    void unboard_all ();

    // Damage individual part. bash means damage
    // must exceed certain threshold to be substracted from hp
    // (a lot light collisions will not destroy parts)
    // Returns damage bypassed
    int damage (int p, int dmg, damage_type type = DT_BASH, bool aimed = true);

    // damage all parts (like shake from strong collision), range from dmg1 to dmg2
    void damage_all (int dmg1, int dmg2, damage_type type, const point &impact);

    //Shifts the coordinates of all parts and moves the vehicle in the opposite direction.
    void shift_parts( point delta );
    bool shift_if_needed();

    void leak_fuel (int p);
    void shed_loose_parts();

    // Gets range of part p if it's a turret
    // If `manual` is true, gets the real item range (gun+ammo range)
    // otherwise gets part range (as in json)
    int get_turret_range( int p, bool manual = true );

    // Returns the number of shots this turret could make with current ammo/gas/batteries/etc.
    // Does not handle tags like FIRE_100
    class turret_ammo_data;
    turret_ammo_data turret_has_ammo( int p ) const;

    // Manual turret aiming menu (select turrets etc.) when shooting from controls
    // Returns whether a valid target was picked
    bool aim_turrets();

    // Maps turret ids to an enum describing their ability to shoot `pos`
    std::map< int, turret_fire_ability > turrets_can_shoot( const tripoint &pos );
    turret_fire_ability turret_can_shoot( const int p, const tripoint &pos );

    // Cycle mode for this turret
    // If `from_controls` is false, only manual modes are allowed
    // and message describing the new mode is printed
    void cycle_turret_mode( int p, bool from_controls );

    // Per-turret mode selection
    void control_turrets();

    // Cycle through available turret modes
    void cycle_global_turret_mode();

    // Set up the turret to fire
    bool fire_turret( int p, bool manual );

    // Fire turret at some automatically acquired target
    bool automatic_fire_turret( int p, const itype &gun, const itype &ammo, long &charges );

    // Manual turret fire - gives the `shooter` a temporary weapon, makes them use it,
    // then gives back the weapon held before (if any).
    // TODO: Make it work correctly with UPS-powered turrets when player has a UPS already
    bool manual_fire_turret( int p, player &shooter, const itype &guntype,
                             const itype &ammotype, long &charges );

    // Update the set of occupied points and return a reference to it
    std::set<tripoint> &get_points( bool force_refresh = false );

    // opens/closes doors or multipart doors
    void open(int part_index);
    void close(int part_index);

    // Consists only of parts with the FOLDABLE tag.
    bool is_foldable() const;
    // Restore parts of a folded vehicle.
    bool restore(const std::string &data);
    //handles locked vehicles interaction
    bool interact_vehicle_locked();
    //true if an alarm part is installed on the vehicle
    bool has_security_working() const;
    /**
     *  Opens everything that can be opened on the same tile as `p`
     */
    void open_all_at(int p);

    // upgrades/refilling/etc. see veh_interact.cpp
    void interact ();
    //scoop operation,pickups, battery drain, etc.
    void operate_scoop();
    void operate_reaper();
    void operate_plow();
    //main method for the control of individual engines
    void control_engines();
    // shows ui menu to select an engine
    int select_engine();
    //returns whether the engine is enabled or not, and has fueltype
    bool is_engine_type_on(int e, const itype_id &ft) const;
    //returns whether the engine is enabled or not
    bool is_engine_on(int e) const;
    //returns whether the part is enabled or not
    bool is_part_on(int p) const;
    //returns whether the engine uses specified fuel type
    bool is_engine_type(int e, const itype_id &ft) const;
    //returns whether there is an active engine at vehicle coordinates
    bool is_active_engine_at(int x, int y) const;
    //returns whether the alternator is operational
    bool is_alternator_on(int a) const;
    //mark engine as on or off
    void toggle_specific_engine(int p, bool on);
    void toggle_specific_part(int p, bool on);
    //true if an engine exists with specified type
    //If enabled true, this engine must be enabled to return true
    bool has_engine_type(const itype_id &ft, bool enabled) const;
    //true if an engine exists without the specified type
    //If enabled true, this engine must be enabled to return true
    bool has_engine_type_not(const itype_id &ft, bool enabled) const;
    //prints message relating to vehicle start failure
    void msg_start_engine_fail();
    //if necessary, damage this engine
    void do_engine_damage(size_t p, int strain);
    //remotely open/close doors
    void control_doors();
    // return a vector w/ 'direction' & 'magnitude', in its own sense of the words.
    rl_vec2d velo_vec() const;
    //normalized vectors, from tilerays face & move
    rl_vec2d face_vec() const;
    rl_vec2d move_vec() const;
    void on_move();
    /**
     * Update the submap coordinates smx, smy, and update the tracker info in the overmap
     * (if enabled).
     * This should be called only when the vehicle has actually been moved, not when
     * the map is just shifted (in the later case simply set smx/smy directly).
     */
    void set_submap_moved(int x, int y);

    const std::string disp_name();

    // config values
    std::string name;   // vehicle name
    /**
     * Type of the vehicle as it was spawned. This will never change, but it can be an invalid
     * type (e.g. if the definition of the prototype has been removed from json or if it has been
     * spawned with the default constructor).
     */
    vproto_id type;
    std::vector<vehicle_part> parts;   // Parts which occupy different tiles
    int removed_part_count;            // Subtract from parts.size() to get the real part count.
    std::map<point, std::vector<int> > relative_parts;    // parts_at_relative(x,y) is used alot (to put it mildly)
    std::set<label> labels;            // stores labels
    std::vector<int> lights;           // List of light part indices
    std::vector<int> alternators;      // List of alternator indices
    std::vector<int> fuel;             // List of fuel tank indices
    std::vector<int> engines;          // List of engine indices
    std::vector<int> reactors;         // List of reactor indices
    std::vector<int> solar_panels;     // List of solar panel indices
    std::vector<int> funnels;          // List of funnel indices
    std::vector<int> loose_parts;      // List of UNMOUNT_ON_MOVE parts
    std::vector<int> wheelcache;       // List of wheels
    std::vector<int> speciality;       // List of parts that will not be on a vehicle very often, or which only one will be present
    std::vector<int> floating;         // List of parts that provide buoyancy to boats
    std::set<std::string> tags;        // Properties of the vehicle

    active_item_cache active_items;

    /**
     * Submap coordinates of the currently loaded submap (see game::m)
     * that contains this vehicle. These values are changed when the map
     * shifts (but the vehicle is not actually moved than, it also stays on
     * the same submap, only the relative coordinates in map::grid have changed).
     * These coordinates must always refer to the submap in map::grid that contains
     * this vehicle.
     * When the vehicle is really moved (by map::displace_vehicle), set_submap_moved
     * is called and updates these values, when the map is only shifted or when a submap
     * is loaded into the map the values are directly set. The vehicles position does
     * not change therefor no call to set_submap_moved is required.
     */
    int smx, smy, smz;

    float alternator_load;
    calendar last_repair_turn = -1; // Turn it was last repaired, used to make consecutive repairs faster.

    // Points occupied by the vehicle
    std::set<tripoint> occupied_points;
    calendar occupied_cache_turn = -1; // Turn occupied points were calculated

    // Turn the vehicle was last processed
    calendar last_update_turn = -1;
    // Retroactively pass time spent outside bubble
    // Funnels, solars
    void update_time( const calendar &update_to );

    // save values
    /**
     * Position of the vehicle *inside* the submap that contains the vehicle.
     * This will (nearly) always be in the range (0...SEEX-1).
     * Note that vehicles are "moved" by map::displace_vehicle. You should not
     * set them directly, except when initializing the vehicle or during mapgen.
     */
    int posx = 0;
    int posy = 0;
    tileray face;       // frame direction
    tileray move;       // direction we are moving
    int velocity = 0;       // vehicle current velocity, mph * 100
    int cruise_velocity = 0; // velocity vehicle's cruise control trying to achieve
    int vertical_velocity = 0; // Only used for collisions, vehicle falls instantly
    std::string music_id;    // what music storage device is in the stereo
    int om_id;          // id of the om_vehicle struct corresponding to this vehicle
    int turn_dir;       // direction, to which vehicle is turning (player control). will rotate frame on next move

    int last_turn = 0;      // amount of last turning (for calculate skidding due to handbrake)
    float of_turn;      // goes from ~1 to ~0 while proceeding every turn
    float of_turn_carry;// leftover from prev. turn

    int turret_mode = 0;    // turret firing mode: 0 = off, 1 = burst fire

    int lights_epower       = 0; // total power of components with LIGHT or CONE_LIGHT flag
    int overhead_epower     = 0; // total power of components with CIRCLE_LIGHT flag
    int tracking_epower     = 0; // total power consumed by tracking devices (why would you use more than one?)
    int fridge_epower       = 0; // total power consumed by fridges
    int alarm_epower        = 0;
    int dome_lights_epower  = 0;
    int aisle_lights_epower = 0;
    int recharger_epower    = 0; // total power consumed by rechargers
    int camera_epower       = 0; // power consumed by camera system
    int extra_drag          = 0;
    int scoop_epower        = 0;
    // TODO: change these to a bitset + enum?
    bool cruise_on                  = true;  // cruise control on/off
    bool reactor_on                 = false; // reactor on/off
    bool engine_on                  = false; // at least one engine is on, of any type
    bool lights_on                  = false; // lights on/off
    bool stereo_on                  = false;
    bool chimes_on                  = false; // ice cream truck chimes
    bool tracking_on                = false; // vehicle tracking on/off
    bool is_locked                  = false; // vehicle has no key
    bool is_alarm_on                = false; // vehicle has alarm on
    bool camera_on                  = false;
    bool overhead_lights_on         = false; // circle lights on/off
    bool dome_lights_on             = false; // dome lights (rear view mirror lights)
    bool has_atomic_lights          = false; // has any always-on atomic lights on
    bool aisle_lights_on            = false; // aisle lights on
    bool fridge_on                  = false; // fridge on/off
    bool recharger_on               = false; // recharger on/off
    bool skidding                   = false; // skidding mode
    bool check_environmental_effects= false; // has bloody or smoking parts
    bool insides_dirty              = true;  // "inside" flags are outdated and need refreshing
    bool falling                    = false; // Is the vehicle hanging in the air and expected to fall down in the next turn?
    bool plow_on                    = false; // Is the vehicle running a plow?
    bool planter_on                 = false; // Is the vehicle sprawing seeds everywhere?
    bool scoop_on                   = false; //Does the vehicle have a scoop? Which picks up items.
    bool reaper_on                  = false; //Is the reaper active?
};

#endif
