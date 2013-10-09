#if (!defined SDLTILES)

#include "game.h"
/* Explosion Animation */
void game::draw_explosion(int x, int y, int radius, nc_color col)
{
    timespec ts;    // Timespec for the animation of the explosion
    ts.tv_sec = 0;
    ts.tv_nsec = EXPLOSION_SPEED;
    for (int i = 1; i <= radius; i++) {
        mvwputch(w_terrain, y - i + VIEWY - u.posy - u.view_offset_y,
                      x - i + VIEWX - u.posx - u.view_offset_x, col, '/');
        mvwputch(w_terrain, y - i + VIEWY - u.posy - u.view_offset_y,
                      x + i + VIEWX - u.posx - u.view_offset_x, col,'\\');
        mvwputch(w_terrain, y + i + VIEWY - u.posy - u.view_offset_y,
                      x - i + VIEWX - u.posx - u.view_offset_x, col,'\\');
        mvwputch(w_terrain, y + i + VIEWY - u.posy - u.view_offset_y,
                      x + i + VIEWX - u.posx - u.view_offset_x, col, '/');
        for (int j = 1 - i; j < 0 + i; j++) {
            mvwputch(w_terrain, y - i + VIEWY - u.posy - u.view_offset_y,
                       x + j + VIEWX - u.posx - u.view_offset_x, col,'-');
            mvwputch(w_terrain, y + i + VIEWY - u.posy - u.view_offset_y,
                       x + j + VIEWX - u.posx - u.view_offset_x, col,'-');
            mvwputch(w_terrain, y + j + VIEWY - u.posy - u.view_offset_y,
                       x - i + VIEWX - u.posx - u.view_offset_x, col,'|');
            mvwputch(w_terrain, y + j + VIEWY - u.posy - u.view_offset_y,
                       x + i + VIEWX - u.posx - u.view_offset_x, col,'|');
        }
        wrefresh(w_terrain);
        nanosleep(&ts, NULL);
    }
}
/* Bullet Animation */
void game::draw_bullet(player &p, int tx, int ty, int i, std::vector<point> trajectory, char bullet, timespec &ts)
{
    if (u_see(tx, ty)) {
        if (i > 0)
        {
            m.drawsq(w_terrain, u, trajectory[i-1].x, trajectory[i-1].y, false,
                     true, u.posx + u.view_offset_x, u.posy + u.view_offset_y);
        }
        /*
        char bullet = '*';
        if (is_aflame)
         bullet = '#';
        */
        mvwputch(w_terrain, ty + VIEWY - u.posy - u.view_offset_y,
                 tx + VIEWX - u.posx - u.view_offset_x, c_red, bullet);
        wrefresh(w_terrain);
        if (&p == &u)
         nanosleep(&ts, NULL);
   }
}
/* Monster hit animation */
void game::draw_hit_mon(int x, int y, monster m, bool dead)
{
    nc_color cMonColor = m.type->color;
    char sMonSym = m.symbol();

    hit_animation(x + VIEWX - u.posx - u.view_offset_x,
                  y + VIEWY - u.posy - u.view_offset_y,
                  red_background(cMonColor), dead?'%':sMonSym);
}
/* Player hit animation */
void game::draw_hit_player(player *p, bool dead)
{
    hit_animation(p->posx + VIEWX - u.posx - u.view_offset_x,
                  p->posy + VIEWY - u.posy - u.view_offset_y,
                  red_background(p->color()), '@');
}
/* Line drawing code, not really an animation but should be separated anyway */

void game::draw_line(const int x, const int y, const point center_point, std::vector<point> ret)
{
    if (u_see( x, y))
    {
        for (int i = 0; i < ret.size(); i++)
        {
            int mondex = mon_at(ret[i].x, ret[i].y),
            npcdex = npc_at(ret[i].x, ret[i].y);

            // NPCs and monsters get drawn with inverted colors
            if (mondex != -1 && u_see(&(_active_monsters[mondex])))
            {
                _active_monsters[mondex].draw(w_terrain, center_point.x, center_point.y, true);
            }
            else if (npcdex != -1)
            {
                active_npc[npcdex]->draw(w_terrain, center_point.x, center_point.y, true);
            }
            else
            {
                m.drawsq(w_terrain, u, ret[i].x, ret[i].y, true,true,center_point.x, center_point.y);
            }
        }
    }
}
void game::draw_line(const int x, const int y, std::vector<point> vPoint)
{
    for (int i = 1; i < vPoint.size(); i++)
    {
        m.drawsq(w_terrain, u, vPoint[i-1].x, vPoint[i-1].y, true, true);
    }

    mvwputch(w_terrain, vPoint[vPoint.size()-1].y + VIEWY - u.posy - u.view_offset_y,
                        vPoint[vPoint.size()-1].x + VIEWX - u.posx - u.view_offset_x, c_white, 'X');
}
//*/
void game::draw_weather(weather_printable wPrint)
{
    for (std::vector<std::pair<int, int> >::iterator weather_iterator = wPrint.vdrops.begin();
         weather_iterator != wPrint.vdrops.end();
         ++weather_iterator)
    {
        mvwputch(w_terrain, weather_iterator->second, weather_iterator->first, wPrint.colGlyph, wPrint.cGlyph);
    }

    //mvwputch(w_terrain, iRandY, iRandX, colGlyph, cGlyph);
}
#endif
