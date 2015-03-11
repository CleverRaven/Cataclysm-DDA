#include "game.h"

#if (defined SDLTILES)
#   include "debug.h"
#   include "cata_tiles.h" // all animation functions will be pushed out to a cata_tiles function in some manner

extern cata_tiles *tilecontext; // obtained from sdltiles.cpp
extern void try_update();

#endif

namespace {
void draw_animation_delay(long const scale)
{
    auto const delay = static_cast<long>(OPTIONS["ANIMATION_DELAY"]) * scale * 1000000l;

    timespec const ts = {0, delay};
    if (ts.tv_nsec > 0) {
        nanosleep(&ts, nullptr);
    }
}

void draw_explosion_curses(WINDOW *const w, int const x, int const y,
    int const r, nc_color const col)
{
    if (r == 0) { // TODO why not always print '*'?
        mvwputch(w, x, y, col, '*');
    }

    for (int i = 1; i <= r; ++i) {
        mvwputch(w, y - i, x - i, col, '/');  // corner: top left
        mvwputch(w, y - i, x + i, col, '\\'); // corner: top right
        mvwputch(w, y + i, x - i, col, '\\'); // corner: bottom left
        mvwputch(w, y + i, x + i, col, '/');  // corner: bottom right
        for (int j = 1 - i; j < 0 + i; j++) {
            mvwputch(w, y - i, x + j, col, '-'); // edge: top
            mvwputch(w, y + i, x + j, col, '-'); // edge: bottom
            mvwputch(w, y + j, x - i, col, '|'); // edge: left
            mvwputch(w, y + j, x + i, col, '|'); // edge: right
        }
    }
    
    wrefresh(w);
    draw_animation_delay(EXPLOSION_MULTIPLIER);
}
} // namespace

#if !defined(SDLTILES)
void game::draw_explosion(int const x, int const y, int const r, nc_color const col)
{
    const int ypos = POSY + (y - (u.posy() + u.view_offset_y));
    const int xpos = POSX + (x - (u.posx() + u.view_offset_x));
    draw_explosion_curses(w_terrain, xpos, ypos, r, col);
}
#else
void game::draw_explosion(int const x, int const y, int const r, nc_color const col)
{
    // added offset values to keep from calculating the same value over and over again.
    const int ypos = POSY + (y - (u.posy() + u.view_offset_y));
    const int xpos = POSX + (x - (u.posx() + u.view_offset_x));

    if (!use_tiles) {
        draw_explosion_curses(w_terrain, xpos, ypos, r, col);
        return;
    }

    for (int i = 1; i <= r; i++) {
        tilecontext->init_explosion(x, y, i); // TODO not xpos ypos?
        wrefresh(w_terrain);
        try_update();
        draw_animation_delay(EXPLOSION_MULTIPLIER);
    }

    if (r > 0) {
        tilecontext->void_explosion();
    }
}
#endif

#if (defined SDLTILES)

/* Bullet Animation -- Maybe change this to animate the ammo itself flying through the air?*/
// need to have a version where there is no player defined, possibly. That way shrapnel works as intended
void game::draw_bullet(Creature const &p, int tx, int ty, int i,
                       std::vector<point> const &trajectory, char bullet_char, timespec const &ts)
{
    (void)i; //unused
    (void)trajectory; //unused
    if (u.sees(tx, ty)) {
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
            mvwputch(w_terrain, POSY + (ty - (u.posy() + u.view_offset_y)),
                     POSX + (tx - (u.posx() + u.view_offset_x)), c_red, bullet_char);
        }
        wrefresh(w_terrain);
        if (p.is_player()) {
            try_update();
            if( ts.tv_nsec != 0 ) {
                nanosleep(&ts, NULL);
            }
        }
        tilecontext->void_bullet();
    }
}
/* Monster hit animation */
void game::draw_hit_mon(int x, int y, const monster &m, bool dead)
{
    if (use_tiles) {
        //int iTimeout = 0;
        tilecontext->init_draw_hit(x, y, m.type->id);
        wrefresh(w_terrain);
        try_update();

        timespec tspec;
        tspec.tv_sec = 0;
        tspec.tv_nsec = 1000000 * OPTIONS["ANIMATION_DELAY"];

        if( tspec.tv_nsec != 0 ) {
            nanosleep(&tspec, NULL);
        }
    } else {
        nc_color cMonColor = m.type->color;
        const std::string &sMonSym = m.symbol();

        hit_animation(POSX + (x - (u.posx() + u.view_offset_x)),
                      POSY + (y - (u.posy() + u.view_offset_y)),
                      red_background(cMonColor), dead ? "%" : sMonSym);
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

        tilecontext->init_draw_hit(p->posx(), p->posy(), pname);
        wrefresh(w_terrain);
        try_update();

        timespec tspec;
        tspec.tv_sec = 0;
        tspec.tv_nsec = 1000000 * OPTIONS["ANIMATION_DELAY"];

        if( tspec.tv_nsec != 0 ) {
            nanosleep(&tspec, NULL);
        }
    } else {
        hit_animation(POSX + (p->posx() - (u.posx() + u.view_offset_x)),
                      POSY + (p->posy() - (u.posy() + u.view_offset_y)),
                      (iDam == 0) ? yellow_background(p->symbol_color()) : red_background(p->symbol_color()), "@");
    }
}

/* Line drawing code, not really an animation but should be separated anyway */

void game::draw_line(const int x, const int y, const point center_point, std::vector<point> ret)
{
    if (u.sees( x, y)) {
        for (std::vector<point>::iterator it = ret.begin();
             it != ret.end(); it++) {
            const Creature *critter = critter_at( it->x, it->y );
            // NPCs and monsters get drawn with inverted colors
            if( critter != nullptr && u.sees( *critter ) ) {
                critter->draw( w_terrain, center_point.x, center_point.y, true );
            } else {
                m.drawsq(w_terrain, u, it->x, it->y, true, true, center_point.x, center_point.y);
            }
        }
    }
    tilecontext->init_draw_line(x, y, ret, "line_target", true);
}

void game::draw_line(const int x, const int y, std::vector<point> vPoint)
{
    int crx = POSX, cry = POSY;

    if(!vPoint.empty()) {
        crx += (vPoint[vPoint.size() - 1].x - (u.posx() + u.view_offset_x));
        cry += (vPoint[vPoint.size() - 1].y - (u.posy() + u.view_offset_y));
    }
    for( std::vector<point>::iterator it = vPoint.begin(); it != vPoint.end() - 1; it++ ) {
        m.drawsq(w_terrain, u, it->x, it->y, true, true);
    }

    mvwputch(w_terrain, cry, crx, c_white, 'X');

    tilecontext->init_draw_line(x, y, vPoint, "line_trail", false);
}

void game::draw_weather(weather_printable wPrint)
{
    if (use_tiles) {
        std::string weather_name;

        switch(wPrint.wtype) {
        // Acid weathers, uses acid droplet tile, fallthrough intended
        case WEATHER_ACID_DRIZZLE:
        case WEATHER_ACID_RAIN:
            weather_name = "weather_acid_drop";
            break;

        // Normal rainy weathers, uses normal raindrop tile, fallthrough intended
        case WEATHER_DRIZZLE:
        case WEATHER_RAINY:
        case WEATHER_THUNDER:
        case WEATHER_LIGHTNING:
            weather_name = "weather_rain_drop";
            break;

        // Snowy weathers, uses snowflake tile, fallthrough intended
        case WEATHER_FLURRIES:
        case WEATHER_SNOW:
        case WEATHER_SNOWSTORM:
            weather_name = "weather_snowflake";
            break;

        default:
            break;
        }

        tilecontext->init_draw_weather(wPrint, weather_name);
    } else {
        for (std::vector<std::pair<int, int> >::iterator weather_iterator = wPrint.vdrops.begin();
             weather_iterator != wPrint.vdrops.end();
             ++weather_iterator) {
            mvwputch(w_terrain, weather_iterator->second, weather_iterator->first, wPrint.colGlyph,
                     wPrint.cGlyph);
        }
    }
}

void game::draw_sct()
{
    if (use_tiles) {
        tilecontext->init_draw_sct();
    } else {
        for (std::vector<scrollingcombattext::cSCT>::iterator iter = SCT.vSCT.begin();
             iter != SCT.vSCT.end(); ++iter) {
            const int iDY = POSY + (iter->getPosY() - (u.posy() + u.view_offset_y));
            const int iDX = POSX + (iter->getPosX() - (u.posx() + u.view_offset_x));

            mvwprintz(w_terrain, iDY, iDX, msgtype_to_color(iter->getMsgType("first"),
                      (iter->getStep() >= SCT.iMaxSteps / 2)), "%s", iter->getText("first").c_str());
            wprintz(w_terrain, msgtype_to_color(iter->getMsgType("second"),
                                                (iter->getStep() >= SCT.iMaxSteps / 2)), iter->getText("second").c_str());
        }
    }
}

void game::draw_zones(const point &p_pointStart, const point &p_pointEnd,
                      const point &p_pointOffset)
{
    if (use_tiles) {
        tilecontext->init_draw_zones(p_pointStart, p_pointEnd, p_pointOffset);
    } else {
        for (int iY = p_pointStart.y; iY <= p_pointEnd.y; ++iY) {
            for (int iX = p_pointStart.x; iX <= p_pointEnd.x; ++iX) {
                mvwputch_inv(w_terrain, iY + p_pointOffset.y, iX + p_pointOffset.x, c_ltgreen, '~');
            }
        }
    }
}
#endif
