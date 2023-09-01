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
#include "handle_liquid.h"
#include "item.h"
#include "itype.h"
#include "item_location.h"
#include "memory_fast.h"
#include "point.h"
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

class aim_activity_actor : public activity_actor
{
    private:
        std::optional<item> fake_weapon;
        std::vector<tripoint> fin_trajectory;

    public:
        bool first_turn = true;
        std::string action;
        int aif_duration = 0; // Counts aim-and-fire duration
        bool aiming_at_critter = false; // Whether aiming at critter or a tile
        bool snap_to_target = false;
        bool shifting_view = false;
        tripoint initial_view_offset;
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

        activity_id get_type() const override {
            return activity_id( "ACT_AIM" );
        }

        void start( player_activity &act, Character &who ) override;
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

        activity_id get_type() const override {
            return activity_id( "ACT_AUTODRIVE" );
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

        activity_id get_type() const override {
            return activity_id( "ACT_GUNMOD_REMOVE" );
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
        explicit hacksaw_activity_actor( const tripoint &target,
                                         const item_location &tool ) : target( target ), tool( tool ) {};

        activity_id get_type() const override {
            return activity_id( "ACT_HACKSAW" );
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
        tripoint target;
        item_location tool;

        bool can_resume_with_internal( const activity_actor &other,
                                       const Character &/*who*/ ) const override;
};

class hacking_activity_actor : public activity_actor
{
    public:
        hacking_activity_actor() = default;

        activity_id get_type() const override {
            return activity_id( "ACT_HACKING" );
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

        activity_id get_type() const override {
            return activity_id( "ACT_BINDER_COPY_RECIPE" );
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

class data_handling_activity_actor: public activity_actor
{
    public:
        explicit data_handling_activity_actor() = default;
        explicit data_handling_activity_actor( const item_location &, const std::vector<item_location> & );

        activity_id get_type() const override {
            return activity_id( "ACT_DATA_HANDLING" );
        }

        bool can_resume_with_internal( const activity_actor &, const Character & ) const override {
            return false;
        }

        void start( player_activity &, Character & ) override;
        void do_turn( player_activity &, Character & ) override;
        void finish( player_activity &, Character & ) override;
        void canceled( player_activity &, Character & ) override;

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<data_handling_activity_actor>( *this );
        }

        void serialize( JsonOut & ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue & );
    private:
        static constexpr time_duration time_per_card = 1_minutes;
        item_location recorder;
        std::vector<item_location> targets;
        time_duration time_until_next_card = 0_seconds;
        int handled_cards = 0;
        int encrypted_cards = 0;
        int downloaded_photos = 0;
        int downloaded_songs = 0;
        std::vector<recipe_id> downloaded_recipes;
        int downloaded_extended_photos = 0;
        int downloaded_monster_photos = 0;
};

class hotwire_car_activity_actor : public activity_actor
{
    private:
        int moves_total;

        /**
         * Position of first vehicle part; used to identify the vehicle
         * TODO: find something more reliable (to cover cases when vehicle is moved/damaged)
         */
        tripoint target;

        bool can_resume_with_internal( const activity_actor &other, const Character & ) const override {
            const hotwire_car_activity_actor &a = static_cast<const hotwire_car_activity_actor &>( other );
            return target == a.target && moves_total == a.moves_total;
        }

    public:
        hotwire_car_activity_actor( int moves_total, const tripoint &target ): moves_total( moves_total ),
            target( target ) {}

        activity_id get_type() const override {
            return activity_id( "ACT_HOTWIRE_CAR" );
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

        activity_id get_type() const override {
            return activity_id( "ACT_BIKERACK_RACKING" );
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

        activity_id get_type() const override {
            return activity_id( "ACT_BIKERACK_UNRACKING" );
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

        activity_id get_type() const override {
            return activity_id( "ACT_READ" );
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
        tripoint relative_destination;

    public:
        move_items_activity_actor( std::vector<item_location> target_items, std::vector<int> quantities,
                                   bool to_vehicle, tripoint relative_destination ) :
            target_items( std::move( target_items ) ), quantities( std::move( quantities ) ),
            to_vehicle( to_vehicle ),
            relative_destination( relative_destination ) {}

        activity_id get_type() const override {
            return activity_id( "ACT_MOVE_ITEMS" );
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

        /**
         * Position of the character when the activity is started. This is
         * stored so that we can cancel the activity if the player moves
         * (e.g. if the player is in a moving vehicle). This should be null
         * if not grabbing from the ground.
         */
        std::optional<tripoint> starting_pos;

    public:
        pickup_activity_actor( const std::vector<item_location> &target_items,
                               const std::vector<int> &quantities,
                               const std::optional<tripoint> &starting_pos,
                               bool autopickup ) : target_items( target_items ),
            quantities( quantities ), starting_pos( starting_pos ), stash_successful( true ),
            autopickup( autopickup ) {}

        /**
          * Used to check at the end of a pickup activity if the character was able
          * to stash everything. If not, a message is displayed to clarify.
          */
        bool stash_successful;
        bool autopickup;

        activity_id get_type() const override {
            return activity_id( "ACT_PICKUP" );
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
        explicit boltcutting_activity_actor( const tripoint &target,
                                             const item_location &tool ) : target( target ), tool( tool ) {};

        activity_id get_type() const override {
            return activity_id( "ACT_BOLTCUTTING" );
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
        tripoint target;
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
        tripoint target;

        lockpick_activity_actor(
            int moves_total,
            const std::optional<item_location> &lockpick,
            const std::optional<item> &fake_lockpick,
            const tripoint &target
        ) : moves_total( moves_total ), lockpick( lockpick ), fake_lockpick( fake_lockpick ),
            target( target ) {}

    public:
        /** Use regular lockpick. 'target' is in global coords */
        static lockpick_activity_actor use_item(
            int moves_total,
            const item_location &lockpick,
            const tripoint &target
        );

        /** Use bionic lockpick. 'target' is in global coords */
        static lockpick_activity_actor use_bionic(
            const tripoint &target
        );

        activity_id get_type() const override {
            return activity_id( "ACT_LOCKPICK" );
        }

        void start( player_activity &act, Character & ) override;
        void do_turn( player_activity &, Character & ) override {}
        void finish( player_activity &act, Character &who ) override;

        static std::optional<tripoint> select_location( avatar &you );

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<lockpick_activity_actor>( *this );
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );
};

class ebooksave_activity_actor : public activity_actor
{
    public:
        explicit ebooksave_activity_actor( const item_location &book, const item_location &ereader ) :
            book( book ), ereader( ereader ) {};

        activity_id get_type() const override {
            return activity_id( "ACT_EBOOKSAVE" );
        }

        static int pages_in_book( const itype_id &book ) {
            // an A4 sheet weights roughly 5 grams
            return units::to_gram( book->weight ) / 5;
        };

        void start( player_activity &act, Character &/*who*/ ) override;
        void do_turn( player_activity &/*act*/, Character &who ) override;
        void finish( player_activity &act, Character &who ) override;

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<ebooksave_activity_actor>( *this );
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );

    private:
        item_location book;
        item_location ereader;
        static constexpr time_duration time_per_page = 5_seconds;

        bool can_resume_with_internal( const activity_actor &other,
                                       const Character &/*who*/ ) const override {
            const ebooksave_activity_actor &actor = static_cast<const ebooksave_activity_actor &>
                                                    ( other );
            return actor.book == book && actor.ereader == ereader;
        }
};

class migration_cancel_activity_actor : public activity_actor
{
    public:
        migration_cancel_activity_actor() = default;

        activity_id get_type() const override {
            return activity_id( "ACT_MIGRATION_CANCEL" );
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
        tripoint placement;

        /**
         * @pre @p other is a open_gate_activity_actor
         */
        bool can_resume_with_internal( const activity_actor &other, const Character & ) const override {
            const open_gate_activity_actor &og_actor = static_cast<const open_gate_activity_actor &>( other );
            return placement == og_actor.placement;
        }

    public:
        open_gate_activity_actor( int gate_moves, const tripoint &gate_placement ) :
            moves_total( gate_moves ), placement( gate_placement ) {}

        activity_id get_type() const override {
            return activity_id( "ACT_OPEN_GATE" );
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

        activity_id get_type() const override {
            return activity_id( "ACT_CONSUME" );
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

        activity_id get_type() const override {
            return activity_id( "ACT_TRY_SLEEP" );
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
        explicit safecracking_activity_actor( const tripoint &safe ) : safe( safe ) {};

        activity_id get_type() const override {
            return activity_id( "ACT_CRACKING" );
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
        tripoint safe;
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
        activity_id get_type() const override {
            return activity_id( "ACT_UNLOAD" );
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

        activity_id get_type() const override {
            return activity_id( "ACT_CRAFT" );
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
        tripoint location;
        time_point stop_time; // can resume if time apart is not above
        activity_id act_id = activity_id( "ACT_WORKOUT_LIGHT" ); // variable activities
        int intensity_modifier = 1;
        int elapsed = 0;

    public:
        explicit workout_activity_actor( const tripoint &loc ) : location( loc ) {}

        // can assume different sub-activities
        activity_id get_type() const override {
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
 * @placement is the offset to the current position of the actor (use tripoint_zero for current pos)
 * @force_ground should the items be forced to the ground instead of e.g. a container at the position
 */
class drop_activity_actor : public activity_actor
{
    public:
        drop_activity_actor() = default;
        drop_activity_actor( const std::vector<drop_or_stash_item_info> &items,
                             const tripoint &placement, const bool force_ground )
            : items( items ), placement( placement ), force_ground( force_ground ) {}

        activity_id get_type() const override {
            return activity_id( "ACT_DROP" );
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
        tripoint placement;
        bool force_ground = false;
};

class stash_activity_actor: public activity_actor
{
    public:
        stash_activity_actor() = default;
        stash_activity_actor( const std::vector<drop_or_stash_item_info> &items,
                              const tripoint &placement )
            : items( items ), placement( placement ) {}

        activity_id get_type() const override {
            return activity_id( "ACT_STASH" );
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
        tripoint placement;
};

class harvest_activity_actor : public activity_actor
{
    public:
        explicit harvest_activity_actor( const tripoint &target,
                                         bool auto_forage = false ) :
            target( target ), auto_forage( auto_forage ) {};

        activity_id get_type() const override {
            return activity_id( "ACT_HARVEST" );
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
        tripoint target;
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

        activity_id get_type() const override {
            return activity_id( "ACT_RELOAD" );
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
        milk_activity_actor( int moves, std::vector<tripoint> coords,
                             std::vector<std::string> str_values ) : total_moves( moves ), monster_coords( std::move( coords ) ),
            string_values( std::move( str_values ) ) {}

        activity_id get_type() const override {
            return activity_id( "ACT_MILK" );
        }

        void start( player_activity &act, Character &/*who*/ ) override;
        void do_turn( player_activity &/*act*/, Character &/*who*/ ) override {}
        void finish( player_activity &act, Character &who ) override;
        void canceled( player_activity &/*act*/, Character &/*who*/ ) override {}

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<milk_activity_actor>( *this );
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue &jsin );

    private:
        int total_moves {};
        std::vector<tripoint> monster_coords {};
        std::vector<std::string> string_values {};
};

class shearing_activity_actor : public activity_actor
{
    private:
        tripoint mon_coords;    // monster is tied for the duration
        bool shearing_tie;      // was the monster tied due to shearing

    public:
        explicit shearing_activity_actor(
            const tripoint &mon_coords, bool shearing_tie = true )
            : mon_coords( mon_coords ), shearing_tie( shearing_tie ) {};

        activity_id get_type() const override {
            return activity_id( "ACT_SHEARING" );
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
        bool use_cached_workbench_multiplier; // NOLINT(cata-serialize)
    public:
        item_location target;

        explicit disassemble_activity_actor( int moves_total ) :
            moves_total( moves_total ) {}
        activity_id get_type() const override {
            return activity_id( "ACT_DISASSEMBLE" );
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
        tripoint dp;
        bool via_ramp;

    public:
        move_furniture_activity_actor( const tripoint &dp, bool via_ramp ) :
            dp( dp ), via_ramp( via_ramp ) {}
        activity_id get_type() const override {
            return activity_id( "ACT_FURNITURE_MOVE" );
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

    public:

        insert_item_activity_actor() = default;
        insert_item_activity_actor( const item_location &holster, const drop_locations &holstered_list ) :
            holster( holster ), items( holstered_list ) {}

        activity_id get_type() const override {
            return activity_id( "ACT_INSERT_ITEM" );
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
        tripoint target;
        int radius = 1;
        item it;
        string_id<furn_t> wall;
        string_id<furn_t> floor;
        std::optional<string_id<furn_t>> floor_center;
        string_id<furn_t> door_closed;

    public:
        tent_placement_activity_actor( int moves_total, tripoint target, int radius, const item &it,
                                       string_id<furn_t> wall, string_id<furn_t> floor, std::optional<string_id<furn_t>> floor_center,
                                       string_id<furn_t> door_closed ) : moves_total( moves_total ), target( target ), radius( radius ),
            it( it ), wall( wall ), floor( floor ), floor_center( floor_center ),
            door_closed( door_closed ) {}

        activity_id get_type() const override {
            return activity_id( "ACT_TENT_PLACE" );
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
        explicit oxytorch_activity_actor( const tripoint &target,
                                          const item_location &tool ) : target( target ), tool( tool ) {};

        activity_id get_type() const override {
            return activity_id( "ACT_OXYTORCH" );
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
        tripoint target;
        item_location tool;

        bool can_resume_with_internal( const activity_actor &other,
                                       const Character &/*who*/ ) const override {
            const oxytorch_activity_actor &actor = static_cast<const oxytorch_activity_actor &>
                                                   ( other );
            return actor.target == target;
        }
};

class meditate_activity_actor : public activity_actor
{
    public:
        meditate_activity_actor() = default;
        activity_id get_type() const override {
            return activity_id( "ACT_MEDITATE" );
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
        activity_id get_type() const override {
            return activity_id( "ACT_PLAY_WITH_PET" );
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
        explicit prying_activity_actor( const tripoint &target,
                                        const item_location &tool ) : target( target ), tool( tool ) {};

        activity_id get_type() const override {
            return activity_id( "ACT_PRYING" );
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
        tripoint target;
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
        tripoint target;
        itype_id tent;

    public:
        tent_deconstruct_activity_actor( int moves_total, int radius, tripoint target,
                                         itype_id tent ) : moves_total( moves_total ), radius( radius ), target( target ), tent( tent ) {}

        activity_id get_type() const override {
            return activity_id( "ACT_TENT_DECONSTRUCT" );
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
        activity_id get_type() const override {
            return activity_id( "ACT_REEL_CABLE" );
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
        activity_id get_type() const override {
            return activity_id( "ACT_SHAVE" );
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
        activity_id get_type() const override {
            return activity_id( "ACT_HAIRCUT" );
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

        activity_id get_type() const override {
            return activity_id( "ACT_VEHICLE_FOLD" );
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

        activity_id get_type() const override {
            return activity_id( "ACT_VEHICLE_UNFOLD" );
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

        activity_id get_type() const override {
            return activity_id( "ACT_WASH" );
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

class wear_activity_actor : public activity_actor
{
    public:
        wear_activity_actor( std::vector<item_location> target_items, std::vector<int> quantities ) :
            target_items( std::move( target_items ) ),
            quantities( std::move( quantities ) ) {};
        activity_id get_type() const override {
            return activity_id( "ACT_WEAR" );
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
        activity_id get_type() const override {
            return activity_id( "ACT_WIELD" );
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
        activity_id get_type() const override {
            return activity_id( "ACT_INVOKE_ITEM" );
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

class pickup_menu_activity_actor : public activity_actor
{
    public:
        pickup_menu_activity_actor( std::optional<tripoint> where,
                                    std::vector<drop_location> selection ) : where( where ),
            selection( std::move( selection ) ) {};
        activity_id get_type() const override {
            return activity_id( "ACT_PICKUP_MENU" );
        }

        void start( player_activity &, Character & ) override {};
        void do_turn( player_activity &, Character &who ) override;
        void finish( player_activity &, Character & ) override {};

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<pickup_menu_activity_actor>( *this );
        }

        void serialize( JsonOut & ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonValue & );

    private:
        std::optional<tripoint> where;
        std::vector<drop_location> selection;
};

class chop_logs_activity_actor : public activity_actor
{
    public:
        chop_logs_activity_actor() = default;
        chop_logs_activity_actor( int moves, const item_location &tool ) : moves( moves ), tool( tool ) {}

        activity_id get_type() const override {
            return activity_id( "ACT_CHOP_LOGS" );
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

        activity_id get_type() const override {
            return activity_id( "ACT_CHOP_PLANKS" );
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

        activity_id get_type() const override {
            return activity_id( "ACT_CHOP_TREE" );
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

        activity_id get_type() const override {
            return activity_id( "ACT_CHURN" );
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

        activity_id get_type() const override {
            return activity_id( "ACT_CLEAR_RUBBLE" );
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
        disable_activity_actor( const tripoint &target, int moves_total,
                                bool reprogram ) : target( target ), moves_total( moves_total ), reprogram( reprogram ) {}

        activity_id get_type() const override {
            return activity_id( "ACT_DISABLE" );
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
        tripoint target;
        int moves_total;
        bool reprogram;
};

class firstaid_activity_actor : public activity_actor
{
    public:
        firstaid_activity_actor() = default;
        firstaid_activity_actor( int moves, std::string name, character_id patientID ) : moves( moves ),
            name( std::move( name ) ), patientID( patientID ) {}

        activity_id get_type() const override {
            return activity_id( "ACT_FIRSTAID" );
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

        activity_id get_type() const override {
            return activity_id( "ACT_FORAGE" );
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

        activity_id get_type() const override {
            return activity_id( "ACT_GUNMOD_ADD" );
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

        activity_id get_type() const override {
            return activity_id( "ACT_LONGSALVAGE" );
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

        activity_id get_type() const override {
            return activity_id( "ACT_MOP" );
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

        activity_id get_type() const override {
            return activity_id( "ACT_UNLOAD_LOOT" );
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
        std::unordered_set<tripoint> coord_set;
        tripoint placement;
};

#endif // CATA_SRC_ACTIVITY_ACTOR_DEFINITIONS_H
