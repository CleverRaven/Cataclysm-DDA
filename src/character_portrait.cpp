#include "sdltiles.h" // IWYU pragma: keep
#include "string_id.h"

#if defined(TILES)
#include "cata_tiles.h"
#endif

struct character_portrait;


/** @relates string_id */
template<>
bool string_id<character_portrait>::is_valid() const
{
#if defined(TILES)
    // This is a stupid expensive validation compared to our normal string_id, be careful where you use it
    return portrait_tilecontext->get_texture_draw_data(
               str(),
               TILE_CATEGORY::PORTRAIT,
               tripoint_bub_ms() ).has_value();
#else
    return false;
#endif
}
