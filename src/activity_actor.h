#pragma once
#ifndef CATA_SRC_ACTIVITY_ACTOR_H
#define CATA_SRC_ACTIVITY_ACTOR_H

#include <memory>
#include <unordered_map>
#include <vector>

#include "clone_ptr.h"
#include "item_location.h"
#include "optional.h"
#include "point.h"
#include "type_id.h"

class Character;
class JsonIn;
class JsonOut;
class player_activity;

class activity_actor
{
    public:
        virtual ~activity_actor() = default;

        /**
         * Should return the activity id of the corresponding activity
         */
        virtual activity_id get_type() const = 0;

        /**
         * Called once at the start of the activity.
         * This may be used to preform setup actions and/or set
         * player_activity::moves_left/moves_total.
         */
        virtual void start( player_activity &act, Character &who ) = 0;

        /**
         * Called on every turn of the activity
         * It may be used to stop the activity prematurely by setting it to null.
         */
        virtual void do_turn( player_activity &act, Character &who ) = 0;

        /**
         * Called when the activity runs out of moves, assuming that it has not
         * already been set to null
         */
        virtual void finish( player_activity &act, Character &who ) = 0;

        /**
         * Returns a deep copy of this object. Example implementation:
         * \code
         * class my_activity_actor {
         *     std::unique_ptr<activity_actor> clone() const override {
         *         return std::make_unique<my_activity_actor>( *this );
         *     }
         * };
         * \endcode
         * The returned value should behave like the original item and must have the same type.
         */
        virtual std::unique_ptr<activity_actor> clone() const = 0;

        /**
         * Must write any custom members of the derived class to json
         * Note that a static member function for deserialization must also be created and
         * added to the `activity_actor_deserializers` hashmap in activity_actor.cpp
         */
        virtual void serialize( JsonOut &jsout ) const = 0;
};

class hacking_activity_actor : public activity_actor
{
    private:
        bool using_bionic = false;

    public:
        struct use_bionic {};

        hacking_activity_actor() = default;
        hacking_activity_actor( use_bionic );

        activity_id get_type() const override {
            return activity_id( "ACT_HACKING" );
        }

        void start( player_activity &act, Character &who ) override;
        void do_turn( player_activity &, Character & ) override {};
        void finish( player_activity &act, Character &who ) override;

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<hacking_activity_actor>( *this );
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

        void start( player_activity &, Character & ) override {};
        void do_turn( player_activity &act, Character &who ) override;
        void finish( player_activity &, Character & ) override {};

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<move_items_activity_actor>( *this );
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

        void start( player_activity &, Character & ) override {};
        void do_turn( player_activity &act, Character &who ) override;
        void finish( player_activity &, Character & ) override {};

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<pickup_activity_actor>( *this );
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

        void start( player_activity &, Character & ) override {};
        void do_turn( player_activity &act, Character &who ) override;
        void finish( player_activity &, Character & ) override {};

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<migration_cancel_activity_actor>( *this );
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonIn &jsin );
};

class open_gate_activity_actor : public activity_actor
{
    private:
        int moves;
        tripoint placement;

    public:
        open_gate_activity_actor( int gate_moves, const tripoint &gate_placement ) :
            moves( gate_moves ), placement( gate_placement ) {}

        activity_id get_type() const override {
            return activity_id( "ACT_OPEN_GATE" );
        }

        void start( player_activity &act, Character & ) override;
        void do_turn( player_activity &, Character & ) override {};
        void finish( player_activity &act, Character & ) override;

        std::unique_ptr<activity_actor> clone() const override {
            return std::make_unique<open_gate_activity_actor>( *this );
        }

        void serialize( JsonOut &jsout ) const override;
        static std::unique_ptr<activity_actor> deserialize( JsonIn &jsin );
};

namespace activity_actors
{

// defined in activity_actor.cpp
extern const std::unordered_map<activity_id, std::unique_ptr<activity_actor>( * )( JsonIn & )>
deserialize_functions;

} // namespace activity_actors

void serialize( const cata::clone_ptr<activity_actor> &actor, JsonOut &jsout );
void deserialize( cata::clone_ptr<activity_actor> &actor, JsonIn &jsin );

#endif // CATA_SRC_ACTIVITY_ACTOR_H
