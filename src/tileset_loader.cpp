#if defined(TILES)
#include "tileset_loader.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "cata_assert.h"
#include "cata_path.h"
#include "cata_utility.h"
#include "cuboid_rectangle.h"
#include "cursesdef.h"
#include "debug.h"
#include "flexbuffer_json.h"
#include "input.h"
#include "json_loader.h"
#include "mod_tileset.h"
#include "options.h"
#include "output.h"
#include "overlay_ordering.h"
#include "path_info.h"
#include "rect_range.h"
#include "sdl_utils.h"
#include "sdl_wrappers.h"
#include "sdltiles.h"
#include "translation.h"
#include "type_id.h"
#include "weighted_list.h"

#define dbg(x) DebugLog((x),D_SDL) << __FILE__ << ":" << __LINE__ << ": "

static const std::string ITEM_HIGHLIGHT( "highlight_item" );

namespace
{
std::string get_ascii_tile_id( const uint32_t sym, const int FG, const int BG )
{
    return std::string( { 'A', 'S', 'C', 'I', 'I', '_', static_cast<char>( sym ),
                          static_cast<char>( FG ), static_cast<char>( BG )
                        } );
}
} // namespace

static void get_tile_information( const cata_path &config_path, std::string &json_path,
                                  std::string &tileset_path, std::string &layering_path )
{
    const std::string default_json = PATH_INFO::defaulttilejson();
    const std::string default_tileset = PATH_INFO::defaulttilepng();
    const std::string default_layering = PATH_INFO::defaultlayeringjson();

    // Get JSON and TILESET vars from config
    const auto reader = [&]( std::istream & fin ) {
        while( !fin.eof() ) {
            std::string sOption;
            fin >> sOption;

            if( string_starts_with( sOption, "JSON" ) ) {
                fin >> json_path;
                dbg( D_INFO ) << "JSON path set to [" << json_path << "].";
            } else if( string_starts_with( sOption, "TILESET" ) ) {
                fin >> tileset_path;
                dbg( D_INFO ) << "TILESET path set to [" << tileset_path << "].";
            } else if( string_starts_with( sOption, "LAYERING" ) ) {
                fin >> layering_path;
                dbg( D_INFO ) << "LAYERING path set to [" << layering_path << "].";

            } else {
                getline( fin, sOption );
            }
        }
    };

    if( !read_from_file( config_path, reader ) ) {
        json_path = default_json;
        tileset_path = default_tileset;
        layering_path = default_layering;
    }

    if( json_path.empty() ) {
        json_path = default_json;
        dbg( D_INFO ) << "JSON set to default [" << json_path << "].";
    }
    if( tileset_path.empty() ) {
        tileset_path = default_tileset;
        dbg( D_INFO ) << "TILESET set to default [" << tileset_path << "].";
    }
    if( layering_path.empty() ) {
        layering_path = default_layering;
        dbg( D_INFO ) << "TILESET set to default [" << layering_path << "].";
    }
}

template<typename PixelConverter>
static SDL_Surface_Ptr apply_color_filter( const SDL_Surface_Ptr &original,
        PixelConverter pixel_converter )
{
    cata_assert( original );
    SDL_Surface_Ptr surf = create_surface_32( original->w, original->h );
    cata_assert( surf );
    throwErrorIf( BlitSurface( original, nullptr, surf, nullptr ) != 0,
                  "SDL_BlitSurface failed" );

    SDL_Color *pix = static_cast<SDL_Color *>( surf->pixels );

    for( int y = 0, ey = surf->h; y < ey; ++y ) {
        for( int x = 0, ex = surf->w; x < ex; ++x, ++pix ) {
            if( pix->a == 0x00 ) {
                // This check significantly improves the performance since
                // vast majority of pixels in the tilesets are completely transparent.
                continue;
            }
            *pix = pixel_converter( *pix );
        }
    }

    return surf;
}

// Compute the tightest bounding box containing non-transparent pixels within
// a sprite rect. The surface must be 32-bit RGBA (e.g. from create_surface_32).
// Returns coordinates relative to the sprite rect origin. A fully transparent
// sprite returns {0, 0, 0, 0}.
static SDL_Rect compute_opaque_rect( const SDL_Surface_Ptr &surf, const SDL_Rect &rect )
{
    int min_x = rect.w; // NOLINT(cata-combine-locals-into-point)
    int min_y = rect.h;
    int max_x = -1; // NOLINT(cata-combine-locals-into-point)
    int max_y = -1;
    const int pitch = surf->pitch / 4;
    const Uint32 *pixels = static_cast<const Uint32 *>( surf->pixels );
    for( int y = 0; y < rect.h; ++y ) {
        for( int x = 0; x < rect.w; ++x ) {
            const Uint32 pixel = pixels[( rect.y + y ) * pitch + ( rect.x + x )];
            Uint8 r;
            Uint8 g;
            Uint8 b;
            Uint8 a;
            GetRGBA( pixel, surf, r, g, b, a );
            if( a > 0 ) {
                if( x < min_x ) {
                    min_x = x;
                }
                if( y < min_y ) {
                    min_y = y;
                }
                if( x > max_x ) {
                    max_x = x;
                }
                if( y > max_y ) {
                    max_y = y;
                }
            }
        }
    }
    if( max_x < 0 ) {
        return { 0, 0, 0, 0 };
    }
    return { min_x, min_y, max_x - min_x + 1, max_y - min_y + 1 };
}

static bool is_contained( const SDL_Rect &smaller, const SDL_Rect &larger )
{
    return smaller.x >= larger.x &&
           smaller.y >= larger.y &&
           smaller.x + smaller.w <= larger.x + larger.w &&
           smaller.y + smaller.h <= larger.y + larger.h;
}

void tileset_cache::loader::copy_surface_to_texture( const SDL_Surface_Ptr &surf,
        const point &offset, std::vector<texture> &target,
        const std::vector<SDL_Rect> &opaque_bounds )
{
    cata_assert( surf );
    const rect_range<SDL_Rect> input_range( sprite_width, sprite_height,
                                            point( surf->w / sprite_width,
                                                    surf->h / sprite_height ) );

    const std::shared_ptr<SDL_Texture> texture_ptr = CreateTextureFromSurface( renderer, surf );
    cata_assert( texture_ptr );

    for( const SDL_Rect rect : input_range ) {
        cata_assert( offset.x % sprite_width == 0 );
        cata_assert( offset.y % sprite_height == 0 );
        const point pos( offset + point( rect.x, rect.y ) );
        cata_assert( pos.x % sprite_width == 0 );
        cata_assert( pos.y % sprite_height == 0 );
        const size_t index = this->offset + ( pos.x / sprite_width ) + ( pos.y / sprite_height ) *
                             ( tile_atlas_width / sprite_width );
        cata_assert( index < target.size() );
        cata_assert( target[index].dimension() == std::make_pair( 0, 0 ) );
        const SDL_Rect &opaque = index < opaque_bounds.size()
                                 ? opaque_bounds[index]
                                 : SDL_Rect{ 0, 0, rect.w, rect.h };
        target[index] = texture( texture_ptr, rect, opaque );
    }
}

void tileset_cache::loader::create_textures_from_tile_atlas( const SDL_Surface_Ptr &tile_atlas,
        const point &offset )
{
    cata_assert( tile_atlas );

    // Compute per-sprite opaque bounds once from the unfiltered atlas.
    // Color filter variants preserve the alpha channel, so rescanning is unnecessary.
    // Blit into a 32-bit surface for format-safe pixel access.
    SDL_Surface_Ptr scan_surf = create_surface_32( tile_atlas->w, tile_atlas->h );
    throwErrorIf( BlitSurface( tile_atlas, nullptr, scan_surf, nullptr ) != 0,
                  "SDL_BlitSurface failed" );

    const rect_range<SDL_Rect> scan_range( sprite_width, sprite_height,
                                           point( tile_atlas->w / sprite_width,
                                                   tile_atlas->h / sprite_height ) );
    // Pre-size to accommodate the maximum index this chunk can produce.
    const int cols = tile_atlas_width / sprite_width;
    const size_t max_index = this->offset +
                             static_cast<size_t>( cols ) * ( tile_atlas->h / sprite_height );
    std::vector<SDL_Rect> opaque_bounds( max_index + cols, SDL_Rect{ 0, 0, 0, 0 } );
    for( const SDL_Rect rect : scan_range ) {
        const point pos( offset + point( rect.x, rect.y ) );
        const size_t index = this->offset + ( pos.x / sprite_width ) + ( pos.y / sprite_height ) *
                             cols;
        if( index < opaque_bounds.size() ) {
            opaque_bounds[index] = compute_opaque_rect( scan_surf, rect );
        }
    }

    /** perform color filter conversion here */
    using tiles_pixel_color_entry = std::tuple<std::vector<texture>*, std::string>;
    std::array<tiles_pixel_color_entry, 6> tile_values_data = {{
            { std::make_tuple( &ts.tile_values, "color_pixel_none" ) },
            { std::make_tuple( &ts.shadow_tile_values, "color_pixel_grayscale" ) },
            { std::make_tuple( &ts.night_tile_values, "color_pixel_nightvision" ) },
            { std::make_tuple( &ts.overexposed_tile_values, "color_pixel_overexposed" ) },
            // TODO: drop tilecontext->memory_map_mode reach; loader pulls a live cata_tiles
            // option here, blocking runtime mode swaps and clean loader/renderer separation.
            { std::make_tuple( &ts.memory_tile_values, tilecontext->memory_map_mode ) },
            { std::make_tuple( &ts.silhouette_tile_values, "color_pixel_silhouette" ) }
        }
    };
    for( tiles_pixel_color_entry &entry : tile_values_data ) {
        std::vector<texture> *tile_values = std::get<0>( entry );
        color_pixel_function_pointer color_pixel_function = get_color_pixel_function( std::get<1>
                ( entry ) );
        if( !color_pixel_function ) {
            // TODO: Move it inside apply_color_filter.
            copy_surface_to_texture( tile_atlas, offset, *tile_values, opaque_bounds );
        } else {
            copy_surface_to_texture( apply_color_filter( tile_atlas, color_pixel_function ), offset,
                                     *tile_values, opaque_bounds );
        }
    }
}

template<typename T>
static void extend_vector_by( std::vector<T> &vec, const size_t additional_size )
{
    vec.resize( vec.size() + additional_size );
}

void tileset_cache::loader::load_tileset( const cata_path &img_path, const bool pump_events )
{
    cata_assert( sprite_width > 0 );
    cata_assert( sprite_height > 0 );
    const SDL_Surface_Ptr tile_atlas = load_image( img_path.get_unrelative_path().u8string().c_str() );
    cata_assert( tile_atlas );
    tile_atlas_width = tile_atlas->w;

    if( R >= 0 && R <= 255 && G >= 0 && G <= 255 && B >= 0 && B <= 255 ) {
        const Uint32 key = MapRGB( tile_atlas, 0, 0, 0 );
        throwErrorIf( SetColorKey( tile_atlas, 1, key ) != 0,
                      "SDL_SetColorKey failed" );
        throwErrorIf( SetSurfaceRLE( tile_atlas, 1 ), "SDL_SetSurfaceRLE failed" );
    }

    int max_tex_w = 0;
    int max_tex_h = 0;
    throwErrorIf( !GetRendererMaxTextureSize( renderer, &max_tex_w, &max_tex_h ),
                  "GetRendererMaxTextureSize failed" );
    // Software rendering stores textures as surfaces with run-length encoding, which makes
    // extracting a part in the middle of the texture slow. Therefore this "simulates" that the
    // renderer only supports one tile per texture.
    if( IsRendererSoftware( renderer ) ) {
        max_tex_w = sprite_width;
        max_tex_h = sprite_height;
    }
    // for debugging only: force a very small maximal texture size, as to trigger
    // splitting the tile atlas.
#if 0
    // +1 to check correct rounding
    max_tex_w = sprite_width * 10 + 1;
    max_tex_h = sprite_height * 20 + 1;
#endif

    const int min_tile_xcount = 128;
    const int min_tile_ycount = min_tile_xcount * 2;

    if( max_tex_w == 0 ) {
        max_tex_w = sprite_width * min_tile_xcount;
        DebugLog( D_INFO, DC_ALL ) << "Renderer max_texture_width was 0. "
                                   << "Changed to " << max_tex_w;
    } else {
        throwErrorIf( max_tex_w < sprite_width,
                      "Maximal texture width is smaller than tile width" );
    }

    if( max_tex_h == 0 ) {
        max_tex_h = sprite_height * min_tile_ycount;
        DebugLog( D_INFO, DC_ALL ) << "Renderer max_texture_height was 0. "
                                   << "Changed to " << max_tex_h;
    } else {
        throwErrorIf( max_tex_h < sprite_height,
                      "Maximal texture height is smaller than tile height" );
    }

    // Number of tiles in each dimension that fits into a (maximal) SDL texture.
    // If the tile atlas contains more than that, we have to split it.
    const int max_tile_xcount = max_tex_w / sprite_width;
    const int max_tile_ycount = max_tex_h / sprite_height;
    // Range over the tile atlas, wherein each rectangle fits into the maximal
    // SDL texture size. In other words: a range over the parts into which the
    // tile atlas needs to be split.
    const rect_range<SDL_Rect> output_range(
        max_tile_xcount * sprite_width,
        max_tile_ycount * sprite_height,
        point( divide_round_up( tile_atlas->w, max_tex_w ),
               divide_round_up( tile_atlas->h, max_tex_h ) ) );

    const int expected_tilecount = ( tile_atlas->w / sprite_width ) *
                                   ( tile_atlas->h / sprite_height );
    extend_vector_by( ts.tile_values, expected_tilecount );
    extend_vector_by( ts.shadow_tile_values, expected_tilecount );
    extend_vector_by( ts.night_tile_values, expected_tilecount );
    extend_vector_by( ts.overexposed_tile_values, expected_tilecount );
    extend_vector_by( ts.memory_tile_values, expected_tilecount );
    extend_vector_by( ts.silhouette_tile_values, expected_tilecount );

    for( const SDL_Rect sub_rect : output_range ) {
        cata_assert( sub_rect.x % sprite_width == 0 );
        cata_assert( sub_rect.y % sprite_height == 0 );
        cata_assert( sub_rect.w % sprite_width == 0 );
        cata_assert( sub_rect.h % sprite_height == 0 );
        SDL_Surface_Ptr smaller_surf;

        if( is_contained( SDL_Rect{ 0, 0, tile_atlas->w, tile_atlas->h }, sub_rect ) ) {
            // can use tile_atlas directly, it is completely contained in the output rectangle
        } else {
            // Need a temporary surface that contains the parts of the tile atlas that fit
            // into sub_rect. But doesn't always need to be as large as sub_rect.
            const int w = std::min( tile_atlas->w - sub_rect.x, sub_rect.w );
            const int h = std::min( tile_atlas->h - sub_rect.y, sub_rect.h );
            smaller_surf = ::create_surface_32( w, h );
            cata_assert( smaller_surf );
            const SDL_Rect inp{ sub_rect.x, sub_rect.y, w, h };
            throwErrorIf( BlitSurface( tile_atlas, &inp, smaller_surf,
                                       nullptr ) != 0, "SDL_BlitSurface failed" );
        }
        const SDL_Surface_Ptr &surf_to_use = smaller_surf ? smaller_surf : tile_atlas;
        cata_assert( surf_to_use );

        create_textures_from_tile_atlas( surf_to_use, point( sub_rect.x, sub_rect.y ) );

        if( pump_events ) {
            inp_mngr.pump_events();
        }
    }

    size = expected_tilecount;
}

void tileset_cache::loader::load( const std::string &tileset_id, const bool precheck,
                                  const bool pump_events, const bool terrain )
{
    std::string json_conf;
    std::string layering;
    std::string tileset_path;
    cata_path tileset_root;

    bool has_layering = true;

    const auto tset_iter = TILESETS.find( tileset_id );
    if( tset_iter != TILESETS.end() ) {
        tileset_root = tset_iter->second;
        dbg( D_INFO ) << '"' << tileset_id << '"' << " tileset: found config file path: " <<
                      tileset_root;
        get_tile_information( tileset_root / PATH_INFO::tileset_conf(),
                              json_conf, tileset_path, layering );
        dbg( D_INFO ) << "Current tileset is: " << tileset_id;
    } else {
        dbg( D_ERROR ) << "Tileset \"" << tileset_id << "\" from options is invalid";
        json_conf = PATH_INFO::defaulttilejson();
        tileset_path = PATH_INFO::defaulttilepng();
        layering = PATH_INFO::defaultlayeringjson();
    }

    cata_path json_path = tileset_root / std::filesystem::u8path( json_conf );
    cata_path img_path = tileset_root / std::filesystem::u8path( tileset_path );
    cata_path layering_path = tileset_root / std::filesystem::u8path( layering );

    dbg( D_INFO ) << "Attempting to Load LAYERING file " << layering_path;
    std::ifstream layering_file( layering_path.get_unrelative_path(),
                                 std::ifstream::in | std::ifstream::binary );

    if( !layering_file.good() ) {
        has_layering = false;
        //throw std::runtime_error(std::string("Failed to open layering info json: ") + layering_path);
    }

    dbg( D_INFO ) << "Attempting to Load JSON file " << json_path;
    std::optional<JsonValue> config_json = json_loader::from_path_opt( json_path );

    if( !config_json.has_value() ) {
        throw std::runtime_error( std::string( "Failed to open tile info json: " ) +
                                  json_path.generic_u8string() );
    }

    JsonObject config = ( *config_json ).get_object();
    config.allow_omitted_members();

    // "tile_info" section must exist.
    if( !config.has_member( "tile_info" ) ) {
        config.throw_error( "\"tile_info\" missing" );
    }

    for( const JsonObject curr_info : config.get_array( "tile_info" ) ) {
        ts.tile_height = curr_info.get_int( "height" );
        ts.tile_width = curr_info.get_int( "width" );
        ts.max_tile_extent = half_open_rectangle<point>( point::zero, { ts.tile_width, ts.tile_height } );
        ts.zlevel_height = curr_info.get_int( "zlevel_height", 0 );
        ts.tile_isometric = curr_info.get_bool( "iso", false );
        ts.tile_pixelscale = curr_info.get_float( "pixelscale", 1.0f );
        ts.prevent_occlusion_min_dist = curr_info.get_float( "retract_dist_min", -1.0f );
        ts.prevent_occlusion_max_dist = curr_info.get_float( "retract_dist_max", 0.0f );
    }

    if( precheck ) {
        config.allow_omitted_members();
        return;
    }

    ts.clear();

    // Load tile information if available.
    offset = 0;
    load_internal( config, tileset_root, img_path, pump_events );

    // Load mod tilesets if available
    for( const mod_tileset &mts : all_mod_tilesets ) {
        // Set sprite_id offset to separate from other tilesets.
        sprite_id_offset = offset;
        tileset_root = mts.get_base_path();
        json_path = mts.get_full_path();

        if( !mts.is_compatible( tileset_id ) ) {
            dbg( D_ERROR ) << "Mod tileset in \"" << json_path << "\" is not compatible with \""
                           << tileset_id << "\".";
            continue;
        }
        dbg( D_INFO ) << "Attempting to Load JSON file " << json_path;
        std::optional<JsonValue> mod_config_json_opt = json_loader::from_path_opt( json_path );

        if( !mod_config_json_opt.has_value() ) {
            throw std::runtime_error( std::string( "Failed to open tile info json: " ) +
                                      json_path.generic_u8string() );
        }

        JsonValue &mod_config_json = *mod_config_json_opt;

        const auto mark_visited = []( const JsonObject & jobj ) {
            // These fields have been visited in load_mod_tileset
            jobj.get_string_array( "compatibility" );
        };

        int num_in_file = 1;
        if( mod_config_json.test_array() ) {
            for( const JsonObject mod_config : mod_config_json.get_array() ) {
                if( mod_config.get_string( "type" ) == "mod_tileset" ) {
                    mark_visited( mod_config );
                    if( num_in_file == mts.num_in_file() ) {
                        // visit this if it exists, it's used elsewhere
                        if( mod_config.has_member( "compatibility" ) ) {
                            mod_config.get_member( "compatibility" );
                        }
                        load_internal( mod_config, tileset_root, img_path, pump_events );
                        break;
                    }
                    num_in_file++;
                }
            }
        } else {
            JsonObject mod_config = mod_config_json.get_object();
            mark_visited( mod_config );
            load_internal( mod_config, tileset_root, img_path, pump_events );
        }
    }

    // loop through all tile ids and eliminate empty/invalid things
    for( auto it = ts.tile_ids.begin(); it != ts.tile_ids.end(); ) {
        // second is the tile_type describing that id
        tile_type &td = it->second;
        process_variations_after_loading( td.fg );
        process_variations_after_loading( td.bg );
        // All tiles need at least foreground or background data, otherwise they are useless.
        if( td.bg.empty() && td.fg.empty() ) {
            dbg( D_ERROR ) << "tile " << it->first << " has no (valid) foreground nor background";
            ts.tile_ids.erase( it++ );
        } else {
            ++it;
        }
    }

    if( !ts.find_tile_type( terrain ? "unknown_terrain" : "unknown" ) ) {
        dbg( D_ERROR ) << "The tileset you're using has no '" << ( terrain ? "unknown_terrain" : "unknown" )
                       << "' tile defined!";
    }
    ensure_default_item_highlight();

    ts.tileset_id = tileset_id;

    // set up layering data
    if( has_layering ) {
        JsonValue layering_json = json_loader::from_path( layering_path );
        JsonObject layer_config = layering_json.get_object();
        layer_config.allow_omitted_members();

        // "variants" section must exist.
        if( !layer_config.has_member( "variants" ) ) {
            layer_config.throw_error( "\"variants\" missing" );
        }

        load_layers( layer_config );
    }

}

void tileset_cache::loader::load_internal( const JsonObject &config,
        const cata_path &tileset_root,
        const cata_path &img_path, const bool pump_events )
{
    if( config.has_array( "tiles-new" ) ) {
        // new system, several entries
        // When loading multiple tileset images this defines where
        // the tiles from the most recently loaded image start from.
        for( const JsonObject tile_part_def : config.get_array( "tiles-new" ) ) {
            const cata_path tileset_image_path = tileset_root / tile_part_def.get_string( "file" );
            R = -1;
            G = -1;
            B = -1;
            if( tile_part_def.has_object( "transparency" ) ) {
                JsonObject tra = tile_part_def.get_object( "transparency" );
                R = tra.get_int( "R" );
                G = tra.get_int( "G" );
                B = tra.get_int( "B" );
            }
            sprite_width = tile_part_def.get_int( "sprite_width", ts.tile_width );
            sprite_height = tile_part_def.get_int( "sprite_height", ts.tile_height );
            // Now load the tile definitions for the loaded tileset image.
            sprite_offset.x = tile_part_def.get_int( "sprite_offset_x", 0 );
            sprite_offset.y = tile_part_def.get_int( "sprite_offset_y", 0 );
            sprite_offset_retracted.x = tile_part_def.get_int( "sprite_offset_x_retracted", sprite_offset.x );
            sprite_offset_retracted.y = tile_part_def.get_int( "sprite_offset_y_retracted", sprite_offset.y );
            sprite_pixelscale = tile_part_def.get_float( "pixelscale", 1.0 );
            // Update maximum tile extent
            ts.max_tile_extent = half_open_rectangle<point> {
                {
                    std::min( { ts.max_tile_extent.p_min.x, sprite_offset.x, sprite_offset_retracted.x } ),
                    std::min( { ts.max_tile_extent.p_min.y, sprite_offset.y, sprite_offset_retracted.y } ),
                }, {
                    std::max( ts.max_tile_extent.p_max.x,
                              sprite_width + std::max( sprite_offset.x, sprite_offset_retracted.x ) ),
                    std::max( ts.max_tile_extent.p_max.y,
                              sprite_height + std::max( sprite_offset.y, sprite_offset_retracted.y ) ),
                }
            };
            // First load the tileset image to get the number of available tiles.
            dbg( D_INFO ) << "Attempting to Load Tileset file " << tileset_image_path;
            load_tileset( tileset_image_path, pump_events );
            load_tilejson_from_file( tile_part_def );
            if( tile_part_def.has_member( "ascii" ) ) {
                load_ascii( tile_part_def );
            }
            // Make sure the tile definitions of the next tileset image don't
            // override the current ones.
            offset += size;
            if( pump_events ) {
                inp_mngr.pump_events();
            }
        }
    } else {
        sprite_width = ts.tile_width;
        sprite_height = ts.tile_height;
        sprite_offset = point::zero;
        sprite_offset_retracted = point::zero;
        sprite_pixelscale = 1.0;
        R = -1;
        G = -1;
        B = -1;
        // old system, no tile file path entry, only one array of tiles
        dbg( D_INFO ) << "Attempting to Load Tileset file " << img_path;
        load_tileset( img_path, pump_events );
        load_tilejson_from_file( config );
        offset = size;
    }

    // allows a tileset to override the order of mutation images being applied to a character
    if( config.has_array( "overlay_ordering" ) ) {
        load_overlay_ordering_into_array( config, tileset_mutation_overlay_ordering );
    }

    // offset should be the total number of sprites loaded from every tileset image
    // eliminate any sprite references that are too high to exist
    // also eliminate negative sprite references
}

void tileset_cache::loader::load_layers( const JsonObject &config )
{
    for( const JsonObject item : config.get_array( "variants" ) ) {
        if( item.has_member( "context" ) && ( item.has_array( "item_variants" ) ||
                                              item.has_array( "field_variants" ) ) ) {

            std::string context;
            std::set<std::string> flags;
            std::string append_suffix;
            furn_str_id furn_exists;
            ter_str_id ter_exists;
            if( item.has_string( "context" ) ) {
                context = item.get_string( "context" );
                furn_exists = furn_str_id( context );
                ter_exists = ter_str_id( context );
                if( !furn_exists.is_valid() && !ter_exists.is_valid() ) {
                    debugmsg( "Layering data: %s not a valid furniture/terrain object", context );
                }
            }
            //currently, only one flag can be defined, and must be in an array
            else if( item.has_array( "context" ) ) {
                context = item.get_array( "context" ).next_value().get_string();
            }

            if( item.has_string( "append_variants" ) ) {
                append_suffix = item.get_string( "append_variants" );
                if( append_suffix.empty() ) {
                    config.throw_error( "append_variants cannot be empty string" );
                }
            }
            std::vector<layer_context_sprites> item_layers;
            std::vector<layer_context_sprites> field_layers;
            if( item.has_array( "item_variants" ) ) {
                for( const JsonObject vars : item.get_array( "item_variants" ) ) {
                    if( vars.has_member( "item" ) && vars.has_member( "layer" ) ) {
                        layer_context_sprites lcs;
                        lcs.id = vars.get_string( "item" );

                        lcs.layer = vars.get_int( "layer" );
                        point offset;
                        if( vars.has_member( "offset_x" ) ) {
                            offset.x = vars.get_int( "offset_x" );
                        }
                        if( vars.has_member( "offset_y" ) ) {
                            offset.y = vars.get_int( "offset_y" );
                        }
                        lcs.offset = offset;
                        lcs.append_suffix = append_suffix;

                        int total_weight = 0;
                        if( vars.has_array( "sprite" ) ) {
                            for( const JsonObject sprites : vars.get_array( "sprite" ) ) {
                                std::string id = sprites.get_string( "id" );
                                int weight = sprites.get_int( "weight", 1 );
                                lcs.sprite.emplace( id, weight );
                                total_weight += weight;
                            }
                        } else {
                            //default if unprovided = item name
                            lcs.sprite.emplace( lcs.id, 1 );
                            total_weight = 1;
                        }
                        lcs.total_weight = total_weight;
                        item_layers.push_back( lcs );
                    } else {
                        config.throw_error( "items configured incorrectly" );
                    }
                }
                // sort them based on layering so we can draw them correctly
                std::sort( item_layers.begin(), item_layers.end(), []( const layer_context_sprites & a,
                const layer_context_sprites & b ) {
                    return a.layer < b.layer;
                } );
                ts.item_layer_data.emplace( context, item_layers );
            }
            if( item.has_array( "field_variants" ) ) {
                for( const JsonObject vars : item.get_array( "field_variants" ) ) {
                    if( vars.has_member( "field" ) && vars.has_array( "sprite" ) ) {
                        layer_context_sprites lcs;
                        lcs.id = vars.get_string( "field" );
                        point offset;
                        if( vars.has_member( "offset_x" ) ) {
                            offset.x = vars.get_int( "offset_x" );
                        }
                        if( vars.has_member( "offset_y" ) ) {
                            offset.y = vars.get_int( "offset_y" );
                        }
                        lcs.offset = offset;

                        int total_weight = 0;
                        for( const JsonObject sprites : vars.get_array( "sprite" ) ) {
                            std::string id = sprites.get_string( "id" );
                            int weight = sprites.get_int( "weight", 1 );
                            lcs.sprite.emplace( id, weight );

                            total_weight += weight;
                        }
                        lcs.total_weight = total_weight;
                        field_layers.push_back( lcs );
                    } else {
                        config.throw_error( "fields configured incorrectly" );
                    }
                }
                ts.field_layer_data.emplace( context, field_layers );
            }
        } else {
            config.throw_error( "layering configured incorrectly" );
        }
    }

}

void tileset_cache::loader::process_variations_after_loading(
    weighted_int_list<std::vector<int>> &vs ) const
{
    // loop through all of the variations
    for( auto &v : vs ) {
        // in a given variation, erase any invalid sprite ids
        v.first.erase(
            std::remove_if(
                v.first.begin(),
                v.first.end(),
        [&]( int id ) {
            return id >= offset || id < 0;
        } ),
        v.first.end()
        );
    }
    // erase any variations with no valid sprite ids left
    vs.erase(
        std::remove_if(
            vs.begin(),
            vs.end(),
    [&]( const std::pair<std::vector<int>, int> &o ) {
        return o.first.empty();
    }
        ),
    vs.end()
    );
    // populate the bookkeeping table used for selecting sprite variations
    vs.precalc();
}

void tileset_cache::loader::add_ascii_subtile( tile_type &curr_tile, const std::string &t_id,
        int sprite_id, const std::string &s_id )
{
    const std::string m_id = t_id + "_" + s_id;
    tile_type curr_subtile;
    curr_subtile.fg.add( std::vector<int>( {sprite_id} ), 1 );
    curr_subtile.rotates = true;
    curr_tile.available_subtiles.push_back( s_id );
    ts.create_tile_type( m_id, std::move( curr_subtile ) );
}

void tileset_cache::loader::load_ascii( const JsonObject &config )
{
    if( !config.has_member( "ascii" ) ) {
        config.throw_error( "\"ascii\" section missing" );
    }
    for( const JsonObject entry : config.get_array( "ascii" ) ) {
        load_ascii_set( entry );
    }
}

void tileset_cache::loader::load_ascii_set( const JsonObject &entry )
{
    // tile for ASCII char 0 is at `in_image_offset`,
    // the other ASCII chars follow from there.
    const int in_image_offset = entry.get_int( "offset" );
    if( in_image_offset >= size ) {
        entry.throw_error_at( "offset", "invalid offset (out of range)" );
    }
    // color, of the ASCII char. Can be -1 to indicate all/default colors.
    int FG = -1;
    const std::string scolor = entry.get_string( "color", "DEFAULT" );
    if( scolor == "BLACK" ) {
        FG = catacurses::black;
    } else if( scolor == "RED" ) {
        FG = catacurses::red;
    } else if( scolor == "GREEN" ) {
        FG = catacurses::green;
    } else if( scolor == "YELLOW" ) {
        FG = catacurses::yellow;
    } else if( scolor == "BLUE" ) {
        FG = catacurses::blue;
    } else if( scolor == "MAGENTA" ) {
        FG = catacurses::magenta;
    } else if( scolor == "CYAN" ) {
        FG = catacurses::cyan;
    } else if( scolor == "WHITE" ) {
        FG = catacurses::white;
    } else if( scolor == "DEFAULT" ) {
        FG = -1;
    } else {
        entry.throw_error_at( "color", "invalid color for ASCII" );
    }
    // Add an offset for bold colors (ncurses has this bold attribute,
    // this mimics it). bold does not apply to default color.
    if( FG != -1 && entry.get_bool( "bold", false ) ) {
        FG += 8;
    }
    const int base_offset = offset + in_image_offset;
    // Finally load all 256 ASCII chars (actually extended ASCII)
    for( int ascii_char = 0; ascii_char < 256; ascii_char++ ) {
        const int index_in_image = ascii_char + in_image_offset;
        if( index_in_image < 0 || index_in_image >= size ) {
            // Out of range is ignored for now.
            continue;
        }
        const std::string id = get_ascii_tile_id( ascii_char, FG, -1 );
        tile_type curr_tile;
        curr_tile.offset = sprite_offset;
        curr_tile.offset_retracted = sprite_offset_retracted;
        curr_tile.pixelscale = sprite_pixelscale;
        auto &sprites = *curr_tile.fg.add( std::vector<int>( {index_in_image + offset} ), 1 );
        switch( ascii_char ) {
            // box bottom/top side (horizontal line)
            case LINE_OXOX_C:
                sprites[0] = 196 + base_offset;
                break;
            // box left/right side (vertical line)
            case LINE_XOXO_C:
                sprites[0] = 179 + base_offset;
                break;
            // box top left
            case LINE_OXXO_C:
                sprites[0] = 218 + base_offset;
                break;
            // box top right
            case LINE_OOXX_C:
                sprites[0] = 191 + base_offset;
                break;
            // box bottom right
            case LINE_XOOX_C:
                sprites[0] = 217 + base_offset;
                break;
            // box bottom left
            case LINE_XXOO_C:
                sprites[0] = 192 + base_offset;
                break;
            // box bottom north T (left, right, up)
            case LINE_XXOX_C:
                sprites[0] = 193 + base_offset;
                break;
            // box bottom east T (up, right, down)
            case LINE_XXXO_C:
                sprites[0] = 195 + base_offset;
                break;
            // box bottom south T (left, right, down)
            case LINE_OXXX_C:
                sprites[0] = 194 + base_offset;
                break;
            // box X (left down up right)
            case LINE_XXXX_C:
                sprites[0] = 197 + base_offset;
                break;
            // box bottom east T (left, down, up)
            case LINE_XOXX_C:
                sprites[0] = 180 + base_offset;
                break;
        }
        if( ascii_char == LINE_XOXO_C || ascii_char == LINE_OXOX_C ) {
            curr_tile.rotates = false;
            curr_tile.multitile = true;
            add_ascii_subtile( curr_tile, id, 197 + base_offset, "center" );
            add_ascii_subtile( curr_tile, id, 218 + base_offset, "corner" );
            add_ascii_subtile( curr_tile, id, 179 + base_offset, "edge" );
            add_ascii_subtile( curr_tile, id, 194 + base_offset, "t_connection" );
            add_ascii_subtile( curr_tile, id, 179 + base_offset, "end_piece" );
            add_ascii_subtile( curr_tile, id, 197 + base_offset, "unconnected" );
        }
        ts.create_tile_type( id, std::move( curr_tile ) );
    }
}

void tileset_cache::loader::load_tilejson_from_file( const JsonObject &config )
{
    if( !config.has_member( "tiles" ) ) {
        config.throw_error( "\"tiles\" section missing" );
    }

    for( const JsonObject entry : config.get_array( "tiles" ) ) {
        std::vector<std::string> ids;
        if( entry.has_string( "id" ) ) {
            ids.push_back( entry.get_string( "id" ) );
        } else if( entry.has_array( "id" ) ) {
            ids = entry.get_string_array( "id" );
        }
        for( const std::string &t_id : ids ) {
            tile_type &curr_tile = load_tile( entry, t_id );
            curr_tile.offset = sprite_offset;
            curr_tile.offset_retracted = sprite_offset_retracted;
            curr_tile.pixelscale = sprite_pixelscale;
            bool t_multi = entry.get_bool( "multitile", false );
            bool t_rota = entry.get_bool( "rotates", t_multi );
            int t_h3d = entry.get_int( "height_3d", 0 );
            if( t_multi ) {
                // fetch additional tiles
                for( const JsonObject subentry : entry.get_array( "additional_tiles" ) ) {
                    const std::string s_id = subentry.get_string( "id" );
                    const std::string m_id = str_cat( t_id, "_", s_id );
                    tile_type &curr_subtile = load_tile( subentry, m_id );
                    curr_subtile.offset = sprite_offset;
                    curr_subtile.offset_retracted = sprite_offset_retracted;
                    curr_subtile.pixelscale = sprite_pixelscale;
                    curr_subtile.rotates = true;
                    curr_subtile.height_3d = t_h3d;
                    curr_subtile.animated = subentry.get_bool( "animated", false );
                    curr_tile.available_subtiles.push_back( s_id );
                }
            } else if( entry.has_array( "additional_tiles" ) ) {
                try {
                    entry.throw_error( "Additional tiles defined, but 'multitile' is not true." );
                } catch( const JsonError &err ) {
                    debugmsg( "(json-error)\n%s", err.what() );
                }
            }
            // write the information of the base tile to curr_tile
            curr_tile.multitile = t_multi;
            curr_tile.rotates = t_rota;
            curr_tile.height_3d = t_h3d;
            curr_tile.animated = entry.get_bool( "animated", false );
        }
    }
    dbg( D_INFO ) << "Tile Width: " << ts.tile_width << " Tile Height: " << ts.tile_height <<
                  " Tile Definitions: " << ts.tile_ids.size();
}

/**
 * Load a tile definition and add it to the @ref tileset::tile_ids map.
 * All loaded tiles go into one vector (@ref tileset::tile_values), their index in it is their id.
 * The JSON data (loaded here) contains tile ids relative to the associated image.
 * They are translated into global ids by adding the @p offset, which is the number of
 * previously loaded tiles (excluding the tiles from the associated image).
 * @param id The id of the new tile definition (which is the key in @ref tileset::tile_ids).
 * Any existing definition of the same id is overridden.
 * @return A reference to the loaded tile inside the @ref tileset::tile_ids map.
 */
tile_type &tileset_cache::loader::load_tile( const JsonObject &entry, const std::string &id )
{
    if( ts.find_tile_type( id ) ) {
        ts.duplicate_ids.insert( id );
    }
    tile_type curr_subtile;

    load_tile_spritelists( entry, curr_subtile.fg, "fg" );
    load_tile_spritelists( entry, curr_subtile.bg, "bg" );

    return ts.create_tile_type( id, std::move( curr_subtile ) );
}

void tileset_cache::loader::load_tile_spritelists( const JsonObject &entry,
        weighted_int_list<std::vector<int>> &vs,
        std::string_view objname ) const
{
    // json array indicates rotations or variations
    if( entry.has_array( objname ) ) {
        JsonArray g_array = entry.get_array( objname );
        // int elements of array indicates rotations
        // create one variation, populate sprite_ids with list of ints
        if( g_array.test_int() ) {
            std::vector<int> v;
            for( const int entry : g_array ) {
                const int sprite_id = entry + sprite_id_offset;
                if( sprite_id >= 0 ) {
                    v.push_back( sprite_id );
                }
            }
            vs.add( v, 1 );
        }
        // object elements of array indicates variations
        // create one variation per object
        else if( g_array.test_object() ) {
            for( const JsonObject vo : g_array ) {
                std::vector<int> v;
                int weight = vo.get_int( "weight" );
                // negative weight is invalid
                if( weight < 0 ) {
                    vo.throw_error_at( objname, "Invalid weight for sprite variation (<0)" );
                }
                // int sprite means one sprite
                if( vo.has_int( "sprite" ) ) {
                    const int sprite_id = vo.get_int( "sprite" ) + sprite_id_offset;
                    if( sprite_id >= 0 ) {
                        v.push_back( sprite_id );
                    }
                }
                // array sprite means rotations
                else if( vo.has_array( "sprite" ) ) {
                    for( const int entry : vo.get_array( "sprite" ) ) {
                        const int sprite_id = entry + sprite_id_offset;
                        if( sprite_id >= 0 ) {
                            v.push_back( sprite_id );
                        }
                    }
                }
                if( v.size() != 1 &&
                    v.size() != 2 &&
                    v.size() != 4 ) {
                    vo.throw_error_at( objname, "Invalid number of sprites (not 1, 2, or 4)" );
                }
                vs.add( v, weight );
            }
        }
    }
    // json int indicates a single sprite id
    else if( entry.has_int( objname ) && entry.get_int( objname ) >= 0 ) {
        vs.add( std::vector<int>( {entry.get_int( objname ) + sprite_id_offset} ), 1 );
    }
}
void tileset_cache::loader::ensure_default_item_highlight()
{
    if( ts.find_tile_type( ITEM_HIGHLIGHT ) ) {
        return;
    }
    const Uint8 highlight_alpha = 127;

    int index = ts.tile_values.size();

    const SDL_Surface_Ptr surface = create_surface_32( ts.tile_width, ts.tile_height );
    cata_assert( surface );
    throwErrorIf( FillRect( surface, nullptr, MapRGBA( surface, 0, 0, 127,
                            highlight_alpha ) ) != 0, "SDL_FillRect failed" );
    ts.tile_values.emplace_back( CreateTextureFromSurface( renderer, surface ),
                                 SDL_Rect{ 0, 0, ts.tile_width, ts.tile_height } );
    ts.tile_ids[ITEM_HIGHLIGHT].fg.add( std::vector<int>( {index} ), 1 );
}

#endif // defined(TILES)
