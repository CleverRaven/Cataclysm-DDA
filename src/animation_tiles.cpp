#if (defined SDLTILES)

#include "game.h"
#include "debug.h"
#include "cata_tiles.h" // all animation functions will be pushed out to a cata_tiles function in some manner

extern cata_tiles *tilecontext; // obtained from sdltiles.cpp
extern void try_update();

/* Tiles version of Explosion Animation */
void game::draw_explosion(int x, int y, int radius, nc_color col)
{
    timespec ts;    // Timespec for the animation of the explosion
    ts.tv_sec = 0;
    ts.tv_nsec = EXPLOSION_SPEED;
    // added offset values to keep from calculating the same value over and over again.
    const int ypos = POSY + (y - (u.posy + u.view_offset_y));
    const int xpos = POSX + (x - (u.posx + u.view_offset_x));

    for (int i = 1; i <= radius; i++) {
        if (use_tiles) {
            tilecontext->init_explosion(x, y, i);
        } else {
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
        }

        wrefresh(w_terrain);
        try_update();
        nanosleep(&ts, NULL);
    }
    tilecontext->void_explosion();
}
/* Bullet Animation -- Maybe change this to animate the ammo itself flying through the air?*/
// need to have a version where there is no player defined, possibly. That way shrapnel works as intended
void game::draw_bullet(Creature &p, int tx, int ty, int i,
                       std::vector<point> trajectory, char bullet_char, timespec &ts)
{
    (void)i; //unused
    (void)trajectory; //unused
    if (u_see(tx, ty)) {
        std::string bullet;// = "animation_bullet_normal";
        switch(bullet_char) {
        case '*':
            bullet = "animation_bullet_normal";
            break;
        case '#':
            bullet = "animation_bullet_flame";
            break;
        case '`':
            bullet = "animation_bullet_shrapnel";
            break;
        }

        if (use_tiles) {
            tilecontext->init_draw_bullet(tx, ty, bullet);
        } else {
            mvwputch(w_terrain, POSY + (ty - (u.posy + u.view_offset_y)),
                 POSX + (tx - (u.posx + u.view_offset_x)), c_red, bullet_char);
        }
        wrefresh(w_terrain);
        if (p.is_player()) {
            try_update();
            nanosleep(&ts, NULL);
        }
        tilecontext->void_bullet();
    }
}
/* Monster hit animation */
void game::draw_hit_mon(int x, int y, monster m, bool dead)
{
    if (use_tiles) {
        //int iTimeout = 0;
        tilecontext->init_draw_hit(x, y, m.type->id);
        wrefresh(w_terrain);
        try_update();

        timespec tspec;
        tspec.tv_sec = 0;
        tspec.tv_nsec = BULLET_SPEED;

        nanosleep(&tspec, NULL);
    } else {
        nc_color cMonColor = m.type->color;
        char sMonSym = m.symbol();

        hit_animation(POSX + (x - (u.posx + u.view_offset_x)),
                      POSY + (y - (u.posy + u.view_offset_y)),
                      red_background(cMonColor), dead ? '%' : sMonSym);
    }
}
/* Player hit animation */
void game::draw_hit_player(player *p, const int iDam, bool dead)
{
    (void)dead; //unused
    if (use_tiles) {
        // get base name of player id
        std::string pname = (p->is_npc() ? "npc_" : "player_");
        // get sex of player
        pname += (p->male ? "male" : "female");

        tilecontext->init_draw_hit(p->posx, p->posy, pname);
        wrefresh(w_terrain);
        try_update();

        timespec tspec;
        tspec.tv_sec = 0;
        tspec.tv_nsec = BULLET_SPEED;

        nanosleep(&tspec, NULL);
    } else {
        hit_animation(POSX + (p->posx - (u.posx + u.view_offset_x)),
                      POSY + (p->posy - (u.posy + u.view_offset_y)),
                      (iDam == 0) ? yellow_background(p->color()) : red_background(p->color()), '@');
    }
}

/* Line drawing code, not really an animation but should be separated anyway */

void game::draw_line(const int x, const int y, const point center_point, std::vector<point> ret)
{
    if (u_see( x, y)) {
        for (size_t i = 0; i < ret.size(); i++) {
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
    tilecontext->init_draw_line(x, y, ret, "line_target", true);
}

void game::draw_line(const int x, const int y, std::vector<point> vPoint)
{
    int crx = POSX, cry = POSY;

    if(!vPoint.empty()) {
        crx += (vPoint[vPoint.size() - 1].x - (u.posx + u.view_offset_x));
        cry += (vPoint[vPoint.size() - 1].y - (u.posy + u.view_offset_y));
    }
    for (size_t i = 1; i < vPoint.size(); i++) {
        m.drawsq(w_terrain, u, vPoint[i - 1].x, vPoint[i - 1].y, true, true);
    }

    mvwputch(w_terrain, cry, crx, c_white, 'X');

    tilecontext->init_draw_line(x, y, vPoint, "line_trail", false);
}

void game::draw_weather(weather_printable wPrint)
{
    if (use_tiles) {
        std::string weather_name;
        /*
        WEATHER_ACID_DRIZZLE | WEATHER_ACID_RAIN = "weather_acid_drop"
        WEATHER_DRIZZLE | WEATHER_RAINY | WEATHER_THUNDER | WEATHER_LIGHTNING = "weather_rain_drop"
        WEATHER_FLURRIES | WEATHER_SNOW | WEATHER_SNOWSTORM = "weather_snowflake"
        */
        switch(wPrint.wtype) {
            // Acid weathers, uses acid droplet tile, fallthrough intended
            case WEATHER_ACID_DRIZZLE:
            case WEATHER_ACID_RAIN: weather_name = "weather_acid_drop"; break;
            // Normal rainy weathers, uses normal raindrop tile, fallthrough intended
            case WEATHER_DRIZZLE:
            case WEATHER_RAINY:
            case WEATHER_THUNDER:
            case WEATHER_LIGHTNING: weather_name = "weather_rain_drop"; break;
            // Snowy weathers, uses snowflake tile, fallthrough intended
            case WEATHER_FLURRIES:
            case WEATHER_SNOW:
            case WEATHER_SNOWSTORM: weather_name = "weather_snowflake"; break;

            default:
                break;
        }
        /*
        // may have been the culprit of slowdown. Seems to be the same speed now for both weathered and non-weathered display
        for (std::vector<std::pair<int, int> >::iterator weather_iterator = wPrint.vdrops.begin();
             weather_iterator != wPrint.vdrops.end();
             ++weather_iterator)
        {
            mvwputch(w_terrain, weather_iterator->second, weather_iterator->first, wPrint.colGlyph, wPrint.cGlyph);

        }
        */
        tilecontext->init_draw_weather(wPrint, weather_name);
    } else {
        for (std::vector<std::pair<int, int> >::iterator weather_iterator = wPrint.vdrops.begin();
             weather_iterator != wPrint.vdrops.end();
             ++weather_iterator)
        {
            mvwputch(w_terrain, weather_iterator->second, weather_iterator->first, wPrint.colGlyph, wPrint.cGlyph);
        }
    }
}

void game::draw_sct()
{
    if (use_tiles) {
        tilecontext->init_draw_sct();
    } else {
        for (std::vector<scrollingcombattext::cSCT>::iterator iter = SCT.vSCT.begin(); iter != SCT.vSCT.end(); ++iter) {
            const int iDY = POSY + (iter->getPosY() - (u.posy + u.view_offset_y));
            const int iDX = POSX + (iter->getPosX() - (u.posx + u.view_offset_x));

            mvwprintz(w_terrain, iDY, iDX, msgtype_to_color(iter->getMsgType("first"), (iter->getStep() >= SCT.iMaxSteps/2)), "%s", iter->getText("first").c_str());
            wprintz(w_terrain, msgtype_to_color(iter->getMsgType("second"), (iter->getStep() >= SCT.iMaxSteps/2)), iter->getText("second").c_str());
        }
    }
}
#endif
