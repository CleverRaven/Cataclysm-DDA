#if (!defined SDLTILES)

// see game.cpp
bool is_valid_in_w_terrain(int x, int y);

#include "game.h"

/* Monster hit animation */
void game::draw_hit_mon(int x, int y, const monster &m, bool dead)
{
    nc_color cMonColor = m.type->color;
    const std::string &sMonSym = m.symbol();

    hit_animation(POSX + (x - (u.posx() + u.view_offset_x)),
                  POSY + (y - (u.posy() + u.view_offset_y)),
                  red_background(cMonColor), dead ? "%" : sMonSym);
}
/* Player hit animation */
void game::draw_hit_player(player *p, const int iDam, bool dead)
{
    (void)dead; //unused
    const std::string &sMonSym = p->symbol();
    hit_animation(POSX + (p->posx() - (u.posx() + u.view_offset_x)),
                  POSY + (p->posy() - (u.posy() + u.view_offset_y)),
                  (iDam == 0) ? yellow_background(p->symbol_color()) : red_background(p->symbol_color()), sMonSym);
}
/* Line drawing code, not really an animation but should be separated anyway */

void game::draw_line(const int x, const int y, const point center_point, std::vector<point> ret)
{
    if (u.sees( x, y)) {
        for( auto &elem : ret ) {
            const Creature *critter = critter_at( elem.x, elem.y );
            // NPCs and monsters get drawn with inverted colors
            if( critter != nullptr && u.sees( *critter ) ) {
                critter->draw( w_terrain, center_point.x, center_point.y, true );
            } else {
                m.drawsq( w_terrain, u, elem.x, elem.y, true, true, center_point.x,
                          center_point.y );
            }
        }
    }
}
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
