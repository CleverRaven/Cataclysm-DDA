#pragma once
#ifndef CATA_SRC_CURRENT_MAP_H
#define CATA_SRC_CURRENT_MAP_H

class map;

// The purpose of this class is to contain a reference to the current map.
// This map is typically the reality bubble map, but will differ e.g. when
// mapgen is invoked. Its only intended use is to be kept by the game object.
class current_map
{
        friend class game;
        friend class swap_map;

    protected:
        void set( map *new_map ) {
            current = new_map;
        }

    public:
        map &get_map();
    protected:
        map *current;
};

// The purpose of this class is to swap out the current map for the duration of
// this object's lifetime. This will typically be for the processing of some map
// that's not the reality bubble one, such as one used during mapgen.
class swap_map
{
    public:
        explicit swap_map( map &new_map );
        ~swap_map();

    private:
        map *previous;
};

#endif // CATA_SRC_CURRENT_MAP_H
