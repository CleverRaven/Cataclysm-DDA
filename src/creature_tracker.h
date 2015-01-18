#ifndef CREATURE_TRACKER_H
#define CREATURE_TRACKER_H

#include "monster.h"
#include "creature.h"
#include "enums.h"
#include <vector>
#include <unordered_map>

class Creature_tracker
{
    public:
        Creature_tracker();
        ~Creature_tracker();
        /** Returns the monster at the given index. */
        monster &find(int index);
        /** Returns the monster index of the monster at the given point. */
        int mon_at(point coords) const;
        /** Adds the given monster to the creature_tracker. Returns whether the operation was successful. */
        bool add(monster &critter);
        size_t size() const;
        /** Updates the position of the given monster to the given point. Returns whether the operation 
         *  was successful. */
        bool update_pos(const monster &critter, const point new_pos);
        /** Removes the given monster index from the Creature tracker, adjusting other entries as needed. */
        void remove(const int idx);
        void clear();
        void rebuild_cache();
        const std::vector<monster> &list() const;

    private:
        std::vector<monster *> _old_monsters_list;
        std::unordered_map<point, int> _old_monsters_by_location;
        /** Returns the monster index of the dead monster at the given point. */
        int dead_mon_at(point coords) const;
};

#endif
