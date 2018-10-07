#pragma once
#ifndef CREATURE_TRACKER_H
#define CREATURE_TRACKER_H

#include "enums.h"

#include <list>
#include <memory>
#include <vector>
#include <unordered_map>

class monster;
class JsonIn;
class JsonOut;

class Creature_tracker
{
    public:
        Creature_tracker();
        ~Creature_tracker();
        /**
         * Returns the monster at the given location.
         * If there is no monster, it returns a `nullptr`.
         * Dead monsters are ignored and not returned.
         */
        std::shared_ptr<monster> find( const tripoint &pos ) const;
        /** Adds the given monster to the creature_tracker. Returns whether the operation was successful. */
        bool add( monster &critter );
        size_t size() const;
        /** Updates the position of the given monster to the given point. Returns whether the operation
         *  was successful. */
        bool update_pos( const monster &critter, const tripoint &new_pos );
        /** Removes the given monster from the Creature tracker, adjusting other entries as needed. */
        void remove( const monster &critter );
        void clear();
        void rebuild_cache();
        /** Swaps the positions of two monsters */
        void swap_positions( monster &first, monster &second );
        /** Kills 0 hp monsters. Returns if it killed any. */
        bool kill_marked_for_death();
        /** Removes dead monsters from. Their pointers are invalidated. */
        void remove_dead();

        const std::unordered_map<tripoint, std::shared_ptr<monster>> &get_monsters() const {
            return monsters_by_location;
        }

        void serialize( JsonOut &jsout ) const;
        void deserialize( JsonIn &jsin );

    private:
        bool check_location( const tripoint &pos );
        std::unordered_map<tripoint, std::shared_ptr<monster>> monsters_by_location;
        std::list<std::shared_ptr<monster>> dead_monsters;
};

#endif
