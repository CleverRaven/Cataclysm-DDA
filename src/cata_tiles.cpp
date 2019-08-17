#if defined(TILES)
#include "cata_tiles.h"

#include <cmath>
#include <cstdint>
#include <algorithm>
#include <array>
#include <cassert>
#include <fstream>
#include <bitset>
#include <iterator>
#include <stdexcept>
#include <tuple>
#include <set>
#include <sstream>

#include "avatar.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "clzones.h"
#include "cursesport.h"
#include "debug.h"
#include "field.h"
#include "game.h"
#include "item.h"
#include "item_factory.h"
#include "itype.h"
#include "json.h"
#include "map.h"
#include "mapdata.h"
#include "mod_tileset.h"
#include "monster.h"
#include "monstergenerator.h"
#include "mtype.h"
#include "npc.h"
#include "output.h"
#include "overlay_ordering.h"
#include "path_info.h"
#include "player.h"
#include "pixel_minimap.h"
#include "rect_range.h"
#include "sdl_utils.h"
#include "sdl_wrappers.h"
#include "scent_map.h"
#include "sounds.h"
#include "submap.h"
#include "trap.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "weather.h"
#include "weighted_list.h"
#include "calendar.h"
#include "character.h"
#include "color.h"
#include "creature.h"
#include "cursesdef.h"
#include "int_id.h"
#include "map_memory.h"
#include "optional.h"
#include "sdltiles.h"
#include "string_id.h"
#include "tileray.h"
#include "translations.h"
#include "type_id.h"
#include "game_constants.h"

#define dbg(x) DebugLog((x),D_SDL) << __FILE__ << ":" << __LINE__ << ": "

static const std::string ITEM_HIGHLIGHT( "highlight_item" );

static const std::array<std::string, 8> multitile_keys = {{
        "center",
        "corner",
        "edge",
        "t_connection",
        "end_piece",
        "unconnected",
        "open",
        "broken"
    }
};

extern int fontwidth;
extern int fontheight;
const efftype_id effect_ridden( "ridden" );
const efftype_id effect_riding( "riding" );
static const std::string empty_string;
static const std::array<std::string, 12> TILE_CATEGORY_IDS = {{
        "", // C_NONE,
        "vehicle_part", // C_VEHICLE_PART,
        "terrain", // C_TERRAIN,
        "item", // C_ITEM,
        "furniture", // C_FURNITURE,
        "trap", // C_TRAP,
        "field", // C_FIELD,
        "lighting", // C_LIGHTING,
        "monster", // C_MONSTER,
        "bullet", // C_BULLET,
        "hit_entity", // C_HIT_ENTITY,
        "weather", // C_WEATHER,
    }
};

namespace
{

std::string get_ascii_tile_id( const uint32_t sym, const int FG, const int BG )
{
    return std::string( { 'A', 'S', 'C', 'I', 'I', '_', static_cast<char>( sym ), static_cast<char>( FG ), static_cast<char>( BG ) } );
}

pixel_minimap_mode pixel_minimap_mode_from_string( const std::string &mode )
{
    if( mode == "solid" ) {
        return pixel_minimap_mode::solid;
    } else if( mode == "squares" ) {
        return pixel_minimap_mode::squares;
    } else if( mode == "dots" ) {
        return pixel_minimap_mode::dots;
    }

    debugmsg( "Unsupported pixel minimap mode \"" + mode + "\"." );
    return pixel_minimap_mode::solid;
}

} // namespace

static int msgtype_to_tilecolor( const game_message_type type, const bool bOldMsg )
{
    const int iBold = bOldMsg ? 0 : 8;

    switch( type ) {
        case m_good:
            return iBold + catacurses::green;
        case m_bad:
            return iBold + catacurses::red;
        case m_mixed:
        case m_headshot:
            return iBold + catacurses::magenta;
        case m_neutral:
            return iBold + catacurses::white;
        case m_warning:
        case m_critical:
            return iBold + catacurses::yellow;
        case m_info:
        case m_grazing:
            return iBold + catacurses::blue;
        default:
            break;
    }

    return -1;
}

formatted_text::formatted_text( const std::string &text, const int color,
                                const direction direction )
    : text( text ), color( color )
{
    switch( direction ) {
        case NORTHWEST:
        case WEST:
        case SOUTHWEST:
            alignment = TEXT_ALIGNMENT_RIGHT;
            break;
        case NORTH:
        case CENTER:
        case SOUTH:
            alignment = TEXT_ALIGNMENT_CENTER;
            break;
        default:
            alignment = TEXT_ALIGNMENT_LEFT;
            break;
    }
}

cata_tiles::cata_tiles( const SDL_Renderer_Ptr &renderer ) :
    renderer( renderer ),
    minimap( new pixel_minimap( renderer ) )
{
    assert( renderer );

    tile_height = 0;
    tile_width = 0;
    tile_ratiox = 0;
    tile_ratioy = 0;

    in_animation = false;
    do_draw_explosion = false;
    do_draw_custom_explosion = false;
    do_draw_bullet = false;
    do_draw_hit = false;
    do_draw_line = false;
    do_draw_cursor = false;
    do_draw_highlight = false;
    do_draw_weather = false;
    do_draw_sct = false;
    do_draw_zones = false;

    nv_goggles_activated = false;

    on_options_changed();
}

cata_tiles::~cata_tiles() = default;

void cata_tiles::on_options_changed()
{
    memory_map_mode = get_option <std::string>( "MEMORY_MAP_MODE" );

    pixel_minimap_settings settings;

    settings.mode = pixel_minimap_mode_from_string( get_option<std::string>( "PIXEL_MINIMAP_MODE" ) );
    settings.brightness = get_option<int>( "PIXEL_MINIMAP_BRIGHTNESS" );
    settings.beacon_size = get_option<int>( "PIXEL_MINIMAP_BEACON_SIZE" );
    settings.beacon_blink_interval = get_option<int>( "PIXEL_MINIMAP_BLINK" );
    settings.square_pixels = get_option<bool>( "PIXEL_MINIMAP_RATIO" );
    settings.scale_to_fit = get_option<bool>( "PIXEL_MINIMAP_SCALE_TO_FIT" );

    minimap->set_settings( settings );
}

const tile_type *tileset::find_tile_type( const std::string &id ) const
{
    const auto iter = tile_ids.find( id );
    return iter != tile_ids.end() ? &iter->second : nullptr;
}

tile_type &tileset::create_tile_type( const std::string &id, tile_type &&new_tile_type )
{
    tile_type &result = tile_ids[id];
    result = std::move( new_tile_type );
    return result;
}

void cata_tiles::load_tileset( const std::string &tileset_id, const bool precheck,
                               const bool force )
{
    if( tileset_ptr && tileset_ptr->get_tileset_id() == tileset_id && !force ) {
        return;
    }
    // TODO: move into clear or somewhere else.
    // reset the overlay ordering from the previous loaded tileset
    tileset_mutation_overlay_ordering.clear();

    // Load the tileset into a separate instance and only set this->tileset_ptr
    // when the loading has succeeded.
    std::unique_ptr<tileset> new_tileset_ptr = std::make_unique<tileset>();
    tileset_loader loader( *new_tileset_ptr, renderer );
    loader.load( tileset_id, precheck );
    tileset_ptr = std::move( new_tileset_ptr );

    set_draw_scale( 16 );

    minimap->set_type( tile_iso ? pixel_minimap_type::iso : pixel_minimap_type::ortho );
}

void cata_tiles::reinit()
{
    set_draw_scale( 16 );
    RenderClear( renderer );
}

static void get_tile_information( const std::string &config_path, std::string &json_path,
                                  std::string &tileset_path )
{
    const std::string default_json = FILENAMES["defaulttilejson"];
    const std::string default_tileset = FILENAMES["defaulttilepng"];

    // Get JSON and TILESET vars from config
    const auto reader = [&]( std::istream & fin ) {
        while( !fin.eof() ) {
            std::string sOption;
            fin >> sOption;

            if( sOption.empty() ) {
                getline( fin, sOption );
            } else if( sOption[0] == '#' ) { // Skip comment
                getline( fin, sOption );
            } else if( sOption.find( "JSON" ) != std::string::npos ) {
                fin >> json_path;
                dbg( D_INFO ) << "JSON path set to [" << json_path << "].";
            } else if( sOption.find( "TILESET" ) != std::string::npos ) {
                fin >> tileset_path;
                dbg( D_INFO ) << "TILESET path set to [" << tileset_path << "].";
            } else {
                getline( fin, sOption );
            }
        }
    };

    if( !read_from_file( config_path, reader ) ) {
        json_path = default_json;
        tileset_path = default_tileset;
    }

    if( json_path.empty() ) {
        json_path = default_json;
        dbg( D_INFO ) << "JSON set to default [" << json_path << "].";
    }
    if( tileset_path.empty() ) {
        tileset_path = default_tileset;
        dbg( D_INFO ) << "TILESET set to default [" << tileset_path << "].";
    }
}

template<typename PixelConverter>
static SDL_Surface_Ptr apply_color_filter( const SDL_Surface_Ptr &original,
        PixelConverter pixel_converter )
{
    assert( original );
    SDL_Surface_Ptr surf = create_surface_32( original->w, original->h );
    assert( surf );
    throwErrorIf( SDL_BlitSurface( original.get(), nullptr, surf.get(), nullptr ) != 0,
                  "SDL_BlitSurface failed" );

    auto pix = reinterpret_cast<SDL_Color *>( surf->pixels );

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

static bool is_contained( const SDL_Rect &smaller, const SDL_Rect &larger )
{
    return smaller.x >= larger.x &&
           smaller.y >= larger.y &&
           smaller.x + smaller.w <= larger.x + larger.w &&
           smaller.y + smaller.h <= larger.y + larger.h;
}

void tileset_loader::copy_surface_to_texture( const SDL_Surface_Ptr &surf, const point &offset,
        std::vector<texture> &target )
{
    assert( surf );
    const rect_range<SDL_Rect> input_range( sprite_width, sprite_height, surf->w / sprite_width,
                                            surf->h / sprite_height );

    const std::shared_ptr<SDL_Texture> texture_ptr = CreateTextureFromSurface( renderer, surf );
    assert( texture_ptr );

    for( const SDL_Rect rect : input_range ) {
        assert( offset.x % sprite_width == 0 );
        assert( offset.y % sprite_height == 0 );
        const point pos( offset + point( rect.x, rect.y ) );
        assert( pos.x % sprite_width == 0 );
        assert( pos.y % sprite_height == 0 );
        const size_t index = this->offset + ( pos.x / sprite_width ) + ( pos.y / sprite_height ) *
                             ( tile_atlas_width / sprite_width );
        assert( index < target.size() );
        assert( target[index].dimension() == std::make_pair( 0, 0 ) );
        target[index] = texture( texture_ptr, rect );
    }
}

void tileset_loader::create_textures_from_tile_atlas( const SDL_Surface_Ptr &tile_atlas,
        const point &offset )
{
    assert( tile_atlas );

    /** perform color filter conversion here */
    using tiles_pixel_color_entry = std::tuple<std::vector<texture>*, std::string>;
    std::array<tiles_pixel_color_entry, 5> tile_values_data = {{
            { std::make_tuple( &ts.tile_values, "color_pixel_none" ) },
            { std::make_tuple( &ts.shadow_tile_values, "color_pixel_grayscale" ) },
            { std::make_tuple( &ts.night_tile_values, "color_pixel_nightvision" ) },
            { std::make_tuple( &ts.overexposed_tile_values, "color_pixel_overexposed" ) },
            { std::make_tuple( &ts.memory_tile_values, tilecontext->memory_map_mode ) }
        }
    };
    for( tiles_pixel_color_entry &entry : tile_values_data ) {
        std::vector<texture> *tile_values = std::get<0>( entry );
        color_pixel_function_pointer color_pixel_function = get_color_pixel_function( std::get<1>
                ( entry ) );
        if( !color_pixel_function ) {
            // TODO: Move it inside apply_color_filter.
            copy_surface_to_texture( tile_atlas, offset, *tile_values );
        } else {
            copy_surface_to_texture( apply_color_filter( tile_atlas, color_pixel_function ), offset,
                                     *tile_values );
        }
    }
}

template<typename T>
static void extend_vector_by( std::vector<T> &vec, const size_t additional_size )
{
    vec.resize( vec.size() + additional_size );
}

void tileset_loader::load_tileset( const std::string &img_path )
{
    const SDL_Surface_Ptr tile_atlas = load_image( img_path.c_str() );
    assert( tile_atlas );
    tile_atlas_width = tile_atlas->w;

    if( R >= 0 && R <= 255 && G >= 0 && G <= 255 && B >= 0 && B <= 255 ) {
        const Uint32 key = SDL_MapRGB( tile_atlas->format, 0, 0, 0 );
        throwErrorIf( SDL_SetColorKey( tile_atlas.get(), SDL_TRUE, key ) != 0, "SDL_SetColorKey failed" );
        throwErrorIf( SDL_SetSurfaceRLE( tile_atlas.get(), 1 ), "SDL_SetSurfaceRLE failed" );
    }

    SDL_RendererInfo info;
    throwErrorIf( SDL_GetRendererInfo( renderer.get(), &info ) != 0, "SDL_GetRendererInfo failed" );
    // Software rendering stores textures as surfaces with run-length encoding, which makes extracting a part
    // in the middle of the texture slow. Therefore this "simulates" that the renderer only supports one tile
    // per texture. Each tile will go on its own texture object.
    if( info.flags & SDL_RENDERER_SOFTWARE ) {
        info.max_texture_width = sprite_width;
        info.max_texture_height = sprite_height;
    }
    // for debugging only: force a very small maximal texture size, as to trigger
    // splitting the tile atlas.
#if 0
    // +1 to check correct rounding
    info.max_texture_width = sprite_width * 10 + 1;
    info.max_texture_height = sprite_height * 20 + 1;
#endif

    const int min_tile_xcount = 128;
    const int min_tile_ycount = min_tile_xcount * 2;

    if( info.max_texture_width == 0 ) {
        info.max_texture_width = sprite_width * min_tile_xcount;
        DebugLog( D_INFO, DC_ALL ) << "SDL_RendererInfo max_texture_width was set to 0.  Changing it to " <<
                                   info.max_texture_width;
    } else {
        throwErrorIf( info.max_texture_width < sprite_width,
                      "Maximal texture width is smaller than tile width" );
    }

    if( info.max_texture_height == 0 ) {
        info.max_texture_height = sprite_height * min_tile_ycount;
        DebugLog( D_INFO, DC_ALL ) << "SDL_RendererInfo max_texture_height was set to 0.  Changing it to "
                                   << info.max_texture_height;
    } else {
        throwErrorIf( info.max_texture_height < sprite_height,
                      "Maximal texture height is smaller than tile height" );
    }

    // Number of tiles in each dimension that fits into a (maximal) SDL texture.
    // If the tile atlas contains more than that, we have to split it.
    const int max_tile_xcount = info.max_texture_width / sprite_width;
    const int max_tile_ycount = info.max_texture_height / sprite_height;
    // Range over the tile atlas, wherein each rectangle fits into the maximal
    // SDL texture size. In other words: a range over the parts into which the
    // tile atlas needs to be split.
    const rect_range<SDL_Rect> output_range(
        max_tile_xcount * sprite_width,
        max_tile_ycount * sprite_height,
        divide_round_up( tile_atlas->w, info.max_texture_width ),
        divide_round_up( tile_atlas->h, info.max_texture_height ) );

    const int expected_tilecount = ( tile_atlas->w / sprite_width ) * ( tile_atlas->h / sprite_height );
    extend_vector_by( ts.tile_values, expected_tilecount );
    extend_vector_by( ts.shadow_tile_values, expected_tilecount );
    extend_vector_by( ts.night_tile_values, expected_tilecount );
    extend_vector_by( ts.overexposed_tile_values, expected_tilecount );
    extend_vector_by( ts.memory_tile_values, expected_tilecount );

    for( const SDL_Rect sub_rect : output_range ) {
        assert( sub_rect.x % sprite_width == 0 );
        assert( sub_rect.y % sprite_height == 0 );
        assert( sub_rect.w % sprite_width == 0 );
        assert( sub_rect.h % sprite_height == 0 );
        SDL_Surface_Ptr smaller_surf;

        if( is_contained( SDL_Rect{ 0, 0, tile_atlas->w, tile_atlas->h }, sub_rect ) ) {
            // can use tile_atlas directly, it is completely contained in the output rectangle
        } else {
            // Need a temporary surface that contains the parts of the tile atlas that fit
            // into sub_rect. But doesn't always need to be as large as sub_rect.
            const int w = std::min( tile_atlas->w - sub_rect.x, sub_rect.w );
            const int h = std::min( tile_atlas->h - sub_rect.y, sub_rect.h );
            smaller_surf = ::create_surface_32( w, h );
            assert( smaller_surf );
            const SDL_Rect inp{ sub_rect.x, sub_rect.y, w, h };
            throwErrorIf( SDL_BlitSurface( tile_atlas.get(), &inp, smaller_surf.get(), nullptr ) != 0,
                          "SDL_BlitSurface failed" );
        }
        const SDL_Surface_Ptr &surf_to_use = smaller_surf ? smaller_surf : tile_atlas;
        assert( surf_to_use );

        create_textures_from_tile_atlas( surf_to_use, point( sub_rect.x, sub_rect.y ) );
    }

    size = expected_tilecount;
}

void cata_tiles::set_draw_scale( int scale )
{
    assert( tileset_ptr );
    tile_width = tileset_ptr->get_tile_width() * tileset_ptr->get_tile_pixelscale() * scale / 16;
    tile_height = tileset_ptr->get_tile_height() * tileset_ptr->get_tile_pixelscale() * scale / 16;

    tile_ratiox = ( static_cast<float>( tile_width ) / static_cast<float>( fontwidth ) );
    tile_ratioy = ( static_cast<float>( tile_height ) / static_cast<float>( fontheight ) );
}

void tileset_loader::load( const std::string &tileset_id, const bool precheck )
{
    std::string json_conf;
    std::string tileset_path;
    std::string tileset_root;

    const auto tset_iter = TILESETS.find( tileset_id );
    if( tset_iter != TILESETS.end() ) {
        tileset_root = tset_iter->second;
        dbg( D_INFO ) << '"' << tileset_id << '"' << " tileset: found config file path: " << tileset_root;
        get_tile_information( tileset_root + '/' + FILENAMES["tileset-conf"],
                              json_conf, tileset_path );
        dbg( D_INFO ) << "Current tileset is: " << tileset_id;
    } else {
        dbg( D_ERROR ) << "Tileset \"" << tileset_id << "\" from options is invalid";
        json_conf = FILENAMES["defaulttilejson"];
        tileset_path = FILENAMES["defaulttilepng"];
    }

    std::string json_path = tileset_root + '/' + json_conf;
    std::string img_path = tileset_root + '/' + tileset_path;

    dbg( D_INFO ) << "Attempting to Load JSON file " << json_path;
    std::ifstream config_file( json_path.c_str(), std::ifstream::in | std::ifstream::binary );

    if( !config_file.good() ) {
        throw std::runtime_error( std::string( "Failed to open tile info json: " ) + json_path );
    }

    JsonIn config_json( config_file );
    JsonObject config = config_json.get_object();

    // "tile_info" section must exist.
    if( !config.has_member( "tile_info" ) ) {
        config.throw_error( "\"tile_info\" missing" );
    }

    JsonArray info = config.get_array( "tile_info" );
    while( info.has_more() ) {
        JsonObject curr_info = info.next_object();
        ts.tile_height = curr_info.get_int( "height" );
        ts.tile_width = curr_info.get_int( "width" );
        tile_iso = curr_info.get_bool( "iso", false );
        ts.tile_pixelscale = curr_info.get_float( "pixelscale", 1.0f );
    }

    if( precheck ) {
        return;
    }

    // Load tile information if available.
    offset = 0;
    load_internal( config, tileset_root, img_path );

    // Load mod tilesets if available
    for( const mod_tileset &mts : all_mod_tilesets ) {
        // Set sprite_id offset to separate from other tilesets.
        sprite_id_offset = offset;
        tileset_root = mts.get_base_path();
        json_path = mts.get_full_path();

        if( !mts.is_compatible( tileset_id ) ) {
            dbg( D_ERROR ) << "Mod tileset in \"" << json_path << "\" is not compatible.";
            continue;
        }
        dbg( D_INFO ) << "Attempting to Load JSON file " << json_path;
        std::ifstream mod_config_file( json_path.c_str(), std::ifstream::in | std::ifstream::binary );

        if( !mod_config_file.good() ) {
            throw std::runtime_error( std::string( "Failed to open tile info json: " ) + json_path );
        }

        JsonIn mod_config_json( mod_config_file );

        int num_in_file = 1;
        if( mod_config_json.test_array() ) {
            JsonArray mod_config_array = mod_config_json.get_array();
            while( mod_config_array.has_more() ) {
                JsonObject mod_config = mod_config_array.next_object();
                if( mod_config.get_string( "type" ) == "mod_tileset" ) {
                    if( num_in_file == mts.num_in_file() ) {
                        load_internal( mod_config, tileset_root, img_path );
                        break;
                    }
                    num_in_file++;
                }
            }
        } else {
            JsonObject mod_config = mod_config_json.get_object();
            load_internal( mod_config, tileset_root, img_path );
        }
    }

    // loop through all tile ids and eliminate empty/invalid things
    for( auto it = ts.tile_ids.begin(); it != ts.tile_ids.end(); ) {
        auto &td = it->second;        // second is the tile_type describing that id
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

    if( !ts.find_tile_type( "unknown" ) ) {
        dbg( D_ERROR ) << "The tileset you're using has no 'unknown' tile defined!";
    }
    ensure_default_item_highlight();

    ts.tileset_id = tileset_id;
}

void tileset_loader::load_internal( JsonObject &config, const std::string &tileset_root,
                                    const std::string &img_path )
{
    if( config.has_array( "tiles-new" ) ) {
        // new system, several entries
        // When loading multiple tileset images this defines where
        // the tiles from the most recently loaded image start from.
        JsonArray tiles_new = config.get_array( "tiles-new" );
        while( tiles_new.has_more() ) {
            JsonObject tile_part_def = tiles_new.next_object();
            const std::string tileset_image_path = tileset_root + '/' + tile_part_def.get_string( "file" );
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
            // First load the tileset image to get the number of available tiles.
            dbg( D_INFO ) << "Attempting to Load Tileset file " << tileset_image_path;
            load_tileset( tileset_image_path );
            load_tilejson_from_file( tile_part_def );
            if( tile_part_def.has_member( "ascii" ) ) {
                load_ascii( tile_part_def );
            }
            // Make sure the tile definitions of the next tileset image don't
            // override the current ones.
            offset += size;
        }
    } else {
        sprite_width = ts.tile_width;
        sprite_height = ts.tile_height;
        sprite_offset = point_zero;
        R = -1;
        G = -1;
        B = -1;
        // old system, no tile file path entry, only one array of tiles
        dbg( D_INFO ) << "Attempting to Load Tileset file " << img_path;
        load_tileset( img_path );
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

void tileset_loader::process_variations_after_loading( weighted_int_list<std::vector<int>> &vs )
{
    // loop through all of the variations
    for( auto &v : vs ) {
        // in a given variation, erase any invalid sprite ids
        v.obj.erase(
            std::remove_if(
                v.obj.begin(),
                v.obj.end(),
        [&]( int id ) {
            return id >= offset || id < 0;
        } ),
        v.obj.end()
        );
    }
    // erase any variations with no valid sprite ids left
    vs.erase(
        std::remove_if(
            vs.begin(),
            vs.end(),
    [&]( weighted_object<int, std::vector<int>> o ) {
        return o.obj.empty();
    }
        ),
    vs.end()
    );
    // populate the bookkeeping table used for selecting sprite variations
    vs.precalc();
}

void tileset_loader::add_ascii_subtile( tile_type &curr_tile, const std::string &t_id,
                                        int sprite_id,
                                        const std::string &s_id )
{
    const std::string m_id = t_id + "_" + s_id;
    tile_type curr_subtile;
    curr_subtile.fg.add( std::vector<int>( {sprite_id} ), 1 );
    curr_subtile.rotates = true;
    curr_tile.available_subtiles.push_back( s_id );
    ts.create_tile_type( m_id, std::move( curr_subtile ) );
}

void tileset_loader::load_ascii( JsonObject &config )
{
    if( !config.has_member( "ascii" ) ) {
        config.throw_error( "\"ascii\" section missing" );
    }
    JsonArray ascii = config.get_array( "ascii" );
    while( ascii.has_more() ) {
        JsonObject entry = ascii.next_object();
        load_ascii_set( entry );
    }
}

void tileset_loader::load_ascii_set( JsonObject &entry )
{
    // tile for ASCII char 0 is at `in_image_offset`,
    // the other ASCII chars follow from there.
    const int in_image_offset = entry.get_int( "offset" );
    if( in_image_offset >= size ) {
        entry.throw_error( "invalid offset (out of range)", "offset" );
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
        entry.throw_error( "invalid color for ASCII", "color" );
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
        auto &sprites = *( curr_tile.fg.add( std::vector<int>( {index_in_image + offset} ), 1 ) );
        switch( ascii_char ) {
            // box bottom/top side (horizontal line)
            case LINE_OXOX_C:
                sprites[0] = 205 + base_offset;
                break;
            // box left/right side (vertical line)
            case LINE_XOXO_C:
                sprites[0] = 186 + base_offset;
                break;
            // box top left
            case LINE_OXXO_C:
                sprites[0] = 201 + base_offset;
                break;
            // box top right
            case LINE_OOXX_C:
                sprites[0] = 187 + base_offset;
                break;
            // box bottom right
            case LINE_XOOX_C:
                sprites[0] = 188 + base_offset;
                break;
            // box bottom left
            case LINE_XXOO_C:
                sprites[0] = 200 + base_offset;
                break;
            // box bottom north T (left, right, up)
            case LINE_XXOX_C:
                sprites[0] = 202 + base_offset;
                break;
            // box bottom east T (up, right, down)
            case LINE_XXXO_C:
                sprites[0] = 208 + base_offset;
                break;
            // box bottom south T (left, right, down)
            case LINE_OXXX_C:
                sprites[0] = 203 + base_offset;
                break;
            // box X (left down up right)
            case LINE_XXXX_C:
                sprites[0] = 206 + base_offset;
                break;
            // box bottom east T (left, down, up)
            case LINE_XOXX_C:
                sprites[0] = 184 + base_offset;
                break;
        }
        if( ascii_char == LINE_XOXO_C || ascii_char == LINE_OXOX_C ) {
            curr_tile.rotates = false;
            curr_tile.multitile = true;
            add_ascii_subtile( curr_tile, id, 206 + base_offset, "center" );
            add_ascii_subtile( curr_tile, id, 201 + base_offset, "corner" );
            add_ascii_subtile( curr_tile, id, 186 + base_offset, "edge" );
            add_ascii_subtile( curr_tile, id, 203 + base_offset, "t_connection" );
            add_ascii_subtile( curr_tile, id, 210 + base_offset, "end_piece" );
            add_ascii_subtile( curr_tile, id, 219 + base_offset, "unconnected" );
        }
        ts.create_tile_type( id, std::move( curr_tile ) );
    }
}

void tileset_loader::load_tilejson_from_file( JsonObject &config )
{
    if( !config.has_member( "tiles" ) ) {
        config.throw_error( "\"tiles\" section missing" );
    }

    JsonArray tiles = config.get_array( "tiles" );
    while( tiles.has_more() ) {
        JsonObject entry = tiles.next_object();

        std::vector<std::string> ids;
        if( entry.has_string( "id" ) ) {
            ids.push_back( entry.get_string( "id" ) );
        } else if( entry.has_array( "id" ) ) {
            ids = entry.get_string_array( "id" );
        }
        for( const std::string &t_id : ids ) {
            tile_type &curr_tile = load_tile( entry, t_id );
            curr_tile.offset = sprite_offset;
            bool t_multi = entry.get_bool( "multitile", false );
            bool t_rota = entry.get_bool( "rotates", t_multi );
            int t_h3d = entry.get_int( "height_3d", 0 );
            if( t_multi ) {
                // fetch additional tiles
                JsonArray subentries = entry.get_array( "additional_tiles" );
                while( subentries.has_more() ) {
                    JsonObject subentry = subentries.next_object();
                    const std::string s_id = subentry.get_string( "id" );
                    const std::string m_id = t_id + "_" + s_id;
                    tile_type &curr_subtile = load_tile( subentry, m_id );
                    curr_subtile.offset = sprite_offset;
                    curr_subtile.rotates = true;
                    curr_subtile.height_3d = t_h3d;
                    curr_tile.available_subtiles.push_back( s_id );
                }
            }
            // write the information of the base tile to curr_tile
            curr_tile.multitile = t_multi;
            curr_tile.rotates = t_rota;
            curr_tile.height_3d = t_h3d;
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
 * @param id The id of the new tile definition (which is the key in @ref tileset::tile_ids). Any existing
 * definition of the same id is overridden.
 * @return A reference to the loaded tile inside the @ref tileset::tile_ids map.
 */
tile_type &tileset_loader::load_tile( JsonObject &entry, const std::string &id )
{
    tile_type curr_subtile;

    load_tile_spritelists( entry, curr_subtile.fg, "fg" );
    load_tile_spritelists( entry, curr_subtile.bg, "bg" );

    return ts.create_tile_type( id, std::move( curr_subtile ) );
}

void tileset_loader::load_tile_spritelists( JsonObject &entry,
        weighted_int_list<std::vector<int>> &vs,
        const std::string &objname )
{
    // json array indicates rotations or variations
    if( entry.has_array( objname ) ) {
        JsonArray g_array = entry.get_array( objname );
        // int elements of array indicates rotations
        // create one variation, populate sprite_ids with list of ints
        if( g_array.test_int() ) {
            std::vector<int> v;
            while( g_array.has_more() ) {
                const int sprite_id = g_array.next_int() + sprite_id_offset;
                if( sprite_id >= 0 ) {
                    v.push_back( sprite_id );
                }
            }
            vs.add( v, 1 );
        }
        // object elements of array indicates variations
        // create one variation per object
        else if( g_array.test_object() ) {
            while( g_array.has_more() ) {
                std::vector<int> v;
                JsonObject vo = g_array.next_object();
                int weight = vo.get_int( "weight" );
                // negative weight is invalid
                if( weight < 0 ) {
                    vo.throw_error( "Invalid weight for sprite variation (<0)", objname );
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
                    JsonArray sprites = vo.get_array( "sprite" );
                    while( sprites.has_more() ) {
                        const int sprite_id = sprites.next_int() + sprite_id_offset;
                        if( sprite_id >= 0 && sprite_id < size ) {
                            v.push_back( sprite_id );
                        } else {
                            // vo.throw_error("Invalid value for sprite id (out of range)", objname);
                            v.push_back( sprite_id + offset );
                        }
                    }
                }
                if( v.size() != 1 &&
                    v.size() != 2 &&
                    v.size() != 4 ) {
                    vo.throw_error( "Invalid number of sprites (not 1, 2, or 4)", objname );
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

struct tile_render_info {
    const tripoint pos;
    int height_3d = 0; // accumulator for 3d tallness of sprites rendered here so far
    tile_render_info( const tripoint &pos, int height_3d ) : pos( pos ), height_3d( height_3d ) {
    }
};

void cata_tiles::draw( int destx, int desty, const tripoint &center, int width, int height,
                       std::multimap<point, formatted_text> &overlay_strings, color_block_overlay_container &color_blocks )
{
    if( !g ) {
        return;
    }

#if defined(__ANDROID__)
    // Attempted bugfix for Google Play crash - prevent divide-by-zero if no tile width/height specified
    if( tile_width == 0 || tile_height == 0 ) {
        return;
    }
#endif

    {
        //set clipping to prevent drawing over stuff we shouldn't
        SDL_Rect clipRect = {destx, desty, width, height};
        printErrorIf( SDL_RenderSetClipRect( renderer.get(), &clipRect ) != 0,
                      "SDL_RenderSetClipRect failed" );

        //fill render area with black to prevent artifacts where no new pixels are drawn
        render_fill_rect( renderer, clipRect, 0, 0, 0 );
    }

    const point pos = center.xy();

    int sx = 0;
    int sy = 0;
    get_window_tile_counts( width, height, sx, sy );

    init_light();
    g->m.update_visibility_cache( center.z );
    const visibility_variables &cache = g->m.get_visibility_variables_cache();

    const bool iso_mode = tile_iso;

    o = iso_mode ? pos : pos - point( POSX, POSY );

    op = point( destx, desty );
    // Rounding up to include incomplete tiles at the bottom/right edges
    screentile_width = divide_round_up( width, tile_width );
    screentile_height = divide_round_up( height, tile_height );

    const int min_col = 0;
    const int max_col = sx;
    const int min_row = 0;
    const int max_row = sy;

    //limit the render area to maximum view range (121x121 square centered on player)
    const int min_visible_x = g->u.posx() % SEEX;
    const int min_visible_y = g->u.posy() % SEEY;
    const int max_visible_x = ( g->u.posx() % SEEX ) + ( MAPSIZE - 1 ) * SEEX;
    const int max_visible_y = ( g->u.posy() % SEEY ) + ( MAPSIZE - 1 ) * SEEY;

    tripoint temp;
    temp.z = center.z;
    int &x = temp.x;
    int &y = temp.y;
    auto &ch = g->m.access_cache( temp.z );

    //set up a default tile for the edges outside the render area
    visibility_type offscreen_type = VIS_DARK;
    if( cache.u_is_boomered ) {
        offscreen_type = VIS_BOOMER_DARK;
    }

    //retrieve night vision goggle status once per draw
    auto vision_cache = g->u.get_vision_modes();
    nv_goggles_activated = vision_cache[NV_GOGGLES];

    // check that the creature for which we'll draw the visibility map is still alive at that point
    if( g->displaying_visibility && g->displaying_visibility_creature != nullptr )  {
        const Creature *creature = g->displaying_visibility_creature;
        const auto is_same_creature_predicate = [&creature]( const Creature & c ) {
            return creature == &c;
        };
        if( g->get_creature_if( is_same_creature_predicate ) == nullptr )  {
            g->displaying_visibility_creature = nullptr;
        }
    }

    for( int row = min_row; row < max_row; row ++ ) {
        std::vector<tile_render_info> draw_points;
        draw_points.reserve( max_col );
        for( int col = min_col; col < max_col; col ++ ) {
            if( iso_mode ) {
                //in isometric, rows and columns represent a checkerboard screen space, and we place
                //the appropriate tile in valid squares by getting position relative to the screen center.
                if( modulo( row + o.y, 2 ) != modulo( col + o.x, 2 ) ) {
                    continue;
                }
                x = ( col - row - sx / 2 + sy / 2 ) / 2 + o.x;
                y = ( row + col - sy / 2 - sx / 2 ) / 2 + o.y;
            } else {
                x = col + o.x;
                y = row + o.y;
            }
            if( y < min_visible_y || y > max_visible_y || x < min_visible_x || x > max_visible_x ) {
                int height_3d = 0;
                if( !draw_terrain_from_memory( tripoint( x, y, center.z ), height_3d ) ) {
                    apply_vision_effects( temp, offscreen_type );
                }
                continue;
            }

            // Add scent value to the overlay_strings list for every visible tile when displaying scent
            if( g->displaying_scent ) {
                const int scent_value = g->scent.get( {x, y, center.z} );
                if( scent_value > 0 ) {
                    overlay_strings.emplace( player_to_screen( x, y ) + point( tile_width / 2, 0 ),
                                             formatted_text( std::to_string( scent_value ), 8 + catacurses::yellow,
                                                     NORTH ) );
                }
            }

            if( g->displaying_radiation ) {
                const int rad_value = g->m.get_radiation( {x, y, center.z} );
                catacurses::base_color col;
                if( rad_value > 0 ) {
                    col = catacurses::green;
                } else {
                    col = catacurses::cyan;
                }
                overlay_strings.emplace( player_to_screen( x, y ) + point( tile_width / 2, 0 ),
                                         formatted_text( std::to_string( rad_value ), 8 + col, NORTH ) );
            }

            // Add temperature value to the overlay_strings list for every visible tile when displaying temperature
            if( g->displaying_temperature ) {
                int temp_value = g->weather.get_temperature( {x, y, center.z} );
                int ctemp = temp_to_celsius( temp_value );
                short color;
                const short bold = 8;
                if( ctemp > 40 ) {
                    color = catacurses::red;
                } else if( ctemp > 25 ) {
                    color = catacurses::yellow + bold;
                } else if( ctemp > 10 ) {
                    color = catacurses::green + bold;
                } else if( ctemp > 0 ) {
                    color = catacurses::white + bold;
                } else if( ctemp > -10 ) {
                    color = catacurses::cyan + bold;
                } else {
                    color = catacurses::blue + bold;
                }
                if( get_option<std::string>( "USE_CELSIUS" ) == "celsius" ) {
                    temp_value = temp_to_celsius( temp_value );
                } else if( get_option<std::string>( "USE_CELSIUS" ) == "kelvin" ) {
                    temp_value = temp_to_kelvin( temp_value );

                }
                overlay_strings.emplace( player_to_screen( x, y ) + point( tile_width / 2, 0 ),
                                         formatted_text( std::to_string( temp_value ), color,
                                                 NORTH ) );
            }

            if( g->displaying_visibility && ( g->displaying_visibility_creature != nullptr ) ) {
                const bool visibility = g->displaying_visibility_creature->sees( { x, y, center.z } );

                // color overlay.
                auto block_color = visibility ? windowsPalette[catacurses::green] : SDL_Color{ 192, 192, 192, 255 };
                block_color.a = 100;
                color_blocks.first = SDL_BLENDMODE_BLEND;
                color_blocks.second.emplace( player_to_screen( x, y ), block_color );

                // overlay string
                std::string visibility_str = visibility ? "+" : "-";
                overlay_strings.emplace( player_to_screen( x, y ) + point( tile_width / 4, tile_height / 4 ),
                                         formatted_text( visibility_str, catacurses::black, NORTH ) );
            }

            if( apply_vision_effects( temp, g->m.get_visibility( ch.visibility_cache[x][y], cache ) ) ) {
                int height_3d = 0;
                draw_terrain_from_memory( tripoint( x, y, center.z ), height_3d );
                const auto critter = g->critter_at( tripoint( x, y, center.z ), true );
                if( critter != nullptr && g->u.sees_with_infrared( *critter ) ) {
                    // TODO: defer drawing this until later when we know how tall
                    //     the terrain/furniture under the creature is.
                    draw_from_id_string( "infrared_creature", C_NONE, empty_string, temp, 0, 0, LL_LIT, false );
                }
                continue;
            }

            int height_3d = 0;

            // light level is now used for choosing between grayscale filter and normal lit tiles.
            // Draw Terrain if possible. If not possible then we need to continue on to the next part of loop
            if( !draw_terrain( tripoint( x, y, center.z ), ch.visibility_cache[x][y], height_3d ) ) {
                continue;
            }

            draw_points.push_back( tile_render_info( tripoint( x, y, center.z ), height_3d ) );
        }
        const std::array<decltype( &cata_tiles::draw_furniture ), 10> drawing_layers = {{
                &cata_tiles::draw_furniture, &cata_tiles::draw_graffiti, &cata_tiles::draw_trap,
                &cata_tiles::draw_field_or_item, &cata_tiles::draw_vpart,
                &cata_tiles::draw_vpart_below, &cata_tiles::draw_critter_at_below,
                &cata_tiles::draw_terrain_below, &cata_tiles::draw_critter_at,
                &cata_tiles::draw_zone_mark
            }
        };
        // for each of the drawing layers in order, back to front ...
        for( auto f : drawing_layers ) {
            // ... draw all the points we drew terrain for, in the same order
            for( auto &p : draw_points ) {
                ( this->*f )( p.pos, ch.visibility_cache[p.pos.x][p.pos.y], p.height_3d );
            }
        }
    }

    //Memorize everything the character just saw even if it wasn't displayed.
    for( int mem_y = 0; mem_y < MAPSIZE_Y; mem_y++ ) {
        for( int mem_x = 0; mem_x < MAPSIZE_X; mem_x++ ) {
            tripoint p( mem_x, mem_y, center.z );
            //just finished o through s+o so skip them
            rectangle skip_region( o, o + point( sx, sy ) );
            if( skip_region.contains_half_open( p.xy() ) ) {
                continue;
            }
            int height_3d = 0;
            if( iso_mode ) {
                //Iso_mode skips in a checkerboard
                if( modulo( mem_y, 2 ) != modulo( mem_x, 2 ) ) {
                    continue;
                }
                //in isometric, rows and columns represent a checkerboard screen space, and we place
                //the appropriate tile in valid squares by getting position relative to the screen center.
                //The MAPSIZE_X/2 offset is to keep the rectangle in the upper right quadrant.
                p.x = ( mem_x - mem_y - MAPSIZE_X / 2 + MAPSIZE_Y / 2 ) / 2 + MAPSIZE_X / 2;
                p.y = ( mem_y + mem_x - MAPSIZE_Y / 2 - MAPSIZE_X / 2 ) / 2 + MAPSIZE_Y / 2;
                //Check if we're in previously done iso_mode space
                if( p.x >= ( 0 - sy - sx / 2 + sy / 2 ) / 2 + o.x && p.x < ( sx - 0 - sx / 2 + sy / 2 ) / 2 + o.x &&
                    p.y >= ( 0 + 0 - sy / 2 - sx / 2 ) / 2 + o.y && p.y < ( sy + sx - sy / 2 - sx / 2 ) / 2 + o.y ) {
                    continue;
                }
            }
            lit_level lighting = ch.visibility_cache[p.x][p.y];
            if( apply_vision_effects( p, g->m.get_visibility( lighting, cache ) ) ) {
                continue;
            }
            //calling draw to memorize everything.
            draw_terrain( p, lighting, height_3d );
            draw_furniture( p, lighting, height_3d );
            draw_trap( p, lighting, height_3d );
            draw_vpart( p, lighting, height_3d );
        }
    }

    in_animation = do_draw_explosion || do_draw_custom_explosion ||
                   do_draw_bullet || do_draw_hit || do_draw_line ||
                   do_draw_cursor || do_draw_highlight || do_draw_weather ||
                   do_draw_sct || do_draw_zones;

    draw_footsteps_frame();
    if( in_animation ) {
        if( do_draw_explosion ) {
            draw_explosion_frame();
        }
        if( do_draw_custom_explosion ) {
            draw_custom_explosion_frame();
        }
        if( do_draw_bullet ) {
            draw_bullet_frame();
        }
        if( do_draw_hit ) {
            draw_hit_frame();
            void_hit();
        }
        if( do_draw_line ) {
            draw_line();
            void_line();
        }
        if( do_draw_weather ) {
            draw_weather_frame();
            void_weather();
        }
        if( do_draw_sct ) {
            draw_sct_frame( overlay_strings );
            void_sct();
        }
        if( do_draw_zones ) {
            draw_zones_frame();
            void_zones();
        }
        if( do_draw_cursor ) {
            draw_cursor();
            void_cursor();
        }
        if( do_draw_highlight ) {
            draw_highlight();
            void_highlight();
        }
    } else if( g->u.view_offset != tripoint_zero && !g->u.in_vehicle ) {
        // check to see if player is located at ter
        draw_from_id_string( "cursor", C_NONE, empty_string,
                             tripoint( g->ter_view_p.xy(), center.z ), 0, 0, LL_LIT,
                             false );
    }
    if( g->u.controlling_vehicle ) {
        if( cata::optional<tripoint> indicator_offset = g->get_veh_dir_indicator_location( true ) ) {
            draw_from_id_string( "cursor", C_NONE, empty_string, indicator_offset->xy() + tripoint( g->u.posx(),
                                 g->u.posy(), center.z ),
                                 0, 0, LL_LIT, false );
        }
    }

    printErrorIf( SDL_RenderSetClipRect( renderer.get(), nullptr ) != 0,
                  "SDL_RenderSetClipRect failed" );
}

void cata_tiles::draw_minimap( int destx, int desty, const tripoint &center, int width, int height )
{
    minimap->draw( SDL_Rect{ destx, desty, width, height }, center );
}

void cata_tiles::get_window_tile_counts( const int width, const int height, int &columns,
        int &rows ) const
{
    if( tile_iso ) {
        columns = ceil( static_cast<double>( width ) / tile_width ) * 2 + 4;
        rows = ceil( static_cast<double>( height ) / ( tile_width / 2.0 - 1 ) ) * 2 + 4;
    } else {
        columns = ceil( static_cast<double>( width ) / tile_width );
        rows = ceil( static_cast<double>( height ) / tile_height );
    }
}

bool cata_tiles::draw_from_id_string( std::string id, const tripoint &pos, int subtile, int rota,
                                      lit_level ll, bool apply_night_vision_goggles )
{
    int nullint = 0;
    return cata_tiles::draw_from_id_string( std::move( id ), C_NONE, empty_string, pos, subtile, rota,
                                            ll, apply_night_vision_goggles, nullint );
}

bool cata_tiles::draw_from_id_string( std::string id, TILE_CATEGORY category,
                                      const std::string &subcategory, const tripoint &pos,
                                      int subtile, int rota, lit_level ll,
                                      bool apply_night_vision_goggles )
{
    int nullint = 0;
    return cata_tiles::draw_from_id_string( id, category, subcategory, pos, subtile, rota,
                                            ll, apply_night_vision_goggles, nullint );
}

bool cata_tiles::draw_from_id_string( std::string id, const tripoint &pos, int subtile, int rota,
                                      lit_level ll, bool apply_night_vision_goggles, int &height_3d )
{
    return cata_tiles::draw_from_id_string( std::move( id ), C_NONE, empty_string, pos, subtile, rota,
                                            ll, apply_night_vision_goggles, height_3d );
}

const tile_type *cata_tiles::find_tile_with_season( std::string &id )
{
    constexpr size_t suffix_len = 15;
    constexpr char season_suffix[4][suffix_len] = {
        "_season_spring", "_season_summer", "_season_autumn", "_season_winter"
    };

    std::string seasonal_id = id + season_suffix[season_of_year( calendar::turn )];

    const tile_type *tt = tileset_ptr->find_tile_type( seasonal_id );
    if( tt ) {
        id = seasonal_id;
    } else {
        tt = tileset_ptr->find_tile_type( id );
    }
    return tt;
}

const tile_type *cata_tiles::find_tile_looks_like( std::string &id, TILE_CATEGORY category )
{
    std::string looks_like = id;
    for( int cnt = 0; cnt < 10 && !looks_like.empty(); cnt++ ) {
        const tile_type *lltt = find_tile_with_season( looks_like );
        if( lltt ) {
            id = looks_like;
            return lltt;
        }
        if( category == C_FURNITURE ) {
            const furn_str_id fid( looks_like );
            if( !fid.is_valid() ) {
                return nullptr;
            }
            const furn_t &furn = fid.obj();
            looks_like = furn.looks_like;
        } else if( category == C_TERRAIN ) {
            const ter_str_id tid( looks_like );
            if( !tid.is_valid() ) {
                return nullptr;
            }
            const ter_t &ter = tid.obj();
            looks_like = ter.looks_like;
        } else if( category == C_MONSTER ) {
            const mtype_id mid( looks_like );
            if( !mid.is_valid() ) {
                return nullptr;
            }
            const mtype &mt = mid.obj();
            looks_like = mt.looks_like;
        } else if( category == C_VEHICLE_PART ) {
            // vehicle parts start with vp_ for their tiles, but not their IDs
            const vpart_id new_vpid( looks_like.substr( 3 ) );
            if( !new_vpid.is_valid() ) {
                return nullptr;
            }
            const vpart_info &new_vpi = new_vpid.obj();
            looks_like = "vp_" + new_vpi.looks_like;
        } else if( category == C_ITEM ) {
            if( !item::type_is_defined( looks_like ) ) {
                if( looks_like.substr( 0, 7 ) == "corpse_" ) {
                    looks_like = "corpse";
                    continue;
                }
                return nullptr;
            }
            const itype *new_it = item::find_type( looks_like );
            looks_like = new_it->looks_like;
        } else {
            return nullptr;
        }
    }
    return nullptr;
}

bool cata_tiles::find_overlay_looks_like( const bool male, const std::string &overlay,
        std::string &draw_id )
{
    bool exists = false;

    std::string looks_like;
    std::string over_type;

    if( overlay.substr( 0, 5 ) == "worn_" ) {
        looks_like = overlay.substr( 5 );
        over_type = "worn_";
    } else if( overlay.substr( 0, 8 ) == "wielded_" ) {
        looks_like = overlay.substr( 8 );
        over_type = "wielded_";
    } else {
        looks_like = overlay;
    }

    for( int cnt = 0; cnt < 10 && !looks_like.empty(); cnt++ ) {
        draw_id = ( male ? "overlay_male_" : "overlay_female_" ) + over_type + looks_like;
        if( tileset_ptr->find_tile_type( draw_id ) ) {
            exists = true;
            break;
        }
        draw_id = "overlay_" + over_type + looks_like;
        if( tileset_ptr->find_tile_type( draw_id ) ) {
            exists = true;
            break;
        }
        if( looks_like.substr( 0, 16 ) == "mutation_active_" ) {
            looks_like = "mutation_" + looks_like.substr( 16 );
            continue;
        }
        if( !item::type_is_defined( looks_like ) ) {
            break;
        }
        const itype *new_it = item::find_type( looks_like );
        looks_like = new_it->looks_like;
    }
    return exists;
}

bool cata_tiles::draw_from_id_string( std::string id, TILE_CATEGORY category,
                                      const std::string &subcategory, const tripoint &pos,
                                      int subtile, int rota, lit_level ll,
                                      bool apply_night_vision_goggles, int &height_3d )
{
    // If the ID string does not produce a drawable tile
    // it will revert to the "unknown" tile.
    // The "unknown" tile is one that is highly visible so you kinda can't miss it :D

    // check to make sure that we are drawing within a valid area
    // [0->width|height / tile_width|height]

    rectangle screen_bounds( o, o + point( screentile_width, screentile_height ) );
    if( !tile_iso &&
        !screen_bounds.contains_half_open( pos.xy() ) ) {
        return false;
    }

    const tile_type *tt = find_tile_looks_like( id, category );

    if( !tt ) {
        uint32_t sym = UNKNOWN_UNICODE;
        nc_color col = c_white;
        if( category == C_FURNITURE ) {
            const furn_str_id fid( id );
            if( fid.is_valid() ) {
                const furn_t &f = fid.obj();
                sym = f.symbol();
                col = f.color();
            }
        } else if( category == C_TERRAIN ) {
            const ter_str_id tid( id );
            if( tid.is_valid() ) {
                const ter_t &t = tid.obj();
                sym = t.symbol();
                col = t.color();
            }
        } else if( category == C_MONSTER ) {
            const mtype_id mid( id );
            if( mid.is_valid() ) {
                const mtype &mt = mid.obj();
                sym = UTF8_getch( mt.sym );
                col = mt.color;
            }
        } else if( category == C_VEHICLE_PART ) {
            const vpart_id vpid( id.substr( 3 ) );
            if( vpid.is_valid() ) {
                const vpart_info &v = vpid.obj();

                if( subtile == open_ ) {
                    sym = '\'';
                } else if( subtile == broken ) {
                    sym = v.sym_broken;
                } else {
                    sym = v.sym;
                }
                subtile = -1;

                tileray face = tileray( rota );
                sym = special_symbol( face.dir_symbol( sym ) );
                rota = 0;

                col = v.color;
            }
        } else if( category == C_FIELD ) {
            const field_type_id fid = field_type_id( id );
            sym = fid.obj().get_codepoint();
            // TODO: field intensity?
            col = fid.obj().get_color();
        } else if( category == C_TRAP ) {
            const trap_str_id tmp( id );
            if( tmp.is_valid() ) {
                const trap &t = tmp.obj();
                sym = t.sym;
                col = t.color;
            }
        } else if( category == C_ITEM ) {
            item tmp;
            if( 0 == id.compare( 0, 7, "corpse_" ) ) {
                tmp = item( "corpse", 0 );
            } else {
                tmp = item( id, 0 );
            }
            sym = tmp.symbol().empty() ? ' ' : tmp.symbol().front();
            col = tmp.color();
        }
        // Special cases for walls
        switch( sym ) {
            case LINE_XOXO:
                sym = LINE_XOXO_C;
                break;
            case LINE_OXOX:
                sym = LINE_OXOX_C;
                break;
            case LINE_XXOO:
                sym = LINE_XXOO_C;
                break;
            case LINE_OXXO:
                sym = LINE_OXXO_C;
                break;
            case LINE_OOXX:
                sym = LINE_OOXX_C;
                break;
            case LINE_XOOX:
                sym = LINE_XOOX_C;
                break;
            case LINE_XXXO:
                sym = LINE_XXXO_C;
                break;
            case LINE_XXOX:
                sym = LINE_XXOX_C;
                break;
            case LINE_XOXX:
                sym = LINE_XOXX_C;
                break;
            case LINE_OXXX:
                sym = LINE_OXXX_C;
                break;
            case LINE_XXXX:
                sym = LINE_XXXX_C;
                break;
            default:
                break; // sym goes unchanged
        }
        if( sym != 0 && sym < 256 ) {
            // see cursesport.cpp, function wattron
            const int pairNumber = col.to_color_pair_index();
            const cata_cursesport::pairs &colorpair = cata_cursesport::colorpairs[pairNumber];
            // What about isBlink?
            const bool isBold = col.is_bold();
            const int FG = colorpair.FG + ( isBold ? 8 : 0 );
            std::string generic_id = get_ascii_tile_id( sym, FG, -1 );

            // do not rotate fallback tiles!
            if( sym != LINE_XOXO_C && sym != LINE_OXOX_C ) {
                rota = 0;
            }

            if( tileset_ptr->find_tile_type( generic_id ) ) {
                return draw_from_id_string( generic_id, pos, subtile, rota,
                                            ll, apply_night_vision_goggles );
            }
            // Try again without color this time (using default color).
            generic_id = get_ascii_tile_id( sym, -1, -1 );
            if( tileset_ptr->find_tile_type( generic_id ) ) {
                return draw_from_id_string( generic_id, pos, subtile, rota,
                                            ll, apply_night_vision_goggles );
            }
        }
    }

    // if id is not found, try to find a tile for the category+subcategory combination
    if( !tt ) {
        const std::string &category_id = TILE_CATEGORY_IDS[category];
        if( !category_id.empty() && !subcategory.empty() ) {
            tt = tileset_ptr->find_tile_type( "unknown_" + category_id + "_" + subcategory );
        }
    }

    // if at this point we have no tile, try just the category
    if( !tt ) {
        const std::string &category_id = TILE_CATEGORY_IDS[category];
        if( !category_id.empty() ) {
            tt = tileset_ptr->find_tile_type( "unknown_" + category_id );
        }
    }

    // if we still have no tile, we're out of luck, fall back to unknown
    if( !tt ) {
        tt = tileset_ptr->find_tile_type( "unknown" );
    }

    //  this really shouldn't happen, but the tileset creator might have forgotten to define an unknown tile
    if( !tt ) {
        return false;
    }

    const tile_type &display_tile = *tt;
    // check to see if the display_tile is multitile, and if so if it has the key related to subtile
    if( subtile != -1 && display_tile.multitile ) {
        const auto &display_subtiles = display_tile.available_subtiles;
        const auto end = std::end( display_subtiles );
        if( std::find( begin( display_subtiles ), end, multitile_keys[subtile] ) != end ) {
            // append subtile name to tile and re-find display_tile
            return draw_from_id_string(
                       std::move( id.append( "_", 1 ).append( multitile_keys[subtile] ) ),
                       category, subcategory, pos, -1, rota, ll, apply_night_vision_goggles, height_3d );
        }
    }

    // translate from player-relative to screen relative tile position
    const point screen_pos = player_to_screen( pos.x, pos.y );

    // seed the PRNG to get a reproducible random int
    // TODO: faster solution here
    unsigned int seed = 0;
    // TODO: determine ways other than category to differentiate more types of sprites
    switch( category ) {
        case C_TERRAIN:
        case C_FIELD:
        case C_LIGHTING:
            // stationary map tiles, seed based on map coordinates
            seed = g->m.getabs( pos ).x + g->m.getabs( pos ).y * 65536;
            break;
        case C_VEHICLE_PART:
            // vehicle parts, seed based on coordinates within the vehicle
            // TODO: also use some vehicle id, for less predictability
        {
            // new scope for variable declarations
            const optional_vpart_position vp = g->m.veh_at( pos );
            if( vp ) {
                seed = vp->mount().x + vp->mount().y * 65536;
            }

            //convert vehicle 360-degree direction (0=E,45=SE, etc) to 4-way tile rotation (0=N,1=W,etc)
            tileray face = tileray( rota );
            rota = 3 - face.dir4();

        }
        break;
        case C_ITEM:
        case C_FURNITURE:
        case C_TRAP:
        case C_NONE:
        case C_BULLET:
        case C_HIT_ENTITY:
        case C_WEATHER:
            // TODO: come up with ways to make random sprites consistent for these types
            break;
        case C_MONSTER:
            // FIXME: add persistent id to Creature type, instead of using monster pointer address
            seed = reinterpret_cast<uintptr_t>( g->critter_at<monster>( pos ) );
            break;
        default:
            // player
            if( id.substr( 7 ) == "player_" ) {
                seed = g->u.name[0];
                break;
            }
            // NPC
            if( id.substr( 4 ) == "npc_" ) {
                if( npc *const guy = g->critter_at<npc>( pos ) ) {
                    seed = guy->getID();
                    break;
                }
            }
    }

    // make sure we aren't going to rotate the tile if it shouldn't be rotated
    if( !display_tile.rotates && !( category == C_NONE ) && !( category == C_MONSTER ) ) {
        rota = 0;
    }

    unsigned int loc_rand = 0;
    // only bother mixing up a hash/random value if the tile has some sprites to randomly pick between
    if( display_tile.fg.size() > 1 || display_tile.bg.size() > 1 ) {
        static const auto rot32 = []( const unsigned int x, const int k ) {
            return ( x << k ) | ( x >> ( 32 - k ) );
        };
        // use a fair mix function to turn the "random" seed into a random int
        // taken from public domain code at http://burtleburtle.net/bob/c/lookup3.c 2015/12/11
        unsigned int a = seed, b = -seed, c = seed * seed;
        c ^= b;
        c -= rot32( b, 14 );
        a ^= c;
        a -= rot32( c, 11 );
        b ^= a;
        b -= rot32( a, 25 );
        c ^= b;
        c -= rot32( b, 16 );
        a ^= c;
        a -= rot32( c, 4 );
        b ^= a;
        b -= rot32( a, 14 );
        c ^= b;
        c -= rot32( b, 24 );
        loc_rand = c;
    }

    //draw it!
    draw_tile_at( display_tile, screen_pos.x, screen_pos.y, loc_rand, rota, ll,
                  apply_night_vision_goggles, height_3d );

    return true;
}

bool cata_tiles::draw_sprite_at( const tile_type &tile,
                                 const weighted_int_list<std::vector<int>> &svlist,
                                 int x, int y, unsigned int loc_rand, bool rota_fg, int rota, lit_level ll,
                                 bool apply_night_vision_goggles )
{
    int nullint = 0;
    return cata_tiles::draw_sprite_at( tile, svlist, x, y, loc_rand, rota_fg, rota, ll,
                                       apply_night_vision_goggles, nullint );
}

bool cata_tiles::draw_sprite_at( const tile_type &tile,
                                 const weighted_int_list<std::vector<int>> &svlist,
                                 int x, int y, unsigned int loc_rand, bool rota_fg, int rota, lit_level ll,
                                 bool apply_night_vision_goggles, int &height_3d )
{
    auto picked = svlist.pick( loc_rand );
    if( !picked ) {
        return true;
    }
    auto &spritelist = *picked;
    if( spritelist.empty() ) {
        return true;
    }

    int ret = 0;
    // blit foreground based on rotation
    bool rotate_sprite = false;
    int sprite_num = 0;
    if( !rota_fg && spritelist.size() == 1 ) {
        // don't rotate, a background tile without manual rotations
        rotate_sprite = false;
        sprite_num = 0;
    } else if( spritelist.size() == 1 ) {
        // just one tile, apply SDL sprite rotation if not in isometric mode
        rotate_sprite = true;
        sprite_num = 0;
    } else {
        // multiple rotated tiles defined, don't apply sprite rotation after picking one
        rotate_sprite = false;
        // two tiles, tile 0 is N/S, tile 1 is E/W
        // four tiles, 0=N, 1=E, 2=S, 3=W
        // extending this to more than 4 rotated tiles will require changing rota to degrees
        sprite_num = rota % spritelist.size();
    }

    const texture *sprite_tex = tileset_ptr->get_tile( spritelist[sprite_num] );

    //use night vision colors when in use
    //then use low light tile if available
    if( ll == LL_MEMORIZED ) {
        if( const auto ptr = tileset_ptr->get_memory_tile( spritelist[sprite_num] ) ) {
            sprite_tex = ptr;
        }
    } else if( apply_night_vision_goggles ) {
        if( ll != LL_LOW ) {
            if( const auto ptr = tileset_ptr->get_overexposed_tile( spritelist[sprite_num] ) ) {
                sprite_tex = ptr;
            }
        } else {
            if( const auto ptr = tileset_ptr->get_night_tile( spritelist[sprite_num] ) ) {
                sprite_tex = ptr;
            }
        }
    } else if( ll == LL_LOW ) {
        if( const auto ptr = tileset_ptr->get_shadow_tile( spritelist[sprite_num] ) ) {
            sprite_tex = ptr;
        }
    }

    int width = 0;
    int height = 0;
    std::tie( width, height ) = sprite_tex->dimension();

    SDL_Rect destination;
    destination.x = x + tile.offset.x * tile_width / tileset_ptr->get_tile_width();
    destination.y = y + ( tile.offset.y - height_3d ) * tile_width / tileset_ptr->get_tile_width();
    destination.w = width * tile_width / tileset_ptr->get_tile_width();
    destination.h = height * tile_height / tileset_ptr->get_tile_height();

    if( rotate_sprite ) {
        switch( rota ) {
            default:
            case 0: // unrotated (and 180, with just two sprites)
                ret = sprite_tex->render_copy_ex( renderer, &destination, 0, nullptr, SDL_FLIP_NONE );
                break;
            case 1: // 90 degrees (and 270, with just two sprites)
#if defined(_WIN32)
                destination.y -= 1;
#endif
                if( ! tile_iso ) { // never rotate isometric tiles
                    ret = sprite_tex->render_copy_ex( renderer, &destination, -90, nullptr, SDL_FLIP_NONE );
                } else {
                    ret = sprite_tex->render_copy_ex( renderer, &destination, 0, nullptr, SDL_FLIP_NONE );
                }
                break;
            case 2: // 180 degrees, implemented with flips instead of rotation
                if( ! tile_iso ) { // never flip isometric tiles vertically
                    ret = sprite_tex->render_copy_ex( renderer, &destination, 0, nullptr,
                                                      static_cast<SDL_RendererFlip>( SDL_FLIP_HORIZONTAL | SDL_FLIP_VERTICAL ) );
                } else {
                    ret = sprite_tex->render_copy_ex( renderer, &destination, 0, nullptr, SDL_FLIP_NONE );
                }
                break;
            case 3: // 270 degrees
#if defined(_WIN32)
                destination.x -= 1;
#endif
                if( ! tile_iso ) { // never rotate isometric tiles
                    ret = sprite_tex->render_copy_ex( renderer, &destination, 90, nullptr, SDL_FLIP_NONE );
                } else {
                    ret = sprite_tex->render_copy_ex( renderer, &destination, 0, nullptr, SDL_FLIP_NONE );
                }
                break;
            case 4: // flip horizontally
                ret = sprite_tex->render_copy_ex( renderer, &destination, 0, nullptr,
                                                  static_cast<SDL_RendererFlip>( SDL_FLIP_HORIZONTAL ) );
        }
    } else { // don't rotate, same as case 0 above
        ret = sprite_tex->render_copy_ex( renderer, &destination, 0, nullptr, SDL_FLIP_NONE );
    }

    printErrorIf( ret != 0, "SDL_RenderCopyEx() failed" );
    // this reference passes all the way back up the call chain back to
    // cata_tiles::draw() std::vector<tile_render_info> draw_points[].height_3d
    // where we are accumulating the height of every sprite stacked up in a tile
    height_3d += tile.height_3d;
    return true;
}

bool cata_tiles::draw_tile_at( const tile_type &tile, int x, int y, unsigned int loc_rand,
                               int rota,
                               lit_level ll, bool apply_night_vision_goggles, int &height_3d )
{
    draw_sprite_at( tile, tile.bg, x, y, loc_rand, /*fg:*/ false, rota, ll,
                    apply_night_vision_goggles );
    draw_sprite_at( tile, tile.fg, x, y, loc_rand, /*fg:*/ true, rota, ll, apply_night_vision_goggles,
                    height_3d );
    return true;
}

bool cata_tiles::apply_vision_effects( const tripoint &pos,
                                       const visibility_type visibility )
{
    std::string light_name;
    switch( visibility ) {
        case VIS_HIDDEN:
            light_name = "lighting_hidden";
            break;
        case VIS_LIT:
            light_name = "lighting_lowlight_light";
            break;
        case VIS_BOOMER:
            light_name = "lighting_boomered_light";
            break;
        case VIS_BOOMER_DARK:
            light_name = "lighting_boomered_dark";
            break;
        case VIS_DARK:
            light_name = "lighting_lowlight_dark";
            break;
        case VIS_CLEAR: // Handled by the caller.
            return false;
    }

    // lighting is never rotated, though, could possibly add in random rotation?
    draw_from_id_string( light_name, C_LIGHTING, empty_string, pos, 0, 0, LL_LIT, false );

    return true;
}

bool cata_tiles::draw_terrain_below( const tripoint &p, lit_level /*ll*/, int &/*height_3d*/ )
{
    if( !g->m.need_draw_lower_floor( p ) ) {
        return false;
    }

    tripoint pbelow = tripoint( p.xy(), p.z - 1 );
    SDL_Color tercol = curses_color_to_SDL( c_dark_gray );

    const ter_t &curr_ter = g->m.ter( pbelow ).obj();
    const furn_t &curr_furn = g->m.furn( pbelow ).obj();
    int part_below;
    int sizefactor = 2;
    const vehicle *veh;
    //        const vehicle *veh;
    if( curr_furn.has_flag( TFLAG_SEEN_FROM_ABOVE ) ) {
        tercol = curses_color_to_SDL( curr_furn.color() );
    } else if( curr_furn.movecost < 0 ) {
        tercol = curses_color_to_SDL( curr_furn.color() );
    } else if( ( veh = g->m.veh_at_internal( pbelow, part_below ) ) != nullptr ) {
        const int roof = veh->roof_at_part( part_below );
        const auto vpobst = vpart_position( const_cast<vehicle &>( *veh ), part_below ).obstacle_at_part();
        tercol = curses_color_to_SDL( ( roof >= 0 ||
                                        vpobst ) ? c_light_gray : c_magenta );
        sizefactor = ( roof >= 0 || vpobst ) ? 4 : 2;
    } else if( curr_ter.has_flag( TFLAG_SEEN_FROM_ABOVE ) || curr_ter.movecost == 0 ) {
        tercol = curses_color_to_SDL( curr_ter.color() );
    } else if( !curr_ter.has_flag( TFLAG_NO_FLOOR ) ) {
        sizefactor = 4;
        tercol = curses_color_to_SDL( curr_ter.color() );
    } else {
        tercol = curses_color_to_SDL( curr_ter.color() );
    }

    SDL_Rect belowRect;
    belowRect.h = tile_width / sizefactor;
    belowRect.w = tile_height / sizefactor;
    if( tile_iso ) {
        belowRect.h = ( belowRect.h * 2 ) / 3;
        belowRect.w = ( belowRect.w * 3 ) / 4;
    }
    // translate from player-relative to screen relative tile position
    int screen_x = 0;
    int screen_y = 0;
    if( tile_iso ) {
        screen_x = ( ( pbelow.x - o.x ) - ( o.y - pbelow.y ) + screentile_width - 2 ) * tile_width / 2 +
                   op.x;
        // y uses tile_width because width is definitive for iso tiles
        // tile footprints are half as tall as wide, arbitrarily tall
        screen_y = ( ( pbelow.y - o.y ) - ( pbelow.x - o.x ) - 4 ) * tile_width / 4 +
                   screentile_height * tile_height / 2 + // TODO: more obvious centering math
                   op.y;
    } else {
        screen_x = ( pbelow.x - o.x ) * tile_width + op.x;
        screen_y = ( pbelow.y - o.y ) * tile_height + op.y;
    }
    belowRect.x = screen_x + ( tile_width - belowRect.w ) / 2;
    belowRect.y = screen_y + ( tile_height - belowRect.h ) / 2;
    if( tile_iso ) {
        belowRect.y += tile_height / 8;
    }
    render_fill_rect( renderer, belowRect, tercol.r, tercol.g, tercol.b );

    return true;
}

bool cata_tiles::draw_terrain( const tripoint &p, lit_level ll, int &height_3d )
{
    const ter_id t = g->m.ter( p ); // get the ter_id value at this point
    // check for null, if null return false
    if( t == t_null ) {
        return false;
    }

    //char alteration = 0;
    int subtile = 0;
    int rotation = 0;

    int connect_group;
    if( g->m.ter( p ).obj().connects( connect_group ) ) {
        get_connect_values( p, subtile, rotation, connect_group );
    } else {
        get_terrain_orientation( p, rotation, subtile );
        // do something to get other terrain orientation values
    }

    const std::string &tname = t.obj().id.str();
    if( !g->m.check_and_set_seen_cache( p ) ) {
        g->u.memorize_tile( g->m.getabs( p ), tname, subtile, rotation );
    }

    return draw_from_id_string( tname, C_TERRAIN, empty_string, p, subtile, rotation, ll,
                                nv_goggles_activated, height_3d );
}

bool cata_tiles::draw_terrain_from_memory( const tripoint &p, int &height_3d )
{
    if( !g->u.should_show_map_memory() ) {
        return false;
    }
    const memorized_terrain_tile t = g->u.get_memorized_tile( g->m.getabs( p ) );
    if( t.tile.empty() ) {
        return false;
    }

    TILE_CATEGORY category = C_NONE;

    switch( t.tile[0] ) {
        case 't':
            category = C_TERRAIN;
            break;
        case 'f':
            category = C_FURNITURE;
            break;
        case 'v':
            category = C_VEHICLE_PART;
            break;
        default:
            debugmsg( "Could not infer category for memorized tile %s", t.tile );
            break;
    }

    return draw_from_id_string( t.tile, category, empty_string, p, t.subtile, t.rotation,
                                LL_MEMORIZED, nv_goggles_activated, height_3d );
}

bool cata_tiles::draw_furniture( const tripoint &p, lit_level ll, int &height_3d )
{
    // get furniture ID at x,y
    bool has_furn = g->m.has_furn( p );
    if( !has_furn ) {
        return false;
    }

    const furn_id f_id = g->m.furn( p );

    // for rotation information
    const int neighborhood[4] = {
        static_cast<int>( g->m.furn( p + point_south ) ),
        static_cast<int>( g->m.furn( p + point_east ) ),
        static_cast<int>( g->m.furn( p + point_west ) ),
        static_cast<int>( g->m.furn( p + point_north ) )
    };

    int subtile = 0;
    int rotation = 0;
    get_tile_values( f_id, neighborhood, subtile, rotation );

    // get the name of this furniture piece
    const std::string &f_name = f_id.obj().id.str();
    if( !g->m.check_and_set_seen_cache( p ) ) {
        g->u.memorize_tile( g->m.getabs( p ), f_name, subtile, rotation );
    }

    bool ret = draw_from_id_string( f_name, C_FURNITURE, empty_string, p, subtile, rotation, ll,
                                    nv_goggles_activated, height_3d );
    return ret;
}

bool cata_tiles::draw_trap( const tripoint &p, lit_level ll, int &height_3d )
{
    const trap &tr = g->m.tr_at( p );
    if( !tr.can_see( p, g->u ) ) {
        return false;
    }

    const int neighborhood[4] = {
        static_cast<int>( g->m.tr_at( p + point_south ).loadid ),
        static_cast<int>( g->m.tr_at( p + point_east ).loadid ),
        static_cast<int>( g->m.tr_at( p + point_west ).loadid ),
        static_cast<int>( g->m.tr_at( p + point_north ).loadid )
    };

    int subtile = 0;
    int rotation = 0;
    get_tile_values( tr.loadid, neighborhood, subtile, rotation );

    if( !g->m.check_and_set_seen_cache( p ) ) {
        g->u.memorize_tile( g->m.getabs( p ), tr.id.str(), subtile, rotation );
    }

    return draw_from_id_string( tr.id.str(), C_TRAP, empty_string, p, subtile, rotation, ll,
                                nv_goggles_activated, height_3d );
}

bool cata_tiles::draw_graffiti( const tripoint &p, lit_level ll, int &height_3d )
{
    if( !g->m.has_graffiti_at( p ) ) {
        return false;
    }

    return draw_from_id_string( "graffiti", C_NONE, empty_string, p, 0, 0, ll, false, height_3d );
}

bool cata_tiles::draw_field_or_item( const tripoint &p, lit_level ll, int &height_3d )
{
    const field &f = g->m.field_at( p );
    const field_type &fld_type = g->m.field_at( p ).displayed_field_type().obj();
    bool ret_draw_field = true;
    bool ret_draw_items = true;
    if( fld_type.display_field ) {
        // for rotation information
        const int neighborhood[4] = {
            static_cast<int>( g->m.field_at( p + point_south ).displayed_field_type() ),
            static_cast<int>( g->m.field_at( p + point_east ).displayed_field_type() ),
            static_cast<int>( g->m.field_at( p + point_west ).displayed_field_type() ),
            static_cast<int>( g->m.field_at( p + point_north ).displayed_field_type() )
        };

        int subtile = 0;
        int rotation = 0;
        get_tile_values( f.displayed_field_type(), neighborhood, subtile, rotation );

        ret_draw_field = draw_from_id_string( fld_type.id.c_str(), C_FIELD, empty_string, p, subtile,
                                              rotation, ll, nv_goggles_activated );
    }
    if( fld_type.display_items ) {
        if( !g->m.sees_some_items( p, g->u ) ) {
            return false;
        }
        const maptile &cur_maptile = g->m.maptile_at( p );
        // get the last item in the stack, it will be used for display
        const item &displayed_item = cur_maptile.get_uppermost_item();
        // get the item's name, as that is the key used to find it in the map
        const std::string it_name = displayed_item.is_corpse() ? "corpse_" +
                                    displayed_item.get_mtype()->id.str() : displayed_item.typeId();

        const std::string it_category = displayed_item.type->get_item_type_string();
        ret_draw_items = draw_from_id_string( it_name, C_ITEM, it_category, p, 0, 0, ll,
                                              nv_goggles_activated, height_3d );
        if( ret_draw_items && cur_maptile.get_item_count() > 1 ) {
            draw_item_highlight( p );
        }
    }
    return ret_draw_field && ret_draw_items;
}

bool cata_tiles::draw_vpart_below( const tripoint &p, lit_level /*ll*/, int &/*height_3d*/ )
{
    if( !g->m.need_draw_lower_floor( p ) ) {
        return false;
    }
    tripoint pbelow( p.xy(), p.z - 1 );
    int height_3d_below = 0;
    return draw_vpart( pbelow, LL_LOW, height_3d_below );
}

bool cata_tiles::draw_vpart( const tripoint &p, lit_level ll, int &height_3d )
{
    const optional_vpart_position vp = g->m.veh_at( p );

    if( !vp ) {
        return false;
    }
    vehicle *const veh = &vp->vehicle();
    // veh_part is the index of the part
    const int veh_part = vp->part_index();

    // Gets the visible part, should work fine once tileset vp_ids are updated to work with the vehicle part json ids
    // get the vpart_id
    char part_mod = 0;
    const vpart_id &vp_id = veh->part_id_string( veh_part, part_mod );

    // prefix with vp_ ident
    const std::string vpid = "vp_" + vp_id.str();
    int subtile = 0;
    if( part_mod > 0 ) {
        switch( part_mod ) {
            case 1:
                subtile = open_;
                break;
            case 2:
                subtile = broken;
                break;
        }
    }
    const cata::optional<vpart_reference> cargopart = vp.part_with_feature( "CARGO", true );
    bool draw_highlight = cargopart && !veh->get_items( cargopart->part_index() ).empty();

    int veh_dir = veh->face.dir();
    if( !veh->forward_velocity() && !veh->player_in_control( g->u ) ) {
        if( !g->m.check_and_set_seen_cache( p ) ) {
            g->u.memorize_tile( g->m.getabs( p ), vpid, subtile, veh_dir );
        }
    }

    bool ret = draw_from_id_string( vpid, C_VEHICLE_PART, empty_string, p, subtile, veh_dir,
                                    ll, nv_goggles_activated, height_3d );
    if( ret && draw_highlight ) {
        draw_item_highlight( p );
    }
    return ret;
}

bool cata_tiles::draw_critter_at_below( const tripoint &p, lit_level, int & )
{
    // Check if we even need to draw below. If not, bail.
    if( !g->m.need_draw_lower_floor( p ) ) {
        return false;
    }

    tripoint pbelow( p.xy(), p.z - 1 );

    // Get the critter at the location below. If there isn't one,
    // we can bail.
    const Creature *critter = g->critter_at( pbelow, true );
    if( critter == nullptr ) {
        return false;
    }

    // Check if the player can actually see the critter. We don't care if
    // it's via infrared or not, just whether or not they're seen. If not,
    // we can bail.
    if( !g->u.sees( *critter ) && !g->u.sees_with_infrared( *critter ) ) {
        return false;
    }

    const point screen_point = player_to_screen( pbelow.x, pbelow.y );

    SDL_Color tercol = curses_color_to_SDL( c_red );
    const int sizefactor = 2;

    SDL_Rect belowRect;
    belowRect.h = tile_width / sizefactor;
    belowRect.w = tile_height / sizefactor;

    if( tile_iso ) {
        belowRect.h = ( belowRect.h * 2 ) / 3;
        belowRect.w = ( belowRect.w * 3 ) / 4;
    }

    belowRect.x = screen_point.x + ( tile_width - belowRect.w ) / 2;
    belowRect.y = screen_point.y + ( tile_height - belowRect.h ) / 2;

    if( tile_iso ) {
        belowRect.y += tile_height / 8;
    }

    render_fill_rect( renderer, belowRect, tercol.r, tercol.g, tercol.b );

    return true;
}

bool cata_tiles::draw_critter_at( const tripoint &p, lit_level ll, int &height_3d )
{
    const auto critter = g->critter_at( p, true );
    if( critter != nullptr ) {
        return draw_entity( *critter, p, ll, height_3d );
    }
    return false;
}

bool cata_tiles::draw_zone_mark( const tripoint &p, lit_level ll, int &height_3d )
{
    if( !g->is_zones_manager_open() ) {
        return false;
    }

    const zone_manager &mgr = zone_manager::get_manager();
    const tripoint &abs = g->m.getabs( p );
    const auto zone = mgr.get_bottom_zone( abs );

    if( zone && zone->has_options() ) {
        auto option = dynamic_cast<const mark_option *>( &zone->get_options() );

        if( option && !option->get_mark().empty() ) {
            return draw_from_id_string( option->get_mark(), C_NONE, empty_string, p, 0, 0, ll,
                                        nv_goggles_activated, height_3d );
        }
    }

    return false;
}

bool cata_tiles::draw_entity( const Creature &critter, const tripoint &p, lit_level ll,
                              int &height_3d )
{
    if( !g->u.sees( critter ) ) {
        if( g->u.sees_with_infrared( critter ) ) {
            return draw_from_id_string( "infrared_creature", C_NONE, empty_string, p, 0, 0,
                                        LL_LIT, false, height_3d );
        }
        return false;
    }
    bool result = false;
    bool sees_player = false;
    bool is_player = false;
    Creature::Attitude attitude = Creature::A_ANY;
    const monster *m = dynamic_cast<const monster *>( &critter );
    if( m != nullptr ) {
        const auto ent_category = C_MONSTER;
        std::string ent_subcategory = empty_string;
        if( !m->type->species.empty() ) {
            ent_subcategory = m->type->species.begin()->str();
        }
        const int subtile = corner;
        // depending on the toggle flip sprite left or right
        int rot_facing = -1;
        if( m->facing == FD_RIGHT ) {
            rot_facing = 0;
        } else if( m->facing == FD_LEFT ) {
            rot_facing = 4;
        }
        if( rot_facing >= 0 ) {
            const auto ent_name = m->type->id;
            std::string chosen_id = ent_name.str();
            if( m->has_effect( effect_ridden ) ) {
                int pl_under_height = 6;
                draw_entity_with_overlays( g->u, p, ll, pl_under_height );
                const std::string prefix = "rid_";
                std::string copy_id = chosen_id;
                const std::string ridden_id = copy_id.insert( 0, prefix );
                const tile_type *tt = tileset_ptr->find_tile_type( ridden_id );
                if( tt ) {
                    chosen_id = ridden_id;
                }
            }
            result = draw_from_id_string( chosen_id, ent_category, ent_subcategory, p, subtile, rot_facing,
                                          ll, false, height_3d );
            sees_player = m->sees( g->u );
            attitude = m->attitude_to( g-> u );
        }
    }
    const player *pl = dynamic_cast<const player *>( &critter );
    if( pl != nullptr ) {
        draw_entity_with_overlays( *pl, p, ll, height_3d );
        result = true;
        if( pl->is_player() ) {
            is_player = true;
        } else {
            sees_player = pl->sees( g-> u );
            attitude = pl->attitude_to( g-> u );
        }
    }

    if( result && !is_player ) {
        std::ostringstream tmp_id;
        tmp_id << "overlay_" << Creature::attitude_raw_string( attitude );
        if( sees_player ) {
            tmp_id << "_sees_player";
        }
        const std::string draw_id = tmp_id.str();
        if( tileset_ptr->find_tile_type( draw_id ) ) {
            draw_from_id_string( draw_id, C_NONE, empty_string, p, 0, 0, LL_LIT, false, height_3d );
        }
    }
    return result;
}

void cata_tiles::draw_entity_with_overlays( const player &pl, const tripoint &p, lit_level ll,
        int &height_3d )
{
    std::string ent_name;

    if( pl.is_npc() ) {
        ent_name = pl.male ? "npc_male" : "npc_female";
    } else {
        ent_name = pl.male ? "player_male" : "player_female";
    }
    // first draw the character itself(i guess this means a tileset that
    // takes this seriously needs a naked sprite)
    int prev_height_3d = height_3d;

    // depending on the toggle flip sprite left or right
    if( pl.facing == FD_RIGHT ) {
        draw_from_id_string( ent_name, C_NONE, "", p, corner, 0, ll, false, height_3d );
    } else if( pl.facing == FD_LEFT ) {
        draw_from_id_string( ent_name, C_NONE, "", p, corner, 4, ll, false, height_3d );
    }

    // next up, draw all the overlays
    std::vector<std::string> overlays = pl.get_overlay_ids();
    for( const std::string &overlay : overlays ) {
        std::string draw_id = overlay;
        if( find_overlay_looks_like( pl.male, overlay, draw_id ) ) {
            int overlay_height_3d = prev_height_3d;
            if( pl.facing == FD_RIGHT ) {
                draw_from_id_string( draw_id, C_NONE, "", p, corner, /*rota:*/ 0, ll, false, overlay_height_3d );
            } else if( pl.facing == FD_LEFT ) {
                draw_from_id_string( draw_id, C_NONE, "", p, corner, /*rota:*/ 4, ll, false, overlay_height_3d );
            }
            // the tallest height-having overlay is the one that counts
            height_3d = std::max( height_3d, overlay_height_3d );
        }
    }
}

bool cata_tiles::draw_item_highlight( const tripoint &pos )
{
    return draw_from_id_string( ITEM_HIGHLIGHT, C_NONE, empty_string, pos, 0, 0, LL_LIT, false );
}

void tileset_loader::ensure_default_item_highlight()
{
    if( ts.find_tile_type( ITEM_HIGHLIGHT ) ) {
        return;
    }
    const Uint8 highlight_alpha = 127;

    int index = ts.tile_values.size();

    const SDL_Surface_Ptr surface = create_surface_32( ts.tile_width, ts.tile_height );
    assert( surface );
    throwErrorIf( SDL_FillRect( surface.get(), nullptr, SDL_MapRGBA( surface->format, 0, 0, 127,
                                highlight_alpha ) ) != 0, "SDL_FillRect failed" );
    ts.tile_values.emplace_back( CreateTextureFromSurface( renderer, surface ), SDL_Rect{ 0, 0, ts.tile_width, ts.tile_height } );
    ts.tile_ids[ITEM_HIGHLIGHT].fg.add( std::vector<int>( {index} ), 1 );
}

/* Animation Functions */
/* -- Inits */
void cata_tiles::init_explosion( const tripoint &p, int radius )
{
    do_draw_explosion = true;
    exp_pos = p;
    exp_rad = radius;
}
void cata_tiles::init_custom_explosion_layer( const std::map<tripoint, explosion_tile> &layer )
{
    do_draw_custom_explosion = true;
    custom_explosion_layer = layer;
}
void cata_tiles::init_draw_bullet( const tripoint &p, std::string name )
{
    do_draw_bullet = true;
    bul_pos = p;
    bul_id = std::move( name );
}
void cata_tiles::init_draw_hit( const tripoint &p, std::string name )
{
    do_draw_hit = true;
    hit_pos = p;
    hit_entity_id = std::move( name );
}
void cata_tiles::init_draw_line( const tripoint &p, std::vector<tripoint> trajectory,
                                 std::string name, bool target_line )
{
    do_draw_line = true;
    is_target_line = target_line;
    line_pos = p;
    line_endpoint_id = std::move( name );
    line_trajectory = std::move( trajectory );
}
void cata_tiles::init_draw_cursor( const tripoint &p )
{
    do_draw_cursor = true;
    cursors.emplace_back( p );
}
void cata_tiles::init_draw_highlight( const tripoint &p )
{
    do_draw_highlight = true;
    highlights.emplace_back( p );
}
void cata_tiles::init_draw_weather( weather_printable weather, std::string name )
{
    do_draw_weather = true;
    weather_name = std::move( name );
    anim_weather = std::move( weather );
}
void cata_tiles::init_draw_sct()
{
    do_draw_sct = true;
}
void cata_tiles::init_draw_zones( const tripoint &_start, const tripoint &_end,
                                  const tripoint &_offset )
{
    do_draw_zones = true;
    zone_start = _start;
    zone_end = _end;
    zone_offset = _offset;
}
/* -- Void Animators */
void cata_tiles::void_explosion()
{
    do_draw_explosion = false;
    exp_pos = {-1, -1, -1};
    exp_rad = -1;
}
void cata_tiles::void_custom_explosion()
{
    do_draw_custom_explosion = false;
    custom_explosion_layer.clear();
}
void cata_tiles::void_bullet()
{
    do_draw_bullet = false;
    bul_pos = { -1, -1, -1 };
    bul_id.clear();
}
void cata_tiles::void_hit()
{
    do_draw_hit = false;
    hit_pos = { -1, -1, -1 };
    hit_entity_id.clear();
}
void cata_tiles::void_line()
{
    do_draw_line = false;
    is_target_line = false;
    line_pos = { -1, -1, -1 };
    line_endpoint_id.clear();
    line_trajectory.clear();
}
void cata_tiles::void_cursor()
{
    do_draw_cursor = false;
    cursors.clear();
}
void cata_tiles::void_highlight()
{
    do_draw_highlight = false;
    highlights.clear();
}
void cata_tiles::void_weather()
{
    do_draw_weather = false;
    weather_name.clear();
    anim_weather.vdrops.clear();
}
void cata_tiles::void_sct()
{
    do_draw_sct = false;
}
void cata_tiles::void_zones()
{
    do_draw_zones = false;
}
/* -- Animation Renders */
void cata_tiles::draw_explosion_frame()
{
    std::string exp_name = "explosion";
    int subtile = 0;
    int rotation = 0;

    for( int i = 1; i < exp_rad; ++i ) {
        subtile = corner;
        rotation = 0;

        draw_from_id_string( exp_name, exp_pos + point( -i, -i ),
                             subtile, rotation++, LL_LIT, nv_goggles_activated );
        draw_from_id_string( exp_name, exp_pos + point( -i, i ),
                             subtile, rotation++, LL_LIT, nv_goggles_activated );
        draw_from_id_string( exp_name, exp_pos + point( i, i ),
                             subtile, rotation++, LL_LIT, nv_goggles_activated );
        draw_from_id_string( exp_name, exp_pos + point( i, -i ),
                             subtile, rotation++, LL_LIT, nv_goggles_activated );

        subtile = edge;
        for( int j = 1 - i; j < 0 + i; j++ ) {
            rotation = 0;
            draw_from_id_string( exp_name, exp_pos + point( j, -i ),
                                 subtile, rotation, LL_LIT, nv_goggles_activated );
            draw_from_id_string( exp_name, exp_pos + point( j, i ),
                                 subtile, rotation, LL_LIT, nv_goggles_activated );

            rotation = 1;
            draw_from_id_string( exp_name, exp_pos + point( -i, j ),
                                 subtile, rotation, LL_LIT, nv_goggles_activated );
            draw_from_id_string( exp_name, exp_pos + point( i, j ),
                                 subtile, rotation, LL_LIT, nv_goggles_activated );
        }
    }
}

void cata_tiles::draw_custom_explosion_frame()
{
    // TODO: Make the drawing code handle all the missing tiles: <^>v and *
    // TODO: Add more explosion tiles, like "strong explosion", so that it displays more info
    static const std::string exp_strong = "explosion";
    static const std::string exp_medium = "explosion_medium";
    static const std::string exp_weak = "explosion_weak";
    int subtile = 0;
    int rotation = 0;

    for( const auto &pr : custom_explosion_layer ) {
        const explosion_neighbors ngh = pr.second.neighborhood;
        const nc_color col = pr.second.color;

        switch( ngh ) {
            case N_NORTH:
            case N_SOUTH:
                subtile = edge;
                rotation = 1;
                break;
            case N_WEST:
            case N_EAST:
                subtile = edge;
                rotation = 0;
                break;
            case N_NORTH | N_SOUTH:
            case N_NORTH | N_SOUTH | N_WEST:
            case N_NORTH | N_SOUTH | N_EAST:
                subtile = edge;
                rotation = 1;
                break;
            case N_WEST | N_EAST:
            case N_WEST | N_EAST | N_NORTH:
            case N_WEST | N_EAST | N_SOUTH:
                subtile = edge;
                rotation = 0;
                break;
            case N_SOUTH | N_EAST:
                subtile = corner;
                rotation = 0;
                break;
            case N_NORTH | N_EAST:
                subtile = corner;
                rotation = 1;
                break;
            case N_NORTH | N_WEST:
                subtile = corner;
                rotation = 2;
                break;
            case N_SOUTH | N_WEST:
                subtile = corner;
                rotation = 3;
                break;
            case N_NO_NEIGHBORS:
                subtile = edge;
                break;
            case N_WEST | N_EAST | N_NORTH | N_SOUTH:
                // Needs some special tile
                subtile = edge;
                break;
        }

        const tripoint &p = pr.first;
        if( col == c_red ) {
            draw_from_id_string( exp_strong, p, subtile, rotation, LL_LIT, nv_goggles_activated );
        } else if( col == c_yellow ) {
            draw_from_id_string( exp_medium, p, subtile, rotation, LL_LIT, nv_goggles_activated );
        } else {
            draw_from_id_string( exp_weak, p, subtile, rotation, LL_LIT, nv_goggles_activated );
        }
    }
}
void cata_tiles::draw_bullet_frame()
{
    draw_from_id_string( bul_id, C_BULLET, empty_string, bul_pos, 0, 0, LL_LIT, false );
}
void cata_tiles::draw_hit_frame()
{
    std::string hit_overlay = "animation_hit";

    draw_from_id_string( hit_entity_id, C_HIT_ENTITY, empty_string, hit_pos, 0, 0, LL_LIT, false );
    draw_from_id_string( hit_overlay, hit_pos, 0, 0, LL_LIT, false );
}
void cata_tiles::draw_line()
{
    if( line_trajectory.empty() ) {
        return;
    }
    static std::string line_overlay = "animation_line";
    if( !is_target_line || g->u.sees( line_pos ) ) {
        for( auto it = line_trajectory.begin(); it != line_trajectory.end() - 1; ++it ) {
            draw_from_id_string( line_overlay, *it, 0, 0, LL_LIT, false );
        }
    }

    draw_from_id_string( line_endpoint_id, line_trajectory.back(), 0, 0, LL_LIT, false );
}
void cata_tiles::draw_cursor()
{
    for( const tripoint &p : cursors ) {
        draw_from_id_string( "cursor", p, 0, 0, LL_LIT, false );
    }
}
void cata_tiles::draw_highlight()
{
    for( const tripoint &p : highlights ) {
        draw_from_id_string( "highlight", p, 0, 0, LL_LIT, false );
    }
}
void cata_tiles::draw_weather_frame()
{

    for( auto &vdrop : anim_weather.vdrops ) {
        // TODO: Z-level awareness if weather ever happens on anything but z-level 0.
        tripoint p( vdrop.first, vdrop.second, 0 );
        if( !tile_iso ) {
            // currently in ASCII screen coordinates
            p += o;
        }
        draw_from_id_string( weather_name, C_WEATHER, empty_string, p, 0, 0,
                             LL_LIT, nv_goggles_activated );
    }
}

void cata_tiles::draw_sct_frame( std::multimap<point, formatted_text> &overlay_strings )
{
    const bool use_font = get_option<bool>( "ANIMATION_SCT_USE_FONT" );

    for( auto iter = SCT.vSCT.begin(); iter != SCT.vSCT.end(); ++iter ) {
        const int iDX = iter->getPosX();
        const int iDY = iter->getPosY();
        const int full_text_length = iter->getText().length();

        int iOffsetX = 0;
        int iOffsetY = 0;

        for( int j = 0; j < 2; ++j ) {
            std::string sText = iter->getText( ( j == 0 ) ? "first" : "second" );
            int FG = msgtype_to_tilecolor( iter->getMsgType( ( j == 0 ) ? "first" : "second" ),
                                           iter->getStep() >= SCT.iMaxSteps / 2 );

            if( use_font ) {
                const auto direction = iter->getDirecton();
                // Compensate for string length offset added at SCT creation
                // (it will be readded using font size and proper encoding later).
                const int direction_offset = ( -direction_XY( direction ).x + 1 ) * full_text_length / 2;

                overlay_strings.emplace(
                    player_to_screen( iDX + direction_offset, iDY ),
                    formatted_text( sText, FG, direction ) );
            } else {
                for( auto &it : sText ) {
                    const std::string generic_id = get_ascii_tile_id( it, FG, -1 );

                    if( tileset_ptr->find_tile_type( generic_id ) ) {
                        draw_from_id_string( generic_id, C_NONE, empty_string,
                        { iDX + iOffsetX, iDY + iOffsetY, g->u.pos().z }, 0, 0, LL_LIT, false );
                    }

                    if( tile_iso ) {
                        iOffsetY++;
                    }
                    iOffsetX++;
                }
            }
        }
    }
}

void cata_tiles::draw_zones_frame()
{
    for( int iY = zone_start.y; iY <= zone_end.y; ++ iY ) {
        for( int iX = zone_start.x; iX <= zone_end.x; ++iX ) {
            draw_from_id_string( "highlight", C_NONE, empty_string,
                                 zone_offset.xy() + tripoint( iX, iY, g->u.pos().z ),
                                 0, 0, LL_LIT, false );
        }
    }

}
void cata_tiles::draw_footsteps_frame()
{
    static const std::string footstep_tilestring = "footstep";
    for( const auto &footstep : sounds::get_footstep_markers() ) {
        draw_from_id_string( footstep_tilestring, footstep, 0, 0, LL_LIT, false );
    }
}
/* END OF ANIMATION FUNCTIONS */

void cata_tiles::init_light()
{
    g->reset_light_level();
}

void cata_tiles::get_terrain_orientation( const tripoint &p, int &rota, int &subtile )
{
    // get terrain at x,y
    ter_id tid = g->m.ter( p );
    if( tid == t_null ) {
        subtile = 0;
        rota = 0;
        return;
    }

    // get terrain neighborhood
    const ter_id neighborhood[4] = {
        g->m.ter( p + point_south ),
        g->m.ter( p + point_east ),
        g->m.ter( p + point_west ),
        g->m.ter( p + point_north )
    };

    bool connects[4];
    char val = 0;

    // populate connection information
    for( int i = 0; i < 4; ++i ) {
        connects[i] = ( neighborhood[i] == tid );

        if( connects[i] ) {
            val += 1 << i;
        }
    }

    get_rotation_and_subtile( val, rota, subtile );
}

void cata_tiles::get_rotation_and_subtile( const char val, int &rotation, int &subtile )
{
    switch( val ) {
        // no connections
        case 0:
            subtile = unconnected;
            rotation = 0;
            break;
        // all connections
        case 15:
            subtile = center;
            rotation = 0;
            break;
        // end pieces
        case 8:
            subtile = end_piece;
            rotation = 2;
            break;
        case 4:
            subtile = end_piece;
            rotation = 3;
            break;
        case 2:
            subtile = end_piece;
            rotation = 1;
            break;
        case 1:
            subtile = end_piece;
            rotation = 0;
            break;
        // edges
        case 9:
            subtile = edge;
            rotation = 0;
            break;
        case 6:
            subtile = edge;
            rotation = 1;
            break;
        // corners
        case 12:
            subtile = corner;
            rotation = 2;
            break;
        case 10:
            subtile = corner;
            rotation = 1;
            break;
        case 3:
            subtile = corner;
            rotation = 0;
            break;
        case 5:
            subtile = corner;
            rotation = 3;
            break;
        // all t_connections
        case 14:
            subtile = t_connection;
            rotation = 2;
            break;
        case 11:
            subtile = t_connection;
            rotation = 1;
            break;
        case 7:
            subtile = t_connection;
            rotation = 0;
            break;
        case 13:
            subtile = t_connection;
            rotation = 3;
            break;
    }
}

void cata_tiles::get_connect_values( const tripoint &p, int &subtile, int &rotation,
                                     int connect_group )
{
    uint8_t connections = g->m.get_known_connections( p, connect_group );
    get_rotation_and_subtile( connections, rotation, subtile );
}

void cata_tiles::get_tile_values( const int t, const int *tn, int &subtile, int &rotation )
{
    bool connects[4];
    char val = 0;
    for( int i = 0; i < 4; ++i ) {
        connects[i] = ( tn[i] == t );
        if( connects[i] ) {
            val += 1 << i;
        }
    }
    get_rotation_and_subtile( val, rotation, subtile );
}

void cata_tiles::do_tile_loading_report()
{
    DebugLog( D_INFO, DC_ALL ) << "Loaded tileset: " << get_option<std::string>( "TILES" );

    if( !g->is_core_data_loaded() ) {
        return; // There's nothing to do anymore without the core data.
    }

    tile_loading_report<ter_t>( ter_t::count(), "Terrain", "" );
    tile_loading_report<furn_t>( furn_t::count(), "Furniture", "" );

    std::map<itype_id, const itype *> items;
    for( const itype *e : item_controller->all() ) {
        items.emplace( e->get_id(), e );
    }
    tile_loading_report( items, "Items", "" );

    auto mtypes = MonsterGenerator::generator().get_all_mtypes();
    lr_generic( mtypes.begin(), mtypes.end(), []( const std::vector<mtype>::iterator & m ) {
        return ( *m ).id.str();
    }, "Monsters", "" );
    tile_loading_report( vpart_info::all(), "Vehicle Parts", "vp_" );
    tile_loading_report<trap>( trap::count(), "Traps", "" );
    tile_loading_report<field_type>( field_type::count(), "Field Types", "" );

    // needed until DebugLog ostream::flush bugfix lands
    DebugLog( D_INFO, DC_ALL );
}

point cata_tiles::player_to_screen( const int x, const int y ) const
{
    int screen_x = 0;
    int screen_y = 0;
    if( tile_iso ) {
        screen_x = ( ( x - o.x ) - ( o.y - y ) + screentile_width - 2 ) * tile_width / 2 +
                   op.x;
        // y uses tile_width because width is definitive for iso tiles
        // tile footprints are half as tall as wide, arbitrarily tall
        screen_y = ( ( y - o.y ) - ( x - o.x ) - 4 ) * tile_width / 4 +
                   screentile_height * tile_height / 2 + // TODO: more obvious centering math
                   op.y;
    } else {
        screen_x = ( x - o.x ) * tile_width + op.x;
        screen_y = ( y - o.y ) * tile_height + op.y;
    }
    return {screen_x, screen_y};
}

template<typename Iter, typename Func>
void cata_tiles::lr_generic( Iter begin, Iter end, Func id_func, const std::string &label,
                             const std::string &prefix )
{
    int missing = 0;
    int present = 0;
    std::string missing_list;
    for( ; begin != end; ++begin ) {
        const std::string id_string = id_func( begin );
        if( !tileset_ptr->find_tile_type( prefix + id_string ) ) {
            missing++;
            missing_list.append( id_string + " " );
        } else {
            present++;
        }
    }
    DebugLog( D_INFO, DC_ALL ) << "Missing " << label << ": " << missing_list;
}

template <typename maptype>
void cata_tiles::tile_loading_report( const maptype &tiletypemap, const std::string &label,
                                      const std::string &prefix )
{
    lr_generic( tiletypemap.begin(), tiletypemap.end(),
    []( const decltype( tiletypemap.begin() ) & v ) {
        // c_str works for std::string and for string_id!
        return v->first.c_str();
    }, label, prefix );
}

template <typename base_type>
void cata_tiles::tile_loading_report( const size_t count, const std::string &label,
                                      const std::string &prefix )
{
    lr_generic( static_cast<size_t>( 0 ), count,
    []( const size_t i ) {
        return int_id<base_type>( i ).id().str();
    }, label, prefix );
}

template <typename arraytype>
void cata_tiles::tile_loading_report( const arraytype &array, int array_length,
                                      const std::string &label, const std::string &prefix )
{
    const auto begin = &( array[0] );
    lr_generic( begin, begin + array_length,
    []( decltype( begin ) const v ) {
        return v->id;
    }, label, prefix );
}

std::vector<options_manager::id_and_option> cata_tiles::build_renderer_list()
{
    std::vector<options_manager::id_and_option> renderer_names;
    std::vector<options_manager::id_and_option> default_renderer_names = {
#if defined(TILES)
#   if defined(_WIN32)
        { "direct3d", translate_marker( "direct3d" ) },
#   endif
        { "software", translate_marker( "software" ) },
        { "opengl", translate_marker( "opengl" ) },
        { "opengles2", translate_marker( "opengles2" ) },
#else
        { "software", translate_marker( "software" ) }
#endif

    };
    int numRenderDrivers = SDL_GetNumRenderDrivers();
    DebugLog( D_INFO, DC_ALL ) << "Number of render drivers on your system: " << numRenderDrivers;
    for( int ii = 0; ii < numRenderDrivers; ii++ ) {
        SDL_RendererInfo ri;
        SDL_GetRenderDriverInfo( ii, &ri );
        DebugLog( D_INFO, DC_ALL ) << "Render driver: " << ii << "/" << ri.name;
        // First default renderer name we will put first on the list. We can use it later as default value.
        if( ri.name == default_renderer_names.front().first ) {
            renderer_names.emplace( renderer_names.begin(), default_renderer_names.front() );
        } else {
            renderer_names.emplace_back( ri.name, ri.name );
        }

    }

    return renderer_names.empty() ? default_renderer_names : renderer_names;
}

#endif // SDL_TILES
