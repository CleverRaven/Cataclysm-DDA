#pragma once
#ifndef CATA_SRC_CREATURE_TRACKER_H
#define CATA_SRC_CREATURE_TRACKER_H

#include <cstddef>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <unordered_map>
#include <vector>

#include "memory_fast.h"
#include "point.h"
#include "type_id.h"

class Creature;
class game;
class JsonIn;
class JsonOut;
class monster;
class npc;

class creature_tracker
{
        friend game;
    private:

        void add_to_faction_map( const shared_ptr_fast<monster> &critter );

        class weak_ptr_comparator
        {
            public:
                bool operator()( const weak_ptr_fast<monster> &lhs,
                                 const weak_ptr_fast<monster> &rhs ) const {
                    return lhs.lock().get() < rhs.lock().get();
                }
        };

        using MonstersByZ = std::map<int, std::set<weak_ptr_fast<monster>, weak_ptr_comparator>>;
        std::unordered_map<mfaction_id, MonstersByZ> monster_faction_map_; // NOLINT(cata-serialize)

        /**
         * Creatures that get removed via @ref remove are stored here until the end of the turn.
         * This keeps the objects valid and they can still be accessed instead of causing UB.
         */
        std::vector<shared_ptr_fast<monster>> removed_; // NOLINT(cata-serialize)

    public:
        creature_tracker();
        ~creature_tracker();
        /**
         * Returns the monster at the given location.
         * If there is no monster, it returns a `nullptr`.
         * Dead monsters are ignored and not returned.
         */
        shared_ptr_fast<monster> find( const tripoint &pos ) const;
        /**
         * Returns a temporary id of the given monster (which must exist in the tracker).
         * The id is valid until monsters are added or removed from the tracker.
         * The id remains valid through serializing and deserializing.
         * Use @ref from_temporary_id to get the monster pointer back. (The later may
         * return a nullptr if the given id is not valid.)
         */
        int temporary_id( const monster &critter ) const;
        shared_ptr_fast<monster> from_temporary_id( int id );
        /**
        * Adds the given monster to the tracker. @p critter must not be null.
         * If the operation succeeded, the monster pointer is now managed by this tracker.
         * @return Whether the operation was successful. It may fail if there is already
         * another monster at the location of the new monster.
         */
        bool add( const shared_ptr_fast<monster> &critter );
        size_t size() const;
        /** Updates the position of the given monster to the given point. Returns whether the operation
         *  was successful. */
        bool update_pos( const monster &critter, const tripoint &old_pos, const tripoint &new_pos );
        /** Removes the given monster from the Creature tracker, adjusting other entries as needed. */
        void remove( const monster &critter );
        void clear();
        void clear_npcs() {
            active_npc.clear();
        }
        void rebuild_cache();
        /** Swaps the positions of two monsters */
        void swap_positions( monster &first, monster &second );
        /** Kills 0 hp monsters. Returns if it killed any. */
        bool kill_marked_for_death();
        /** Removes dead monsters from. Their pointers are invalidated. */
        void remove_dead();

        /**
         * Returns the Creature at the given location. Optionally casted to the given
         * type of creature: @ref npc, @ref player, @ref monster - if there is a creature,
         * but it's not of the requested type, returns nullptr.
         * @param allow_hallucination Whether to return monsters that are actually hallucinations.
         */
        template<typename T = Creature>
        T * creature_at( const tripoint &p, bool allow_hallucination = false );
        template<typename T = Creature>
        const T * creature_at( const tripoint &p, bool allow_hallucination = false ) const;

        const std::vector<shared_ptr_fast<monster>> &get_monsters_list() const {
            return monsters_list;
        }

        void serialize( JsonOut &jsout ) const;
        void deserialize( JsonIn &jsin );

        const decltype( monster_faction_map_ ) &factions() const {
            return monster_faction_map_;
        }

    private:
        std::list<shared_ptr_fast<npc>> active_npc; // NOLINT(cata-serialize)
        std::vector<shared_ptr_fast<monster>> monsters_list;
        // NOLINTNEXTLINE(cata-serialize)
        std::unordered_map<tripoint, shared_ptr_fast<monster>> monsters_by_location;
        /** Remove the monsters entry in @ref monsters_by_location */
        void remove_from_location_map( const monster &critter );
};

creature_tracker &get_creature_tracker();

#endif // CATA_SRC_CREATURE_TRACKER_H
