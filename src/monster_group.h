#ifndef MONSTER_GROUP_H
#define MONSTER_GROUP_H

#include "monster.h"

#include "enums.h"
#include "senses.h"

#include <map>
#include <vector>

class monster_group : public JsonSerializer, public JsonDeserializer
{
    public:
        void add_monster( monster &new_monster );
        void add_monster_group( class monster_group &new_monsters );

        void apply_stimulus( const tripoint &location, const monster_group_stimulus &stimulus );

        // Pass in a target turn number and a senses object.
        // Returns a map of destinations for the processed monster groups for re-insertion
        // into other groups.
        std::map<tripoint, monster_group> move(
            const tripoint &location, const int turn_number, const senses &sensor );
    private:
        // TODO: consider replacing with a std::priority_queue
        std::vector<monster> monster_list;
        monster_group_stimulus strongest_stimulus = null_monster_stimulus;
    protected:
        void serialize( JsonOut &jsout ) const;
        void deserialize( JsonIn &jsin );

};

#endif
