#ifndef SCENT_H
#define SCENT_H

#include "game_constants.h"

#include <memory>

using scent_array = std::array<std::array<int, SEEY *MAPSIZE>, SEEX *MAPSIZE>;

class scent_cache
{
        scent_cache();
        ~scent_cache();

        std::array<std::unique_ptr<scent_array>, OVERMAP_LAYERS> scents;
};

#endif
