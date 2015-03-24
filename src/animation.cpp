#include "game.h"
#include "cata_tiles.h" // all animation functions will be pushed out to a cata_tiles function in some manner

extern cata_tiles *tilecontext; // obtained from sdltiles.cpp
extern void try_update();
bool is_valid_in_w_terrain(int x, int y); // see game.cpp

namespace {
//! Get (x, y) relative to u's current position and view
point relative_view_pos(player const &u, int const x, int const y) noexcept
{
    return point { POSX + x - u.posx() - u.view_offset_x,
                   POSY + y - u.posy() - u.view_offset_y };
}

point relative_view_pos(player const &u, point const &p) noexcept
{
    return relative_view_pos(u, p.x, p.y);
}

void draw_animation_delay(long const scale = 1)
{
    const long delay = OPTIONS["ANIMATION_DELAY"] * scale * 1000000l;

    timespec const ts = {0, delay};
    if (ts.tv_nsec > 0) {
        nanosleep(&ts, nullptr);
    }
}

void draw_explosion_curses(game &g, int const x, int const y, int const r, nc_color const col)
{
    point const p = relative_view_pos(g.u, x, y);

    if (r == 0) { // TODO why not always print '*'?
        mvwputch(g.w_terrain, p.x, p.y, col, '*');
    }

    for (int i = 1; i <= r; ++i) {
        mvwputch(g.w_terrain, p.y - i, p.x - i, col, '/');  // corner: top left
        mvwputch(g.w_terrain, p.y - i, p.x + i, col, '\\'); // corner: top right
        mvwputch(g.w_terrain, p.y + i, p.x - i, col, '\\'); // corner: bottom left
        mvwputch(g.w_terrain, p.y + i, p.x + i, col, '/');  // corner: bottom right
        for (int j = 1 - i; j < 0 + i; j++) {
            mvwputch(g.w_terrain, p.y - i, p.x + j, col, '-'); // edge: top
            mvwputch(g.w_terrain, p.y + i, p.x + j, col, '-'); // edge: bottom
            mvwputch(g.w_terrain, p.y + j, p.x - i, col, '|'); // edge: left
            mvwputch(g.w_terrain, p.y + j, p.x + i, col, '|'); // edge: right
        }

        wrefresh(g.w_terrain);
        draw_animation_delay(EXPLOSION_MULTIPLIER);
    }
}
} // namespace

#if defined(SDLTILES)
void game::draw_explosion(int const x, int const y, int const r, nc_color const col)
{
    if (!use_tiles) {
        draw_explosion_curses(*this, x, y, r, col);
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
#else
void game::draw_explosion(int const x, int const y, int const r, nc_color const col)
{
    draw_explosion_curses(*this, x, y, r, col);
}
#endif

namespace {
void draw_bullet_curses(WINDOW *const w, player &u, map &m, int const tx, int const ty,
    char const bullet, point const *const p, bool const wait)
{
    int const vx = u.posx() + u.view_offset_x;
    int const vy = u.posy() + u.view_offset_y;

    if (p) {
        m.drawsq(w, u, p->x, p->y, false, true, vx, vy);
    }

    mvwputch(w, POSY + (ty - vy), POSX + (tx - vx), c_red, bullet);
    wrefresh(w);

    if (wait) {
        draw_animation_delay();
    }
}

} ///namespace

#if defined(SDLTILES)
/* Bullet Animation -- Maybe change this to animate the ammo itself flying through the air?*/
// need to have a version where there is no player defined, possibly. That way shrapnel works as intended
void game::draw_bullet(Creature const &p, int const tx, int const ty, int const i,
    std::vector<point> const &trajectory, char const bullet)
{
    //TODO signature and impl could be changed to eliminate these params

    (void)i;          //unused
    (void)trajectory; //unused

    if (!u.sees(tx, ty)) {
        return;
    }

    if (!use_tiles) {
        draw_bullet_curses(w_terrain, u, m, tx, ty, bullet, nullptr, p.is_player());
        return;
    }

    static std::string const bullet_unknown  {};
    static std::string const bullet_normal   {"animation_bullet_normal"};
    static std::string const bullet_flame    {"animation_bullet_flame"};
    static std::string const bullet_shrapnel {"animation_bullet_shrapnel"};

    std::string const &bullet_type =
        (bullet == '*') ? bullet_normal
      : (bullet == '#') ? bullet_flame
      : (bullet == '`') ? bullet_shrapnel
      : bullet_unknown;

    tilecontext->init_draw_bullet(tx, ty, bullet_type);
    wrefresh(w_terrain);

    if (p.is_player()) {
        try_update();
        draw_animation_delay();
    }

    tilecontext->void_bullet();
}
#else
void game::draw_bullet(Creature const &p, int const tx, int const ty, int const i,
    std::vector<point> const &trajectory, char const bullet)
{
    if (!u.sees(tx, ty)) {
        return;
    }

    draw_bullet_curses(w_terrain, u, m, tx, ty, bullet,
        (i > 0) ? &trajectory[i - 1] : nullptr, p.is_player());
}
#endif

namespace {
void draw_hit_mon_curses(int const x, int const y, const monster &m, player const& u, bool const dead)
{
    point const p = relative_view_pos(u, x, y);
    hit_animation(p.x, p.y, red_background(m.type->color), dead ? "%" : m.symbol());
}

} // namespace

#if defined(SDLTILES)
void game::draw_hit_mon(int const x, int const y, const monster &m, bool const dead)
{
    if (!use_tiles) {
        draw_hit_mon_curses(x, y, m, u, dead);
        return;
    }

    tilecontext->init_draw_hit(x, y, m.type->id);
    wrefresh(w_terrain);
    try_update();
    draw_animation_delay();
}
#else
void game::draw_hit_mon(int const x, int const y, const monster &m, bool const dead)
{
    draw_hit_mon_curses(x, y, m, u, dead);
}
#endif

namespace {
void draw_hit_player_curses(game const& g, player const &p, const int dam)
{
    nc_color const col = (!dam) ? yellow_background(p.symbol_color())
                                : red_background(p.symbol_color());

    point const q = relative_view_pos(g.u, p.pos());
    hit_animation(q.x, q.y, col, p.symbol());
}
} //namespace

#if defined(SDLTILES)
void game::draw_hit_player(player const &p, const int dam)
{
    if (!use_tiles) {
        draw_hit_player_curses(*this, p, dam);
        return;
    }

    static std::string const player_male   {"player_male"};
    static std::string const player_female {"player_female"};
    static std::string const npc_male      {"npc_male"};
    static std::string const npc_female    {"npc_female"};

    std::string const& type = p.is_player() ? (p.male ? player_male : player_female)
                                            : (p.male ? npc_male    : npc_female);

    tilecontext->init_draw_hit(p.posx(), p.posy(), type);
    wrefresh(w_terrain);
    try_update();
    draw_animation_delay();
}
#else
void game::draw_hit_player(player const &p, const int dam)
{
    draw_hit_player_curses(*this, p, dam);
}
#endif

/* Line drawing code, not really an animation but should be separated anyway */
namespace {
void draw_line_curses(game &g, int const x, int const y, point const center,
    std::vector<point> const &ret)
{
    (void)x; // unused
    (void)y; // unused

    for (point const &p : ret) {
        auto const critter = g.critter_at(p.x, p.y);

        // NPCs and monsters get drawn with inverted colors
        if (critter && g.u.sees(*critter)) {
            critter->draw(g.w_terrain, center.x, center.y, true);
        } else {
            g.m.drawsq(g.w_terrain, g.u, p.x, p.y, true, true, center.x, center.y);
        }
    }
}
} //namespace

#if defined(SDLTILES)
void game::draw_line(int const x, int const y, point const center, std::vector<point> const &ret)
{
    if (!u.sees(x, y)) {
        return;
    }

    if (!use_tiles) {
        draw_line_curses(*this, x, y, center, ret); // TODO needed for tiles ver too??
        return;
    }

    tilecontext->init_draw_line(x, y, ret, "line_target", true);
}
#else
void game::draw_line(int const x, int const y, point const center, std::vector<point> const &ret)
{
    if (!u.sees(x, y)) {
        return;
    }

    draw_line_curses(*this, x, y, center, ret);
}
#endif

namespace {
void draw_line_curses(game &g, std::vector<point> const &points)
{
    for (point const& p : points) {
        g.m.drawsq(g.w_terrain, g.u, p.x, p.y, true, true);
    }

    point const p = points.empty() ? point {POSX, POSY} :
        relative_view_pos(g.u, points.back());
    mvwputch(g.w_terrain, p.y, p.x, c_white, 'X');
}
} //namespace

#if defined(SDLTILES)
void game::draw_line(const int x, const int y, std::vector<point> const &vPoint)
{
    draw_line_curses(*this, vPoint);
    tilecontext->init_draw_line(x, y, vPoint, "line_trail", false);
}
#else
void game::draw_line(const int x, const int y, std::vector<point> const &vPoint)
{
    (void)x; //unused
    (void)y; //unused

    draw_line_curses(*this, vPoint);
}
#endif

namespace {
void draw_weather_curses(WINDOW *const win, weather_printable const &w)
{
    for (auto const &drop : w.vdrops) {
        mvwputch(win, drop.second, drop.first, w.colGlyph, w.cGlyph);
    }
}
} //namespace

#if defined(SDLTILES)
void game::draw_weather(weather_printable const &w)
{
    if (!use_tiles) {
        draw_weather_curses(w_terrain, w);
        return;
    }

    static std::string const weather_acid_drop {"weather_acid_drop"};
    static std::string const weather_rain_drop {"weather_rain_drop"};
    static std::string const weather_snowflake {"weather_snowflake"};

    std::string weather_name;
    switch (w.wtype) {
    // Acid weathers; uses acid droplet tile, fallthrough intended
    case WEATHER_ACID_DRIZZLE:
    case WEATHER_ACID_RAIN:
        weather_name = weather_acid_drop;
        break;
    // Normal rainy weathers; uses normal raindrop tile, fallthrough intended
    case WEATHER_DRIZZLE:
    case WEATHER_RAINY:
    case WEATHER_THUNDER:
    case WEATHER_LIGHTNING:
        weather_name = weather_rain_drop;
        break;
    // Snowy weathers; uses snowflake tile, fallthrough intended
    case WEATHER_FLURRIES:
    case WEATHER_SNOW:
    case WEATHER_SNOWSTORM:
        weather_name = weather_snowflake;
        break;
    default:
        break;
    }

    tilecontext->init_draw_weather(w, std::move(weather_name));
}
#else
void game::draw_weather(weather_printable const &w)
{
    draw_weather_curses(w_terrain, w);
}
#endif

namespace {
void draw_sct_curses(game &g)
{
    point const off = relative_view_pos(g.u, 0, 0);

    for (auto const& text : SCT.vSCT) {
        const int dy = off.y + text.getPosY();
        const int dx = off.x + text.getPosX();

        if(!is_valid_in_w_terrain(dx, dy)) {
            continue;
        }

        bool const is_old = text.getStep() >= SCT.iMaxSteps / 2;

        nc_color const col1 = msgtype_to_color(text.getMsgType("first"),  is_old);
        nc_color const col2 = msgtype_to_color(text.getMsgType("second"), is_old);

        mvwprintz(g.w_terrain, dy, dx, col1, "%s", text.getText("first").c_str());
        wprintz(g.w_terrain, col2, "%s", text.getText("second").c_str());
    }
}
} //namespace

#if defined(SDLTILES)
void game::draw_sct()
{
    if (use_tiles) {
        tilecontext->init_draw_sct();
    } else {
        draw_sct_curses(*this);
    }
}
#else
void game::draw_sct()
{
    draw_sct_curses(*this);
}
#endif

namespace {
void draw_zones_curses(WINDOW *const w, point const &beg, point const &end, point const &off)
{
    if (end.x < beg.x || end.y < beg.y) {
        return;
    }

    nc_color    const col = invert_color(c_ltgreen);
    std::string const line(end.x - beg.x + 1, '~');
    int         const x = beg.x - off.x;

    for (int y = beg.y; y <= end.y; ++y) {
        mvwprintz(w, y - off.y, x, col, line.c_str());
    }
}
} //namespace

#if defined(SDLTILES)
void game::draw_zones(point const &beg, point const &end, point const &off)
{
    if (use_tiles) {
        tilecontext->init_draw_zones(beg, end, off);
    } else {
        draw_zones_curses(w_terrain, beg, end, off);
    }
}
#else
void game::draw_zones(point const &beg, point const &end, point const &off)
{
    draw_zones_curses(w_terrain, beg, end, off);
}
#endif
