#include "animation.h"


#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <thread>
#include <utility>
#include <vector>

#include "avatar.h"
#include "cached_options.h"
#include "character.h"
#include "coordinates.h"
#include "creature.h"
#include "creature_tracker.h"
#include "cursesdef.h"
#include "explosion.h"
#include "game.h"
#include "game_constants.h"
#include "input.h"
#include "map.h"
#include "memory_fast.h"
#include "monster.h"
#include "mtype.h"
#include "options.h"
#include "output.h"
#include "point.h"
#include "popup.h"
#include "string_formatter.h"
#include "translations.h"
#include "type_id.h"
#include "ui_manager.h"
#include "units_fwd.h"
#include "viewer.h"
#include "weather.h"

#if defined(TILES)
#include "cata_tiles.h" // all animation functions will be pushed out to a cata_tiles function in some manner
#include "sdltiles.h"
#include "weather_type.h"
#endif

namespace
{

class basic_animation
{
    public:
        explicit basic_animation( const int scale ) :
            delay( get_option<int>( "ANIMATION_DELAY" ) * scale * 1000000L ) {
        }

        void draw() const {
            static_popup popup;
            popup
            .wait_message( "%s", _( "Hang on a bit…" ) )
            .on_top( true );

            g->invalidate_main_ui_adaptor();
            ui_manager::redraw_invalidated();
            refresh_display();
        }

        void progress() const {
            draw();

            const auto sleep_till = std::chrono::steady_clock::now() + std::chrono::nanoseconds( delay );
            do {
                const auto sleep_for = std::min( sleep_till - std::chrono::steady_clock::now(),
                                                 // Pump events every 100 ms
                                                 std::chrono::nanoseconds( 100000000 ) );
                if( sleep_for > std::chrono::nanoseconds( 0 ) ) {
                    std::this_thread::sleep_for( sleep_for );
                    inp_mngr.pump_events();
                } else {
                    break;
                }
            } while( true );
        }

    private:
        int_least64_t delay;
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

bool is_point_visible( const tripoint_bub_ms &p, int margin = 0 )
{
    const map &here = get_map();

    return g->is_in_viewport( p, margin ) && get_player_view().sees( here, p );
}

bool is_radius_visible( const tripoint_bub_ms &center, int radius )
{
    return is_point_visible( center, -radius );
}

bool is_layer_visible( const std::map<tripoint_bub_ms, explosion_tile> &layer )
{
    return std::any_of( layer.begin(), layer.end(),
    []( const std::pair<tripoint_bub_ms, explosion_tile> &element ) {
        return is_point_visible( element.first );
    } );
}

//! Get p relative to u's current position and view
tripoint_rel_ms relative_view_pos( const avatar &u, const tripoint_bub_ms &p ) noexcept
{
    return p - u.pos_bub() - u.view_offset + point_rel_ms( POSX, POSY );
}

// Convert p to screen position relative to the current terrain view
tripoint_rel_ms relative_view_pos( const game &g, const tripoint_bub_ms &p ) noexcept
{
    return p - g.ter_view_p + point( POSX, POSY );
}

void draw_explosion_curses( game &g, const tripoint_bub_ms &center, const int r,
                            const nc_color &col )
{
    if( !is_radius_visible( center, r ) ) {
        return;
    }
    // TODO: Make it look different from above/below
    const tripoint_rel_ms p = relative_view_pos( get_avatar(), center );

    explosion_animation anim;

    int frame = 0;
    shared_ptr_fast<game::draw_callback_t> explosion_cb =
    make_shared_fast<game::draw_callback_t>( [&]() {
        wattron( g.w_terrain, col );
        if( r == 0 ) {
            mvwaddch( g.w_terrain, point( p.y(), p.x() ), '*' );
        }

        for( int i = 1; i <= frame; ++i ) {
            // corner: top left
            mvwaddch( g.w_terrain, p.xy().raw() + point( -i, -i ), '/' );
            // corner: top right
            mvwaddch( g.w_terrain, p.xy().raw() + point( i, -i ), '\\' );
            // corner: bottom left
            mvwaddch( g.w_terrain, p.xy().raw() + point( -i, i ), '\\' );
            // corner: bottom right
            mvwaddch( g.w_terrain, p.xy().raw() + point( i, i ), '/' );
            for( int j = 1 - i; j < 0 + i; j++ ) {
                // edge: top
                mvwaddch( g.w_terrain, p.xy().raw() + point( j, -i ), '-' );
                // edge: bottom
                mvwaddch( g.w_terrain, p.xy().raw() + point( j, i ), '-' );
                // edge: left
                mvwaddch( g.w_terrain, p.xy().raw() + point( -i, j ), '|' );
                // edge: right
                mvwaddch( g.w_terrain, p.xy().raw() + point( i, j ), '|' );
            }
        }
        wattroff( g.w_terrain, col );
    } );
    g.add_draw_callback( explosion_cb );

    for( frame = 1; frame <= r; ++frame ) {
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
                                   const std::list< std::map<tripoint_bub_ms, explosion_tile> > &layers )
{
    avatar &player_character = get_avatar();
    // calculate screen offset relative to player + view offset position
    const tripoint_bub_ms center = player_character.pos_bub() + player_character.view_offset;
    const tripoint topleft( center.x() - getmaxx( g.w_terrain ) / 2,
                            center.y() - getmaxy( g.w_terrain ) / 2, 0 );

    explosion_animation anim;

    auto last_layer_it = layers.begin();
    shared_ptr_fast<game::draw_callback_t> explosion_cb =
    make_shared_fast<game::draw_callback_t>( [&]() {
        for( auto it = layers.begin(); it != std::next( last_layer_it ); ++it ) {
            for( const auto &pr : *it ) {
                // update tripoint in relation to top left corner of curses window
                // mvwputch already filters out of bounds coordinates
                const tripoint p = pr.first.raw() - topleft;
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
        }
    } );
    g.add_draw_callback( explosion_cb );

    for( last_layer_it = layers.begin(); last_layer_it != layers.end(); ++last_layer_it ) {
        if( is_layer_visible( *last_layer_it ) ) {
            anim.progress();
        }
    }
}
} // namespace

#if defined(TILES)
void explosion_handler::draw_explosion( const tripoint_bub_ms &p, const int r, const nc_color &col )
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

    int i = 1;
    shared_ptr_fast<game::draw_callback_t> explosion_cb =
    make_shared_fast<game::draw_callback_t>( [&]() {
        // TODO: not xpos ypos?
        tilecontext->init_explosion( p, i );
    } );
    g->add_draw_callback( explosion_cb );

    const bool visible = is_radius_visible( p, r );
    for( i = 1; i <= r; i++ ) {
        if( visible ) {
            anim.progress();
        }
    }

    if( r > 0 ) {
        tilecontext->void_explosion();
    }
}

#else
void explosion_handler::draw_explosion( const tripoint_bub_ms &p, const int r, const nc_color &col )
{
    draw_explosion_curses( *g, p, r, col );
}
#endif

void explosion_handler::draw_custom_explosion(
    const std::map<tripoint_bub_ms, nc_color> &all_area, const std::optional<std::string> &tile_id )
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
    std::map<tripoint_bub_ms, explosion_tile> neighbors;
    avatar &player_character = get_avatar();
#if defined(TILES)
    if( !use_tiles ) {
        for( const auto &pr : all_area ) {
            const tripoint_rel_ms relative_point = relative_view_pos( player_character, pr.first );
            if( relative_point.z() == 0 ) {
                neighbors[pr.first] = explosion_tile{ N_NO_NEIGHBORS, pr.second, tile_id };
            }
        }
    } else {
        // In tiles mode, the coordinates have to be absolute
        const tripoint_rel_ms view_center = relative_view_pos( player_character,
                                            player_character.pos_bub() );
        for( const auto &pr : all_area ) {
            // Relative point is only used for z level check
            const tripoint_rel_ms relative_point = relative_view_pos( player_character, pr.first );
            if( relative_point.z() == view_center.z() ) {
                neighbors[pr.first] = explosion_tile{ N_NO_NEIGHBORS, pr.second, tile_id };
            }
        }
    }
#else
    for( const auto &pr : all_area ) {
        const tripoint_rel_ms relative_point = relative_view_pos( player_character, pr.first );
        if( relative_point.z( ) == 0 ) {
            neighbors[pr.first] = explosion_tile{ N_NO_NEIGHBORS, pr.second, tile_id };
        }
    }
#endif

    // Searches for a neighbor, sets the neighborhood flag on current point and on the neighbor
    const auto set_neighbors = [&]( const tripoint_bub_ms & pos,
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
    const auto unset_neighbor = [&]( const tripoint_bub_ms & pos,
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
        const tripoint_bub_ms &pt = pr.first;
        explosion_neighbors &ngh = pr.second.neighborhood;

        set_neighbors( pt + point::west, ngh, N_WEST, N_EAST );
        set_neighbors( pt + point::east, ngh, N_EAST, N_WEST );
        set_neighbors( pt + point::north, ngh, N_NORTH, N_SOUTH );
        set_neighbors( pt + point::south, ngh, N_SOUTH, N_NORTH );
    }

    // We need to save the layers because we will draw them in reverse order
    std::list< std::map<tripoint_bub_ms, explosion_tile> > layers;
    while( !neighbors.empty() ) {
        std::map<tripoint_bub_ms, explosion_tile> layer;
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
            const tripoint_bub_ms &pt = pr.first;
            const explosion_neighbors ngh = pr.second.neighborhood;

            unset_neighbor( pt + point::west, ngh, N_WEST, N_EAST );
            unset_neighbor( pt + point::east, ngh, N_EAST, N_WEST );
            unset_neighbor( pt + point::north, ngh, N_NORTH, N_SOUTH );
            unset_neighbor( pt + point::south, ngh, N_SOUTH, N_NORTH );
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
    std::map<tripoint_bub_ms, explosion_tile> combined_layer;

    shared_ptr_fast<game::draw_callback_t> explosion_cb =
    make_shared_fast<game::draw_callback_t>( [&]() {
        tilecontext->init_custom_explosion_layer( combined_layer );
    } );
    g->add_draw_callback( explosion_cb );

    for( const auto &layer : layers ) {
        combined_layer.insert( layer.begin(), layer.end() );
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

void draw_bullet_curses( map &m, const tripoint_bub_ms &t, const char bullet,
                         const tripoint_bub_ms *const p )
{
    if( !is_point_visible( t ) ) {
        return;
    }

    avatar &player_character = get_avatar();
    const tripoint_bub_ms vp = player_character.pos_bub() + player_character.view_offset;

    if( vp.z() != t.z() ) {
        return;
    }

    shared_ptr_fast<game::draw_callback_t> bullet_cb = make_shared_fast<game::draw_callback_t>( [&]() {
        if( p != nullptr && p->z() == vp.z() ) {
            m.drawsq( g->w_terrain, *p, drawsq_params().center( vp ) );
        }
        mvwputch( g->w_terrain, t.xy().raw() - vp.xy().raw() + point( POSX, POSY ), c_red, bullet );
    } );
    g->add_draw_callback( bullet_cb );
    bullet_animation().progress();
}

} // namespace

#if defined(TILES)
/* Bullet Animation -- Maybe change this to animate the ammo itself flying through the air?*/
// need to have a version where there is no player defined, possibly. That way shrapnel works as intended
void game::draw_bullet( const tripoint_bub_ms &t, const int /*i*/,
                        const std::vector<tripoint_bub_ms> &/*trajectory*/, const char bullet )
{
    if( test_mode ) {
        // avoid segfault from null tilecontext in tests
        return;
    }
    if( !use_tiles ) {
        draw_bullet_curses( get_map(), t, bullet, nullptr );
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

    shared_ptr_fast<draw_callback_t> bullet_cb = make_shared_fast<draw_callback_t>( [&]() {
        tilecontext->init_draw_bullet( t, bullet_type );
    } );
    add_draw_callback( bullet_cb );

    bullet_animation().progress();
    tilecontext->void_bullet();
}

#else
void game::draw_bullet( const tripoint_bub_ms &t, const int i,
                        const std::vector<tripoint_bub_ms> &trajectory,
                        const char bullet )
{
    draw_bullet_curses( m, t, bullet, &trajectory[i] );
}
#endif

namespace
{
// short visual animation (player, monster, ...) (hit, dodge, ...)
// cTile is a UTF-8 strings, and must be a single cell wide!
void hit_animation( const avatar &u, const tripoint_bub_ms &center, nc_color cColor,
                    const std::string &cTile )
{
    const tripoint_rel_ms init_pos = relative_view_pos( u, center );
    // Only show animation if initially visible
    if( init_pos.z() == 0 && is_valid_in_w_terrain( init_pos.xy() ) ) {
        shared_ptr_fast<game::draw_callback_t> hit_cb = make_shared_fast<game::draw_callback_t>( [&]() {
            // In case the window is resized during waiting, we always re-calculate the animation position
            const tripoint_rel_ms pos = relative_view_pos( u, center );
            if( pos.z() == 0 && is_valid_in_w_terrain( pos.xy() ) ) {
                mvwprintz( g->w_terrain, pos.xy().raw(), cColor, cTile );
            }
        } );
        g->add_draw_callback( hit_cb );

        ui_manager::redraw();
        inp_mngr.set_timeout( get_option<int>( "ANIMATION_DELAY" ) );
        // Skip input (if any), because holding down a key with sleep_for can get yourself killed
        inp_mngr.get_input_event();
        inp_mngr.reset_timeout();
    }
}

void draw_hit_mon_curses( const tripoint_bub_ms &center, const monster &m, const avatar &u,
                          const bool dead )
{
    hit_animation( u, center, red_background( m.type->color ), dead ? "%" : m.symbol() );
}

} // namespace

#if defined(TILES)
void game::draw_hit_mon( const tripoint_bub_ms &p, const monster &m, const bool dead )
{
    if( test_mode ) {
        // avoid segfault from null tilecontext in tests
        return;
    }

    if( !use_tiles ) {
        draw_hit_mon_curses( p, m, u, dead );
        return;
    }

    shared_ptr_fast<draw_callback_t> hit_cb = make_shared_fast<draw_callback_t>( [&]() {
        tilecontext->init_draw_hit( p, m.type->id.str() );
    } );
    add_draw_callback( hit_cb );

    bullet_animation().progress();
}

#else
void game::draw_hit_mon( const tripoint_bub_ms &p, const monster &m, const bool dead )
{
    draw_hit_mon_curses( p, m, u, dead );
}
#endif

namespace
{
void draw_hit_player_curses( const game &/* g */, const Character &p, const int dam )
{
    nc_color const col = !dam ? yellow_background( p.symbol_color() ) : red_background(
                             p.symbol_color() );
    hit_animation( get_avatar(), p.pos_bub(), col, p.symbol() );
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

    const std::string &type = p.is_avatar() ? ( p.male ? player_male : player_female )
                              : p.male ? npc_male : npc_female;

    shared_ptr_fast<draw_callback_t> hit_cb = make_shared_fast<draw_callback_t>( [&]() {
        tilecontext->init_draw_hit( p.pos_bub(), type );
    } );
    add_draw_callback( hit_cb );

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
void draw_line_curses( game &g, const tripoint_bub_ms &center,
                       const std::vector<tripoint_bub_ms> &ret,
                       bool noreveal )
{

    avatar &player_character = get_avatar();
    map &here = get_map();
    drawsq_params params = drawsq_params().highlight( true ).center( center );
    creature_tracker &creatures = get_creature_tracker();
    for( const tripoint_bub_ms &p : ret ) {
        const Creature *critter = creatures.creature_at( p, true );

        // NPCs and monsters get drawn with inverted colors
        if( critter && player_character.sees( here, *critter ) ) {
            critter->draw( g.w_terrain, center, true );
        } else if( noreveal && !player_character.sees( here,  p ) ) {
            // Draw a meaningless symbol. Avoids revealing tile, but keeps feedback
            const char sym = '?';
            const nc_color col = c_dark_gray;
            const catacurses::window &w = g.w_terrain;
            const int k = p.x() + getmaxx( w ) / 2 - center.x();
            const int j = p.y() + getmaxy( w ) / 2 - center.y();
            mvwputch( w, point( k, j ), col, sym );
        } else {
            // This function reveals tile at p and writes it to the player's memory
            here.drawsq( g.w_terrain, p, params );
        }
    }
}
} //namespace

#if defined(TILES)
void game::draw_line( const tripoint_bub_ms &p, const tripoint_bub_ms &center,
                      const std::vector<tripoint_bub_ms> &points, bool noreveal )
{
    const map &here = get_map();

    if( !u.sees( here, p ) ) {
        return;
    }

    if( !use_tiles ) {
        draw_line_curses( *this, center, points, noreveal );
        return;
    }

    tilecontext->init_draw_line( p, points, "line_target", true );
}

#else
void game::draw_line( const tripoint_bub_ms &p, const tripoint_bub_ms &center,
                      const std::vector<tripoint_bub_ms> &points, bool noreveal )
{
    const map &here = get_map();

    if( !u.sees( here, p ) ) {
        return;
    }

    draw_line_curses( *this, center, points, noreveal );
}
#endif

namespace
{
void draw_line_curses( game &g, const std::vector<tripoint_bub_ms> &points )
{
    avatar &player_character = get_avatar();
    map &here = get_map();
    for( const tripoint_bub_ms &p : points ) {
        here.drawsq( g.w_terrain, p, drawsq_params().highlight( true ) );
    }

    const tripoint_rel_ms p = points.empty() ? tripoint_rel_ms {POSX, POSY, 0} :
                              relative_view_pos( player_character, points.back() );
    mvwputch( g.w_terrain, p.xy().raw(), c_white, 'X' );
}
} //namespace

#if defined(TILES)
void game::draw_line( const tripoint_bub_ms &p, const std::vector<tripoint_bub_ms> &points )
{
    draw_line_curses( *this, points );
    tilecontext->init_draw_line( p, points, "line_trail", false );
}
#else
void game::draw_line( const tripoint_bub_ms &/*p*/, const std::vector<tripoint_bub_ms> &points )
{
    draw_line_curses( *this, points );
}
#endif

#if defined(TILES)
void game::draw_cursor( const tripoint_bub_ms &p ) const
{
    const tripoint_rel_ms rp = relative_view_pos( *this, p );
    mvwputch_inv( w_terrain, rp.xy().raw(), c_light_green, 'X' );
    tilecontext->init_draw_cursor( p );
}
#else
void game::draw_cursor( const tripoint_bub_ms &p ) const
{
    const tripoint_rel_ms rp = relative_view_pos( *this, p );
    mvwputch_inv( w_terrain, rp.xy().raw(), c_light_green, 'X' );
}
#endif

void game::draw_cursor_unobscuring( const tripoint_bub_ms &p ) const
{
    const tripoint_rel_ms rp = relative_view_pos( *this, p );
    mvwputch_inv( w_terrain, ( rp.xy() + point::north_east ).raw(), c_cyan, "↙" );
    mvwputch_inv( w_terrain, ( rp.xy() + point::south_east ).raw(), c_cyan, "↖" );
    mvwputch_inv( w_terrain, ( rp.xy() + point::north_west ).raw(), c_cyan, "↘" );
    mvwputch_inv( w_terrain, ( rp.xy() + point::south_west ).raw(), c_cyan, "↗" );
#if defined(TILES)
    tilecontext->init_draw_cursor( p );
#endif
}

#if defined(TILES)
void game::draw_highlight( const tripoint_bub_ms &p )
{
    tilecontext->init_draw_highlight( p );
}
#else
void game::draw_highlight( const tripoint_bub_ms & )
{
    // Do nothing
}
#endif

namespace
{
void draw_weather_curses( const catacurses::window &win, const weather_printable &w )
{
    wattron( win, w.colGlyph );
    const std::string symbol = w.get_symbol();
    for( const auto &drop : w.vdrops ) {
        mvwprintw( win, point( drop.first, drop.second ), symbol );
    }
    wattroff( win, w.colGlyph );
}
} //namespace

#if defined(TILES)
void game::draw_weather( const weather_printable &w ) const
{
    if( !use_tiles ) {
        draw_weather_curses( w_terrain, w );
        return;
    }

    tilecontext->init_draw_weather( w, w.wtype->tiles_animation );
}
#else
void game::draw_weather( const weather_printable &w ) const
{
    draw_weather_curses( w_terrain, w );
}
#endif

namespace
{
void draw_sct_curses( const game &g )
{
    avatar &player_character = get_avatar();
    const tripoint_rel_ms off = relative_view_pos( player_character,
                                tripoint_bub_ms::zero );

    for( const scrollingcombattext::cSCT &text : SCT.vSCT ) {
        const int dy = off.y() + text.getPosY();
        const int dx = off.x() + text.getPosX();

        if( !is_valid_in_w_terrain( { dx, dy } ) ) {
            continue;
        }

        const bool is_old = text.getStep() >= scrollingcombattext::iMaxSteps / 2;

        nc_color const col1 = msgtype_to_color( text.getMsgType( "first" ),  is_old );
        nc_color const col2 = msgtype_to_color( text.getMsgType( "second" ), is_old );

        mvwprintz( g.w_terrain, point( dx, dy ), col1, text.getText( "first" ) );
        wprintz( g.w_terrain, col2, text.getText( "second" ) );
    }
}
} //namespace

#if defined(TILES)
void game::draw_sct() const
{
    if( use_tiles ) {
        tilecontext->init_draw_sct();
    } else {
        draw_sct_curses( *this );
    }
}
#else
void game::draw_sct() const
{
    draw_sct_curses( *this );
}
#endif

namespace
{
void draw_zones_curses( const catacurses::window &w, const tripoint_bub_ms &start,
                        const tripoint_bub_ms &end,
                        const tripoint_rel_ms &offset )
{
    if( end.x() < start.x() || end.y() < start.y() || end.z() < start.z() ) {
        return;
    }

    nc_color    const col = invert_color( c_light_green );

    wattron( w, col );
    mvwrectf( w, ( start - offset ).xy().raw(), '~', end.x() - start.x() + 1, end.y() - start.y() + 1 );
    wattroff( w, col );
}
} //namespace

#if defined(TILES)
void game::draw_zones( const tripoint_bub_ms &start, const tripoint_bub_ms &end,
                       const tripoint_rel_ms &offset ) const
{
    if( use_tiles ) {
        tilecontext->init_draw_zones( start, end, offset );
    } else {
        draw_zones_curses( w_terrain, start, end, offset );
    }
}
#else
void game::draw_zones( const tripoint_bub_ms &start, const tripoint_bub_ms &end,
                       const tripoint_rel_ms &offset ) const
{
    draw_zones_curses( w_terrain, start, end, offset );
}
#endif

#if defined(TILES)
void game::draw_async_anim( const tripoint_bub_ms &p, const std::string &tile_id,
                            const std::string &ncstr,
                            const nc_color &nccol )
{
    const map &here = get_map();

    if( test_mode ) {
        // avoid segfault from null tilecontext in tests
        return;
    }

    if( !get_option<bool>( "ANIMATIONS" ) ) {
        return;
    }

    if( !u.sees( here, p ) ) {
        return;
    }

    if( !use_tiles ) {
        if( !ncstr.empty() ) {
            g->init_draw_async_anim_curses( p, ncstr, nccol );
        }
        return;
    }

    tilecontext->init_draw_async_anim( p, tile_id );
    g->invalidate_main_ui_adaptor();
}
#else
void game::draw_async_anim( const tripoint_bub_ms &p, const std::string &,
                            const std::string &ncstr,
                            const nc_color &nccol )
{
    if( !ncstr.empty() ) {
        g->init_draw_async_anim_curses( p, ncstr, nccol );
    }
}
#endif

#if defined(TILES)
void game::draw_radiation_override( const tripoint_bub_ms &p, const int rad )
{
    if( use_tiles ) {
        tilecontext->init_draw_radiation_override( p, rad );
    }
}
#else
void game::draw_radiation_override( const tripoint_bub_ms &, const int )
{
}
#endif

#if defined(TILES)
void game::draw_terrain_override( const tripoint_bub_ms &p, const ter_id &id )
{
    if( use_tiles ) {
        tilecontext->init_draw_terrain_override( p, id );
    }
}
#else
void game::draw_terrain_override( const tripoint_bub_ms &, const ter_id & )
{
}
#endif

#if defined(TILES)
void game::draw_furniture_override( const tripoint_bub_ms &p, const furn_id &id )
{
    if( use_tiles ) {
        tilecontext->init_draw_furniture_override( p, id );
    }
}
#else
void game::draw_furniture_override( const tripoint_bub_ms &, const furn_id & )
{
}
#endif

#if defined(TILES)
void game::draw_graffiti_override( const tripoint_bub_ms &p, const bool has )
{
    if( use_tiles ) {
        tilecontext->init_draw_graffiti_override( p, has );
    }
}
#else
void game::draw_graffiti_override( const tripoint_bub_ms &, const bool )
{
}
#endif

#if defined(TILES)
void game::draw_trap_override( const tripoint_bub_ms &p, const trap_id &id )
{
    if( use_tiles ) {
        tilecontext->init_draw_trap_override( p, id );
    }
}
#else
void game::draw_trap_override( const tripoint_bub_ms &, const trap_id & )
{
}
#endif

#if defined(TILES)
void game::draw_field_override( const tripoint_bub_ms &p, const field_type_id &id )
{
    if( use_tiles ) {
        tilecontext->init_draw_field_override( p, id );
    }
}
#else
void game::draw_field_override( const tripoint_bub_ms &, const field_type_id & )
{
}
#endif

#if defined(TILES)
void game::draw_item_override( const tripoint_bub_ms &p, const itype_id &id, const mtype_id &mid,
                               const bool hilite )
{
    if( use_tiles ) {
        tilecontext->init_draw_item_override( p, id, mid, hilite );
    }
}
#else
void game::draw_item_override( const tripoint_bub_ms &, const itype_id &, const mtype_id &,
                               const bool )
{
}
#endif

#if defined(TILES)
void game::draw_vpart_override(
    const tripoint_bub_ms &p, const vpart_id &id, const int part_mod, const units::angle &veh_dir,
    const bool hilite, const point_rel_ms &mount )
{
    if( use_tiles ) {
        tilecontext->init_draw_vpart_override( p, id, part_mod, veh_dir, hilite, mount );
    }
}
#else
void game::draw_vpart_override( const tripoint_bub_ms &, const vpart_id &, const int,
                                const units::angle &, const bool, const point_rel_ms & )
{
}
#endif

#if defined(TILES)
void game::draw_below_override( const tripoint_bub_ms &p, const bool draw )
{
    if( use_tiles ) {
        tilecontext->init_draw_below_override( p, draw );
    }
}
#else
void game::draw_below_override( const tripoint_bub_ms &, const bool )
{
}
#endif

#if defined(TILES)
void game::draw_monster_override( const tripoint_bub_ms &p, const mtype_id &id, const int count,
                                  const bool more, const Creature::Attitude att )
{
    if( use_tiles ) {
        tilecontext->init_draw_monster_override( p, id, count, more, att );
    }
}
#else
void game::draw_monster_override( const tripoint_bub_ms &, const mtype_id &, const int,
                                  const bool, const Creature::Attitude )
{
}
#endif
