#ifndef _CREATURE_TRACKER_H_
#define _CREATURE_TRACKER_H_

#include "monster.h"
#include "creature.h"
#include "enums.h"
#include <vector>

class Creature_tracker
{
    public:
        Creature_tracker();
        monster &find(int index);
        int mon_at(point coords) const;
        int mon_at(int x_pos, int y_pos) const;
        bool add(monster &critter);
        size_t size() const;
        bool update_pos(const monster &critter, const int new_x_pos, const int new_y_pos);
        void remove(const int idx);
        void clear();
        void rebuild_cache();
        const std::vector<monster>& list() const;

    private:
        std::vector<monster> _old_monsters_list;
        std::map<point, int> _old_monsters_by_location;
};

#endif
