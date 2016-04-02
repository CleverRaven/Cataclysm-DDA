#ifndef SCENT_H
#define SCENT_H

#include "game_constants.h"

#include <memory>

using scent_array = std::array<std::array<int, SEEY *MAPSIZE>, SEEX *MAPSIZE>;

struct scent_layer {
    scent_layer();

    scent_array values;
};

class scent_cache
{
    public:
        scent_cache();
        ~scent_cache();

        int &get_ref( const tripoint &p );
        const int &get_ref( const tripoint &p ) const;
        // No bounds checks
        int &get_internal( const tripoint &p );

        void update( int minz, int maxz );
        // @todo `bool` mask to avoid decaying indoor scents with rain
        void decay( int zlev, int amount );

        void clear( int zlev );

        scent_layer &get_layer( int zlev );

    private:

        std::array<std::unique_ptr<scent_layer>, OVERMAP_LAYERS> scents;
};

#endif
