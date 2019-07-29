#pragma once
#ifndef VEHICLE_H
#define VEHICLE_H

#include <climits>
#include <cstddef>
#include <array>
#include <map>
#include <stack>
#include <vector>
#include <functional>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>

#include "active_item_cache.h"
#include "calendar.h"
#include "colony.h"
#include "clzones.h"
#include "damage.h"
#include "game_constants.h"
#include "faction.h"
#include "item.h"
#include "item_group.h"
#include "item_stack.h"
#include "line.h"
#include "string_id.h"
#include "tileray.h"
#include "units.h"
#include "item_location.h"
#include "type_id.h"
#include "optional.h"
#include "point.h"

class monster;

class Creature;
class nc_color;
class player;
class npc;
class vehicle;
class vehicle_part_range;
class JsonIn;
class JsonOut;
class vehicle_cursor;
class zone_data;
struct itype;
struct uilist_entry;
template <typename T> class visitable;
class vpart_info;

enum vpart_bitflags : int;
enum ter_bitflags : int;
template<typename feature_type>
class vehicle_part_with_feature_range;

namespace catacurses
{
class window;
} // namespace catacurses
namespace vehicles
{
extern point cardinal_d[5];
// ratio of constant rolling resistance to the part that varies with velocity
constexpr double rolling_constant_to_variable = 33.33;
constexpr float vmiph_per_tile = 400.0f;
} // namespace vehicles
struct rider_data {
    Creature *psg = nullptr;
    int prt = -1;
    bool moved = false;
};
//collision factor for vehicle-vehicle collision; delta_v in mph
float get_collision_factor( float delta_v );

//How far to scatter parts from a vehicle when the part is destroyed (+/-)
constexpr int SCATTER_DISTANCE = 3;
//adjust this to balance collision damage
constexpr int k_mvel = 200;

enum class part_status_flag : int {
    any = 0,
    working = 1 << 0,
    available = 1 << 1,
    enabled = 1 << 2
};
part_status_flag inline operator|( const part_status_flag &rhs, const part_status_flag &lhs )
{
    return static_cast<part_status_flag>( static_cast<int>( lhs ) | static_cast<int>( rhs ) );
}
int inline operator&( const part_status_flag &rhs, const part_status_flag &lhs )
{
    return static_cast<int>( lhs ) & static_cast<int>( rhs );
}

enum veh_coll_type : int {
    veh_coll_nothing,  // 0 - nothing,
    veh_coll_body,     // 1 - monster/player/npc
    veh_coll_veh,      // 2 - vehicle
    veh_coll_bashable, // 3 - bashable
    veh_coll_other,    // 4 - other
    num_veh_coll_types
};

struct veh_collision {
    //int veh?
    int           part        = 0;
    veh_coll_type type        = veh_coll_nothing;
    int           imp         = 0; // impulse
    void         *target      = nullptr;  //vehicle
    int           target_part = 0; //vehicle partnum
    std::string   target_name;

    veh_collision() = default;
};

class vehicle_stack : public item_stack
{
    private:
        point location;
        vehicle *myorigin;
        int part_num;
    public:
        vehicle_stack( cata::colony<item> *newstack, point newloc, vehicle *neworigin, int part ) :
            item_stack( newstack ), location( newloc ), myorigin( neworigin ), part_num( part ) {}
        iterator erase( const_iterator it ) override;
        void insert( const item &newitem ) override;
        int count_limit() const override {
            return MAX_ITEM_IN_VEHICLE_STORAGE;
        }
        units::volume max_volume() const override;
};

struct bounding_box {
    point p1;
    point p2;
};

char keybind( const std::string &opt, const std::string &context = "VEHICLE" );

int mps_to_vmiph( double mps );
double vmiph_to_mps( int vmiph );
int cmps_to_vmiph( int cmps );
int vmiph_to_cmps( int vmiph );
static constexpr float accel_g = 9.81f;

/**
 * Structure, describing vehicle part (ie, wheel, seat)
 */
struct vehicle_part {
        friend vehicle;
        friend class veh_interact;
        friend visitable<vehicle_cursor>;
        friend item_location;
        friend class turret_data;

        enum : int { passenger_flag = 1,
                     animal_flag = 2,
                     carried_flag = 4,
                     carrying_flag = 8
                   };

        vehicle_part(); /** DefaultConstructible */

        vehicle_part( const vpart_id &vp, const point &dp, item &&it );

        /** Check this instance is non-null (not default constructed) */
        explicit operator bool() const;

        bool has_flag( const int flag ) const noexcept {
            return flag & flags;
        }
        int  set_flag( const int flag )       noexcept {
            return flags |= flag;
        }
        int  remove_flag( const int flag )    noexcept {
            return flags &= ~flag;
        }

        /**
         * Translated name of a part inclusive of any current status effects
         * with_prefix as true indicates the durability symbol should be prepended
         */
        std::string name( bool with_prefix = true ) const;

        static constexpr int name_offset = 7;
        /** Stack of the containing vehicle's name, when it it stored as part of another vehicle */
        std::stack<std::string, std::vector<std::string> > carry_names;

        /** Specific type of fuel, charges or ammunition currently contained by a part */
        itype_id ammo_current() const;

        /** Maximum amount of fuel, charges or ammunition that can be contained by a part */
        int ammo_capacity() const;

        /** Amount of fuel, charges or ammunition currently contained by a part */
        int ammo_remaining() const;

        /** Type of fuel used by an engine */
        itype_id fuel_current() const;
        /** Set an engine to use a different type of fuel */
        bool fuel_set( const itype_id &fuel );
        /**
         * Set fuel, charges or ammunition for this part removing any existing ammo
         * @param ammo specific type of ammo (must be compatible with vehicle part)
         * @param qty maximum ammo (capped by part capacity) or negative to fill to capacity
         * @return amount of ammo actually set or negative on failure
         */
        int ammo_set( const itype_id &ammo, int qty = -1 );

        /** Remove all fuel, charges or ammunition (if any) from this part */
        void ammo_unset();

        /**
         * Consume fuel, charges or ammunition (if available)
         * @param qty maximum amount of ammo that should be consumed
         * @param pos current global location of part from which ammo is being consumed
         * @return amount consumed which will be between 0 and specified qty
         */
        int ammo_consume( int qty, const tripoint &pos );

        /**
         * Consume fuel by energy content.
         * @param ftype Type of fuel to consume
         * @param energy_j Energy to consume, in J
         * @return Energy actually consumed, in J
         */
        double consume_energy( const itype_id &ftype, double energy_j );

        /* @retun true if part in current state be reloaded optionally with specific itype_id */
        bool can_reload( const item &obj = item() ) const;

        /**
         * If this part is capable of wholly containing something, process the
         * items in there.
         * @param pos Position of this part for item::process
         * @param e_heater Engine has a heater and is on
         */
        void process_contents( const tripoint &pos, const bool e_heater );

        /**
         *  Try adding @param liquid to tank optionally limited by @param qty
         *  @return whether any of the liquid was consumed (which may be less than qty)
         */
        bool fill_with( item &liquid, int qty = INT_MAX );

        /** Current faults affecting this part (if any) */
        const std::set<fault_id> &faults() const;

        /** Faults which could potentially occur with this part (if any) */
        std::set<fault_id> faults_potential() const;

        /** Try to set fault returning false if specified fault cannot occur with this item */
        bool fault_set( const fault_id &f );

        /** Get wheel diameter times wheel width (inches^2) or return 0 if part is not wheel */
        int wheel_area() const;

        /** Get wheel diameter (inches) or return 0 if part is not wheel */
        int wheel_diameter() const;

        /** Get wheel width (inches) or return 0 if part is not wheel */
        int wheel_width() const;

        /**
         *  Get NPC currently assigned to this part (seat, turret etc)?
         *  @note checks crew member is alive and currently allied to the player
         *  @return nullptr if no valid crew member is currently assigned
         */
        npc *crew() const;

        /** Set crew member for this part (seat, turret etc) who must be a player ally)
         *  @return true if part can have crew members and passed npc was suitable
         */
        bool set_crew( const npc &who );

        /** Remove any currently assigned crew member for this part */
        void unset_crew();

        /** Reset the target for this part. */
        void reset_target( const tripoint &pos );

        /**
         * @name Part capabilities
         *
         * A part can provide zero or more capabilities. Some capabilities are mutually
         * exclusive, for example a part cannot be both a fuel tank and battery
         */
        /*@{*/

        /** Can this part provide power or propulsion? */
        bool is_engine() const;

        /** Is this any type of vehicle light? */
        bool is_light() const;

        /** Can this part store fuel of any type
         * @skip_broke exclude broken parts
         */
        bool is_fuel_store( const bool skip_broke = true ) const;

        /** Can this part contain liquid fuels? */
        bool is_tank() const;

        /** Can this part store electrical charge? */
        bool is_battery() const;

        /** Is this part a reactor? */
        bool is_reactor() const;

        /** Can this part function as a turret? */
        bool is_turret() const;

        /** Can a player or NPC use this part as a seat? */
        bool is_seat() const;

        /*@}*/

    public:
        /** mount point: x is on the forward/backward axis, y is on the left/right axis */
        point mount;

        /** mount translated to face.dir [0] and turn_dir [1] */
        std::array<point, 2> precalc = { { point( -1, -1 ), point( -1, -1 ) } };

        /** current part health with range [0,durability] */
        int hp() const;

        /** Current part damage in same units as item::damage. */
        int damage() const;

        /** Current part damage level in same units as item::damage_level */
        int damage_level( int max ) const;

        /** Current part damage as a percentage of maximum, with 0.0 being perfect condition */
        double damage_percent() const;
        /** Current part health as a percentage of maximum, with 1.0 being perfect condition */
        double health_percent() const;

        /** parts are considered broken at zero health */
        bool is_broken() const;

        /** parts are unavailable if broken or if carried is true, if they have the CARRIED flag */
        bool is_unavailable( const bool carried = true ) const;
        /** parts are available if they aren't unavailable */
        bool is_available( const bool carried = true ) const;

        /** how much blood covers part (in turns). */
        int blood = 0;

        /**
         * if tile provides cover.
         * WARNING: do not read it directly, use vpart_position::is_inside() instead
         */
        bool inside = false;

        /**
         * true if this part is removed. The part won't disappear until the end of the turn
         * so our indices can remain consistent.
         */
        bool removed = false;
        bool enabled = true;
        int flags = 0;

        /** ID of player passenger */
        int passenger_id = 0;

        /** door is open */
        bool open = false;

        /** direction the part is facing */
        int direction = 0;

        /**
         * Coordinates for some kind of target; jumper cables and turrets use this
         * Two coordinate pairs are stored: actual target point, and target vehicle center.
         * Both cases use absolute coordinates (relative to world origin)
         */
        std::pair<tripoint, tripoint> target = { tripoint_min, tripoint_min };

    private:
        /** What type of part is this? */
        vpart_id id;

        /** As a performance optimization we cache the part information here on first lookup */
        mutable const vpart_info *info_cache = nullptr;

        item base;
        cata::colony<item> items; // inventory

        /** Preferred ammo type when multiple are available */
        itype_id ammo_pref = "null";

        /**
         *  What NPC (if any) is assigned to this part (seat, turret etc)?
         *  @see vehicle_part::crew() accessor which excludes dead and non-allied NPC's
         */
        int crew_id = -1;

    public:
        /** Get part definition common to all parts of this type */
        const vpart_info &info() const;

        void serialize( JsonOut &jsout ) const;
        void deserialize( JsonIn &jsin );

        const item &get_base() const;
        void set_base( const item &new_base );
        /**
         * Generate the corresponding item from this vehicle part. It includes
         * the hp (item damage), fuel charges (battery or liquids), aspect, ...
         */
        item properties_to_item() const;
        /**
         * Returns an ItemList of the pieces that should arise from breaking
         * this part.
         */
        item_group::ItemList pieces_for_broken_part() const;
};

class turret_data
{
        friend vehicle;

    public:
        turret_data() = default;
        turret_data( const turret_data & ) = delete;
        turret_data &operator=( const turret_data & ) = delete;
        turret_data( turret_data && ) = default;
        turret_data &operator=( turret_data && ) = default;

        /** Is this a valid instance? */
        explicit operator bool() const {
            return veh && part;
        }

        std::string name() const;

        /** Get base item location */
        item_location base();
        const item_location base() const;

        /** Quantity of ammunition available for use */
        int ammo_remaining() const;

        /** Maximum quantity of ammunition turret can itself contain */
        int ammo_capacity() const;

        /** Specific ammo data or returns nullptr if no ammo available */
        const itype *ammo_data() const;

        /** Specific ammo type or returns "null" if no ammo available */
        itype_id ammo_current() const;

        /** What ammo is available for this turret (may be multiple if uses tanks) */
        std::set<itype_id> ammo_options() const;

        /** Attempts selecting ammo type and returns true if selection was valid */
        bool ammo_select( const itype_id &ammo );

        /** Effects inclusive of any from ammo loaded from tanks */
        std::set<std::string> ammo_effects() const;

        /** Maximum range considering current ammo (if any) */
        int range() const;

        /**
         * Prepare the turret for firing, called by firing function.
         * This sets up vehicle tanks, recoil adjustments, vehicle rooftop status,
         * and performs any other actions that must be done before firing a turret.
         * @param p the player that is firing the gun, subject to recoil adjustment.
         */
        void prepare_fire( player &p );

        /**
         * Reset state after firing a prepared turret, called by the firing function.
         * @param p the player that just fired (or attempted to fire) the turret.
         * @param shots the number of shots fired by the most recent call to turret::fire.
         */
        void post_fire( player &p, int shots );

        /**
         * Fire the turret's gun at a given target.
         * @param p the player firing the turret, passed to pre_fire and post_fire calls.
         * @param target coordinates that will be fired on.
         * @return the number of shots actually fired (may be zero).
         */
        int fire( player &p, const tripoint &target );

        bool can_reload() const;
        bool can_unload() const;

        enum class status {
            invalid,
            no_ammo,
            no_power,
            ready
        };

        status query() const;

    private:
        turret_data( vehicle *veh, vehicle_part *part )
            : veh( veh ), part( part ) {}
        double cached_recoil;

    protected:
        vehicle *veh = nullptr;
        vehicle_part *part = nullptr;
};

/**
 * Struct used for storing labels
 * (easier to json opposed to a std::map<point, std::string>)
 */
struct label : public point {
    label() = default;
    label( const point &p ) : point( p ) {}
    label( const point &p, std::string text ) : point( p ), text( std::move( text ) ) {}

    std::string text;
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
 *   (`0, 0` in mount coordinates). There can be more than one part at
 *   given mount coordinates, and they are mounted in different slots.
 *   Check tileray.h file to see a picture of coordinate axes.
 * - Vehicle can be rotated to arbitrary degree. This means that
 *   mount coordinates are rotated to match vehicle's face direction before
 *   their actual positions are known. For optimization purposes
 *   mount coordinates are precalculated for current vehicle face direction
 *   and stored in `precalc[0]`. `precalc[1]` stores mount coordinates for
 *   next move (vehicle can move and turn). Method `map::displace_vehicle()`
 *   assigns `precalc[1]` to `precalc[0]`. At any time (except
 *   `map::vehmove()` innermost cycle) you can get actual part coordinates
 *   relative to vehicle's position by reading `precalc[0]`.
 *   Vehicles rotate around a (possibly changing) pivot point, and
 *   the precalculated coordinates always put the pivot point at (0,0).
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
 *   coordinates. If it shows debug messages that it can't add parts, when you start
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
class vehicle
{
    private:
        bool has_structural_part( const point &dp ) const;
        bool is_structural_part_removed() const;
        void open_or_close( int part_index, bool opening );
        bool is_connected( const vehicle_part &to, const vehicle_part &from,
                           const vehicle_part &excluded ) const;
        void add_missing_frames();
        void add_steerable_wheels();

        // direct damage to part (armor protection and internals are not counted)
        // returns damage bypassed
        int damage_direct( int p, int dmg, damage_type type = DT_TRUE );
        // Removes the part, breaks it into pieces and possibly removes parts attached to it
        int break_off( int p, int dmg );
        // Returns if it did actually explode
        bool explode_fuel( int p, damage_type type );
        //damages vehicle controls and security system
        void smash_security_system();
        // get vpart powerinfo for part number, accounting for variable-sized parts and hps.
        int part_vpower_w( int index, bool at_full_hp = false ) const;

        // get vpart epowerinfo for part number.
        int part_epower_w( int index ) const;

        // convert watts over time to battery energy
        int power_to_energy_bat( const int power_w, const int t_seconds ) const;

        // convert vhp to watts.
        static int vhp_to_watts( int power );

        //Refresh all caches and re-locate all parts
        void refresh();

        // Do stuff like clean up blood and produce smoke from broken parts. Returns false if nothing needs doing.
        bool do_environmental_effects();

        units::volume total_folded_volume() const;

        // Vehicle fuel indicator (by fuel)
        void print_fuel_indicator( const catacurses::window &w, int y, int x,
                                   const itype_id &fuelType,
                                   bool verbose = false, bool desc = false );
        void print_fuel_indicator( const catacurses::window &w, int y, int x,
                                   const itype_id &fuelType,
                                   std::map<itype_id, int> fuel_usages,
                                   bool verbose = false, bool desc = false );

        // Calculate how long it takes to attempt to start an engine
        int engine_start_time( const int e ) const;

        // How much does the temperature effect the engine starting (0.0 - 1.0)
        double engine_cold_factor( const int e ) const;

        // refresh pivot_cache, clear pivot_dirty
        void refresh_pivot() const;

        void refresh_mass() const;
        void calc_mass_center( bool precalc ) const;

        /** empty the contents of a tank, battery or turret spilling liquids randomly on the ground */
        void leak_fuel( vehicle_part &pt );

        /*
         * Fire turret at automatically acquired targets
         * @return number of shots actually fired (which may be zero)
         */
        int automatic_fire_turret( vehicle_part &pt );
        /**
         * Find a possibly off-map vehicle. If necessary, loads up its submap through
         * the global MAPBUFFER and pulls it from there. For this reason, you should only
         * give it the coordinates of the origin tile of a target vehicle.
         * @param where Location of the other vehicle's origin tile.
         */
        static vehicle *find_vehicle( const tripoint &where );

        /**
         * Traverses the graph of connected vehicles, starting from start_veh, and continuing
         * along all vehicles connected by some kind of POWER_TRANSFER part.
         * @param start_veh The vehicle to start traversing from. NB: the start_vehicle is
         * assumed to have been already visited!
         * @param amount An amount of power to traverse with. This is passed back to the visitor,
         * and reset to the visitor's return value at each step.
         * @param visitor A function(vehicle* veh, int amount, int loss) returning int. The function
         * may do whatever it desires, and may be a lambda (including a capturing lambda).
         * NB: returning 0 from a visitor will stop traversal immediately!
         * @return The last visitor's return value.
         */
        template <typename Func, typename Vehicle>
        static int traverse_vehicle_graph( Vehicle *start_veh, int amount, Func visitor );
    public:
        vehicle( const vproto_id &type_id, int veh_init_fuel = -1, int veh_init_status = -1 );
        vehicle();
        ~vehicle();

        /** Disable or enable refresh() ; used to speed up performance when creating a vehicle */
        void suspend_refresh();
        void enable_refresh();

        /**
         * Set stat for part constrained by range [0,durability]
         * @note does not invoke base @ref item::on_damage callback
         */
        void set_hp( vehicle_part &pt, int qty );

        /**
         * Apply damage to part constrained by range [0,durability] possibly destroying it
         * @param pt Part being damaged
         * @param qty maximum amount by which to adjust damage (negative permissible)
         * @param dt type of damage which may be passed to base @ref item::on_damage callback
         * @return whether part was destroyed as a result of the damage
         */
        bool mod_hp( vehicle_part &pt, int qty, damage_type dt = DT_NULL );

        // check if given player controls this vehicle
        bool player_in_control( const player &p ) const;
        // check if player controls this vehicle remotely
        bool remote_controlled( const player &p ) const;

        // init parts state for randomly generated vehicle
        void init_state( int veh_init_fuel, int veh_init_status );

        // damages all parts of a vehicle by a random amount
        void smash( float hp_percent_loss_min = 0.1f, float hp_percent_loss_max = 1.2f,
                    float percent_of_parts_to_affect = 1.0f, point damage_origin = point_zero,
                    float damage_size = 0 );

        void serialize( JsonOut &jsout ) const;
        void deserialize( JsonIn &jsin );
        // Vehicle parts list - all the parts on a single tile
        int print_part_list( const catacurses::window &win, int y1, int max_y, int width, int p,
                             int hl = -1, bool detail = false ) const;

        // Vehicle parts descriptions - descriptions for all the parts on a single tile
        void print_vparts_descs( const catacurses::window &win, int max_y, int width, int p,
                                 int &start_at, int &start_limit ) const;
        // owner functions
        void set_old_owner( const faction *temp_owner ) {
            theft_time = calendar::turn;
            old_owner = temp_owner;
        }
        void remove_old_owner() {
            theft_time = cata::nullopt;
            old_owner = nullptr;
        }
        void set_owner( faction *new_owner ) {
            owner = new_owner;
        }
        void remove_owner() {
            owner = nullptr;
        }
        const faction *get_owner() const {
            return owner;
        }
        const faction *get_old_owner() const {
            return old_owner;
        }
        bool has_owner() const {
            return owner;
        }
        bool handle_potential_theft( player &p, bool check_only = false, bool prompt = true );
        // project a tileray forward to predict obstacles
        std::set<point> immediate_path( int rotate = 0 );
        void do_autodrive();
        /**
         *  Operate vehicle controls
         *  @param pos location of physical controls to operate (ignored during remote operation)
         */
        void use_controls( const tripoint &pos );

        // Fold up the vehicle
        bool fold_up();

        // Attempt to start an engine
        bool start_engine( const int e );

        // Attempt to start the vehicle's active engines
        void start_engines( const bool take_control = false );

        // Engine backfire, making a loud noise
        void backfire( const int e ) const;

        // get vpart type info for part number (part at given vector index)
        const vpart_info &part_info( int index, bool include_removed = false ) const;

        // check if certain part can be mounted at certain position (not accounting frame direction)
        bool can_mount( const point &dp, const vpart_id &id ) const;

        // check if certain part can be unmounted
        bool can_unmount( int p ) const;
        bool can_unmount( int p, std::string &reason ) const;

        // install a new part to vehicle
        int install_part( const point &dp, const vpart_id &id, bool force = false );

        // Install a copy of the given part, skips possibility check
        int install_part( const point &dp, const vehicle_part &part );

        /** install item specified item to vehicle as a vehicle part */
        int install_part( const point &dp, const vpart_id &id, item &&obj, bool force = false );

        // find a single tile wide vehicle adjacent to a list of part indices
        bool try_to_rack_nearby_vehicle( const std::vector<std::vector<int>> &list_of_racks );
        // merge a previously found single tile vehicle into this vehicle
        bool merge_rackable_vehicle( vehicle *carry_veh, const std::vector<int> &rack_parts );

        bool remove_part( int p );
        void part_removal_cleanup();

        // remove the carried flag from a vehicle after it has bee removed from a rack
        void remove_carried_flag();
        // remove a vehicle specified by a list of part indices
        bool remove_carried_vehicle( const std::vector<int> &carried_vehicle );
        // split the current vehicle into up to four vehicles if they have no connection other
        // than the structure part at exclude
        bool find_and_split_vehicles( int exclude );
        // relocate passengers to the same part on a new vehicle
        void relocate_passengers( const std::vector<player *> &passengers );
        // remove a bunch of parts, specified by a vector indices, and move them to a new vehicle at
        // the same global position
        // optionally specify the new vehicle position and the mount points on the new vehicle
        bool split_vehicles( const std::vector<std::vector <int>> &new_vehs,
                             const std::vector<vehicle *> &new_vehicles,
                             const std::vector<std::vector <point>> &new_mounts );
        bool split_vehicles( const std::vector<std::vector <int>> &new_veh );

        /** Get handle for base item of part */
        item_location part_base( int p );

        /** Get index of part with matching base item or INT_MIN if not found */
        int find_part( const item &it ) const;

        /**
         * Remove a part from a targeted remote vehicle. Useful for, e.g. power cables that have
         * a vehicle part on both sides.
         */
        void remove_remote_part( int part_num );
        /**
         * Yields a range containing all parts (including broken ones) that can be
         * iterated over.
         */
        // TODO: maybe not include broken ones? Have a separate function for that?
        // TODO: rename to just `parts()` and rename the data member to `parts_`.
        vehicle_part_range get_all_parts() const;
        /**
         * Yields a range of parts of this vehicle that each have the given feature
         * and are available: not broken, removed, or part of a carried vehicle.
         * The enabled status of the part is ignored.
         */
        /**@{*/
        vehicle_part_with_feature_range<std::string> get_avail_parts( std::string feature ) const;
        vehicle_part_with_feature_range<vpart_bitflags> get_avail_parts( vpart_bitflags f ) const;
        /**@}*/
        /**
         * Yields a range of parts of this vehicle that each have the given feature
         * and are not broken or removed.
         * The enabled status of the part is ignored.
         */
        /**@{*/
        vehicle_part_with_feature_range<std::string> get_parts_including_carried(
            std::string feature ) const;
        vehicle_part_with_feature_range<vpart_bitflags> get_parts_including_carried(
            vpart_bitflags f ) const;
        /**@}*/
        /**
         * Yields a range of parts of this vehicle that each have the given feature and not removed.
         * The enabled status of the part is ignored.
         */
        /**@{*/
        vehicle_part_with_feature_range<std::string> get_any_parts( std::string feature ) const;
        vehicle_part_with_feature_range<vpart_bitflags> get_any_parts( vpart_bitflags f ) const;
        /**@}*/
        /**
         * Yields a range of parts of this vehicle that each have the given feature
         * and are enabled and available: not broken, removed, or part of a carried vehicle.
         */
        /**@{*/
        vehicle_part_with_feature_range<std::string> get_enabled_parts( std::string feature ) const;
        vehicle_part_with_feature_range<vpart_bitflags> get_enabled_parts( vpart_bitflags f ) const;
        /**@}*/

        // returns the list of indices of parts at certain position (not accounting frame direction)
        std::vector<int> parts_at_relative( const point &dp, bool use_cache ) const;

        // returns index of part, inner to given, with certain flag, or -1
        int part_with_feature( int p, const std::string &f, bool unbroken ) const;
        int part_with_feature( const point &pt, const std::string &f, bool unbroken ) const;
        int part_with_feature( int p, vpart_bitflags f, bool unbroken ) const;

        // returns index of part, inner to given, with certain flag, or -1
        int avail_part_with_feature( int p, const std::string &f, bool unbroken ) const;
        int avail_part_with_feature( const point &pt, const std::string &f, bool unbroken ) const;
        int avail_part_with_feature( int p, vpart_bitflags f, bool unbroken ) const;

        /**
         *  Check if vehicle has at least one unbroken part with specified flag
         *  @param flag Specified flag to search parts for
         *  @param enabled if set part must also be enabled to be considered
         *  @returns true if part is found
         */
        bool has_part( const std::string &flag, bool enabled = false ) const;

        /**
         *  Check if vehicle has at least one unbroken part with specified flag
         *  @param pos limit check for parts to this global position
         *  @param flag The specified flag
         *  @param enabled if set part must also be enabled to be considered
         */
        bool has_part( const tripoint &pos, const std::string &flag, bool enabled = false ) const;

        /**
         *  Get all enabled, available, unbroken vehicle parts at specified position
         *  @param pos position to check
         *  @param flag if set only flags with this part will be considered
         *  @param condition enum to include unabled, unavailable, and broken parts
         */
        std::vector<vehicle_part *> get_parts_at( const tripoint &pos, const std::string &flag,
                const part_status_flag condition );
        std::vector<const vehicle_part *> get_parts_at( const tripoint &pos,
                const std::string &flag, const part_status_flag condition ) const;

        /** Test if part can be enabled (unbroken, sufficient fuel etc), optionally displaying failures to user */
        bool can_enable( const vehicle_part &pt, bool alert = false ) const;

        /**
         *  Return the index of the next part to open at `p`'s location
         *
         *  The next part to open is the first unopened part in the reversed list of
         *  parts at part `p`'s coordinates.
         *
         *  @param p Part who's coordinates provide the location to check
         *  @param outside If true, give parts that can be opened from outside only
         *  @return part index or -1 if no part
         */
        int next_part_to_open( int p, bool outside = false ) const;

        /**
         *  Return the index of the next part to close at `p`
         *
         *  The next part to open is the first opened part in the list of
         *  parts at part `p`'s coordinates. Returns -1 for no more to close.
         *
         *  @param p Part who's coordinates provide the location to check
         *  @param outside If true, give parts that can be closed from outside only
         *  @return part index or -1 if no part
         */
        int next_part_to_close( int p, bool outside = false ) const;

        // returns indices of all parts in the given location slot
        std::vector<int> all_parts_at_location( const std::string &location ) const;

        // Given a part and a flag, returns the indices of all contiguously adjacent parts
        // with the same flag on the X and Y Axis
        std::vector<std::vector<int>> find_lines_of_parts( int part, const std::string &flag );

        // returns true if given flag is present for given part index
        bool part_flag( int p, const std::string &f ) const;
        bool part_flag( int p, vpart_bitflags f ) const;

        // Translate mount coordinates "p" using current pivot direction and anchor and return tile coordinates
        point coord_translate( const point &p ) const;

        // Translate mount coordinates "p" into tile coordinates "q" using given pivot direction and anchor
        void coord_translate( int dir, const point &pivot, const point &p, point &q ) const;
        // Translate mount coordinates "p" into tile coordinates "q" using given tileray and anchor
        // should be faster than previous call for repeated translations
        void coord_translate( tileray tdir, const point &pivot, const point &p, point &q ) const;

        // Rotates mount coordinates "p" from old_dir to new_dir along pivot
        point rotate_mount( int old_dir, int new_dir, const point &pivot, const point &p ) const;

        tripoint mount_to_tripoint( const point &mount ) const;
        tripoint mount_to_tripoint( const point &mount, const point &offset ) const;

        // Seek a vehicle part which obstructs tile with given coordinates relative to vehicle position
        int part_at( const point &dp ) const;
        int part_displayed_at( const point &dp ) const;
        int roof_at_part( int p ) const;

        // Given a part, finds its index in the vehicle
        int index_of_part( const vehicle_part *part, bool check_removed = false ) const;

        // get symbol for map
        char part_sym( int p, bool exact = false ) const;
        vpart_id part_id_string( int p, char &part_mod ) const;

        // get color for map
        nc_color part_color( int p, bool exact = false ) const;

        // Get all printable fuel types
        std::vector<itype_id> get_printable_fuel_types() const;

        // Vehicle fuel indicators (all of them)
        void print_fuel_indicators( const catacurses::window &win, int y, int x, int startIndex = 0,
                                    bool fullsize = false, bool verbose = false, bool desc = false, bool isHorizontal = false );

        // Pre-calculate mount points for (idir=0) - current direction or (idir=1) - next turn direction
        void precalc_mounts( int idir, int dir, const point &pivot );

        // get a list of part indices where is a passenger inside
        std::vector<int> boarded_parts() const;

        // get a list of part indices and Creature pointers with a rider
        std::vector<rider_data> get_riders() const;

        // get passenger at part p
        player *get_passenger( int p ) const;
        // get monster on a boardable part at p
        monster *get_pet( int p ) const;

        /**
         * Get the coordinates (in map squares) of this vehicle, it's the same
         * coordinate system that player::posx uses.
         * Global apparently means relative to the currently loaded map (game::m).
         * This implies:
         * <code>g->m.veh_at(this->global_pos3()) == this;</code>
         */
        tripoint global_pos3() const;
        /**
         * Get the coordinates of the studied part of the vehicle
         */
        tripoint global_part_pos3( const int &index ) const;
        tripoint global_part_pos3( const vehicle_part &pt ) const;
        /**
         * All the fuels that are in all the tanks in the vehicle, nicely summed up.
         * Note that empty tanks don't count at all. The value is the amount as it would be
         * reported by @ref fuel_left, it is always greater than 0. The key is the fuel item type.
         */
        std::map<itype_id, int> fuels_left() const;

        // Checks how much certain fuel left in tanks.
        int fuel_left( const itype_id &ftype, bool recurse = false ) const;
        // Checks how much of the part p's current fuel is left
        int fuel_left( const int p, bool recurse = false ) const;
        // Checks how much of an engine's current fuel is left in the tanks.
        int engine_fuel_left( const int e, bool recurse = false ) const;
        int fuel_capacity( const itype_id &ftype ) const;

        // Returns the total specific energy of this fuel type. Frozen is ignored.
        float fuel_specific_energy( const itype_id &ftype ) const;

        // drains a fuel type (e.g. for the kitchen unit)
        // returns amount actually drained, does not engage reactor
        int drain( const itype_id &ftype, int amount );
        int drain( const int index, int amount );
        /**
         * Consumes enough fuel by energy content. Does not support cable draining.
         * @param ftype Type of fuel
         * @param energy_w Desired amount of energy of fuel to consume
         * @return Amount of energy actually consumed. May be more or less than energy.
         */
        double drain_energy( const itype_id &ftype, double energy_w );

        // fuel consumption of vehicle engines of given type
        int basic_consumption( const itype_id &ftype ) const;
        int consumption_per_hour( const itype_id &ftype, int fuel_rate ) const;

        void consume_fuel( int load, const int t_seconds = 6, bool skip_battery = false );

        /**
         * Maps used fuel to its basic (unscaled by load/strain) consumption.
         */
        std::map<itype_id, int> fuel_usage() const;

        /**
         * Get all vehicle lights (excluding any that are destroyed)
         * @param active if true return only lights which are enabled
         */
        std::vector<vehicle_part *> lights( bool active = false );

        // Calculate vehicle's total drain or production of electrical power, including nominal
        // solar power.
        int total_epower_w();
        // Calculate vehicle's total drain or production of electrical power, optionally
        // including nominal solar power.  Return engine power as engine_power
        int total_epower_w( int &engine_power, bool skip_solar = true );
        // Calculate the total available power rating of all reactors
        int total_reactor_epower_w() const;
        // Produce and consume electrical power, with excess power stored or taken from
        // batteries
        void power_parts();

        /**
         * Try to charge our (and, optionally, connected vehicles') batteries by the given amount.
         * @return amount of charge left over.
         */
        int charge_battery( int amount, bool recurse = true );

        /**
         * Try to discharge our (and, optionally, connected vehicles') batteries by the given amount.
         * @return amount of request unfulfilled (0 if totally successful).
         */
        int discharge_battery( int amount, bool recurse = true );

        /**
         * Mark mass caches and pivot cache as dirty
         */
        void invalidate_mass();

        // get the total mass of vehicle, including cargo and passengers
        units::mass total_mass() const;

        // Gets the center of mass calculated for precalc[0] coordinates
        const point &rotated_center_of_mass() const;
        // Gets the center of mass calculated for mount point coordinates
        const point &local_center_of_mass() const;

        // Get the pivot point of vehicle; coordinates are unrotated mount coordinates.
        // This may result in refreshing the pivot point if it is currently stale.
        const point &pivot_point() const;

        // Get the (artificial) displacement of the vehicle due to the pivot point changing
        // between precalc[0] and precalc[1]. This needs to be subtracted from any actual
        // vehicle motion after precalc[1] is prepared.
        point pivot_displacement() const;

        // Get combined power of all engines. If fueled == true, then only engines which
        // vehicle have fuel for are accounted.  If safe == true, then limit engine power to
        // their safe power.
        int total_power_w( bool fueled = true, bool safe = false ) const;

        // Get ground acceleration gained by combined power of all engines. If fueled == true,
        // then only engines which the vehicle has fuel for are included
        int ground_acceleration( bool fueled = true, int at_vel_in_vmi = -1 ) const;
        // Get water acceleration gained by combined power of all engines. If fueled == true,
        // then only engines which the vehicle has fuel for are included
        int water_acceleration( bool fueled = true, int at_vel_in_vmi = -1 ) const;
        // Get acceleration for the current movement mode
        int acceleration( bool fueled = true, int at_vel_in_vmi = -1 ) const;

        // Get the vehicle's actual current acceleration
        int current_acceleration( bool fueled = true ) const;

        // is the vehicle currently moving?
        bool is_moving() const;

        // can the vehicle use rails?
        bool can_use_rails() const;

        // Get maximum ground velocity gained by combined power of all engines.
        // If fueled == true, then only the engines which the vehicle has fuel for are included
        int max_ground_velocity( bool fueled = true ) const;
        // Get maximum water velocity gained by combined power of all engines.
        // If fueled == true, then only the engines which the vehicle has fuel for are included
        int max_water_velocity( bool fueled = true ) const;
        // Get maximum velocity for the current movement mode
        int max_velocity( bool fueled = true ) const;

        // Get safe ground velocity gained by combined power of all engines.
        // If fueled == true, then only the engines which the vehicle has fuel for are included
        int safe_ground_velocity( bool fueled = true ) const;
        // Get safe water velocity gained by combined power of all engines.
        // If fueled == true, then only the engines which the vehicle has fuel for are included
        int safe_water_velocity( bool fueled = true ) const;
        // Get maximum velocity for the current movement mode
        int safe_velocity( bool fueled = true ) const;

        // Generate field from a part, either at front or back of vehicle depending on velocity.
        void spew_field( double joules, int part, field_type_id type, int intensity = 1 );

        // Loop through engines and generate noise and smoke for each one
        void noise_and_smoke( int load, time_duration time = 1_turns );

        /**
         * Calculates the sum of the area under the wheels of the vehicle.
         */
        int wheel_area() const;
        // average off-road rating for displaying off-road perfomance
        float average_or_rating() const;

        /**
         * Physical coefficients used for vehicle calculations.
         */
        /*@{*/
        /**
         * coefficient of air drag in kg/m
         * multiplied by the square of speed to calculate air drag force in N
         * proportional to cross sectional area of the vehicle, times the density of air,
         * times a dimensional constant based on the vehicle's shape
         */
        double coeff_air_drag() const;

        /**
         * coefficient of rolling resistance
         * multiplied by velocity to get the variable part of rolling resistance drag in N
         * multiplied by a constant to get the constant part of rolling resistance drag in N
         * depends on wheel design, wheel number, and vehicle weight
         */
        double coeff_rolling_drag() const;

        /**
         * coefficient of water drag in kg/m
         * multiplied by the square of speed to calculate water drag force in N
         * proportional to cross sectional area of the vehicle, times the density of water,
         * times a dimensional constant based on the vehicle's shape
         */
        double coeff_water_drag() const;

        /**
         * water draft in meters - how much of the vehicle's body is under water
         * must be less than the hull height or the boat will sink
         * at some point, also add boats with deep draft running around
         */
        double water_draft() const;

        /**
         * can_float
         * does the vehicle have freeboard or does it overflow with whater?
         */
        bool can_float() const;
        /**
         * is the vehicle mostly in water or mostly on fairly dry land?
         */
        bool is_in_water() const;

        /**
         * Traction coefficient of the vehicle.
         * 1.0 on road. Outside roads, depends on mass divided by wheel area
         * and the surface beneath wheels.
         *
         * Affects safe velocity, acceleration and handling difficulty.
         */
        float k_traction( float wheel_traction_area ) const;
        /*@}*/

        // Extra drag on the vehicle from components other than wheels.
        // @param actual is current drag if true or nominal drag otherwise
        int static_drag( bool actual = true ) const;

        // strain of engine(s) if it works higher that safe speed (0-1.0)
        float strain() const;

        // Calculate if it can move using its wheels
        bool sufficient_wheel_config() const;
        bool balanced_wheel_config() const;
        bool valid_wheel_config() const;

        // return the relative effectiveness of the steering (1.0 is normal)
        // <0 means there is no steering installed at all.
        float steering_effectiveness() const;

        /** Returns roughly driving skill level at which there is no chance of fumbling. */
        float handling_difficulty() const;

        // idle fuel consumption
        void idle( bool on_map = true );
        // continuous processing for running vehicle alarms
        void alarm();
        // leak from broken tanks
        void slow_leak();

        // thrust (1) or brake (-1) vehicle
        void thrust( int thd );

        //deceleration due to ground friction and air resistance
        int slowdown( int velocity ) const;

        // depending on skid vectors, chance to recover.
        void possibly_recover_from_skid();

        //forward component of velocity.
        float forward_velocity() const;

        // cruise control
        void cruise_thrust( int amount );

        // turn vehicle left (negative) or right (positive), degrees
        void turn( int deg );

        // Returns if any collision occurred
        bool collision( std::vector<veh_collision> &colls,
                        const tripoint &dp,
                        bool just_detect, bool bash_floor = false );

        // Handle given part collision with vehicle, monster/NPC/player or terrain obstacle
        // Returns collision, which has type, impulse, part, & target.
        veh_collision part_collision( int part, const tripoint &p,
                                      bool just_detect, bool bash_floor );

        // Process the trap beneath
        void handle_trap( const tripoint &p, int part );

        /**
         * Player is driving the vehicle
         * @param x direction player is steering
         * @param y direction player is steering
         */
        void pldrive( int x, int y );

        // stub for per-vpart limit
        units::volume max_volume( int part ) const;
        units::volume free_volume( int part ) const;
        units::volume stored_volume( int part ) const;
        /**
         * Update an item's active status, for example when adding
         * hot or perishable liquid to a container.
         */
        void make_active( item_location &loc );
        /**
         * Try to add an item to part's cargo.
         *
         * @returns cata::nullopt if it can't be put here (not a cargo part, adding this would violate
         * the volume limit or item count limit, not all charges can fit, etc.)
         * Otherwise, returns an iterator to the added item in the vehicle stack
         */
        cata::optional<vehicle_stack::iterator> add_item( int part, const item &obj );
        /** Like the above */
        cata::optional<vehicle_stack::iterator> add_item( vehicle_part &pt, const item &obj );
        /**
         * Add an item counted by charges to the part's cargo.
         *
         * @returns The number of charges added.
         */
        int add_charges( int part, const item &itm );

        // remove item from part's cargo
        bool remove_item( int part, item *it );
        vehicle_stack::iterator remove_item( int part, vehicle_stack::const_iterator it );

        vehicle_stack get_items( int part ) const;
        vehicle_stack get_items( int part );

        // Generates starting items in the car, should only be called when placed on the map
        void place_spawn_items();

        void gain_moves();

        // reduces velocity to 0
        void stop( bool update_cache = true );

        void refresh_insides();

        void unboard_all();

        // Damage individual part. bash means damage
        // must exceed certain threshold to be subtracted from hp
        // (a lot light collisions will not destroy parts)
        // Returns damage bypassed
        int damage( int p, int dmg, damage_type type = DT_BASH, bool aimed = true );

        // damage all parts (like shake from strong collision), range from dmg1 to dmg2
        void damage_all( int dmg1, int dmg2, damage_type type, const point &impact );

        //Shifts the coordinates of all parts and moves the vehicle in the opposite direction.
        void shift_parts( const point &delta );
        bool shift_if_needed();

        void shed_loose_parts();

        /**
         * @name Vehicle turrets
         *
         *@{*/

        /** Get all vehicle turrets (excluding any that are destroyed) */
        std::vector<vehicle_part *> turrets();

        /** Get all vehicle turrets loaded and ready to fire at target */
        std::vector<vehicle_part *> turrets( const tripoint &target );

        /** Get firing data for a turret */
        turret_data turret_query( vehicle_part &pt );
        const turret_data turret_query( const vehicle_part &pt ) const;

        turret_data turret_query( const tripoint &pos );
        const turret_data turret_query( const tripoint &pos ) const;

        /** Set targeting mode for specific turrets */
        void turrets_set_targeting();

        /** Set firing mode for specific turrets */
        void turrets_set_mode();

        /*
         * Set specific target for automatic turret fire
         * @param manual if true, allows target assignment for manually controlled turrets.
         * @param automatic if true, allows target assignment for automatically controlled turrets.
         * @param tur_part pointer to a turret aimed regardless of target mode filters, if not nullptr.
         * @returns whether a valid target was selected.
         */
        bool turrets_aim( bool manual = true, bool automatic = false,
                          vehicle_part *tur_part = nullptr );

        /*
         * Call turrets_aim and then fire turrets if we get a valid target.
         * @param manual if true, allows targeting and firing for manual turrets.
         * @param automatic if true, allows targeting and firing for automatic turrets.
         * @param tur_part pointer to a turret aimed regardless of target mode filters, if not nullptr.
         * @return the number of shots fired.
         */
        int turrets_aim_and_fire( bool manual = true, bool automatic = false,
                                  vehicle_part *tur_part = nullptr );

        /*
         * Call turrets_aim and then fire a selected single turret if we have a valid target.
         * @param tur_part if not null, this turret is aimed instead of bringing up the selection menu.
         * @return the number of shots fired.
         */
        int turrets_aim_single( vehicle_part *tur_part = nullptr );

        /*
         * @param pt the vehicle part containing the turret we're trying to target.
         * @return npc object with suitable attributes for targeting a vehicle turret.
         */
        npc get_targeting_npc( const vehicle_part &pt );
        /*@}*/

        /**
         *  Try to assign a crew member (who must be a player ally) to a specific seat
         *  @note enforces NPC's being assigned to only one seat (per-vehicle) at once
         */
        bool assign_seat( vehicle_part &pt, const npc &who );

        // Update the set of occupied points and return a reference to it
        std::set<tripoint> &get_points( bool force_refresh = false );

        // opens/closes doors or multipart doors
        void open( int part_index );
        void close( int part_index );
        // returns whether the door is open or not
        bool is_open( int part_index ) const;

        // Consists only of parts with the FOLDABLE tag.
        bool is_foldable() const;
        // Restore parts of a folded vehicle.
        bool restore( const std::string &data );
        //handles locked vehicles interaction
        bool interact_vehicle_locked();
        //true if an alarm part is installed on the vehicle
        bool has_security_working() const;
        /**
         *  Opens everything that can be opened on the same tile as `p`
         */
        void open_all_at( int p );

        // Honk the vehicle's horn, if there are any
        void honk_horn();
        void reload_seeds( const tripoint &pos );
        void beeper_sound();
        void play_music();
        void play_chimes();
        void operate_planter();
        std::string tracking_toggle_string();
        void toggle_tracking();
        //scoop operation,pickups, battery drain, etc.
        void operate_scoop();
        void operate_reaper();
        void transform_terrain();
        void add_toggle_to_opts( std::vector<uilist_entry> &options,
                                 std::vector<std::function<void()>> &actions, const std::string &name, char key,
                                 const std::string &flag );
        void set_electronics_menu_options( std::vector<uilist_entry> &options,
                                           std::vector<std::function<void()>> &actions );
        //main method for the control of multiple electronics
        void control_electronics();
        //main method for the control of individual engines
        void control_engines();
        // shows ui menu to select an engine
        int select_engine();
        //returns whether the engine is enabled or not, and has fueltype
        bool is_engine_type_on( int e, const itype_id &ft ) const;
        //returns whether the engine is enabled or not
        bool is_engine_on( int e ) const;
        //returns whether the part is enabled or not
        bool is_part_on( int p ) const;
        //returns whether the engine uses specified fuel type
        bool is_engine_type( int e, const itype_id &ft ) const;
        //returns whether the alternator is operational
        bool is_alternator_on( int a ) const;
        //mark engine as on or off
        void toggle_specific_engine( int p, bool on );
        void toggle_specific_part( int p, bool on );
        //true if an engine exists with specified type
        //If enabled true, this engine must be enabled to return true
        bool has_engine_type( const itype_id &ft, bool enabled ) const;
        //true if an engine exists without the specified type
        //If enabled true, this engine must be enabled to return true
        bool has_engine_type_not( const itype_id &ft, bool enabled ) const;
        //returns true if there's another engine with the same exclusion list; conflict_type holds
        //the exclusion
        bool has_engine_conflict( const vpart_info *possible_engine, std::string &conflict_type ) const;
        //returns true if the engine doesn't consume fuel
        bool is_perpetual_type( int e ) const;
        //if necessary, damage this engine
        void do_engine_damage( size_t p, int strain );
        //remotely open/close doors
        void control_doors();
        // return a vector w/ 'direction' & 'magnitude', in its own sense of the words.
        rl_vec2d velo_vec() const;
        //normalized vectors, from tilerays face & move
        rl_vec2d face_vec() const;
        rl_vec2d move_vec() const;
        // As above, but calculated for the actually used variable `dir`
        rl_vec2d dir_vec() const;
        // update vehicle parts as the vehicle moves
        void on_move();
        // move the vehicle on the map. Returns updated pointer to self.
        vehicle *act_on_map();
        // check if the vehicle should be falling or is in water
        void check_falling_or_floating();

        /** Precalculate vehicle turn. Counts wheels that will land on ter_flag_to_check
         * new_turn_dir             - turn direction to calculate
         * falling_only             - is vehicle falling
         * check_rail_direction     - check if vehicle should land on diagonal/not rail tile (use for trucks only)
         * ter_flag_to_check        - terrain flag vehicle wheel should land on
         ** Results:
         * &wheels_on_rail          - resulting wheels that land on ter_flag_to_check
         * &turning_wheels_that_are_one_axis_counter - number of wheels that are on one axis and will land on rail
         */
        void precalculate_vehicle_turning( int new_turn_dir, bool check_rail_direction,
                                           const ter_bitflags ter_flag_to_check, int &wheels_on_rail,
                                           int &turning_wheels_that_are_one_axis_counter ) const;
        bool allow_auto_turn_on_rails( int &corrected_turn_dir ) const;
        bool allow_manual_turn_on_rails( int &corrected_turn_dir ) const;
        bool is_wheel_state_correct_to_turn_on_rails( int wheels_on_rail, int wheel_count,
                int turning_wheels_that_are_one_axis ) const;
        /**
         * Update the submap coordinates smx, smy, and update the tracker info in the overmap
         * (if enabled).
         * This should be called only when the vehicle has actually been moved, not when
         * the map is just shifted (in the later case simply set smx/smy directly).
         */
        void set_submap_moved( int x, int y );
        void use_washing_machine( int p );
        void use_monster_capture( int part, const tripoint &pos );
        void use_bike_rack( int part );
        void use_harness( int part, const tripoint &pos );

        void interact_with( const tripoint &pos, int interact_part );

        const std::string disp_name() const;

        /** Required strength to be able to successfully lift the vehicle unaided by equipment */
        int lift_strength() const;

        // Called by map.cpp to make sure the real position of each zone_data is accurate
        bool refresh_zones();

        bounding_box get_bounding_box();
        // Retroactively pass time spent outside bubble
        // Funnels, solar panels
        void update_time( const time_point &update_to );

        // The faction that owns this vehicle.
        const faction *owner = nullptr;
        // The faction that previously owned this vehicle
        const faction *old_owner = nullptr;

    private:
        mutable double coefficient_air_resistance = 1;
        mutable double coefficient_rolling_resistance = 1;
        mutable double coefficient_water_resistance = 1;
        mutable double draft_m = 1;
        mutable double hull_height = 0.3;

        // Cached points occupied by the vehicle
        std::set<tripoint> occupied_points;

    public:
        std::vector<vehicle_part> parts;   // Parts which occupy different tiles
        std::vector<tripoint> omt_path; // route for overmap-scale auto-driving
        std::vector<int> alternators;      // List of alternator indices
        std::vector<int> engines;          // List of engine indices
        std::vector<int> reactors;         // List of reactor indices
        std::vector<int> solar_panels;     // List of solar panel indices
        std::vector<int> wind_turbines;     // List of wind turbine indices
        std::vector<int> water_wheels;     // List of water wheel indices
        std::vector<int> sails;            // List of sail indices
        std::vector<int> funnels;          // List of funnel indices
        std::vector<int> emitters;         // List of emitter parts
        std::vector<int> loose_parts;      // List of UNMOUNT_ON_MOVE parts
        std::vector<int> wheelcache;       // List of wheels
        std::vector<int> rail_wheelcache;  // List of rail wheels
        std::vector<int> steering;         // List of STEERABLE parts
        // List of parts that will not be on a vehicle very often, or which only one will be present
        std::vector<int> speciality;
        std::vector<int> floating;         // List of parts that provide buoyancy to boats

        // config values
        std::string name;   // vehicle name
        /**
         * Type of the vehicle as it was spawned. This will never change, but it can be an invalid
         * type (e.g. if the definition of the prototype has been removed from json or if it has been
         * spawned with the default constructor).
         */
        vproto_id type;
        // parts_at_relative(dp) is used a lot (to put it mildly)
        std::map<point, std::vector<int>> relative_parts;
        std::set<label> labels;            // stores labels
        std::set<std::string> tags;        // Properties of the vehicle
        // After fuel consumption, this tracks the remainder of fuel < 1, and applies it the next time.
        std::map<itype_id, float> fuel_remainder;
        std::unordered_multimap<point, zone_data> loot_zones;
        active_item_cache active_items;

    private:
        mutable units::mass mass_cache;
        // cached pivot point
        mutable point pivot_cache;
        /*
         * The co-ordinates of the bounding box of the vehicle's mount points
         */
        mutable point mount_max;
        mutable point mount_min;
        mutable point mass_center_precalc;
        mutable point mass_center_no_precalc;
        tripoint autodrive_local_target = tripoint_zero; // currrent node the autopilot is aiming for

    public:
        // Subtract from parts.size() to get the real part count.
        int removed_part_count;

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
        int smx;
        int smy;
        int smz;

        // alternator load as a percentage of engine power, in units of 0.1% so 1000 is 100.0%
        int alternator_load;
        /// Time occupied points were calculated.
        time_point occupied_cache_time = calendar::before_time_starts;
        // Turn the vehicle was last processed
        time_point last_update = calendar::before_time_starts;
        // save values
        /**
         * Position of the vehicle *inside* the submap that contains the vehicle.
         * This will (nearly) always be in the range (0...SEEX-1).
         * Note that vehicles are "moved" by map::displace_vehicle. You should not
         * set them directly, except when initializing the vehicle or during mapgen.
         */
        int posx = 0;
        int posy = 0;
        // vehicle current velocity, mph * 100
        int velocity = 0;
        // velocity vehicle's cruise control trying to achieve
        int cruise_velocity = 0;
        // Only used for collisions, vehicle falls instantly
        int vertical_velocity = 0;
        // id of the om_vehicle struct corresponding to this vehicle
        int om_id;
        // direction, to which vehicle is turning (player control). will rotate frame on next move
        // must be a multiple of 15 degrees
        int turn_dir = 0;
        // amount of last turning (for calculate skidding due to handbrake)
        int last_turn = 0;
        // goes from ~1 to ~0 while proceeding every turn
        float of_turn;
        // leftover from previous turn
        float of_turn_carry;
        int extra_drag = 0;
        // last time point the fluid was inside tanks was checked for processing
        time_point last_fluid_check = calendar::turn_zero;
        // the time point when it was succesfully stolen
        cata::optional<time_point> theft_time;
        // rotation used for mount precalc values
        std::array<int, 2> pivot_rotation = { { 0, 0 } };

        bounding_box rail_wheel_bounding_box;
        // points used for rotation of mount precalc values
        std::array<point, 2> pivot_anchor;
        // frame direction
        tileray face;
        // direction we are moving
        tileray move;

    private:
        bool no_refresh = false;

        // if true, pivot_cache needs to be recalculated
        mutable bool pivot_dirty;
        mutable bool mass_dirty = true;
        mutable bool mass_center_precalc_dirty = true;
        mutable bool mass_center_no_precalc_dirty = true;
        // cached values for air, water, and  rolling resistance;
        mutable bool coeff_rolling_dirty = true;
        mutable bool coeff_air_dirty = true;
        mutable bool coeff_water_dirty = true;
        // air uses a two stage dirty check: one dirty bit gets set on part install,
        // removal, or breakage. The other dirty bit only gets set during part_removal_cleanup,
        // and that's the bit that controls recalculation.  The intent is to only recalculate
        // the coeffs once per turn, even if multiple parts are destroyed in a collision
        mutable bool coeff_air_changed = true;
        // is the vehicle currently mostly in water
        mutable bool is_floating = false;

    public:
        bool is_autodriving = false;
        bool all_wheels_on_one_axis;
        // TODO: change these to a bitset + enum?
        // cruise control on/off
        bool cruise_on = true;
        // at least one engine is on, of any type
        bool engine_on = false;
        // vehicle tracking on/off
        bool tracking_on = false;
        // vehicle has no key
        bool is_locked = false;
        // vehicle has alarm on
        bool is_alarm_on = false;
        bool camera_on = false;
        // skidding mode
        bool skidding = false;
        // has bloody or smoking parts
        bool check_environmental_effects = false;
        // "inside" flags are outdated and need refreshing
        bool insides_dirty = true;
        // Is the vehicle hanging in the air and expected to fall down in the next turn?
        bool is_falling = false;
        // zone_data positions are outdated and need refreshing
        bool zones_dirty = true;

        // current noise of vehicle (engine working, etc.)
        unsigned char vehicle_noise = 0;
};

#endif
