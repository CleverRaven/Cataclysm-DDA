#if (!defined SDLTILES)

// see game.cpp
bool is_valid_in_w_terrain(int x, int y);

#include "game.h"

/* Line drawing code, not really an animation but should be separated anyway */

void game::draw_zones(const point &p_pointStart, const point &p_pointEnd,
                      const point &p_pointOffset)
{
    for (int iY = p_pointStart.y; iY <= p_pointEnd.y; ++iY) {
        for (int iX = p_pointStart.x; iX <= p_pointEnd.x; ++iX) {
            mvwputch_inv(w_terrain, iY - p_pointOffset.y, iX - p_pointOffset.x, c_ltgreen, '~');
        }
    }
}

#endif
