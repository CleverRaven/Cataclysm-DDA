#if defined(TILES)

#include "pixel_minimap.h"

#include <algorithm>
#include <array>
#include <bitset>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <tuple>
#include <utility>
#include <vector>

#include "cached_options.h"
#include "cata_assert.h"
#include "cata_tiles.h"
#include "cata_utility.h"
#include "character.h"
#include "color.h"
#include "creature.h"
#include "creature_tracker.h"
#include "debug.h"
#include "level_cache.h"
#include "lightmap.h"
#include "map.h"
#include "map_scale_constants.h"
#include "mapdata.h"
#include "math_defines.h"
#include "mdarray.h"
#include "monster.h"
#include "pixel_minimap_projectors.h"
#include "sdl_utils.h"
#include "type_id.h"
#include "vehicle.h"
#include "viewer.h"
#include "vpart_position.h"

class game;

// NOLINTNEXTLINE(cata-static-declarations)
extern std::unique_ptr<game> g;

namespace
{

const point total_tiles_count = { MAX_VIEW_DISTANCE * 2 + 1, MAX_VIEW_DISTANCE * 2 + 1 };

point get_pixel_size( const point &tile_size, pixel_minimap_mode mode )
{
    switch( mode ) {
        case pixel_minimap_mode::solid:
            return tile_size;

        case pixel_minimap_mode::squares:
            return { std::max( tile_size.x - 1, 1 ), std::max( tile_size.y - 1, 1 ) };

        case pixel_minimap_mode::dots:
            return { point::south_east };
    }

    return {};
}

/// Returns a number in range [0..1]. The range lasts for @param phase_length_ms (milliseconds).
float get_animation_phase( int phase_length_ms )
{
    if( phase_length_ms == 0 ) {
        return 0.0f;
    }

    return std::fmod<float>( SDL_GetTicks(), phase_length_ms ) / phase_length_ms;
}

//creates the texture that individual minimap updates are drawn to
//later, the main texture is drawn to the display buffer
//the surface is needed to determine the color format needed by the texture
SDL_Texture_Ptr create_cache_texture( const SDL_Renderer_Ptr &renderer, int tile_width,
                                      int tile_height )
{
    return CreateTexture( renderer,
                          SDL_PIXELFORMAT_ARGB8888,
                          SDL_TEXTUREACCESS_TARGET,
                          tile_width,
                          tile_height );
}

SDL_Color get_map_color_at( const tripoint_bub_ms &p )
{
    const map &here = get_map();
    if( const optional_vpart_position vp = here.veh_at( p ) ) {
        const vpart_display vd = vp->vehicle().get_display_of_tile( vp->mount_pos() );
        return curses_color_to_SDL( vd.color );
    }

    if( const furn_id &furn_id = here.furn( p ) ) {
        return curses_color_to_SDL( furn_id->color() );
    }

    return curses_color_to_SDL( here.ter( p )->color() );
}

SDL_Color get_critter_color( Creature *critter, int flicker, int mixture )
{
    SDL_Color result = curses_color_to_SDL( critter->symbol_color() );

    if( const monster *m = dynamic_cast<monster *>( critter ) ) {
        //faction status (attacking or tracking) determines if red highlights get applied to creature
        const monster_attitude matt = m->attitude( &get_player_character() );

        if( MATT_ATTACK == matt || MATT_FOLLOW == matt ) {
            const SDL_Color red_pixel = SDL_Color{ 0xFF, 0x0, 0x0, 0xFF };
            result = adjust_color_brightness( mix_colors( result, red_pixel, mixture ), flicker );
        }
    }

    return result;
}

} // namespace

// a texture pool to avoid recreating textures every time player changes their view
// at most 142 out of 144 textures can be in use due to regular player movement
//  (moving from submap corner to new corner) with MAPSIZE = 11
// textures are dumped when the player moves more than one submap in one update
//  (teleporting, z-level change) to prevent running out of the remaining pool
class pixel_minimap::shared_texture_pool
{
    public:
        explicit shared_texture_pool( const std::function<SDL_Texture_Ptr()> &generator ) {
            const size_t pool_size = ( MAPSIZE + 1 ) * ( MAPSIZE + 1 );

            texture_pool.reserve( pool_size );
            inactive_index.reserve( pool_size );

            for( size_t i = 0; i < pool_size; ++i ) {
                texture_pool.emplace_back( generator() );
                inactive_index.push_back( i );
            }
        }

        //reserves a texture from the inactive group and returns tracking info
        SDL_Texture_Ptr request_tex( size_t &index ) {
            if( inactive_index.empty() ) {
                debugmsg( "Ran out of available textures in the pool." );
                //shouldn't be happening, but minimap will just be default color instead of crashing
                return nullptr;
            }
            index = inactive_index.back();
            inactive_index.pop_back();
            return std::move( texture_pool[index] );
        }

        //releases the provided texture back into the inactive pool to be used again
        //called automatically in the submap cache destructor
        void release_tex( size_t index, SDL_Texture_Ptr &&ptr ) {
            if( ptr ) {
                inactive_index.push_back( index );
                texture_pool[index] = std::move( ptr );
            }
        }

    private:
        std::vector<SDL_Texture_Ptr> texture_pool;
        std::vector<size_t> inactive_index;
};

struct pixel_minimap::submap_cache {
    //the color stored for each submap tile
    std::array<SDL_Color, SEEX *SEEY> minimap_colors = {};
    //checks if the submap has been looked at by the minimap routine
    bool touched = false;
    //the texture updates are drawn to
    SDL_Texture_Ptr chunk_tex;
    //the submap being handled
    size_t texture_index = 0;
    //the list of updates to apply to the texture
    //reduces render target switching to once per submap
    std::vector<point> update_list;
    //flag used to indicate that the texture needs to be cleared before first use
    bool ready = false;
    shared_texture_pool &pool;

    //reserve the SEEX * SEEY submap tiles
    explicit submap_cache( shared_texture_pool &pool ) :
        pool( pool ) {
        chunk_tex = pool.request_tex( texture_index );
    }

    //handle the release of the borrowed texture
    ~submap_cache() {
        pool.release_tex( texture_index, std::move( chunk_tex ) );
    }

    submap_cache( const submap_cache & ) = delete;
    submap_cache( submap_cache && ) = default;

    SDL_Color &color_at( const point &p ) {
        cata_assert( p.x < SEEX );
        cata_assert( p.y < SEEY );

        return minimap_colors[p.y * SEEX + p.x];
    }
};

pixel_minimap::pixel_minimap( const SDL_Renderer_Ptr &renderer,
                              const GeometryRenderer_Ptr &geometry ) :
    renderer( renderer ),
    geometry( geometry ),
    type( pixel_minimap_type::ortho ),
    screen_rect{ 0, 0, 0, 0 }
{
}

pixel_minimap::~pixel_minimap() = default;

void pixel_minimap::set_type( pixel_minimap_type type )
{
    if( this->type != type ) {
        this->type = type;
        reset();
    }
}

void pixel_minimap::set_settings( const pixel_minimap_settings &settings )
{
    this->settings = settings;
    reset();
}

void pixel_minimap::prepare_cache_for_updates( const tripoint_bub_ms &center )
{
    const tripoint_abs_sm new_center_sm = get_map().get_abs_sub() + rebase_rel(
            coords::project_to<coords::sm>
            ( center ) );
    const tripoint_rel_sm center_sm_diff = cached_center_sm - new_center_sm;

    //invalidate the cache if the game shifted more than one submap in the last update, or if z-level changed.
    if( std::abs( center_sm_diff.x() ) > 1 ||
        std::abs( center_sm_diff.y() ) > 1 ||
        std::abs( center_sm_diff.z() ) > 0 ) {
        cache.clear();
    } else {
        for( auto &mcp : cache ) {
            mcp.second.touched = false;
        }
    }

    cached_center_sm = new_center_sm;
}

//deletes the mapping of unused submap caches from the main map
//the touched flag prevents deletion
void pixel_minimap::clear_unused_cache()
{
    for( auto it = cache.begin(); it != cache.end(); ) {
        it = it->second.touched ? std::next( it ) : cache.erase( it );
    }
}

//draws individual updates to the submap cache texture
//the render target will be set back to display_buffer after all submaps are updated
void pixel_minimap::flush_cache_updates()
{
    for( auto &mcp : cache ) {
        if( mcp.second.update_list.empty() ) {
            continue;
        }

        SetRenderTarget( renderer, mcp.second.chunk_tex );

        if( !mcp.second.ready ) {
            mcp.second.ready = true;

            SetRenderDrawColor( renderer, 0x00, 0x00, 0x00, 0x00 );
            RenderClear( renderer );

            for( int y = 0; y < SEEY; ++y ) {
                for( int x = 0; x < SEEX; ++x ) {
                    const point tile_pos = projector->get_tile_pos( { x, y }, { SEEX, SEEY } );
                    const point tile_size = projector->get_tile_size();

                    const SDL_Rect rect = SDL_Rect{ tile_pos.x, tile_pos.y, tile_size.x, tile_size.y };

                    geometry->rect( renderer, rect, SDL_Color() );
                }
            }
        }

        for( const point &p : mcp.second.update_list ) {
            const point tile_pos = projector->get_tile_pos( p, { SEEX, SEEY } );
            const SDL_Color tile_color = mcp.second.color_at( p );

            if( pixel_size.x == 1 && pixel_size.y == 1 ) {
                SetRenderDrawColor( renderer, tile_color.r, tile_color.g, tile_color.b, tile_color.a );
                RenderDrawPoint( renderer, tile_pos );
            } else {
                geometry->rect( renderer, tile_pos, pixel_size.x, pixel_size.y, tile_color );
            }
        }

        mcp.second.update_list.clear();
    }
}

void pixel_minimap::update_cache_at( const tripoint_bub_sm &sm_pos )
{
    const map &here = get_map();
    const level_cache &access_cache = here.access_cache( sm_pos.z() );
    const bool nv_goggle = get_player_character().get_vision_modes()[NV_GOGGLES];

    submap_cache &cache_item = get_cache_at( here.get_abs_sub() + rebase_rel( sm_pos ) );
    const tripoint_bub_ms ms_pos = coords::project_to<coords::ms>( sm_pos );

    cache_item.touched = true;

    for( int y = 0; y < SEEY; ++y ) {
        for( int x = 0; x < SEEX; ++x ) {
            const tripoint_bub_ms p = ms_pos + tripoint{x, y, 0};
            const lit_level lighting = access_cache.visibility_cache[p.x()][p.y()];

            SDL_Color color;

            if( lighting == lit_level::BLANK || lighting == lit_level::DARK ) {
                // TODO: Map memory?
                color = { Uint8( pixel_minimap_r ), Uint8( pixel_minimap_g ), Uint8( pixel_minimap_b ), Uint8( pixel_minimap_a ) };
            } else {
                color = get_map_color_at( p );

                //color terrain according to lighting conditions
                if( nv_goggle ) {
                    if( lighting == lit_level::LOW ) {
                        color = color_pixel_nightvision( color );
                    } else if( lighting != lit_level::DARK && lighting != lit_level::BLANK ) {
                        color = color_pixel_overexposed( color );
                    }
                } else if( lighting == lit_level::LOW ) {
                    color = color_pixel_grayscale( color );
                }

                color = adjust_color_brightness( color, settings.brightness );
            }

            SDL_Color &current_color = cache_item.color_at( { x, y } );

            if( current_color != color ) {
                current_color = color;
                cache_item.update_list.emplace_back( x, y );
            }
        }
    }
}

pixel_minimap::submap_cache &pixel_minimap::get_cache_at( const tripoint_abs_sm &abs_sm_pos )
{
    auto it = cache.find( abs_sm_pos );

    if( it == cache.end() ) {
        it = cache.emplace( abs_sm_pos, submap_cache( *tex_pool ) ).first;
    }

    return it->second;
}

void pixel_minimap::process_cache( const tripoint_bub_ms &center )
{
    prepare_cache_for_updates( center );

    for( int y = 0; y < MAPSIZE; ++y ) {
        for( int x = 0; x < MAPSIZE; ++x ) {
            update_cache_at( { x, y, center.z()} );
        }
    }

    flush_cache_updates();
    clear_unused_cache();
}

void pixel_minimap::set_screen_rect( const SDL_Rect &screen_rect )
{
    if( this->screen_rect == screen_rect && main_tex && tex_pool && projector ) {
        return;
    }

    this->screen_rect = screen_rect;

    projector = create_projector( screen_rect );
    pixel_size = get_pixel_size( projector->get_tile_size(), settings.mode );

    const point size_on_screen = projector->get_tiles_size( total_tiles_count );

    if( settings.scale_to_fit ) {
        main_tex_clip_rect = SDL_Rect{ 0, 0, size_on_screen.x, size_on_screen.y };
        screen_clip_rect = fit_rect_inside( main_tex_clip_rect, screen_rect );

        SDL_SetHint( SDL_HINT_RENDER_SCALE_QUALITY, "1" );
        main_tex = create_cache_texture( renderer, size_on_screen.x, size_on_screen.y );
        SDL_SetHint( SDL_HINT_RENDER_SCALE_QUALITY, "0" );

    } else {
        const point d( ( size_on_screen.x - screen_rect.w ) / 2, ( size_on_screen.y - screen_rect.h ) / 2 );

        main_tex_clip_rect = SDL_Rect{
            std::max( d.x, 0 ),
            std::max( d.y, 0 ),
            size_on_screen.x - 2 * std::max( d.x, 0 ),
            size_on_screen.y - 2 * std::max( d.y, 0 )
        };

        screen_clip_rect = SDL_Rect{
            screen_rect.x - std::min( d.x, 0 ),
            screen_rect.y - std::min( d.y, 0 ),
            main_tex_clip_rect.w,
            main_tex_clip_rect.h
        };

        main_tex = create_cache_texture( renderer, size_on_screen.x, size_on_screen.y );
    }

    cache.clear();

    const point chunk_size = projector->get_tiles_size( { SEEX, SEEY } );

    const auto chunk_texture_generator = [&chunk_size, this]() {
        SDL_Texture_Ptr result = create_cache_texture( renderer, chunk_size.x, chunk_size.y );
        SetTextureBlendMode( result, SDL_BLENDMODE_BLEND );
        return result;
    };

    tex_pool = std::make_unique<shared_texture_pool>( chunk_texture_generator );
}

void pixel_minimap::reset()
{
    projector.reset();
    cache.clear();
    main_tex.reset();
    tex_pool.reset();
}

void pixel_minimap::render( const tripoint_bub_ms &center )
{
    SetRenderTarget( renderer, main_tex );
    SetRenderDrawColor( renderer, pixel_minimap_r, pixel_minimap_g, pixel_minimap_b, pixel_minimap_a );
    RenderClear( renderer );

    render_cache( center );
    render_critters( center );

    //set display buffer to main screen
    set_displaybuffer_rendertarget();
    //paint intermediate texture to screen
    RenderCopy( renderer, main_tex, &main_tex_clip_rect, &screen_clip_rect );
}

void pixel_minimap::render_cache( const tripoint_bub_ms &center )
{
    const tripoint_abs_sm sm_center = get_map().get_abs_sub() + rebase_rel(
                                          coords::project_to<coords::sm>
                                          ( center ) );
    const tripoint_rel_sm sm_offset {
        total_tiles_count.x / SEEX / 2,
        total_tiles_count.y / SEEY / 2, 0
    };

    point_rel_ms ms_offset;
    tripoint_bub_sm quotient;
    point_sm_ms remainder;
    std::tie( quotient, remainder ) = coords::project_remain<coords::sm>( center );

    point_sm_ms ms_base_offset = point_sm_ms( ( total_tiles_count.x / 2 ) % SEEX,
                                 ( total_tiles_count.y / 2 ) % SEEY );
    ms_offset = ms_base_offset - remainder;

    for( const auto &elem : cache ) {
        if( !elem.second.touched ) {
            continue;   // What you gonna do with all that junk?
        }

        const tripoint_rel_sm rel_pos = elem.first - sm_center;

        if( std::abs( rel_pos.x() ) > sm_offset.x() + 1 ||
            std::abs( rel_pos.y() ) > sm_offset.y() + 1 ||
            rel_pos.z() != 0 ) {
            continue;
        }

        const tripoint_rel_sm sm_pos = tripoint_rel_sm( rel_pos ) + sm_offset;
        const tripoint_rel_ms ms_pos = coords::project_to<coords::ms>( sm_pos ) + ms_offset;

        const SDL_Rect chunk_rect = projector->get_chunk_rect( ms_pos.xy().raw(), {SEEX, SEEY} );

        RenderCopy( renderer, elem.second.chunk_tex, nullptr, &chunk_rect );
    }
}

void pixel_minimap::render_critters( const tripoint_bub_ms &center )
{
    const map &m = get_map();

    //handles the enemy faction red highlights
    //this value should be divisible by 200
    const int indicator_length = settings.beacon_blink_interval * 200; //default is 2000 ms, 2 seconds

    int flicker = 100;
    int mixture = 0;

    if( indicator_length > 0 ) {
        const float t = get_animation_phase( 2 * indicator_length );
        const float s = std::sin( 2 * M_PI * t );

        flicker = lerp_clamped( 25, 100, std::abs( s ) );
        mixture = lerp_clamped( 0, 100, std::max( s, 0.0f ) );
    }

    const level_cache &access_cache = m.access_cache( center.z() );

    const point_rel_ms start( center.x() - total_tiles_count.x / 2,
                              center.y() - total_tiles_count.y / 2 );
    const point beacon_size = {
        std::max<int>( projector->get_tile_size().x *settings.beacon_size / 2, 2 ),
        std::max<int>( projector->get_tile_size().y *settings.beacon_size / 2, 2 )
    };

    creature_tracker &creatures = get_creature_tracker();
    for( int y = 0; y < total_tiles_count.y; y++ ) {
        for( int x = 0; x < total_tiles_count.x; x++ ) {
            const tripoint_bub_ms p = start + tripoint_bub_ms( x, y, center.z() );
            if( !m.inbounds( p ) ) {
                // p might be out-of-bounds when peeking at submap boundary. Example: center=(64,59,-5), start=(4,-1) -> p=(4,-1,-5)
                continue;
            }
            const lit_level lighting = access_cache.visibility_cache[p.x()][p.y()];

            if( lighting == lit_level::DARK || lighting == lit_level::BLANK ) {
                continue;
            }

            Creature *critter = creatures.creature_at( p, true );

            if( critter == nullptr || !get_player_view().sees( m, *critter ) ) {
                continue;
            }

            const point critter_pos = projector->get_tile_pos( { x, y }, total_tiles_count );
            const SDL_Rect critter_rect = SDL_Rect{ critter_pos.x, critter_pos.y, beacon_size.x, beacon_size.y };
            const SDL_Color critter_color = get_critter_color( critter, flicker, mixture );

            draw_beacon( critter_rect, critter_color );
        }
    }
}

//the main call for drawing the pixel minimap to the screen
void pixel_minimap::draw( const SDL_Rect &screen_rect, const tripoint_bub_ms &center )
{
    if( !g ) {
        return;
    }

    if( screen_rect.w <= 0 || screen_rect.h <= 0 ) {
        return;
    }

    set_screen_rect( screen_rect );
    process_cache( center );
    render( center );
}

void pixel_minimap::draw_beacon( const SDL_Rect &rect, const SDL_Color &color )
{
    for( int x = -rect.w, x_max = rect.w; x <= x_max; ++x ) {
        for( int y = -rect.h + std::abs( x ), y_max = rect.h - std::abs( x ); y <= y_max; ++y ) {
            const int divisor = 2 * ( std::abs( y ) == rect.h - std::abs( x ) ? 1 : 0 ) + 1;

            SetRenderDrawColor( renderer, color.r / divisor, color.g / divisor, color.b / divisor, 0xFF );
            RenderDrawPoint( renderer, point( rect.x + x, rect.y + y ) );
        }
    }
}

std::unique_ptr<pixel_minimap_projector> pixel_minimap::create_projector(
    const SDL_Rect &max_screen_rect )
const
{
    switch( type ) {
        case pixel_minimap_type::ortho:
            return std::make_unique<pixel_minimap_ortho_projector> ( total_tiles_count, max_screen_rect,
                    settings.square_pixels );

        case pixel_minimap_type::iso:
            return std::make_unique<pixel_minimap_iso_projector>( total_tiles_count, max_screen_rect,
                    settings.square_pixels );
    }

    return nullptr;
}

#endif // SDL_TILES
