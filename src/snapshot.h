#ifndef SNAPSHOT_H
#define SNAPSHOT_H

#include <map>
#include <set>
#include <string>

#include "game_constants.h"
#include "enums.h"
#include "string_id.h"
#include "item_location.h"

using itype_id = std::string;

struct quality;
using quality_id = string_id<quality>;

class snapshot
{
    public:
        snapshot( const tripoint &pos, int range = PICKUP_RANGE ) :
            pos( pos ), range( range ) {
            refresh();
        }

        void refresh();

        int charges_of( const std::string &what, int limit = INT_MAX ) const;

    private:
        const tripoint pos;
        const int range;

        mutable std::map<itype_id, std::set<item_location>> items;
        mutable std::map<std::string, std::set<item_location>> charges;
        mutable std::map<quality_id, std::set<item_location>> quals;

        std::set<item_location> invalid;
};

#endif
