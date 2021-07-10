#pragma once
#ifndef CATA_SRC_ACTIVITY_ACTOR_DEFINITIONS_H
#define CATA_SRC_ACTIVITY_ACTOR_DEFINITIONS_H

#include <algorithm>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "activity_type.h"
#include "calendar.h"
#include "character.h"
#include "clone_ptr.h"
#include "handle_liquid.h"
#include "item.h"
#include "item_location.h"
#include "memory_fast.h"
#include "optional.h"
#include "point.h"
#include "string_id.h"
#include "type_id.h"
#include "units.h"
#include "units_fwd.h"
#include "vehicle.h"

#include "activity_actor.h"

class Character;
class Creature;
class JsonIn;
class JsonOut;
class avatar;
class player_activity;

class aim_activity_actor : public activity_actor
{
    private:
        cata::optional<item> fake_weapon;
        units::energy bp_cost_per_shot = 0_J;
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
        std::vector<weak_ptr_fast<Creature>> acceptable_losses;

        aim_activity_actor();

        /** Aiming wielded gun */
        static aim_activity_actor use_wielded();

        /** Aiming fake gun provided by a bionic */
        static aim_activity_actor use_bionic( const item &fake_gun, const units::energy &cost_per_shot );

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
        static std::unique_ptr<activity_actor> deserialize( JsonIn &jsin );

        item *get_weapon();
        void restore_view();
        // Load/unload a RELOAD_AND_SHOOT weapon
        bool load_RAS_weapon();
        void unload_RAS_weapon();
};

class autodrive_activity_actor : public activity_actor
{
    private:
        vehicle *player_vehicle = nullptr;

    public:
        autodrive_activity_actor() = default;

        activity_id get_type() const override {
            return activity_id( "ACT_AUTODRIVE" );
        }

        void start( player_activity &act, Character & ) override;
        void do_turn( player_activity &, Character & ) override;
        void canceled( player_activity &, Character & ) override;
        void finish( player_activity &act, Character & ) override;

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<autodrive_activity_actor>( *this );
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonIn &jsin );
};

class dig_activity_actor : public activity_actor
{
    private:
        int moves_total;
        /** location of the dig **/
        tripoint location;
        std::string result_terrain;
        tripoint byproducts_location;
        int byproducts_count;
        item_group_id byproducts_item_group;

        /**
         * Returns true if @p other and `this` are "equivalent" in the sense that
         *  `this` can be resumed instead of starting @p other.
         */
        bool equivalent_activity( const dig_activity_actor &other ) const {
            return  location == other.location &&
                    result_terrain == other.result_terrain &&
                    byproducts_location == other.byproducts_location &&
                    byproducts_count == other.byproducts_count &&
                    byproducts_item_group == other.byproducts_item_group;
        }

        /**
         * @pre @p other is a `dig_activity_actor`
         */
        bool can_resume_with_internal( const activity_actor &other, const Character & ) const override {
            const dig_activity_actor &d_actor = static_cast<const dig_activity_actor &>( other );
            return equivalent_activity( d_actor );
        }

    public:
        dig_activity_actor(
            int dig_moves, const tripoint &dig_loc,
            const std::string &resulting_ter, const tripoint &dump_loc,
            int dump_spawn_count, const std::string &dump_item_group
        ):
            moves_total( dig_moves ), location( dig_loc ),
            result_terrain( resulting_ter ),
            byproducts_location( dump_loc ),
            byproducts_count( dump_spawn_count ),
            byproducts_item_group( dump_item_group ) {}

        activity_id get_type() const override {
            return activity_id( "ACT_DIG" );
        }

        void start( player_activity &act, Character & ) override;
        void do_turn( player_activity &, Character & ) override;
        void finish( player_activity &act, Character &who ) override;

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<dig_activity_actor>( *this );
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonIn &jsin );
};

class dig_channel_activity_actor : public activity_actor
{
    private:
        int moves_total;
        /** location of the dig **/
        tripoint location;
        std::string result_terrain;
        tripoint byproducts_location;
        int byproducts_count;
        item_group_id byproducts_item_group;

        /**
         * Returns true if @p other and `this` are "equivalent" in the sense that
         *  `this` can be resumed instead of starting @p other.
         */
        bool equivalent_activity( const dig_channel_activity_actor &other ) const {
            return  location == other.location &&
                    result_terrain == other.result_terrain &&
                    byproducts_location == other.byproducts_location &&
                    byproducts_count == other.byproducts_count &&
                    byproducts_item_group == other.byproducts_item_group;
        }

        /**
         * @pre @p other is a `dig_activity_actor`
         */
        bool can_resume_with_internal( const activity_actor &other, const Character & ) const override {
            const dig_channel_activity_actor &dc_actor = static_cast<const dig_channel_activity_actor &>
                    ( other );
            return equivalent_activity( dc_actor );
        }

    public:
        dig_channel_activity_actor(
            int dig_moves, const tripoint &dig_loc,
            const std::string &resulting_ter, const tripoint &dump_loc,
            int dump_spawn_count, const std::string &dump_item_group
        ):
            moves_total( dig_moves ), location( dig_loc ),
            result_terrain( resulting_ter ),
            byproducts_location( dump_loc ),
            byproducts_count( dump_spawn_count ),
            byproducts_item_group( dump_item_group ) {}

        activity_id get_type() const override {
            return activity_id( "ACT_DIG_CHANNEL" );
        }

        void start( player_activity &act, Character & ) override;
        void do_turn( player_activity &, Character & ) override;
        void finish( player_activity &act, Character &who ) override;

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<dig_channel_activity_actor>( *this );
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonIn &jsin );
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
        static std::unique_ptr<activity_actor> deserialize( JsonIn &jsin );
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
        static std::unique_ptr<activity_actor> deserialize( JsonIn &jsin );
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
            const auto &a = static_cast<const hotwire_car_activity_actor &>( other );
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
        static std::unique_ptr<activity_actor> deserialize( JsonIn &jsin );
};

class bikerack_racking_activity_actor : public activity_actor
{
    private:
        int moves_total;
        vehicle &parent_vehicle;
        std::vector<std::vector<int>> parts;
    public:
        bikerack_racking_activity_actor( int moves_total, vehicle &parent_vehicle,
                                         std::vector<std::vector<int>> parts ): moves_total( moves_total ),
            parent_vehicle( parent_vehicle ), parts( parts ) {}

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
        static std::unique_ptr<activity_actor> deserialize( JsonIn &jsin );
};

class bikerack_unracking_activity_actor : public activity_actor
{
    private:
        int moves_total;
        vehicle &parent_vehicle;
        std::vector<int> parts;
        std::vector<int> racks;

    public:
        bikerack_unracking_activity_actor( int moves_total, vehicle &parent_vehicle,
                                           std::vector<int> parts, std::vector<int> racks ) : moves_total( moves_total ),
            parent_vehicle( parent_vehicle ), parts( parts ), racks( racks ) {}

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
        static std::unique_ptr<activity_actor> deserialize( JsonIn &jsin );
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
            target_items( target_items ), quantities( quantities ), to_vehicle( to_vehicle ),
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
        static std::unique_ptr<activity_actor> deserialize( JsonIn &jsin );
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
        cata::optional<tripoint> starting_pos;

    public:
        pickup_activity_actor( const std::vector<item_location> &target_items,
                               const std::vector<int> &quantities,
                               const cata::optional<tripoint> &starting_pos ) : target_items( target_items ),
            quantities( quantities ), starting_pos( starting_pos ) {}

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

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonIn &jsin );
};

class lockpick_activity_actor : public activity_actor
{
    private:
        int moves_total;
        cata::optional<item_location> lockpick;
        cata::optional<item> fake_lockpick;
        tripoint target;

        lockpick_activity_actor(
            int moves_total,
            const cata::optional<item_location> &lockpick,
            const cata::optional<item> &fake_lockpick,
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

        static cata::optional<tripoint> select_location( avatar &you );

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<lockpick_activity_actor>( *this );
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonIn &jsin );
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
        static std::unique_ptr<activity_actor> deserialize( JsonIn &jsin );
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
        static std::unique_ptr<activity_actor> deserialize( JsonIn &jsin );
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
        bool refuel = false;
        /**
         * @pre @p other is a consume_activity_actor
         */
        bool can_resume_with_internal( const activity_actor &other, const Character & ) const override {
            const consume_activity_actor &c_actor = static_cast<const consume_activity_actor &>( other );
            return ( consume_location == c_actor.consume_location &&
                     canceled == c_actor.canceled && &consume_item == &c_actor.consume_item );
        }
    public:
        consume_activity_actor( const item_location &consume_location,
                                std::vector<int> consume_menu_selections,
                                const std::vector<item_location> &consume_menu_selected_items,
                                const std::string &consume_menu_filter, activity_id type, bool refuel = false ) :
            consume_location( consume_location ), consume_menu_selections( consume_menu_selections ),
            consume_menu_selected_items( consume_menu_selected_items ),
            consume_menu_filter( consume_menu_filter ),
            type( type ), refuel( refuel ) {}

        explicit consume_activity_actor( const item_location &consume_location ) :
            consume_location( consume_location ), consume_menu_selections( std::vector<int>() ) {}

        explicit consume_activity_actor( const item &consume_item ) :
            consume_item( consume_item ), consume_menu_selections( std::vector<int>() ) {}

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
        static std::unique_ptr<activity_actor> deserialize( JsonIn &jsin );
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

        void query_keep_trying( player_activity &act, Character &who );

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<try_sleep_activity_actor>( *this );
        }

        std::string get_progress_message( const player_activity & ) const override {
            return std::string();
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonIn &jsin );
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
        static std::unique_ptr<activity_actor> deserialize( JsonIn &jsin );
};

class craft_activity_actor : public activity_actor
{
    private:
        item_location craft_item;
        bool is_long;

        float activity_override = NO_EXERCISE;
        cata::optional<requirement_data> cached_continuation_requirements;
        float cached_crafting_speed;
        int cached_assistants;
        double cached_base_total_moves;
        double cached_cur_total_moves;

        bool check_if_craft_okay( item_location &craft_item, Character &crafter );
    public:
        craft_activity_actor( item_location &it, bool is_long );

        activity_id get_type() const override {
            return activity_id( "ACT_CRAFT" );
        }

        void start( player_activity &act, Character &crafter ) override;
        void do_turn( player_activity &act, Character &crafter ) override;
        void finish( player_activity &act, Character &crafter ) override;

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<craft_activity_actor>( *this );
        }

        std::string get_progress_message( const player_activity & ) const override;

        float exertion_level() const override;

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonIn &jsin );
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
        static std::unique_ptr<activity_actor> deserialize( JsonIn &jsin );
};

class drop_or_stash_item_info
{
    public:
        drop_or_stash_item_info()
            : _loc( item_location::nowhere ) {}
        drop_or_stash_item_info( const item_location &_loc, const int _count )
            : _loc( _loc ), _count( _count ) {}

        void serialize( JsonOut &jsout ) const;
        void deserialize( JsonIn &jsin );

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
        static std::unique_ptr<activity_actor> deserialize( JsonIn &jsin );

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
        static std::unique_ptr<activity_actor> deserialize( JsonIn &jsin );

    private:
        std::vector<drop_or_stash_item_info> items;
        contents_change_handler handler;
        tripoint placement;
};

class reload_activity_actor : public activity_actor
{
    public:
        reload_activity_actor() = default;
        reload_activity_actor( int moves, int qty,
                               std::vector<item_location> &targets ) : moves_total( moves ), quantity( qty ),
            reload_targets( targets ) {
        }

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
        static std::unique_ptr<activity_actor> deserialize( JsonIn &jsin );

    private:
        int moves_total{};
        int quantity{};
        std::vector<item_location> reload_targets{};

        bool can_reload() const;
        static void make_reload_sound( Character &who, item &reloadable );

};

class milk_activity_actor : public activity_actor
{
    public:
        milk_activity_actor() = default;
        milk_activity_actor( int moves, std::vector<tripoint> coords,
                             std::vector<std::string> str_values ) : total_moves( moves ), monster_coords( coords ),
            string_values( str_values ) {}

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
        static std::unique_ptr<activity_actor> deserialize( JsonIn &jsin );

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
        static std::unique_ptr<activity_actor> deserialize( JsonIn &jsin );
};

class disassemble_activity_actor : public activity_actor
{
    private:
        int moves_total;

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

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<disassemble_activity_actor>( *this );
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonIn &jsin );
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
        static std::unique_ptr<activity_actor> deserialize( JsonIn &jsin );
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
        static std::unique_ptr<activity_actor> deserialize( JsonIn &jsin );
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
        cata::optional<string_id<furn_t>> floor_center;
        string_id<furn_t> door_closed;

    public:
        tent_placement_activity_actor( int moves_total, tripoint target, int radius, item it,
                                       string_id<furn_t> wall, string_id<furn_t> floor, cata::optional<string_id<furn_t>> floor_center,
                                       string_id<furn_t> door_closed ) : moves_total( moves_total ), target( target ), radius( radius ),
            it( it ), wall( wall ), floor( floor ), floor_center( floor_center ), door_closed( door_closed ) {}

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
        static std::unique_ptr<activity_actor> deserialize( JsonIn &jsin );
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
        static std::unique_ptr<activity_actor> deserialize( JsonIn & );
};

class play_with_pet_activity_actor : public activity_actor
{
    private:
        std::string pet_name;
    public:
        play_with_pet_activity_actor() = default;
        explicit play_with_pet_activity_actor( const std::string &pet_name ) :
            pet_name( pet_name ) {}
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
        static std::unique_ptr<activity_actor> deserialize( JsonIn &jsin );
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
        static std::unique_ptr<activity_actor> deserialize( JsonIn &jsin );
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
        static std::unique_ptr<activity_actor> deserialize( JsonIn & );
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
        static std::unique_ptr<activity_actor> deserialize( JsonIn & );
};

#endif // CATA_SRC_ACTIVITY_ACTOR_DEFINITIONS_H
