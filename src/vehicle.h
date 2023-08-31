#pragma once
#ifndef CATA_SRC_VEHICLE_H
#define CATA_SRC_VEHICLE_H

#include <array>
#include <climits>
#include <cstddef>
#include <functional>
#include <iosfwd>
#include <list>
#include <map>
#include <new>
#include <optional>
#include <set>
#include <stack>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "active_item_cache.h"
#include "calendar.h"
#include "character_id.h"
#include "clzones.h"
#include "colony.h"
#include "coordinates.h"
#include "damage.h"
#include "game_constants.h"
#include "item.h"
#include "item_group.h"
#include "item_location.h"
#include "item_stack.h"
#include "line.h"
#include "point.h"
#include "tileray.h"
#include "type_id.h"
#include "units.h"
#include "vpart_range.h"

class Character;
class Creature;
class JsonObject;
class JsonOut;
class map;
class monster;
class nc_color;
class npc;
class vehicle;
class vehicle_part_range;
class veh_menu;
class vpart_info;
class vpart_position;
class vpart_variant;
class zone_data;
struct input_event;
struct itype;
struct uilist_entry;
template <typename E> struct enum_traits;

enum vpart_bitflags : int;
enum class ter_furn_flag : int;
template<typename feature_type>
class vehicle_part_with_feature_range;

void handbrake();

void practice_athletic_proficiency( Character &p );
void practice_pilot_proficiencies( Character &p, bool &boating );

namespace catacurses
{
class window;
} // namespace catacurses

namespace vehicles
{
// ratio of constant rolling resistance to the part that varies with velocity
constexpr double rolling_constant_to_variable = 33.33;
constexpr float vmiph_per_tile = 400.0f;
// steering/turning increment
constexpr units::angle steer_increment = 15_degrees;
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

template<>
struct enum_traits<part_status_flag> {
    static constexpr bool is_flag_enum = true;
};

enum veh_coll_type : int {
    veh_coll_nothing,  // 0 - nothing,
    veh_coll_body,     // 1 - monster/player/npc
    veh_coll_veh,      // 2 - vehicle
    veh_coll_bashable, // 3 - bashable
    veh_coll_other,    // 4 - other
    num_veh_coll_types
};

struct smart_controller_cache {
    time_point created = calendar::turn;
    time_point gas_engine_last_turned_on = calendar::start_of_cataclysm;
    bool gas_engine_shutdown_forbidden = false;
    int velocity = 0;
    int battery_percent = 0;
    units::power battery_net_charge_rate = 0_W;
    float load = 0.0f;
};

struct smart_controller_config {
    int battery_lo = 25;
    int battery_hi = 90;

    void deserialize( const JsonObject &data );
    void serialize( JsonOut &json ) const;
};

struct veh_collision {
    //int veh?
    int part  = 0;
    veh_coll_type type = veh_coll_nothing;
    // impulse
    int  imp = 0;
    //vehicle
    void *target  = nullptr;
    //vehicle partnum
    int target_part = 0;
    std::string target_name;

    veh_collision() = default;
};

struct vpart_edge_info {
    int forward;
    int back;
    int left;
    int right;
    bool left_edge = false;
    bool right_edge = false;
    vpart_edge_info() : forward( -1 ), back( -1 ), left( -1 ), right( -1 ) {}
    vpart_edge_info( int forward, int back, int left, int right, bool left_edge, bool right_edge ) :
        forward( forward ), back( back ), left( left ), right( right ), left_edge( left_edge ),
        right_edge( right_edge ) {}

    bool is_edge_mount() const {
        return left_edge || right_edge || left == -1 || right == -1;
    }
    bool is_left_edge() const {
        return left_edge || left == -1;
    }
};

class vehicle_stack : public item_stack
{
    private:
        vehicle &veh;
        vehicle_part &vp;
    public:
        vehicle_stack( vehicle &veh, vehicle_part &vp );
        item_stack::iterator erase( item_stack::const_iterator it ) override;
        void insert( const item &newitem ) override;
        int count_limit() const override;
        units::volume max_volume() const override;
};

enum towing_point_side : int {
    TOW_FRONT,
    TOW_SIDE,
    TOW_BACK,
    NUM_TOW_TYPES
};

class towing_data
{
    private:
        vehicle *towing;
        vehicle *towed_by;
    public:
        explicit towing_data( vehicle *towed_veh = nullptr, vehicle *tower_veh = nullptr ) :
            towing( towed_veh ), towed_by( tower_veh ) {}
        vehicle *get_towed_by() const {
            return towed_by;
        }
        bool set_towing( vehicle *tower_veh, vehicle *towed_veh );
        vehicle *get_towed() const {
            return towing;
        }
        void clear_towing() {
            towing = nullptr;
            towed_by = nullptr;
        }
        towing_point_side tow_direction = NUM_TOW_TYPES;
        // temp variable used for saving/loading
        tripoint other_towing_point;
};

struct bounding_box {
    point p1;
    point p2;
};

int mps_to_vmiph( double mps );
double vmiph_to_mps( int vmiph );
int cmps_to_vmiph( int cmps );
int vmiph_to_cmps( int vmiph );
constexpr float accel_g = 9.81f;

enum class vp_flag : uint32_t {
    none = 0,
    passenger_flag = 1,
    animal_flag = 2,
    carried_flag = 4,
    carrying_flag = 8,
    tracked_flag = 16, //carried vehicle part with tracking enabled
    linked_flag = 32 //a cable is attached to this
};

class turret_cpu
{
        friend vehicle_part;
    private:
        std::unique_ptr<npc> brain;
    public:
        turret_cpu() = default;
        turret_cpu( const turret_cpu & );
        turret_cpu &operator=( const turret_cpu & );
        ~turret_cpu();
};

/**
 * Structure, describing vehicle part (i.e., wheel, seat)
 */
struct vehicle_part {
        friend vehicle;
        friend class veh_interact;
        friend class vehicle_cursor;
        friend class vehicle_stack;
        friend item_location;
        friend class turret_data;

        // DefaultConstructible, with vpart_id::NULL_ID type and default base item
        vehicle_part();
        // constructs part with \p type and std::move()-ing \p base as part's base
        vehicle_part( const vpart_id &type, item &&base );

        // gets reference to the current base item
        const item &get_base() const;
        // set part base to \p new_base, std::move()-ing it
        void set_base( item &&new_base );

        bool has_flag( const vp_flag flag ) const noexcept {
            const uint32_t flag_as_uint32 = static_cast<uint32_t>( flag );
            return ( flags & flag_as_uint32 ) == flag_as_uint32;
        }
        void set_flag( const vp_flag flag ) noexcept {
            flags |= static_cast<uint32_t>( flag );
        }
        void remove_flag( const vp_flag flag ) noexcept {
            flags &= ~static_cast<uint32_t>( flag );
        }

        /**
         * Translated name of a part inclusive of any current status effects
         * with_prefix as true indicates the durability symbol should be prepended
         */
        std::string name( bool with_prefix = true ) const;

        struct carried_part_data {
            tripoint mount;        // if value is tripoint_zero this is the pivot
            units::angle face_dir; // direction relative to the carrier vehicle
            std::string veh_name;  // carried vehicle name this part belongs to

            void deserialize( const JsonObject &data );
            void serialize( JsonOut &json ) const;
        };

        // each time this vehicle part is racked this will push the data required to unrack to stack
        std::stack<carried_part_data> carried_stack;

        /** Specific type of fuel, charges or ammunition currently contained by a part */
        itype_id ammo_current() const;

        /** Maximum amount of fuel, charges or ammunition that can be contained by a part */
        int ammo_capacity( const ammotype &ammo ) const;

        /** Amount of fuel, charges or ammunition currently contained by a part */
        int ammo_remaining() const;
        int remaining_ammo_capacity() const;

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
         * @param wanted_energy Energy to consume
         * @return Energy actually consumed
         */
        units::energy consume_energy( const itype_id &ftype, units::energy wanted_energy );

        /* @return true if part in current state be reloaded optionally with specific itype_id */
        bool can_reload( const item &obj = item() ) const;

        // No need to serialize this, it can simply be reinitialized after starting a new game.
        turret_cpu cpu; //NOLINT(cata-serialize)
        /**
         * If this part is capable of wholly containing something, process the
         * items in there.
         * @param pos Position of this part for item::process
         * @param e_heater Engine has a heater and is on
         */
        void process_contents( map &here, const tripoint &pos, bool e_heater );

        /**
         *  Try adding @param liquid to tank optionally limited by @param qty
         *  @return whether any of the liquid was consumed (which may be less than qty)
         */
        bool fill_with( item &liquid, int qty = INT_MAX );

        /** Current faults affecting this part (if any) */
        const std::set<fault_id> &faults() const;

        /** Does this vehicle part have a fault with this flag */
        bool has_fault_flag( const std::string &searched_flag ) const;

        /** Faults which could potentially occur with this part (if any) */
        std::set<fault_id> faults_potential() const;

        /** Try to set fault returning false if specified fault cannot occur with this item */
        bool fault_set( const fault_id &f );

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
        bool is_fuel_store( bool skip_broke = true ) const;

        /** Can this part contain liquid fuels? */
        bool is_tank() const;

        /** Does this part currently contain some liquid? */
        bool contains_liquid() const;

        /** Can this part store electrical charge? */
        bool is_battery() const;

        /** Is this part a reactor? */
        bool is_reactor() const;

        /** is this part currently unable to retain to fluid/charge?
         *  this doesn't take into account whether or not the part has any contents
         *  remaining to leak
         */
        bool is_leaking() const;

        /** Can this part function as a turret? */
        bool is_turret() const;

        /** Can a player or NPC use this part as a seat? */
        bool is_seat() const;

        /* if this is a carried part, what is the name of the carried vehicle */
        std::string carried_name() const;
        /*@}*/

    public:
        /** mount point: x is on the forward/backward axis, y is on the left/right axis */
        point mount;

        /** mount translated to face.dir [0] and turn_dir [1] */
        // NOLINTNEXTLINE(cata-use-named-point-constants)
        std::array<tripoint, 2> precalc = { { tripoint( -1, -1, 0 ), tripoint( -1, -1, 0 ) } };

        /** current part health with range [0,durability] */
        int hp() const;

        /** Current part damage in same units as item::damage. */
        int damage() const;
        /** Current part degradation of part base */
        int degradation() const;
        /** max damage of part base */
        int max_damage() const;
        // @returns true if part can be repaired, accounting for part degradation
        bool is_repairable() const;

        /** Current part damage as a percentage of maximum, with 0.0 being perfect condition */
        double damage_percent() const;
        /** Current part health as a percentage of maximum, with 1.0 being perfect condition */
        double health_percent() const;

        /** parts are considered broken at zero health */
        bool is_broken() const;

        /** Is this an enabled autoclave, dishwasher, or washing machine? */
        bool is_cleaner_on() const;

        /** parts are unavailable if broken or if carried is true, if they have the CARRIED flag */
        bool is_unavailable( bool carried = true ) const;
        /** parts are available if they aren't unavailable */
        bool is_available( bool carried = true ) const;

        /*
         * @param veh the vehicle containing this part.
         * @return npc object with suitable attributes for targeting a vehicle turret.
        */
        npc &get_targeting_npc( vehicle &veh );
        /*@}*/

        /** how much blood covers part (in turns). */
        int blood = 0;

        /**
         * if tile provides cover.
         * WARNING: do not read it directly, use vpart_position::is_inside() instead
         */
        bool inside = false; // NOLINT(cata-serialize)

        /**
         * true if this part is removed. The part won't disappear until the end of the turn
         * so our indices can remain consistent.
         */
        bool removed = false; // NOLINT(cata-serialize)
        bool enabled = true;

        /** ID of player passenger */
        character_id passenger_id;

        /** door is open */
        bool open = false;

        /** door is locked */
        bool locked = false;

        /** direction the part is facing */
        units::angle direction = 0_degrees;

        /**
         * Coordinates for some kind of target; jumper cables and turrets use this
         * Two coordinate pairs are stored: actual target point, and target vehicle center.
         * Both cases use absolute coordinates (relative to world origin)
         */
        std::pair<tripoint, tripoint> target = { tripoint_min, tripoint_min };

        /** If it's a part with variants, which variant it is */
        std::string variant;

        time_point last_disconnected = calendar::before_time_starts;

    private:
        // part type definition
        // note: this could be a const& but doing so would require hassle with implementing
        // operator= and deserializing the part, this must be always a valid pointer
        const vpart_info *info_; // NOLINT(cata-serialize)
        // flags storage, see vp_flag
        uint32_t flags = 0;
        // part base item, e.g. the container of a TANK, gun of the turret, etc...
        item base;
        // tools attached to VEH_TOOLS parts
        std::vector<item> tools;
        // items of CARGO parts
        cata::colony<item> items;

        /** Preferred ammo type when multiple are available */
        itype_id ammo_pref = itype_id::NULL_ID();

        /**
         *  What NPC (if any) is assigned to this part (seat, turret etc)?
         *  @see vehicle_part::crew() accessor which excludes dead and non-allied NPC's
         */
        character_id crew_id;

    public:
        /** Get part definition common to all parts of this type */
        const vpart_info &info() const;

        void serialize( JsonOut &json ) const;
        void deserialize( const JsonObject &data );
        /**
         * Returns an ItemList of the pieces that should arise from breaking
         * this part.
         */
        item_group::ItemList pieces_for_broken_part() const;

        /* probably should be private, but I'm too lazy to write setters and getters */
        bool is_fake = false; // NOLINT(cata-serialize)
        bool is_active_fake = false; // NOLINT(cata-serialize)
        int fake_part_to = -1; // NOLINT(cata-serialize)
        int fake_protrusion_on = -1; // NOLINT(cata-serialize)
        bool has_fake = false; // NOLINT(cata-serialize)
        int fake_part_at = -1; // NOLINT(cata-serialize)

        bool is_real_or_active_fake() const {
            return !is_fake || is_active_fake;
        }
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
        item_location base() const;

        /** @returns true if turret is using vehicle tanks or batteries */
        bool uses_vehicle_tanks_or_batteries() const;

        /** Quantity of ammunition available for use */
        int ammo_remaining() const;

        /** Maximum quantity of ammunition turret can itself contain */
        int ammo_capacity( const ammotype &ammo ) const;

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
         * Check if target is in range of this turret (considers current ammo)
         * Assumes this turret's status is 'ready'
         */
        bool in_range( const tripoint &target ) const;

        /**
         * Prepare the turret for firing, called by firing function.
         * This sets up vehicle tanks, recoil adjustments, vehicle rooftop status,
         * and performs any other actions that must be done before firing a turret.
         * @param p the player that is firing the gun, subject to recoil adjustment.
         */
        void prepare_fire( Character &you );

        /**
         * Reset state after firing a prepared turret, called by the firing function.
         * @param p the player that just fired (or attempted to fire) the turret.
         * @param shots the number of shots fired by the most recent call to turret::fire.
         */
        void post_fire( Character &you, int shots );

        /**
         * Fire the turret's gun at a given target.
         * @param p the player firing the turret, passed to pre_fire and post_fire calls.
         * @param target coordinates that will be fired on.
         * @return the number of shots actually fired (may be zero).
         */
        int fire( Character &c, const tripoint &target );

        bool can_reload() const;
        bool can_unload() const;

        enum class status : int {
            invalid,
            no_ammo,
            no_power,
            ready
        };

        status query() const;

    private:
        turret_data( vehicle *veh, vehicle_part *part )
            : veh( veh ), part( part ) {}
        double cached_recoil = 0;

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
    explicit label( const point &p ) : point( p ) {}
    label( const point &p, std::string text ) : point( p ), text( std::move( text ) ) {}

    std::string text;

    void deserialize( const JsonObject &data );
    void serialize( JsonOut &json ) const;
};

enum class autodrive_result : int {
    // the driver successfully performed course correction or simply did nothing
    // in order to keep going forward
    ok,
    // something bad happened (navigation error, crash, loss of visibility, or just
    // couldn't find a way around obstacles) and autodrive cannot continue
    abort,
    // arrived at the destination
    finished
};

class RemovePartHandler;

class vpart_display
{
    public:
        vpart_display();
        explicit vpart_display( const vehicle_part &vp );

        const vpart_id &id;
        const vpart_variant &variant;
        nc_color color = c_black;
        char32_t symbol = ' '; // symbol in unicode
        int symbol_curses = ' '; // symbol converted to curses ACS encoding if needed
        bool is_broken = false;
        bool is_open = false;
        bool has_cargo = false;
        // @returns string for tileset: "vp_{id}_{variant}" or "vp_{id}" if variant is empty
        std::string get_tileset_id() const;
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
 *   constant info: see veh_type.h, `vpart_info` structure that is accessible
 *   through `vehicle_part::info()` method.
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
        void lock_or_unlock( int part_index, bool locking );
        bool is_connected( const vehicle_part &to, const vehicle_part &from,
                           const vehicle_part &excluded ) const;

        // direct damage to part (armor protection and internals are not counted)
        // @returns damage still left to apply
        int damage_direct( map &here, vehicle_part &vp, int dmg,
                           const damage_type_id &type = damage_type_id( "pure" ) );
        // Removes the part, breaks it into pieces and possibly removes parts attached to it
        int break_off( map &here, vehicle_part &vp, int dmg );
        // Returns if it did actually explode
        bool explode_fuel( vehicle_part &vp, const damage_type_id &type );
        //damages vehicle controls and security system
        void smash_security_system();
        // get vpart powerinfo for part number, accounting for variable-sized parts and hps.
        units::power part_vpower_w( const vehicle_part &vp, bool at_full_hp = false ) const;

        // Get part power consumption/production for part number.
        units::power part_epower( const vehicle_part &vp ) const;

        // convert watts over time to battery energy (kJ)
        int power_to_energy_bat( units::power power, const time_duration &d ) const;

        // Do stuff like clean up blood and produce smoke from broken parts. Returns false if nothing needs doing.
        bool do_environmental_effects() const;

        // Vehicle fuel indicator (by fuel)
        void print_fuel_indicator( const catacurses::window &w, const point &p,
                                   const itype_id &fuel_type,
                                   bool verbose = false, bool desc = false );
        void print_fuel_indicator( const catacurses::window &w, const point &p,
                                   const itype_id &fuel_type,
                                   std::map<itype_id, units::energy> fuel_usages,
                                   bool verbose = false, bool desc = false );

        // Calculate how long it takes to attempt to start an engine
        time_duration engine_start_time( const vehicle_part &vp ) const;

        // How much does the temperature effect the engine starting (0.0 - 1.0)
        double engine_cold_factor( const vehicle_part &vp ) const;

        // refresh pivot_cache, clear pivot_dirty
        void refresh_pivot() const;

        void calc_mass_center( bool precalc ) const;

        /** empty the contents of a tank, battery or turret spilling liquids randomly on the ground */
        void leak_fuel( vehicle_part &pt ) const;

        /// Returns a map of connected vehicle pointers to power loss factor:
        /// Keys are vehicles connected by POWER_TRANSFER parts, includes self
        /// Values are line loss, 0.01 corresponds to 1% charge loss to wire resistance
        /// May load the connected vehicles' submaps
        /// Templated to support const and non-const vehicle*
        template<typename Vehicle>
        static std::map<Vehicle *, float> search_connected_vehicles( Vehicle *start );
    public:
        /**
         * Find a possibly off-map vehicle. If necessary, loads up its submap through
         * the global MAPBUFFER and pulls it from there. For this reason, you should only
         * give it the coordinates of the origin tile of a target vehicle.
         * @param where Location of the other vehicle's origin tile.
         */
        static vehicle *find_vehicle( const tripoint_abs_ms &where );
        //! @copydoc vehicle::search_connected_vehicles( Vehicle *start )
        std::map<vehicle *, float> search_connected_vehicles();
        //! @copydoc vehicle::search_connected_vehicles( Vehicle *start )
        std::map<const vehicle *, float> search_connected_vehicles() const;
        //! @copydoc vehicle::search_connected_vehicles( Vehicle *start )
        void get_connected_vehicles( std::unordered_set<vehicle *> &dest );

        /// Returns a map of connected battery references to power loss factor
        /// Keys are batteries in vehicles (includes self) connected by POWER_TRANSFER parts
        /// Values are line loss, 0.01 corresponds to 1% charge loss to wire resistance
        /// May load the connected vehicles' submaps
        std::map<vpart_reference, float> search_connected_batteries();

        // constructs a vehicle, if the given \p proto_id is an empty string the vehicle is
        // constructed empty, invalid proto_id will construct empty and raise a debugmsg,
        // if given \p proto_id is valid then parts are copied from the vproto's blueprint,
        // and prototype's fuel/ammo will be spawned in init_state / place_spawn_items
        explicit vehicle( const vproto_id &proto_id );
        vehicle( const vehicle & ) = delete;
        ~vehicle();
        vehicle &operator=( vehicle && ) = default;

    private:
        vehicle &operator=( const vehicle & ) = default;

    public:
        /** Disable or enable refresh() ; used to speed up performance when creating a vehicle */
        void suspend_refresh();
        void enable_refresh();
        //Refresh all caches and re-locate all parts
        void refresh( bool remove_fakes = true );

        // Refresh active_item cache for vehicle parts
        void refresh_active_item_cache();

        /** Return a pointer-like type that's automatically invalidated if this
        * item is destroyed or assigned-to */
        safe_reference<vehicle> get_safe_reference();

        /**
         * Set stat for part constrained by range [0,durability]
         * @note does not invoke base @ref item::on_damage callback
         */
        void set_hp( vehicle_part &pt, int qty, bool keep_degradation, int new_degradation = -1 );

        /**
         * Apply damage to part constrained by range [0,durability] possibly destroying it
         * @param pt Part being damaged
         * @param qty maximum amount by which to adjust damage (negative permissible)
         * @return whether part was destroyed as a result of the damage
         */
        bool mod_hp( vehicle_part &pt, int qty );

        // check if given player controls this vehicle
        bool player_in_control( const Character &p ) const;
        // check if player controls this vehicle remotely
        bool remote_controlled( const Character &p ) const;

        // initializes parts and fuel state for randomly generated vehicle and calls refresh()
        void init_state( map &placed_on, int init_veh_fuel, int init_veh_status );

        // damages all parts of a vehicle by a random amount
        void smash( map &m, float hp_percent_loss_min = 0.1f, float hp_percent_loss_max = 1.2f,
                    float percent_of_parts_to_affect = 1.0f, point damage_origin = point_zero, float damage_size = 0 );

        void serialize( JsonOut &json ) const;
        // deserializes vehicle from json (including parts)
        void deserialize( const JsonObject &data );
        // deserializes parts from json to parts vector (clearing it first)
        void deserialize_parts( const JsonArray &data );
        // Vehicle parts list - all the parts on a single tile
        int print_part_list( const catacurses::window &win, int y1, int max_y, int width, int p,
                             int hl = -1, bool detail = false, bool include_fakes = true ) const;

        // Vehicle parts descriptions - descriptions for all the parts on a single tile
        void print_vparts_descs( const catacurses::window &win, int max_y, int width, int p,
                                 int &start_at, int &start_limit ) const;
        // towing functions
        void invalidate_towing( bool first_vehicle = false, Character *remover = nullptr );
        void do_towing_move();
        bool tow_cable_too_far() const;
        bool no_towing_slack() const;
        bool is_towing() const;
        bool has_tow_attached() const;
        int get_tow_part() const;
        bool is_external_part( const tripoint &part_pt ) const;
        bool is_towed() const;
        void set_tow_directions();
        /// @return true if vehicle is an appliance
        bool is_appliance() const;
        // owner functions
        bool is_owned_by( const Character &c, bool available_to_take = false ) const;
        bool is_old_owner( const Character &c, bool available_to_take = false ) const;
        std::string get_owner_name() const;
        void set_old_owner( const faction_id &temp_owner ) {
            theft_time = calendar::turn;
            old_owner = temp_owner;
        }
        void remove_old_owner() {
            theft_time = std::nullopt;
            old_owner = faction_id::NULL_ID();
        }
        void set_owner( const faction_id &new_owner ) {
            owner = new_owner;
        }
        void set_owner( const Character &c );
        faction_id get_owner() const {
            return owner;
        }
        faction_id get_old_owner() const {
            return old_owner;
        }
        bool has_owner() const {
            return !owner.is_null();
        }
        bool has_old_owner() const {
            return !old_owner.is_null();
        }
        bool handle_potential_theft( Character const &you, bool check_only = false, bool prompt = true );
        // project a tileray forward to predict obstacles
        std::set<point> immediate_path( const units::angle &rotate = 0_degrees );
        std::set<point> collision_check_points; // NOLINT(cata-serialize)
        void autopilot_patrol();
        units::angle get_angle_from_targ( const tripoint &targ ) const;
        void drive_to_local_target( const tripoint &target, bool follow_protocol );
        // Drive automatically towards some destination for one turn.
        autodrive_result do_autodrive( Character &driver );
        // Stop any kind of automatic vehicle control and apply the brakes.
        void stop_autodriving( bool apply_brakes = true );

        item init_cord( const tripoint &pos );
        void plug_in( const tripoint &pos );
        void connect( const tripoint &source_pos, const tripoint &target_pos );

        bool precollision_check( units::angle &angle, map &here, bool follow_protocol );
        // Try select any fuel for engine, returns true if some fuel is available
        bool auto_select_fuel( vehicle_part &vp );
        // Attempt to start an engine
        bool start_engine( vehicle_part &vp );
        // stop all engines
        void stop_engines();
        // Attempt to start the vehicle's active engines
        void start_engines( bool take_control = false, bool autodrive = false );

        // Engine backfire, making a loud noise
        void backfire( const vehicle_part &vp ) const;

        /**
         * @param dp The coordinate to mount at (in vehicle mount point coords)
         * @param vpi The part type to check
         * @return true if the part can be mounted at specified position.
         */
        ret_val<void> can_mount( const point &dp, const vpart_info &vpi ) const;

        // @returns true if part \p vp_to_remove can be uninstalled
        ret_val<void> can_unmount( const vehicle_part &vp_to_remove ) const;

        // install a part of type \p type at mount \p dp
        // @return installed part index or -1 if can_mount(...) failed
        int install_part( const point &dp, const vpart_id &type );

        // install a part of type \p type at mount \p dp with \p base (std::move -ing it)
        // @return installed part index or -1 if can_mount(...) failed
        int install_part( const point &dp, const vpart_id &type, item &&base );

        // install the given part \p vp (std::move -ing it)
        // @return installed part index or -1 if can_mount(...) failed
        int install_part( const point &dp, vehicle_part &&vp );

        struct rackable_vehicle {
            std::string name;
            vehicle *veh;
            std::vector<int> racks;
        };

        struct unrackable_vehicle {
            std::string name;
            std::vector<int> racks;
            std::vector<int> parts;
        };

        // attempts to find any nearby vehicles that can be racked on any of the list_of_racks
        // @returns vector of structs with data required to rack each vehicle
        std::vector<rackable_vehicle> find_vehicles_to_rack( int rack ) const;

        // attempts to find any racked vehicles that can be unracked on any of the list_of_racks
        // @returns vector of structs with data required to unrack each vehicle
        std::vector<unrackable_vehicle> find_vehicles_to_unrack( int rack ) const;

        // merge a previously found single tile vehicle into this vehicle
        bool merge_rackable_vehicle( vehicle *carry_veh, const std::vector<int> &rack_parts );
        // merges vehicles together by copying parts, does not account for any vehicle complexities
        bool merge_vehicle_parts( vehicle *veh );
        void merge_appliance_into_grid( vehicle &veh_target );

        bool is_powergrid() const;

        /**
         * @param handler A class that receives various callbacks, e.g. for placing items.
         * This handler is different when called during mapgen (when items need to be placed
         * on the temporary mapgen map), and when called during normal game play (when items
         * go on the main map g->m).
         */

        // Mark a part to be removed from the vehicle.
        // @returns true if the vehicle's 0,0 point shifted.
        bool remove_part( vehicle_part &vp );
        // Mark a part to be removed from the vehicle.
        // @returns true if the vehicle's 0,0 point shifted.
        bool remove_part( vehicle_part &vp, RemovePartHandler &handler );

        void part_removal_cleanup();
        // inner look for part_removal_cleanup.  returns true if a part is removed
        // also called by remove_fake_parts
        bool do_remove_part_actual();

        // remove a vehicle specified by a list of part indices
        bool remove_carried_vehicle( const std::vector<int> &carried_parts, const std::vector<int> &racks );
        // split the current vehicle into up to four vehicles if they have no connection other
        // than the structure part at exclude
        bool find_and_split_vehicles( map &here, std::set<int> exclude );
        // relocate passengers to the same part on a new vehicle
        void relocate_passengers( const std::vector<Character *> &passengers ) const;
        // Split a vehicle into an old vehicle and one or more new vehicles by moving vehicle_parts
        // from one the old vehicle to the new vehicles. Some of the logic borrowed from remove_part
        // skipped the grab, curtain, player activity, and engine checks because they deal with pos,
        // not a vehicle pointer
        // @param new_vehs vector of vectors of part indexes to move to new vehicles
        // @param new_vehicles vector of vehicle pointers containing the new vehicles; if empty, new
        // vehicles will be created
        // @param new_mounts vector of vector of mount points. must have one vector for every vehicle*
        // in new_vehicles, and forces the part indices in new_vehs to be mounted on the new vehicle
        // at those mount points
        // @param added_vehicles if not nullptr any newly added vehicles will be appended to the vector
        bool split_vehicles( map &here, const std::vector<std::vector <int>> &new_vehs,
                             const std::vector<vehicle *> &new_vehicles,
                             const std::vector<std::vector<point>> &new_mounts,
                             std::vector<vehicle *> *added_vehicles = nullptr );

        /** Get handle for base item of part */
        item_location part_base( int p );

        /**
         * Get the remote vehicle and part that a part is targeting.
         * Useful for, e.g. power cables that have a vehicle part on both sides.
         * @param vp_local Vehicle part that is connected to the remote part.
         */
        std::optional<std::pair<vehicle *, vehicle_part *>> get_remote_part(
                    const vehicle_part &vp_local ) const;
        /**
         * Remove the part on a targeted remote vehicle that a part is targeting.
         */
        void remove_remote_part( const vehicle_part &vp_local ) const;
        /**
         * Yields a range containing all parts (including broken ones) that can be
         * iterated over.
         */
        // TODO: maybe not include broken ones? Have a separate function for that?
        // TODO: rename to just `parts()` and rename the data member to `parts_`.
        vehicle_part_range get_all_parts() const;
        // Yields a range containing all parts including fake ones that only map cares about
        vehicle_part_with_fakes_range get_all_parts_with_fakes( bool with_inactive = false ) const;
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
        std::vector<int> parts_at_relative( const point &dp, bool use_cache,
                                            bool include_fake = false ) const;

        /**
        *  Returns index of part at mount point \p pt which has given \p f flag
        *  @note does not use relative_parts cache
        *  @param pt only returns parts from this mount point
        *  @param f required flag in part's vpart_info flags collection
        *  @param unbroken if true also requires the part to be !is_broken
        *  @returns part index or -1
        */
        int part_with_feature( const point &pt, const std::string &f, bool unbroken ) const;
        /**
        *  Returns part index at mount point \p pt which has given \p f flag
        *  @note uses relative_parts cache
        *  @param pt only returns parts from this mount point
        *  @param f required flag in part's vpart_info flags collection
        *  @param unbroken if true also requires the part to be !is_broken()
        *  @returns part index or -1
        */
        int part_with_feature( const point &pt, vpart_bitflags f, bool unbroken ) const;
        /**
        *  Returns \p p or part index at mount point \p pt which has given \p f flag
        *  @note uses relative_parts cache
        *  @param p index of part to start searching from
        *  @param f required flag in part's vpart_info flags collection
        *  @param unbroken if true also requires the part to be !is_broken()
        *  @returns part index or -1
        */
        int part_with_feature( int p, vpart_bitflags f, bool unbroken ) const;
        /**
        *  Returns index of part at mount point \p pt which has given \p f flag
        *  and is_available(), or -1 if no such part or it's not is_available()
        *  @note does not use relative_parts cache
        *  @param pt only returns parts from this mount point
        *  @param f required flag in part's vpart_info flags collection
        *  @param unbroken if true also requires the part to be !is_broken
        *  @returns part index or -1
        */
        int avail_part_with_feature( const point &pt, const std::string &f ) const;
        /**
        *  Returns \p p or part index at mount point \p pt which has given \p f flag
        *  and is_available(), or -1 if no such part or it's not is_available()
        *  @note uses relative_parts cache
        *  @param p index of part to start searching from
        *  @param f required flag in part's vpart_info flags collection
        *  @param unbroken if true also requires the part to be !is_broken()
        *  @returns part index or -1
        */
        int avail_part_with_feature( int p, vpart_bitflags f ) const;

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
                part_status_flag condition );
        std::vector<const vehicle_part *> get_parts_at( const tripoint &pos,
                const std::string &flag, part_status_flag condition ) const;

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

        /**
         *  Return the index of the next part to lock at part `p`'s location.
         *
         *  The next part to lock is the first closed, unlocked part in the list of
         *  parts at part `p`'s coordinates with the lockable_door flag.
         *
         *  @param p part whose coordinates provide the location to check
         *  @param outside if true, give parts that can be locked from outside only
         *  @returns a part index or -1 if no part
         */
        int next_part_to_lock( int p, bool outside = false ) const;

        /**
         *  Return the index of the next part to unlock at part `p`'s location.
         *
         *  The next part to unlock is the first locked part in the list of
         *  parts at part `p`'s coordinates with the lockable_door flag.
         *
         *  @param p part whose coordinates provide the location to check
         *  @param outside if true, give parts that can be unlocked from outside only
         *  @returns part index or -1 if no part
         */
        int next_part_to_unlock( int p, bool outside = false ) const;

        // returns indices of all parts in the given location slot
        std::vector<int> all_parts_at_location( const std::string &location ) const;
        // shifts an index to next available of that type for NPC activities
        int get_next_shifted_index( int original_index, Character &you ) const;
        // Given a part and a flag, returns the indices of all contiguously adjacent parts
        // with the same flag on the X and Y Axis
        std::vector<std::vector<int>> find_lines_of_parts( int part, const std::string &flag ) const;

        // Translate mount coordinates "p" using current pivot direction and anchor and return tile coordinates
        point coord_translate( const point &p ) const;

        // Translate mount coordinates "p" into tile coordinates "q" using given pivot direction and anchor
        void coord_translate( const units::angle &dir, const point &pivot, const point &p,
                              tripoint &q ) const;
        // Translate mount coordinates "p" into tile coordinates "q" using given tileray and anchor
        // should be faster than previous call for repeated translations
        void coord_translate( tileray tdir, const point &pivot, const point &p, tripoint &q ) const;

        tripoint mount_to_tripoint( const point &mount ) const;
        tripoint mount_to_tripoint( const point &mount, const point &offset ) const;

        // Seek a vehicle part which obstructs tile with given coordinates relative to vehicle position
        int part_at( const point &dp ) const;
        int part_displayed_at( const point &dp, bool include_fake = false,
                               bool below_roof = true, bool roof = true ) const;
        int roof_at_part( int p ) const;

        // Finds index of a given vehicle_part in parts vector, compared by address
        // @return index or -1 if part is nullptr, not found or removed and include_removed is false.
        int index_of_part( const vehicle_part *part, bool include_removed = false ) const;

        // @param dp mount point to check
        // @param rotate symbol is rotated using vehicle facing, or north facing if false
        // @param include_fake if true fake parts are included
        // @param below_roof if true parts below roof are included
        // @param roof if true roof parts are included
        // @returns filled vpart_display struct or default constructed if no part displayed
        vpart_display get_display_of_tile( const point &dp, bool rotate = true, bool include_fake = true,
                                           bool below_roof = true, bool roof = true ) const;

        // Get all printable fuel types
        std::vector<itype_id> get_printable_fuel_types() const;

        // Vehicle fuel indicators (all of them)
        void print_fuel_indicators(
            const catacurses::window &win, const point &, int start_index = 0,
            bool fullsize = false, bool verbose = false, bool desc = false,
            bool isHorizontal = false );

        /**
         * Vehicle speed gauge
         *
         * Prints: `target speed` `<` `current speed` `speed unit`
         * @param spacing Sets size of space between components
         * @warning if spacing is negative it is changed to 0
         */
        void print_speed_gauge( const catacurses::window &win, const point &, int spacing = 0 ) const;

        // Pre-calculate mount points for (idir=0) - current direction or
        // (idir=1) - next turn direction
        void precalc_mounts( int idir, const units::angle &dir, const point &pivot );

        // get a list of part indices where is a passenger inside
        std::vector<int> boarded_parts() const;

        // get a list of part indices and Creature pointers with a rider
        std::vector<rider_data> get_riders() const;

        // is given character a passenger of this vehicle
        bool is_passenger( Character &c ) const;
        // get passenger at part p
        Character *get_passenger( int you ) const;
        // get monster on a boardable part at p
        monster *get_monster( int p ) const;

        bool enclosed_at( const tripoint &pos ); // not const because it calls refresh_insides
        // Returns the location of the vehicle in global map square coordinates.
        tripoint_abs_ms global_square_location() const;
        // Returns the location of the vehicle in global overmap terrain coordinates.
        tripoint_abs_omt global_omt_location() const;
        // Returns the coordinates (in map squares) of the vehicle relative to the local map.
        // Warning: Don't assume this position contains a vehicle part
        tripoint global_pos3() const;
        // Warning: Don't assume this position contains a vehicle part
        tripoint_bub_ms pos_bub() const;
        /**
         * Get the coordinates of the studied part of the vehicle
         */
        // TODO: fix point types (remove global_part_pos3 in favour of
        // bub_part_pos)
        tripoint global_part_pos3( const int &index ) const;
        tripoint global_part_pos3( const vehicle_part &pt ) const;
        tripoint_bub_ms bub_part_pos( int index ) const;
        tripoint_bub_ms bub_part_pos( const vehicle_part &pt ) const;
        /**
         * All the fuels that are in all the tanks in the vehicle, nicely summed up.
         * Note that empty tanks don't count at all. The value is the amount as it would be
         * reported by @ref fuel_left, it is always greater than 0. The key is the fuel item type.
         */
        std::map<itype_id, int> fuels_left() const;

        /**
         * All the individual fuel items that are in all the tanks in the vehicle.
        */
        std::list<item *> fuel_items_left();

        // Checks how much certain fuel left in tanks.
        int64_t fuel_left( const itype_id &ftype,
                           const std::function<bool( const vehicle_part & )> &filter = return_true<const vehicle_part &> )
        const;
        // Checks how much of an engine's current fuel is left in the tanks.
        int engine_fuel_left( const vehicle_part &vp ) const;
        // Returns total vehicle fuel capacity for the given fuel type
        int fuel_capacity( const itype_id &ftype ) const;

        // drains a fuel type (e.g. for the kitchen unit)
        // returns amount actually drained, does not engage reactor
        int drain( const itype_id &ftype, int amount,
                   const std::function<bool( vehicle_part & )> &filter = return_true< vehicle_part &> );
        int drain( int index, int amount );
        /**
         * Consumes enough fuel by energy content. Does not support cable draining.
         * @param ftype Type of fuel
         * @param wanted_energy Desired amount of energy of fuel to consume
         * @return Amount of energy actually consumed. May be more or less than energy.
         */
        units::energy drain_energy( const itype_id &ftype, units::energy wanted_energy );

        // fuel consumption of vehicle engines of given type
        units::power basic_consumption( const itype_id &ftype ) const;
        // Fuel consumption mL/hour
        int consumption_per_hour( const itype_id &ftype, units::energy fuel_per_s ) const;

        void consume_fuel( int load, bool idling );

        /**
         * Maps used fuel to its basic (unscaled by load/strain) consumption.
         */
        std::map<itype_id, units::power> fuel_usage() const;

        /**
        * Fuel usage for specific engine
        * @param e is the index of the engine in the engines array
        */
        units::power engine_fuel_usage( const vehicle_part &vp ) const;
        // Returns all active, available, non-destroyed vehicle lights
        std::vector<vehicle_part *> lights();

        void update_alternator_load();

        // Total drain or production of electrical power from engines.
        units::power total_engine_epower() const;
        // Total production of electrical power from alternators.
        units::power total_alternator_epower() const;
        // Total power (W) currently being produced by all solar panels.
        units::power total_solar_epower() const;
        // Total power currently being produced by all wind turbines.
        units::power total_wind_epower() const;
        // Total power currently being produced by all water wheels.
        units::power total_water_wheel_epower() const;
        // Total power drain across all vehicle accessories.
        units::power total_accessory_epower() const;
        // Total power draw from all cable-connected devices. Is cleared every turn during idle().
        units::power linked_item_epower_this_turn; // NOLINT(cata-serialize)
        // Net power draw or drain on batteries.
        units::power net_battery_charge_rate( bool include_reactors ) const;
        // Maximum available power available from all reactors. Power from
        // reactors is only drawn when batteries are empty.
        units::power max_reactor_epower() const;
        // Active power from reactors that is actually being drained by batteries.
        units::power active_reactor_epower() const;
        // Produce and consume electrical power, with excess power stored or
        // taken from batteries.
        void power_parts();

        // Current and total battery power (kJ) level as a pair
        std::pair<int, int> battery_power_level() const;

        // Current and total battery power (kJ) level of all connected vehicles as a pair
        std::pair<int, int> connected_battery_power_level() const;

        /**
        * @param apply_loss if true apply wire loss when charge crosses vehicle power cables
        * @return battery charge in kJ from all connected batteries
        */
        int64_t battery_left( bool apply_loss = true ) const;

        /**
         * Charges batteries in connected vehicles/appliances
         * @param amount to charge in kJ
         * @param apply_loss if true apply wire loss when charge crosses vehicle power cables
         * @return 0 or left over charge in kJ which does not fit in any connected batteries
         */
        int charge_battery( int amount, bool apply_loss = true );

        /**
         * Discharges batteries in connected vehicles/appliances
         * @param amount to discharge in kJ
         * @param apply_loss if true apply wire loss when charge crosses vehicle power cables
         * @return 0 or unfulfilled part of \p amount in kJ
         */
        int discharge_battery( int amount, bool apply_loss = true );

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
        units::power total_power( bool fueled = true, bool safe = false ) const;

        // Get ground acceleration gained by combined power of all engines. If fueled == true,
        // then only engines which the vehicle has fuel for are included
        int ground_acceleration( bool fueled = true, int at_vel_in_vmi = -1 ) const;
        // Get water acceleration gained by combined power of all engines. If fueled == true,
        // then only engines which the vehicle has fuel for are included
        int water_acceleration( bool fueled = true, int at_vel_in_vmi = -1 ) const;
        // get air acceleration gained by combined power of all engines. If fueled == true,
        // then only engines which the vehicle hs fuel for are included
        int rotor_acceleration( bool fueled = true, int at_vel_in_vmi = -1 ) const;
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
        // get maximum air velocity based on rotor physics
        int max_rotor_velocity( bool fueled = true ) const;
        // Get maximum velocity for the current movement mode
        int max_velocity( bool fueled = true ) const;
        // Get maximum reverse velocity for the current movement mode
        int max_reverse_velocity( bool fueled = true ) const;

        // Get safe ground velocity gained by combined power of all engines.
        // If fueled == true, then only the engines which the vehicle has fuel for are included
        int safe_ground_velocity( bool fueled = true ) const;
        // get safe air velocity gained by combined power of all engines.
        // if fueled == true, then only the engines which the vehicle hs fuel for are included
        int safe_rotor_velocity( bool fueled = true ) const;
        // Get safe water velocity gained by combined power of all engines.
        // If fueled == true, then only the engines which the vehicle has fuel for are included
        int safe_water_velocity( bool fueled = true ) const;
        // Get maximum velocity for the current movement mode
        int safe_velocity( bool fueled = true ) const;

        // Generate field from a part, either at front or back of vehicle depending on velocity.
        void spew_field( double joules, int part, field_type_id type, int intensity = 1 ) const;

        // Loop through engines and generate noise and smoke for each one
        void noise_and_smoke( int load, time_duration time = 1_turns );

        /**
         * Calculates the sum of the area under the wheels of the vehicle.
         */
        int wheel_area() const;
        // average off-road rating for displaying off-road performance
        float average_offroad_rating() const;

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
         * watertight hull height in meters measures distance from bottom of vehicle
         * to the point where the vehicle will start taking on water
         */
        double water_hull_height() const;

        /**
         * water draft in meters - how much of the vehicle's body is under water
         * must be less than the hull height or the boat will sink
         * at some point, also add boats with deep draft running around
         */
        double water_draft() const;

        /**
         * can_float
         * does the vehicle have freeboard or does it overflow with water?
         */
        bool can_float() const;
        /**
         * is the vehicle mostly in water or mostly on fairly dry land?
         */
        bool is_in_water( bool deep_water = false ) const;
        /**
         * should vehicle be handled using watercraft logic
         * as determined by amount of water it is in (and whether it is amphibious)
         * result being true does not guarantee it is viable boat -- check @ref can_float()
         */
        bool is_watercraft() const;
        /**
         * is the vehicle flying? is it a rotorcraft?
         */
        double lift_thrust_of_rotorcraft( bool fuelled, bool safe = false ) const;
        bool has_sufficient_rotorlift() const;
        int get_z_change() const;
        bool is_flying_in_air() const;
        void set_flying( bool new_flying_value );
        bool is_rotorcraft() const;
        // Can the vehicle safely fly? E.g. there haven't been any player modifications
        // of non-simple parts
        bool is_flyable() const;
        void set_flyable( bool val );
        // Would interacting with this part prevent the vehicle from being flyable?
        bool would_install_prevent_flyable( const vpart_info &vpinfo, const Character &pc ) const;
        bool would_removal_prevent_flyable( const vehicle_part &vp, const Character &pc ) const;
        bool would_repair_prevent_flyable( const vehicle_part &vp, const Character &pc ) const;
        // Can control this vehicle?
        bool can_control_in_air( const Character &pc ) const;
        bool can_control_on_land( const Character &pc ) const;
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
        units::power static_drag( bool actual = true ) const;

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
        // @param on_map if true vehicle processes noise/smoke and updates time
        void idle( bool on_map );
        // continuous processing for running vehicle alarms
        void alarm();
        // leak from broken tanks
        void slow_leak();

        // thrust (1) or brake (-1) vehicle
        // @param z = z thrust for helicopters etc
        void thrust( int thd, int z = 0 );

        /**
         * if smart controller is enabled, turns on and off engines depending on load and battery level
         * @param thrusting must be true when called from vehicle::thrust and vehicle is thrusting
         * @param k_traction_cache cached value of vehicle::k_traction, if empty, will be computed
         */
        void smart_controller_handle_turn( const std::optional<float> &k_traction_cache = std::nullopt );

        bool has_available_electric_engine();
        void disable_smart_controller_if_needed();

        //deceleration due to ground friction and air resistance
        int slowdown( int velocity ) const;

        // depending on skid vectors, chance to recover.
        void possibly_recover_from_skid();

        //forward component of velocity.
        float forward_velocity() const;

        // cruise control
        void cruise_thrust( int amount );

        // turn vehicle left (negative) or right (positive), degrees
        void turn( units::angle deg );

        // Returns if any collision occurred
        bool collision( std::vector<veh_collision> &colls,
                        const tripoint &dp,
                        bool just_detect, bool bash_floor = false );

        // Handle given part collision with vehicle, monster/NPC/player or terrain obstacle
        // Returns collision, which has type, impulse, part, & target.
        veh_collision part_collision( int part, const tripoint &p,
                                      bool just_detect, bool bash_floor );

        // Process the trap beneath
        void handle_trap( const tripoint &p, vehicle_part &vp_wheel );
        void activate_magical_follow();
        void activate_animal_follow();
        /**
         * vehicle is driving itself
         */
        void selfdrive( const point & );
        /**
         * can the helicopter descend/ascend here?
         */
        bool check_heli_descend( Character &p ) const;
        bool check_heli_ascend( Character &p ) const;
        bool check_is_heli_landed();
        /**
         * Player is driving the vehicle
         * @param p direction player is steering
         * @param z for vertical movement - e.g helicopters
         */
        void pldrive( Character &driver, const point &p, int z = 0 );

        /**
        * Flags item \p tool with PSEUDO, if it has MOD pocket then a `pseudo_magazine_mod` is
        * installed and a `pseudo_magazine` is inserted into the magazine well pocket with however
        * many ammo charges of the first ammo type required by the tool are available to this vehicle.
        * @param tool the tool item to modify
        * @return amount of ammo in the `pseudo_magazine` or 0
        */
        int prepare_tool( item &tool ) const;
        /**
        * if \p tool is not an itype with tool != nullptr this returns { itype::NULL_ID(), 0 } pair
        * @param tool the item to examine
        * @return a pair of tool's first ammo type and the amount of it available from tanks / batteries
        */
        std::pair<const itype_id &, int> tool_ammo_available( const itype_id &tool_type ) const;
        /**
        * @return pseudo- and attached tools available from this vehicle part,
        * marked with PSEUDO flags, pseudo_magazine_mod and pseudo_magazine attached, magazines filled
        * with the first ammo type required by the tool. pseudo tools are mapped to their hotkey if exists.
        */
        std::map<item, int> prepare_tools( const vehicle_part &vp ) const;

        /**
         * Update an item's active status, for example when adding
         * hot or perishable liquid to a container.
         */
        void make_active( item_location &loc );
        /**
         * Try to add an item to part's cargo.
         * @return iterator to added item or std::nullopt if it can't be put here (not a cargo part, adding
         * this would violate the volume limit or item count limit, not all charges can fit, etc.)
         */
        std::optional<vehicle_stack::iterator> add_item( vehicle_part &vp, const item &itm );
        /**
         * Add an item counted by charges to the part's cargo.
         *
         * @returns The number of charges added.
         */
        int add_charges( vehicle_part &vp, const item &itm );

        // remove item from part's cargo
        bool remove_item( vehicle_part &vp, item *it );
        vehicle_stack::iterator remove_item( vehicle_part &vp, const vehicle_stack::const_iterator &it );

        // HACK: callers could modify items through this
        // TODO: a const version of vehicle_stack is needed
        vehicle_stack get_items( const vehicle_part &vp ) const;
        vehicle_stack get_items( vehicle_part &vp );

        std::vector<item> &get_tools( vehicle_part &vp );
        const std::vector<item> &get_tools( const vehicle_part &vp ) const;

        // Generates starting items in the car, should only be called when placed on the map
        void place_spawn_items();

        void place_zones( map &pmap ) const;

        void gain_moves();

        // if its a summoned vehicle - its gotta disappear at some point, return true if destroyed
        bool decrement_summon_timer();

        // reduces velocity to 0
        void stop();

        void refresh_insides();

        void unboard_all() const;

        // Damage individual part. bash means damage
        // must exceed certain threshold to be subtracted from hp
        // (a lot light collisions will not destroy parts)
        // Returns damage bypassed
        int damage( map &here, int p, int dmg, const damage_type_id &type = damage_type_id( "bash" ),
                    bool aimed = true );

        // damage all parts (like shake from strong collision), range from dmg1 to dmg2
        void damage_all( int dmg1, int dmg2, const damage_type_id &type, const point &impact );

        //Shifts the coordinates of all parts and moves the vehicle in the opposite direction.
        void shift_parts( map &here, const point &delta );
        bool shift_if_needed( map &here );

        /**
         * Drop parts with UNMOUNT_ON_MOVE onto the ground.
         * @param shed_cables If cable parts should also be dropped.
         * @param - If set to trinary::NONE, the default, don't drop any cables.
         * @param - If set to trinary::SOME, calculate cable length, updating remote parts, and drop if it's too long.
         * @param - If set to trinary::ALL, drop all cables.
         * @param dst Future vehicle position, used for calculating cable length when shed_cables == trinary::SOME.
         */
        void shed_loose_parts( trinary shed_cables = trinary::NONE, const tripoint_bub_ms *dst = nullptr );

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
        turret_data turret_query( const tripoint &pos );

        /** Set targeting mode for specific turrets */
        void turrets_set_targeting();

        /** Set firing mode for specific turrets */
        void turrets_set_mode();

        /** Select a single ready turret, aim it using the aiming UI and fire. */
        void turrets_aim_and_fire_single();

        /*
         * Find all ready turrets that are set to manual mode, aim them using the aiming UI and fire.
         * @param show_msg Show 'no such turrets found' message. Does not affect returned value.
         * @return False if there are no such turrets
         */
        bool turrets_aim_and_fire_all_manual( bool show_msg = false );

        /** Set target for automatic turrets using the aiming UI */
        void turrets_override_automatic_aim();

        /*
         * Fire turret at automatically acquired target
         * @return number of shots actually fired (which may be zero)
         */
        int automatic_fire_turret( vehicle_part &pt );

    private:
        /*
         * Find all turrets that are ready to fire.
         * @param manual Include turrets set to 'manual' targeting mode
         * @param automatic Include turrets set to 'automatic' targeting mode
         */
        std::vector<vehicle_part *> find_all_ready_turrets( bool manual, bool automatic );

        /*
         * Select target using the aiming UI and set turrets to aim at it.
         * Assumes all turrets are ready to fire.
         * @return False if target selection was aborted / no target was found
         */
        bool turrets_aim( std::vector<vehicle_part *> &turrets );

        /*
         * Select target using the aiming UI, set turrets to aim at it and fire them.
         * Assumes all turrets are ready to fire.
         * @return Number of shots fired by all turrets (which may be zero)
         */
        int turrets_aim_and_fire( std::vector<vehicle_part *> &turrets );

    public:
        /**
         *  Try to assign a crew member (who must be a player ally) to a specific seat
         *  @note enforces NPC's being assigned to only one seat (per-vehicle) at once
         */
        bool assign_seat( vehicle_part &pt, const npc &who );

        // Update the set of occupied points and return a reference to it
        const std::set<tripoint> &get_points( bool force_refresh = false, bool no_fake = false ) const;

        /**
        * Consumes specified charges (or fewer) from the vehicle part
        * @param what specific type of charge required, e.g. 'battery'
        * @param qty maximum charges to consume. On return set to number of charges not found (or zero)
        * @param filter Must return true for use to occur.
        * @return items that provide consumed charges
        */
        std::list<item> use_charges( const vpart_position &vp, const itype_id &type, int &quantity,
                                     const std::function<bool( const item & )> &filter, bool in_tools = false );

        // opens/closes/locks doors or multipart doors
        void open( int part_index );
        void close( int part_index );
        void lock( int part_index );
        void unlock( int part_index );
        // returns whether the door can be locked with an attached DOOR_LOCKING part.
        bool part_has_lock( int part_index ) const;
        bool can_close( int part_index, Character &who );

        // @returns true if vehicle only has foldable parts
        bool is_foldable() const;
        // @returns how long should folding activity take
        time_duration folding_time() const;
        // @returns how long should unfolding activity take
        time_duration unfolding_time() const;
        // @returns item of this vehicle folded
        item get_folded_item() const;
        // restores vehicle parts from a folded item
        // @returns true if succeeded
        bool restore_folded_parts( const item &it );

        //true if an alarm part is installed on the vehicle
        bool has_security_working() const;
        // unlocks the vehicle, turns off SECURITY parts, clears engine immobilizer faults
        void unlock();
        /**
         *  Opens everything that can be opened on the same tile as `p`
         */
        void open_all_at( int p );

        // Honk the vehicle's horn, if there are any
        void honk_horn() const;
        void reload_seeds( const tripoint &pos );
        void beeper_sound() const;
        void play_music() const;
        void play_chimes() const;
        void operate_planter();
        void autopilot_patrol_check();
        void toggle_autopilot();
        void enable_patrol();
        void toggle_tracking();
        //scoop operation,pickups, battery drain, etc.
        void operate_scoop();
        void operate_reaper();
        // for destroying any terrain around vehicle part. Automated mining tool.
        void crash_terrain_around();
        void transform_terrain();
        //main method for the control of individual engines
        void control_engines();
        //returns whether the engine is enabled or not, and has fueltype
        bool is_engine_type_on( const vehicle_part &vp, const itype_id &ft ) const;
        //returns whether the engine is enabled or not
        bool is_engine_on( const vehicle_part &vp ) const;
        //returns whether the engine uses specified fuel type
        bool is_engine_type( const vehicle_part &vp, const itype_id &ft ) const;
        //returns whether the engine uses one of specific "combustion" fuel types (gas, diesel and diesel substitutes)
        bool is_engine_type_combustion( const vehicle_part &vp ) const;
        //returns whether the alternator is operational
        bool is_alternator_on( const vehicle_part &vp ) const;
        // try to turn engine on or off
        // (tries to start it and toggles it on if successful, shutdown is always a success)
        // returns true if engine status was changed
        bool start_engine( vehicle_part &vp, bool turn_on );
        //true if an engine exists with specified type
        //If enabled true, this engine must be enabled to return true
        bool has_engine_type( const itype_id &ft, bool enabled ) const;
        monster *get_harnessed_animal() const;
        //true if an engine exists without the specified type
        //If enabled true, this engine must be enabled to return true
        bool has_engine_type_not( const itype_id &ft, bool enabled ) const;
        // @returns engine conflict string if another engine has same exclusion list or std::nullopt
        std::optional<std::string> has_engine_conflict( const vpart_info &possible_conflict ) const;
        //returns true if the engine doesn't consume fuel
        bool is_perpetual_type( const vehicle_part &vp ) const;
        //if necessary, damage this engine
        void do_engine_damage( vehicle_part &vp, int strain );
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
        void precalculate_vehicle_turning( units::angle new_turn_dir, bool check_rail_direction,
                                           ter_furn_flag ter_flag_to_check, int &wheels_on_rail,
                                           int &turning_wheels_that_are_one_axis_counter ) const;
        bool allow_auto_turn_on_rails( units::angle &corrected_turn_dir ) const;
        bool allow_manual_turn_on_rails( units::angle &corrected_turn_dir ) const;
        bool is_wheel_state_correct_to_turn_on_rails( int wheels_on_rail, int wheel_count,
                int turning_wheels_that_are_one_axis ) const;
        /**
         * Update the submap coordinates and update the tracker info in the overmap
         * (if enabled).
         * This should be called only when the vehicle has actually been moved, not when
         * the map is just shifted (in the later case simply set smx/smy directly).
         */
        void set_submap_moved( const tripoint &p );
        void use_autoclave( int p );
        void use_washing_machine( int p );
        void use_dishwasher( int p );
        void use_monster_capture( int part, const tripoint &pos );
        void use_harness( int part, const tripoint &pos );

        void build_electronics_menu( veh_menu &menu );
        void build_bike_rack_menu( veh_menu &menu, int part );
        void build_interact_menu( veh_menu &menu, const tripoint &p, bool with_pickup );
        void interact_with( const tripoint &p, bool with_pickup = false );

        std::string disp_name() const;

        /** Required strength to be able to successfully lift the vehicle unaided by equipment */
        int lift_strength() const;

        // Called by map.cpp to make sure the real position of each zone_data is accurate
        bool refresh_zones();

        bounding_box get_bounding_box( bool use_precalc = true, bool no_fake = false );
        // Retroactively pass time spent outside bubble
        // Funnels, solar panels
        void update_time( const time_point &update_to );

        // The faction that owns this vehicle.
        faction_id owner = faction_id::NULL_ID();
        // The faction that previously owned this vehicle
        faction_id old_owner = faction_id::NULL_ID();

    private:
        mutable double coefficient_air_resistance = 1; // NOLINT(cata-serialize)
        mutable double coefficient_rolling_resistance = 1; // NOLINT(cata-serialize)
        mutable double coefficient_water_resistance = 1; // NOLINT(cata-serialize)
        mutable double draft_m = 1; // NOLINT(cata-serialize)
        mutable double hull_height = 0.3; // NOLINT(cata-serialize)

        // Global location when cache was last refreshed.
        mutable tripoint occupied_cache_pos = { -1, -1, -1 }; // NOLINT(cata-serialize)
        // Vehicle facing when cache was last refreshed.
        mutable units::angle occupied_cache_direction = 0_degrees; // NOLINT(cata-serialize)
        // Cached points occupied by the vehicle
        mutable std::set<tripoint> occupied_points; // NOLINT(cata-serialize)

        // Master list of parts installed in the vehicle.
        std::vector<vehicle_part> parts; // NOLINT(cata-serialize)
        // Used in savegame.cpp to only save real parts to json
        std::vector<std::reference_wrapper<const vehicle_part>> real_parts() const;
        // Map of edge parts and their adjacency information
        std::map<point, vpart_edge_info> edges; // NOLINT(cata-serialize)
        // For a given mount point, returns its adjacency info
        vpart_edge_info get_edge_info( const point &mount ) const;

        // Removes fake parts from the parts vector
        void remove_fake_parts( bool cleanup = true );
        bool should_enable_fake( const tripoint &fake_precalc, const tripoint &parent_precalc,
                                 const tripoint &neighbor_precalc ) const;
    public:
        // Number of parts contained in this vehicle
        int part_count() const;
        // Number of real parts in this vehicle, iterates parts to count
        int part_count_real() const;
        // Number of real parts in this vehicle, returns parts.size() - fake_parts.size()
        int part_count_real_cached() const;

        // Returns the vehicle_part with the given part number
        vehicle_part &part( int part_num );
        const vehicle_part &part( int part_num ) const;
        // get the parent part of a fake part or return part_num otherwise
        int get_non_fake_part( int part_num ) const;
        // Updates active state on all fake_mounts based on whether they can fill a gap
        // map.cpp calls this in displace_vehicle
        void update_active_fakes();

        /**
        * Generate the corresponding item from this vehicle part. It includes
        * the hp (item damage), fuel charges (battery or liquids), aspect, ...
        */
        item part_to_item( const vehicle_part &vp ) const;

        // Updates the internal precalculated mount offsets after the vehicle has been displaced
        // used in map::displace_vehicle()
        std::set<int> advance_precalc_mounts( const point &new_pos, const tripoint &src,
                                              const tripoint &dp, int ramp_offset,
                                              bool adjust_pos, std::set<int> parts_to_move );
        // make sure the vehicle is supported across z-levels or on the same z-level
        bool level_vehicle();

        // These are lists of part numbers used for quick lookup of various kinds of vehicle parts.
        // They are rebuilt in vehicle::refresh()
        std::vector<int> alternators; // NOLINT(cata-serialize)
        std::vector<int> engines; // NOLINT(cata-serialize)
        std::vector<int> reactors; // NOLINT(cata-serialize)
        std::vector<int> solar_panels; // NOLINT(cata-serialize)
        std::vector<int> wind_turbines; // NOLINT(cata-serialize)
        std::vector<int> water_wheels; // NOLINT(cata-serialize)
        std::vector<int> sails; // NOLINT(cata-serialize)
        std::vector<int> funnels; // NOLINT(cata-serialize)
        std::vector<int> emitters; // NOLINT(cata-serialize)
        // Parts that will fall off and cables that might disconnect when the vehicle moves.
        std::vector<int> loose_parts; // NOLINT(cata-serialize)
        std::vector<int> wheelcache; // NOLINT(cata-serialize)
        std::vector<int> rotors; // NOLINT(cata-serialize)
        std::vector<int> rail_wheelcache; // NOLINT(cata-serialize)
        std::vector<int> steering; // NOLINT(cata-serialize)
        // Intended to be a misc list, but currently only security systems.
        std::vector<int> speciality; // NOLINT(cata-serialize)
        std::vector<int> floating; // NOLINT(cata-serialize)
        std::vector<int> batteries; // NOLINT(cata-serialize)
        std::vector<int> fuel_containers; // NOLINT(cata-serialize)
        std::vector<int> turret_locations; // NOLINT(cata-serialize)
        std::vector<int> mufflers; // NOLINT(cata-serialize)
        std::vector<int> planters; // NOLINT(cata-serialize)
        std::vector<int> accessories; // NOLINT(cata-serialize)
        std::vector<int> cable_ports; // NOLINT(cata-serialize)
        std::vector<int> fake_parts; // NOLINT(cata-serialize)
        std::vector<int> control_req_parts; // NOLINT(cata-serialize)

        // config values
        std::string name;   // vehicle name
        /**
         * Type of the vehicle as it was spawned. This will never change, but it can be an invalid
         * type (e.g. if the definition of the prototype has been removed from json or if it has been
         * spawned with the default constructor).
         */
        vproto_id type;
        // parts_at_relative(dp) is used a lot (to put it mildly)
        std::map<point, std::vector<int>> relative_parts; // NOLINT(cata-serialize)
        std::set<label> labels;            // stores labels
        std::set<std::string> tags;        // Properties of the vehicle
        // After fuel consumption, this tracks the remainder of fuel < 1, and applies it the next time.
        // The value is negative.
        std::map<itype_id, units::energy> fuel_remainder;
        std::map<itype_id, units::energy> fuel_used_last_turn;
        std::unordered_multimap<point, zone_data> loot_zones;
        active_item_cache active_items; // NOLINT(cata-serialize)
        // a magic vehicle, powered by magic.gif
        bool magic = false;
        // when does the magic vehicle disappear?
        std::optional<time_duration> summon_time_limit = std::nullopt;
        // cached values of the factors that determined last chosen engine state
        // NOLINTNEXTLINE(cata-serialize)
        std::optional<smart_controller_cache> smart_controller_state = std::nullopt;
        // SC config. optional, as majority of vehicles don't have SC installed
        std::optional<smart_controller_config> smart_controller_cfg = std::nullopt;
        bool has_enabled_smart_controller = false; // NOLINT(cata-serialize)

        void add_tag( const std::string &tag );
        bool has_tag( const std::string &tag ) const;

    private:
        safe_reference_anchor anchor; // NOLINT(cata-serialize)
        mutable units::mass mass_cache; // NOLINT(cata-serialize)
        // cached pivot point
        mutable point pivot_cache; // NOLINT(cata-serialize)
        /*
         * The co-ordinates of the bounding box of the vehicle's mount points
         */
        mutable point mount_max; // NOLINT(cata-serialize)
        mutable point mount_min; // NOLINT(cata-serialize)
        mutable point mass_center_precalc; // NOLINT(cata-serialize)
        mutable point mass_center_no_precalc; // NOLINT(cata-serialize)
        tripoint autodrive_local_target = tripoint_zero; // current node the autopilot is aiming for
        class autodrive_controller;
        std::shared_ptr<autodrive_controller> active_autodrive_controller; // NOLINT(cata-serialize)

    public:
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
         * not change therefore no call to set_submap_moved is required.
         */
        tripoint sm_pos = tripoint_zero; // NOLINT(cata-serialize)

        // alternator load as a percentage of engine power, in units of 0.1% so 1000 is 100.0%
        int alternator_load = 0; // NOLINT(cata-serialize)
        // Turn the vehicle was last processed
        time_point last_update = calendar::before_time_starts;
        // save values
        /**
         * Position of the vehicle *inside* the submap that contains the vehicle.
         * This will (nearly) always be in the range (0...SEEX-1).
         * Note that vehicles are "moved" by map::displace_vehicle. You should not
         * set them directly, except when initializing the vehicle or during mapgen.
         */
        point pos = point_zero;
        // vehicle current velocity, mph * 100
        int velocity = 0;
        /**
         * vehicle continuous moving average velocity
         * see https://en.wikipedia.org/wiki/Moving_average#Exponential_moving_average
         * alpha is 0.5:  avg_velocity = avg_velocity + 0.5(velocity - avg_velocity) = 0.5 avg_velocity + 0.5 velocity
         */
        int avg_velocity = 0;
        // velocity the vehicle is trying to achieve
        int cruise_velocity = 0;
        // Only used for collisions, vehicle falls instantly
        int vertical_velocity = 0;
        // id of the om_vehicle struct corresponding to this vehicle
        int om_id = 0;
        // direction, to which vehicle is turning (player control). will rotate frame on next move
        // must be a multiple of 15 degrees
        units::angle turn_dir = 0_degrees;
        // amount of last turning (for calculate skidding due to handbrake)
        units::angle last_turn = 0_degrees;
        // goes from ~1 to ~0 while proceeding every turn
        float of_turn = 0.0f; // NOLINT(cata-serialize)
        // leftover from previous turn
        float of_turn_carry = 0.0f;
        units::power extra_drag = 0_W; // NOLINT(cata-serialize)
        // the time point when it was successfully stolen
        std::optional<time_point> theft_time;
        // rotation used for mount precalc values
        // NOLINTNEXTLINE(cata-serialize)
        std::array<units::angle, 2> pivot_rotation = { { 0_degrees, 0_degrees } };

        bounding_box rail_wheel_bounding_box; // NOLINT(cata-serialize)
        point front_left; // NOLINT(cata-serialize)
        point front_right; // NOLINT(cata-serialize)
        towing_data tow_data;
        // points used for rotation of mount precalc values
        std::array<point, 2> pivot_anchor;
        // frame direction
        tileray face;
        // direction we are moving
        tileray move;

    private:
        bool no_refresh = false; // NOLINT(cata-serialize)

        // if true, pivot_cache needs to be recalculated
        mutable bool pivot_dirty = true; // NOLINT(cata-serialize)
        mutable bool mass_dirty = true; // NOLINT(cata-serialize)
        mutable bool mass_center_precalc_dirty = true; // NOLINT(cata-serialize)
        mutable bool mass_center_no_precalc_dirty = true; // NOLINT(cata-serialize)
        // cached values for air, water, and  rolling resistance;
        mutable bool coeff_rolling_dirty = true; // NOLINT(cata-serialize)
        mutable bool coeff_air_dirty = true; // NOLINT(cata-serialize)
        mutable bool coeff_water_dirty = true; // NOLINT(cata-serialize)
        // air uses a two stage dirty check: one dirty bit gets set on part install,
        // removal, or breakage. The other dirty bit only gets set during part_removal_cleanup,
        // and that's the bit that controls recalculation.  The intent is to only recalculate
        // the coeffs once per turn, even if multiple parts are destroyed in a collision
        mutable bool coeff_air_changed = true; // NOLINT(cata-serialize)
        // is at least 2/3 of the vehicle on deep water tiles
        // -- this is the "sink or swim" threshold
        mutable bool in_deep_water = false;
        // is at least 1/2 of the vehicle on water tiles
        // -- fordable for ground vehicles; non-amphibious boats float
        mutable bool in_water = false;
        // is the vehicle currently flying
        mutable bool is_flying = false;
        bool flyable = true;
        int requested_z_change = 0;

    public:
        bool is_on_ramp = false;
        // vehicle being driven by player/npc automatically
        bool is_autodriving = false;
        bool is_following = false;
        bool is_patrolling = false;
        bool all_wheels_on_one_axis = false; // NOLINT(cata-serialize)
        // at least one engine is on, of any type
        bool engine_on = false;
        // vehicle tracking on/off
        bool tracking_on = false;
        // vehicle has no key
        bool is_locked = false;
        // vehicle has alarm on
        bool is_alarm_on = false;
        bool camera_on = false;
        bool autopilot_on = false;
        bool precollision_on = true;
        // skidding mode
        bool skidding = false;
        // has bloody or smoking parts
        bool check_environmental_effects = false; // NOLINT(cata-serialize)
        // "inside" flags are outdated and need refreshing
        bool insides_dirty = true; // NOLINT(cata-serialize)
        // Is the vehicle hanging in the air and expected to fall down in the next turn?
        bool is_falling = false;
        // zone_data positions are outdated and need refreshing
        bool zones_dirty = true; // NOLINT(cata-serialize)

        // current noise of vehicle (engine working, etc.)
        unsigned char vehicle_noise = 0;

        // return vehicle part index and muffle value
        std::pair<int, double> get_exhaust_part() const;

        // destination for exhaust emissions
        tripoint exhaust_dest( int part ) const;

        // Returns debug data to overlay on the screen, a vector of {map tile position
        // relative to vehicle pos, color and text}.
        std::vector<std::tuple<point, int, std::string>> get_debug_overlay_data() const;
};

// For reference what each function is supposed to do, see their implementation in
// @ref DefaultRemovePartHandler. Add compatible code for it into @ref MapgenRemovePartHandler,
// if needed.
class RemovePartHandler
{
    public:
        virtual ~RemovePartHandler() = default;

        virtual void unboard( const tripoint &loc ) = 0;
        virtual void add_item_or_charges( const tripoint &loc, item it, bool permit_oob ) = 0;
        virtual void set_transparency_cache_dirty( int z ) = 0;
        virtual void set_floor_cache_dirty( int z ) = 0;
        virtual void removed( vehicle &veh, int part ) = 0;
        virtual void spawn_animal_from_part( item &base, const tripoint &loc ) = 0;
        virtual map &get_map_ref() = 0;
};

class DefaultRemovePartHandler : public RemovePartHandler
{
    public:
        ~DefaultRemovePartHandler() override = default;

        void unboard( const tripoint &loc ) override {
            get_map().unboard_vehicle( loc );
        }
        void add_item_or_charges( const tripoint &loc, item it, bool /*permit_oob*/ ) override {
            get_map().add_item_or_charges( loc, std::move( it ) );
        }
        void set_transparency_cache_dirty( const int z ) override {
            map &here = get_map();
            here.set_transparency_cache_dirty( z );
            here.set_seen_cache_dirty( tripoint_zero );
        }
        void set_floor_cache_dirty( const int z ) override {
            get_map().set_floor_cache_dirty( z );
        }
        void removed( vehicle &veh, int part ) override;
        void spawn_animal_from_part( item &base, const tripoint &loc ) override {
            base.release_monster( loc, 1 );
        }
        map &get_map_ref() override {
            return get_map();
        }
};

class MapgenRemovePartHandler : public RemovePartHandler
{
    private:
        map &m;

    public:
        explicit MapgenRemovePartHandler( map &m ) : m( m ) { }

        ~MapgenRemovePartHandler() override = default;

        void unboard( const tripoint &/*loc*/ ) override {
            debugmsg( "Tried to unboard during mapgen!" );
            // Ignored. Will almost certainly not be called anyway, because
            // there are no creatures that could have been mounted during mapgen.
        }
        void add_item_or_charges( const tripoint &loc, item it, bool permit_oob ) override;
        void set_transparency_cache_dirty( const int /*z*/ ) override {
            // Ignored for now. We don't initialize the transparency cache in mapgen anyway.
        }
        void set_floor_cache_dirty( const int /*z*/ ) override {
            // Ignored for now. We don't initialize the floor cache in mapgen anyway.
        }
        void removed( vehicle &veh, const int /*part*/ ) override {
            // TODO: check if this is necessary, it probably isn't during mapgen
            m.dirty_vehicle_list.insert( &veh );
        }
        void spawn_animal_from_part( item &/*base*/, const tripoint &/*loc*/ ) override {
            debugmsg( "Tried to spawn animal from vehicle part during mapgen!" );
            // Ignored. The base item will not be changed and will spawn as is:
            // still containing the animal.
            // This should not happend during mapgen anyway.
            // TODO: *if* this actually happens: create a spawn point for the animal instead.
        }
        map &get_map_ref() override {
            return m;
        }
};

#endif // CATA_SRC_VEHICLE_H
