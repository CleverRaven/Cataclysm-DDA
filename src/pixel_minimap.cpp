#include "pixel_minimap.h"

#include "coordinate_conversions.h"
#include "game.h"
#include "map.h"
#include "mapdata.h"
#include "monster.h"
#include "sdl_utils.h"
#include "options.h"
#include "player.h"
#include "vehicle.h"
#include "vpart_position.h"

#include <set>
#include <vector>

namespace
{

/// Returns a number in range [0..1]. The range lasts for @param phase_length_ms (milliseconds).
float get_animation_phase( int phase_length_ms )
{
    if( phase_length_ms == 0 ) {
        return 0.0f;
    }

    return std::fmod<float>( SDL_GetTicks(), phase_length_ms ) / phase_length_ms;
}

//converts the local x,y point into the global submap coordinates
static tripoint convert_tripoint_to_abs_submap( const tripoint &p )
{
    //get the submap coordinates of the current location
    tripoint sm_loc = ms_to_sm_copy( p );
    //add it to the absolute map coordinates
    tripoint abs_sm_loc = g->m.get_abs_sub();
    return abs_sm_loc + sm_loc;
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
        reinit();
    }

    void reinit() {
        inactive_index.clear();
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
    prep( false ),
    reinit_flag( false ),
    renderer( renderer ),
    tex_pool( new shared_texture_pool() )
{
}

pixel_minimap::~pixel_minimap() = default;

void pixel_minimap::reinit()
{
    cache.clear();
    tex_pool->texture_pool.clear();
    reinit_flag = true;
}

//creates the texture that individual minimap updates are drawn to
//later, the main texture is drawn to the display buffer
//the surface is needed to determine the color format needed by the texture
SDL_Texture_Ptr pixel_minimap::create_cache_texture( int tile_width, int tile_height )
{
    const SDL_Surface_Ptr temp = create_surface_32( tile_width, tile_height );
    assert( temp );
    SDL_Texture_Ptr tex( SDL_CreateTexture( renderer.get(), temp->format->format,
                                            SDL_TEXTUREACCESS_TARGET, tile_width, tile_height ) );
    throwErrorIf( !tex, "SDL_CreateTexture failed to create minimap texture" );
    return tex;
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

    const std::string mode = get_option<std::string>( "PIXEL_MINIMAP_MODE" );
    if( mode == "solid" ) {
        rectangle.w = tile_size.x;
        rectangle.h = tile_size.y;
    } else if( mode == "squares" ) {
        rectangle.w = std::max( tile_size.x - 1, 1 );
        rectangle.h = std::max( tile_size.y - 1, 1 );
        draw_with_dots = rectangle.w == 1 && rectangle.h == 1;
    } else if( mode == "dots" ) {
        draw_with_dots = true;
    }

    for( auto &mcp : cache ) {
        if( !mcp.second.update_list.empty() ) {
            SetRenderTarget( renderer, mcp.second.minimap_tex );

            //draw a default dark-colored rectangle over the texture which may have been used previously
            if( !mcp.second.ready ) {
                mcp.second.ready = true;
                SetRenderDrawColor( renderer, 0, 0, 0, 255 );
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
}

//finds the correct submap cache and applies the new minimap color blip if it doesn't match the current one
void pixel_minimap::update_cache( const tripoint &loc, const SDL_Color &color )
{
    tripoint current_submap_loc = convert_tripoint_to_abs_submap( loc );
    auto it = cache.find( current_submap_loc );
    if( it == cache.end() ) {
        it = cache.emplace( current_submap_loc, *tex_pool ).first;
    }

    it->second.touched = true;

    point offset( loc.x, loc.y );
    ms_to_sm_remain( offset );

    SDL_Color &current_color = it->second.minimap_colors[offset.y * SEEX + offset.x];

    if( current_color != color ) {
        current_color = color;
        it->second.update_list.push_back( offset );
    }
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

//store the known persistent values used in drawing the minimap
//since modifying the minimap properties requires a restart
void pixel_minimap::init( int destx, int desty, int width, int height )
{
    prep = true;
    min_size.x = 0;
    min_size.y = 0;
    max_size.x = MAPSIZE_X;
    max_size.y = MAPSIZE_Y;
    tiles_range.x = ( MAPSIZE - 2 ) * SEEX;
    tiles_range.y = ( MAPSIZE - 2 ) * SEEY;
    tile_size.x = std::max( width / tiles_range.x, 1 );
    tile_size.y = std::max( height / tiles_range.y, 1 );
    //maintain a square "pixel" shape
    if( get_option<bool>( "PIXEL_MINIMAP_RATIO" ) ) {
        int smallest_size = std::min( tile_size.x, tile_size.y );
        tile_size.x = smallest_size;
        tile_size.y = smallest_size;
    }
    tiles_limit.x = std::min( width / tile_size.x, tiles_range.x );
    tiles_limit.y = std::min( height / tile_size.y, tiles_range.y );
    // Center the drawn area within the total area.
    drawn_width = tiles_limit.x * tile_size.x;
    drawn_height = tiles_limit.y * tile_size.y;
    border_width = std::max( ( width - drawn_width ) / 2, 0 );
    border_height = std::max( ( height - drawn_height ) / 2, 0 );
    //prepare the minimap clipped area
    clip_rect.x = destx + border_width;
    clip_rect.y = desty + border_height;
    clip_rect.w = width - border_width * 2;
    clip_rect.h = height - border_height * 2;

    main_tex.reset();
    main_tex = create_cache_texture( clip_rect.w, clip_rect.h );

    previous_submap_view = tripoint_min;

    //allocate the textures for the texture pool
    for( auto &i : tex_pool->texture_pool ) {
        i = create_cache_texture( tile_size.x * SEEX,
                                  tile_size.y * SEEY );
    }
}

//the main call for drawing the pixel minimap to the screen
void pixel_minimap::draw( int destx, int desty, const tripoint &center, int width, int height )
{
    if( !g ) {
        return;
    }

    //set up class variables on the first run
    if( !prep || reinit_flag ) {
        reinit_flag = false;
        cache.clear();
        tex_pool->texture_pool.clear();
        tex_pool->reinit();
        init( destx, desty, width, height );
    }

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

    const int start_x = center.x - tiles_limit.x / 2;
    const int start_y = center.y - tiles_limit.y / 2;

    auto &ch = g->m.access_cache( center.z );
    //retrieve night vision goggle status once per draw
    auto vision_cache = g->u.get_vision_modes();
    bool nv_goggle = vision_cache[NV_GOGGLES];

    const int brightness = get_option<int>( "PIXEL_MINIMAP_BRIGHTNESS" );
    //check all of exposed submaps (MAPSIZE*MAPSIZE submaps) and apply new color changes to the cache
    for( int y = 0; y < MAPSIZE_Y; y++ ) {
        for( int x = 0; x < MAPSIZE_X; x++ ) {
            tripoint p( x, y, center.z );

            lit_level lighting = ch.visibility_cache[p.x][p.y];
            SDL_Color color;
            color.a = 255;
            if( lighting == LL_BLANK ) {
                color.r = 0;
                color.g = 0;
                color.b = 0;
            } else if( lighting == LL_DARK ) {
                color.r = 12;
                color.g = 12;
                color.b = 12;
            } else if( const optional_vpart_position vp = g->m.veh_at( p ) ) {
                color = curses_color_to_SDL( vp->vehicle().part_color( vp->part_index() ) );
            } else if( g->m.has_furn( p ) ) {
                auto &furniture = g->m.furn( p ).obj();
                color = curses_color_to_SDL( furniture.color() );
            } else {
                auto &terrain = g->m.ter( p ).obj();
                color = curses_color_to_SDL( terrain.color() );
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

            //add an individual color update to the cache
            update_cache( p, adjust_color_brightness( color, brightness ) );
        }
    }

    //update minimap textures
    process_cache_updates();
    //prepare to copy to intermediate texture
    SetRenderTarget( renderer, main_tex );

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

    //unused submap caches get deleted
    clear_unused_cache();

    //handles the enemy faction red highlights
    //this value should be divisible by 200
    const int indicator_length = get_option<int>( "PIXEL_MINIMAP_BLINK" ) *
                                 200; //default is 2000 ms, 2 seconds

    int flicker = 100;
    int mixture = 0;

    if( indicator_length > 0 ) {
        const float t = get_animation_phase( 2 * indicator_length );
        const float s = std::sin( 2 * M_PI * t );

        flicker = lerp_clamped( 25, 100, std::abs( s ) );
        mixture = lerp_clamped( 0, 100, std::max( s, 0.0f ) );
    }

    // Now draw critters over terrain.
    for( int y = 0; y < tiles_limit.y; y++ ) {
        if( start_y + y < min_size.y || start_y + y >= max_size.y ) {
            continue;
        }
        for( int x = 0; x < tiles_limit.x; x++ ) {
            if( start_x + x < min_size.x || start_x + x >= max_size.x ) {
                continue;
            }
            tripoint p( start_x + x, start_y + y, center.z );

            lit_level lighting = ch.visibility_cache[p.x][p.y];

            if( lighting != LL_DARK && lighting != LL_BLANK ) {
                Creature *critter = g->critter_at( p, true );
                if( critter != nullptr ) {
                    // use player::sees, otherwise shady zombies or worms will be visible early
                    if( critter == &( g->u ) || g->u.sees( *critter ) ) {
                        SDL_Color c = curses_color_to_SDL( critter->symbol_color() );
                        c.a = 255;
                        if( indicator_length > 0 ) {
                            const auto m = dynamic_cast<monster *>( critter );
                            if( m != nullptr ) {
                                //faction status (attacking or tracking) determines if red highlights get applied to creature
                                monster_attitude matt = m->attitude( &( g->u ) );
                                if( MATT_ATTACK == matt || MATT_FOLLOW == matt ) {
                                    const auto red_pixel = SDL_Color{ 0xFF, 0x0, 0x0, 0xFF };

                                    c = mix_colors( c, red_pixel, mixture );
                                    c = adjust_color_brightness( c, flicker );
                                }
                            }
                        }
                        draw_rhombus(
                            destx + border_width + x * tile_size.x,
                            desty + border_height + y * tile_size.y,
                            tile_size.x,
                            c,
                            width,
                            height
                        );
                    }
                }
            }
        }
    }
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
