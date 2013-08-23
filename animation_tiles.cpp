#if (defined SDLTILES)

#include "game.h"
#include "debug.h"
#include "cata_tiles.h" // all animation functions will be pushed out to a cata_tiles function in some manner

extern cata_tiles *tilecontext; // obtained from sdltiles.cpp
extern int fontheight;
extern int fontwidth;

/* Tiles version of Explosion Animation */
void game::draw_explosion(int x, int y, int radius, nc_color col)
{
    timespec ts;    // Timespec for the animation of the explosion
    ts.tv_sec = 0;
    ts.tv_nsec = EXPLOSION_SPEED;
// added offset values to keep from calculating the same value over and over again.
    const int offset_x = VIEWX - u.posx - u.view_offset_x;
    const int offset_y = VIEWY - u.posy - u.view_offset_y;

    for (int i = 1; i <= radius; i++) {
        mvwputch(w_terrain, y - i + offset_y,
                      x - i + offset_x, col, '/');
        mvwputch(w_terrain, y - i + offset_y,
                      x + i + offset_x, col,'\\');
        mvwputch(w_terrain, y + i + offset_y,
                      x - i + offset_x, col,'\\');
        mvwputch(w_terrain, y + i + offset_y,
                      x + i + offset_x, col, '/');
        for (int j = 1 - i; j < 0 + i; j++) {
            mvwputch(w_terrain, y - i + offset_y,
                       x + j + offset_x, col,'-');
            mvwputch(w_terrain, y + i + offset_y,
                       x + j + offset_x, col,'-');
            mvwputch(w_terrain, y + j + offset_y,
                       x - i + offset_x, col,'|');
            mvwputch(w_terrain, y + j + offset_y,
                       x + i + offset_x, col,'|');
        }
        tilecontext->init_explosion(x, y, i);
        //tilecontext->draw_explosion_frame(x, y, radius, offset_x, offset_y);

        wrefresh(w_terrain);
        nanosleep(&ts, NULL);
    }
    tilecontext->void_explosion();
}
/* Bullet Animation -- Maybe change this to animate the ammo itself flying through the air?*/
// need to have a version where there is no player defined, possibly. That way shrapnel works as intended
void game::draw_bullet(player &p, int tx, int ty, int i, std::vector<point> trajectory, char bullet_char, timespec &ts)
{
    if (u_see(tx, ty)) {
        std::string bullet;// = "animation_bullet_normal";
        switch(bullet_char)
        {
            case '*': bullet = "animation_bullet_normal"; break;
            case '#': bullet = "animation_bullet_flame"; break;
            case '`': bullet = "animation_bullet_shrapnel"; break;
        }

        mvwputch(w_terrain, ty + VIEWY - u.posy - u.view_offset_y,
                 tx + VIEWX - u.posx - u.view_offset_x, c_red, bullet_char);
        // pass to tilecontext
        tilecontext->init_draw_bullet(tx, ty, bullet);
        wrefresh(w_terrain);
        if (&p == &u)
         nanosleep(&ts, NULL);
        tilecontext->void_bullet();
   }
}
#endif
