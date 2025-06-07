#pragma once
#ifndef CATA_SRC_ACTIVITY_ACTOR_DEFINITIONS_H
#define CATA_SRC_ACTIVITY_ACTOR_DEFINITIONS_H

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "activity_type.h"
#include "calendar.h"
#include "character.h"
#include "clone_ptr.h"
#include "contents_change_handler.h"
#include "game.h"
#include "handle_liquid.h"
#include "item.h"
#include "itype.h"
#include "item_location.h"
#include "memory_fast.h"
#include "pickup.h"
#include "point.h"
#include "recipe.h"
#include "string_id.h"
#include "type_id.h"
#include "units.h"
#include "units_fwd.h"
#include "vehicle.h"

#include "activity_actor.h"

class Character;
class Creature;
class JsonOut;
class avatar;
class npc;
class SkillLevel;
class player_activity;

struct islot_book;
struct pulp_data;

class aim_activity_actor : public activity_actor
{
    private:
        std::optional<item> fake_weapon;
        std::vector<tripoint_bub_ms> fin_trajectory;

    public:
        std::string action;
        int aif_duration = 0; // Counts aim-and-fire duration
        bool aiming_at_critter = false; // Whether aiming at critter or a tile
        bool should_unload_RAS = false;
        bool snap_to_target = false;
        /* Item location for RAS weapon reload */
        item_location reload_loc;
        bool shifting_view = false;
        tripoint_rel_ms initial_view_offset;
        /** Target UI requested to abort aiming */
        bool aborted = false;
        /** if true abort if no targets are available when re-entering aiming ui after shooting */
        bool abort_if_no_targets = false;
        /**
         * Target UI requested to abort aiming and reload weapon
         * Implies aborted = true
         */
        bool reload_requested = false;
        /**
         * A friendly creature may enter line of fire during aim-and-shoot,
         * and that generates a warning to proceed/abort. If player decides to
         * proceed, that creature is saved in this vector to prevent the same warning
         * from popping up on the following turn(s).
         */
        std::vector<weak_ptr_fast<Creature>> acceptable_losses; // NOLINT(cata-serialize)

        aim_activity_actor();

        /** Aiming wielded gun */
        static aim_activity_actor use_wielded();

        /** Aiming fake gun provided by a bionic */
        static aim_activity_actor use_bionic( const item &fake_gun );

        /** Aiming fake gun provided by a mutation */
        static aim_activity_actor use_mutation( const item &fake_gun );

        const activity_id &get_type() const override {
            static const activity_id ACT_AIM( "ACT_AIM" );
            return ACT_AIM;
        }

        void start( player_activity &act, Character &who ) override;
        bool check_gun_ability_to_shoot( Character &who, item &it );
        void do_turn( player_activity &act, Character &who ) override;
        void finish( player_activity &act, Character &who ) override;
        void canceled( player_activity &act, Character &who ) override;

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<aim_activity_actor>( *this );
        }
        std::string get_progress_message( const player_activity & ) const override {
            return std::string();
        }
        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );

        item_location get_weapon();
        void restore_view() const;
        // Load/unload a RELOAD_AND_SHOOT weapon
        bool load_RAS_weapon();
        void unload_RAS_weapon();
};

class autodrive_activity_actor : public activity_actor
{
    private:
        // The player's vehicle, updated at the very start.
        // Will be updated again if, e.g., the player gets unboarded,
        // to make sure the pointer is still valid; the pointer might be invalid due to summoning despawn.
        vehicle *player_vehicle = nullptr;

        // Update player_vehicle; will set it to nullptr if the player no longer has a vehicle.
        void update_player_vehicle( Character & );

    public:
        autodrive_activity_actor() = default;

        const activity_id &get_type() const override {
            static const activity_id ACT_AUTODRIVE( "ACT_AUTODRIVE" );
            return ACT_AUTODRIVE;
        }

        void start( player_activity &, Character & ) override;
        void do_turn( player_activity &, Character & ) override;
        void canceled( player_activity &, Character & ) override;
        void finish( player_activity &act, Character & ) override;

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<autodrive_activity_actor>( *this );
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );
};

class bash_activity_actor : public activity_actor
{
    private:
        tripoint_bub_ms target;

    public:
        explicit bash_activity_actor( const tripoint_bub_ms &where ) : target( where ) {}

        const activity_id &get_type() const override {
            static const activity_id ACT_BASH( "ACT_BASH" );
            return ACT_BASH;
        };

        void start( player_activity &, Character & ) override {}
        void canceled( player_activity &, Character & ) override {}
        void finish( player_activity &, Character & ) override {}

        void do_turn( player_activity &, Character & ) override;

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<bash_activity_actor>( *this );
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );
};

class gunmod_remove_activity_actor : public activity_actor
{
    private:
        int moves_total;
        item_location gun;
        int gunmod_idx;

    public:
        gunmod_remove_activity_actor(
            int moves_total,
            const item_location &gun,
            int gunmod_idx
        ) : moves_total( moves_total ), gun( gun ), gunmod_idx( gunmod_idx ) {}

        const activity_id &get_type() const override {
            static const activity_id ACT_GUNMOD_REMOVE( "ACT_GUNMOD_REMOVE" );
            return ACT_GUNMOD_REMOVE;
        }

        void start( player_activity &act, Character & ) override;
        void do_turn( player_activity &, Character & ) override {}
        void finish( player_activity &act, Character &who ) override;

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<gunmod_remove_activity_actor>( *this );
        }

        static bool gunmod_unload( Character &who, item &gunmod );
        static void gunmod_remove( Character &who, item &gun, item &mod );

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );
};

class hacksaw_activity_actor : public activity_actor
{
    public:
        explicit hacksaw_activity_actor( const tripoint_bub_ms &target,
                                         const item_location &tool ) : target( target ), tool( tool ) {};
        explicit hacksaw_activity_actor( const tripoint_bub_ms &target, const itype_id &type,
                                         const tripoint_bub_ms &veh_pos ) : target( target ), type( type ), veh_pos( veh_pos ) {};
        const activity_id &get_type() const override {
            static const activity_id ACT_HACKSAW( "ACT_HACKSAW" );
            return ACT_HACKSAW;
        }

        void start( player_activity &act, Character &who ) override;
        void do_turn( player_activity &/*act*/, Character &who ) override;
        void finish( player_activity &act, Character &who ) override;

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<hacksaw_activity_actor>( *this );
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );

        // debugmsg causes a backtrace when fired during cata_test
        bool testing = false;  // NOLINT(cata-serialize)
    private:
        tripoint_bub_ms target;
        item_location tool;
        std::optional<itype_id> type;
        std::optional<tripoint_bub_ms> veh_pos;
        bool can_resume_with_internal( const activity_actor &other,
                                       const Character &/*who*/ ) const override;
};

class hacking_activity_actor : public activity_actor
{
    public:
        explicit hacking_activity_actor( const item_location &tool ): tool( tool ) {};

        const activity_id &get_type() const override {
            static const activity_id ACT_HACKING( "ACT_HACKING" );
            return ACT_HACKING;
        }

        void start( player_activity &act, Character &who ) override;
        void do_turn( player_activity &, Character & ) override {}
        void finish( player_activity &act, Character &who ) override;

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<hacking_activity_actor>( *this );
        }

        std::string get_progress_message( const player_activity & ) const override {
            return std::string();
        }
        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );

    private:
        item_location tool;
};

class bookbinder_copy_activity_actor: public activity_actor
{
    private:
        item_location book_binder;
        recipe_id rec_id;
        int pages = 0;

    public:

        bookbinder_copy_activity_actor() = default;
        bookbinder_copy_activity_actor(
            const item_location &book_binder,
            const recipe_id &rec_id
        ) : book_binder( book_binder ), rec_id( rec_id ) {};

        const activity_id &get_type() const override {
            static const activity_id ACT_BINDER_COPY_RECIPE( "ACT_BINDER_COPY_RECIPE" );
            return ACT_BINDER_COPY_RECIPE;
        }

        bool can_resume_with_internal( const activity_actor &other, const Character & ) const override {
            const bookbinder_copy_activity_actor &act = static_cast<const bookbinder_copy_activity_actor &>
                    ( other );
            return rec_id == act.rec_id && book_binder == act.book_binder;
        }

        static int pages_for_recipe( const recipe &r ) {
            return 1 + r.difficulty / 2;
        }

        void start( player_activity &act, Character & ) override;
        void do_turn( player_activity &, Character &p ) override;
        void finish( player_activity &act, Character &p ) override;

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<bookbinder_copy_activity_actor>( *this );
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );
};

class hotwire_car_activity_actor : public activity_actor
{
    private:
        int moves_total;

        /**
         * Position of first vehicle part; used to identify the vehicle
         * TODO: find something more reliable (to cover cases when vehicle is moved/damaged)
         */
        tripoint_abs_ms target;

        bool can_resume_with_internal( const activity_actor &other, const Character & ) const override {
            const hotwire_car_activity_actor &a = static_cast<const hotwire_car_activity_actor &>( other );
            return target == a.target && moves_total == a.moves_total;
        }

    public:
        hotwire_car_activity_actor( int moves_total,
                                    const tripoint_abs_ms &target ): moves_total( moves_total ),
            target( target ) {}

        const activity_id &get_type() const override {
            static const activity_id ACT_HOTWIRE_CAR( "ACT_HOTWIRE_CAR" );
            return ACT_HOTWIRE_CAR;
        }

        void start( player_activity &act, Character & ) override;
        void do_turn( player_activity &, Character & ) override;
        void finish( player_activity &act, Character &who ) override;

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<hotwire_car_activity_actor>( *this );
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );
};

class glide_activity_actor : public activity_actor
{
    private:
        int jump_direction;
        int glide_distance;
        int moved_tiles = 0;
        int moves_total = to_moves<int>( 1_seconds );
        explicit glide_activity_actor() = default;

    public:
        explicit glide_activity_actor( Character *you, int jump_direction, int glide_distance );

        const activity_id &get_type() const override {
            static const activity_id ACT_GLIDE( "ACT_GLIDE" );
            return ACT_GLIDE;
        }

        void start( player_activity &act, Character & ) override;
        void do_turn( player_activity &act, Character &you ) override;
        void finish( player_activity &act, Character &you ) override;

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<glide_activity_actor>( *this );
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );
};

class bikerack_racking_activity_actor : public activity_actor
{
    private:
        int moves_total = to_moves<int>( 5_minutes );
        tripoint_bub_ms parent_vehicle_pos;
        tripoint_bub_ms racked_vehicle_pos;
        std::vector<int> racks;

        explicit bikerack_racking_activity_actor() = default;
    public:
        explicit bikerack_racking_activity_actor( const vehicle &parent_vehicle,
                const vehicle &racked_vehicle, const std::vector<int> &racks );

        const activity_id &get_type() const override {
            static const activity_id ACT_BIKERACK_RACKING( "ACT_BIKERACK_RACKING" );
            return ACT_BIKERACK_RACKING;
        }

        void start( player_activity &act, Character & ) override;
        void do_turn( player_activity &, Character & ) override {}
        void finish( player_activity &act, Character & ) override;

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<bikerack_racking_activity_actor>( *this );
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );
};

class bikerack_unracking_activity_actor : public activity_actor
{
    private:
        int moves_total = to_moves<int>( 5_minutes );
        tripoint_bub_ms parent_vehicle_pos;
        std::vector<int> parts;
        std::vector<int> racks;

        explicit bikerack_unracking_activity_actor() = default;
    public:
        explicit bikerack_unracking_activity_actor( const vehicle &parent_vehicle,
                const std::vector<int> &parts, const std::vector<int> &racks );

        const activity_id &get_type() const override {
            static const activity_id ACT_BIKERACK_UNRACKING( "ACT_BIKERACK_UNRACKING" );
            return ACT_BIKERACK_UNRACKING;
        }

        void start( player_activity &act, Character & ) override;
        void do_turn( player_activity &, Character & ) override {}
        void finish( player_activity &act, Character & ) override;

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<bikerack_unracking_activity_actor>( *this );
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );
};

class read_activity_actor : public activity_actor
{
    public:
        enum class book_type : int { normal, martial_art };

        read_activity_actor() = default;

        explicit read_activity_actor(
            time_duration read_time, item_location &book, item_location &ereader,
            bool continuous = false, int learner_id = -1 )
            : moves_total( to_moves<int>( read_time ) ), book( book ), ereader( ereader ),
              continuous( continuous ), learner_id( learner_id ) {};

        const activity_id &get_type() const override {
            static const activity_id ACT_READ( "ACT_READ" );
            return ACT_READ;
        }

        static void read_book( Character &learner, const cata::value_ptr<islot_book> &islotbook,
                               SkillLevel &skill_level, double penalty );

        void start( player_activity &act, Character &who ) override;
        void do_turn( player_activity &act, Character &who ) override;
        void finish( player_activity &act, Character &who ) override;

        std::string get_progress_message( const player_activity & ) const override;

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<read_activity_actor>( *this );
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );

    private:
        int moves_total;
        item_location book;
        std::optional<book_type> bktype; // NOLINT(cata-serialize)

        // Using an electronic book reader
        item_location ereader;
        bool using_ereader = false;

        // Read until the learner with this ID gets a level
        bool continuous = false;
        int learner_id = -1;

        // Will return true if activity must be set to null
        bool player_read( avatar &you );
        bool player_readma( avatar &you );  // Martial arts book
        bool npc_read( npc &learner );

        bool can_resume_with_internal( const activity_actor &other,
                                       const Character & ) const override;
};

class move_items_activity_actor : public activity_actor
{
    private:
        std::vector<item_location> target_items;
        std::vector<int> quantities;
        bool to_vehicle;
        tripoint_rel_ms relative_destination;
        bool hauling_mode;

    public:
        move_items_activity_actor( std::vector<item_location> target_items, std::vector<int> quantities,
                                   bool to_vehicle, tripoint_rel_ms relative_destination, bool hauling_mode = false ) :
            target_items( std::move( target_items ) ), quantities( std::move( quantities ) ),
            to_vehicle( to_vehicle ),
            relative_destination( relative_destination ),
            hauling_mode( hauling_mode ) {}

        const activity_id &get_type() const override {
            static const activity_id ACT_MOVE_ITEMS( "ACT_MOVE_ITEMS" );
            return ACT_MOVE_ITEMS;
        }

        void start( player_activity &, Character & ) override {}
        void do_turn( player_activity &act, Character &who ) override;
        void finish( player_activity &, Character & ) override {}

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<move_items_activity_actor>( *this );
        }

        std::string get_progress_message( const player_activity & ) const override {
            return std::string();
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );
};

class pickup_activity_actor : public activity_actor
{
    private:
        /** Target items and the quantities thereof */
        std::vector<item_location> target_items;
        std::vector<int> quantities;
        Pickup::pick_info info;

        /**
         * Position of the character when the activity is started. This is
         * stored so that we can cancel the activity if the player moves
         * (e.g. if the player is in a moving vehicle). This should be null
         * if not grabbing from the ground.
         */
        std::optional<tripoint_bub_ms> starting_pos;

    public:
        pickup_activity_actor( const std::vector<item_location> &target_items,
                               const std::vector<int> &quantities,
                               const std::optional<tripoint_bub_ms> &starting_pos,
                               bool autopickup ) : target_items( target_items ),
            quantities( quantities ), starting_pos( starting_pos ), stash_successful( true ),
            autopickup( autopickup ) {}

        /**
          * Used to check at the end of a pickup activity if the character was able
          * to stash everything. If not, a message is displayed to clarify.
          */
        bool stash_successful;
        bool autopickup;

        const activity_id &get_type() const override {
            static const activity_id ACT_PICKUP( "ACT_PICKUP" );
            return ACT_PICKUP;
        }

        void start( player_activity &, Character & ) override {}
        void do_turn( player_activity &act, Character &who ) override;
        void finish( player_activity &, Character & ) override {}

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<pickup_activity_actor>( *this );
        }

        std::string get_progress_message( const player_activity & ) const override {
            return std::string();
        }

        bool do_drop_invalid_inventory() const override {
            return false;
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );
};

class boltcutting_activity_actor : public activity_actor
{
    public:
        explicit boltcutting_activity_actor( const tripoint_bub_ms &target,
                                             const item_location &tool ) : target( target ), tool( tool ) {};

        const activity_id &get_type() const override {
            static const activity_id ACT_BOLTCUTTING( "ACT_BOLTCUTTING" );
            return ACT_BOLTCUTTING;
        }

        void start( player_activity &act, Character &/*who*/ ) override;
        void do_turn( player_activity &/*act*/, Character &who ) override;
        void finish( player_activity &act, Character &who ) override;

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<boltcutting_activity_actor>( *this );
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );

        // debugmsg causes a backtrace when fired during cata_test
        bool testing = false; // NOLINT(cata-serialize)

    private:
        tripoint_bub_ms target;
        item_location tool;

        bool can_resume_with_internal( const activity_actor &other,
                                       const Character &/*who*/ ) const override {
            const boltcutting_activity_actor &actor = static_cast<const boltcutting_activity_actor &>
                    ( other );
            return actor.target == target && actor.tool == tool;
        }
};

class lockpick_activity_actor : public activity_actor
{
    private:
        int moves_total;
        std::optional<item_location> lockpick;
        std::optional<item> fake_lockpick;
        tripoint_abs_ms target;

        lockpick_activity_actor(
            int moves_total,
            const std::optional<item_location> &lockpick,
            const std::optional<item> &fake_lockpick,
            const tripoint_abs_ms &target
        ) : moves_total( moves_total ), lockpick( lockpick ), fake_lockpick( fake_lockpick ),
            target( target ) {}

    public:
        /** Use regular lockpick. */
        static lockpick_activity_actor use_item(
            int moves_total,
            const item_location &lockpick,
            const tripoint_abs_ms &target
        );

        /** Use bionic lockpick. */
        static lockpick_activity_actor use_bionic(
            const tripoint_abs_ms &target
        );

        const activity_id &get_type() const override {
            static const activity_id ACT_LOCKPICK( "ACT_LOCKPICK" );
            return ACT_LOCKPICK;
        }

        void start( player_activity &act, Character & ) override;
        void do_turn( player_activity &, Character & ) override {}
        void finish( player_activity &act, Character &who ) override;

        static std::optional<tripoint_bub_ms> select_location( avatar &you );

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<lockpick_activity_actor>( *this );
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );
};

class ebooksave_activity_actor : public activity_actor
{
    public:
        explicit ebooksave_activity_actor( const std::vector<item_location> &books,
                                           const item_location &ereader ) :
            books( books ), ereader( ereader ) {};

        const activity_id &get_type() const override {
            static const activity_id ACT_EBOOKSAVE( "ACT_EBOOKSAVE" );
            return ACT_EBOOKSAVE;
        }

        static int total_pages( const std::vector<item_location> &books );
        static time_duration required_time( const std::vector<item_location> &books );
        static int required_charges( const std::vector<item_location> &books,
                                     const item_location &ereader );

        void start( player_activity &act, Character &/*who*/ ) override;
        void do_turn( player_activity &act, Character &who ) override;
        void finish( player_activity &act, Character &who ) override;
        void canceled( player_activity &act, Character &who ) override;

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<ebooksave_activity_actor>( *this );
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );

    private:
        /**
         * All remaining books left to scan.
         * After a book is fully scanned to ereader, this list is modified so that the scanned book is removed from here.
        */
        std::vector<item_location> books;
        item_location ereader;
        /** How many books this activity has scanned to ereader so far. */
        int handled_books = 0;
        /** How much time is left on this book until it is fully scanned and we can move on to the next book. */
        int turns_left_on_current_book = 0;

        static constexpr time_duration time_per_page = 5_seconds;
        // Every 120 pages requires one charge of the ereader (equivalent of 1500 mWh, so roughly 6 charges per hour )
        static constexpr int pages_per_charge = 120;

        void start_scanning_next_book( player_activity &act );
        void completed_scanning_current_book( player_activity &act, Character &who );
};

/**
Handles any e-file processing on an e-device
*/

enum efile_action : int {
    EF_BROWSE,
    EF_READ,
    EF_MOVE_FROM_THIS,
    EF_MOVE_ONTO_THIS,
    EF_COPY_FROM_THIS,
    EF_COPY_ONTO_THIS,
    EF_WIPE,
    EF_INVALID,
    EF_ACTION_COUNT
};
template<>
struct enum_traits<efile_action> {
    static constexpr efile_action last = efile_action::EF_ACTION_COUNT;
};
enum efile_combo : int {
    COMBO_MOVE_ONTO_BROWSE,
    COMBO_NONE,
    EFILE_COMBO_COUNT
};
template<>
struct enum_traits<efile_combo> {
    static constexpr efile_combo last = efile_combo::EFILE_COMBO_COUNT;
};

namespace io
{
template<>
std::string enum_to_string<efile_action>( efile_action data );

template<>
std::string enum_to_string<efile_combo>( efile_combo data );
} // namespace io

/**
Holds an e-file transfer's devices
*/
struct efile_transfer {
    //the e-device initiating the action -- always usable
    const item_location &used_edevice;
    //the e-device receiving the action -- optionally usable
    const item_location &target_edevice;

    efile_transfer( const item_location &used_edevice, const item_location &target_edevice ) :
        used_edevice( used_edevice ),
        target_edevice( target_edevice ) {};
};

/**
* This activity handles electronic file browsing/transferring for electronic devices.
* On a per-turn basis:
* e-devices are "booted" until success or failure.
* for each e-device, e-files are processed until success or failure.
* See related functions: iuse::efiledevice, game_menus::inv::edevice_select/efile_select
*/
class efile_activity_actor : public activity_actor
{
    public:
        explicit efile_activity_actor() = default;
        /**
        * @param action_type - which sub-activity to do
        * @param combo_type - which sub-activity to do immediately after action_type
        * @param used_edevice - the edevice used from iuse::efiledevice
        * @param target_edevices - all e-devices to perform e-file operation with
        * @param selected_efiles - if provided, action only uses the given files
        */
        explicit efile_activity_actor(
            const item_location &used_edevice,
            const std::vector<item_location> &target_edevices,
            const std::vector<item_location> &selected_efiles,
            efile_action action_type,
            efile_combo combo_type
        ) : used_edevice( used_edevice ),
            target_edevices( target_edevices ), selected_efiles( selected_efiles ),
            action_type( action_type ), combo_type( combo_type ) {};

        const activity_id &get_type() const override {
            static const activity_id ACT_E_FILE( "ACT_E_FILE" );
            return ACT_E_FILE;
        }

        void start( player_activity &act, Character &who ) override;
        void do_turn( player_activity &act, Character &who ) override;
        void finish( player_activity &act, Character &who ) override;
        void canceled( player_activity &act, Character &who ) override;

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<efile_activity_actor>( *this );
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );

        //TODO: base all of these constants on actual statistics?
        /** Abstract flat time to look at a file's filename and extension,
        determine whether it's junk or not, and open it to verify its contents */
        static constexpr time_duration time_per_browsed_efile = 15_seconds;
        /** One charge consumed per 4 minutes*/
        static constexpr time_duration charge_per_transfer_time = 4_minutes;
        /** One charge consumed per 32 files (8 minutes) */
        static constexpr time_duration charge_per_browse_time = time_per_browsed_efile * 32;
        /** Time required to boot an e-device */
        static constexpr time_duration device_bootup_time = 10_seconds;
        /** Generic local file transfer speed */
        static constexpr units::ememory local_etransfer_rate = 200_MB;

        /** Converts `efile_action` enum to human-readable string
        * @param extended - for move/copy actions, e.g. "move files onto" */
        static std::string efile_action_name( efile_action action_type, bool past_tense = false,
                                              bool extended = false );
        /** Returns if the action automatically excludes the used e-device with its target e-devices */
        static bool efile_action_exclude_used( efile_action action_type );
        /** Returns if the action is moving files from an e-device */
        static bool efile_action_is_from( efile_action action_type );
        /** Returns if we can skip transfering this file because a copy of it already exists */
        static bool efile_skip_copy( const efile_transfer &transfer, const item &efile );
        /** Returns time needed to process the given file with the given transfer */
        static time_duration efile_processing_time( const item_location &efile,
                const efile_transfer &transfer,
                efile_action action_type, Character &who, bool computer_low_skill );
        /** Returns time needed to process all given e-devices with the given transfer */
        static time_duration total_processing_time( const item_location &used_edevice,
                const std::vector<item_location> &currently_processed_edevices,
                const std::vector<item_location> &selected_efiles,
                efile_action action_type, Character &who, bool computer_low_skill );
        /** Returns the effective electronic transfer rate with `external_transfer_rate` factored in */
        static units::ememory current_etransfer_rate( Character &who, const efile_transfer &transfer,
                const item_location &efile );
        /** Returns all e-files on this device that are in the filter_files list,
        * and removes matching efiles from filter_files list
        * @param filter_files list of e-files to use, usually selected_files */
        static std::vector<item_location> filter_edevice_efiles( const item_location &edevice,
                std::vector<item_location> &filter_files );
        /** Returns the most optimal estorage (if needed) for improving transfer speed given the two edevices provided */
        static item_location find_external_transfer_estorage( Character &p,
                const item_location &efile, const item_location &ed1, const item_location &ed2 );
        /** Returns whether the given edevice can process files (expand later if needed) */
        static bool edevice_has_use( const item *edevice );
        enum edevice_compatible {
            ECOMPAT_NONE,
            ECOMPAT_SLOW,
            ECOMPAT_FAST
        };
        /** Returns the given edevices' compatibility type based on their itypes and e_port_banned */
        static edevice_compatible edevices_compatible( const item_location &ed1, const item_location &ed2 );
        static edevice_compatible edevices_compatible( const item &ed1, const item &ed2 );
    private:
        item_location used_edevice;
        std::vector<item_location> target_edevices;
        /** Held for combo activity */
        std::vector<item_location> target_edevices_copy;
        /** e-files that have yet to be processed, across all devices */
        std::vector<item_location> selected_efiles;
        efile_action action_type = EF_INVALID;
        efile_combo combo_type = COMBO_NONE;
        /** e-files currently processed on current e-device */
        std::vector<item_location> currently_processed_efiles;
        /** How many e-devices this activity started with. */
        int target_edevices_count = 0;
        /** How many e-devices this activity has successfully processed so far. */
        int processed_edevices = 0;
        /** How many e-devices this activity has failed to process so far. */
        int failed_edevices = 0;
        /** How many e-files this activity has successfully processed so far. */
        int processed_efiles = 0;
        /** How many e-files this activity has failed to process so far. */
        int failed_efiles = 0;
        /** Have we started processing e-devices? */
        bool started_processing = false;
        /** Have we finished processing e-devices? */
        bool done_processing = false;
        /** Have we finished booting the current e-device? */
        bool next_edevice_booted = false;
        /** actor computer skill < 1 */
        bool computer_low_skill = false;
        /** How many turns left on this e-device until it is fully proccessed.
        If empty, no e-device is currently being processed */
        std::optional<int> turns_left_on_current_edevice;
        /** How many turns left on this file until it is fully proccessed. */
        int turns_left_on_current_efile = 0;

        void start_processing_next_edevice();
        void start_processing_next_efile( player_activity &/*act*/, Character &who );
        void failed_processing_current_edevice();
        void completed_processing_current_edevice();
        void failed_processing_current_efile( player_activity &act, Character &who );
        /** Action upon completing processing an e-file (depends on action_type) */
        void completed_processing_current_efile( player_activity &/*act*/, Character &who );

        item_location &get_currently_processed_edevice();
        item_location &get_currently_processed_efile();
        bool processed_edevices_remain() const;
        bool processed_efiles_remain() const;

        /** Segway to the next player activity for a combo type */
        void combo_next_activity( Character &who );
        time_duration charge_time( efile_action action_type );
};

class migration_cancel_activity_actor : public activity_actor
{
    public:
        migration_cancel_activity_actor() = default;

        const activity_id &get_type() const override {
            static const activity_id ACT_MIGRATION_CANCEL( "ACT_MIGRATION_CANCEL" );
            return ACT_MIGRATION_CANCEL;
        }

        void start( player_activity &, Character & ) override {}
        void do_turn( player_activity &act, Character &who ) override;
        void finish( player_activity &, Character & ) override {}

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<migration_cancel_activity_actor>( *this );
        }

        std::string get_progress_message( const player_activity & ) const override {
            return std::string();
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );
};

class open_gate_activity_actor : public activity_actor
{
    private:
        int moves_total;
        tripoint_bub_ms placement;

        /**
         * @pre @p other is a open_gate_activity_actor
         */
        bool can_resume_with_internal( const activity_actor &other, const Character & ) const override {
            const open_gate_activity_actor &og_actor = static_cast<const open_gate_activity_actor &>( other );
            return placement == og_actor.placement;
        }

    public:
        open_gate_activity_actor( int gate_moves, const tripoint_bub_ms &gate_placement ) :
            moves_total( gate_moves ), placement( gate_placement ) {}

        const activity_id &get_type() const override {
            static const activity_id ACT_OPEN_GATE( "ACT_OPEN_GATE" );
            return ACT_OPEN_GATE;
        }

        void start( player_activity &act, Character & ) override;
        void do_turn( player_activity &, Character & ) override {}
        void finish( player_activity &act, Character & ) override;

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<open_gate_activity_actor>( *this );
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );
};

class consume_activity_actor : public activity_actor
{
    private:
        item_location consume_location;
        item consume_item;
        std::vector<int> consume_menu_selections;
        std::vector<item_location> consume_menu_selected_items;
        std::string consume_menu_filter;
        bool canceled = false;
        activity_id type;
        /**
         * @pre @p other is a consume_activity_actor
         */
        bool can_resume_with_internal( const activity_actor &other, const Character & ) const override {
            const consume_activity_actor &c_actor = static_cast<const consume_activity_actor &>( other );
            return consume_location == c_actor.consume_location &&
                   canceled == c_actor.canceled && &consume_item == &c_actor.consume_item;
        }
    public:
        consume_activity_actor( const item_location &consume_location,
                                std::vector<int> consume_menu_selections,
                                const std::vector<item_location> &consume_menu_selected_items,
                                const std::string &consume_menu_filter, activity_id type ) :
            consume_location( consume_location ),
            consume_menu_selections( std::move( consume_menu_selections ) ),
            consume_menu_selected_items( consume_menu_selected_items ),
            consume_menu_filter( consume_menu_filter ),
            type( type ) {}

        explicit consume_activity_actor( const item_location &consume_location ) :
            consume_location( consume_location ) {}

        explicit consume_activity_actor( const item &consume_item ) :
            consume_item( consume_item ) {}

        const activity_id &get_type() const override {
            static const activity_id ACT_CONSUME( "ACT_CONSUME" );
            return ACT_CONSUME;
        }

        void start( player_activity &act, Character &guy ) override;
        void do_turn( player_activity &, Character & ) override {}
        void finish( player_activity &act, Character & ) override;

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<consume_activity_actor>( *this );
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );
};

class try_sleep_activity_actor : public activity_actor
{
    private:
        bool disable_query = false;
        time_duration duration;

    public:
        /*
         * @param dur Total duration, from when the character starts
         * trying to fall asleep toexplicit explicit  when they're supposed to wake up
         */
        explicit try_sleep_activity_actor( const time_duration &dur ) : duration( dur ) {}

        const activity_id &get_type() const override {
            static const activity_id ACT_TRY_SLEEP( "ACT_TRY_SLEEP" );
            return ACT_TRY_SLEEP;
        }

        void start( player_activity &act, Character &who ) override;
        void do_turn( player_activity &act, Character &who ) override;
        void finish( player_activity &act, Character &who ) override;
        void canceled( player_activity &, Character &who ) override;

        void query_keep_trying( player_activity &act, Character &who );

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<try_sleep_activity_actor>( *this );
        }

        std::string get_progress_message( const player_activity & ) const override {
            return std::string();
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );
};

class safecracking_activity_actor : public activity_actor
{
    public:
        explicit safecracking_activity_actor( const tripoint_bub_ms &safe ) : safe( safe ) {};

        const activity_id &get_type() const override {
            static const activity_id ACT_CRACKING( "ACT_CRACKING" );
            return ACT_CRACKING;
        }

        static time_duration safecracking_time( const Character &who );

        void start( player_activity &act, Character &who ) override;
        void do_turn( player_activity &act, Character &who ) override;
        void finish( player_activity &act, Character &who ) override;

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<safecracking_activity_actor>( *this );
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );

    private:
        tripoint_bub_ms safe;
        int exp_step = 0;

        bool can_resume_with_internal( const activity_actor &other,
                                       const Character &/*who*/ ) const override {
            const safecracking_activity_actor &actor = static_cast<const safecracking_activity_actor &>
                    ( other );
            return actor.safe == safe;
        }
};

class unload_activity_actor : public activity_actor
{
    private:
        int moves_total;
        item_location target;
    public:
        unload_activity_actor( int moves_total, const item_location &target ) :
            moves_total( moves_total ), target( target ) {}
        const activity_id &get_type() const override {
            static const activity_id ACT_UNLOAD( "ACT_UNLOAD" );
            return ACT_UNLOAD;
        }

        bool can_resume_with_internal( const activity_actor &other, const Character & ) const override {
            const unload_activity_actor &act = static_cast<const unload_activity_actor &>( other );
            return target == act.target;
        }

        void start( player_activity &act, Character & ) override;
        void do_turn( player_activity &, Character & ) override {}
        void finish( player_activity &act, Character &who ) override;

        /** Unloads the magazine instantly. Can be called without an activity. May destroy the item. */
        static void unload( Character &who, item_location &target );

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<unload_activity_actor>( *this );
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );
};

class craft_activity_actor : public activity_actor
{
    private:
        craft_activity_actor() = default;

        item_location craft_item;
        bool is_long;

        float activity_override = NO_EXERCISE;
        std::optional<requirement_data> cached_continuation_requirements; // NOLINT(cata-serialize)
        float cached_crafting_speed; // NOLINT(cata-serialize)
        int cached_assistants; // NOLINT(cata-serialize)
        double cached_base_total_moves; // NOLINT(cata-serialize)
        double cached_cur_total_moves; // NOLINT(cata-serialize)
        float cached_workbench_multiplier; // NOLINT(cata-serialize)
        bool use_cached_workbench_multiplier; // NOLINT(cata-serialize)
        bool check_if_craft_okay( item_location &craft_item, Character &crafter );
    public:
        craft_activity_actor( item_location &it, bool is_long );

        const activity_id &get_type() const override {
            static const activity_id ACT_CRAFT( "ACT_CRAFT" );
            return ACT_CRAFT;
        }

        void start( player_activity &act, Character &crafter ) override;
        void do_turn( player_activity &act, Character &crafter ) override;
        void finish( player_activity &act, Character &crafter ) override;
        void canceled( player_activity &/*act*/, Character &/*who*/ ) override;

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<craft_activity_actor>( *this );
        }

        std::string get_progress_message( const player_activity & ) const override;

        float exertion_level() const override;

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );
};

class workout_activity_actor : public activity_actor
{
    private:
        bool disable_query = false; // disables query, continue as long as possible
        bool rest_mode = false; // work or rest during training session
        time_duration duration;
        tripoint_bub_ms location;
        time_point stop_time; // can resume if time apart is not above
        activity_id act_id = activity_id( "ACT_WORKOUT_LIGHT" ); // variable activities
        int intensity_modifier = 1;
        int elapsed = 0;

    public:
        explicit workout_activity_actor( const tripoint_bub_ms &loc ) : location( loc ) {}

        // can assume different sub-activities
        const activity_id &get_type() const override {
            return act_id;
        }

        bool can_resume_with_internal( const activity_actor &other, const Character & ) const override {
            const workout_activity_actor &w_actor = static_cast<const workout_activity_actor &>( other );
            return ( location == w_actor.location && calendar::turn - stop_time <= 10_minutes );
        }

        void start( player_activity &act, Character &who ) override;
        void do_turn( player_activity &act, Character &who ) override;
        void finish( player_activity &act, Character &who ) override;
        void canceled( player_activity &/*act*/, Character &/*who*/ ) override;

        bool query_keep_training( player_activity &act, Character &who );

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<workout_activity_actor>( *this );
        }

        std::string get_progress_message( const player_activity & ) const override {
            return std::string();
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );
};

class drop_or_stash_item_info
{
    public:
        drop_or_stash_item_info()
            : _loc( item_location::nowhere ) {}
        drop_or_stash_item_info( const item_location &_loc, const int _count )
            : _loc( _loc ), _count( _count ) {}

        void serialize( JsonOut &jsout ) const;
        void deserialize( const JsonObject &jsobj );

        const item_location &loc() const {
            return _loc;
        }

        int count() const {
            return _count;
        }
    private:
        item_location _loc;
        int _count = 0;
};

/**
 * Activity to drop items to the ground or into a vehicle cargo part.
 * @items is the list of items to drop
 * @placement is the offset to the current position of the actor (use tripoint::zero for current pos)
 * @force_ground should the items be forced to the ground instead of e.g. a container at the position
 */
class drop_activity_actor : public activity_actor
{
    public:
        drop_activity_actor() = default;
        drop_activity_actor( const std::vector<drop_or_stash_item_info> &items,
                             const tripoint_rel_ms &placement, const bool force_ground )
            : items( items ), placement( placement ), force_ground( force_ground ) {}

        const activity_id &get_type() const override {
            static const activity_id ACT_DROP( "ACT_DROP" );
            return ACT_DROP;
        }

        void start( player_activity &/*act*/, Character &/*who*/ ) override {}
        void do_turn( player_activity &act, Character &who ) override;
        void finish( player_activity &/*act*/, Character &/*who*/ ) override {}
        void canceled( player_activity &act, Character &who ) override;

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<drop_activity_actor>( *this );
        }

        std::string get_progress_message( const player_activity & ) const override {
            return std::string();
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );

    private:
        std::vector<drop_or_stash_item_info> items;
        contents_change_handler handler;
        tripoint_rel_ms placement;
        bool force_ground = false;
        bool current_bulk_unload = false;
};

class stash_activity_actor: public activity_actor
{
    public:
        stash_activity_actor() = default;
        stash_activity_actor( const std::vector<drop_or_stash_item_info> &items,
                              const tripoint_rel_ms &placement )
            : items( items ), placement( placement ) {}

        const activity_id &get_type() const override {
            static const activity_id ACT_STASH( "ACT_STASH" );
            return ACT_STASH;
        }

        void start( player_activity &/*act*/, Character &/*who*/ ) override {}
        void do_turn( player_activity &act, Character &who ) override;
        void finish( player_activity &/*act*/, Character &/*who*/ ) override {}
        void canceled( player_activity &act, Character &who ) override;

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<stash_activity_actor>( *this );
        }

        std::string get_progress_message( const player_activity & ) const override {
            return std::string();
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );

    private:
        std::vector<drop_or_stash_item_info> items;
        contents_change_handler handler;
        tripoint_rel_ms placement;
        bool current_bulk_unload = false;
};

class harvest_activity_actor : public activity_actor
{
    public:
        explicit harvest_activity_actor( const tripoint_bub_ms &target,
                                         bool auto_forage = false ) :
            target( target ), auto_forage( auto_forage ) {};

        const activity_id &get_type() const override {
            static const activity_id ACT_HARVEST( "ACT_HARVEST" );
            return ACT_HARVEST;
        }

        void start( player_activity &/*act*/, Character &/*who*/ ) override;
        void do_turn( player_activity &/*act*/, Character &/*who*/ ) override {};
        void finish( player_activity &act, Character &who ) override;

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<harvest_activity_actor>( *this );
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );

    private:
        tripoint_bub_ms target;
        bool exam_furn = false;
        bool nectar = false;
        bool auto_forage = false;

        bool can_resume_with_internal( const activity_actor &other,
                                       const Character &/*who*/ ) const override {
            const harvest_activity_actor &actor = static_cast<const harvest_activity_actor &>
                                                  ( other );
            return target == actor.target && auto_forage == actor.auto_forage &&
                   exam_furn == actor.exam_furn && nectar == actor.nectar;
        }
};

class reload_activity_actor : public activity_actor
{
    public:
        explicit reload_activity_actor( item::reload_option &&opt, int extra_moves = 0 );

        const activity_id &get_type() const override {
            static const activity_id ACT_RELOAD( "ACT_RELOAD" );
            return ACT_RELOAD;
        }

        void start( player_activity &/*act*/, Character &/*who*/ ) override;
        void do_turn( player_activity &/*act*/, Character &/*who*/ ) override {}
        void finish( player_activity &act, Character &who ) override;
        void canceled( player_activity &act, Character &/*who*/ ) override;

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<reload_activity_actor>( *this );
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );

    private:
        explicit reload_activity_actor() = default;
        int moves_total = 0;
        int quantity = 0;
        item_location target_loc;
        item_location ammo_loc;

        bool can_reload() const;
        static void make_reload_sound( Character &who, item &reloadable );

};

class milk_activity_actor : public activity_actor
{
    public:
        milk_activity_actor() = default;
        milk_activity_actor( int moves, int moves_per_unit, tripoint_abs_ms coords,
                             liquid_dest_opt &target, bool milking_tie = true ) : total_moves( moves ),
            moves_per_unit( moves_per_unit ), monster_coords( coords ), target( target ),
            milking_tie( milking_tie ) {}

        const activity_id &get_type() const override {
            static const activity_id ACT_MILK( "ACT_MILK" );
            return ACT_MILK;
        }

        void start( player_activity &act, Character &/*who*/ ) override;
        void do_turn( player_activity &/*act*/, Character &/*who*/ ) override;
        void finish( player_activity &act, Character &who ) override;
        void canceled( player_activity &/*act*/, Character &/*who*/ ) override {}

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<milk_activity_actor>( *this );
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );

    private:
        int total_moves {};
        int moves_per_unit;
        tripoint_abs_ms monster_coords;
        liquid_dest_opt target;
        bool milking_tie; // was the monster tied due to milking.
        time_point next_unit_move;
};

class shearing_activity_actor : public activity_actor
{
    private:
        tripoint_bub_ms mon_coords;    // monster is tied for the duration
        bool shearing_tie;      // was the monster tied due to shearing

    public:
        explicit shearing_activity_actor(
            const tripoint_bub_ms &mon_coords, bool shearing_tie = true )
            : mon_coords( mon_coords ), shearing_tie( shearing_tie ) {};

        const activity_id &get_type() const override {
            static const activity_id ACT_SHEARING( "ACT_SHEARING" );
            return ACT_SHEARING;
        }

        void start( player_activity &act, Character &who ) override;
        void do_turn( player_activity &/*act*/, Character &who ) override;
        void finish( player_activity &act, Character &who ) override;
        void canceled( player_activity &/*act*/, Character &/*who*/ ) override;

        bool can_resume_with_internal( const activity_actor &other,
                                       const Character &/*who*/ ) const override {
            const shearing_activity_actor &actor = static_cast<const shearing_activity_actor &>
                                                   ( other );
            return actor.mon_coords == mon_coords;
        }

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<shearing_activity_actor>( *this );
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );
};

class disassemble_activity_actor : public activity_actor
{
    private:
        int moves_total;
        float activity_override = NO_EXERCISE; // NOLINT(cata-serialize)
        float cached_workbench_multiplier; // NOLINT(cata-serialize)
        bool use_cached_workbench_multiplier = false; // NOLINT(cata-serialize)
    public:
        item_location target;

        explicit disassemble_activity_actor( int moves_total ) :
            moves_total( moves_total ) {}
        const activity_id &get_type() const override {
            static const activity_id ACT_DISASSEMBLE( "ACT_DISASSEMBLE" );
            return ACT_DISASSEMBLE;
        }

        void start( player_activity &act, Character & ) override;
        void do_turn( player_activity &, Character & ) override;
        void finish( player_activity &act, Character &who ) override;
        void canceled( player_activity &act, Character &who ) override;

        float exertion_level() const override;

        std::string get_progress_message( const player_activity & ) const override;

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<disassemble_activity_actor>( *this );
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );
};

class move_furniture_activity_actor : public activity_actor
{
    private:
        tripoint_rel_ms dp;
        bool via_ramp;

    public:
        move_furniture_activity_actor( const tripoint_rel_ms &dp, bool via_ramp ) :
            dp( dp ), via_ramp( via_ramp ) {}
        const activity_id &get_type() const override {
            static const activity_id ACT_FURNITURE_MOVE( "ACT_FURNITURE_MOVE" );
            return ACT_FURNITURE_MOVE;
        }

        void start( player_activity &act, Character & ) override;
        void do_turn( player_activity &, Character & ) override {}
        void finish( player_activity &act, Character &who ) override;
        void canceled( player_activity &act, Character &who ) override;

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<move_furniture_activity_actor>( *this );
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );
};

class insert_item_activity_actor : public activity_actor
{
    private:
        item_location holster;
        drop_locations items;
        contents_change_handler handler;
        bool all_pockets_rigid;
        bool reopen_menu;
        // allow put charge items into holster's nested  pocket
        bool allow_fill_count_by_charge_item_nested;

    public:

        insert_item_activity_actor() = default;
        insert_item_activity_actor( const item_location &holster, const drop_locations &holstered_list,
                                    bool reopen_menu = false, bool allow_fill_count_by_charge_item_nested = true ) : holster( holster ),
            items( holstered_list ),
            reopen_menu( reopen_menu ), allow_fill_count_by_charge_item_nested(
                allow_fill_count_by_charge_item_nested ) {}

        const activity_id &get_type() const override {
            static const activity_id ACT_INSERT_ITEM( "ACT_INSERT_ITEM" );
            return ACT_INSERT_ITEM;
        }

        void start( player_activity &act, Character &who ) override;
        void do_turn( player_activity &/*act*/, Character &/*who*/ ) override {};
        void finish( player_activity &act, Character &who ) override;
        void canceled( player_activity &/*act*/, Character &who ) override;

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<insert_item_activity_actor>( *this );
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );
};

class tent_placement_activity_actor : public activity_actor
{
    private:
        int moves_total;
        tripoint_rel_ms target;
        int radius = 1;
        item it;
        string_id<furn_t> wall;
        string_id<furn_t> floor;
        std::optional<string_id<furn_t>> floor_center;
        string_id<furn_t> door_closed;

    public:
        tent_placement_activity_actor( int moves_total, tripoint_rel_ms target, int radius, const item &it,
                                       string_id<furn_t> wall, string_id<furn_t> floor, std::optional<string_id<furn_t>> floor_center,
                                       string_id<furn_t> door_closed ) : moves_total( moves_total ), target( target ), radius( radius ),
            it( it ), wall( wall ), floor( floor ), floor_center( floor_center ),
            door_closed( door_closed ) {}

        const activity_id &get_type() const override {
            static const activity_id ACT_TENT_PLACE( "ACT_TENT_PLACE" );
            return ACT_TENT_PLACE;
        }

        void start( player_activity &act, Character & ) override;
        void do_turn( player_activity &, Character & ) override {}
        void finish( player_activity &act, Character &p ) override;
        void canceled( player_activity &, Character &p ) override;

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<tent_placement_activity_actor>( *this );
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );
};

class oxytorch_activity_actor : public activity_actor
{
    public:
        explicit oxytorch_activity_actor( const tripoint_bub_ms &target,
                                          const item_location &tool ) : target( target ), tool( tool ) {};

        const activity_id &get_type() const override {
            static const activity_id ACT_OXYTORCH( "ACT_OXYTORCH" );
            return ACT_OXYTORCH;
        }

        void start( player_activity &act, Character &who ) override;
        void do_turn( player_activity &/*act*/, Character &who ) override;
        void finish( player_activity &act, Character &who ) override;

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<oxytorch_activity_actor>( *this );
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );

        // debugmsg causes a backtrace when fired during cata_test
        bool testing = false;  // NOLINT(cata-serialize)
    private:
        tripoint_bub_ms target;
        item_location tool;

        bool can_resume_with_internal( const activity_actor &other,
                                       const Character &/*who*/ ) const override {
            const oxytorch_activity_actor &actor = static_cast<const oxytorch_activity_actor &>
                                                   ( other );
            return actor.target == target;
        }
};

class outfit_swap_actor : public activity_actor
{
    public:
        explicit outfit_swap_actor( const item_location &outfit_item ) : outfit_item( outfit_item ) {};
        const activity_id &get_type() const override {
            static const activity_id ACT_OUTFIT_SWAP( "ACT_OUTFIT_SWAP" );
            return ACT_OUTFIT_SWAP;
        }

        void start( player_activity &act, Character &who ) override;
        void do_turn( player_activity &, Character & ) override {}
        void finish( player_activity &act, Character &who ) override;

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<outfit_swap_actor>( *this );
        }

        void serialize( JsonOut & ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue & );
    private:
        item_location outfit_item;

        bool can_resume_with_internal( const activity_actor &, const Character & ) const override {
            return false;
        }
};

class meditate_activity_actor : public activity_actor
{
    public:
        meditate_activity_actor() = default;
        const activity_id &get_type() const override {
            static const activity_id ACT_MEDITATE( "ACT_MEDITATE" );
            return ACT_MEDITATE;
        }

        void start( player_activity &act, Character & ) override;
        void do_turn( player_activity &, Character & ) override {}
        void finish( player_activity &act, Character &who ) override;

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<meditate_activity_actor>( *this );
        }

        void serialize( JsonOut & ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue & );
};

class play_with_pet_activity_actor : public activity_actor
{
    private:
        std::string pet_name;
        std::string playstr;
    public:
        play_with_pet_activity_actor() = default;
        explicit play_with_pet_activity_actor( const std::string &pet_name ) :
            pet_name( pet_name ) {};
        explicit play_with_pet_activity_actor( const std::string &pet_name, const std::string &playstr ) :
            pet_name( pet_name ), playstr( playstr ) {};
        const activity_id &get_type() const override {
            static const activity_id ACT_PLAY_WITH_PET( "ACT_PLAY_WITH_PET" );
            return ACT_PLAY_WITH_PET;
        }

        void start( player_activity &act, Character & ) override;
        void do_turn( player_activity &, Character & ) override {}
        void finish( player_activity &act, Character &who ) override;

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<play_with_pet_activity_actor>( *this );
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );
};

class prying_activity_actor : public activity_actor
{
    public:
        explicit prying_activity_actor( const tripoint_bub_ms &target,
                                        const item_location &tool ) : target( target ), tool( tool ) {};

        const activity_id &get_type() const override {
            static const activity_id ACT_PRYING( "ACT_PRYING" );
            return ACT_PRYING;
        }

        static time_duration prying_time( const activity_data_common &data, const item_location &tool,
                                          const Character &who );

        void start( player_activity &act, Character &who ) override;
        void do_turn( player_activity &/*act*/, Character &who ) override;
        void finish( player_activity &act, Character &who ) override;

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<prying_activity_actor>( *this );
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );

        // debugmsg causes a backtrace when fired during cata_test
        bool testing = false;  // NOLINT(cata-serialize)
    private:
        tripoint_bub_ms target;
        item_location tool;
        bool prying_nails = false;

        bool can_resume_with_internal( const activity_actor &other,
                                       const Character &/*who*/ ) const override {
            const prying_activity_actor &actor = static_cast<const prying_activity_actor &>
                                                 ( other );
            return actor.target == target;
        }

        // regular prying has lots of conditionals...
        void handle_prying( Character &who );
        // prying nails is much simpler
        void handle_prying_nails( Character &who );
};

class tent_deconstruct_activity_actor : public activity_actor
{
    private:
        int moves_total;
        int radius;
        tripoint_bub_ms target;
        itype_id tent;

    public:
        tent_deconstruct_activity_actor( int moves_total, int radius, tripoint_bub_ms target,
                                         itype_id tent ) : moves_total( moves_total ), radius( radius ), target( target ), tent( tent ) {}

        const activity_id &get_type() const override {
            static const activity_id ACT_TENT_DECONSTRUCT( "ACT_TENT_DECONSTRUCT" );
            return ACT_TENT_DECONSTRUCT;
        }

        void start( player_activity &act, Character & ) override;
        void do_turn( player_activity &, Character & ) override {}
        void finish( player_activity &act, Character & ) override;

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<tent_deconstruct_activity_actor>( *this );
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );
};

class reel_cable_activity_actor : public activity_actor
{
    private:
        int moves_total;
        item_location cable;
    public:
        reel_cable_activity_actor( int moves_total, const item_location &cable ) :
            moves_total( moves_total ), cable( cable ) {}
        const activity_id &get_type() const override {
            static const activity_id ACT_REEL_CABLE( "ACT_REEL_CABLE" );
            return ACT_REEL_CABLE;
        }

        bool can_resume_with_internal( const activity_actor &other,
                                       const Character &/*who*/ ) const override {
            const reel_cable_activity_actor &actor = static_cast<const reel_cable_activity_actor &>
                    ( other );
            return actor.cable == cable;
        }

        void start( player_activity &act, Character & ) override;
        void do_turn( player_activity &, Character & ) override {}
        void finish( player_activity &act, Character &who ) override;

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<reel_cable_activity_actor>( *this );
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );
};

class shave_activity_actor : public activity_actor
{
    public:
        shave_activity_actor() = default;
        const activity_id &get_type() const override {
            static const activity_id ACT_SHAVE( "ACT_SHAVE" );
            return ACT_SHAVE;
        }

        void start( player_activity &act, Character & ) override;
        void do_turn( player_activity &, Character & ) override {}
        void finish( player_activity &act, Character &who ) override;

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<shave_activity_actor>( *this );
        }

        void serialize( JsonOut & ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue & );
};

class haircut_activity_actor : public activity_actor
{
    public:
        haircut_activity_actor() = default;
        const activity_id &get_type() const override {
            static const activity_id ACT_HAIRCUT( "ACT_HAIRCUT" );
            return ACT_HAIRCUT;
        }

        void start( player_activity &act, Character & ) override;
        void do_turn( player_activity &, Character & ) override {}
        void finish( player_activity &act, Character &who ) override;

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<haircut_activity_actor>( *this );
        }

        void serialize( JsonOut & ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue & );
};

class vehicle_folding_activity_actor : public activity_actor
{
    private:
        // position of vehicle; used to identify the vehicle
        tripoint_bub_ms target_pos;
        // time folding the vehicle should take
        time_duration folding_time;
        // This tries folding, optionally bailing early and only checking for errors
        // @param check_only if true stops early, only checking if folding is possible
        // @returns ( check_only && folding is possible ) || ( !check_only and successfully folded )
        bool fold_vehicle( Character &p, bool check_only ) const;

        // private empty constructor for deserialization
        explicit vehicle_folding_activity_actor() = default;
    public:
        explicit vehicle_folding_activity_actor( const vehicle &target );

        const activity_id &get_type() const override {
            static const activity_id ACT_VEHICLE_FOLD( "ACT_VEHICLE_FOLD" );
            return ACT_VEHICLE_FOLD;
        }

        void start( player_activity &act, Character &p ) override;
        void do_turn( player_activity &act, Character &p ) override;
        void finish( player_activity &act, Character &p ) override;

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<vehicle_folding_activity_actor>( *this );
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );
};

class vehicle_unfolding_activity_actor : public activity_actor
{
    private:
        // folded item to restore from
        item it;
        // time unfolding the vehicle should take
        time_duration unfolding_time;
        // This tries unfolding, optionally bailing early and only checking for errors
        // @param check_only if true stops early, only checking if unfolding is possible
        // @returns ( check_only && unfolding is possible ) || ( !check_only and successfully unfolded )
        bool unfold_vehicle( Character &p, bool check_only ) const;

        // private empty constructor for deserialization
        explicit vehicle_unfolding_activity_actor() = default;
    public:
        explicit vehicle_unfolding_activity_actor( const item &it );

        const activity_id &get_type() const override {
            static const activity_id ACT_VEHICLE_UNFOLD( "ACT_VEHICLE_UNFOLD" );
            return ACT_VEHICLE_UNFOLD;
        }

        void start( player_activity &act, Character &p ) override;
        void do_turn( player_activity &act, Character &p ) override;
        void finish( player_activity &act, Character &p ) override;
        void canceled( player_activity &act, Character &p ) override;

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<vehicle_unfolding_activity_actor>( *this );
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );
};

class wash_activity_actor : public activity_actor
{
    private:
        wash_activity_actor() = default;
    public:
        wash_activity_actor( drop_locations to_wash,
                             washing_requirements &requirements ) :
            to_wash( std::move( to_wash ) ),
            requirements( requirements ) {};

        const activity_id &get_type() const override {
            static const activity_id ACT_WASH( "ACT_WASH" );
            return ACT_WASH;
        }

        void start( player_activity &act, Character & ) override;
        void do_turn( player_activity &, Character & ) override {};
        void finish( player_activity &act, Character &p ) override;

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<wash_activity_actor>( *this );
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );

    private:
        drop_locations to_wash;
        washing_requirements requirements;
};

class heat_activity_actor : public activity_actor
{
    private:
        heat_activity_actor() = default;
        int get_available_heater( Character &p, item_location &loc ) const;
    public:
        heat_activity_actor( drop_locations to_heat,
                             heating_requirements &requirements,
                             heater h ) :
            to_heat( std::move( to_heat ) ), requirements( requirements ), h( std::move( h ) ) {};

        const activity_id &get_type() const override {
            static const activity_id ACT_HEATING( "ACT_HEATING" );
            return ACT_HEATING;
        }

        void start( player_activity &act, Character & ) override;
        void do_turn( player_activity &act, Character &p ) override;
        void finish( player_activity &act, Character &p ) override;

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<heat_activity_actor>( *this );
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );

    private:
        drop_locations to_heat;
        heating_requirements requirements;
        heater h;
};

class wear_activity_actor : public activity_actor
{
    public:
        wear_activity_actor( std::vector<item_location> target_items, std::vector<int> quantities ) :
            target_items( std::move( target_items ) ),
            quantities( std::move( quantities ) ) {};
        const activity_id &get_type() const override {
            static const activity_id ACT_WEAR( "ACT_WEAR" );
            return ACT_WEAR;
        }

        void start( player_activity &, Character & ) override {};
        void do_turn( player_activity &, Character &who ) override;
        void finish( player_activity &, Character & ) override {};

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<wear_activity_actor>( *this );
        }

        void serialize( JsonOut & ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue & );

    private:
        std::vector<item_location> target_items;
        std::vector<int> quantities;
        contents_change_handler handler;
};

class wield_activity_actor : public activity_actor
{
    public:
        wield_activity_actor( item_location target_item, int quantity ) :
            target_item( std::move( target_item ) ),
            quantity( quantity ) {};
        const activity_id &get_type() const override {
            static const activity_id ACT_WIELD( "ACT_WIELD" );
            return ACT_WIELD;
        }

        void start( player_activity &, Character & ) override {};
        void do_turn( player_activity &, Character &who ) override;
        void finish( player_activity &, Character & ) override {};

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<wield_activity_actor>( *this );
        }

        void serialize( JsonOut & ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue & );

    private:
        item_location target_item;
        int quantity;
        contents_change_handler handler;
};

class invoke_item_activity_actor : public activity_actor
{
    public:
        invoke_item_activity_actor( item_location item, std::string method ) :
            item( std::move( item ) ),
            method( std::move( method ) ) {};
        const activity_id &get_type() const override {
            static const activity_id ACT_INVOKE_ITEM( "ACT_INVOKE_ITEM" );
            return ACT_INVOKE_ITEM;
        }

        void start( player_activity &, Character & ) override {};
        void do_turn( player_activity &, Character &who ) override;
        void finish( player_activity &, Character & ) override {};

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<invoke_item_activity_actor>( *this );
        }

        void serialize( JsonOut & ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue & );

    private:
        item_location item;
        std::string method;
};

class chop_logs_activity_actor : public activity_actor
{
    public:
        chop_logs_activity_actor() = default;
        chop_logs_activity_actor( int moves, const item_location &tool ) : moves( moves ), tool( tool ) {}

        const activity_id &get_type() const override {
            static const activity_id ACT_CHOP_LOGS( "ACT_CHOP_LOGS" );
            return ACT_CHOP_LOGS;
        }

        void start( player_activity &act, Character &who ) override;
        void do_turn( player_activity &, Character & ) override;
        void finish( player_activity &act, Character &who ) override;

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<chop_logs_activity_actor>( *this );
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );

    private:
        int moves;
        item_location tool;
};

class chop_planks_activity_actor : public activity_actor
{
    public:
        chop_planks_activity_actor() = default;
        explicit chop_planks_activity_actor( int moves ) : moves( moves ) {}

        const activity_id &get_type() const override {
            static const activity_id ACT_CHOP_PLANKS( "ACT_CHOP_PLANKS" );
            return ACT_CHOP_PLANKS;
        }

        void start( player_activity &act, Character &who ) override;
        void do_turn( player_activity &, Character & ) override;
        void finish( player_activity &act, Character &who ) override;

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<chop_planks_activity_actor>( *this );
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );

    private:
        int moves;
};

class chop_tree_activity_actor : public activity_actor
{
    public:
        chop_tree_activity_actor() = default;
        chop_tree_activity_actor( int moves, const item_location &tool ) : moves( moves ), tool( tool ) {}

        const activity_id &get_type() const override {
            static const activity_id ACT_CHOP_TREE( "ACT_CHOP_TREE" );
            return ACT_CHOP_TREE;
        }

        void start( player_activity &act, Character &who ) override;
        void do_turn( player_activity &, Character & ) override;
        void finish( player_activity &act, Character &who ) override;

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<chop_tree_activity_actor>( *this );
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );

    private:
        int moves;
        item_location tool;
};

class churn_activity_actor : public activity_actor
{
    public:
        churn_activity_actor() = default;
        churn_activity_actor( int moves, const item_location &tool ) : moves( moves ), tool( tool ) {}

        const activity_id &get_type() const override {
            static const activity_id ACT_CHURN( "ACT_CHURN" );
            return ACT_CHURN;
        }

        void start( player_activity &act, Character &who ) override;
        void do_turn( player_activity &, Character & ) override {}
        void finish( player_activity &act, Character &who ) override;

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<churn_activity_actor>( *this );
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );

    private:
        int moves;
        item_location tool;
};

class clear_rubble_activity_actor : public activity_actor
{
    public:
        clear_rubble_activity_actor() = default;
        explicit clear_rubble_activity_actor( int moves ) : moves( moves ) {}

        const activity_id &get_type() const override {
            static const activity_id ACT_CLEAR_RUBBLE( "ACT_CLEAR_RUBBLE" );
            return ACT_CLEAR_RUBBLE;
        }

        void start( player_activity &act, Character &who ) override;
        void do_turn( player_activity &, Character & ) override {}
        void finish( player_activity &act, Character &who ) override;

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<clear_rubble_activity_actor>( *this );
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );

    private:
        int moves;
};

class disable_activity_actor : public activity_actor
{
    public:
        disable_activity_actor() = default;
        disable_activity_actor( const tripoint_bub_ms &target, int moves_total,
                                bool reprogram ) : target( target ), moves_total( moves_total ), reprogram( reprogram ) {}

        const activity_id &get_type() const override {
            static const activity_id ACT_DISABLE( "ACT_DISABLE" );
            return ACT_DISABLE;
        }

        void start( player_activity &act, Character &who ) override;
        void do_turn( player_activity & /*&act*/, Character &who ) override;
        void finish( player_activity &act, Character &who ) override;

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<disable_activity_actor>( *this );
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );

        /** Returns whether the given monster is a robot and can currently be disabled or reprogrammed */
        static bool can_disable_or_reprogram( const monster &monster );

        static int get_disable_turns();

    private:
        tripoint_bub_ms target;
        int moves_total;
        bool reprogram;
};

class firstaid_activity_actor : public activity_actor
{
    public:
        firstaid_activity_actor() = default;
        firstaid_activity_actor( int moves, std::string name, character_id patientID ) : moves( moves ),
            name( std::move( name ) ), patientID( patientID ) {}

        const activity_id &get_type() const override {
            static const activity_id ACT_FIRSTAID( "ACT_FIRSTAID" );
            return ACT_FIRSTAID;
        }

        void start( player_activity &act, Character &who ) override;
        void do_turn( player_activity &, Character & ) override {};
        void finish( player_activity &act, Character &who ) override;

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<firstaid_activity_actor>( *this );
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );

    private:
        int moves;
        std::string name;
        character_id patientID;
};

class forage_activity_actor : public activity_actor
{
    public:
        forage_activity_actor() = default;
        explicit forage_activity_actor( int moves ) : moves( moves ) {}

        const activity_id &get_type() const override {
            static const activity_id ACT_FORAGE( "ACT_FORAGE" );
            return ACT_FORAGE;
        }

        void start( player_activity &act, Character &who ) override;
        void do_turn( player_activity &, Character & ) override {};
        void finish( player_activity &act, Character &who ) override;

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<forage_activity_actor>( *this );
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );

    private:
        int moves;
};

class gunmod_add_activity_actor : public activity_actor
{
    public:
        gunmod_add_activity_actor() = default;
        gunmod_add_activity_actor( int moves, std::string name ) : moves( moves ),
            name( std::move( name ) ) {}

        const activity_id &get_type() const override {
            static const activity_id ACT_GUNMOD_ADD( "ACT_GUNMOD_ADD" );
            return ACT_GUNMOD_ADD;
        }

        void start( player_activity &act, Character &who ) override;
        void do_turn( player_activity &, Character & ) override {};
        void finish( player_activity &act, Character &who ) override;

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<gunmod_add_activity_actor>( *this );
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );

    private:
        int moves;
        std::string name;
};

class longsalvage_activity_actor : public activity_actor
{
    public:
        longsalvage_activity_actor() = default;
        explicit longsalvage_activity_actor( int index ) : index( index ) {}

        const activity_id &get_type() const override {
            static const activity_id ACT_LONGSALVAGE( "ACT_LONGSALVAGE" );
            return ACT_LONGSALVAGE;
        }

        void start( player_activity &act, Character &who ) override;
        void do_turn( player_activity &, Character & ) override {};
        void finish( player_activity &act, Character &who ) override;

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<longsalvage_activity_actor>( *this );
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );

    private:
        int index;
};

class mop_activity_actor : public activity_actor
{
    public:
        mop_activity_actor() = default;
        explicit mop_activity_actor( int moves ) : moves( moves ) {}

        const activity_id &get_type() const override {
            static const activity_id ACT_MOP( "ACT_MOP" );
            return ACT_MOP;
        }

        void start( player_activity &act, Character &who ) override;
        void do_turn( player_activity &, Character & ) override {};
        void finish( player_activity &act, Character &who ) override;

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<mop_activity_actor>( *this );
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );

    private:
        int moves;
};

class unload_loot_activity_actor : public activity_actor
{
    public:
        unload_loot_activity_actor() = default;
        explicit unload_loot_activity_actor( int moves ) : moves( moves ) {}

        const activity_id &get_type() const override {
            static const activity_id ACT_UNLOAD_LOOT( "ACT_UNLOAD_LOOT" );
            return ACT_UNLOAD_LOOT;
        }

        void start( player_activity &act, Character &who ) override;
        void do_turn( player_activity &act, Character &you ) override;
        void finish( player_activity &, Character & ) override {};

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<unload_loot_activity_actor>( *this );
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );

    private:
        int moves;
        int num_processed;
        int stage;
        std::unordered_set<tripoint_abs_ms> coord_set;
        tripoint_abs_ms placement;
};

class pulp_activity_actor : public activity_actor
{
    public:
        pulp_activity_actor() = default;
        explicit pulp_activity_actor( const tripoint_abs_ms placement ) : placement( { placement } ),
        num_corpses( 0 ) {}
        explicit pulp_activity_actor( const std::set<tripoint_abs_ms> &placement ) : placement( placement ),
            num_corpses( 0 ) {}
        const activity_id &get_type() const override {
            static const activity_id ACT_PULP( "ACT_PULP" );
            return ACT_PULP;
        }

        void start( player_activity &act, Character &you ) override;
        void do_turn( player_activity &act, Character &you ) override;
        bool punch_corpse_once( item &corpse, Character &you, tripoint_bub_ms pos, map &here );
        void finish( player_activity &, Character & ) override;

        void send_final_message( Character &you ) const;

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<pulp_activity_actor>( *this );
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );

    private:
        // tripoints with corpses we need to pulp;
        // either single tripoint shoved into set, or 3x3 zone around u/npc
        std::set<tripoint_abs_ms> placement;

        float float_corpse_damage_accum = 0.0f; // NOLINT(cata-serialize)

        int unpulped_corpses_qty = 0;

        // query player if they want to pulp corpses that cost more than 10 minutes to pulp
        bool too_long_to_pulp = false;
        bool too_long_to_pulp_interrupted = false;
        // if corpse cost more than hour to pulp, drop it
        bool way_too_long_to_pulp = false;

        bool acid_corpse = false;

        // how many corpses we pulped
        int num_corpses = 0;

        pulp_data pd;
};

class wait_stamina_activity_actor : public activity_actor
{

    public:
        // Wait until stamina is at the maximum.
        wait_stamina_activity_actor() = default;

        // If we're given a threshold, wait until stamina is at least this value.
        explicit wait_stamina_activity_actor( int stamina_threshold ) : stamina_threshold(
                stamina_threshold ) {};

        void start( player_activity &act, Character &who ) override;
        void do_turn( player_activity &act, Character &you ) override;
        void finish( player_activity &act, Character &you ) override;

        const activity_id &get_type() const override {
            static const activity_id ACT_WAIT_STAMINA( "ACT_WAIT_STAMINA" );
            return ACT_WAIT_STAMINA;
        }

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<wait_stamina_activity_actor>( *this );
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );

    private:
        int stamina_threshold = -1;
        int initial_stamina = -1;
};

#endif // CATA_SRC_ACTIVITY_ACTOR_DEFINITIONS_H
