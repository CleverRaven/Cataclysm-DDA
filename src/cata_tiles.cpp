#if defined(TILES)
#include "cata_tiles.h"

#include <algorithm>
#include <array>
#include <bitset>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <optional>
#include <set>
#include <stdexcept>
#include <tuple>
#include <unordered_set>
#include <variant>

#include "action.h"
#include "avatar.h"
#include "cached_options.h"
#include "calendar.h"
#include "cata_assert.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "character.h"
#include "character_id.h"
#include "clzones.h"
#include "color.h"
#include "creature_tracker.h"
#include "cursesdef.h"
#include "cursesport.h"
#include "debug.h"
#include "field.h"
#include "field_type.h"
#include "filesystem.h"
#include "game.h"
#include "game_constants.h"
#include "int_id.h"
#include "item.h"
#include "item_factory.h"
#include "itype.h"
#include "json.h"
#include "json_loader.h"
#include "map.h"
#include "map_extras.h"
#include "map_memory.h"
#include "mapdata.h"
#include "mod_tileset.h"
#include "monster.h"
#include "monstergenerator.h"
#include "mtype.h"
#include "npc.h"
#include "output.h"
#include "overlay_ordering.h"
#include "path_info.h"
#include "pixel_minimap.h"
#include "rect_range.h"
#include "scent_map.h"
#include "sdl_utils.h"
#include "sdl_wrappers.h"
#include "sdltiles.h"
#include "sounds.h"
#include "string_formatter.h"
#include "string_id.h"
#include "submap.h"
#include "tileray.h"
#include "translations.h"
#include "trap.h"
#include "type_id.h"
#include "units_utility.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "weather.h"
#include "weighted_list.h"

#define dbg(x) DebugLog((x),D_SDL) << __FILE__ << ":" << __LINE__ << ": "

static const efftype_id effect_ridden( "ridden" );

static const itype_id itype_corpse( "corpse" );
static const mon_flag_str_id mon_flag_ALWAYS_VISIBLE( "ALWAYS_VISIBLE" );
static const trait_id trait_INATTENTIVE( "INATTENTIVE" );
static const trap_str_id tr_unfinished_construction( "tr_unfinished_construction" );

static const std::string ITEM_HIGHLIGHT( "highlight_item" );
static const std::string ZOMBIE_REVIVAL_INDICATOR( "zombie_revival_indicator" );

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

static const std::string empty_string;
static const std::array<std::string, 15> TILE_CATEGORY_IDS = {{
        "", // TILE_CATEGORY::NONE,
        "vehicle_part", // TILE_CATEGORY::VEHICLE_PART,
        "terrain", // TILE_CATEGORY::TERRAIN,
        "item", // TILE_CATEGORY::ITEM,
        "furniture", // TILE_CATEGORY::FURNITURE,
        "trap", // TILE_CATEGORY::TRAP,
        "field", // TILE_CATEGORY::FIELD,
        "lighting", // TILE_CATEGORY::LIGHTING,
        "monster", // TILE_CATEGORY::MONSTER,
        "bullet", // TILE_CATEGORY::BULLET,
        "hit_entity", // TILE_CATEGORY::HIT_ENTITY,
        "weather", // TILE_CATEGORY::WEATHER,
        "overmap_terrain", // TILE_CATEGORY::OVERMAP_TERRAIN
        "map_extra", // TILE_CATEGORY::MAP_EXTRA
        "overmap_note", // TILE_CATEGORY::OVERMAP_NOTE
    }
};

static_assert( TILE_CATEGORY_IDS.size() == static_cast<size_t>( TILE_CATEGORY::last ),
               "TILE_CATEGORY_IDS must match list of TILE_CATEGORY values" );

namespace
{

std::string get_ascii_tile_id( const uint32_t sym, const int FG, const int BG )
{
    return std::string( { 'A', 'S', 'C', 'I', 'I', '_', static_cast<char>( sym ),
                          static_cast<char>( FG ), static_cast<char>( BG )
                        } );
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
                                const direction text_direction )
    : text( text ), color( color )
{
    switch( text_direction ) {
        case direction::NORTHWEST:
        case direction::WEST:
        case direction::SOUTHWEST:
            alignment = text_alignment::right;
            break;
        case direction::NORTH:
        case direction::CENTER:
        case direction::SOUTH:
            alignment = text_alignment::center;
            break;
        default:
            alignment = text_alignment::left;
            break;
    }
}

cata_tiles::cata_tiles( const SDL_Renderer_Ptr &renderer, const GeometryRenderer_Ptr &geometry,
                        tileset_cache &cache ) :
    renderer( renderer ),
    geometry( geometry ),
    cache( cache ),
    minimap( renderer, geometry )
{
    cata_assert( renderer );

    tile_height = 0;
    tile_width = 0;

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

    settings.mode =
        pixel_minimap_mode_from_string( get_option<std::string>( "PIXEL_MINIMAP_MODE" ) );
    settings.brightness = get_option<int>( "PIXEL_MINIMAP_BRIGHTNESS" );
    settings.beacon_size = get_option<int>( "PIXEL_MINIMAP_BEACON_SIZE" );
    settings.beacon_blink_interval = get_option<int>( "PIXEL_MINIMAP_BLINK" );
    settings.square_pixels = get_option<bool>( "PIXEL_MINIMAP_RATIO" );
    settings.scale_to_fit = get_option<bool>( "PIXEL_MINIMAP_SCALE_TO_FIT" );

    minimap->set_settings( settings );
}

void tileset::clear()
{
    tile_values.clear();
    shadow_tile_values.clear();
    night_tile_values.clear();
    overexposed_tile_values.clear();
    memory_tile_values.clear();
    duplicate_ids.clear();
    tile_ids.clear();
    for( std::unordered_map<std::string, season_tile_value> &m : tile_ids_by_season ) {
        m.clear();
    }
    item_layer_data.clear();
    field_layer_data.clear();
}

const tile_type *tileset::find_tile_type( const std::string &id ) const
{
    const auto iter = tile_ids.find( id );
    return iter != tile_ids.end() ? &iter->second : nullptr;
}

std::optional<tile_lookup_res>
tileset::find_tile_type_by_season( const std::string &id, season_type season ) const
{
    cata_assert( season < season_type::NUM_SEASONS );
    const auto iter = tile_ids_by_season[season].find( id );

    if( iter == tile_ids_by_season[season].end() ) {
        return std::nullopt;
    }
    const tileset::season_tile_value &res = iter->second;
    if( res.season_tile ) {
        return *res.season_tile;
    } else if( res.default_tile ) { // can skip this check, but just in case
        return tile_lookup_res( iter->first, *res.default_tile );
    }
    debugmsg( "empty record found in `tile_ids_by_season` for key: %s", id );
    return std::nullopt;
}

tile_type &tileset::create_tile_type( const std::string &id, tile_type &&new_tile_type )
{
    // Must overwrite existing tile
    // TODO: c++17 - replace [] + find() with insert_or_assign()
    tile_ids[id] = std::move( new_tile_type );
    auto inserted = tile_ids.find( id );

    const std::string &inserted_id = inserted->first;
    tile_type &inserted_tile = inserted->second;

    // populate cache by season
    constexpr size_t suffix_len = 15;
    // NOLINTNEXTLINE(cata-use-mdarray,modernize-avoid-c-arrays)
    constexpr char season_suffix[NUM_SEASONS][suffix_len] = {
        "_season_spring", "_season_summer", "_season_autumn", "_season_winter"
    };
    bool has_season_suffix = false;
    for( int i = 0; i < NUM_SEASONS; i++ ) {
        if( string_ends_with( id, season_suffix[i] ) ) {
            has_season_suffix = true;
            // key is id without _season suffix
            season_tile_value &value = tile_ids_by_season[i][id.substr( 0,
                                       id.size() - strlen( season_suffix[i] ) )];
            // value stores reference to string id with _season suffix
            value.season_tile = tile_lookup_res( inserted_id, inserted_tile );
            break;
        }
    }
    // tile doesn't have _season suffix, add it as "default" into all four seasons
    if( !has_season_suffix ) {
        for( auto &by_season_map : tile_ids_by_season ) {
            by_season_map[id].default_tile = &inserted_tile;
        }
    }

    return inserted_tile;
}

void cata_tiles::load_tileset( const std::string &tileset_id, const bool precheck,
                               const bool force, const bool pump_events )
{
    if( tileset_ptr && tileset_ptr->get_tileset_id() == tileset_id && !force ) {
        return;
    }
    // TODO: move into clear or somewhere else.
    // reset the overlay ordering from the previous loaded tileset
    tileset_mutation_overlay_ordering.clear();

    tileset_ptr = cache.load_tileset( tileset_id, renderer, precheck, force, pump_events );

    set_draw_scale( 16 );

    // Precalculate fog transparency
    // On isometric tilesets, fog intensity scales with zlevel_height in tile_config.json
    fog_alpha = is_isometric() ? std::min( std::max( int( 255.0f - 255.0f * pow( 155.0f / 255.0f,
                                           zlevel_height / 100.0f ) ), 40 ), 150 ) : 100;
}

void cata_tiles::reinit()
{
    set_draw_scale( 16 );
    RenderClear( renderer );
}

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
    throwErrorIf( SDL_BlitSurface( original.get(), nullptr, surf.get(), nullptr ) != 0,
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

static bool is_contained( const SDL_Rect &smaller, const SDL_Rect &larger )
{
    return smaller.x >= larger.x &&
           smaller.y >= larger.y &&
           smaller.x + smaller.w <= larger.x + larger.w &&
           smaller.y + smaller.h <= larger.y + larger.h;
}

void tileset_cache::loader::copy_surface_to_texture( const SDL_Surface_Ptr &surf,
        const point &offset, std::vector<texture> &target )
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
        target[index] = texture( texture_ptr, rect );
    }
}

void tileset_cache::loader::create_textures_from_tile_atlas( const SDL_Surface_Ptr &tile_atlas,
        const point &offset )
{
    cata_assert( tile_atlas );

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

void tileset_cache::loader::load_tileset( const cata_path &img_path, const bool pump_events )
{
    cata_assert( sprite_width > 0 );
    cata_assert( sprite_height > 0 );
    const SDL_Surface_Ptr tile_atlas = load_image( img_path.get_unrelative_path().u8string().c_str() );
    cata_assert( tile_atlas );
    tile_atlas_width = tile_atlas->w;

    if( R >= 0 && R <= 255 && G >= 0 && G <= 255 && B >= 0 && B <= 255 ) {
        const Uint32 key = SDL_MapRGB( tile_atlas->format, 0, 0, 0 );
        throwErrorIf( SDL_SetColorKey( tile_atlas.get(), SDL_TRUE, key ) != 0,
                      "SDL_SetColorKey failed" );
        throwErrorIf( SDL_SetSurfaceRLE( tile_atlas.get(), 1 ), "SDL_SetSurfaceRLE failed" );
    }

    SDL_RendererInfo info;
    throwErrorIf( SDL_GetRendererInfo( renderer.get(), &info ) != 0, "SDL_GetRendererInfo failed" );
    // Software rendering stores textures as surfaces with run-length encoding, which makes
    // extracting a part in the middle of the texture slow. Therefore this "simulates" that the
    // renderer only supports one tile
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
        DebugLog( D_INFO, DC_ALL ) << "SDL_RendererInfo max_texture_width was set to 0. " <<
                                   " Changing it to " << info.max_texture_width;
    } else {
        throwErrorIf( info.max_texture_width < sprite_width,
                      "Maximal texture width is smaller than tile width" );
    }

    if( info.max_texture_height == 0 ) {
        info.max_texture_height = sprite_height * min_tile_ycount;
        DebugLog( D_INFO, DC_ALL ) << "SDL_RendererInfo max_texture_height was set to 0. " <<
                                   " Changing it to " << info.max_texture_height;
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
        point( divide_round_up( tile_atlas->w, info.max_texture_width ),
               divide_round_up( tile_atlas->h, info.max_texture_height ) ) );

    const int expected_tilecount = ( tile_atlas->w / sprite_width ) *
                                   ( tile_atlas->h / sprite_height );
    extend_vector_by( ts.tile_values, expected_tilecount );
    extend_vector_by( ts.shadow_tile_values, expected_tilecount );
    extend_vector_by( ts.night_tile_values, expected_tilecount );
    extend_vector_by( ts.overexposed_tile_values, expected_tilecount );
    extend_vector_by( ts.memory_tile_values, expected_tilecount );

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
            throwErrorIf( SDL_BlitSurface( tile_atlas.get(), &inp, smaller_surf.get(),
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

void cata_tiles::set_draw_scale( int scale )
{
    cata_assert( tileset_ptr );
    const int mult = tileset_ptr->get_tile_pixelscale() * scale;
    const int div = 16;
    tile_width = tileset_ptr->get_tile_width() * mult / div;
    tile_height = tileset_ptr->get_tile_height() * mult / div;
    max_tile_extent = tileset_ptr->get_max_tile_extent();
    // Rounding down because the extent may be negative
    max_tile_extent.p_min.x = divide_round_down( max_tile_extent.p_min.x * mult, div );
    max_tile_extent.p_min.y = divide_round_down( max_tile_extent.p_min.y * mult, div );
    max_tile_extent.p_max.x = divide_round_down( max_tile_extent.p_max.x * mult, div );
    max_tile_extent.p_max.y = divide_round_down( max_tile_extent.p_max.y * mult, div );
    zlevel_height = tileset_ptr->get_zlevel_height();
}

void tileset_cache::loader::load( const std::string &tileset_id, const bool precheck,
                                  const bool pump_events )
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

    cata_path json_path = tileset_root / fs::u8path( json_conf );
    cata_path img_path = tileset_root / fs::u8path( tileset_path );
    cata_path layering_path = tileset_root / fs::u8path( layering );

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
        ts.max_tile_extent = half_open_rectangle<point>( point_zero, { ts.tile_width, ts.tile_height } );
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

    if( !ts.find_tile_type( "unknown" ) ) {
        dbg( D_ERROR ) << "The tileset you're using has no 'unknown' tile defined!";
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
                    std::min( ts.max_tile_extent.p_min.x,
                              std::min( sprite_offset.x, sprite_offset_retracted.x ) ),
                    std::min( ts.max_tile_extent.p_min.y,
                              std::min( sprite_offset.y, sprite_offset_retracted.y ) ),
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
        sprite_offset = point_zero;
        sprite_offset_retracted = point_zero;
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
            context = item.get_string( "context" );
            std::vector<layer_variant> item_variants;
            std::vector<layer_variant> field_variants;
            if( item.has_array( "item_variants" ) ) {
                for( const JsonObject vars : item.get_array( "item_variants" ) ) {
                    if( vars.has_member( "item" ) && vars.has_array( "sprite" ) && vars.has_member( "layer" ) ) {
                        layer_variant v;
                        v.id = vars.get_string( "item" );

                        v.layer = vars.get_int( "layer" );
                        v.offset = point( vars.get_int( "offset_x", 0 ), vars.get_int( "offset_y", 0 ) );

                        int total_weight = 0;
                        for( const JsonObject sprites : vars.get_array( "sprite" ) ) {
                            std::string id = sprites.get_string( "id" );
                            int weight = sprites.get_int( "weight", 1 );
                            v.sprite.emplace( id, weight );

                            total_weight += weight;
                        }
                        v.total_weight = total_weight;
                        item_variants.push_back( v );
                    } else {
                        config.throw_error( "item_variants configured incorrectly" );
                    }
                }
                // sort them based on layering so we can draw them correctly
                std::sort( item_variants.begin(), item_variants.end(), []( const layer_variant & a,
                const layer_variant & b ) {
                    return a.layer < b.layer;
                } );
                ts.item_layer_data.emplace( context, item_variants );
            }
            if( item.has_array( "field_variants" ) ) {
                for( const JsonObject vars : item.get_array( "field_variants" ) ) {
                    if( vars.has_member( "field" ) && vars.has_array( "sprite" ) ) {
                        layer_variant v;
                        v.id = vars.get_string( "field" );
                        v.offset = point( vars.get_int( "offset_x", 0 ), vars.get_int( "offset_y", 0 ) );

                        int total_weight = 0;
                        for( const JsonObject sprites : vars.get_array( "sprite" ) ) {
                            std::string id = sprites.get_string( "id" );
                            int weight = sprites.get_int( "weight", 1 );
                            v.sprite.emplace( id, weight );

                            total_weight += weight;
                        }
                        v.total_weight = total_weight;
                        field_variants.push_back( v );
                    } else {
                        config.throw_error( "field_variants configured incorrectly" );
                    }
                }
                ts.field_layer_data.emplace( context, field_variants );
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
    [&]( const weighted_object<int, std::vector<int>> &o ) {
        return o.obj.empty();
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
        const std::string_view objname ) const
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

struct tile_render_info {
    struct common {
        const tripoint pos;
        // accumulator for 3d tallness of sprites rendered here so far;
        int height_3d = 0;
        int draw_min_z = -OVERMAP_DEPTH;

        common( const tripoint &pos, const int height_3d, const int draw_min_z )
            : pos( pos ), height_3d( height_3d ), draw_min_z( draw_min_z ) {}
    };

    struct vision_effect {
        visibility_type vis;

        explicit vision_effect( const visibility_type vis )
            : vis( vis ) {}
    };

    struct sprite {
        lit_level ll;
        std::array<bool, 5> invisible;

        sprite( const lit_level ll, const std::array<bool, 5> &inv )
            : ll( ll ), invisible( inv ) {}
    };

    common com;
    std::variant<vision_effect, sprite> var;

    tile_render_info( const common &com, const vision_effect &var )
        : com( com ), var( var ) {}

    tile_render_info( const common &com, const sprite &var )
        : com( com ), var( var ) {}
};

static std::map<tripoint, int> display_npc_attack_potential()
{
    avatar &you = get_avatar();
    npc avatar_as_npc;
    std::ostringstream os;
    JsonOut jsout( os );
    jsout.write( you );
    JsonValue jsin = json_loader::from_string( os.str() );
    jsin.read( avatar_as_npc );
    avatar_as_npc.regen_ai_cache();
    avatar_as_npc.evaluate_best_weapon( nullptr );
    std::map<tripoint, int> effectiveness_map;
    std::vector<npc_attack_rating> effectiveness =
        avatar_as_npc.get_current_attack()->all_evaluations( avatar_as_npc, nullptr );
    for( const npc_attack_rating &effectiveness_at_point : effectiveness ) {
        if( !effectiveness_at_point.value() ) {
            continue;
        }
        effectiveness_map[effectiveness_at_point.target()] = *effectiveness_at_point.value();
    }
    return effectiveness_map;
}

void cata_tiles::draw( const point &dest, const tripoint &center, int width, int height,
                       std::multimap<point, formatted_text> &overlay_strings,
                       color_block_overlay_container &color_blocks )
{
    if( !g ) {
        return;
    }

#if defined(__ANDROID__)
    // Attempted bugfix for Google Play crash - prevent divide-by-zero if no tile
    // width/height specified
    if( tile_width == 0 || tile_height == 0 ) {
        return;
    }
#endif

    {
        //set clipping to prevent drawing over stuff we shouldn't
        SDL_Rect clipRect = {dest.x, dest.y, width, height};
        printErrorIf( SDL_RenderSetClipRect( renderer.get(), &clipRect ) != 0,
                      "SDL_RenderSetClipRect failed" );

        //fill render area with black to prevent artifacts where no new pixels are drawn
        geometry->rect( renderer, clipRect, SDL_Color() );
    }

    const point s = get_window_base_tile_counts( point( width, height ) );

    init_light();
    map &here = get_map();
    const visibility_variables &cache = here.get_visibility_variables_cache();

    o = is_isometric() ? center.xy() : center.xy() - point( POSX, POSY );

    op = dest;
    screentile_width = s.x;
    screentile_height = s.y;

    // Limit draw depth to vertical vision setting
    const int max_draw_depth = fov_3d_z_range;
    const int num_ranges = is_isometric() ? 1 + std::max( 0, max_draw_depth ) : 1;
    std::vector<half_open_rectangle<point>> z_any_tile_range( num_ranges );
    for( int z = 0; z < num_ranges; ++z ) {
        z_any_tile_range[z] = get_window_any_tile_range( { width, height }, -z );
    }
    const half_open_rectangle<point> &top_any_tile_range = z_any_tile_range[0];
    const half_open_rectangle<point> &bottom_any_tile_range = z_any_tile_range[num_ranges - 1];
    cata_assert( top_any_tile_range.p_min.x == bottom_any_tile_range.p_min.x );
    cata_assert( top_any_tile_range.p_max.x == bottom_any_tile_range.p_max.x );
    cata_assert( top_any_tile_range.p_min.y >= bottom_any_tile_range.p_min.y );
    cata_assert( top_any_tile_range.p_max.y >= bottom_any_tile_range.p_max.y );
    const int min_col = top_any_tile_range.p_min.x;
    const int max_col = top_any_tile_range.p_max.x;
    const int min_row = bottom_any_tile_range.p_min.y;
    const int max_row = top_any_tile_range.p_max.y;

    avatar &you = get_avatar();
    //limit the render area to maximum view range (121x121 square centered on player)
    const point min_visible( you.posx() % SEEX, you.posy() % SEEY );
    const point max_visible( ( you.posx() % SEEX ) + ( MAPSIZE - 1 ) * SEEX,
                             ( you.posy() % SEEY ) + ( MAPSIZE - 1 ) * SEEY );

    const level_cache &ch = here.access_cache( center.z );

    // Map memory should be at least the size of the view range
    // so that new tiles can be memorized, and at least the size of the display
    // since at farthest zoom displayed area may be bigger than view range.
    point min_mm_reg = min_visible;
    point max_mm_reg = max_visible;
    if( is_isometric() ) {
        std::optional<point> northmost = tile_to_player( { min_col, min_row } );
        if( !northmost.has_value() ) {
            northmost = tile_to_player( { min_col + 1, min_row } );
        }
        std::optional<point> southmost = tile_to_player( { max_col, max_row } );
        if( !southmost.has_value() ) {
            southmost = tile_to_player( { max_col - 1, max_row } );
        }
        std::optional<point> westmost = tile_to_player( { min_col, max_row } );
        if( !westmost.has_value() ) {
            westmost = tile_to_player( { min_col + 1, max_row } );
        }
        std::optional<point> eastmost = tile_to_player( { max_col, min_row } );
        if( !eastmost.has_value() ) {
            eastmost = tile_to_player( { max_col - 1, min_row } );
        }
        if( northmost.has_value() && southmost.has_value()
            && westmost.has_value() && eastmost.has_value() ) {
            min_mm_reg = point( std::min( min_mm_reg.x, westmost->x ),
                                std::min( min_mm_reg.y, northmost->y ) );
            max_mm_reg = point( std::max( max_mm_reg.x, eastmost->x ),
                                std::max( max_mm_reg.y, southmost->y ) );
        }
    } else {
        std::optional<point> northwest = tile_to_player( { min_col, min_row } );
        std::optional<point> southeast = tile_to_player( { max_col, max_row } );
        if( northwest.has_value() && southeast.has_value() ) {
            min_mm_reg = point( std::min( min_mm_reg.x, northwest->x ),
                                std::min( max_mm_reg.y, northwest->y ) );
            max_mm_reg = point( std::max( max_mm_reg.x, southeast->x ),
                                std::max( max_mm_reg.y, southeast->y ) );
        }
    }
    you.prepare_map_memory_region(
        here.getglobal( tripoint( min_mm_reg, center.z ) ),
        here.getglobal( tripoint( max_mm_reg, center.z ) )
    );

    //set up a default tile for the edges outside the render area
    visibility_type offscreen_type = visibility_type::HIDDEN;
    if( cache.u_is_boomered ) {
        offscreen_type = visibility_type::BOOMER_DARK;
    }

    //retrieve night vision goggle status once per draw
    auto vision_cache = you.get_vision_modes();
    nv_goggles_activated = vision_cache[NV_GOGGLES];

    // check that the creature for which we'll draw the visibility map is still alive at that point
    if( g->display_overlay_state( ACTION_DISPLAY_VISIBILITY ) &&
        g->displaying_visibility_creature ) {
        const Creature *creature = g->displaying_visibility_creature;
        const auto is_same_creature_predicate = [&creature]( const Creature & c ) {
            return creature == &c;
        };
        if( g->get_creature_if( is_same_creature_predicate ) == nullptr )  {
            g->displaying_visibility_creature = nullptr;
        }
    }
    const point half_tile( tile_width / 2, 0 );
    const point quarter_tile( tile_width / 4, tile_height / 4 );
    if( g->display_overlay_state( ACTION_DISPLAY_VEHICLE_AI ) ) {
        for( const wrapped_vehicle &elem : here.get_vehicles() ) {
            const vehicle &veh = *elem.v;
            const point veh_pos = veh.global_pos3().xy();
            for( const auto &overlay_data : veh.get_debug_overlay_data() ) {
                const point pt = veh_pos + std::get<0>( overlay_data );
                const int color = std::get<1>( overlay_data );
                const std::string &text = std::get<2>( overlay_data );
                overlay_strings.emplace( player_to_screen( pt ),
                                         formatted_text( text, color,
                                                 text_alignment::left ) );
            }
        }
    }
    const auto apply_visible = [&]( const tripoint & np, const level_cache & ch, map & here ) {
        return np.y < min_visible.y || np.y > max_visible.y ||
               np.x < min_visible.x || np.x > max_visible.x ||
               would_apply_vision_effects( here.get_visibility( ch.visibility_cache[np.x][np.y],
                                           cache ) );
    };
    std::map<tripoint, int> npc_attack_rating_map;
    int max_npc_effectiveness = 0;
    if( g->display_overlay_state( ACTION_DISPLAY_NPC_ATTACK_POTENTIAL ) ) {
        npc_attack_rating_map = display_npc_attack_potential();
        for( const std::pair<const tripoint, int> &pair : npc_attack_rating_map ) {
            max_npc_effectiveness = std::max( pair.second, max_npc_effectiveness );
        }
    }

    creature_tracker &creatures = get_creature_tracker();
    std::map<int, std::vector<tile_render_info>> draw_points;
    for( int row = min_row; row < max_row; row ++ ) {
        draw_points[row].reserve( std::max( 0, max_col - min_col ) );
        for( int col = min_col; col < max_col; col ++ ) {
            const std::optional<point> temp = tile_to_player( { col, row } );
            if( !temp.has_value() ) {
                continue;
            }
            const tripoint pos( temp.value(), center.z );
            const tripoint_abs_ms pos_global = here.getglobal( pos );
            const int &x = pos.x;
            const int &y = pos.y;

            // light level is used for choosing between grayscale filter and normal lit tiles.
            lit_level ll;
            // invisible to normal eyes
            std::array<bool, 5> invisible;
            invisible[0] = false;

            if( y < min_visible.y || y > max_visible.y || x < min_visible.x || x > max_visible.x ) {
                if( has_memory_at( pos_global ) ) {
                    ll = lit_level::MEMORIZED;
                    invisible[0] = true;
                } else if( has_draw_override( pos ) ) {
                    ll = lit_level::DARK;
                    invisible[0] = true;
                } else {
                    if( would_apply_vision_effects( offscreen_type ) ) {
                        draw_points[row].emplace_back( tile_render_info::common{ pos, 0, pos.z },
                                                       tile_render_info::vision_effect{ offscreen_type } );
                    }
                    continue;
                }
            } else {
                ll = ch.visibility_cache[x][y];
            }

            // Add scent value to the overlay_strings list for every visible tile when
            // displaying scent
            if( g->display_overlay_state( ACTION_DISPLAY_SCENT ) && !invisible[0] ) {
                const int scent_value = get_scent().get( pos );
                if( scent_value > 0 ) {
                    overlay_strings.emplace( player_to_screen( point( x, y ) ) + half_tile,
                                             formatted_text( std::to_string( scent_value ),
                                                     8 + catacurses::yellow, direction::NORTH ) );
                }
            }

            // Add scent type to the overlay_strings list for every visible tile when
            // displaying scent
            if( g->display_overlay_state( ACTION_DISPLAY_SCENT_TYPE ) && !invisible[0] ) {
                const scenttype_id scent_type = get_scent().get_type( pos );
                if( !scent_type.is_empty() ) {
                    overlay_strings.emplace( player_to_screen( point( x, y ) ) + half_tile,
                                             formatted_text( scent_type.c_str(),
                                                     8 + catacurses::yellow, direction::NORTH ) );
                }
            }

            if( g->display_overlay_state( ACTION_DISPLAY_RADIATION ) ) {
                const auto rad_override = radiation_override.find( pos );
                const bool rad_overridden = rad_override != radiation_override.end();
                if( rad_overridden || !invisible[0] ) {
                    const int rad_value = rad_overridden ? rad_override->second :
                                          here.get_radiation( pos );
                    catacurses::base_color col;
                    if( rad_value > 0 ) {
                        col = catacurses::green;
                    } else {
                        col = catacurses::cyan;
                    }
                    overlay_strings.emplace( player_to_screen( point( x, y ) ) + half_tile,
                                             formatted_text( std::to_string( rad_value ),
                                                     8 + col, direction::NORTH ) );
                }
            }

            if( g->display_overlay_state( ACTION_DISPLAY_NPC_ATTACK_POTENTIAL ) ) {
                if( npc_attack_rating_map.count( pos ) ) {
                    const int val = npc_attack_rating_map.at( pos );
                    short color;
                    if( val <= 0 ) {
                        color = catacurses::red;
                    } else if( val == max_npc_effectiveness ) {
                        color = catacurses::cyan;
                    } else {
                        color = catacurses::white;
                    }
                    overlay_strings.emplace( player_to_screen( point( x, y ) ) + half_tile,
                                             formatted_text( std::to_string( val ), color,
                                                     direction::NORTH ) );
                }
            }

            // Add temperature value to the overlay_strings list for every visible tile when
            // displaying temperature
            if( g->display_overlay_state( ACTION_DISPLAY_TEMPERATURE ) && !invisible[0] ) {
                const units::temperature temp_value = get_weather().get_temperature( pos );
                const float celsius_temp_value = units::to_celsius( temp_value );
                short color;
                const short bold = 8;
                if( celsius_temp_value > 40 ) {
                    color = catacurses::red;
                } else if( celsius_temp_value > 25 ) {
                    color = catacurses::yellow + bold;
                } else if( celsius_temp_value > 10 ) {
                    color = catacurses::green + bold;
                } else if( celsius_temp_value > 0 ) {
                    color = catacurses::white + bold;
                } else if( celsius_temp_value > -10 ) {
                    color = catacurses::cyan + bold;
                } else {
                    color = catacurses::blue + bold;
                }

                std::string temp_str;
                if( get_option<std::string>( "USE_CELSIUS" ) == "celsius" ) {
                    temp_str = string_format( "%.0f", celsius_temp_value );
                } else if( get_option<std::string>( "USE_CELSIUS" ) == "kelvin" ) {
                    temp_str = string_format( "%.0f", units::to_kelvin( temp_value ) );
                } else {
                    temp_str = string_format( "%.0f", units::to_fahrenheit( temp_value ) );
                }
                overlay_strings.emplace( player_to_screen( point( x, y ) ),
                                         formatted_text( temp_str, color,
                                                 text_alignment::left ) );
            }

            if( g->display_overlay_state( ACTION_DISPLAY_VISIBILITY ) &&
                g->displaying_visibility_creature && !invisible[0] ) {
                const bool visibility = g->displaying_visibility_creature->sees( pos );

                // color overlay.
                SDL_Color block_color = visibility ? windowsPalette[catacurses::green] :
                                        SDL_Color{ 192, 192, 192, 255 };
                block_color.a = 100;
                color_blocks.first = SDL_BLENDMODE_BLEND;
                color_blocks.second.emplace( player_to_screen( point( x, y ) ), block_color );

                // overlay string
                std::string visibility_str = visibility ? "+" : "-";
                overlay_strings.emplace( player_to_screen( point( x, y ) ) + quarter_tile,
                                         formatted_text( visibility_str, catacurses::black,
                                                 direction::NORTH ) );
            }

            static std::vector<SDL_Color> lighting_colors;
            // color hue in the range of [0..10], 0 being white,  10 being blue
            auto draw_debug_tile = [&]( const int color_hue, const std::string & text ) {
                if( lighting_colors.empty() ) {
                    SDL_Color white = { 255, 255, 255, 255 };
                    SDL_Color blue = { 0, 0, 255, 255 };
                    lighting_colors = color_linear_interpolate( white, blue, 9 );
                }
                point tile_pos = player_to_screen( point( x, y ) );

                // color overlay
                SDL_Color color = lighting_colors[std::min( std::max( 0, color_hue ), 10 )];
                color.a = 100;
                color_blocks.first = SDL_BLENDMODE_BLEND;
                color_blocks.second.emplace( tile_pos, color );

                // string overlay
                overlay_strings.emplace(
                    tile_pos + quarter_tile,
                    formatted_text( text, catacurses::black, direction::NORTH ) );
            };

            if( g->display_overlay_state( ACTION_DISPLAY_LIGHTING ) ) {
                if( g->displaying_lighting_condition == 0 ) {
                    const float light = here.ambient_light_at( {x, y, center.z} );
                    // note: lighting will be constrained in the [1.0, 11.0] range.
                    int intensity =
                        static_cast<int>( std::max( 1.0, LIGHT_AMBIENT_LIT - light + 1.0 ) ) - 1;
                    draw_debug_tile( intensity, string_format( "%.1f", light ) );
                }
            }

            if( g->display_overlay_state( ACTION_DISPLAY_TRANSPARENCY ) ) {
                const float tr = here.light_transparency( {x, y, center.z} );
                int intensity =  tr <= LIGHT_TRANSPARENCY_SOLID ? 10 :  static_cast<int>
                                 ( ( tr - LIGHT_TRANSPARENCY_OPEN_AIR ) * 8 );
                draw_debug_tile( intensity, string_format( "%.2f", tr ) );
            }

            if( g->display_overlay_state( ACTION_DISPLAY_REACHABILITY_ZONES ) ) {
                tripoint tile_pos( x, y, center.z );
                int value = here.reachability_cache_value( tile_pos,
                            g->debug_rz_display.r_cache_vertical, g->debug_rz_display.quadrant );
                // use color to denote reachability from you to the target tile according to the
                // cache
                bool reachable = here.has_potential_los( you.pos(), tile_pos );
                draw_debug_tile( reachable ? 0 : 6, std::to_string( value ) );
            }

            if( !invisible[0] ) {
                const visibility_type vis_type = here.get_visibility( ll, cache );
                if( would_apply_vision_effects( vis_type ) ) {
                    const Creature *critter = creatures.creature_at( pos, true );
                    if( has_draw_override( pos ) || has_memory_at( pos_global ) ||
                        ( critter &&
                          ( critter->has_flag( mon_flag_ALWAYS_VISIBLE )
                            || you.sees_with_infrared( *critter )
                            || you.sees_with_specials( *critter ) ) ) ) {
                        invisible[0] = true;
                    } else {
                        draw_points[row].emplace_back( tile_render_info::common{ pos, 0, pos.z },
                                                       tile_render_info::vision_effect{ vis_type } );
                        continue;
                    }
                }
            }
            for( int i = 0; i < 4; i++ ) {
                const tripoint np = pos + neighborhood[i];
                invisible[1 + i] = apply_visible( np, ch, here );
            }

            // Determine lowest z-level to draw for 3D vision. Some off-screen
            // tiles are also considered here to simply the logic.
            int draw_min_z;
            tripoint p_temp = pos;
            while( !here.dont_draw_lower_floor( p_temp ) && pos.z - p_temp.z < max_draw_depth ) {
                p_temp.z -= 1;
            }
            draw_min_z = p_temp.z;

            draw_points[row].emplace_back( tile_render_info::common{ pos, 0, draw_min_z },
                                           tile_render_info::sprite{ ll, invisible } );
        }
    }

    // List all layers for a single z-level
    const std::array<decltype( &cata_tiles::draw_furniture ), 11> drawing_layers = {{
            &cata_tiles::draw_terrain, &cata_tiles::draw_furniture, &cata_tiles::draw_graffiti, &cata_tiles::draw_trap, &cata_tiles::draw_part_con,
            &cata_tiles::draw_field_or_item,
            &cata_tiles::draw_vpart_no_roof, &cata_tiles::draw_vpart_roof,
            &cata_tiles::draw_critter_at, &cata_tiles::draw_zone_mark,
            &cata_tiles::draw_zombie_revival_indicators
        }
    };

    // Legacy code to use when vertical vision range is 0
    const std::array<decltype( &cata_tiles::draw_furniture ), 14> drawing_layers_legacy = {{
            &cata_tiles::draw_terrain, &cata_tiles::draw_furniture, &cata_tiles::draw_graffiti, &cata_tiles::draw_trap, &cata_tiles::draw_part_con,
            &cata_tiles::draw_field_or_item, &cata_tiles::draw_vpart_below,
            &cata_tiles::draw_critter_at_below, &cata_tiles::draw_terrain_below,
            &cata_tiles::draw_vpart_no_roof, &cata_tiles::draw_vpart_roof,
            &cata_tiles::draw_critter_at, &cata_tiles::draw_zone_mark,
            &cata_tiles::draw_zombie_revival_indicators
        }
    };

    // Skip drawing shadow of critters above if there is no shadow sprite
    bool do_draw_shadow = false;
    if( find_tile_looks_like( "shadow", TILE_CATEGORY::NONE, "" ) ) {
        do_draw_shadow = true;
    }

    if( max_draw_depth <= 0 ) {
        // Legacy draw mode
        for( int row = min_row; row < max_row; row ++ ) {
            for( auto f : drawing_layers_legacy ) {
                for( tile_render_info &p : draw_points[row] ) {
                    if( const tile_render_info::vision_effect * const
                        var = std::get_if<tile_render_info::vision_effect>( &p.var ) ) {
                        if( f == &cata_tiles::draw_terrain ) {
                            apply_vision_effects( p.com.pos, var->vis, p.com.height_3d );
                        }
                    } else if( const tile_render_info::sprite * const
                               var = std::get_if<tile_render_info::sprite>( &p.var ) ) {
                        ( this->*f )( p.com.pos, var->ll, p.com.height_3d, var->invisible, false );
                    }
                }
            }
        }
    } else {
        // Multi z-level draw mode
        // Start drawing from the lowest visible z-level (some off-screen tiles
        // are considered visible here to simplify the logic.)
        int cur_zlevel = center.z + 1;
        for( const std::pair<const int, std::vector<tile_render_info>> &pts : draw_points ) {
            for( const tile_render_info &p : pts.second ) {
                cur_zlevel = std::min( cur_zlevel, p.com.draw_min_z );
                if( cur_zlevel <= center.z - max_draw_depth
                    || cur_zlevel <= -OVERMAP_DEPTH ) {
                    break;
                }
            }
        }
        while( cur_zlevel <= center.z ) {
            const half_open_rectangle<point> &cur_any_tile_range = is_isometric()
                    ? z_any_tile_range[center.z - cur_zlevel] : top_any_tile_range;
            // For each row
            for( int row = cur_any_tile_range.p_min.y; row < cur_any_tile_range.p_max.y; row ++ ) {
                // Set base height for each tile
                for( tile_render_info &p : draw_points[row] ) {
                    p.com.height_3d = ( cur_zlevel - center.z ) * zlevel_height;
                }
                // For each layer
                for( auto f : drawing_layers ) {
                    // For each tile
                    for( tile_render_info &p : draw_points[row] ) {
                        // Skip if z-level less than draw_min_z
                        // Basically occlusion culling
                        if( cur_zlevel < p.com.draw_min_z ) {
                            continue;
                        }
                        tripoint draw_loc = p.com.pos;
                        draw_loc.z = cur_zlevel;
                        if( const tile_render_info::vision_effect * const
                            var = std::get_if<tile_render_info::vision_effect>( &p.var ) ) {
                            if( f == &cata_tiles::draw_terrain ) {
                                apply_vision_effects( draw_loc, var->vis, p.com.height_3d );
                            }
                        } else if( const tile_render_info::sprite * const
                                   var = std::get_if<tile_render_info::sprite>( &p.var ) ) {

                            // Get visibility variables
                            lit_level ll;
                            std::array<bool, 5> invisible;
                            if( cur_zlevel == center.z ) {
                                // For the same z-level, use tile_render_info vars
                                ll = var->ll;
                                invisible = var->invisible;
                            } else {
                                // Otherwise, recalculate ll and invisible
                                if( here.ll_invis_cache.count( draw_loc ) == 0 ) {
                                    const std::pair<lit_level, std::array<bool, 5>> ll_invis = calc_ll_invis( draw_loc );
                                    ll = ll_invis.first;
                                    invisible = ll_invis.second;
                                    // Only cache ll_invis if not in test mode
                                    if( !test_mode ) {
                                        here.ll_invis_cache[ draw_loc ] = ll_invis;
                                    }
                                } else {
                                    ll = here.ll_invis_cache[ draw_loc ].first;
                                    invisible = here.ll_invis_cache[ draw_loc ].second;
                                }
                            }

                            if( f == &cata_tiles::draw_vpart_no_roof || f == &cata_tiles::draw_vpart_roof ) {
                                int temp_height_3d = p.com.height_3d;
                                // Reset height_3d to base when drawing vehicles
                                p.com.height_3d = ( cur_zlevel - center.z ) * zlevel_height;
                                // Draw
                                if( !( this->*f )( draw_loc, ll, p.com.height_3d, invisible, false ) ) {
                                    // If no vpart drawn, revert height_3d changes
                                    p.com.height_3d = temp_height_3d;
                                }
                            } else if( f == &cata_tiles::draw_critter_at ) {
                                // Draw
                                if( !( this->*f )( draw_loc, ll, p.com.height_3d, invisible, false ) && do_draw_shadow &&
                                    cur_zlevel == p.com.draw_min_z ) {
                                    // Draw shadow of flying critters on bottom-most tile if no other critter drawn
                                    draw_critter_above( draw_loc, ll, p.com.height_3d, invisible );
                                }
                            } else {
                                // Draw
                                ( this->*f )( draw_loc, ll, p.com.height_3d, invisible, false );
                            }
                        }
                    }
                }
            }
            cur_zlevel += 1;
        }
    }

    // display number of monsters to spawn in mapgen preview
    for( int row = top_any_tile_range.p_min.y; row < top_any_tile_range.p_max.y; row ++ ) {
        for( const tile_render_info &p : draw_points[row] ) {
            const tile_render_info::sprite *const
            var = std::get_if<tile_render_info::sprite>( &p.var );
            if( !var ) {
                continue;
            }
            const auto mon_override = monster_override.find( p.com.pos );
            if( mon_override != monster_override.end() ) {
                const int count = std::get<1>( mon_override->second );
                const bool more = std::get<2>( mon_override->second );
                if( count > 1 || more ) {
                    std::string text = "x" + std::to_string( count );
                    if( more ) {
                        text += "+";
                    }
                    overlay_strings.emplace( player_to_screen( p.com.pos.xy() ) + half_tile,
                                             formatted_text( text, catacurses::red,
                                                     direction::NORTH ) );
                }
            }
            if( !var->invisible[0] ) {
                here.memory_cache_ter_set_dirty( p.com.pos, false );
            }
        }
    }
    // tile overrides are already drawn in the previous code
    void_radiation_override();
    void_terrain_override();
    void_furniture_override();
    void_graffiti_override();
    void_trap_override();
    void_field_override();
    void_item_override();
    void_vpart_override();
    void_draw_below_override();
    void_monster_override();

    //Memorize everything the character just saw even if it wasn't displayed.
    for( int mem_y = min_visible.y; mem_y <= max_visible.y; mem_y++ ) {
        for( int mem_x = min_visible.x; mem_x <= max_visible.x; mem_x++ ) {
            const point colrow = player_to_tile( { mem_x, mem_y } );
            if( is_isometric() && top_any_tile_range.contains( colrow ) ) {
                continue;
            }
            const tripoint p( mem_x, mem_y, center.z );
            lit_level lighting = ch.visibility_cache[p.x][p.y];
            // `apply_vision_effects` does not memorize anything so we only need
            // to call `would_apply_vision_effects` here.
            if( would_apply_vision_effects( here.get_visibility( lighting, cache ) ) ) {
                continue;
            }
            int height_3d = 0;
            std::array<bool, 5> invisible;
            invisible[0] = false;
            for( int i = 0; i < 4; i++ ) {
                const tripoint np = p + neighborhood[i];
                invisible[1 + i] = apply_visible( np, ch, here );
            }
            //calling draw to memorize (and only memorize) everything.
            //bypass cache check in case we learn something new about the terrain's connections
            draw_terrain( p, lighting, height_3d, invisible, true );
            if( here.memory_cache_dec_is_dirty( p ) ) {
                you.memorize_clear_decoration( here.getglobal( p ), "" );
                draw_furniture( p, lighting, height_3d, invisible, true );
                draw_trap( p, lighting, height_3d, invisible, true );
                draw_part_con( p, lighting, height_3d, invisible, true );
                draw_vpart_no_roof( p, lighting, height_3d, invisible, true );
                draw_vpart_roof( p, lighting, height_3d, invisible, true );
                here.memory_cache_dec_set_dirty( p, false );
            }
        }
    }

    in_animation = do_draw_explosion || do_draw_custom_explosion ||
                   do_draw_bullet || do_draw_hit || do_draw_line ||
                   do_draw_cursor || do_draw_highlight || do_draw_weather ||
                   do_draw_sct || do_draw_zones || do_draw_async_anim;

    draw_footsteps_frame( center );
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
        if( do_draw_async_anim ) {
            draw_async_anim();
        }
    } else if( you.view_offset != tripoint_zero && !you.in_vehicle ) {
        // check to see if player is located at ter
        draw_from_id_string( "cursor", TILE_CATEGORY::NONE, empty_string,
                             tripoint( g->ter_view_p.xy(), center.z ), 0, 0, lit_level::LIT,
                             false );
    }
    if( you.controlling_vehicle ) {
        std::optional<tripoint> indicator_offset = g->get_veh_dir_indicator_location( true );
        if( indicator_offset ) {
            draw_from_id_string( "cursor", TILE_CATEGORY::NONE, empty_string,
                                 indicator_offset->xy() +
                                 tripoint( you.posx(), you.posy(), center.z ),
                                 0, 0, lit_level::LIT, false );
        }
    }

    printErrorIf( SDL_RenderSetClipRect( renderer.get(), nullptr ) != 0,
                  "SDL_RenderSetClipRect failed" );
}

std::pair<lit_level, std::array<bool, 5>> cata_tiles::calc_ll_invis( const tripoint &draw_loc )
{
    avatar &you = get_avatar();
    map &here = get_map();
    creature_tracker &creatures = get_creature_tracker();
    const visibility_variables &cache = here.get_visibility_variables_cache();
    const point min_visible( you.posx() % SEEX, you.posy() % SEEY );
    const point max_visible( ( you.posx() % SEEX ) + ( MAPSIZE - 1 ) * SEEX,
                             ( you.posy() % SEEY ) + ( MAPSIZE - 1 ) * SEEY );
    const level_cache &ch = here.access_cache( draw_loc.z );
    const auto apply_visible = [&]( const tripoint & np, const level_cache & ch, map & here ) {
        return np.y < min_visible.y || np.y > max_visible.y ||
               np.x < min_visible.x || np.x > max_visible.x ||
               would_apply_vision_effects( here.get_visibility( ch.visibility_cache[np.x][np.y],
                                           cache ) );
    };

    lit_level ll  = lit_level::DARK;
    // invisible to normal eyes
    std::array<bool, 5> invisible;
    invisible[0] = false;
    tripoint pos = draw_loc;
    tripoint_abs_ms pos_global = here.getglobal( pos );

    if( draw_loc.y < min_visible.y || draw_loc.y > max_visible.y || draw_loc.x < min_visible.x ||
        draw_loc.x > max_visible.x ) {
        if( has_memory_at( pos_global ) ) {
            ll = lit_level::MEMORIZED;
            invisible[0] = true;
        } else if( has_draw_override( pos ) ) {
            ll = lit_level::DARK;
            invisible[0] = true;
        }
    } else {
        ll = here.access_cache( draw_loc.z ).visibility_cache[draw_loc.x][draw_loc.y];
    }

    if( !invisible[0] ) {
        const visibility_type vis_type = here.get_visibility( ll, cache );
        if( would_apply_vision_effects( vis_type ) ) {
            const Creature *critter = creatures.creature_at( pos, true );
            if( has_draw_override( pos ) || has_memory_at( pos_global ) ||
                ( critter &&
                  ( critter->has_flag( mon_flag_ALWAYS_VISIBLE )
                    || you.sees_with_infrared( *critter )
                    || you.sees_with_specials( *critter ) ) ) ) {
                invisible[0] = true;
            }
        }
    }
    for( int i = 0; i < 4; i++ ) {
        const tripoint np = pos + neighborhood[i];
        invisible[1 + i] = apply_visible( np, ch, here );
    }

    std::pair<lit_level, std::array<bool, 5>> ret( ll, invisible );
    return ret;
}

void cata_tiles::clear_draw_caches()
{
    get_map().ll_invis_cache.clear();
}

void cata_tiles::draw_minimap( const point &dest, const tripoint &center, int width, int height )
{
    minimap->set_type( is_isometric() ? pixel_minimap_type::iso : pixel_minimap_type::ortho );
    minimap->draw( SDL_Rect{ dest.x, dest.y, width, height }, center );
}

point cata_tiles::get_window_base_tile_counts( const point &size ) const
{
    if( is_isometric() ) {
        //  |---sx---|
        //  w        |
        //  ~
        //         1\  }h
        //  /\/\/\/\2\ --
        //  \/\/\/\3\/  |
        //  /\/\/\/\4\  sy
        //  \/\/\/\5\/  |
        //  /\/\/\/\6\ --
        //
        // The rhombuses above represent the basic tiles excluding any extra
        // pixel that represents things in the z direction. `w = tile_width / 2`
        // is half basic tile width and `h = tile_width / 4` is half basic tile
        // height. `tile_height` is unrelated to and can be large than the basic
        // tile height `tile_width / 2`.
        //
        // There is no strict requirement about the exact position of pixels
        // in a sprite, except that the basic tiles should be able to densely
        // tile the screen area, and any extra pixels should only be used to
        // represent the z direction because they will be occluded by sprites in
        // the next row and occlude sprites in the previous row.
        //
        // Only the area from (0, 0) to (tile_width, tile_width / 2) is
        // considered when checking which tiles are on-screen, and the exact
        // position of pixels is disregarded. The numbers in the figure above
        // denote the first to the last drawn cells in the vertical direction.
        // Therefore, we can see that,
        //
        // cols = 1 + divide_round_up(sx, w)
        // rows = 1 + divide_round_up(sy, h)
        // ||
        // \/
        const int columns = divide_round_up( size.x * 2, tile_width ) + 1;
        const int rows = divide_round_up( size.y * 4, tile_width ) + 1;
        return point( columns, rows );
    } else {
        // Only the area from (0, 0) to (tile_width, tile_height) is considered
        // when checking which tiles are on-screen.
        const int columns = divide_round_up( size.x, tile_width );
        const int rows = divide_round_up( size.y, tile_height );
        return point( columns, rows );
    }
}

half_open_rectangle<point> cata_tiles::get_window_any_tile_range(
    const point &size, const int z ) const
{
    // (following comments in get_window_base_tile_counts)
    //
    // Based on the maximum tile extent, these ensure that any tile that can be
    // possibly on screen is included in the range.
    if( is_isometric() ) {
        // z-level below => negative z_height => smaller row_beg and row_end
        const int z_height = zlevel_height * z;
        const int col_beg = divide_round_down( ( tile_width - max_tile_extent.p_max.x ) * 2,
                                               tile_width );
        const int col_end = divide_round_up( ( size.x - max_tile_extent.p_min.x ) * 2,
                                             tile_width ) + 1;
        const int row_beg = divide_round_down( ( z_height + tile_height
                                               - max_tile_extent.p_max.y ) * 4,
                                               tile_width );
        // The difference between the default sprite height `tile_height` and
        // base tile height `tile_width / 2` should be added to the tile extent
        // here. Because `player_to_screen` shifts the sprite up by this extra
        // height, it means this extra height should be subtracted from the
        // minimum extent.
        // (sy + zh - miny + th - tw / 2) / (tw / 4) + 1 =>
        const int row_end = divide_round_up( ( size.y + z_height
                                               - max_tile_extent.p_min.y + tile_height ) * 4,
                                             tile_width ) - 1;
        return { { col_beg, row_beg }, { col_end, row_end } };
    } else {
        const int col_beg = divide_round_down( tile_width - max_tile_extent.p_max.x, tile_width );
        const int col_end = divide_round_up( size.x - max_tile_extent.p_min.x, tile_width );
        const int row_beg = divide_round_down( tile_height - max_tile_extent.p_max.y, tile_height );
        const int row_end = divide_round_up( size.y - max_tile_extent.p_min.y, tile_height );
        return { { col_beg, row_beg }, { col_end, row_end } };
    }
}

half_open_rectangle<point> cata_tiles::get_window_full_base_tile_range( const point &size ) const
{
    if( is_isometric() ) {
        // (following comments in get_window_base_tile_counts)
        //
        // Only the area from (0, 0) to (tile_width, tile_width / 2) is
        // considered when checking which tiles are fully on-screen, and the
        // exact position of pixels is disregarded. In the figure above, as
        // opposed to 1-6, only 2-5 are fully shown on-screen. Therefore, we can
        // see that,
        //
        // cols = divide_round_down(sx, w) - 1
        // rows = divide_round_down(sy, h) - 1
        // ||
        // \/
        const int columns = divide_round_down( size.x * 2, tile_width ) - 1;
        const int rows = divide_round_down( size.y * 4, tile_width ) - 1;
        //NOLINTNEXTLINE(cata-use-named-point-constants): the name would be confusing here (screen 'NSWE' are not map NSWE)
        return { { 1, 1 }, { columns + 1, rows + 1 } };
    } else {
        // Only the area from (0, 0) to (tile_width, tile_height) is considered
        // when checking which tiles are fully on-screen.
        const int columns = divide_round_down( size.x, tile_width );
        const int rows = divide_round_down( size.y, tile_height );
        return { point_zero, { columns, rows } };
    }
}

std::optional<point> cata_tiles::tile_to_player( const point &colrow ) const
{
    if( is_isometric() ) {
        // (following comments in get_window_base_tile_counts)
        //
        // Based on the screen tile pattern, the player position can be calculated
        // from the column and row numbers as follows
        if( modulo( colrow.y - screentile_height / 2, 2 )
            != modulo( colrow.x - screentile_width / 2, 2 ) ) {
            return std::nullopt;
        }
        return o + point {
            divide_round_down( colrow.x - colrow.y - screentile_width / 2
                               + screentile_height / 2, 2 ),
            divide_round_down( colrow.y + colrow.x - screentile_height / 2
                               - screentile_width / 2, 2 ),
        };
    } else {
        return colrow + o;
    }
}

point cata_tiles::player_to_tile( const point &pos ) const
{
    // (calculate the screen position according to cata_tiles::tile_to_player)
    if( is_isometric() ) {
        // (division rounded down):
        //
        // pos.x = ( col - row - sx / 2 + sy / 2 ) / 2 + o.x;
        // pos.y = ( row + col - sy / 2 - sx / 2 ) / 2 + o.y;
        // ( col - sx / 2 ) % 2 = ( row - sy / 2 ) % 2
        // ||
        // \/
        const int col = pos.y + pos.x + screentile_width / 2 - o.y - o.x;
        const int row = pos.y - pos.x + screentile_height / 2 - o.y + o.x;
        return { col, row };
    } else {
        return pos - o;
    }
}

point cata_tiles::player_to_screen( const point &pos ) const
{
    const point colrow = player_to_tile( pos );
    if( is_isometric() ) {
        // To ensure the first fully drawn basic tile (col or row = 1, according
        // to the definition in get_window_full_base_tile_range) starts at 0,
        return op + point {
            // left = ( col - 1 ) * ( tw / 2.0 ) =>
            divide_round_down( ( colrow.x - 1 ) * tile_width, 2 ),
            // top = ( row - 1 ) * ( tw / 4.0 ) - th + tw / 2.0 =>
            // (shifted so that sprites with default height end at the basic height
            // of `tile_width / 4`)
            divide_round_down( ( colrow.y + 1 ) * tile_width, 4 ) - tile_height,
        };
    } else {
        return op + point { colrow.x * tile_width, colrow.y * tile_height };
    }
}

bool cata_tiles::draw_from_id_string( const std::string &id, const tripoint &pos, int subtile,
                                      int rota,
                                      lit_level ll, bool apply_night_vision_goggles )
{
    int nullint = 0;
    return cata_tiles::draw_from_id_string( id, TILE_CATEGORY::NONE, empty_string, pos, subtile,
                                            rota, ll, apply_night_vision_goggles, nullint, 0 );
}

bool cata_tiles::draw_from_id_string( const std::string &id, TILE_CATEGORY category,
                                      const std::string &subcategory, const tripoint &pos,
                                      int subtile, int rota, lit_level ll,
                                      bool apply_night_vision_goggles )
{
    int nullint = 0;
    return cata_tiles::draw_from_id_string( id, category, subcategory, pos, subtile, rota,
                                            ll, apply_night_vision_goggles, nullint, 0 );
}

bool cata_tiles::draw_from_id_string( const std::string &id, const tripoint &pos, int subtile,
                                      int rota,
                                      lit_level ll, bool apply_night_vision_goggles,
                                      int &height_3d )
{
    return cata_tiles::draw_from_id_string( id, TILE_CATEGORY::NONE, empty_string, pos, subtile,
                                            rota, ll, apply_night_vision_goggles, height_3d, 0 );
}

bool cata_tiles::draw_from_id_string( const std::string &id, TILE_CATEGORY category,
                                      const std::string &subcategory, const tripoint &pos,
                                      int subtile, int rota, lit_level ll,
                                      bool apply_night_vision_goggles, int &height_3d, int intensity )
{
    return cata_tiles::draw_from_id_string_internal( id, category, subcategory, pos, subtile, rota,
            ll, -1, apply_night_vision_goggles, height_3d, intensity, "", point() );
}

bool cata_tiles::draw_from_id_string( const std::string &id, TILE_CATEGORY category,
                                      const std::string &subcategory, const tripoint &pos,
                                      int subtile, int rota, lit_level ll,
                                      bool apply_night_vision_goggles, int &height_3d )
{
    return cata_tiles::draw_from_id_string_internal( id, category, subcategory, pos, subtile, rota,
            ll, -1, apply_night_vision_goggles, height_3d, 0, "", point() );
}

bool cata_tiles::draw_from_id_string( const std::string &id, TILE_CATEGORY category,
                                      const std::string &subcategory, const tripoint &pos,
                                      int subtile, int rota, lit_level ll,
                                      bool apply_night_vision_goggles, int &height_3d,
                                      int intensity_level, const std::string &variant )
{
    return cata_tiles::draw_from_id_string_internal( id, category, subcategory, pos, subtile, rota,
            ll, -1, apply_night_vision_goggles, height_3d, intensity_level,
            variant, point() );
}

bool cata_tiles::draw_from_id_string( const std::string &id, TILE_CATEGORY category,
                                      const std::string &subcategory, const tripoint &pos,
                                      int subtile, int rota, lit_level ll,
                                      bool apply_night_vision_goggles, int &height_3d,
                                      int intensity_level, const std::string &variant,
                                      const point &offset )
{
    return cata_tiles::draw_from_id_string_internal( id, category, subcategory, pos, subtile, rota,
            ll, -1, apply_night_vision_goggles, height_3d, intensity_level,
            variant, offset );
}
bool cata_tiles::draw_from_id_string_internal( const std::string &id, const tripoint &pos,
        int subtile,
        int rota,
        lit_level ll, int retract, bool apply_night_vision_goggles, int &height_3d )
{
    return cata_tiles::draw_from_id_string_internal( id, TILE_CATEGORY::NONE, empty_string, pos,
            subtile,
            rota, ll, retract, apply_night_vision_goggles, height_3d, 0, "", point() );
}

std::optional<tile_lookup_res>
cata_tiles::find_tile_with_season( const std::string &id ) const
{
    const season_type season = season_of_year( calendar::turn );
    return tileset_ptr->find_tile_type_by_season( id, season );
}

template<typename T>
std::optional<tile_lookup_res>
cata_tiles::find_tile_looks_like_by_string_id( const std::string_view id, TILE_CATEGORY category,
        const int looks_like_jumps_limit ) const
{
    const string_id<T> s_id( id );
    if( !s_id.is_valid() ) {
        return std::nullopt;
    }
    const T &obj = s_id.obj();
    return find_tile_looks_like( obj.looks_like, category, "", looks_like_jumps_limit - 1 );
}

std::optional<tile_lookup_res>
cata_tiles::find_tile_looks_like( const std::string &id, TILE_CATEGORY category,
                                  const std::string &variant,
                                  const int looks_like_jumps_limit ) const
{
    if( id.empty() || looks_like_jumps_limit <= 0 ) {
        return std::nullopt;
    }

    /*
    *  Note on memory management:
    *  This method must returns pointers to the objects (std::string *id  and tile_type * tile)
    *  that are valid when this method returns. Ideally they should have the lifetime
    *  that is equal or exceeds lifetime of `this` or `this::tileset_ptr`.
    *  For example, `id` argument may have shorter lifetime and thus should not be returned!
    *  The result of `find_tile_with_season` is OK to be returned, because it's guaranteed to
    *  return pointers to the keys and values that are stored inside the `tileset_ptr`.
    */
    // Try the variant first
    if( !variant.empty() ) {
        if( category != TILE_CATEGORY::VEHICLE_PART ) {
            if( auto ret = find_tile_with_season( id + "_var_" + variant ) ) {
                return ret; // with variant
            }
        } else {
            std::string_view variant_chunk = variant;
            while( !variant_chunk.empty() ) {
                if( auto ret = find_tile_with_season( id + "_" + std::string( variant_chunk ) ) ) {
                    return ret; // with variant, but vehicle parts have weird variant suffixes
                }
                const size_t next_start = variant_chunk.rfind( '_' );
                if( next_start != std::string::npos ) {
                    variant_chunk = variant_chunk.substr( 0, next_start );
                } else {
                    variant_chunk = variant_chunk.substr( 0, 0 );
                }
            }
        }
    }
    if( auto ret = find_tile_with_season( id ) ) {
        return ret; // no variant
    }

    // Then do looks_like
    switch( category ) {
        case TILE_CATEGORY::FURNITURE:
            return find_tile_looks_like_by_string_id<furn_t>( id, category,
                    looks_like_jumps_limit );
        case TILE_CATEGORY::TERRAIN:
            return find_tile_looks_like_by_string_id<ter_t>( id, category, looks_like_jumps_limit );
        case TILE_CATEGORY::FIELD:
            return find_tile_looks_like_by_string_id<field_type>( id, category,
                    looks_like_jumps_limit );
        case TILE_CATEGORY::MONSTER:
            return find_tile_looks_like_by_string_id<mtype>( id, category, looks_like_jumps_limit );
        case TILE_CATEGORY::OVERMAP_TERRAIN: {
            std::optional<tile_lookup_res> ret;
            const oter_type_str_id type_tmp( id );
            if( !type_tmp.is_valid() ) {
                return ret;
            }

            int jump_limit = looks_like_jumps_limit;
            for( const std::string &looks_like : type_tmp.obj().looks_like ) {

                ret = find_tile_looks_like( looks_like, category, "", jump_limit - 1 );
                if( ret.has_value() ) {
                    return ret;
                }

                jump_limit--;
                if( jump_limit <= 0 ) {
                    return ret;
                }
            }

            return ret;
        }

        case TILE_CATEGORY::VEHICLE_PART: {
            const int lljl = looks_like_jumps_limit - 1;
            // vehicle parts start with vp_ for their tiles, but not their IDs
            const vpart_id vpid( id.substr( 3 ) );
            if( !vpid.is_valid() ) {
                return std::nullopt;
            }
            const std::string &looks_like = vpid->looks_like;
            if( looks_like.empty() ) {
                return std::nullopt;
            }
            if( auto ret = find_tile_looks_like( "vp_" + looks_like, category, variant, lljl ) ) {
                return ret;
            }
            if( auto ret = find_tile_looks_like( looks_like, category, variant, lljl ) ) {
                return ret;
            }
            return std::nullopt;
        }

        case TILE_CATEGORY::ITEM: {
            if( !item::type_is_defined( itype_id( id ) ) ) {
                if( string_starts_with( id, "corpse_" ) ) {
                    return find_tile_looks_like(
                               "corpse", category, "", looks_like_jumps_limit - 1
                           );
                }
                return std::nullopt;
            }
            const itype *new_it = item::find_type( itype_id( id ) );
            return find_tile_looks_like( new_it->looks_like.str(), category, "",
                                         looks_like_jumps_limit - 1 );
        }

        default:
            return std::nullopt;
    }
}

bool cata_tiles::find_overlay_looks_like( const bool male, const std::string &overlay,
        const std::string &variant, std::string &draw_id )
{
    bool exists = false;

    std::string looks_like;
    std::string over_type;

    if( string_starts_with( overlay, "worn_" ) ) {
        looks_like = overlay.substr( 5 );
        over_type = "worn_";
    } else if( string_starts_with( overlay, "wielded_" ) ) {
        looks_like = overlay.substr( 8 );
        over_type = "wielded_";
    } else {
        looks_like = overlay;
    }

    // Try to draw variants, then fall back to drawing the base
    // We can potentially do this twice for a variant of an active mutation
    for( int i = 0; i < 2; ++i ) {
        draw_id.clear();
        str_append( draw_id,
                    ( male ? "overlay_male_" : "overlay_female_" ), over_type, looks_like, "_var_",
                    variant );
        if( tileset_ptr->find_tile_type( draw_id ) ) {
            return true;
        }
        draw_id.clear();
        str_append( draw_id, "overlay_", over_type, looks_like, "_var_", variant );
        if( tileset_ptr->find_tile_type( draw_id ) ) {
            return true;
        }
        if( string_starts_with( looks_like, "mutation_active_" ) ) {
            looks_like = "mutation_" + looks_like.substr( 16 );
            continue;
        }
        break;
    }
    for( int cnt = 0; cnt < 10 && !looks_like.empty(); cnt++ ) {
        draw_id.clear();
        str_append( draw_id,
                    ( male ? "overlay_male_" : "overlay_female_" ), over_type, looks_like );
        if( tileset_ptr->find_tile_type( draw_id ) ) {
            exists = true;
            break;
        }
        draw_id.clear();
        str_append( draw_id, "overlay_", over_type, looks_like );
        if( tileset_ptr->find_tile_type( draw_id ) ) {
            exists = true;
            break;
        }
        if( string_starts_with( looks_like, "mutation_active_" ) ) {
            looks_like = "mutation_" + looks_like.substr( 16 );
            continue;
        }
        if( !item::type_is_defined( itype_id( looks_like ) ) ) {
            break;
        }
        const itype *new_it = item::find_type( itype_id( looks_like ) );
        looks_like = new_it->looks_like.str();
    }
    return exists;
}

bool cata_tiles::draw_from_id_string_internal( const std::string &id, TILE_CATEGORY category,
        const std::string &subcategory, const tripoint &pos,
        int subtile, int rota, lit_level ll, int retract,
        bool apply_night_vision_goggles, int &height_3d,
        int intensity_level, const std::string &variant,
        const point &offset )
{
    bool nv_color_active = apply_night_vision_goggles && get_option<bool>( "NV_GREEN_TOGGLE" );
    // If the ID string does not produce a drawable tile
    // it will revert to the "unknown" tile.
    // The "unknown" tile is one that is highly visible so you kinda can't miss it :D

    const tile_type *tt = nullptr;
    std::optional<tile_lookup_res> res;

    // translate from player-relative to screen relative tile position
    const point screen_pos = player_to_screen( pos.xy() );

    if( retract < 0 && ( prevent_occlusion_transp || prevent_occlusion_retract ) ) {
        if( prevent_occlusion == 0 ) {
            retract = 0;
        } else if( prevent_occlusion == 1 ) {
            retract = 100;
        } else {
            const float distance = o.distance( pos.xy() );
            const float d_min = prevent_occlusion_min_dist > 0.0 ? prevent_occlusion_min_dist :
                                tileset_ptr->get_prevent_occlusion_min_dist();
            const float d_max = prevent_occlusion_max_dist > 0.0 ? prevent_occlusion_max_dist :
                                tileset_ptr->get_prevent_occlusion_max_dist();

            const float d_range = d_max - d_min;
            const float d_slope = d_range <= 0.0f ? 100.0 : 1.0 / d_range;

            retract = static_cast<int>( 100.0 * ( 1.0 - clamp( ( distance - d_min ) * d_slope, 0.0f, 1.0f ) ) );
        }

        if( prevent_occlusion_transp && retract > 0 ) {
            res = find_tile_looks_like( id + "_transparent", category, variant );
            if( res ) {
                tt = &res -> tile();
            }
        }
    }

    // check if there is an available intensity tile and if there is use that instead of the basic tile
    // this is only relevant for fields
    if( intensity_level > 0 ) {
        res = find_tile_looks_like( id + "_int" + std::to_string( intensity_level ), category, variant );
        if( res ) {
            tt = &res -> tile();
        }
    }
    // if a tile with intensity hasn't already been found then fall back to a base tile
    if( !res ) {
        res = find_tile_looks_like( id, category, variant );
        if( res ) {
            tt = &res -> tile();
        }
    }

    map &here = get_map();
    const std::string &found_id = res ? res->id() : id;

    if( !tt ) {
        // Use fog overlay as fallback for transparent terrain
        if( category == TILE_CATEGORY::TERRAIN && !here.dont_draw_lower_floor( pos ) ) {
            draw_zlevel_overlay( pos, ll, height_3d );

            // t_open_air is plentiful at high z-levels
            // Skipping the rest of the fallback code will significantly improve performance
            if( id == "t_open_air" ) {
                return true;
            }
        }

        uint32_t sym = UNKNOWN_UNICODE;
        nc_color col = c_white;
        if( category == TILE_CATEGORY::FURNITURE ) {
            const furn_str_id fid( found_id );
            if( fid.is_valid() ) {
                const furn_t &f = fid.obj();
                sym = f.symbol();
                col = f.color();
            }
        } else if( category == TILE_CATEGORY::TERRAIN ) {
            const ter_str_id tid( found_id );
            if( tid.is_valid() ) {
                const ter_t &t = tid.obj();
                sym = t.symbol();
                col = t.color();
            }
        } else if( category == TILE_CATEGORY::MONSTER ) {
            const mtype_id mid( found_id );
            if( mid.is_valid() ) {
                const mtype &mt = mid.obj();
                sym = UTF8_getch( mt.sym );
                col = mt.color;
            }
        } else if( category == TILE_CATEGORY::VEHICLE_PART ) {
            const vpart_id vpid( string_starts_with( found_id, "vp_" ) ? found_id.substr( 3 ) : found_id );
            if( vpid.is_valid() ) {
                const vpart_info &vpi = *vpid;
                if( vpi.variants.count( variant ) ) {
                    const vpart_variant &vv = vpi.variants.at( variant );
                    if( subtile == open_ ) {
                        sym = '\'';
                    } else {
                        sym = vv.get_symbol_curses( 90_degrees * rota, subtile == broken );
                    }
                    col = vpi.color;
                    subtile = -1;
                    rota = 0;
                } else if( ll != lit_level::MEMORIZED ) {
                    debugmsg( "invalid variant '%s' on vpart_id '%s'", variant, vpid.str() );
                }
            } else if( ll != lit_level::MEMORIZED ) {
                debugmsg( "invalid vpart_id '%s'", vpid.str() );
            }
        } else if( category == TILE_CATEGORY::FIELD ) {
            const field_type_id fid = field_type_id( found_id );
            sym = fid->get_intensity_level().symbol;
            col = fid->get_intensity_level().color;
        } else if( category == TILE_CATEGORY::TRAP ) {
            const trap_str_id tmp( found_id );
            if( tmp.is_valid() ) {
                const trap &t = tmp.obj();
                sym = t.sym;
                col = t.color;
            }
        } else if( category == TILE_CATEGORY::ITEM ) {
            item tmp;
            if( string_starts_with( found_id, "corpse_" ) ) {
                tmp = item( itype_corpse, calendar::turn_zero );
            } else {
                tmp = item( found_id, calendar::turn_zero );
            }
            if( !variant.empty() ) {
                tmp.set_itype_variant( variant );
            } else {
                tmp.clear_itype_variant();
            }
            sym = static_cast<uint8_t>( tmp.symbol().empty() ? ' ' : tmp.symbol().front() );
            col = tmp.color();
        } else if( category == TILE_CATEGORY::OVERMAP_TERRAIN ) {
            const oter_type_str_id tmp( id );
            if( tmp.is_valid() ) {
                if( !tmp->is_linear() ) {
                    sym = tmp->get_rotated( static_cast<om_direction::type>( rota ) )->get_uint32_symbol();
                } else {
                    sym = tmp->symbol;
                }
                col = tmp->color;
            }
        } else if( category == TILE_CATEGORY::OVERMAP_NOTE ) {
            sym = static_cast<uint8_t>( id[5] );
            col = color_from_string( id.substr( 7, id.length() - 1 ) );
        }
        // Special cases for walls
        switch( sym ) {
            case LINE_XOXO:
            case LINE_XOXO_UNICODE:
                sym = LINE_XOXO_C;
                break;
            case LINE_OXOX:
            case LINE_OXOX_UNICODE:
                sym = LINE_OXOX_C;
                break;
            case LINE_XXOO:
            case LINE_XXOO_UNICODE:
                sym = LINE_XXOO_C;
                break;
            case LINE_OXXO:
            case LINE_OXXO_UNICODE:
                sym = LINE_OXXO_C;
                break;
            case LINE_OOXX:
            case LINE_OOXX_UNICODE:
                sym = LINE_OOXX_C;
                break;
            case LINE_XOOX:
            case LINE_XOOX_UNICODE:
                sym = LINE_XOOX_C;
                break;
            case LINE_XXXO:
            case LINE_XXXO_UNICODE:
                sym = LINE_XXXO_C;
                break;
            case LINE_XXOX:
            case LINE_XXOX_UNICODE:
                sym = LINE_XXOX_C;
                break;
            case LINE_XOXX:
            case LINE_XOXX_UNICODE:
                sym = LINE_XOXX_C;
                break;
            case LINE_OXXX:
            case LINE_OXXX_UNICODE:
                sym = LINE_OXXX_C;
                break;
            case LINE_XXXX:
            case LINE_XXXX_UNICODE:
                sym = LINE_XXXX_C;
                break;
            default:
                // sym goes unchanged
                break;
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
                return draw_from_id_string_internal( generic_id, pos, subtile, rota,
                                                     ll, retract, nv_color_active, height_3d );
            }
            // Try again without color this time (using default color).
            generic_id = get_ascii_tile_id( sym, -1, -1 );
            if( tileset_ptr->find_tile_type( generic_id ) ) {
                return draw_from_id_string_internal( generic_id, pos, subtile, rota,
                                                     ll, retract, nv_color_active, height_3d );
            }
        }
    }

    // if id is not found, try to find a tile for the category+subcategory combination
    const std::string &category_id = TILE_CATEGORY_IDS[static_cast<size_t>( category )];
    if( !tt ) {
        if( !category_id.empty() && !subcategory.empty() ) {
            tt = tileset_ptr->find_tile_type( "unknown_" + category_id + "_" + subcategory );
        }
    }

    // if at this point we have no tile, try just the category
    if( !tt ) {
        if( !category_id.empty() ) {
            tt = tileset_ptr->find_tile_type( "unknown_" + category_id );
        }
    }

    // if we still have no tile, we're out of luck, fall back to unknown
    if( !tt ) {
        tt = tileset_ptr->find_tile_type( "unknown" );
    }

    //  this really shouldn't happen, but the tileset creator might have forgotten to define
    // an unknown tile
    if( !tt ) {
        return false;
    }

    const tile_type &display_tile = *tt;
    // check to see if the display_tile is multitile, and if so if it has the key related to
    // subtile
    if( subtile != -1 && display_tile.multitile ) {
        const auto &display_subtiles = display_tile.available_subtiles;
        const auto end = std::end( display_subtiles );
        if( std::find( begin( display_subtiles ), end, multitile_keys[subtile] ) != end ) {
            // append subtile name to tile and re-find display_tile
            return draw_from_id_string_internal(
                       found_id + "_" + multitile_keys[subtile], category, subcategory, pos, -1, rota, ll,
                       retract, nv_color_active, height_3d, 0, "", point() );
        }
    }

    auto simple_point_hash = []( const auto & p ) {
        return p.x + p.y * 65536;
    };

    // seed the PRNG to get a reproducible random int
    // TODO: faster solution here
    unsigned int seed = 0;
    creature_tracker &creatures = get_creature_tracker();
    // TODO: determine ways other than category to differentiate more types of sprites
    switch( category ) {
        case TILE_CATEGORY::TERRAIN:
        case TILE_CATEGORY::FIELD:
        case TILE_CATEGORY::LIGHTING:
            // stationary map tiles, seed based on map coordinates
            seed = simple_point_hash( here.getabs( pos ) );
            break;
        case TILE_CATEGORY::VEHICLE_PART:
            // vehicle parts, seed based on coordinates within the vehicle
            // TODO: also use some vehicle id, for less predictability
        {
            // new scope for variable declarations
            const auto vp_override = vpart_override.find( pos );
            const bool vp_overridden = vp_override != vpart_override.end();
            if( vp_overridden ) {
                const vpart_id &vp_id = std::get<0>( vp_override->second );
                if( vp_id ) {
                    const point &mount = std::get<4>( vp_override->second );
                    seed = simple_point_hash( mount );
                }
            } else {
                const optional_vpart_position vp = here.veh_at( pos );
                if( vp ) {
                    seed = simple_point_hash( vp->mount() );
                }
            }
        }
        break;
        case TILE_CATEGORY::FURNITURE: {
            // If the furniture is not movable, we'll allow seeding by the position
            // since we won't get the behavior that occurs where the tile constantly
            // changes when the player grabs the furniture and drags it, causing the
            // seed to change.
            const furn_str_id fid( found_id );
            if( fid.is_valid() ) {
                const furn_t &f = fid.obj();
                if( !f.is_movable() ) {
                    seed = simple_point_hash( here.getabs( pos ) );
                }
            }
        }
        break;
        case TILE_CATEGORY::OVERMAP_TERRAIN:
        case TILE_CATEGORY::MAP_EXTRA:
            seed = simple_point_hash( pos );
            break;
        case TILE_CATEGORY::NONE:
            // graffiti
            if( found_id == "graffiti" ) {
                seed = std::hash<std::string> {}( here.graffiti_at( pos ) );
            } else if( string_starts_with( found_id, "graffiti" ) ) {
                seed = simple_point_hash( here.getabs( pos ) );
            }
            break;
        case TILE_CATEGORY::ITEM:
        case TILE_CATEGORY::TRAP:
        case TILE_CATEGORY::BULLET:
        case TILE_CATEGORY::HIT_ENTITY:
        case TILE_CATEGORY::WEATHER:
            // TODO: come up with ways to make random sprites consistent for these types
            break;
        case TILE_CATEGORY::MONSTER:
            // FIXME: add persistent id to Creature type, instead of using monster pointer address
            if( monster_override.find( pos ) == monster_override.end() ) {
                seed = reinterpret_cast<uintptr_t>( creatures.creature_at<monster>( pos ) );
            }
            break;
        default:
            // player
            if( string_starts_with( found_id, "player_" ) ) {
                seed = std::hash<std::string> {}( get_player_character().name );
                break;
            }
            // NPC
            if( string_starts_with( found_id, "npc_" ) ) {
                if( npc *const guy = creatures.creature_at<npc>( pos ) ) {
                    seed = guy->getID().get_value();
                    break;
                }
            }
    }

    // make sure we aren't going to rotate the tile if it shouldn't be rotated
    if( !display_tile.rotates && !( category == TILE_CATEGORY::NONE )
        && !( category == TILE_CATEGORY::MONSTER ) ) {
        rota = 0;
    }

    unsigned int loc_rand = 0;
    // only bother mixing up a hash/random value if the tile has some sprites to randomly pick
    // between
    if( display_tile.fg.size() > 1 || display_tile.bg.size() > 1 ) {
        static const auto rot32 = []( const unsigned int x, const int k ) {
            return ( x << k ) | ( x >> ( 32 - k ) );
        };
        // use a fair mix function to turn the "random" seed into a random int
        // taken from public domain code at http://burtleburtle.net/bob/c/lookup3.c 2015/12/11
        unsigned int a = seed;
        unsigned int b = -seed;
        unsigned int c = seed * seed;
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

        // idle tile animations:
        if( display_tile.animated ) {
            // idle animations run during the user's turn, and the animation speed
            // needs to be defined by the tileset to look good, so we use system clock:
            std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
            auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>( now );
            std::chrono::milliseconds value = now_ms.time_since_epoch();
            // aiming roughly at the standard 60 frames per second:
            int animation_frame = value.count() / 17;
            // offset by log_rand so that everything does not blink at the same time:
            animation_frame += loc_rand;
            int frames_in_loop = display_tile.fg.get_weight();
            if( frames_in_loop == 1 ) {
                frames_in_loop = display_tile.bg.get_weight();
            }
            // loc_rand is actually the weighed index of the selected tile, and
            // for animations the "weight" is the number of frames to show the tile for:
            loc_rand = animation_frame % frames_in_loop;
        }
    }

    if( ! prevent_occlusion_retract ) {
        retract = 0;
    }

    //draw it!
    draw_tile_at( display_tile, screen_pos, loc_rand, rota, ll,
                  nv_color_active, retract, height_3d, offset );

    return true;
}

bool cata_tiles::draw_sprite_at(
    const tile_type &tile, const weighted_int_list<std::vector<int>> &svlist,
    const point &p, unsigned int loc_rand, bool rota_fg, int rota, lit_level ll,
    bool apply_night_vision_goggles, int retract, int &height_3d, const point &offset )
{
    const std::vector<int> *picked = svlist.pick( loc_rand );
    if( !picked ) {
        return true;
    }
    const std::vector<int> &spritelist = *picked;
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

    const int sprite_index = spritelist[sprite_num];
    const texture *sprite_tex = tileset_ptr->get_tile( sprite_index );

    //use night vision colors when in use
    //then use low light tile if available
    if( ll == lit_level::MEMORIZED ) {
        if( const texture *ptr = tileset_ptr->get_memory_tile( sprite_index ) ) {
            sprite_tex = ptr;
        }
    } else if( apply_night_vision_goggles ) {
        if( ll != lit_level::LOW ) {
            if( const texture *ptr = tileset_ptr->get_overexposed_tile( sprite_index ) ) {
                sprite_tex = ptr;
            }
        } else {
            if( const texture *ptr = tileset_ptr->get_night_tile( sprite_index ) ) {
                sprite_tex = ptr;
            }
        }
    } else if( ll == lit_level::LOW ) {
        if( const texture *ptr = tileset_ptr->get_shadow_tile( sprite_index ) ) {
            sprite_tex = ptr;
        }
    }

    int width = 0;
    int height = 0;
    std::tie( width, height ) = sprite_tex->dimension();

    const point &tile_offset = retract <= 0
                               ? tile.offset
                               : ( retract >= 100
                                   ? tile.offset_retracted
                                   : tile.offset
                                   + ( ( tile.offset_retracted - tile.offset ) * retract ) / 100
                                 );
    SDL_Rect destination;
    // Using divide_round_down because the offset might be negative.
    destination.x = p.x + divide_round_down( ( tile_offset.x + offset.x ) * tile_width,
                    tileset_ptr->get_tile_width() );
    destination.y = p.y + divide_round_down( ( tile_offset.y + offset.y - height_3d ) * tile_width,
                    tileset_ptr->get_tile_width() );
    destination.w = width * tile_width * tile.pixelscale / tileset_ptr->get_tile_width();
    destination.h = height * tile_height * tile.pixelscale / tileset_ptr->get_tile_height();

    if( rotate_sprite ) {
        if( rota == -1 ) {
            // flip horizontally
            ret = sprite_tex->render_copy_ex(
                      renderer, &destination, 0, nullptr,
                      static_cast<SDL_RendererFlip>( SDL_FLIP_HORIZONTAL ) );
        } else {
            switch( rota % 4 ) {
                default:
                case 0:
                    // unrotated (and 180, with just two sprites)
                    ret = sprite_tex->render_copy_ex( renderer, &destination, 0, nullptr,
                                                      SDL_FLIP_NONE );
                    break;
                case 1:
                    // 90 degrees (and 270, with just two sprites)
#if defined(_WIN32) && defined(CROSS_LINUX)
                    // For an unknown reason, additional offset is required in direct3d mode
                    // for cross-compilation from Linux to Windows
                    if( direct3d_mode ) {
                        destination.y -= 1;
                    }
#endif
                    if( !is_isometric() ) {
                        // never rotate isometric tiles
                        ret = sprite_tex->render_copy_ex( renderer, &destination, -90, nullptr,
                                                          SDL_FLIP_NONE );
                    } else {
                        ret = sprite_tex->render_copy_ex( renderer, &destination, 0, nullptr,
                                                          SDL_FLIP_NONE );
                    }
                    break;
                case 2:
                    // 180 degrees, implemented with flips instead of rotation
                    if( !is_isometric() ) {
                        // never flip isometric tiles vertically
                        ret = sprite_tex->render_copy_ex(
                                  renderer, &destination, 0, nullptr,
                                  static_cast<SDL_RendererFlip>( SDL_FLIP_HORIZONTAL | SDL_FLIP_VERTICAL ) );
                    } else {
                        ret = sprite_tex->render_copy_ex( renderer, &destination, 0, nullptr,
                                                          SDL_FLIP_NONE );
                    }
                    break;
                case 3:
                    // 270 degrees
#if defined(_WIN32) && defined(CROSS_LINUX)
                    // For an unknown reason, additional offset is required in direct3d mode
                    // for cross-compilation from Linux to Windows
                    if( direct3d_mode ) {
                        destination.x -= 1;
                    }
#endif
                    if( !is_isometric() ) {
                        // never rotate isometric tiles
                        ret = sprite_tex->render_copy_ex( renderer, &destination, 90, nullptr,
                                                          SDL_FLIP_NONE );
                    } else {
                        ret = sprite_tex->render_copy_ex( renderer, &destination, 0, nullptr,
                                                          SDL_FLIP_NONE );
                    }
                    break;
            }
        }
    } else {
        // don't rotate, same as case 0 above
        ret = sprite_tex->render_copy_ex( renderer, &destination, 0, nullptr, SDL_FLIP_NONE );
    }

    printErrorIf( ret != 0, "SDL_RenderCopyEx() failed" );
    // this reference passes all the way back up the call chain back to
    // cata_tiles::draw() draw_points[row][col].com.height_3d
    // where we are accumulating the height of every sprite stacked up in a tile
    height_3d += tile.height_3d;
    return true;
}

bool cata_tiles::draw_tile_at(
    const tile_type &tile, const point &p, unsigned int loc_rand, int rota,
    lit_level ll, bool apply_night_vision_goggles, int retract, int &height_3d,
    const point &offset )
{
    int fake_int = height_3d;
    draw_sprite_at( tile, tile.bg, p, loc_rand, /*fg:*/ false, rota, ll,
                    apply_night_vision_goggles, retract, fake_int, offset );
    draw_sprite_at( tile, tile.fg, p, loc_rand, /*fg:*/ true, rota, ll,
                    apply_night_vision_goggles, retract, height_3d, offset );
    return true;
}

bool cata_tiles::would_apply_vision_effects( const visibility_type visibility ) const
{
    return visibility != visibility_type::CLEAR;
}

bool cata_tiles::apply_vision_effects( const tripoint &pos,
                                       const visibility_type visibility,
                                       int &height_3d )
{
    if( !would_apply_vision_effects( visibility ) ) {
        return false;
    }
    std::string light_name;
    switch( visibility ) {
        case visibility_type::HIDDEN:
            light_name = "lighting_hidden";
            break;
        case visibility_type::LIT:
            light_name = "lighting_lowlight_light";
            break;
        case visibility_type::BOOMER:
            light_name = "lighting_boomered_light";
            break;
        case visibility_type::BOOMER_DARK:
            light_name = "lighting_boomered_dark";
            break;
        case visibility_type::DARK:
            light_name = "lighting_lowlight_dark";
            break;
        case visibility_type::CLEAR:
            // should never happen
            break;
    }

    // lighting is never rotated, though, could possibly add in random rotation?
    draw_from_id_string( light_name, TILE_CATEGORY::LIGHTING, empty_string, pos, 0, 0,
                         lit_level::LIT, false, height_3d );

    return true;
}

bool cata_tiles::has_memory_at( const tripoint_abs_ms &p ) const
{
    const memorized_tile &mt = get_avatar().get_memorized_tile( p );
    return !mt.get_ter_id().empty() || !mt.get_dec_id().empty();
}

const memorized_tile &cata_tiles::get_terrain_memory_at( const tripoint_abs_ms &p ) const
{
    const memorized_tile &mt = get_avatar().get_memorized_tile( p );
    if( !mt.get_ter_id().empty() ) {
        return mt;
    }
    return mm_submap::default_tile;
}

const memorized_tile &cata_tiles::get_furniture_memory_at( const tripoint_abs_ms &p ) const
{
    const memorized_tile &mt = get_avatar().get_memorized_tile( p );
    if( string_starts_with( mt.get_dec_id(), "f_" ) ) {
        return mt;
    }
    return mm_submap::default_tile;
}

const memorized_tile &cata_tiles::get_trap_memory_at( const tripoint_abs_ms &p ) const
{
    const memorized_tile &mt = get_avatar().get_memorized_tile( p );
    if( string_starts_with( mt.get_dec_id(), "tr_" ) ) {
        return mt;
    }
    return mm_submap::default_tile;
}

const memorized_tile &cata_tiles::get_vpart_memory_at( const tripoint_abs_ms &p ) const
{
    const memorized_tile &mt = get_avatar().get_memorized_tile( p );
    if( string_starts_with( mt.get_dec_id(), "vp_" ) ) {
        return mt;
    }
    return mm_submap::default_tile;
}

void cata_tiles::draw_square_below( const point &p, const nc_color &col, const int sizefactor )
{
    const SDL_Color sdlcol = curses_color_to_SDL( col );
    SDL_Rect sdlrect;
    const point screen = player_to_screen( p );
    if( is_isometric() ) {
        // See comments in get_window_base_tile_counts for an explanation of tile width and height
        // tw / sizefactor * 3.0 / 4.0
        sdlrect.w = ( tile_width * 3 ) / ( sizefactor * 4 );
        if( tile_width % 2 != sdlrect.w % 2 ) {
            sdlrect.w++;
        }
        // (tw / 2.0) / sizefactor * 3.0 / 4.0
        sdlrect.h = ( tile_width * 3 ) / ( sizefactor * 8 );
        if( tile_width / 2 % 2 != sdlrect.h % 2 ) {
            sdlrect.h++;
        }
        // scrx + (tw - rectw) / 2.0
        sdlrect.x = screen.x + divide_round_down( tile_width - sdlrect.w, 2 );
        // scry + th - tw / 2.0 + (tw / 2.0 - recth) / 2.0
        // Adding th and subtracting tw/2 here to cancel out the shifting in player_to_screen.
        sdlrect.y = screen.y + tile_height + divide_round_down( -tile_width - sdlrect.h * 2, 4 );
    } else {
        sdlrect.w = tile_width / sizefactor;
        if( tile_width % 2 != sdlrect.w % 2 ) {
            sdlrect.w++;
        }
        sdlrect.h = tile_height / sizefactor;
        if( tile_height % 2 != sdlrect.h % 2 ) {
            sdlrect.h++;
        }
        sdlrect.x = screen.x + divide_round_down( tile_width - sdlrect.w, 2 );
        sdlrect.y = screen.y + divide_round_down( tile_height - sdlrect.h, 2 );
    }
    geometry->rect( renderer, sdlrect, sdlcol );
}

bool cata_tiles::draw_terrain_below( const tripoint &p, const lit_level, int &,
                                     const std::array<bool, 5> &invisible, const bool memorize_only )
{
    if( memorize_only ) {
        return false;
    }

    map &here = get_map();
    const auto low_override = draw_below_override.find( p );
    const bool low_overridden = low_override != draw_below_override.end();
    if( low_overridden ? !low_override->second :
        ( invisible[0] || here.dont_draw_lower_floor( p ) ) ) {
        return false;
    }

    tripoint pbelow = tripoint( p.xy(), p.z - 1 );
    nc_color col = c_dark_gray;

    const ter_t &curr_ter = here.ter( pbelow ).obj();
    const furn_t &curr_furn = here.furn( pbelow ).obj();
    int part_below;
    int sizefactor = 2;
    if( curr_furn.has_flag( ter_furn_flag::TFLAG_SEEN_FROM_ABOVE ) || curr_furn.movecost < 0 ) {
        col = curr_furn.color();
    } else if( const vehicle *veh = here.veh_at_internal( pbelow, part_below ) ) {
        const int roof = veh->roof_at_part( part_below );
        const auto vpobst = vpart_position( const_cast<vehicle &>( *veh ),
                                            part_below ).obstacle_at_part();
        col = ( roof >= 0 || vpobst ) ? c_light_gray : c_magenta;
        sizefactor = ( roof >= 0 || vpobst ) ? 4 : 2;
    } else if( curr_ter.has_flag( ter_furn_flag::TFLAG_SEEN_FROM_ABOVE ) ||
               curr_ter.has_flag( ter_furn_flag::TFLAG_NO_FLOOR ) ||
               curr_ter.has_flag( ter_furn_flag::TFLAG_NO_FLOOR_WATER ) ||
               curr_ter.movecost == 0 ) {
        col = curr_ter.color();
    } else {
        sizefactor = 4;
        col = curr_ter.color();
    }

    draw_square_below( pbelow.xy(), col, sizefactor );
    return true;
}

bool cata_tiles::draw_terrain( const tripoint &p, const lit_level ll, int &height_3d,
                               const std::array<bool, 5> &invisible, const bool memorize_only )
{
    map &here = get_map();
    const auto override = terrain_override.find( p );
    const bool overridden = override != terrain_override.end();
    bool neighborhood_overridden = overridden;
    if( !neighborhood_overridden ) {
        for( const point &dir : neighborhood ) {
            if( terrain_override.find( p + dir ) != terrain_override.end() ) {
                neighborhood_overridden = true;
                break;
            }
        }
    }
    // first memorize the actual terrain
    const ter_id &t = here.ter( p );
    const std::string &tname = t.id().str();
    // Legacy mode does not draw fog sprites
    if( fov_3d_z_range == 0 && tname == "t_open_air" ) {
        return false;
    }
    if( t && !invisible[0] ) {
        int subtile = 0;
        int rotation = 0;
        const std::bitset<NUM_TERCONN> &connect_group = t.obj().connect_to_groups;
        const std::bitset<NUM_TERCONN> &rotate_group = t.obj().rotate_to_groups;

        if( connect_group.any() ) {
            get_connect_values( p, subtile, rotation, connect_group, rotate_group, {} );
            // re-memorize previously seen terrain in case new connections have been seen
            here.memory_cache_ter_set_dirty( p, true );
        } else {
            get_terrain_orientation( p, rotation, subtile, {}, invisible, rotate_group );
            // do something to get other terrain orientation values
        }
        if( here.memory_cache_ter_is_dirty( p ) ) {
            get_avatar().memorize_terrain( here.getglobal( p ), tname, subtile, rotation );
        }
        // draw the actual terrain if there's no override
        if( !neighborhood_overridden ) {
            return memorize_only
                   ? false
                   : draw_from_id_string( tname, TILE_CATEGORY::TERRAIN, empty_string, p, subtile,
                                          rotation, ll, nv_goggles_activated, height_3d );
        }
    }
    if( invisible[0] ? overridden : neighborhood_overridden ) {
        // and then draw the override terrain
        const ter_id &t2 = overridden ? override->second : t;
        if( t2 ) {
            // both the current and neighboring overrides may change the appearance
            // of the tile, so always re-calculate it.
            int subtile = 0;
            int rotation = 0;
            const std::bitset<NUM_TERCONN> &connect_group = t2.obj().connect_to_groups;
            const std::bitset<NUM_TERCONN> &rotate_group = t2.obj().rotate_to_groups;

            if( connect_group.any() ) {
                get_connect_values( p, subtile, rotation, connect_group, rotate_group, terrain_override );
            } else {
                get_terrain_orientation( p, rotation, subtile, terrain_override, invisible, rotate_group );
            }
            const std::string &tname = t2.id().str();
            // tile overrides are never memorized
            // tile overrides are always shown with full visibility
            const lit_level lit = overridden ? lit_level::LIT : ll;
            const bool nv = overridden ? false : nv_goggles_activated;
            return memorize_only
                   ? false
                   : draw_from_id_string( tname, TILE_CATEGORY::TERRAIN, empty_string, p, subtile,
                                          rotation, lit, nv, height_3d );
        }
    } else if( invisible[0] ) {
        // try drawing memory if invisible and not overridden
        const memorized_tile &mt = get_terrain_memory_at( here.getglobal( p ) );
        if( !mt.get_ter_id().empty() ) {
            return memorize_only
                   ? false
                   : draw_from_id_string(
                       mt.get_ter_id(), TILE_CATEGORY::TERRAIN, empty_string, p, mt.get_ter_subtile(),
                       mt.get_ter_rotation(), lit_level::MEMORIZED, nv_goggles_activated, height_3d );
        }
    }
    return false;
}

bool cata_tiles::draw_furniture( const tripoint &p, const lit_level ll, int &height_3d,
                                 const std::array<bool, 5> &invisible, const bool memorize_only )
{
    avatar &you = get_avatar();
    const auto override = furniture_override.find( p );
    const bool overridden = override != furniture_override.end();
    bool neighborhood_overridden = overridden;
    if( !neighborhood_overridden ) {
        for( const point &dir : neighborhood ) {
            if( furniture_override.find( p + dir ) != furniture_override.end() ) {
                neighborhood_overridden = true;
                break;
            }
        }
    }
    map &here = get_map();
    // first memorize the actual furniture
    const furn_id &f = here.furn( p );
    if( f && !invisible[0] ) {
        const std::array<int, 4> neighborhood = {
            static_cast<int>( here.furn( p + point_south ) ),
            static_cast<int>( here.furn( p + point_east ) ),
            static_cast<int>( here.furn( p + point_west ) ),
            static_cast<int>( here.furn( p + point_north ) )
        };
        int subtile = 0;
        int rotation = 0;
        const std::bitset<NUM_TERCONN> &connect_group = f.obj().connect_to_groups;
        const std::bitset<NUM_TERCONN> &rotate_group = f.obj().rotate_to_groups;

        if( connect_group.any() ) {
            get_furn_connect_values( p, subtile, rotation, connect_group, rotate_group, {} );
        } else {
            get_tile_values_with_ter( p, f.to_i(), neighborhood, subtile, rotation, rotate_group );
        }
        const std::string &fname = f.id().str();
        if( !( you.get_grab_type() == object_type::FURNITURE
               && p == you.pos() + you.grab_point )
            && here.memory_cache_dec_is_dirty( p ) ) {
            you.memorize_decoration( here.getglobal( p ), fname, subtile, rotation );
        }
        // draw the actual furniture if there's no override
        if( !neighborhood_overridden ) {
            return memorize_only
                   ? false
                   : draw_from_id_string( fname, TILE_CATEGORY::FURNITURE, empty_string, p, subtile,
                                          rotation, ll, nv_goggles_activated, height_3d );
        }
    }
    if( invisible[0] ? overridden : neighborhood_overridden ) {
        // and then draw the override furniture
        const furn_id &f2 = overridden ? override->second : f;
        if( f2 ) {
            // both the current and neighboring overrides may change the appearance
            // of the tile, so always re-calculate it.
            const auto furn = [&]( const tripoint & q, const bool invis ) -> furn_id {
                const auto it = furniture_override.find( q );
                return it != furniture_override.end() ? it->second :
                ( !overridden || !invis ) ? here.furn( q ) : f_null;
            };
            const std::array<int, 4> neighborhood = {
                static_cast<int>( furn( p + point_south, invisible[1] ) ),
                static_cast<int>( furn( p + point_east, invisible[2] ) ),
                static_cast<int>( furn( p + point_west, invisible[3] ) ),
                static_cast<int>( furn( p + point_north, invisible[4] ) )
            };
            int subtile = 0;
            int rotation = 0;
            const std::bitset<NUM_TERCONN> &connect_group = f.obj().connect_to_groups;
            const std::bitset<NUM_TERCONN> &rotate_group = f.obj().rotate_to_groups;

            if( connect_group.any() ) {
                get_furn_connect_values( p, subtile, rotation, connect_group, rotate_group, {} );
            } else {
                get_tile_values_with_ter( p, f.to_i(), neighborhood, subtile, rotation, rotate_group );
            }
            get_tile_values_with_ter( p, f2.to_i(), neighborhood, subtile, rotation, 0 );
            const std::string &fname = f2.id().str();
            // tile overrides are never memorized
            // tile overrides are always shown with full visibility
            const lit_level lit = overridden ? lit_level::LIT : ll;
            const bool nv = overridden ? false : nv_goggles_activated;
            return memorize_only
                   ? false
                   : draw_from_id_string( fname, TILE_CATEGORY::FURNITURE, empty_string, p, subtile,
                                          rotation, lit, nv, height_3d );
        }
    } else if( invisible[0] ) {
        // try drawing memory if invisible and not overridden
        const memorized_tile &mt = get_furniture_memory_at( here.getglobal( p ) );
        if( !mt.get_dec_id().empty() ) {
            return memorize_only
                   ? false
                   : draw_from_id_string(
                       mt.get_dec_id(), TILE_CATEGORY::FURNITURE, empty_string, p, mt.get_dec_subtile(),
                       mt.get_dec_rotation(), lit_level::MEMORIZED, nv_goggles_activated, height_3d );
        }
    }
    return false;
}

bool cata_tiles::draw_trap( const tripoint &p, const lit_level ll, int &height_3d,
                            const std::array<bool, 5> &invisible, const bool memorize_only )
{
    const auto override = trap_override.find( p );
    const bool overridden = override != trap_override.end();
    bool neighborhood_overridden = overridden;
    if( !neighborhood_overridden ) {
        for( const point &dir : neighborhood ) {
            if( trap_override.find( p + dir ) != trap_override.end() ) {
                neighborhood_overridden = true;
                break;
            }
        }
    }

    avatar &you = get_avatar();
    map &here = get_map();
    // first memorize the actual trap
    const trap &tr = here.tr_at( p );
    if( !tr.is_null() && !invisible[0] && tr.can_see( p, you ) ) {
        const std::array<int, 4> neighborhood = {
            static_cast<int>( here.tr_at( p + point_south ).loadid ),
            static_cast<int>( here.tr_at( p + point_east ).loadid ),
            static_cast<int>( here.tr_at( p + point_west ).loadid ),
            static_cast<int>( here.tr_at( p + point_north ).loadid )
        };
        int subtile = 0;
        int rotation = 0;
        get_tile_values( tr.loadid.to_i(), neighborhood, subtile, rotation, 0 );
        const std::string trname = tr.loadid.id().str();
        if( here.memory_cache_dec_is_dirty( p ) ) {
            you.memorize_decoration( here.getglobal( p ), trname, subtile, rotation );
        }
        // draw the actual trap if there's no override
        if( !neighborhood_overridden ) {
            return memorize_only
                   ? false
                   : draw_from_id_string( trname, TILE_CATEGORY::TRAP, empty_string, p, subtile,
                                          rotation, ll, nv_goggles_activated, height_3d );
        }
    }
    if( overridden || ( !invisible[0] && neighborhood_overridden &&
                        tr.can_see( p, you ) ) ) {
        // and then draw the override trap
        const trap_id &tr2 = overridden ? override->second : tr.loadid;
        if( tr2 ) {
            // both the current and neighboring overrides may change the appearance
            // of the tile, so always re-calculate it.
            const auto tr_at = [&]( const tripoint & q, const bool invis ) -> trap_id {
                const auto it = trap_override.find( q );
                return it != trap_override.end() ? it->second :
                ( !overridden || !invis ) ? here.tr_at( q ).loadid : tr_null;
            };
            const std::array<int, 4> neighborhood = {
                static_cast<int>( tr_at( p + point_south, invisible[1] ) ),
                static_cast<int>( tr_at( p + point_east, invisible[2] ) ),
                static_cast<int>( tr_at( p + point_west, invisible[3] ) ),
                static_cast<int>( tr_at( p + point_north, invisible[4] ) )
            };
            int subtile = 0;
            int rotation = 0;
            get_tile_values( tr2.to_i(), neighborhood, subtile, rotation, 0 );
            const std::string &trname = tr2.id().str();
            // tile overrides are never memorized
            // tile overrides are always shown with full visibility
            const lit_level lit = overridden ? lit_level::LIT : ll;
            const bool nv = overridden ? false : nv_goggles_activated;
            return memorize_only
                   ? false
                   : draw_from_id_string( trname, TILE_CATEGORY::TRAP, empty_string, p, subtile,
                                          rotation, lit, nv, height_3d );
        }
    } else if( invisible[0] ) {
        // try drawing memory if invisible and not overridden
        const memorized_tile &mt = get_trap_memory_at( here.getglobal( p ) );
        if( !mt.get_dec_id().empty() ) {
            return memorize_only
                   ? false
                   : draw_from_id_string(
                       mt.get_dec_id(), TILE_CATEGORY::TRAP, empty_string, p, mt.get_dec_subtile(), mt.get_dec_rotation(),
                       lit_level::MEMORIZED, nv_goggles_activated, height_3d );
        }
    }
    return false;
}

bool cata_tiles::draw_part_con( const tripoint &p, const lit_level ll, int &height_3d,
                                const std::array<bool, 5> &invisible, const bool memorize_only )
{
    map &here = get_map();
    // FIXME: fix tripoint type
    if( here.partial_con_at( tripoint_bub_ms( p ) ) != nullptr && !invisible[0] ) {
        avatar &you = get_avatar();
        std::string const &trname = tr_unfinished_construction.str();;
        if( here.memory_cache_dec_is_dirty( p ) ) {
            you.memorize_decoration( here.getglobal( p ), trname, 0, 0 );
        }
        return memorize_only
               ? false
               : draw_from_id_string( trname, TILE_CATEGORY::TRAP, empty_string, p, 0,
                                      0, ll, nv_goggles_activated, height_3d );
    }
    return false;
}

bool cata_tiles::draw_graffiti( const tripoint &p, const lit_level ll, int &height_3d,
                                const std::array<bool, 5> &invisible, const bool memorize_only )
{
    if( memorize_only ) {
        return false;
    }

    map &here = get_map();
    const auto override = graffiti_override.find( p );
    const bool overridden = override != graffiti_override.end();
    if( overridden ? !override->second : ( invisible[0] || !here.has_graffiti_at( p ) ) ) {
        return false;
    }
    const lit_level lit = overridden ? lit_level::LIT : ll;
    const int rotation = here.passable( p ) ? 1 : 0;
    const std::string tile = "graffiti_" +
                             to_upper_case( string_replace( remove_punctuations( here.graffiti_at( p ) ), " ",
                                            "_" ) ).substr( 0, 32 );
    return draw_from_id_string( tileset_ptr->find_tile_type( tile ) ? tile : "graffiti",
                                TILE_CATEGORY::NONE, empty_string, p, 0, rotation, lit, false, height_3d );
}

bool cata_tiles::draw_field_or_item( const tripoint &p, const lit_level ll, int &height_3d,
                                     const std::array<bool, 5> &invisible, const bool memorize_only )
{
    if( memorize_only ) {
        return false;
    }

    const auto fld_override = field_override.find( p );
    const bool fld_overridden = fld_override != field_override.end();
    map &here = get_map();
    const field_type_id &fld = fld_overridden ?
                               fld_override->second : here.field_at( p ).displayed_field_type();
    bool ret_draw_field = false;
    bool ret_draw_items = false;
    // go through each field and draw it
    if( !fld_overridden ) {
        const maptile &tile = here.maptile_at( p );

        for( const std::pair<const field_type_id, field_entry> &fd_pr : here.field_at( p ) ) {
            const field_type_id &fld = fd_pr.first;
            if( !invisible[0] && fld.obj().display_field ) {
                const lit_level lit = ll;
                const bool nv = nv_goggles_activated;

                auto has_field = [&]( field_type_id fld, const tripoint & q, const bool invis ) -> field_type_id {
                    // go through the fields and see if they are equal
                    field_type_id found = fd_null;
                    for( std::pair<const field_type_id, field_entry> &this_fld : here.field_at( q ) )
                    {
                        if( this_fld.first == fld ) {
                            found = fld;
                        }
                    }
                    const auto it = field_override.find( q );
                    return it != field_override.end() ? it->second :
                           ( !fld_overridden || !invis ) ?  found : fd_null;
                };
                // for rotation information
                const std::array<int, 4> neighborhood = { {
                        static_cast<int>( has_field( fld, p + point_south, invisible[1] ) ),
                        static_cast<int>( has_field( fld, p + point_east, invisible[2] ) ),
                        static_cast<int>( has_field( fld, p + point_west, invisible[3] ) ),
                        static_cast<int>( has_field( fld, p + point_north, invisible[4] ) )
                    }
                };

                int subtile = 0;
                int rotation = 0;
                get_tile_values( fld.to_i(), neighborhood, subtile, rotation, 0 );

                //get field intensity
                int intensity = fd_pr.second.get_field_intensity();

                bool has_drawn = false;

                // start by drawing the layering data if available
                // start for checking if layer data is available for the furniture
                auto itt = tileset_ptr->field_layer_data.find( tile.get_furn_t().id.str() );
                if( itt != tileset_ptr->field_layer_data.end() ) {

                    // the furniture has layer info
                    // go through all the layer variants
                    for( const layer_variant &layer_var : itt->second ) {
                        if( fld.id().str() == layer_var.id ) {

                            // get the sprite to draw
                            // roll should be based on the maptile seed to keep visuals consistent
                            int roll = 1;
                            std::string sprite_to_draw;
                            for( const auto &sprite_list : layer_var.sprite ) {
                                roll = roll - sprite_list.second;
                                if( roll < 0 ) {
                                    sprite_to_draw = sprite_list.first;
                                    break;
                                }
                            }

                            // if we have found info on the item go through and draw its stuff
                            ret_draw_field = draw_from_id_string( sprite_to_draw, TILE_CATEGORY::FIELD, empty_string, p,
                                                                  subtile, rotation, lit, nv, height_3d, intensity, "", layer_var.offset );
                            has_drawn = true;
                            break;
                        }
                    }
                } else {
                    // check if the terrain has data
                    auto itt = tileset_ptr->field_layer_data.find( tile.get_ter_t().id.str() );
                    if( itt != tileset_ptr->field_layer_data.end() ) {
                        // the furniture has layer info
                        // go through all the layer variants
                        for( const layer_variant &layer_var : itt->second ) {
                            if( fld.id().str() == layer_var.id ) {

                                // get the sprite to draw
                                // roll should be based on the maptile seed to keep visuals consistent
                                int roll = 1;
                                std::string sprite_to_draw;
                                for( const auto &sprite_list : layer_var.sprite ) {
                                    roll = roll - sprite_list.second;
                                    if( roll < 0 ) {
                                        sprite_to_draw = sprite_list.first;
                                        break;
                                    }
                                }

                                // if we have found info on the item go through and draw its stuff
                                ret_draw_field = draw_from_id_string( sprite_to_draw, TILE_CATEGORY::FIELD, empty_string, p,
                                                                      subtile, rotation, lit, nv, height_3d, intensity, "", layer_var.offset );
                                has_drawn = true;
                                break;
                            }
                        }
                    }
                }

                // draw the default sprite
                if( !has_drawn ) {
                    ret_draw_field = draw_from_id_string( fld.id().str(), TILE_CATEGORY::FIELD, empty_string,
                                                          p, subtile, rotation, lit, nv, height_3d, intensity );
                }

            }
        }
    } else {
        // draw the override
        const field_type_id &fld = fld_override->second;
        if( fld.obj().display_field ) {
            const lit_level lit = lit_level::LIT;
            const bool nv = false;

            auto field_at = [&]( const tripoint & q, const bool invis ) -> field_type_id {
                const auto it = field_override.find( q );
                return it != field_override.end() ? it->second :
                ( !fld_overridden || !invis ) ? here.field_at( q ).displayed_field_type() : fd_null;
            };
            // for rotation information
            const std::array<int, 4> neighborhood = {
                static_cast<int>( field_at( p + point_south, invisible[1] ) ),
                static_cast<int>( field_at( p + point_east, invisible[2] ) ),
                static_cast<int>( field_at( p + point_west, invisible[3] ) ),
                static_cast<int>( field_at( p + point_north, invisible[4] ) )
            };

            int subtile = 0;
            int rotation = 0;
            get_tile_values( fld.to_i(), neighborhood, subtile, rotation, 0 );

            //get field intensity
            int intensity = fld_overridden ? 0 : here.field_at( p ).displayed_intensity();
            ret_draw_field = draw_from_id_string( fld.id().str(), TILE_CATEGORY::FIELD, empty_string,
                                                  p, subtile, rotation, lit, nv, height_3d, intensity );
        }
    }

    if( fld.obj().display_items ) {
        const auto it_override = item_override.find( p );
        const bool it_overridden = it_override != item_override.end();

        itype_id it_id;
        mtype_id mon_id;
        std::string variant;
        bool hilite = false;
        bool drawtop = true;
        const itype *it_type;
        const maptile &tile = here.maptile_at( p );

        if( !invisible[0] ) {
            // start by drawing the layering data if available
            // start for checking if layer data is available for the furniture
            auto itt = tileset_ptr->item_layer_data.find( tile.get_furn_t().id.str() );
            if( itt != tileset_ptr->item_layer_data.end() ) {

                // the furniture has layer info
                // go through all the layer variants
                for( const layer_variant &layer_var : itt->second ) {
                    for( const item &i : tile.get_items() ) {
                        if( i.typeId().str() == layer_var.id ) {
                            // if an item matches draw it and break
                            const std::string layer_it_category = i.typeId()->get_item_type_string();
                            const lit_level layer_lit = ll;
                            const bool layer_nv = nv_goggles_activated;

                            // get the sprite to draw
                            // roll should be based on the maptile seed to keep visuals consistent
                            int roll = i.seed % layer_var.total_weight;
                            std::string sprite_to_draw;
                            for( const auto &sprite_list : layer_var.sprite ) {
                                roll = roll - sprite_list.second;
                                if( roll < 0 ) {
                                    sprite_to_draw = sprite_list.first;
                                    break;
                                }
                            }

                            if( i.has_itype_variant() ) {
                                variant = i.itype_variant().id;
                            }
                            // if we have found info on the item go through and draw its stuff
                            draw_from_id_string( sprite_to_draw, TILE_CATEGORY::ITEM, layer_it_category, p, 0,
                                                 0, layer_lit, layer_nv, height_3d, 0, variant, layer_var.offset );

                            // if the top item is already being layered don't draw it later
                            if( i.typeId() == tile.get_uppermost_item().typeId() ) {
                                drawtop = false;
                            }

                            break;
                        }
                    }

                }

            } else {
                // check if the terrain has data
                auto itt = tileset_ptr->item_layer_data.find( tile.get_ter_t().id.str() );
                if( itt != tileset_ptr->item_layer_data.end() ) {

                    // the furniture has layer info
                    // go through all the layer variants
                    for( const layer_variant &layer_var : itt->second ) {
                        for( const item &i : tile.get_items() ) {
                            if( i.typeId().str() == layer_var.id ) {
                                // if an item matches draw it and break
                                const std::string layer_it_category = i.typeId()->get_item_type_string();
                                const lit_level layer_lit = ll;
                                const bool layer_nv = nv_goggles_activated;

                                // get the sprite to draw
                                // roll should be based on the maptile seed to keep visuals consistent
                                int roll = i.seed % layer_var.total_weight;
                                std::string sprite_to_draw;
                                for( const auto &sprite_list : layer_var.sprite ) {
                                    roll = roll - sprite_list.second;
                                    if( roll < 0 ) {
                                        sprite_to_draw = sprite_list.first;
                                        break;
                                    }
                                }

                                if( i.has_itype_variant() ) {
                                    variant = i.itype_variant().id;
                                }
                                // if we have found info on the item go through and draw its stuff
                                draw_from_id_string( sprite_to_draw, TILE_CATEGORY::ITEM, layer_it_category, p, 0,
                                                     0, layer_lit, layer_nv, height_3d, 0, variant, layer_var.offset );

                                // if the top item is already being layered don't draw it later
                                if( i.typeId() == tile.get_uppermost_item().typeId() ) {
                                    drawtop = false;
                                }

                                break;
                            }
                        }

                    }

                }
            }
        }

        variant.clear();
        if( drawtop || it_overridden ) {
            if( it_overridden ) {
                it_id = std::get<0>( it_override->second );
                mon_id = std::get<1>( it_override->second );
                hilite = std::get<2>( it_override->second );
                it_type = item::find_type( it_id );
            } else if( !invisible[0] && here.sees_some_items( p, get_player_character() ) ) {
                const item &itm = tile.get_uppermost_item();
                if( itm.has_itype_variant() ) {
                    variant = itm.itype_variant().id;
                }
                const mtype *const mon = itm.get_mtype();
                it_id = itm.typeId();
                mon_id = mon ? mon->id : mtype_id::NULL_ID();
                hilite = tile.get_item_count() > 1;
                it_type = itm.type;
            } else {
                it_type = nullptr;
            }
            if( it_type && !it_id.is_null() ) {

                const std::string disp_id = it_id == itype_corpse && mon_id ?
                                            "corpse_" + mon_id.str() : it_id.str();
                const std::string it_category = it_type->get_item_type_string();
                const lit_level lit = it_overridden ? lit_level::LIT : ll;
                const bool nv = it_overridden ? false : nv_goggles_activated;

                ret_draw_items = draw_from_id_string( disp_id, TILE_CATEGORY::ITEM, it_category, p, 0,
                                                      0, lit, nv, height_3d, 0, variant );
                if( ret_draw_items && hilite ) {
                    draw_item_highlight( p, height_3d );
                }
            }
        }
        // we may still need to draw the highlight
        else if( tile.get_item_count() > 1 && here.sees_some_items( p, get_player_character() ) ) {
            draw_item_highlight( p, height_3d );
        }
    }
    return ret_draw_field && ret_draw_items;
}

bool cata_tiles::draw_vpart_below( const tripoint &p, const lit_level /*ll*/, int &/*height_3d*/,
                                   const std::array<bool, 5> &invisible, const bool memorize_only )
{
    if( memorize_only ) {
        return false;
    }

    const auto low_override = draw_below_override.find( p );
    const bool low_overridden = low_override != draw_below_override.end();
    if( low_overridden ? !low_override->second : ( invisible[0] ||
            get_map().dont_draw_lower_floor( p ) ) ) {
        return false;
    }
    tripoint pbelow( p.xy(), p.z - 1 );
    int height_3d_below = 0;
    std::array<bool, 5> below_invisible;
    std::fill( below_invisible.begin(), below_invisible.end(), false );
    return draw_vpart_no_roof( pbelow, lit_level::LOW, height_3d_below, below_invisible,
                               memorize_only );
}

bool cata_tiles::draw_vpart_no_roof( const tripoint &p, lit_level ll, int &height_3d,
                                     const std::array<bool, 5> &invisible, const bool memorize_only )
{
    return draw_vpart( p, ll, height_3d, invisible, false, memorize_only );
}

bool cata_tiles::draw_vpart_roof( const tripoint &p, lit_level ll, int &height_3d,
                                  const std::array<bool, 5> &invisible, const bool memorize_only )
{
    return draw_vpart( p, ll, height_3d, invisible, true, memorize_only );
}

bool cata_tiles::draw_vpart( const tripoint &p, lit_level ll, int &height_3d,
                             const std::array<bool, 5> &invisible, bool roof, const bool memorize_only )
{
    const auto override = vpart_override.find( p );
    const bool overridden = override != vpart_override.end();
    map &here = get_map();
    // first memorize the actual vpart
    const optional_vpart_position ovp = here.veh_at( p );
    if( ovp && !invisible[0] ) {
        const vehicle &veh = ovp->vehicle();
        const vpart_display vd = veh.get_display_of_tile( ovp->mount() );
        if( !vd.id.is_null() ) {
            const int subtile = vd.is_open ? open_ : vd.is_broken ? broken : 0;
            const int rotation = angle_to_dir4( 270_degrees - veh.face.dir() );
            avatar &you = get_avatar();
            if( !veh.forward_velocity() && !veh.player_in_control( you )
                && !( you.get_grab_type() == object_type::VEHICLE
                      && veh.get_points().count( you.pos() + you.grab_point ) )
                && here.memory_cache_dec_is_dirty( p ) ) {
                you.memorize_decoration( here.getglobal( p ), vd.get_tileset_id(), subtile, rotation );
            }
            if( !overridden ) {
                int height_3d_temp = height_3d;
                const bool ret = memorize_only
                                 ? false
                                 : draw_from_id_string( "vp_" + vd.id.str(), TILE_CATEGORY::VEHICLE_PART,
                                                        empty_string, p, subtile, rotation, ll,
                                                        nv_goggles_activated, height_3d_temp, 0, vd.variant.id );
                if( ret && vd.has_cargo ) {
                    draw_item_highlight( p, height_3d_temp );
                }
                // Do not increment height_3d for roof vparts
                if( !roof ) {
                    height_3d = height_3d_temp;
                }
                return ret;
            }
        }
    }
    if( overridden ) {
        // and then draw the override vpart
        const vpart_id &vp2 = std::get<0>( override->second );
        if( vp2 ) {
            const char part_mod = std::get<1>( override->second );
            const int subtile = part_mod == 1 ? open_ : part_mod == 2 ? broken : 0;
            const int rotation = angle_to_dir4( 270_degrees - std::get<2>( override->second ) );
            const int draw_highlight = std::get<3>( override->second );
            const std::string vpname = "vp_" + vp2.str();
            // tile overrides are never memorized
            // tile overrides are always shown with full visibility
            int height_3d_temp = height_3d;
            const bool ret = memorize_only
                             ? false
                             : draw_from_id_string( vpname, TILE_CATEGORY::VEHICLE_PART, empty_string, p, subtile,
                                                    rotation, lit_level::LIT, false, height_3d_temp );
            if( ret && draw_highlight ) {
                draw_item_highlight( p, height_3d_temp );
            }
            if( !roof ) {
                height_3d = height_3d_temp;
            }
            return ret;
        }
    } else if( !roof && invisible[0] ) {
        // try drawing memory if invisible and not overridden
        const memorized_tile &t = get_vpart_memory_at( here.getglobal( p ) );
        std::string_view tid = t.get_dec_id();
        if( !tid.empty() ) {
            int height_3d_temp = height_3d;
            std::string_view tvar;
            const size_t variant_separator = tid.find( vehicles::variant_separator );
            if( variant_separator != std::string::npos ) {
                tvar = tid.substr( variant_separator + 1 );
                tid = tid.substr( 0, variant_separator );
            }
            return memorize_only
                   ? false
                   : draw_from_id_string(
                       std::string( tid ), TILE_CATEGORY::VEHICLE_PART, empty_string, p, t.get_dec_subtile(),
                       t.get_dec_rotation(), lit_level::MEMORIZED, nv_goggles_activated, height_3d_temp, 0,
                       std::string( tvar ) );
        }
    }
    return false;
}

bool cata_tiles::draw_critter_at_below( const tripoint &p, const lit_level, int &,
                                        const std::array<bool, 5> &invisible, const bool memorize_only )
{
    if( memorize_only ) {
        return false;
    }

    // Check if we even need to draw below. If not, bail.
    const auto low_override = draw_below_override.find( p );
    const bool low_overridden = low_override != draw_below_override.end();
    if( low_overridden ? !low_override->second : ( invisible[0] ||
            get_map().dont_draw_lower_floor( p ) ) ) {
        return false;
    }

    tripoint pbelow( p.xy(), p.z - 1 );

    // Get the critter at the location below. If there isn't one,
    // we can bail.
    const Creature *critter = get_creature_tracker().creature_at( pbelow, true );
    if( critter == nullptr ) {
        return false;
    }

    Character &you = get_player_character();
    // Check if the player can actually see the critter. We don't care if
    // it's via infrared or not, just whether or not they're seen. If not,
    // we can bail.
    if( !you.sees( *critter ) &&
        !( you.sees_with_infrared( *critter ) || you.sees_with_specials( *critter ) ) ) {
        return false;
    }

    draw_square_below( pbelow.xy(), c_red, 2 );

    return true;
}

bool cata_tiles::draw_critter_at( const tripoint &p, lit_level ll, int &height_3d,
                                  const std::array<bool, 5> &invisible, const bool memorize_only )
{
    if( memorize_only ) {
        return false;
    }

    bool result;
    bool is_player;
    bool sees_player;
    Creature::Attitude attitude;
    Character &you = get_player_character();
    const Creature *pcritter = get_creature_tracker().creature_at( p, true );
    const bool always_visible = pcritter && pcritter->has_flag( mon_flag_ALWAYS_VISIBLE );
    const auto override = monster_override.find( p );
    if( override != monster_override.end() ) {
        const mtype_id id = std::get<0>( override->second );
        if( !id ) {
            return false;
        }
        is_player = false;
        sees_player = false;
        attitude = std::get<3>( override->second );
        const std::string &chosen_id = id.str();
        const std::string &ent_subcategory = id.obj().species.empty() ?
                                             empty_string : id.obj().species.begin()->str();
        result = draw_from_id_string( chosen_id, TILE_CATEGORY::MONSTER, ent_subcategory, p,
                                      corner, 0, lit_level::LIT, false, height_3d );
    } else if( !invisible[0] || always_visible ) {
        if( pcritter == nullptr ) {
            return false;
        }
        const Creature &critter = *pcritter;

        if( !you.sees( critter ) ) {
            if( you.sees_with_infrared( critter ) ||
                you.sees_with_specials( critter ) ) {
                return draw_from_id_string( "infrared_creature", TILE_CATEGORY::NONE, empty_string,
                                            p, 0, 0, lit_level::LIT, false, height_3d );
            }
            return false;
        }
        result = false;
        sees_player = false;
        is_player = false;
        attitude = Creature::Attitude::ANY;
        const monster *m = dynamic_cast<const monster *>( &critter );
        if( m != nullptr ) {
            const TILE_CATEGORY ent_category = TILE_CATEGORY::MONSTER;
            std::string ent_subcategory = empty_string;
            if( !m->type->species.empty() ) {
                ent_subcategory = m->type->species.begin()->str();
            }
            const int subtile = corner;
            // depending on the toggle flip sprite left or right
            int rot_facing = -2;
            if( m->facing == FacingDirection::RIGHT ) {
                rot_facing = 0;
            } else if( m->facing == FacingDirection::LEFT ) {
                rot_facing = -1;
            }
            if( rot_facing >= -1 ) {
                const mtype_id ent_name = m->type->id;
                std::string chosen_id = ent_name.str();
                if( m->has_effect( effect_ridden ) ) {
                    int pl_under_height = 6;
                    if( m->mounted_player ) {
                        draw_entity_with_overlays( *m->mounted_player, p, ll, pl_under_height );
                    }
                    const std::string prefix = "rid_";
                    std::string copy_id = chosen_id;
                    const std::string ridden_id = copy_id.insert( 0, prefix );
                    const tile_type *tt = tileset_ptr->find_tile_type( ridden_id );
                    if( tt ) {
                        chosen_id = ridden_id;
                    }
                }
                result = draw_from_id_string( chosen_id, ent_category, ent_subcategory, p,
                                              subtile, rot_facing, ll, false, height_3d );
                sees_player = m->sees( you );
                attitude = m->attitude_to( you );
            }
        }
        const Character *pl = dynamic_cast<const Character *>( &critter );
        if( pl != nullptr ) {
            draw_entity_with_overlays( *pl, p, ll, height_3d );
            result = true;
            if( pl->is_avatar() ) {
                is_player = true;
            } else {
                sees_player = pl->sees( you );
                attitude = pl->attitude_to( you );
            }
        }
    } else {
        // invisible
        if( pcritter == nullptr ) {
            return false;
        }
        // scope_is_blocking is true if player is aiming and aim FOV limits obscure that position
        const bool scope_is_blocking = you.is_avatar() && you.as_avatar()->cant_see( p );
        const bool sees_with_infrared = !scope_is_blocking && you.sees_with_infrared( *pcritter );
        if( sees_with_infrared || you.sees_with_specials( *pcritter ) ) {
            // try drawing infrared creature if invisible and not overridden
            // return directly without drawing overlay
            return draw_from_id_string( "infrared_creature", TILE_CATEGORY::NONE, empty_string, p,
                                        0, 0, lit_level::LIT, false, height_3d );
        } else {
            return false;
        }
    }

    if( result && !is_player ) {
        std::string draw_id = "overlay_" + Creature::attitude_raw_string( attitude );
        if( sees_player && !you.has_trait( trait_INATTENTIVE ) ) {
            draw_id += "_sees_player";
        }
        if( tileset_ptr->find_tile_type( draw_id ) ) {
            draw_from_id_string( draw_id, TILE_CATEGORY::NONE, empty_string, p, 0, 0,
                                 lit_level::LIT, false, height_3d );
        }
    }
    return result;
}

bool cata_tiles::draw_critter_above( const tripoint &p, lit_level ll, int &height_3d,
                                     const std::array<bool, 5> &invisible )
{
    if( invisible[0] ) {
        return false;
    }

    tripoint scan_p( p.xy(), p.z + 1 );
    map &here = get_map();
    Character &you = get_player_character();
    const Creature *pcritter = nullptr;
    // Search for a creature above
    while( pcritter == nullptr && !here.dont_draw_lower_floor( scan_p ) &&
           scan_p.z - you.pos().z <= fov_3d_z_range ) {
        pcritter = get_creature_tracker().creature_at( scan_p, true );
        scan_p.z++;
    }

    // Abort if no creature found
    if( pcritter == nullptr ) {
        return false;
    }
    const Creature &critter = *pcritter;

    // Draw shadow
    if( draw_from_id_string( "shadow", TILE_CATEGORY::NONE, empty_string, p,
                             0, 0, ll, false, height_3d ) && scan_p.z - 1 > you.pos().z && you.sees( critter ) ) {

        bool is_player = false;
        bool sees_player = false;
        Creature::Attitude attitude = Creature::Attitude::ANY;

        // Get critter status disposition if monster
        const monster *m = dynamic_cast<const monster *>( &critter );
        if( m != nullptr ) {
            sees_player = m->sees( you );
            attitude = m->attitude_to( you );
        }

        // Get critter status disposition if character
        const Character *pl = dynamic_cast<const Character *>( &critter );
        if( pl != nullptr ) {
            if( pl->is_avatar() ) {
                is_player = true;
            } else {
                sees_player = pl->sees( you );
                attitude = pl->attitude_to( you );
            }
        }

        // Draw overlay for shadow owner
        if( !is_player ) {
            std::string draw_id = "overlay_" + Creature::attitude_raw_string( attitude );
            if( sees_player && !you.has_trait( trait_INATTENTIVE ) ) {
                draw_id += "_sees_player";
            }
            if( tileset_ptr->find_tile_type( draw_id ) ) {
                draw_from_id_string( draw_id, TILE_CATEGORY::NONE, empty_string, p, 0, 0,
                                     lit_level::LIT, false, height_3d );
            }
        }
        return true;
    } else {
        return false;
    }
}

bool cata_tiles::draw_zone_mark( const tripoint &p, lit_level ll, int &height_3d,
                                 const std::array<bool, 5> &invisible, const bool memorize_only )
{
    if( memorize_only ) {
        return false;
    }

    if( invisible[0] ) {
        return false;
    }

    if( !g->is_zones_manager_open() ) {
        return false;
    }

    const zone_manager &mgr = zone_manager::get_manager();
    const tripoint_abs_ms abs = get_map().getglobal( p );
    const zone_data *zone = mgr.get_bottom_zone( abs );

    if( zone && zone->has_options() ) {
        const mark_option *option = dynamic_cast<const mark_option *>( &zone->get_options() );

        if( option && !option->get_mark().empty() ) {
            return draw_from_id_string( option->get_mark(), TILE_CATEGORY::NONE, empty_string, p,
                                        0, 0, ll, nv_goggles_activated, height_3d );
        }
    }

    return false;
}

bool cata_tiles::draw_zombie_revival_indicators( const tripoint &pos, const lit_level /*ll*/,
        int &height_3d, const std::array<bool, 5> &invisible, const bool memorize_only )
{
    if( memorize_only ) {
        return false;
    }

    map &here = get_map();
    if( tileset_ptr->find_tile_type( ZOMBIE_REVIVAL_INDICATOR ) && !invisible[0] &&
        item_override.find( pos ) == item_override.end() &&
        here.could_see_items( pos, get_player_character() ) ) {
        for( item &i : here.i_at( pos ) ) {
            if( i.can_revive() ) {
                return draw_from_id_string( ZOMBIE_REVIVAL_INDICATOR, TILE_CATEGORY::NONE,
                                            empty_string, pos, 0, 0, lit_level::LIT, false, height_3d );
            }
        }
    }
    return false;
}

void cata_tiles::draw_zlevel_overlay( const tripoint &p, const lit_level ll, int &height_3d )
{
    // Draws zlevel fog using geometry renderer
    // Slower than sprites so only use as fallback when sprite missing
    const point screen = player_to_screen( p.xy() );
    SDL_Rect draw_rect;
    if( is_isometric() ) {
        // See comments in get_window_base_tile_counts for an explanation of tile width
        // and height.
        //
        // Because different tilesets may have different tile shapes, we cannot
        // draw a tile that fits every tileset, so this only acts as a placeholder
        // and the tilesets should make a tile that matches the shape of other
        // sprites.
        draw_rect.x = screen.x;
        // scry
        // + th - tw / 2.0   /* cancel out the shifting in player_to_screen */
        // - height_3d
        // + tw / 8.0        /* shift 1/4 of normal height to avoid overlap among overlays */
        draw_rect.y = screen.y + tile_height - tile_width * 3 / 8 - height_3d;
        draw_rect.w = tile_width;
        // Make the tile half the normal height to avoid overlap among overlays
        draw_rect.h = tile_width / 4;
    } else {
        draw_rect.x = screen.x;
        draw_rect.y = screen.y - height_3d;
        draw_rect.w = tile_width;
        draw_rect.h = tile_height;
    }

    // Overlay color is based on light level
    SDL_Color fog_color = curses_color_to_SDL( c_black );
    if( ll == lit_level::BRIGHT_ONLY || ll == lit_level::BRIGHT || ll == lit_level::LIT ) {
        fog_color = curses_color_to_SDL( c_light_gray );
    } else if( ll == lit_level::LOW ) {
        fog_color = curses_color_to_SDL( c_dark_gray );
    }
    // Setting for fog transparency
    // On isometric tilesets, fog intensity scales with zlevel_height in tile_config.json
    fog_color.a = fog_alpha;

    // Change blend mode for transparency to work
    // Disable after to avoid visual bugs
    SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_BLEND );
    geometry->rect( renderer, draw_rect, fog_color );
    SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_NONE );
}

void cata_tiles::draw_entity_with_overlays( const Character &ch, const tripoint &p, lit_level ll,
        int &height_3d )
{
    std::string ent_name;

    if( ch.is_npc() ) {
        ent_name = ch.male ? "npc_male" : "npc_female";
    } else {
        ent_name = ch.male ? "player_male" : "player_female";
    }
    // first draw the character itself(i guess this means a tileset that
    // takes this seriously needs a naked sprite)
    int prev_height_3d = height_3d;

    // depending on the toggle flip sprite left or right
    if( ch.facing == FacingDirection::RIGHT ) {
        draw_from_id_string( ent_name, TILE_CATEGORY::NONE, "", p, corner, 0, ll, false,
                             height_3d );
    } else if( ch.facing == FacingDirection::LEFT ) {
        draw_from_id_string( ent_name, TILE_CATEGORY::NONE, "", p, corner, -1, ll, false,
                             height_3d );
    }

    // next up, draw all the overlays
    std::vector<std::pair<std::string, std::string>> overlays = ch.get_overlay_ids();
    for( const std::pair<std::string, std::string> &overlay : overlays ) {
        std::string draw_id = overlay.first;
        if( find_overlay_looks_like( ch.male, overlay.first, overlay.second, draw_id ) ) {
            int overlay_height_3d = prev_height_3d;
            if( ch.facing == FacingDirection::RIGHT ) {
                draw_from_id_string( draw_id, TILE_CATEGORY::NONE, "", p, corner, /*rota:*/ 0, ll,
                                     false, overlay_height_3d );
            } else if( ch.facing == FacingDirection::LEFT ) {
                draw_from_id_string( draw_id, TILE_CATEGORY::NONE, "", p, corner, /*rota:*/ -1, ll,
                                     false, overlay_height_3d );
            }
            // the tallest height-having overlay is the one that counts
            height_3d = std::max( height_3d, overlay_height_3d );
        }
    }
}

bool cata_tiles::draw_item_highlight( const tripoint &pos, int &height_3d )
{
    return draw_from_id_string( ITEM_HIGHLIGHT, TILE_CATEGORY::NONE, empty_string, pos, 0, 0,
                                lit_level::LIT, false, height_3d );
}

std::shared_ptr<const tileset> tileset_cache::load_tileset( const std::string &tileset_id,
        const SDL_Renderer_Ptr &renderer, const bool precheck, const bool force, const bool pump_events )
{
    const auto get_or_create_tileset = [&]() {
        const auto it = tilesets_.find( tileset_id );
        if( it == tilesets_.end() || it->second.expired() ) {
            std::shared_ptr<tileset> new_ts = std::make_shared<tileset>();
            loader loader( *new_ts, renderer );
            loader.load( tileset_id, precheck, pump_events );
            tilesets_.emplace( tileset_id, new_ts );
            return new_ts;
        }
        return it->second.lock();
    };

    std::shared_ptr<tileset> ts = get_or_create_tileset();

    if( force || ( ts->get_tileset_id().empty() && !precheck ) ) {
        loader loader( *ts, renderer );
        loader.load( tileset_id, precheck, pump_events );
    }
    return ts;
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
    throwErrorIf( SDL_FillRect( surface.get(), nullptr, SDL_MapRGBA( surface->format, 0, 0, 127,
                                highlight_alpha ) ) != 0, "SDL_FillRect failed" );
    ts.tile_values.emplace_back( CreateTextureFromSurface( renderer, surface ),
                                 SDL_Rect{ 0, 0, ts.tile_width, ts.tile_height } );
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
void cata_tiles::init_draw_async_anim( const tripoint &p, const std::string &tile_id )
{
    do_draw_async_anim = true;
    async_anim_layer[ p ] = tile_id;
}
void cata_tiles::init_draw_radiation_override( const tripoint &p, const int rad )
{
    radiation_override.emplace( p, rad );
}
void cata_tiles::init_draw_terrain_override( const tripoint &p, const ter_id &id )
{
    terrain_override.emplace( p, id );
}
void cata_tiles::init_draw_furniture_override( const tripoint &p, const furn_id &id )
{
    furniture_override.emplace( p, id );
}
void cata_tiles::init_draw_graffiti_override( const tripoint &p, const bool has )
{
    graffiti_override.emplace( p, has );
}
void cata_tiles::init_draw_trap_override( const tripoint &p, const trap_id &id )
{
    trap_override.emplace( p, id );
}
void cata_tiles::init_draw_field_override( const tripoint &p, const field_type_id &id )
{
    field_override.emplace( p, id );
}
void cata_tiles::init_draw_item_override( const tripoint &p, const itype_id &id,
        const mtype_id &mid, const bool hilite )
{
    item_override.emplace( p, std::make_tuple( id, mid, hilite ) );
}
void cata_tiles::init_draw_vpart_override( const tripoint &p, const vpart_id &id,
        const int part_mod, const units::angle &veh_dir, const bool hilite, const point &mount )
{
    vpart_override.emplace( p, std::make_tuple( id, part_mod, veh_dir, hilite, mount ) );
}
void cata_tiles::init_draw_below_override( const tripoint &p, const bool draw )
{
    draw_below_override.emplace( p, draw );
}
void cata_tiles::init_draw_monster_override( const tripoint &p, const mtype_id &id, const int count,
        const bool more, const Creature::Attitude att )
{
    monster_override.emplace( p, std::make_tuple( id, count, more, att ) );
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
void cata_tiles::void_async_anim()
{
    do_draw_async_anim = false;
    async_anim_layer.clear();
}
void cata_tiles::void_radiation_override()
{
    radiation_override.clear();
}
void cata_tiles::void_terrain_override()
{
    terrain_override.clear();
}
void cata_tiles::void_furniture_override()
{
    furniture_override.clear();
}
void cata_tiles::void_graffiti_override()
{
    graffiti_override.clear();
}
void cata_tiles::void_trap_override()
{
    trap_override.clear();
}
void cata_tiles::void_field_override()
{
    field_override.clear();
}
void cata_tiles::void_item_override()
{
    item_override.clear();
}
void cata_tiles::void_vpart_override()
{
    vpart_override.clear();
}
void cata_tiles::void_draw_below_override()
{
    draw_below_override.clear();
}
void cata_tiles::void_monster_override()
{
    monster_override.clear();
}

bool cata_tiles::has_draw_override( const tripoint &p ) const
{
    return radiation_override.find( p ) != radiation_override.end() ||
           terrain_override.find( p ) != terrain_override.end() ||
           furniture_override.find( p ) != furniture_override.end() ||
           graffiti_override.find( p ) != graffiti_override.end() ||
           trap_override.find( p ) != trap_override.end() ||
           field_override.find( p ) != field_override.end() ||
           item_override.find( p ) != item_override.end() ||
           vpart_override.find( p ) != vpart_override.end() ||
           draw_below_override.find( p ) != draw_below_override.end() ||
           monster_override.find( p ) != monster_override.end();
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
                             subtile, rotation++, lit_level::LIT, nv_goggles_activated );
        draw_from_id_string( exp_name, exp_pos + point( -i, i ),
                             subtile, rotation++, lit_level::LIT, nv_goggles_activated );
        draw_from_id_string( exp_name, exp_pos + point( i, i ),
                             subtile, rotation++, lit_level::LIT, nv_goggles_activated );
        draw_from_id_string( exp_name, exp_pos + point( i, -i ),
                             subtile, rotation, lit_level::LIT, nv_goggles_activated );

        subtile = edge;
        for( int j = 1 - i; j < 0 + i; j++ ) {
            rotation = 0;
            draw_from_id_string( exp_name, exp_pos + point( j, -i ),
                                 subtile, rotation, lit_level::LIT, nv_goggles_activated );
            draw_from_id_string( exp_name, exp_pos + point( j, i ),
                                 subtile, rotation, lit_level::LIT, nv_goggles_activated );

            rotation = 1;
            draw_from_id_string( exp_name, exp_pos + point( -i, j ),
                                 subtile, rotation, lit_level::LIT, nv_goggles_activated );
            draw_from_id_string( exp_name, exp_pos + point( i, j ),
                                 subtile, rotation, lit_level::LIT, nv_goggles_activated );
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
            case N_WEST | N_EAST | N_NORTH | N_SOUTH:
                // Needs some special tile
                subtile = edge;
                break;
        }

        const tripoint &p = pr.first;
        std::string explosion_tile_id;
        // Use target sprite if exist, otherwise col will determine fallback sprite
        if( pr.second.tile_name &&
            find_tile_looks_like( *pr.second.tile_name, TILE_CATEGORY::NONE, "" ) ) {
            explosion_tile_id = *pr.second.tile_name;
        } else if( col == c_red ) {
            explosion_tile_id = exp_strong;
        } else if( col == c_yellow ) {
            explosion_tile_id = exp_medium;
        } else if( col == c_black ) {
            // c_black results in no fallback sprite
            return;
        } else {
            explosion_tile_id = exp_weak;
        }

        draw_from_id_string( explosion_tile_id, p, subtile, rotation, lit_level::LIT,
                             nv_goggles_activated );
    }
}
void cata_tiles::draw_bullet_frame()
{
    draw_from_id_string( bul_id, TILE_CATEGORY::BULLET, empty_string, bul_pos, 0, 0,
                         lit_level::LIT, false );
}
void cata_tiles::draw_hit_frame()
{
    std::string hit_overlay = "animation_hit";

    draw_from_id_string( hit_entity_id, TILE_CATEGORY::HIT_ENTITY, empty_string, hit_pos, 0, 0,
                         lit_level::LIT, false );
    draw_from_id_string( hit_overlay, hit_pos, 0, 0, lit_level::LIT, false );
}
void cata_tiles::draw_line()
{
    if( line_trajectory.empty() ) {
        return;
    }
    static std::string line_overlay = "animation_line";
    if( !is_target_line || get_player_view().sees( line_pos ) ) {
        for( auto it = line_trajectory.begin(); it != line_trajectory.end() - 1; ++it ) {
            draw_from_id_string( line_overlay, *it, 0, 0, lit_level::LIT, false );
        }
    }

    draw_from_id_string( line_endpoint_id, line_trajectory.back(), 0, 0, lit_level::LIT, false );
}
void cata_tiles::draw_cursor()
{
    for( const tripoint &p : cursors ) {
        draw_from_id_string( "cursor", p, 0, 0, lit_level::LIT, false );
    }
}
void cata_tiles::draw_highlight()
{
    for( const tripoint &p : highlights ) {
        draw_from_id_string( "highlight", p, 0, 0, lit_level::LIT, false );
    }
}
void cata_tiles::draw_weather_frame()
{

    for( auto &vdrop : anim_weather.vdrops ) {
        // TODO: Z-level awareness if weather ever happens on anything but z-level 0.
        point p( vdrop.first, vdrop.second );
        if( !is_isometric() ) {
            // currently in ASCII screen coordinates
            const std::optional temp = tile_to_player( p );
            if( !temp.has_value() ) {
                continue;
            }
            p = temp.value();
        }
        const tripoint pos( p, 0 );
        draw_from_id_string( weather_name, TILE_CATEGORY::WEATHER, empty_string, pos, 0, 0,
                             lit_level::LIT, nv_goggles_activated );
    }
}

void cata_tiles::draw_sct_frame( std::multimap<point, formatted_text> &overlay_strings )
{
    const bool use_font = get_option<bool>( "ANIMATION_SCT_USE_FONT" );
    tripoint player_pos = get_player_character().pos();

    for( const scrollingcombattext::cSCT &sct : SCT.vSCT ) {
        const point iD( sct.getPosX(), sct.getPosY() );
        const int full_text_length = utf8_width( sct.getText() );

        point iOffset;

        for( int j = 0; j < 2; ++j ) {
            std::string sText = sct.getText( ( j == 0 ) ? "first" : "second" );
            int FG = msgtype_to_tilecolor( sct.getMsgType( ( j == 0 ) ? "first" : "second" ),
                                           sct.getStep() >= scrollingcombattext::iMaxSteps / 2 );

            if( use_font ) {
                const direction direction = sct.getDirection();
                // Compensate for string length offset added at SCT creation
                // (it will be readded using font size and proper encoding later).
                const int direction_offset = ( -displace_XY( direction ).x + 1 ) *
                                             full_text_length / 2;

                overlay_strings.emplace(
                    player_to_screen( iD + point( direction_offset, 0 ) ),
                    formatted_text( sText, FG, direction ) );
            } else {
                for( char &it : sText ) {
                    const std::string generic_id = get_ascii_tile_id( it, FG, -1 );

                    if( tileset_ptr->find_tile_type( generic_id ) ) {
                        draw_from_id_string( generic_id, TILE_CATEGORY::NONE, empty_string,
                                             iD + tripoint( iOffset, player_pos.z ),
                                             0, 0, lit_level::LIT, false );
                    }

                    if( is_isometric() ) {
                        iOffset.y++;
                    }
                    iOffset.x++;
                }
            }
        }
    }
}

void cata_tiles::draw_zones_frame()
{
    tripoint player_pos = get_player_character().pos();
    for( int iY = zone_start.y; iY <= zone_end.y; ++ iY ) {
        for( int iX = zone_start.x; iX <= zone_end.x; ++iX ) {
            draw_from_id_string( "highlight", TILE_CATEGORY::NONE, empty_string,
                                 zone_offset.xy() + tripoint( iX, iY, player_pos.z ),
                                 0, 0, lit_level::LIT, false );
        }
    }

}

void cata_tiles::draw_async_anim()
{
    // game::draw_async_anim can be called multiple times, storing each animation to be played in async_anim_layer
    // Iterate through every tripoint-tileid pair in async_anim_layer
    for( const auto &anim : async_anim_layer ) {
        const tripoint p = anim.first;
        const std::string tile_id = anim.second;
        // Only draw if sprite found
        if( find_tile_looks_like( tile_id, TILE_CATEGORY::NONE, "" ) ) {
            draw_from_id_string( tile_id, p, 0, 0, lit_level::LIT, nv_goggles_activated );
        }
    }
}

void cata_tiles::draw_footsteps_frame( const tripoint &center )
{
    static const std::string id_footstep = "footstep";
    static const std::string id_footstep_above = "footstep_above";
    static const std::string id_footstep_below = "footstep_below";

    const tile_type *tl_above = tileset_ptr->find_tile_type( id_footstep_above );
    const tile_type *tl_below = tileset_ptr->find_tile_type( id_footstep_below );

    for( const tripoint &pos : sounds::get_footstep_markers() ) {
        if( pos.z > center.z && tl_above ) {
            draw_from_id_string( id_footstep_above, pos, 0, 0, lit_level::LIT, false );
        } else if( pos.z < center.z && tl_below ) {
            draw_from_id_string( id_footstep_below, pos, 0, 0, lit_level::LIT, false );
        } else {
            draw_from_id_string( id_footstep, pos, 0, 0, lit_level::LIT, false );
        }
    }
}
/* END OF ANIMATION FUNCTIONS */

void cata_tiles::tile_loading_report_dups()
{

    std::vector<std::string> dups_list;
    const std::unordered_set<std::string> &dups_set = tileset_ptr->get_duplicate_ids();
    std::copy( dups_set.begin(), dups_set.end(), std::back_inserter( dups_list ) );
    // NOLINTNEXTLINE(cata-use-localized-sorting)
    std::sort( dups_list.begin(), dups_list.end() );

    std::string res;
    for( const std::string &s : dups_list ) {
        res += s;
        res += " ";
    }
    DebugLog( D_INFO, DC_ALL ) << "Have duplicates: " << res;
}

void cata_tiles::init_light()
{
    g->reset_light_level();
}

void cata_tiles::get_terrain_orientation( const tripoint &p, int &rota, int &subtile,
        const std::map<tripoint, ter_id> &ter_override, const std::array<bool, 5> &invisible,
        const std::bitset<NUM_TERCONN> &rotate_group )
{
    map &here = get_map();
    const bool overridden = ter_override.find( p ) != ter_override.end();
    const auto ter = [&]( const tripoint & q, const bool invis ) -> ter_id {
        const auto override = ter_override.find( q );
        return override != ter_override.end() ? override->second :
        ( !overridden || !invis ) ? here.ter( q ) : t_null;
    };

    // get terrain at x,y
    const ter_id tid = ter( p, invisible[0] );
    if( tid == t_null ) {
        subtile = 0;
        rota = 0;
        return;
    }

    // get terrain neighborhood
    const std::array<ter_id, 4> neighborhood = {
        ter( p + point_south, invisible[1] ),
        ter( p + point_east, invisible[2] ),
        ter( p + point_west, invisible[3] ),
        ter( p + point_north, invisible[4] )
    };

    char val = 0;

    // populate connection information
    for( int i = 0; i < 4; ++i ) {
        if( neighborhood[i] == tid ) {
            val += 1 << i;
        }
    }

    uint8_t rotation_targets = get_map().get_known_rotates_to( p, rotate_group, {} );
    get_rotation_and_subtile( val, rotation_targets, rota, subtile );
}

void cata_tiles::get_rotation_and_subtile( const char val, const char rot_to, int &rotation,
        int &subtile )
{
    const bool no_rotation = rot_to == CHAR_MAX;
    switch( val ) {
        // no connections
        case 0:
            subtile = unconnected;
            if( no_rotation ) {
                rotation = 0;
                break;
            }
            rotation = get_rotation_unconnected( rot_to );
            break;
        // all connections
        case 15:
            subtile = center;
            rotation = 0;
            break;
        // end pieces
        // rotations:
        //
        // --> edge index
        // Nw, Ws, Sw, Es,
        // Ne, Wn, Se, En,  |
        // N+, W+, S+, E+,  V  get_rotation_... return index
        // N-, W-, S-, E-
        //
        // (Nw = north end piece, rotated to west)
        case 8:
            // vertical end piece S
            subtile = end_piece;
            if( no_rotation ) {
                rotation = 2;
                break;
            }
            rotation = 2 + 4 * get_rotation_edge_ns( rot_to );
            break;
        case 4:
            // horizontal end piece E
            subtile = end_piece;
            if( no_rotation ) {
                rotation = 3;
                break;
            }
            rotation = 3 + 4 * get_rotation_edge_ew( rot_to );
            break;
        case 2:
            // horizontal end piece W
            subtile = end_piece;
            if( no_rotation ) {
                rotation = 1;
                break;
            }
            rotation = 1 + 4 * get_rotation_edge_ew( rot_to );
            break;
        case 1:
            // vertical end piece N
            subtile = end_piece;
            if( no_rotation ) {
                rotation = 0;
                break;
            }
            rotation = 4 * get_rotation_edge_ns( rot_to );
            break;
        // edges
        // rotations:
        //
        // --> edge index
        // NSw, EWs,
        // NSe, EWn,  |
        // NS+, EW+,  V  get_rotation_... return index
        // NS-, EW-,
        //
        // (NSw = north-south edge, rotated to west)
        case 9:
            // vertical edge
            subtile = edge;
            if( no_rotation ) {
                rotation = 0;
                break;
            }
            rotation = 2 * get_rotation_edge_ns( rot_to );
            break;
        case 6:
            // horizontal edge
            subtile = edge;
            if( no_rotation ) {
                rotation = 1;
                break;
            }
            rotation = 1 + 2 * get_rotation_edge_ew( rot_to );
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

int cata_tiles::get_rotation_edge_ns( const char rot_to )
{
    if( ( rot_to & static_cast<int>( NEIGHBOUR::EAST ) ) == static_cast<int>( NEIGHBOUR::EAST ) ) {
        if( ( rot_to & static_cast<int>( NEIGHBOUR::WEST ) ) == static_cast<int>( NEIGHBOUR::WEST ) ) {
            // EW
            return 2;
        } else {
            // Ew
            return 1;
        }
    } else { // east -
        if( ( rot_to & static_cast<int>( NEIGHBOUR::WEST ) ) == static_cast<int>( NEIGHBOUR::WEST ) ) {
            // eW
            return 0;
        } else {
            // ew
            return 3;
        }
    }
}

int cata_tiles::get_rotation_edge_ew( const char rot_to )
{
    if( ( rot_to & static_cast<int>( NEIGHBOUR::NORTH ) ) == static_cast<int>( NEIGHBOUR::NORTH ) ) {
        if( ( rot_to & static_cast<int>( NEIGHBOUR::SOUTH ) ) == static_cast<int>( NEIGHBOUR::SOUTH ) ) {
            // NS
            return 2;
        } else {
            // Ns
            return 1;
        }
    } else { // north -
        if( ( rot_to & static_cast<int>( NEIGHBOUR::SOUTH ) ) == static_cast<int>( NEIGHBOUR::SOUTH ) ) {
            // nS
            return 0;
        } else {
            // ns
            return 3;
        }
    }
}

int cata_tiles::get_rotation_unconnected( const char rot_to )
{
    int rotation = 0;
    switch( rot_to ) {
        // Catch no and all first for performance; these are the last sprites!
        case 0: // NONE
            rotation = 15;
            break;
        case 15: // ALL
            rotation = 12;
            break;

        // Cases for single tile to rotate to -> easy
        case static_cast<int>( NEIGHBOUR::NORTH ):
            rotation = 2;
            break;
        case static_cast<int>( NEIGHBOUR::EAST ):
            rotation = 3;
            break;
        case static_cast<int>( NEIGHBOUR::SOUTH ):
            rotation = 0;
            break;
        case static_cast<int>( NEIGHBOUR::WEST ):
            rotation = 1;
            break;
        // Two tiles, resulting in diagonal
        case 10: // NE
            rotation = 6;
            break;
        case 3: // SE
            rotation = 7;
            break;
        case 5: // SW
            rotation = 4;
            break;
        case 12: // NW
            rotation = 5;
            break;
        // Cases for three tiles to rotate to -> easy
        // Arranged to fallback / modulo to fitting index 0-4
        case 14: // 3 but south --> modulo = north
            rotation = 10;
            break;
        case 11: // 3 but west --> modulo = east
            rotation = 11;
            break;
        case 7: // 3 but north --> modulo = south
            rotation = 8;
            break;
        case 13: // 3 but east --> modulo = west
            rotation = 9;
            break;
        // Two opposing tiles, (No tiles, all tiles; see first cases)
        case 9: // N-S
            rotation = 14;
            break;
        case 6: // E-W
            rotation = 13;
            break;
    }

    return rotation;
}

void cata_tiles::get_connect_values( const tripoint &p, int &subtile, int &rotation,
                                     const std::bitset<NUM_TERCONN> &connect_group,
                                     const std::bitset<NUM_TERCONN> &rotate_to_group,
                                     const std::map<tripoint, ter_id> &ter_override )
{
    uint8_t connections = get_map().get_known_connections( p, connect_group, ter_override );
    uint8_t rotation_targets = get_map().get_known_rotates_to( p, rotate_to_group, ter_override );
    get_rotation_and_subtile( connections, rotation_targets, rotation, subtile );
}

void cata_tiles::get_furn_connect_values( const tripoint &p, int &subtile, int &rotation,
        const std::bitset<NUM_TERCONN> &connect_group, const std::bitset<NUM_TERCONN> &rotate_to_group,
        const std::map<tripoint, furn_id> &furn_override )
{
    uint8_t connections = get_map().get_known_connections_f( p, connect_group, furn_override );
    uint8_t rotation_targets = get_map().get_known_rotates_to_f( p, rotate_to_group, {}, {} );
    get_rotation_and_subtile( connections, rotation_targets, rotation, subtile );
}

void cata_tiles::get_tile_values( const int t, const std::array<int, 4> &tn, int &subtile,
                                  int &rotation, const char rotation_targets )
{
    std::array<bool, 4> connects;
    char val = 0;
    for( int i = 0; i < 4; ++i ) {
        connects[i] = ( tn[i] == t );
        if( connects[i] ) {
            val += 1 << i;
        }
    }
    get_rotation_and_subtile( val, rotation_targets, rotation, subtile );
}

void cata_tiles::get_tile_values_with_ter(
    const tripoint &p, const int t, const std::array<int, 4> &tn, int &subtile, int &rotation,
    const std::bitset<NUM_TERCONN> &rotate_to_group )
{
    map &here = get_map();
    uint8_t rotation_targets = get_map().get_known_rotates_to_f( p, rotate_to_group, {} );
    //check if furniture should connect to itself
    if( here.has_flag( ter_furn_flag::TFLAG_NO_SELF_CONNECT, p ) ||
        here.has_flag( ter_furn_flag::TFLAG_ALIGN_WORKBENCH, p ) ) {
        //if we don't ever connect to ourself just return unconnected to be used further
        get_rotation_and_subtile( 0, rotation_targets, rotation, subtile );
    } else {
        //if we do connect to ourself (tables, counters etc.) calculate based on neighbours
        get_tile_values( t, tn, subtile, rotation, rotation_targets );
    }
    // calculate rotation for unconnected tiles based on surrounding walls
    // if not any rotates_to neighbours
    if( subtile == unconnected && ( rotation_targets == 0 || rotation_targets == CHAR_MAX ) ) {
        int val = 0;
        bool use_furniture = false;

        if( here.has_flag( ter_furn_flag::TFLAG_ALIGN_WORKBENCH, p ) ) {
            for( int i = 0; i < 4; ++i ) {
                // align to furniture that has the workbench quality
                const tripoint &pt = p + four_adjacent_offsets[i];
                if( here.has_furn( pt ) && here.furn( pt ).obj().workbench ) {
                    val += 1 << i;
                    use_furniture = true;
                }
            }
        }
        // if still unaligned, try aligning to walls
        if( val == 0 ) {
            for( int i = 0; i < 4; ++i ) {
                const tripoint &pt = p + four_adjacent_offsets[i];
                if( here.has_flag( ter_furn_flag::TFLAG_WALL, pt ) ||
                    here.has_flag( ter_furn_flag::TFLAG_WINDOW, pt ) ||
                    here.has_flag( ter_furn_flag::TFLAG_DOOR, pt ) ) {
                    val += 1 << i;
                }
            }
        }

        switch( val ) {
            case 4:    // south wall
            case 14:   // north opening T
                rotation = 2;
                break;
            case 2:    // east wall
            case 6:    // southeast corner
            case 5:    // E/W corridor
            case 7:    // east opening T
                rotation = 1;
                break;
            case 8:    // west wall
            case 12:   // southwest corner
            case 13:   // west opening T
                rotation = 3;
                break;
            case 0:    // no walls
            case 1:    // north wall
            case 3:    // northeast corner
            case 9:    // northwest corner
            case 10:   // N/S corridor
            case 11:   // south opening T
            case 15:   // surrounded
            default:   // just in case
                rotation = rotate_to_group.none() ? 0 : 15;
                break;
        }

        //
        if( use_furniture ) {
            rotation = ( rotation + 2 ) % 4;
        }
    }
}

void cata_tiles::do_tile_loading_report()
{
    DebugLog( D_INFO, DC_ALL ) << "Loaded tileset: " << get_option<std::string>( "TILES" );

    if( !g->is_core_data_loaded() ) {
        // There's nothing to do anymore without the core data.
        return;
    }

    tile_loading_report_seq_types( vehicles::parts::get_all(), TILE_CATEGORY::VEHICLE_PART, "vp_" );
    tile_loading_report_count<ter_t>( ter_t::count(), TILE_CATEGORY::TERRAIN );

    std::map<itype_id, const itype *> items;
    for( const itype *e : item_controller->all() ) {
        items.emplace( e->get_id(), e );
    }
    tile_loading_report_map( items, TILE_CATEGORY::ITEM );

    tile_loading_report_count<furn_t>( furn_t::count(), TILE_CATEGORY::FURNITURE );
    tile_loading_report_count<trap>( trap::count(), TILE_CATEGORY::TRAP );
    tile_loading_report_count<field_type>( field_type::count(), TILE_CATEGORY::FIELD );

    // TODO: LIGHTING

    tile_loading_report_seq_types( MonsterGenerator::generator().get_all_mtypes(),
                                   TILE_CATEGORY::MONSTER );

    // TODO: BULLET
    // TODO: HIT_ENTITY
    // TODO: WEATHER

    std::vector<oter_type_str_id> oter_types;
    for( const oter_t &oter : overmap_terrains::get_all() ) {
        oter_types.push_back( oter.get_type_id() );
    }
    std::sort( oter_types.begin(), oter_types.end() );
    oter_types.erase( std::unique( oter_types.begin(), oter_types.end() ), oter_types.end() );
    tile_loading_report_seq_ids( oter_types, TILE_CATEGORY::OVERMAP_TERRAIN );

    std::vector<map_extra_id> map_extra_ids = MapExtras::get_all_function_names();
    map_extra_ids.erase(
        std::remove_if( map_extra_ids.begin(), map_extra_ids.end(),
    []( const map_extra_id & id ) {
        return !id->autonote;
    } ), map_extra_ids.end() );
    tile_loading_report_seq_ids( map_extra_ids, TILE_CATEGORY::MAP_EXTRA );

    // TODO: OVERMAP_NOTE

    static_assert( static_cast<int>( TILE_CATEGORY::last ) == 15,
                   "If you add more tile categories then update this tile loading report and then "
                   "increment the value in this static_assert accordingly" );

    tile_loading_report_dups();

    // needed until DebugLog ostream::flush bugfix lands
    DebugLog( D_INFO, DC_ALL );
}

template<typename Iter, typename Func>
void cata_tiles::lr_generic( Iter begin, Iter end, Func id_func, TILE_CATEGORY category,
                             const std::string &prefix )
{
    std::string missing_list;
    std::string missing_with_looks_like_list;
    for( ; begin != end; ++begin ) {
        const std::string id_string = id_func( begin );

        if( !tileset_ptr->find_tile_type( prefix + id_string ) &&
            !find_tile_looks_like( id_string, category, "" ) ) {
            missing_list.append( id_string + " " );
        } else if( !tileset_ptr->find_tile_type( prefix + id_string ) ) {
            missing_with_looks_like_list.append( id_string + " " );
        }
    }
    const std::string &category_name = TILE_CATEGORY_IDS[static_cast<size_t>( category )];
    DebugLog( D_INFO, DC_ALL ) << "Missing " << category_name << ": " << missing_list;
    DebugLog( D_INFO, DC_ALL ) << "Missing " << category_name <<
                               " (but looks_like tile exists): " << missing_with_looks_like_list;
}

template <typename maptype>
void cata_tiles::tile_loading_report_map( const maptype &tiletypemap, TILE_CATEGORY category,
        const std::string &prefix )
{
    lr_generic( tiletypemap.begin(), tiletypemap.end(),
    []( const decltype( tiletypemap.begin() ) & v ) {
        // c_str works for std::string and for string_id!
        return v->first.c_str();
    }, category, prefix );
}

template <typename Sequence>
void cata_tiles::tile_loading_report_seq_types( const Sequence &tiletypes, TILE_CATEGORY category,
        const std::string &prefix )
{
    lr_generic( tiletypes.begin(), tiletypes.end(),
    []( const decltype( tiletypes.begin() ) & v ) {
        return v->id.c_str();
    }, category, prefix );
}

template <typename Sequence>
void cata_tiles::tile_loading_report_seq_ids( const Sequence &tiletypes, TILE_CATEGORY category,
        const std::string &prefix )
{
    lr_generic( tiletypes.begin(), tiletypes.end(),
    []( const decltype( tiletypes.begin() ) & v ) {
        // c_str works for std::string and for string_id!
        return v->c_str();
    }, category, prefix );
}

template <typename base_type>
void cata_tiles::tile_loading_report_count( const size_t count, TILE_CATEGORY category,
        const std::string &prefix )
{
    lr_generic( static_cast<size_t>( 0 ), count,
    []( const size_t i ) {
        return int_id<base_type>( i ).id().str();
    }, category, prefix );
}

std::vector<options_manager::id_and_option> cata_tiles::build_renderer_list()
{
    std::vector<options_manager::id_and_option> renderer_names;
    std::vector<options_manager::id_and_option> default_renderer_names = {
#   if defined(_WIN32)
        { "direct3d", to_translation( "direct3d" ) },
#   endif
        { "software", to_translation( "software" ) },
        { "opengl", to_translation( "opengl" ) },
        { "opengles2", to_translation( "opengles2" ) },
    };
    int numRenderDrivers = SDL_GetNumRenderDrivers();
    for( int ii = 0; ii < numRenderDrivers; ii++ ) {
        SDL_RendererInfo ri;
        SDL_GetRenderDriverInfo( ii, &ri );
        // First default renderer name we will put first on the list. We can use it later as
        // default value.
        if( ri.name == default_renderer_names.front().first ) {
            renderer_names.emplace( renderer_names.begin(), default_renderer_names.front() );
        } else {
            renderer_names.emplace_back( ri.name, no_translation( ri.name ) );
        }
    }
    DebugLog( D_INFO, DC_ALL ) << "SDL render devices: " << enumerate_as_string( renderer_names,
    []( const options_manager::id_and_option & iao ) {
        return iao.first;
    }, enumeration_conjunction::none );

    return renderer_names.empty() ? default_renderer_names : renderer_names;
}

std::vector<options_manager::id_and_option> cata_tiles::build_display_list()
{
    std::vector<options_manager::id_and_option> display_names;
    std::vector<options_manager::id_and_option> default_display_names = {
        { "0", to_translation( "Display 0" ) }
    };

    int numdisplays = SDL_GetNumVideoDisplays();
    display_names.reserve( numdisplays );
    for( int i = 0 ; i < numdisplays ; i++ ) {
        display_names.emplace_back( std::to_string( i ),
                                    no_translation( SDL_GetDisplayName( i ) ) );
    }

    return display_names.empty() ? default_display_names : display_names;
}

#endif // SDL_TILES
