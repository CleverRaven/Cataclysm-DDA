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
        monster &find(int index);
        int mon_at(point coords) const;
        int mon_at(int x_pos, int y_pos) const;
        bool add(monster &critter);
        size_t size() const;
        bool update_pos(const monster &critter, const int new_x_pos, const int new_y_pos);
        void remove(const int idx);
        void clear();
        void rebuild_cache();
        const std::vector<monster> &list() const;

    private:
        std::vector<monster *> _old_monsters_list;
        std::unordered_map<point, int> _old_monsters_by_location;
        // Same as mon_at, but only returns id of dead critters.
        int dead_mon_at(point coords) const;
};

#endif
