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
        ~Creature_tracker();

        // Legacy monster API
        monster &find(int index);
        int mon_at(point coords) const;
        int mon_at(int x_pos, int y_pos) const;
        bool add(monster &critter);
        size_t size() const;
        bool update_pos(Creature &critter, const int new_x_pos, const int new_y_pos);
        void remove(const int idx);
        void clear();
        void rebuild_cache();
        const std::vector<monster>& list() const;

        // New creature API
        Creature *creature_at(point coords) const;
        bool add_creature(Creature *creature);
        std::set<Creature*> creature_set() const;
        void remove_creature(Creature *creature);


    private:
        std::vector<monster*> _old_monsters_list;
        std::map<point, int> _old_monsters_by_location;
        std::map<point, Creature*> _creatures_by_location;
        std::set<Creature*> _creature_set;
        // Same as mon_at, but only returns id of dead critters.
        int dead_mon_at(point coords) const;
};

#endif
