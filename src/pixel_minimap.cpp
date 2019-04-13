#include "pixel_minimap.h"

#include "coordinate_conversions.h"
#include "game.h"
#include "map.h"
#include "mapdata.h"
#include "monster.h"
#include "sdl_utils.h"
#include "player.h"
#include "vehicle.h"
#include "vpart_position.h"

#include <set>
#include <vector>


extern void set_displaybuffer_rendertarget();


namespace
{

const point min_size = { 0, 0 };
const point max_size = { MAPSIZE_X, MAPSIZE_Y };
const point tiles_range = { ( MAPSIZE - 2 ) *SEEX, ( MAPSIZE - 2 ) *SEEY };

/// Returns a number in range [0..1]. The range lasts for @param phase_length_ms (milliseconds).
float get_animation_phase( int phase_length_ms )
{
    if( phase_length_ms == 0 ) {
        return 0.0f;
    }

    return std::fmod<float>( SDL_GetTicks(), phase_length_ms ) / phase_length_ms;
}

//converts the local x,y point into the global submap coordinates
tripoint convert_tripoint_to_abs_submap( const tripoint &p )
{
    //get the submap coordinates of the current location
    tripoint sm_loc = ms_to_sm_copy( p );
    //add it to the absolute map coordinates
    tripoint abs_sm_loc = g->m.get_abs_sub();
    return abs_sm_loc + sm_loc;
}

//creates the texture that individual minimap updates are drawn to
//later, the main texture is drawn to the display buffer
//the surface is needed to determine the color format needed by the texture
SDL_Texture_Ptr create_cache_texture( const SDL_Renderer_Ptr &renderer, int tile_width,
                                      int tile_height )
{
    const SDL_Surface_Ptr temp = create_surface_32( tile_width, tile_height );
    assert( temp );
    SDL_Texture_Ptr tex( SDL_CreateTexture( renderer.get(), temp->format->format,
                                            SDL_TEXTUREACCESS_TARGET, tile_width, tile_height ) );
    throwErrorIf( !tex, "SDL_CreateTexture failed to create minimap texture" );
    return tex;
}

SDL_Color get_map_color_at( const tripoint &p )
{
    const auto &m = g->m;

    if( const auto vp = m.veh_at( p ) ) {
        return curses_color_to_SDL( vp->vehicle().part_color( vp->part_index() ) );
    }

    if( const auto furn_id = m.furn( p ) ) {
        return curses_color_to_SDL( furn_id->color() );
    }

    return curses_color_to_SDL( m.ter( p )->color() );
}

SDL_Color get_critter_color( Creature *critter, int flicker, int mixture )
{
    auto result = curses_color_to_SDL( critter->symbol_color() );

    if( const auto m = dynamic_cast<monster *>( critter ) ) {
        //faction status (attacking or tracking) determines if red highlights get applied to creature
        const auto matt = m->attitude( &( g->u ) );

        if( MATT_ATTACK == matt || MATT_FOLLOW == matt ) {
            const auto red_pixel = SDL_Color{ 0xFF, 0x0, 0x0, 0xFF };
            result = adjust_color_brightness( mix_colors( result, red_pixel, mixture ), flicker );
        }
    }

    return result;
}

}

// a texture pool to avoid recreating textures every time player changes their view
// at most 142 out of 144 textures can be in use due to regular player movement
//  (moving from submap corner to new corner) with MAPSIZE = 11
// textures are dumped when the player moves more than one submap in one update
//  (teleporting, z-level change) to prevent running out of the remaining pool
struct pixel_minimap::shared_texture_pool {
    std::vector<SDL_Texture_Ptr> texture_pool;
    std::set<int> active_index;
    std::vector<int> inactive_index;

    shared_texture_pool() {
        texture_pool.resize( ( MAPSIZE + 1 ) * ( MAPSIZE + 1 ) );
        for( int i = 0; i < static_cast<int>( texture_pool.size() ); i++ ) {
            inactive_index.push_back( i );
        }
    }

    //reserves a texture from the inactive group and returns tracking info
    SDL_Texture_Ptr request_tex( int &i ) {
        if( inactive_index.empty() ) {
            //shouldn't be happening, but minimap will just be default color instead of crashing
            return nullptr;
        }
        const int index = inactive_index.back();
        inactive_index.pop_back();
        active_index.insert( index );
        i = index;
        return std::move( texture_pool[index] );
    }

    //releases the provided texture back into the inactive pool to be used again
    //called automatically in the submap cache destructor
    void release_tex( int i, SDL_Texture_Ptr ptr ) {
        const auto it = active_index.find( i );
        if( it == active_index.end() ) {
            return;
        }
        inactive_index.push_back( i );
        active_index.erase( i );
        texture_pool[i] = std::move( ptr );
    }
};

struct pixel_minimap::submap_cache {
    //the color stored for each submap tile
    std::vector< SDL_Color > minimap_colors;
    //checks if the submap has been looked at by the minimap routine
    bool touched;
    //the texture updates are drawn to
    SDL_Texture_Ptr minimap_tex;
    //the submap being handled
    int texture_index;
    //the list of updates to apply to the texture
    //reduces render target switching to once per submap
    std::vector<point> update_list;
    //if the submap has been drawn to screen during the current draw cycle
    bool drawn;
    //flag used to indicate that the texture needs to be cleared before first use
    bool ready;
    shared_texture_pool &pool;

    //reserve the SEEX * SEEY submap tiles
    submap_cache( shared_texture_pool &pool );
    submap_cache( submap_cache && );
    //handle the release of the borrowed texture
    ~submap_cache();
};


pixel_minimap::pixel_minimap( const SDL_Renderer_Ptr &renderer ) :
    renderer( renderer ),
    previous_submap_view( tripoint_min ),
    screen_rect{ 0, 0, 0, 0 }
{
}

pixel_minimap::~pixel_minimap() = default;

void pixel_minimap::set_settings( const pixel_minimap_settings &settings )
{
    reset();
    this->settings = settings;
}

//resets the touched and drawn properties of each active submap cache
void pixel_minimap::prepare_cache_for_updates()
{
    for( auto &mcp : cache ) {
        mcp.second.touched = false;
        mcp.second.drawn = false;
    }
}

//deletes the mapping of unused submap caches from the main map
//the touched flag prevents deletion
void pixel_minimap::clear_unused_cache()
{
    for( auto it = cache.begin(); it != cache.end(); ) {
        if( !it->second.touched ) {
            cache.erase( it++ );
        } else {
            it++;
        }
    }
}

//draws individual updates to the submap cache texture
//the render target will be set back to display_buffer after all submaps are updated
void pixel_minimap::process_cache_updates()
{
    SDL_Rect rectangle;
    bool draw_with_dots = false;

    switch( settings.mode ) {
        case pixel_minimap_mode::solid:
            rectangle.w = tile_size.x;
            rectangle.h = tile_size.y;
            break;

        case pixel_minimap_mode::squares:
            rectangle.w = std::max( tile_size.x - 1, 1 );
            rectangle.h = std::max( tile_size.y - 1, 1 );
            draw_with_dots = rectangle.w == 1 && rectangle.h == 1;
            break;

        case pixel_minimap_mode::dots:
            draw_with_dots = true;
            break;
    }

    for( auto &mcp : cache ) {
        if( mcp.second.update_list.empty() ) {
            continue;
        }

        SetRenderTarget( renderer, mcp.second.minimap_tex );

        //draw a default dark-colored rectangle over the texture which may have been used previously
        if( !mcp.second.ready ) {
            mcp.second.ready = true;
            SetRenderDrawColor( renderer, 0x00, 0x00, 0x00, 0xFF );
            RenderClear( renderer );
        }

        for( const point &p : mcp.second.update_list ) {
            const SDL_Color &c = mcp.second.minimap_colors[p.y * SEEX + p.x];

            SetRenderDrawColor( renderer, c.r, c.g, c.b, c.a );

            if( draw_with_dots ) {
                printErrorIf( SDL_RenderDrawPoint( renderer.get(), p.x * tile_size.x,
                                                   p.y * tile_size.y ) != 0, "SDL_RenderDrawPoint failed" );
            } else {
                rectangle.x = p.x * tile_size.x;
                rectangle.y = p.y * tile_size.y;

                render_fill_rect( renderer, rectangle, c.r, c.g, c.b );
            }
        }

        mcp.second.update_list.clear();
    }
}

void pixel_minimap::update_cache_at( const tripoint &pos )
{
    const auto &access_cache = g->m.access_cache( pos.z );
    const bool nv_goggle = g->u.get_vision_modes()[NV_GOGGLES];

    auto &cache_item = get_cache_at( g->m.get_abs_sub() + pos );

    cache_item.touched = true;

    for( int y = 0; y < SEEY; ++y ) {
        for( int x = 0; x < SEEX; ++x ) {
            const auto p = sm_to_ms_copy( pos ) + tripoint{ x, y, 0 };
            const auto lighting = access_cache.visibility_cache[p.x][p.y];

            SDL_Color color;

            if( lighting == LL_BLANK ) {
                color = { 0x00, 0x00, 0x00, 0xFF };    // TODO: Map memory.
            } else {
                if( lighting == LL_DARK ) {
                    color = { 0x0C, 0x0C, 0x0C, 0xFF };
                } else {
                    color = get_map_color_at( p );
                }

                //color terrain according to lighting conditions
                if( nv_goggle ) {
                    if( lighting == LL_LOW ) {
                        color = color_pixel_nightvision( color );
                    } else if( lighting != LL_DARK && lighting != LL_BLANK ) {
                        color = color_pixel_overexposed( color );
                    }
                } else if( lighting == LL_LOW ) {
                    color = color_pixel_grayscale( color );
                }
            }

            color = adjust_color_brightness( color, settings.brightness );

            SDL_Color &current_color = cache_item.minimap_colors[y * SEEX + x];

            if( current_color != color ) {
                current_color = color;
                cache_item.update_list.push_back( { x, y } );
            }
        }
    }
}

pixel_minimap::submap_cache &pixel_minimap::get_cache_at( const tripoint &pos )
{
    auto it = cache.find( pos );

    if( it == cache.end() ) {
        it = cache.emplace( pos, *tex_pool ).first;
    }

    return it->second;
}

pixel_minimap::submap_cache::submap_cache( shared_texture_pool &pool ) : ready( false ),
    pool( pool )
{
    //set color to force updates on a new submap texture
    minimap_colors.resize( SEEY * SEEX, SDL_Color{ 0xFF, 0xFF, 0xFF, 0xFF } );
    minimap_tex = pool.request_tex( texture_index );
}

pixel_minimap::submap_cache::~submap_cache()
{
    pool.release_tex( texture_index, std::move( minimap_tex ) );
}

pixel_minimap::submap_cache::submap_cache( submap_cache && ) = default;

void pixel_minimap::set_screen_rect( const SDL_Rect &screen_rect )
{
    if( this->screen_rect == screen_rect && main_tex && tex_pool ) {
        return;
    }

    this->screen_rect = screen_rect;

    tile_size.x = std::max( screen_rect.w / tiles_range.x, 1 );
    tile_size.y = std::max( screen_rect.h / tiles_range.y, 1 );
    //maintain a square "pixel" shape
    if( settings.square_pixels ) {
        const int smallest_size = std::min( tile_size.x, tile_size.y );
        tile_size.x = smallest_size;
        tile_size.y = smallest_size;
    }
    tiles_limit.x = std::min( screen_rect.w / tile_size.x, tiles_range.x );
    tiles_limit.y = std::min( screen_rect.h / tile_size.y, tiles_range.y );
    // Center the drawn area within the total area.
    const int border_width = std::max( ( screen_rect.w - tiles_limit.x * tile_size.x ) / 2, 0 );
    const int border_height = std::max( ( screen_rect.h - tiles_limit.y * tile_size.y ) / 2, 0 );
    //prepare the minimap clipped area
    clip_rect = SDL_Rect{
        screen_rect.x + border_width,
        screen_rect.y + border_height,
        screen_rect.w - border_width * 2,
        screen_rect.h - border_height * 2
    };

    cache.clear();

    main_tex = create_cache_texture( renderer, clip_rect.w, clip_rect.h );
    tex_pool.reset( new shared_texture_pool() );

    for( auto &elem : tex_pool->texture_pool ) {
        elem = create_cache_texture( renderer, tile_size.x * SEEX, tile_size.y * SEEY );
    }
}

void pixel_minimap::reset()
{
    cache.clear();
    main_tex.reset();
    tex_pool.reset();
}

void pixel_minimap::render_cache_to_screen( const tripoint &center )
{
    //prepare to copy to intermediate texture
    SetRenderTarget( renderer, main_tex );

    const int start_x = center.x - tiles_limit.x / 2;
    const int start_y = center.y - tiles_limit.y / 2;

    //attempt to draw the submap cache if any of its tiles are exposed in the minimap area
    //the drawn flag prevents it from being drawn more than once
    SDL_Rect drawrect;
    drawrect.w = SEEX * tile_size.x;
    drawrect.h = SEEY * tile_size.y;

    for( int y = 0; y < tiles_limit.y; y++ ) {
        if( start_y + y < min_size.y || start_y + y >= max_size.y ) {
            continue;
        }
        for( int x = 0; x < tiles_limit.x; x++ ) {
            if( start_x + x < min_size.x || start_x + x >= max_size.x ) {
                continue;
            }

            tripoint p( start_x + x, start_y + y, center.z );
            tripoint current_submap_loc = convert_tripoint_to_abs_submap( p );

            auto it = cache.find( current_submap_loc );

            //a missing submap cache should be pretty improbable
            if( it == cache.end() ) {
                continue;
            }
            if( it->second.drawn ) {
                continue;
            }
            it->second.drawn = true;

            //the position of the submap texture has to account for the actual (current) 12x12 tile size
            //the clipping rectangle handles the portions that need to hide
            tripoint drawpoint( ( p.x / SEEX ) * SEEX - start_x, ( p.y / SEEY ) * SEEY - start_y, p.z );
            drawrect.x = drawpoint.x * tile_size.x;
            drawrect.y = drawpoint.y * tile_size.y;

            RenderCopy( renderer, it->second.minimap_tex, nullptr, &drawrect );
        }
    }
    //set display buffer to main screen
    set_displaybuffer_rendertarget();
    //paint intermediate texture to screen
    RenderCopy( renderer, main_tex, nullptr, &clip_rect );
}

void pixel_minimap::render_critters_to_screen( const tripoint &center )
{
    //handles the enemy faction red highlights
    //this value should be divisible by 200
    const int indicator_length = settings.blink_interval * 200; //default is 2000 ms, 2 seconds

    int flicker = 100;
    int mixture = 0;

    if( indicator_length > 0 ) {
        const float t = get_animation_phase( 2 * indicator_length );
        const float s = std::sin( 2 * M_PI * t );

        flicker = lerp_clamped( 25, 100, std::abs( s ) );
        mixture = lerp_clamped( 0, 100, std::max( s, 0.0f ) );
    }

    const auto &access_cache = g->m.access_cache( center.z );

    const int start_x = center.x - tiles_limit.x / 2;
    const int start_y = center.y - tiles_limit.y / 2;

    for( int y = 0; y < tiles_limit.y; y++ ) {
        if( start_y + y < min_size.y || start_y + y >= max_size.y ) {
            continue;
        }
        for( int x = 0; x < tiles_limit.x; x++ ) {
            if( start_x + x < min_size.x || start_x + x >= max_size.x ) {
                continue;
            }

            const auto p = tripoint{ start_x + x, start_y + y, center.z };
            const auto lighting = access_cache.visibility_cache[p.x][p.y];

            if( lighting == LL_DARK || lighting == LL_BLANK ) {
                continue;
            }

            const auto critter = g->critter_at( p, true );

            if( critter == nullptr ) {
                continue;
            }

            if( critter != &( g->u ) && !g->u.sees( *critter ) ) {
                continue;
            }

            draw_rhombus(
                clip_rect.x + x * tile_size.x,
                clip_rect.y + y * tile_size.y,
                tile_size.x,
                get_critter_color( critter, flicker, mixture ),
                screen_rect.w,
                screen_rect.h
            );
        }
    }
}

//the main call for drawing the pixel minimap to the screen
void pixel_minimap::draw( const SDL_Rect &screen_rect, const tripoint &center )
{
    if( !g ) {
        return;
    }

    if( screen_rect.w <= 0 || screen_rect.h <= 0 ) {
        return;
    }

    set_screen_rect( screen_rect );

    //invalidate the cache if the game shifted more than one submap in the last update, or if z-level changed
    tripoint current_submap_view = g->m.get_abs_sub();
    tripoint submap_view_diff = current_submap_view - previous_submap_view;

    if( abs( submap_view_diff.x ) > 1 || abs( submap_view_diff.y ) > 1 ||
        abs( submap_view_diff.z ) > 0 ) {
        cache.clear();
    }

    previous_submap_view = current_submap_view;

    //clear leftover flags for the current draw cycle
    prepare_cache_for_updates();

    for( int y = 0; y < MAPSIZE; ++y ) {
        for( int x = 0; x < MAPSIZE; ++x ) {
            update_cache_at( { x, y, center.z } );
        }
    }

    //update minimap textures
    process_cache_updates();

    render_cache_to_screen( center );
    render_critters_to_screen( center );

    //unused submap caches get deleted
    clear_unused_cache();

}

void pixel_minimap::draw_rhombus( int destx, int desty, int size, SDL_Color color, int widthLimit,
                                  int heightLimit )
{
    for( int xOffset = -size; xOffset <= size; xOffset++ ) {
        for( int yOffset = -size + abs( xOffset ); yOffset <= size - abs( xOffset ); yOffset++ ) {
            if( xOffset < widthLimit && yOffset < heightLimit ) {
                int divisor = 2 * ( abs( yOffset ) == size - abs( xOffset ) ) + 1;
                SetRenderDrawColor( renderer, color.r / divisor, color.g / divisor, color.b / divisor, 255 );
                printErrorIf( SDL_RenderDrawPoint( renderer.get(), destx + xOffset, desty + yOffset ) != 0,
                              "SDL_RenderDrawPoint failed" );
            }
        }
    }
}
