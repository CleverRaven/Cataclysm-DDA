#if (!defined SDLTILES)

#include "game.h"
/* Explosion Animation */
void game::draw_explosion(int x, int y, int radius, nc_color col)
{
    timespec ts;    // Timespec for the animation of the explosion
    ts.tv_sec = 0;
    ts.tv_nsec = EXPLOSION_SPEED;
    const int ypos = POSY + (y - (u.posy + u.view_offset_y));
    const int xpos = POSX + (x - (u.posx + u.view_offset_x));
    for (int i = 1; i <= radius; i++) {
        mvwputch(w_terrain, ypos - i, xpos - i, col, '/');
        mvwputch(w_terrain, ypos - i, xpos + i, col, '\\');
        mvwputch(w_terrain, ypos + i, xpos - i, col, '\\');
        mvwputch(w_terrain, ypos + i, xpos + i, col, '/');
        for (int j = 1 - i; j < 0 + i; j++) {
            mvwputch(w_terrain, ypos - i, xpos + j, col, '-');
            mvwputch(w_terrain, ypos + i, xpos + j, col, '-');
            mvwputch(w_terrain, ypos + j, xpos - i, col, '|');
            mvwputch(w_terrain, ypos + j, xpos + i, col, '|');
        }
        wrefresh(w_terrain);
        nanosleep(&ts, NULL);
    }
}
/* Bullet Animation */
void game::draw_bullet(Creature& p, int tx, int ty, int i, std::vector<point> trajectory, char bullet, timespec &ts)
{
    if (u_see(tx, ty)) {
        if (i > 0) {
            m.drawsq(w_terrain, u, trajectory[i - 1].x, trajectory[i - 1].y, false,
                     true, u.posx + u.view_offset_x, u.posy + u.view_offset_y);
        }
        /*
        char bullet = '*';
        if (is_aflame)
         bullet = '#';
        */
        mvwputch(w_terrain, POSY + (ty - (u.posy + u.view_offset_y)),
                 POSX + (tx - (u.posx + u.view_offset_x)), c_red, bullet);
        wrefresh(w_terrain);
        if (p.is_player()) {
            nanosleep(&ts, NULL);
        }
    }
}
/* Monster hit animation */
void game::draw_hit_mon(int x, int y, monster m, bool dead)
{
    nc_color cMonColor = m.type->color;
    char sMonSym = m.symbol();

    hit_animation(POSX + (x - (u.posx + u.view_offset_x)),
                  POSY + (y - (u.posy + u.view_offset_y)),
                  red_background(cMonColor), dead ? '%' : sMonSym);
}
/* Player hit animation */
void game::draw_hit_player(player *p, const int iDam, bool dead)
{
    (void)dead; //unused
    hit_animation(POSX + (p->posx - (u.posx + u.view_offset_x)),
                  POSY + (p->posy - (u.posy + u.view_offset_y)),
                  (iDam == 0) ? yellow_background(p->color()) : red_background(p->color()), '@');
}
/* Line drawing code, not really an animation but should be separated anyway */

void game::draw_line(const int x, const int y, const point center_point, std::vector<point> ret)
{
    if (u_see( x, y)) {
        for (unsigned i = 0; i < ret.size(); i++) {
            int mondex = mon_at(ret[i].x, ret[i].y),
                npcdex = npc_at(ret[i].x, ret[i].y);

            // NPCs and monsters get drawn with inverted colors
            if (mondex != -1 && u_see(&(critter_tracker.find(mondex)))) {
                critter_tracker.find(mondex).draw(w_terrain, center_point.x, center_point.y, true);
            } else if (npcdex != -1) {
                active_npc[npcdex]->draw(w_terrain, center_point.x, center_point.y, true);
            } else {
                m.drawsq(w_terrain, u, ret[i].x, ret[i].y, true, true, center_point.x, center_point.y);
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
        crx += (vPoint[vPoint.size() - 1].x - (u.posx + u.view_offset_x));
        cry += (vPoint[vPoint.size() - 1].y - (u.posy + u.view_offset_y));
    }
    for (unsigned i = 1; i < vPoint.size(); i++) {
        m.drawsq(w_terrain, u, vPoint[i - 1].x, vPoint[i - 1].y, true, true);
    }

    mvwputch(w_terrain, cry, crx, c_white, 'X');
}

void game::draw_weather(weather_printable wPrint)
{
    for (std::vector<std::pair<int, int> >::iterator weather_iterator = wPrint.vdrops.begin();
         weather_iterator != wPrint.vdrops.end();
         ++weather_iterator)
    {
        mvwputch(w_terrain, weather_iterator->second, weather_iterator->first, wPrint.colGlyph, wPrint.cGlyph);
    }
}

void game::draw_sct()
{
    for (std::vector<scrollingcombattext::cSCT>::iterator iter = SCT.vSCT.begin(); iter != SCT.vSCT.end(); ++iter) {
        const int iDY = POSY + (iter->getPosY() - (u.posy + u.view_offset_y));
        const int iDX = POSX + (iter->getPosX() - (u.posx + u.view_offset_x));

        mvwprintz(w_terrain, iDY, iDX, msgtype_to_color(iter->getMsgType("first"), (iter->getStep() >= SCT.iMaxSteps/2)), "%s", iter->getText("first").c_str());
        wprintz(w_terrain, msgtype_to_color(iter->getMsgType("second"), (iter->getStep() >= SCT.iMaxSteps/2)), iter->getText("second").c_str());
    }
}

#endif
