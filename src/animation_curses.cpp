#if (!defined SDLTILES)

// see game.cpp
bool is_valid_in_w_terrain(int x, int y);

#include "game.h"

/* Line drawing code, not really an animation but should be separated anyway */

void game::draw_line(const int x, const int y, std::vector<point> vPoint)
{
    (void)x; //unused
    (void)y; //unused
    int crx = POSX, cry = POSY;

    if(!vPoint.empty()) {
        crx += (vPoint[vPoint.size() - 1].x - (u.posx() + u.view_offset_x));
        cry += (vPoint[vPoint.size() - 1].y - (u.posy() + u.view_offset_y));
    }
    for (std::vector<point>::iterator it = vPoint.begin();
         it != vPoint.end() - 1; it++) {
        m.drawsq(w_terrain, u, it->x, it->y, true, true);
    }

    mvwputch(w_terrain, cry, crx, c_white, 'X');
}

void game::draw_weather(weather_printable wPrint)
{
    for (std::vector<std::pair<int, int> >::iterator weather_iterator = wPrint.vdrops.begin();
         weather_iterator != wPrint.vdrops.end();
         ++weather_iterator) {
        mvwputch(w_terrain, weather_iterator->second, weather_iterator->first, wPrint.colGlyph,
                 wPrint.cGlyph);
    }
}

void game::draw_sct()
{
    for (std::vector<scrollingcombattext::cSCT>::iterator iter = SCT.vSCT.begin();
         iter != SCT.vSCT.end(); ++iter) {
        const int iDY = POSY + (iter->getPosY() - (u.posy() + u.view_offset_y));
        const int iDX = POSX + (iter->getPosX() - (u.posx() + u.view_offset_x));
        if( !is_valid_in_w_terrain( iDX, iDY ) ) {
            continue;
        }

        mvwprintz(w_terrain, iDY, iDX, msgtype_to_color(iter->getMsgType("first"),
                  (iter->getStep() >= SCT.iMaxSteps / 2)), "%s", iter->getText("first").c_str());
        wprintz(w_terrain, msgtype_to_color(iter->getMsgType("second"),
                                            (iter->getStep() >= SCT.iMaxSteps / 2)), iter->getText("second").c_str());
    }
}

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
