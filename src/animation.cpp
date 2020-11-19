#include "animation.h"

#include "avatar.h"
#include "character.h"
#include "creature.h"
#include "cursesdef.h"
#include "explosion.h"
#include "game.h"
#include "game_constants.h"
#include "map.h"
#include "monster.h"
#include "mtype.h"
#include "options.h"
#include "output.h"
#include "player.h"
#include "point.h"
#include "popup.h"
#include "posix_time.h"
#include "translations.h"
#include "type_id.h"
#include "ui_manager.h"
#include "weather.h"

#if defined(TILES)
#include <memory>

#include "cata_tiles.h" // all animation functions will be pushed out to a cata_tiles function in some manner
#include "sdltiles.h"
#endif

#include <algorithm>
#include <list>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace
{

class basic_animation
{
    public:
        basic_animation( const int scale ) :
            delay{ 0, get_option<int>( "ANIMATION_DELAY" ) * scale * 1000000L } {
        }

        void draw() const {
            wrefresh( g->w_terrain );
            g->draw_panels();

            query_popup()
            .wait_message( "%s", _( "Hang on a bit…" ) )
            .on_top( true )
            .show();

            catacurses::refresh();
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
            basic_animation( EXPLOSION_MULTIPLIER ) {
        }
};

class bullet_animation : public basic_animation
{
    public:
        bullet_animation() : basic_animation( 1 ) {
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
    []( const std::pair<tripoint, explosion_tile> &element ) {
        return is_point_visible( element.first );
    } );
}

//! Get (x, y) relative to u's current position and view
tripoint relative_view_pos( const player &u, const int x, const int y, const int z ) noexcept
{
    return -u.view_offset + tripoint( POSX + x - u.posx(), POSY + y - u.posy(), z - u.posz() );
}

tripoint relative_view_pos( const player &u, const tripoint &p ) noexcept
{
    return relative_view_pos( u, p.x, p.y, p.z );
}

// Convert p to screen position relative to the current terrain view
tripoint relative_view_pos( const game &g, const tripoint &p ) noexcept
{
    return p - g.ter_view_p + point( POSX, POSY );
}

void draw_explosion_curses( const game &g, const tripoint &center, const int r,
                            const nc_color &col )
{
    if( !is_radius_visible( center, r ) ) {
        return;
    }
    // TODO: Make it look different from above/below
    const tripoint p = relative_view_pos( g.u, center );

    // TODO: why not always print '*'?
    if( r == 0 ) {
        mvwputch( g.w_terrain, point( p.y, p.x ), col, '*' );
    }

    explosion_animation anim;

    for( int i = 1; i <= r; ++i ) {
        // corner: top left
        mvwputch( g.w_terrain, p.xy() + point( -i, -i ), col, '/' );
        // corner: top right
        mvwputch( g.w_terrain, p.xy() + point( i, -i ), col, '\\' );
        // corner: bottom left
        mvwputch( g.w_terrain, p.xy() + point( -i, i ), col, '\\' );
        // corner: bottom right
        mvwputch( g.w_terrain, p.xy() + point( i, i ), col, '/' );
        for( int j = 1 - i; j < 0 + i; j++ ) {
            // edge: top
            mvwputch( g.w_terrain, p.xy() + point( j, -i ), col, '-' );
            // edge: bottom
            mvwputch( g.w_terrain, p.xy() + point( j, i ), col, '-' );
            // edge: left
            mvwputch( g.w_terrain, p.xy() + point( -i, j ), col, '|' );
            // edge: right
            mvwputch( g.w_terrain, p.xy() + point( i, j ), col, '|' );
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

void draw_custom_explosion_curses( const game &g,
                                   const std::list< std::map<tripoint, explosion_tile> > &layers )
{
    // calculate screen offset relative to player + view offset position
    const tripoint center = g.u.pos() + g.u.view_offset;
    const tripoint topleft( center.x - getmaxx( g.w_terrain ) / 2,
                            center.y - getmaxy( g.w_terrain ) / 2, 0 );

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
                    mvwputch( g.w_terrain, p.xy(), col, '^' );
                    break;
                case N_SOUTH:
                    mvwputch( g.w_terrain, p.xy(), col, 'v' );
                    break;
                case N_WEST:
                    mvwputch( g.w_terrain, p.xy(), col, '<' );
                    break;
                case N_EAST:
                    mvwputch( g.w_terrain, p.xy(), col, '>' );
                    break;
                // '|' and '-'
                case N_NORTH | N_SOUTH:
                case N_NORTH | N_SOUTH | N_WEST:
                case N_NORTH | N_SOUTH | N_EAST:
                    mvwputch( g.w_terrain, p.xy(), col, '|' );
                    break;
                case N_WEST | N_EAST:
                case N_WEST | N_EAST | N_NORTH:
                case N_WEST | N_EAST | N_SOUTH:
                    mvwputch( g.w_terrain, p.xy(), col, '-' );
                    break;
                // '/' and '\'
                case N_NORTH | N_WEST:
                case N_SOUTH | N_EAST:
                    mvwputch( g.w_terrain, p.xy(), col, '/' );
                    break;
                case N_SOUTH | N_WEST:
                case N_NORTH | N_EAST:
                    mvwputch( g.w_terrain, p.xy(), col, '\\' );
                    break;
                case N_NO_NEIGHBORS:
                    mvwputch( g.w_terrain, p.xy(), col, '*' );
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
void explosion_handler::draw_explosion( const tripoint &p, const int r, const nc_color &col )
{
    if( test_mode ) {
        // avoid segfault from null tilecontext in tests
        return;
    }

    if( !use_tiles ) {
        draw_explosion_curses( *g, p, r, col );
        return;
    }

    if( !is_radius_visible( p, r ) ) {
        return;
    }

    explosion_animation anim;

    const bool visible = is_radius_visible( p, r );
    for( int i = 1; i <= r; i++ ) {
        // TODO: not xpos ypos?
        tilecontext->init_explosion( p, i );
        if( visible ) {
            anim.progress();
        }
    }

    if( r > 0 ) {
        tilecontext->void_explosion();
    }
}
#else
void explosion_handler::draw_explosion( const tripoint &p, const int r, const nc_color &col )
{
    draw_explosion_curses( *g, p, r, col );
}
#endif

void explosion_handler::draw_custom_explosion( const tripoint &,
        const std::map<tripoint, nc_color> &all_area )
{
    if( test_mode ) {
        // avoid segfault from null tilecontext in tests
        return;
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
            const tripoint relative_point = relative_view_pos( g->u, pr.first );
            if( relative_point.z == 0 ) {
                neighbors[pr.first] = explosion_tile{ N_NO_NEIGHBORS, pr.second };
            }
        }
    } else {
        // In tiles mode, the coordinates have to be absolute
        const tripoint view_center = relative_view_pos( g->u, g->u.pos() );
        for( const auto &pr : all_area ) {
            // Relative point is only used for z level check
            const tripoint relative_point = relative_view_pos( g->u, pr.first );
            if( relative_point.z == view_center.z ) {
                neighbors[pr.first] = explosion_tile{ N_NO_NEIGHBORS, pr.second };
            }
        }
    }
#else
    for( const auto &pr : all_area ) {
        const tripoint relative_point = relative_view_pos( g->u, pr.first );
        if( relative_point.z == 0 ) {
            neighbors[pr.first] = explosion_tile{ N_NO_NEIGHBORS, pr.second };
        }
    }
#endif

    // Searches for a neighbor, sets the neighborhood flag on current point and on the neighbor
    const auto set_neighbors = [&]( const tripoint & pos,
                                    explosion_neighbors & ngh,
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
    const auto unset_neighbor = [&]( const tripoint & pos,
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

        set_neighbors( pt + point_west, ngh, N_WEST, N_EAST );
        set_neighbors( pt + point_east, ngh, N_EAST, N_WEST );
        set_neighbors( pt + point_north, ngh, N_NORTH, N_SOUTH );
        set_neighbors( pt + point_south, ngh, N_SOUTH, N_NORTH );
    }

    // We need to save the layers because we will draw them in reverse order
    std::list< std::map<tripoint, explosion_tile> > layers;
    while( !neighbors.empty() ) {
        std::map<tripoint, explosion_tile> layer;
        bool changed = false;
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

            unset_neighbor( pt + point_west, ngh, N_WEST, N_EAST );
            unset_neighbor( pt + point_east, ngh, N_EAST, N_WEST );
            unset_neighbor( pt + point_north, ngh, N_NORTH, N_SOUTH );
            unset_neighbor( pt + point_south, ngh, N_SOUTH, N_NORTH );
            neighbors.erase( pr.first );
        }

        layers.push_front( std::move( layer ) );
    }

#if defined(TILES)
    if( !use_tiles ) {
        draw_custom_explosion_curses( *g, layers );
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
    draw_custom_explosion_curses( *g, layers );
#endif
}

namespace
{

void draw_bullet_curses( map &m, const tripoint &t, const char bullet, const tripoint *const p )
{
    if( !is_point_visible( t ) ) {
        return;
    }

    const tripoint vp = g->u.pos() + g->u.view_offset;

    if( p != nullptr && p->z == vp.z ) {
        m.drawsq( g->w_terrain, g->u, *p, false, true, vp );
    }

    if( vp.z != t.z ) {
        return;
    }

    mvwputch( g->w_terrain, t.xy() - vp.xy() + point( POSX, POSY ), c_red, bullet );
    bullet_animation().progress();
}

} // namespace

#if defined(TILES)
/* Bullet Animation -- Maybe change this to animate the ammo itself flying through the air?*/
// need to have a version where there is no player defined, possibly. That way shrapnel works as intended
void game::draw_bullet( const tripoint &t, const int /*i*/,
                        const std::vector<tripoint> &/*trajectory*/, const char bullet )
{
    if( !use_tiles ) {
        draw_bullet_curses( m, t, bullet, nullptr );
        return;
    }

    if( !is_point_visible( t ) ) {
        return;
    }

    static const std::string bullet_unknown  {};
    static const std::string bullet_normal   {"animation_bullet_normal"};
    static const std::string bullet_flame    {"animation_bullet_flame"};
    static const std::string bullet_shrapnel {"animation_bullet_shrapnel"};

    const std::string &bullet_type =
        bullet == '*' ? bullet_normal
        : bullet == '#' ? bullet_flame
        : bullet == '`' ? bullet_shrapnel
        : bullet_unknown;

    tilecontext->init_draw_bullet( t, bullet_type );
    bullet_animation().progress();
    tilecontext->void_bullet();
}
#else
void game::draw_bullet( const tripoint &t, const int i, const std::vector<tripoint> &trajectory,
                        const char bullet )
{
    draw_bullet_curses( m, t, bullet, &trajectory[i] );
}
#endif

namespace
{
// short visual animation (player, monster, ...) (hit, dodge, ...)
// cTile is a UTF-8 strings, and must be a single cell wide!
void hit_animation( const player &u, const tripoint &center, nc_color cColor,
                    const std::string &cTile )
{
    const tripoint init_pos = relative_view_pos( u, center );
    // Only show animation if initially visible
    if( init_pos.z == 0 && is_valid_in_w_terrain( init_pos.xy() ) ) {
        shared_ptr_fast<game::draw_callback_t> hit_cb = make_shared_fast<game::draw_callback_t>( [&]() {
            // In case the window is resized during waiting, we always re-calculate the animation position
            const tripoint pos = relative_view_pos( u, center );
            if( pos.z == 0 && is_valid_in_w_terrain( pos.xy() ) ) {
                mvwprintz( g->w_terrain, pos.xy(), cColor, cTile );
            }
        } );
        g->add_draw_callback( hit_cb );

        ui_manager::redraw();
        inp_mngr.set_timeout( get_option<int>( "ANIMATION_DELAY" ) );
        // Skip input (if any), because holding down a key with nanosleep can get yourself killed
        inp_mngr.get_input_event();
        inp_mngr.reset_timeout();
    }
}

void draw_hit_mon_curses( const tripoint &center, const monster &m, const player &u,
                          const bool dead )
{
    hit_animation( u, center, red_background( m.type->color ), dead ? "%" : m.symbol() );
}

} // namespace

#if defined(TILES)
void game::draw_hit_mon( const tripoint &p, const monster &m, const bool dead )
{
    if( test_mode ) {
        // avoid segfault from null tilecontext in tests
        return;
    }

    if( !use_tiles ) {
        draw_hit_mon_curses( p, m, u, dead );
        return;
    }

    tilecontext->init_draw_hit( p, m.type->id.str() );

    bullet_animation().progress();
}
#else
void game::draw_hit_mon( const tripoint &p, const monster &m, const bool dead )
{
    draw_hit_mon_curses( p, m, u, dead );
}
#endif

namespace
{
void draw_hit_player_curses( const game &g, const Character &p, const int dam )
{
    nc_color const col = !dam ? yellow_background( p.symbol_color() ) : red_background(
                             p.symbol_color() );
    hit_animation( g.u, p.pos(), col, p.symbol() );
}
} //namespace

#if defined(TILES)
void game::draw_hit_player( const Character &p, const int dam )
{
    if( test_mode ) {
        // avoid segfault from null tilecontext in tests
        return;
    }

    if( !use_tiles ) {
        draw_hit_player_curses( *this, p, dam );
        return;
    }

    static const std::string player_male   {"player_male"};
    static const std::string player_female {"player_female"};
    static const std::string npc_male      {"npc_male"};
    static const std::string npc_female    {"npc_female"};

    const std::string &type = p.is_player() ? ( p.male ? player_male : player_female )
                              : p.male ? npc_male : npc_female;
    tilecontext->init_draw_hit( p.pos(), type );
    bullet_animation().progress();
}
#else
void game::draw_hit_player( const Character &p, const int dam )
{
    draw_hit_player_curses( *this, p, dam );
}
#endif

/* Line drawing code, not really an animation but should be separated anyway */
namespace
{
void draw_line_curses( game &g, const tripoint &center, const std::vector<tripoint> &ret,
                       bool noreveal )
{
    for( const tripoint &p : ret ) {
        const auto critter = g.critter_at( p, true );

        // NPCs and monsters get drawn with inverted colors
        if( critter && g.u.sees( *critter ) ) {
            critter->draw( g.w_terrain, center, true );
        } else if( noreveal && !g.u.sees( p ) ) {
            // Draw a meaningless symbol. Avoids revealing tile, but keeps feedback
            const char sym = '?';
            const nc_color col = c_dark_gray;
            const catacurses::window &w = g.w_terrain;
            const int k = p.x + getmaxx( w ) / 2 - center.x;
            const int j = p.y + getmaxy( w ) / 2 - center.y;
            mvwputch( w, point( k, j ), col, sym );
        } else {
            // This function reveals tile at p and writes it to the player's memory
            g.m.drawsq( g.w_terrain, g.u, p, true, true, center );
        }
    }
}
} //namespace

#if defined(TILES)
void game::draw_line( const tripoint &p, const tripoint &center,
                      const std::vector<tripoint> &points, bool noreveal )
{
    if( !u.sees( p ) ) {
        return;
    }

    if( !use_tiles ) {
        draw_line_curses( *this, center, points, noreveal );
        return;
    }

    tilecontext->init_draw_line( p, points, "line_target", true );
}
#else
void game::draw_line( const tripoint &p, const tripoint &center,
                      const std::vector<tripoint> &points, bool noreveal )
{
    if( !u.sees( p ) ) {
        return;
    }

    draw_line_curses( *this, center, points, noreveal );
}
#endif

namespace
{
void draw_line_curses( game &g, const std::vector<tripoint> &points )
{
    for( const tripoint &p : points ) {
        g.m.drawsq( g.w_terrain, g.u, p, true, true );
    }

    const tripoint p = points.empty() ? tripoint {POSX, POSY, 0} :
                       relative_view_pos( g.u, points.back() );
    mvwputch( g.w_terrain, p.xy(), c_white, 'X' );
}
} //namespace

#if defined(TILES)
void game::draw_line( const tripoint &p, const std::vector<tripoint> &points )
{
    draw_line_curses( *this, points );
    tilecontext->init_draw_line( p, points, "line_trail", false );
}
#else
void game::draw_line( const tripoint &/*p*/, const std::vector<tripoint> &points )
{
    draw_line_curses( *this, points );
}
#endif

#if defined(TILES)
void game::draw_cursor( const tripoint &p )
{
    const tripoint rp = relative_view_pos( *this, p );
    mvwputch_inv( w_terrain, rp.xy(), c_light_green, 'X' );
    tilecontext->init_draw_cursor( p );
}
#else
void game::draw_cursor( const tripoint &p )
{
    const tripoint rp = relative_view_pos( *this, p );
    mvwputch_inv( w_terrain, rp.xy(), c_light_green, 'X' );
}
#endif

#if defined(TILES)
void game::draw_highlight( const tripoint &p )
{
    tilecontext->init_draw_highlight( p );
}
#else
void game::draw_highlight( const tripoint & )
{
    // Do nothing
}
#endif

namespace
{
void draw_weather_curses( const catacurses::window &win, const weather_printable &w )
{
    for( const auto &drop : w.vdrops ) {
        mvwputch( win, point( drop.first, drop.second ), w.colGlyph, w.cGlyph );
    }
}
} //namespace

#if defined(TILES)
void game::draw_weather( const weather_printable &w )
{
    if( !use_tiles ) {
        draw_weather_curses( w_terrain, w );
        return;
    }

    static const std::string weather_acid_drop {"weather_acid_drop"};
    static const std::string weather_rain_drop {"weather_rain_drop"};
    static const std::string weather_snowflake {"weather_snowflake"};

    std::string weather_name;
    switch( w.wtype ) {
        // Acid weathers; uses acid droplet tile, fallthrough intended
        case WEATHER_ACID_DRIZZLE:
        case WEATHER_ACID_RAIN:
            weather_name = weather_acid_drop;
            break;
        // Normal rainy weathers; uses normal raindrop tile, fallthrough intended
        case WEATHER_LIGHT_DRIZZLE:
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

    tilecontext->init_draw_weather( w, std::move( weather_name ) );
}
#else
void game::draw_weather( const weather_printable &w )
{
    draw_weather_curses( w_terrain, w );
}
#endif

namespace
{
void draw_sct_curses( const game &g )
{
    const tripoint off = relative_view_pos( g.u, tripoint_zero );

    for( const auto &text : SCT.vSCT ) {
        const int dy = off.y + text.getPosY();
        const int dx = off.x + text.getPosX();

        if( !is_valid_in_w_terrain( point( dx, dy ) ) ) {
            continue;
        }

        const bool is_old = text.getStep() >= SCT.iMaxSteps / 2;

        nc_color const col1 = msgtype_to_color( text.getMsgType( "first" ),  is_old );
        nc_color const col2 = msgtype_to_color( text.getMsgType( "second" ), is_old );

        mvwprintz( g.w_terrain, point( dx, dy ), col1, text.getText( "first" ) );
        wprintz( g.w_terrain, col2, text.getText( "second" ) );
    }
}
} //namespace

#if defined(TILES)
void game::draw_sct()
{
    if( use_tiles ) {
        tilecontext->init_draw_sct();
    } else {
        draw_sct_curses( *this );
    }
}
#else
void game::draw_sct()
{
    draw_sct_curses( *this );
}
#endif

namespace
{
void draw_zones_curses( const catacurses::window &w, const tripoint &start, const tripoint &end,
                        const tripoint &offset )
{
    if( end.x < start.x || end.y < start.y || end.z < start.z ) {
        return;
    }

    nc_color    const col = invert_color( c_light_green );
    const std::string line( end.x - start.x + 1, '~' );
    int         const x = start.x - offset.x;

    for( int y = start.y; y <= end.y; ++y ) {
        mvwprintz( w, point( x, y - offset.y ), col, line );
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

#if defined(TILES)
void game::draw_radiation_override( const tripoint &p, const int rad )
{
    if( use_tiles ) {
        tilecontext->init_draw_radiation_override( p, rad );
    }
}
#else
void game::draw_radiation_override( const tripoint &, const int )
{
}
#endif

#if defined(TILES)
void game::draw_terrain_override( const tripoint &p, const ter_id &id )
{
    if( use_tiles ) {
        tilecontext->init_draw_terrain_override( p, id );
    }
}
#else
void game::draw_terrain_override( const tripoint &, const ter_id & )
{
}
#endif

#if defined(TILES)
void game::draw_furniture_override( const tripoint &p, const furn_id &id )
{
    if( use_tiles ) {
        tilecontext->init_draw_furniture_override( p, id );
    }
}
#else
void game::draw_furniture_override( const tripoint &, const furn_id & )
{
}
#endif

#if defined(TILES)
void game::draw_graffiti_override( const tripoint &p, const bool has )
{
    if( use_tiles ) {
        tilecontext->init_draw_graffiti_override( p, has );
    }
}
#else
void game::draw_graffiti_override( const tripoint &, const bool )
{
}
#endif

#if defined(TILES)
void game::draw_trap_override( const tripoint &p, const trap_id &id )
{
    if( use_tiles ) {
        tilecontext->init_draw_trap_override( p, id );
    }
}
#else
void game::draw_trap_override( const tripoint &, const trap_id & )
{
}
#endif

#if defined(TILES)
void game::draw_field_override( const tripoint &p, const field_type_id &id )
{
    if( use_tiles ) {
        tilecontext->init_draw_field_override( p, id );
    }
}
#else
void game::draw_field_override( const tripoint &, const field_type_id & )
{
}
#endif

#if defined(TILES)
void game::draw_item_override( const tripoint &p, const itype_id &id, const mtype_id &mid,
                               const bool hilite )
{
    if( use_tiles ) {
        tilecontext->init_draw_item_override( p, id, mid, hilite );
    }
}
#else
void game::draw_item_override( const tripoint &, const itype_id &, const mtype_id &,
                               const bool )
{
}
#endif

#if defined(TILES)
void game::draw_vpart_override( const tripoint &p, const vpart_id &id, const int part_mod,
                                const int veh_dir, const bool hilite, const point &mount )
{
    if( use_tiles ) {
        tilecontext->init_draw_vpart_override( p, id, part_mod, veh_dir, hilite, mount );
    }
}
#else
void game::draw_vpart_override( const tripoint &, const vpart_id &, const int,
                                const int, const bool, const point & )
{
}
#endif

#if defined(TILES)
void game::draw_below_override( const tripoint &p, const bool draw )
{
    if( use_tiles ) {
        tilecontext->init_draw_below_override( p, draw );
    }
}
#else
void game::draw_below_override( const tripoint &, const bool )
{
}
#endif

#if defined(TILES)
void game::draw_monster_override( const tripoint &p, const mtype_id &id, const int count,
                                  const bool more, const Creature::Attitude att )
{
    if( use_tiles ) {
        tilecontext->init_draw_monster_override( p, id, count, more, att );
    }
}
#else
void game::draw_monster_override( const tripoint &, const mtype_id &, const int,
                                  const bool, const Creature::Attitude )
{
}
#endif
