#include "animation.h"
#include "game.h"
#include "map.h"
#include "options.h"
#include "output.h"
#include "monster.h"
#include "mtype.h"
#include "weather.h"
#include "player.h"
#ifdef TILES
#include "cata_tiles.h" // all animation functions will be pushed out to a cata_tiles function in some manner

#include <memory>

extern std::unique_ptr<cata_tiles> tilecontext; // obtained from sdltiles.cpp
#endif

#include <algorithm>
#include <array>

bool is_valid_in_w_terrain(int x, int y); // see game.cpp

namespace {

class basic_animation
{
public:
    basic_animation( long const scale ) :
        delay{ 0, get_option<int>( "ANIMATION_DELAY" ) * scale * 1000000l }
    {
    }

    void draw() const {
        auto window = create_wait_popup_window( _( "Hang on a bit..." ) );

        wrefresh( g->w_terrain );
        wrefresh( window );

        refresh_display();
    }

    void progress() const {
        draw();

        if( delay.tv_nsec > 0 ) {
            nanosleep( &delay, nullptr );
        }
    }

private:
    timespec delay;
};

class explosion_animation : public basic_animation
{
public:
    explosion_animation() :
        basic_animation( EXPLOSION_MULTIPLIER )
    {
    }
};

class bullet_animation : public basic_animation
{
public:
    bullet_animation() : basic_animation( 1 )
    {
    }
};


bool is_point_visible( const tripoint &p, int margin = 0 )
{
    return g->is_in_viewport( p, margin ) && g->u.sees( p );
}

bool is_radius_visible( const tripoint &center, int radius )
{
    return is_point_visible( center, -radius );
}

bool is_layer_visible( const std::map<tripoint, explosion_tile> &layer )
{
    return std::any_of( layer.begin(), layer.end(),
    [](const std::pair<tripoint, explosion_tile>& element) {
        return is_point_visible( element.first );
    } );
}


//! Get (x, y) relative to u's current position and view
tripoint relative_view_pos( player const &u, int const x, int const y, int const z ) noexcept
{
    return tripoint { POSX + x - u.posx() - u.view_offset.x,
            POSY + y - u.posy() - u.view_offset.y,
            z - u.posz() - u.view_offset.z };
}

tripoint relative_view_pos( player const &u, tripoint const &p ) noexcept
{
    return relative_view_pos( u, p.x, p.y, p.z );
}

void draw_explosion_curses(game &g, const tripoint &center, int const r, nc_color const col)
{
    if( !is_radius_visible( center, r ) ) {
        return;
    }
    // TODO: Make it look different from above/below
    tripoint const p = relative_view_pos( g.u, center );

    if (r == 0) { // TODO why not always print '*'?
        mvwputch(g.w_terrain, p.x, p.y, col, '*');
    }

    explosion_animation anim;

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

        anim.progress();
    }
}

constexpr explosion_neighbors operator | ( explosion_neighbors lhs, explosion_neighbors rhs )
{
    return static_cast<explosion_neighbors>( static_cast< int >( lhs ) | static_cast< int >( rhs ) );
}

constexpr explosion_neighbors operator ^ ( explosion_neighbors lhs, explosion_neighbors rhs )
{
    return static_cast<explosion_neighbors>( static_cast< int >( lhs ) ^ static_cast< int >( rhs ) );
}

void draw_custom_explosion_curses( game &g,
                                   const std::list< std::map<tripoint, explosion_tile> > &layers )
{
    // calculate screen offset relative to player + view offset position
    const tripoint center = g.u.pos() + g.u.view_offset;
    const tripoint topleft( center.x - getmaxx( g.w_terrain ) / 2, center.y - getmaxy( g.w_terrain ) / 2, 0 );

    explosion_animation anim;

    for( const auto &layer : layers ) {
        for( const auto &pr : layer ) {
            // update tripoint in relation to top left corner of curses window
            // mvwputch already filters out of bounds coordinates
            const tripoint p = pr.first - topleft;
            const explosion_neighbors ngh = pr.second.neighborhood;
            const nc_color col = pr.second.color;

            switch( ngh ) {
                // '^', 'v', '<', '>'
                case N_NORTH:
                    mvwputch( g.w_terrain, p.y, p.x, col, '^' );
                    break;
                case N_SOUTH:
                    mvwputch( g.w_terrain, p.y, p.x, col, 'v' );
                    break;
                case N_WEST:
                    mvwputch( g.w_terrain, p.y, p.x, col, '<' );
                    break;
                case N_EAST:
                    mvwputch( g.w_terrain, p.y, p.x, col, '>' );
                    break;
                // '|' and '-'
                case N_NORTH | N_SOUTH:
                case N_NORTH | N_SOUTH | N_WEST:
                case N_NORTH | N_SOUTH | N_EAST:
                    mvwputch( g.w_terrain, p.y, p.x, col, '|' );
                    break;
                case N_WEST | N_EAST:
                case N_WEST | N_EAST | N_NORTH:
                case N_WEST | N_EAST | N_SOUTH:
                    mvwputch( g.w_terrain, p.y, p.x, col, '-' );
                    break;
                // '/' and '\'
                case N_NORTH | N_WEST:
                case N_SOUTH | N_EAST:
                    mvwputch( g.w_terrain, p.y, p.x, col, '/' );
                    break;
                case N_SOUTH | N_WEST:
                case N_NORTH | N_EAST:
                    mvwputch( g.w_terrain, p.y, p.x, col, '\\' );
                    break;
                case N_NO_NEIGHBORS:
                    mvwputch( g.w_terrain, p.y, p.x, col, '*' );
                    break;
                case N_WEST | N_EAST | N_NORTH | N_SOUTH:
                    break;
            }
        }

        if( is_layer_visible( layer ) ) {
            anim.progress();
        }
    }
}
} // namespace

#if defined(TILES)
void game::draw_explosion( const tripoint &p, int const r, nc_color const col )
{
    if( test_mode ) {
        return; // avoid segfault
    }

    if (!use_tiles) {
        draw_explosion_curses(*this, p, r, col);
        return;
    }

    if( !is_radius_visible( p, r ) ) {
        return;
    }

    explosion_animation anim;

    bool visible = is_radius_visible( p, r );
    for (int i = 1; i <= r; i++) {
        tilecontext->init_explosion( p, i ); // TODO not xpos ypos?
        if (visible) {
            anim.progress();
        }
    }

    if (r > 0) {
        tilecontext->void_explosion();
    }
}
#else
void game::draw_explosion( const tripoint &p, int const r, nc_color const col )
{
    draw_explosion_curses(*this, p, r, col);
}
#endif

void game::draw_custom_explosion( const tripoint &, const std::map<tripoint, nc_color> &all_area )
{
    if( test_mode ) {
        return; // avoid segfault
    }

    constexpr explosion_neighbors all_neighbors = N_NORTH | N_SOUTH | N_WEST | N_EAST;
    // We will "shell" the explosion area
    // Each phase will strip a single layer of points
    // A layer contains all points that have less than 4 neighbors in cardinal directions
    // Layers will first be generated, then drawn in inverse order

    // Start by getting rid of everything except current z-level
    std::map<tripoint, explosion_tile> neighbors;
#if defined(TILES)
    if( !use_tiles ) {
        for( const auto &pr : all_area ) {
            const tripoint relative_point = relative_view_pos( u, pr.first );
            if( relative_point.z == 0 ) {
                neighbors[pr.first] = explosion_tile{ N_NO_NEIGHBORS, pr.second };
            }
        }
    } else {
        // In tiles mode, the coordinates have to be absolute
        const tripoint view_center = relative_view_pos( u, u.pos() );
        for( const auto &pr : all_area ) {
            // Relative point is only used for z level check
            const tripoint relative_point = relative_view_pos( u, pr.first );
            if( relative_point.z == view_center.z ) {
                neighbors[pr.first] = explosion_tile{ N_NO_NEIGHBORS, pr.second };
            }
        }
    }
#else
    for( const auto &pr : all_area ) {
        const tripoint relative_point = relative_view_pos( u, pr.first );
        if( relative_point.z == 0 ) {
            neighbors[pr.first] = explosion_tile{ N_NO_NEIGHBORS, pr.second };
        }
    }
#endif

    // Searches for a neighbor, sets the neighborhood flag on current point and on the neighbor
    const auto set_neighbors = [&]( const tripoint &pos,
                                    explosion_neighbors &ngh,
                                    explosion_neighbors here,
                                    explosion_neighbors there ) {
        if( ( ngh & here ) == N_NO_NEIGHBORS ) {
            auto other = neighbors.find( pos );
            if( other != neighbors.end() ) {
                ngh = ngh | here;
                other->second.neighborhood = other->second.neighborhood | there;
            }
        }
    };

    // If the point we are about to remove has a neighbor in a given direction
    // unset that neighbor's flag that our current point is its neighbor
    const auto unset_neighbor = [&]( const tripoint &pos,
                                     const explosion_neighbors ngh,
                                     explosion_neighbors here,
                                     explosion_neighbors there ) {
        if( ( ngh & here ) != N_NO_NEIGHBORS ) {
            auto other = neighbors.find( pos );
            if( other != neighbors.end() ) {
                other->second.neighborhood = ( other->second.neighborhood | there ) ^ there;
            }
        }
    };

    // Find all neighborhoods
    for( auto &pr : neighbors ) {
        const tripoint &pt = pr.first;
        explosion_neighbors &ngh = pr.second.neighborhood;

        set_neighbors( tripoint( pt.x - 1, pt.y, pt.z ), ngh, N_WEST, N_EAST );
        set_neighbors( tripoint( pt.x + 1, pt.y, pt.z ), ngh, N_EAST, N_WEST );
        set_neighbors( tripoint( pt.x, pt.y - 1, pt.z ), ngh, N_NORTH, N_SOUTH );
        set_neighbors( tripoint( pt.x, pt.y + 1, pt.z ), ngh, N_SOUTH, N_NORTH );
    }

    // We need to save the layers because we will draw them in reverse order
    std::list< std::map<tripoint, explosion_tile> > layers;
    bool changed;
    while( !neighbors.empty() ) {
        std::map<tripoint, explosion_tile> layer;
        changed = false;
        // Find a layer that can be drawn
        for( const auto &pr : neighbors ) {
            if( pr.second.neighborhood != all_neighbors ) {
                changed = true;
                layer.insert( pr );
            }
        }
        if( !changed ) {
            // An error, but a minor one - let it slide
            return;
        }
        // Remove the layer from the area to process
        for( const auto &pr : layer ) {
            const tripoint &pt = pr.first;
            const explosion_neighbors ngh = pr.second.neighborhood;

            unset_neighbor( tripoint( pt.x - 1, pt.y, pt.z ), ngh, N_WEST, N_EAST );
            unset_neighbor( tripoint( pt.x + 1, pt.y, pt.z ), ngh, N_EAST, N_WEST );
            unset_neighbor( tripoint( pt.x, pt.y - 1, pt.z ), ngh, N_NORTH, N_SOUTH );
            unset_neighbor( tripoint( pt.x, pt.y + 1, pt.z ), ngh, N_SOUTH, N_NORTH );
            neighbors.erase( pr.first );
        }

        layers.push_front( std::move( layer ) );
    }

#if defined(TILES)
    if( !use_tiles ) {
        draw_custom_explosion_curses( *this, layers );
        return;
    }

    explosion_animation anim;
    // We need to draw all explosions up to now
    std::map<tripoint, explosion_tile> combined_layer;
    for( const auto &layer : layers ) {
        combined_layer.insert( layer.begin(), layer.end() );
        tilecontext->init_custom_explosion_layer( combined_layer );
        if( is_layer_visible( layer ) ) {
            anim.progress();
        }
    }

    tilecontext->void_custom_explosion();
#else
    draw_custom_explosion_curses( *this, layers );
#endif
}

namespace {

void draw_bullet_curses(map &m, const tripoint &t, char const bullet, const tripoint *const p)
{
    if ( !is_point_visible( t ) ) {
        return;
    }

    const tripoint vp = g->u.pos() + g->u.view_offset;

    if( p != nullptr && p->z == vp.z ) {
        m.drawsq( g->w_terrain, g->u, *p, false, true, vp );
    }

    if( vp.z != t.z ) {
        return;
    }

    mvwputch(g->w_terrain, POSY + (t.y - vp.y), POSX + (t.x - vp.x), c_red, bullet);
    bullet_animation().progress();
}

} ///namespace

#if defined(TILES)
/* Bullet Animation -- Maybe change this to animate the ammo itself flying through the air?*/
// need to have a version where there is no player defined, possibly. That way shrapnel works as intended
void game::draw_bullet(const tripoint &t, int const i, const std::vector<tripoint> &trajectory, char const bullet)
{
    //TODO signature and impl could be changed to eliminate these params

    (void)i;          //unused
    (void)trajectory; //unused

    if (!use_tiles) {
        draw_bullet_curses(m, t, bullet, nullptr);
        return;
    }

    if( !is_point_visible( t ) ) {
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

    tilecontext->init_draw_bullet( t, bullet_type );
    bullet_animation().progress();
    tilecontext->void_bullet();
}
#else
void game::draw_bullet(const tripoint &t, int const i, const std::vector<tripoint> &trajectory, char const bullet)
{
    draw_bullet_curses( m, t, bullet, &trajectory[i] );
}
#endif

namespace {
void draw_hit_mon_curses( const tripoint &center, const monster &m, player const& u, bool const dead )
{
    tripoint const p = relative_view_pos( u, center );
    hit_animation( p.x, p.y, red_background(m.type->color), dead ? "%" : m.symbol());
}

} // namespace

#if defined(TILES)
void game::draw_hit_mon( const tripoint &p, const monster &m, bool const dead )
{
    if (!use_tiles) {
        draw_hit_mon_curses( p, m, u, dead );
        return;
    }

    tilecontext->init_draw_hit( p, m.type->id.str() );

    bullet_animation().progress();
}
#else
void game::draw_hit_mon( const tripoint &p, const monster &m, bool const dead )
{
    draw_hit_mon_curses( p, m, u, dead );
}
#endif

namespace {
void draw_hit_player_curses(game const& g, player const &p, const int dam)
{
    tripoint const q = relative_view_pos( g.u, p.pos() );
    if( q.z == 0 ) {
        nc_color const col = !dam ? yellow_background( p.symbol_color() ) : red_background( p.symbol_color() );
        hit_animation( q.x, q.y, col, p.symbol() );
    }
}
} //namespace

#if defined(TILES)
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
    tilecontext->init_draw_hit( p.pos(), type );
    bullet_animation().progress();
}
#else
void game::draw_hit_player(player const &p, const int dam)
{
    draw_hit_player_curses(*this, p, dam);
}
#endif

/* Line drawing code, not really an animation but should be separated anyway */
namespace {
void draw_line_curses(game &g, const tripoint &pos, tripoint const &center,
    std::vector<tripoint> const &ret)
{
    (void)pos; // unused

    for( tripoint const &p : ret ) {
        auto const critter = g.critter_at( p, true );

        // NPCs and monsters get drawn with inverted colors
        if( critter && g.u.sees( *critter ) ) {
            critter->draw( g.w_terrain, center, true );
        } else {
            g.m.drawsq( g.w_terrain, g.u, p, true, true, center );
        }
    }
}
} //namespace

#if defined(TILES)
void game::draw_line( const tripoint &p, tripoint const &center, std::vector<tripoint> const &ret )
{
    if( !u.sees( p ) ) {
        return;
    }

    if (!use_tiles) {
        draw_line_curses(*this, p, center, ret); // TODO needed for tiles ver too??
        return;
    }

    tilecontext->init_draw_line( p, ret, "line_target", true );
}
#else
void game::draw_line( const tripoint &p, tripoint const &center, std::vector<tripoint> const &ret )
{
    if( !u.sees( p ) ) {
        return;
    }

    draw_line_curses( *this, p, center, ret );
}
#endif

namespace {
void draw_line_curses(game &g, std::vector<tripoint> const &points)
{
    for( tripoint const& p : points ) {
        g.m.drawsq(g.w_terrain, g.u, p, true, true);
    }

    tripoint const p = points.empty() ? tripoint {POSX, POSY, 0} :
        relative_view_pos(g.u, points.back());
    mvwputch(g.w_terrain, p.y, p.x, c_white, 'X');
}
} //namespace

#if defined(TILES)
void game::draw_line( const tripoint &p, std::vector<tripoint> const &vPoint )
{
    draw_line_curses(*this, vPoint);
    tilecontext->init_draw_line( p, vPoint, "line_trail", false);
}
#else
void game::draw_line( const tripoint &p, std::vector<tripoint> const &vPoint )
{
    (void)p; //unused

    draw_line_curses(*this, vPoint);
}
#endif

namespace {
void draw_weather_curses( const catacurses::window &win, weather_printable const &w )
{
    for (auto const &drop : w.vdrops) {
        mvwputch(win, drop.second, drop.first, w.colGlyph, w.cGlyph);
    }
}
} //namespace

#if defined(TILES)
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
    tripoint const off = relative_view_pos(g.u, 0, 0, 0);

    for (auto const& text : SCT.vSCT) {
        const int dy = off.y + text.getPosY();
        const int dx = off.x + text.getPosX();

        if(!is_valid_in_w_terrain(dx, dy)) {
            continue;
        }

        bool const is_old = text.getStep() >= SCT.iMaxSteps / 2;

        nc_color const col1 = msgtype_to_color(text.getMsgType("first"),  is_old);
        nc_color const col2 = msgtype_to_color(text.getMsgType("second"), is_old);

        mvwprintz( g.w_terrain, dy, dx, col1, text.getText( "first" ) );
        wprintz( g.w_terrain, col2, text.getText( "second" ) );
    }
}
} //namespace

#if defined(TILES)
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
void draw_zones_curses( const catacurses::window &w, const tripoint &start, const tripoint &end, const tripoint &offset )
{
    if( end.x < start.x || end.y < start.y || end.z < start.z ) {
        return;
    }

    nc_color    const col = invert_color( c_light_green );
    std::string const line( end.x - start.x + 1, '~' );
    int         const x = start.x - offset.x;

    for( int y = start.y; y <= end.y; ++y ) {
        mvwprintz( w, y - offset.y, x, col, line.c_str() );
    }
}
} //namespace

#if defined(TILES)
void game::draw_zones( const tripoint &start, const tripoint &end, const tripoint &offset )
{
    if( use_tiles ) {
        tilecontext->init_draw_zones( start, end, offset );
    } else {
        draw_zones_curses( w_terrain, start, end, offset );
    }
}
#else
void game::draw_zones( const tripoint &start, const tripoint &end, const tripoint &offset )
{
    draw_zones_curses( w_terrain, start, end, offset );
}
#endif
