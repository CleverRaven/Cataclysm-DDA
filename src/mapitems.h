#ifndef MAPITEMS_H
#define MAPITEMS_H

typedef std::string items_location;

static const int num_itloc = 0;

// This is used only for monsters; they get a list of items_locations, and
// a chance that each one will be used.
struct items_location_and_chance {
    items_location loc;
    int chance;
    items_location_and_chance (items_location l, int c)
    {
        loc = l;
        chance = c;
    };
};

#endif
