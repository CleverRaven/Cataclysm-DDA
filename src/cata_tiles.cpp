#if (defined TILES)
#include "cata_tiles.h"

#include "coordinate_conversions.h"
#include "debug.h"
#include "json.h"
#include "path_info.h"
#include "monstergenerator.h"
#include "item_factory.h"
#include "item.h"
#include "veh_type.h"
#include "filesystem.h"
#include "sounds.h"
#include "map.h"
#include "trap.h"
#include "monster.h"
#include "vpart_position.h"
#include "options.h"
#include "overmapbuffer.h"
#include "player.h"
#include "npc.h"
#include "catacharset.h"
#include "itype.h"
#include "vpart_reference.h"
#include "vehicle.h"
#include "game.h"
#include "mapdata.h"
#include "mtype.h"
#include "field.h"
#include "weather.h"
#include "weighted_list.h"
#include "submap.h"
#include "output.h"
#include "overlay_ordering.h"
#include "cata_utility.h"
#include "cursesport.h"
#include "rect_range.h"

#include <cassert>
#include <algorithm>
#include <fstream>
#include <stdlib.h>     /* srand, rand */
#include <sstream>
#include <array>

#include <SDL_image.h>

#define dbg(x) DebugLog((DebugLevel)(x),D_SDL) << __FILE__ << ":" << __LINE__ << ": "

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

extern int fontwidth, fontheight;
extern bool tile_iso;

SDL_Color cursesColorToSDL( const nc_color &color );
///@throws std::exception upon errors.
///@returns Always a valid pointer.
static SDL_Surface_Ptr create_tile_surface( int w, int h );

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
}};


namespace
{
void printErrorIf( const bool condition, const std::string &message )
{
    if( !condition ) {
        return;
    }
    dbg( D_ERROR ) << message << ": " << SDL_GetError();
}

void throwErrorIf( const bool condition, const std::string &message )
{
    if( !condition ) {
        return;
    }
    throw std::runtime_error( message + ": " + SDL_GetError() );
}

    /// Returns a number in range [0..1]. The range lasts for @param phase_length_ms (milliseconds).
    float get_animation_phase( int phase_length_ms )
    {
        if( phase_length_ms == 0 ) {
            return 0.0f;
        }

        return std::fmod<float>( SDL_GetTicks(), phase_length_ms ) / phase_length_ms;
    }

int divide_rounded_up( const int v, const int div )
{
    return ( v / div ) + ( v % div == 0 ? 0 : 1 );
}

std::string get_ascii_tile_id( const uint32_t sym, const int FG, const int BG )
{
    return std::string( { 'A', 'S', 'C', 'I', 'I', '_', static_cast<char>( sym ), static_cast<char>( FG ), static_cast<char>( BG ) } );
}
} // namespace

// Operator overload required to leverage unique_ptr API.
void SDL_Texture_deleter::operator()( SDL_Texture *const ptr )
{
    if( ptr ) {
        SDL_DestroyTexture( ptr );
    }
}

// Operator overload required to leverage unique_ptr API.
void SDL_Surface_deleter::operator()( SDL_Surface *const ptr )
{
    if( ptr ) {
        SDL_FreeSurface( ptr );
    }
}

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

formatted_text::formatted_text( const std::string text, const int color, const direction direction )
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

cata_tiles::cata_tiles(SDL_Renderer *render)
{
    assert( render );
    renderer = render;

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
    do_draw_weather = false;
    do_draw_sct = false;
    do_draw_zones = false;

    nv_goggles_activated = false;
    minimap_prep = false;
    minimap_reinit_flag = false;

    last_pos_x = 0;
    last_pos_y = 0;
}

cata_tiles::~cata_tiles() = default;

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

void cata_tiles::load_tileset( const std::string &tileset_id, const bool precheck )
{
    if( tileset_ptr && tileset_ptr->get_tileset_id() == tileset_id ) {
        return;
    }
    //@todo: move into clear or somewhere else.
    // reset the overlay ordering from the previous loaded tileset
    tileset_mutation_overlay_ordering.clear();

    // Load the tileset into a separate instance and only set this->tileset_ptr
    // when the loading has succeeded.
    std::unique_ptr<tileset> new_tileset_ptr( new tileset() );
    tileset_loader loader( *new_tileset_ptr, renderer );
    loader.load( tileset_id, precheck );
    tileset_ptr = std::move( new_tileset_ptr );

    set_draw_scale(16);
}

void cata_tiles::reinit()
{
    set_draw_scale(16);
    printErrorIf( SDL_RenderClear( renderer ) != 0, "SDL_RenderClear failed" );
    minimap_cache.clear();
    tex_pool.texture_pool.clear();
    reinit_minimap();
}

void cata_tiles::reinit_minimap()
{
    minimap_reinit_flag = true;
}

static void get_tile_information(std::string config_path, std::string &json_path, std::string &tileset_path)
{
    const std::string default_json = FILENAMES["defaulttilejson"];
    const std::string default_tileset = FILENAMES["defaulttilepng"];

    // Get JSON and TILESET vars from config
    const auto reader = [&]( std::istream &fin ) {
        while(!fin.eof()) {
            std::string sOption;
            fin >> sOption;

            if( sOption.empty() ) {
                getline(fin, sOption);
            } else if(sOption[0] == '#') { // Skip comment
                getline(fin, sOption);
            } else if (sOption.find("JSON") != std::string::npos) {
                fin >> json_path;
                dbg( D_INFO ) << "JSON path set to [" << json_path << "].";
            } else if (sOption.find("TILESET") != std::string::npos) {
                fin >> tileset_path;
                dbg( D_INFO ) << "TILESET path set to [" << tileset_path << "].";
            } else {
                getline(fin, sOption);
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

inline static pixel get_pixel_color(SDL_Surface_Ptr &surf, int x, int y, int w)
{
    assert( surf );
    pixel pix;
    const auto pixelarray = reinterpret_cast<unsigned char *>(surf->pixels);
    const auto pixel_ptr = pixelarray + (y * w + x) * 4;
    pix.r = pixel_ptr[0];
    pix.g = pixel_ptr[1];
    pix.b = pixel_ptr[2];
    pix.a = pixel_ptr[3];
    return pix;
}

inline static void set_pixel_color(SDL_Surface_Ptr &surf, int x, int y, int w, pixel pix)
{
    assert( surf );
    const auto pixelarray = reinterpret_cast<unsigned char *>(surf->pixels);
    const auto pixel_ptr = pixelarray + (y * w + x) * 4;
    pixel_ptr[0] = static_cast<unsigned char>(pix.r);
    pixel_ptr[1] = static_cast<unsigned char>(pix.g);
    pixel_ptr[2] = static_cast<unsigned char>(pix.b);
    pixel_ptr[3] = static_cast<unsigned char>(pix.a);
}

static void color_pixel_grayscale(pixel& pix)
{
    bool isBlack = pix.isBlack();
    int result = (pix.r + pix.b + pix.g) / 3;
    result = result * 6 / 10;
    if (result > 255) {
        result = 255;
    }
    //workaround for color key 0 on some tilesets
    if (result<1 && !isBlack){
        result = 1;
    }
    pix.r = result;
    pix.g = result;
    pix.b = result;
}

static void color_pixel_nightvision(pixel& pix)
{
    int result = (pix.r + pix.b + pix.g) / 3;
    result = result * 3 / 4 + 64;
    if (result > 255) {
        result = 255;
    }
    pix.r = result / 4;
    pix.g = result;
    pix.b = result / 7;
}

static void color_pixel_overexposed(pixel& pix)
{
    int result = (pix.r + pix.b + pix.g) / 3;
    result = result / 4 + 192;
    if (result > 255) {
        result = 255;
    }
    pix.r = result / 4;
    pix.g = result;
    pix.b = result / 7;
}

static SDL_Surface_Ptr apply_color_filter( const SDL_Surface_Ptr &original,
        void ( &pixel_converter )( pixel & ) )
{
    assert( original );
    SDL_Surface_Ptr surf = create_tile_surface( original->w, original->h );
    assert( surf );
    throwErrorIf( SDL_BlitSurface( original.get(), NULL, surf.get(), NULL ) != 0, "SDL_BlitSurface failed" );
    for (int y = 0; y < surf->h; y++) {
        for (int x = 0; x < surf->w; x++) {
            pixel pix = get_pixel_color(surf, x, y, surf->w);
            pixel_converter(pix);
            set_pixel_color(surf, x, y, surf->w, pix);
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

void tileset_loader::copy_surface_to_texture( const SDL_Surface_Ptr &surf, const point &offset, std::vector<texture> &target )
{
    assert( surf );
    const rect_range<SDL_Rect> input_range( sprite_width, sprite_height, surf->w / sprite_width,
                                            surf->h / sprite_height );

    const std::shared_ptr<SDL_Texture> texture_ptr( SDL_CreateTextureFromSurface( renderer, surf.get() ), &SDL_DestroyTexture );
    throwErrorIf( !texture_ptr, "SDL_CreateTextureFromSurface failed" );

    for( const SDL_Rect rect : input_range ) {
        assert( offset.x % sprite_width == 0 );
        assert( offset.y % sprite_height == 0 );
        const point pos( offset.x + rect.x, offset.y + rect.y );
        assert( pos.x % sprite_width == 0 );
        assert( pos.y % sprite_height == 0 );
        const size_t index = this->offset + ( pos.x / sprite_width ) + ( pos.y / sprite_height ) * ( tile_atlas_width / sprite_width );
        assert( index < target.size() );
        assert( target[index].dimension() == std::make_pair( 0, 0) );
        target[index] = texture( texture_ptr, rect );
    }
}

void tileset_loader::create_textures_from_tile_atlas( const SDL_Surface_Ptr &tile_atlas, const point &offset )
{
    assert( tile_atlas );
    copy_surface_to_texture( tile_atlas, offset, ts.tile_values );

    /** perform color filter conversion here */
    copy_surface_to_texture( apply_color_filter( tile_atlas, color_pixel_grayscale ), offset, ts.shadow_tile_values );
    copy_surface_to_texture( apply_color_filter( tile_atlas, color_pixel_nightvision ), offset, ts.night_tile_values );
    copy_surface_to_texture( apply_color_filter( tile_atlas, color_pixel_overexposed ), offset, ts.overexposed_tile_values );
}

template<typename T>
static void extend_vector_by( std::vector<T> &vec, const size_t additional_size ) {
    vec.resize( vec.size() + additional_size );
}

void tileset_loader::load_tileset( std::string img_path )
{
    SDL_Surface_Ptr tile_atlas( IMG_Load( img_path.c_str() ) );
    if( !tile_atlas ) {
        throw std::runtime_error( "Could not load tileset image \"" + img_path + "\": " + IMG_GetError() );
    }
    tile_atlas_width = tile_atlas->w;

    if( R >= 0 && R <= 255 && G >= 0 && G <= 255 && B >= 0 && B <= 255 ) {
        const Uint32 key = SDL_MapRGB(tile_atlas->format, 0, 0, 0);
        throwErrorIf( SDL_SetColorKey( tile_atlas.get(), SDL_TRUE, key ) != 0, "SDL_SetColorKey failed" );
        throwErrorIf( SDL_SetSurfaceRLE(tile_atlas.get(), true ), "SDL_SetSurfaceRLE failed" );
    }

    SDL_RendererInfo info;
    throwErrorIf( SDL_GetRendererInfo( renderer, &info ) != 0, "SDL_GetRendererInfo failed" );
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

    if( info.max_texture_width == 0 ){
        info.max_texture_width = sprite_width * min_tile_xcount;
        DebugLog( D_INFO, DC_ALL ) << "SDL_RendererInfo max_texture_width was set to 0.  Changing it to " << info.max_texture_width;
    } else {
        throwErrorIf( info.max_texture_width < sprite_width, "Maximal texture width is smaller than tile width" );
    }

    if( info.max_texture_height == 0 ){
        info.max_texture_height = sprite_height * min_tile_ycount;
        DebugLog( D_INFO, DC_ALL ) << "SDL_RendererInfo max_texture_height was set to 0.  Changing it to " << info.max_texture_height;
    } else {
        throwErrorIf( info.max_texture_height < sprite_height, "Maximal texture height is smaller than tile height" );
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
        divide_rounded_up( tile_atlas->w, info.max_texture_width ),
        divide_rounded_up( tile_atlas->h, info.max_texture_height ) );

    const int expected_tilecount = ( tile_atlas->w / sprite_width ) * ( tile_atlas->h / sprite_height );
    extend_vector_by( ts.tile_values, expected_tilecount );
    extend_vector_by( ts.shadow_tile_values, expected_tilecount );
    extend_vector_by( ts.night_tile_values, expected_tilecount );
    extend_vector_by( ts.overexposed_tile_values, expected_tilecount );

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
            smaller_surf = ::create_tile_surface( w, h );
            assert( smaller_surf );
            const SDL_Rect inp{ sub_rect.x, sub_rect.y, w, h };
            throwErrorIf( SDL_BlitSurface( tile_atlas.get(), &inp, smaller_surf.get(), NULL ) != 0, "SDL_BlitSurface failed" );
        }
        const SDL_Surface_Ptr &surf_to_use = smaller_surf ? smaller_surf : tile_atlas;
        assert( surf_to_use );

        create_textures_from_tile_atlas( surf_to_use, point( sub_rect.x, sub_rect.y ) );
    }

    size = expected_tilecount;
}

void cata_tiles::set_draw_scale(int scale) {
    assert( tileset_ptr );
    tile_width = tileset_ptr->get_tile_width() * tileset_ptr->get_tile_pixelscale() * scale / 16;
    tile_height = tileset_ptr->get_tile_height() * tileset_ptr->get_tile_pixelscale() * scale / 16;

    tile_ratiox = ((float)tile_width/(float)fontwidth);
    tile_ratioy = ((float)tile_height/(float)fontheight);
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
                             json_conf, tileset_path);
        dbg( D_INFO ) << "Current tileset is: " << tileset_id;
    } else {
        dbg( D_ERROR ) << "Tileset \"" << tileset_id << "\" from options is invalid";
        json_conf = FILENAMES["defaulttilejson"];
        tileset_path = FILENAMES["defaulttilepng"];
    }

    std::string json_path = tileset_root + '/' + json_conf;
    std::string img_path = tileset_root + '/' + tileset_path;

    dbg( D_INFO ) << "Attempting to Load JSON file " << json_path;
    std::ifstream config_file(json_path.c_str(), std::ifstream::in | std::ifstream::binary);

    if (!config_file.good()) {
        throw std::runtime_error( std::string("Failed to open tile info json: ") + json_path );
    }

    JsonIn config_json( config_file );
    JsonObject config = config_json.get_object();

    // "tile_info" section must exist.
    if (!config.has_member("tile_info")) {
        config.throw_error( "\"tile_info\" missing" );
    }

    JsonArray info = config.get_array("tile_info");
    while (info.has_more()) {
        JsonObject curr_info = info.next_object();
        ts.tile_height = curr_info.get_int("height");
        ts.tile_width = curr_info.get_int("width");
        tile_iso = curr_info.get_bool("iso", false);
        ts.tile_pixelscale = curr_info.get_float("pixelscale", 1.0f);
    }

    if( precheck ) {
        return;
    }

    // Load tile information if available.
    offset = 0;
    if (config.has_array("tiles-new")) {
        // new system, several entries
        // When loading multiple tileset images this defines where
        // the tiles from the most recently loaded image start from.
        JsonArray tiles_new = config.get_array("tiles-new");
        while (tiles_new.has_more()) {
            JsonObject tile_part_def = tiles_new.next_object();
            const std::string tileset_image_path = tileset_root + '/' + tile_part_def.get_string("file");
            R = -1;
            G = -1;
            B = -1;
            if (tile_part_def.has_object("transparency")) {
                JsonObject tra = tile_part_def.get_object("transparency");
                R = tra.get_int("R");
                G = tra.get_int("G");
                B = tra.get_int("B");
            }
            sprite_width = tile_part_def.get_int( "sprite_width", ts.tile_width );
            sprite_height = tile_part_def.get_int( "sprite_height", ts.tile_height );
            // Now load the tile definitions for the loaded tileset image.
            sprite_offset_x = tile_part_def.get_int("sprite_offset_x",0);
            sprite_offset_y = tile_part_def.get_int("sprite_offset_y",0);
            // First load the tileset image to get the number of available tiles.
            dbg( D_INFO ) << "Attempting to Load Tileset file " << tileset_image_path;
            load_tileset( tileset_image_path );
            load_tilejson_from_file( tile_part_def );
            if (tile_part_def.has_member("ascii")) {
                load_ascii( tile_part_def );
            }
            // Make sure the tile definitions of the next tileset image don't
            // override the current ones.
            offset += size;
        }
    } else {
        sprite_width = ts.tile_width;
        sprite_height = ts.tile_height;
        sprite_offset_x = 0;
        sprite_offset_y = 0;
        R = -1;
        G = -1;
        B = -1;
        // old system, no tile file path entry, only one array of tiles
        dbg( D_INFO ) << "Attempting to Load Tileset file " << img_path;
        load_tileset( img_path );
        load_tilejson_from_file(config);
        offset = size;
    }

    // allows a tileset to override the order of mutation images being applied to a character
    if( config.has_array( "overlay_ordering" ) ) {
        load_overlay_ordering_into_array( config, tileset_mutation_overlay_ordering );
    }

    // offset should be the total number of sprites loaded from every tileset image
    // eliminate any sprite references that are too high to exist
    // also eliminate negative sprite references

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

void tileset_loader::add_ascii_subtile( tile_type &curr_tile, const std::string &t_id, int sprite_id,
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
        curr_tile.offset.x = sprite_offset_x;
        curr_tile.offset.y = sprite_offset_y;
        auto &sprites = *( curr_tile.fg.add( std::vector<int>( {index_in_image + offset} ), 1 ) );
        switch( ascii_char ) {
            case LINE_OXOX_C://box bottom/top side (horizontal line)
                sprites[0] = 205 + base_offset;
                break;
            case LINE_XOXO_C://box left/right side (vertical line)
                sprites[0] = 186 + base_offset;
                break;
            case LINE_OXXO_C://box top left
                sprites[0] = 201 + base_offset;
                break;
            case LINE_OOXX_C://box top right
                sprites[0] = 187 + base_offset;
                break;
            case LINE_XOOX_C://box bottom right
                sprites[0] = 188 + base_offset;
                break;
            case LINE_XXOO_C://box bottom left
                sprites[0] = 200 + base_offset;
                break;
            case LINE_XXOX_C://box bottom north T (left, right, up)
                sprites[0] = 202 + base_offset;
                break;
            case LINE_XXXO_C://box bottom east T (up, right, down)
                sprites[0] = 208 + base_offset;
                break;
            case LINE_OXXX_C://box bottom south T (left, right, down)
                sprites[0] = 203 + base_offset;
                break;
            case LINE_XXXX_C://box X (left down up right)
                sprites[0] = 206 + base_offset;
                break;
            case LINE_XOXX_C://box bottom east T (left, down, up)
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
            add_ascii_subtile( curr_tile, id, 208 + base_offset, "end_piece" );
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
        for( auto t_id : ids ) {
            tile_type &curr_tile = load_tile( entry, t_id );
            curr_tile.offset.x = sprite_offset_x;
            curr_tile.offset.y = sprite_offset_y;
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
                    curr_subtile.offset.x = sprite_offset_x;
                    curr_subtile.offset.y = sprite_offset_y;
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
 * @param size The number of tiles loaded from the current tileset file. This defines the
 * range of valid tile ids that can be loaded. An exception is thrown if any tile id is outside
 * that range.
 * @return A reference to the loaded tile inside the @ref tileset::tile_ids map.
 */
tile_type &tileset_loader::load_tile( JsonObject &entry, const std::string &id )
{
    tile_type curr_subtile;

    load_tile_spritelists( entry, curr_subtile.fg, "fg" );
    load_tile_spritelists( entry, curr_subtile.bg, "bg" );

    return ts.create_tile_type( id, std::move( curr_subtile ) );
}

void tileset_loader::load_tile_spritelists( JsonObject &entry, weighted_int_list<std::vector<int>> &vs,
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
                const int sprite_id = g_array.next_int();
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
                    const int sprite_id = vo.get_int( "sprite" );
                    if( sprite_id >= 0 ) {
                        v.push_back( sprite_id );
                    }
                }
                // array sprite means rotations
                else if( vo.has_array( "sprite" ) ) {
                    JsonArray sprites = vo.get_array( "sprite" );
                    while( sprites.has_more() ) {
                        const int sprite_id = sprites.next_int();
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
        vs.add( std::vector<int>( {entry.get_int( objname )} ), 1 );
    }
}

struct tile_render_info {
    const tripoint pos;
    int height_3d = 0; // accumulator for 3d tallness of sprites rendered here so far
    tile_render_info(const tripoint &pos, int height_3d) : pos(pos), height_3d(height_3d)
    {
    }
};

void cata_tiles::draw( int destx, int desty, const tripoint &center, int width, int height, 
                       std::multimap<point, formatted_text> &overlay_strings )
{
    if (!g) {
        return;
    }

    {
        //set clipping to prevent drawing over stuff we shouldn't
        SDL_Rect clipRect = {destx, desty, width, height};
        printErrorIf( SDL_RenderSetClipRect( renderer, &clipRect ) != 0, "SDL_RenderSetClipRect failed" );

        //fill render area with black to prevent artifacts where no new pixels are drawn
        printErrorIf( SDL_SetRenderDrawColor( renderer, 0, 0, 0, 255 ) != 0, "SDL_SetRenderDrawColor failed" );
        printErrorIf( SDL_RenderFillRect( renderer, &clipRect ) != 0, "SDL_RenderFillRect failed" );
    }

    int posx = center.x;
    int posy = center.y;

    int sx = 0;
    int sy = 0;
    get_window_tile_counts(width, height, sx, sy);

    init_light();
    g->m.update_visibility_cache( center.z );
    const visibility_variables &cache = g->m.get_visibility_variables_cache();

    const bool iso_mode = tile_iso;

    o_x = iso_mode ? posx : posx - POSX;
    o_y = iso_mode ? posy : posy - POSY;

    op_x = destx;
    op_y = desty;
    // Rounding up to include incomplete tiles at the bottom/right edges
    screentile_width = divide_rounded_up( width, tile_width );
    screentile_height = divide_rounded_up( height, tile_height );

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
    if(cache.u_is_boomered) {
        offscreen_type = VIS_BOOMER_DARK;
    }

    //retrieve night vision goggle status once per draw
    auto vision_cache = g->u.get_vision_modes();
    nv_goggles_activated = vision_cache[NV_GOGGLES];

    for( int row = min_row; row < max_row; row ++) {
        std::vector<tile_render_info> draw_points;
        draw_points.reserve(max_col);
        for( int col = min_col; col < max_col; col ++) {
            if(iso_mode) {
                //in isometric, rows and columns represent a checkerboard screen space, and we place
                //the appropriate tile in valid squares by getting position relative to the screen center.
                if( (row + o_y ) % 2 != (col + o_x) % 2 ) {
                    continue;
                }
                x = ( col  - row - sx/2 + sy/2 ) / 2 + o_x;
                y = ( row + col - sy/2 - sx/2 ) / 2 + o_y;
            }
            else {
                x = col + o_x;
                y = row + o_y;
            }
            if( y < min_visible_y || y > max_visible_y || x < min_visible_x || x > max_visible_x ) {
                // draw 1 tile border of offscreen_type around the edge of the max visible area.
                // This is perhaps unnecessary. It only affects tilesets that have a "dark" tile
                // distinguishable from black, allowing them to denote the border of max view range.
                // Adds 4 int comparisons per checked off-map tile, and max 480 extra textures drawn.
                if( y >= min_visible_y - 1 && y <= max_visible_y + 1 && x >= min_visible_x - 1 && x <= max_visible_x + 1 ) {
                    apply_vision_effects( temp, offscreen_type );
                }
                continue;
            }

            if( apply_vision_effects( temp, g->m.get_visibility( ch.visibility_cache[x][y], cache ) ) ) {
                const auto critter = g->critter_at( tripoint(x,y,center.z), true );
                if( critter != nullptr && g->u.sees_with_infrared( *critter ) ) {
                    //TODO defer drawing this until later when we know how tall
                    //     the terrain/furniture under the creature is.
                    draw_from_id_string( "infrared_creature", C_NONE, empty_string, temp, 0, 0, LL_LIT, false );
                }
                continue;
            }

            int height_3d = 0;

            // light level is now used for choosing between grayscale filter and normal lit tiles.
            // Draw Terrain if possible. If not possible then we need to continue on to the next part of loop
            if( !draw_terrain( tripoint(x,y,center.z), ch.visibility_cache[x][y], height_3d ) ) {
                continue;
            }

            draw_points.push_back( tile_render_info( tripoint( x, y, center.z ), height_3d ) );
        }
        const std::array<decltype ( &cata_tiles::draw_furniture ), 7> drawing_layers = {{
            &cata_tiles::draw_furniture, &cata_tiles::draw_trap,
            &cata_tiles::draw_field_or_item, &cata_tiles::draw_vpart,
            &cata_tiles::draw_vpart_below, &cata_tiles::draw_terrain_below,
            &cata_tiles::draw_critter_at
        }};
        // for each of the drawing layers in order, back to front ...
        for( auto f : drawing_layers ) {
            // ... draw all the points we drew terrain for, in the same order
            for( auto &p : draw_points ) {
                (this->*f)( p.pos, ch.visibility_cache[p.pos.x][p.pos.y], p.height_3d );
            }
        }
    }

    in_animation = do_draw_explosion || do_draw_custom_explosion ||
                   do_draw_bullet || do_draw_hit || do_draw_line ||
                   do_draw_weather || do_draw_sct ||
                   do_draw_zones;

    draw_footsteps_frame();
    if (in_animation) {
        if (do_draw_explosion) {
            draw_explosion_frame();
        }
        if (do_draw_custom_explosion) {
            draw_custom_explosion_frame();
        }
        if (do_draw_bullet) {
            draw_bullet_frame();
        }
        if (do_draw_hit) {
            draw_hit_frame();
            void_hit();
        }
        if (do_draw_line) {
            draw_line();
            void_line();
        }
        if (do_draw_weather) {
            draw_weather_frame();
            void_weather();
        }
        if (do_draw_sct) {
            draw_sct_frame( overlay_strings );
            void_sct();
        }
        if (do_draw_zones) {
            draw_zones_frame();
            void_zones();
        }
    } else if( g->u.posx() + g->u.view_offset.x != g->ter_view_x ||
               g->u.posy() + g->u.view_offset.y != g->ter_view_y ) {
        // check to see if player is located at ter
        draw_from_id_string( "cursor", C_NONE, empty_string,
                             {g->ter_view_x, g->ter_view_y, center.z}, 0, 0, LL_LIT, false );
    }
    if( g->u.controlling_vehicle ) {
        tripoint indicator_offset = g->get_veh_dir_indicator_location( true );
        if( indicator_offset != tripoint_min ) {
            draw_from_id_string( "cursor", C_NONE, empty_string,
                                 { indicator_offset.x + g->u.posx(),
                                   indicator_offset.y + g->u.posy(), center.z },
                                 0, 0, LL_LIT, false );
        }
    }

    printErrorIf( SDL_RenderSetClipRect( renderer, nullptr ) != 0, "SDL_RenderSetClipRect failed" );
}

void cata_tiles::draw_rhombus(int destx, int desty, int size, SDL_Color color, int widthLimit, int heightLimit) {
    for(int xOffset = -size; xOffset <= size; xOffset++) {
        for(int yOffset = -size + abs(xOffset); yOffset <= size - abs(xOffset); yOffset++) {
            if(xOffset < widthLimit && yOffset < heightLimit){
                int divisor = 2 * (abs(yOffset) == size - abs(xOffset)) + 1;
                printErrorIf( SDL_SetRenderDrawColor( renderer, color.r / divisor, color.g / divisor, color.b / divisor, 255 ) != 0, "SDL_SetRenderDrawColor failed" );
                printErrorIf( SDL_RenderDrawPoint( renderer, destx + xOffset, desty + yOffset ) != 0, "SDL_RenderDrawPoint failed" );
            }
        }
    }
}

//converts the local x,y point into the global submap coordinates
static tripoint convert_tripoint_to_abs_submap(const tripoint& p)
{
    //get the submap coordinates of the current location
    tripoint sm_loc = ms_to_sm_copy(p);
    //add it to the absolute map coordinates
    tripoint abs_sm_loc = g->m.get_abs_sub();
    return abs_sm_loc + sm_loc;
}

//creates the texture that individual minimap updates are drawn to
//later, the main texture is drawn to the display buffer
//the surface is needed to determine the color format needed by the texture
SDL_Texture_Ptr cata_tiles::create_minimap_cache_texture(int tile_width, int tile_height)
{
    const SDL_Surface_Ptr temp = create_tile_surface();
    assert( temp );
    SDL_Texture_Ptr tex ( SDL_CreateTexture(renderer, temp->format->format, SDL_TEXTUREACCESS_TARGET,
                                            tile_width, tile_height) );
    throwErrorIf( !tex, "SDL_CreateTexture failed to create minimap texture" );
    return tex;
}

//resets the touched and drawn properties of each active submap cache
void cata_tiles::prepare_minimap_cache_for_updates()
{
    for(auto &mcp : minimap_cache) {
        mcp.second.touched = false;
        mcp.second.drawn = false;
    }
}

//deletes the mapping of unused submap caches from the main map
//the touched flag prevents deletion
void cata_tiles::clear_unused_minimap_cache()
{
    for(auto it = minimap_cache.begin(); it != minimap_cache.end(); ) {
        if(!it->second.touched) {
            minimap_cache.erase(it++);
        } else {
            it++;
        }
    }
}

//draws individual updates to the submap cache texture
//the render target will be set back to display_buffer after all submaps are updated
void cata_tiles::process_minimap_cache_updates()
{
    SDL_Rect rectangle;
    bool draw_with_dots = false;

    const std::string mode = get_option<std::string>( "PIXEL_MINIMAP_MODE" );
    if( mode == "solid" ) {
        rectangle.w = minimap_tile_size.x;
        rectangle.h = minimap_tile_size.y;
    } else if( mode == "squares" ) {
        rectangle.w = std::max( minimap_tile_size.x - 1, 1 );
        rectangle.h = std::max( minimap_tile_size.y - 1, 1 );
        draw_with_dots = rectangle.w == 1 && rectangle.h == 1;
    } else if( mode == "dots" ) {
        draw_with_dots = true;
    }

    for( auto &mcp : minimap_cache ) {
        if( !mcp.second.update_list.empty() ) {
            printErrorIf( SDL_SetRenderTarget( renderer, mcp.second.minimap_tex.get() ) != 0, "SDL_SetRenderTarget failed" );

            //draw a default dark-colored rectangle over the texture which may have been used previously
            if( !mcp.second.ready ) {
                mcp.second.ready = true;
                printErrorIf( SDL_SetRenderDrawColor( renderer, 0, 0, 0, 255 ) != 0, "SDL_SetRenderDrawColor failed" );
                printErrorIf( SDL_RenderClear( renderer ) != 0, "SDL_RenderClear failed" );
            }

            for( const point &p : mcp.second.update_list ) {
                const pixel &current_pix = mcp.second.minimap_colors[p.y * SEEX + p.x];
                const SDL_Color c = current_pix.getSdlColor();


                printErrorIf( SDL_SetRenderDrawColor( renderer, c.r, c.g, c.b, c.a ) != 0, "SDL_SetRenderDrawColor failed" );

                if( draw_with_dots ) {
                    printErrorIf( SDL_RenderDrawPoint( renderer, p.x * minimap_tile_size.x, p.y * minimap_tile_size.y ) != 0, "SDL_RenderDrawPoint failed" );
                } else {
                    rectangle.x = p.x * minimap_tile_size.x;
                    rectangle.y = p.y * minimap_tile_size.y;

                    printErrorIf( SDL_RenderFillRect( renderer, &rectangle ) != 0, "SDL_RenderFillRect failed" );
                }
            }
            mcp.second.update_list.clear();
        }
    }
}

//finds the correct submap cache and applies the new minimap color blip if it doesn't match the current one
void cata_tiles::update_minimap_cache( const tripoint &loc, pixel &pix )
{
    tripoint current_submap_loc = convert_tripoint_to_abs_submap( loc );
    auto it = minimap_cache.find( current_submap_loc );
    if( it == minimap_cache.end() ) {
        it = minimap_cache.emplace( current_submap_loc, tex_pool ).first;
    }

    it->second.touched = true;

    point offset( loc.x, loc.y );
    ms_to_sm_remain( offset );

    pixel &current_pix = it->second.minimap_colors[offset.y * SEEX + offset.x];
    if( current_pix != pix ) {
        current_pix = pix;
        it->second.update_list.push_back( offset );
    }
}

minimap_submap_cache::minimap_submap_cache( minimap_shared_texture_pool &pool ) : ready( false ), pool( pool )
{
    //set color to force updates on a new submap texture
    minimap_colors.resize( SEEY * SEEX, pixel( -1, -1, -1, -1 ) );
    minimap_tex = pool.request_tex( texture_index );
}

minimap_submap_cache::~minimap_submap_cache()
{
    pool.release_tex( texture_index, std::move( minimap_tex ) );
}

minimap_submap_cache::minimap_submap_cache( minimap_submap_cache && ) = default;

//store the known persistent values used in drawing the minimap
//since modifying the minimap properties requires a restart
void cata_tiles::init_minimap( int destx, int desty, int width, int height )
{
    minimap_prep = true;
    minimap_min.x = 0;
    minimap_min.y = 0;
    minimap_max.x = MAPSIZE * SEEX;
    minimap_max.y = MAPSIZE * SEEY;
    minimap_tiles_range.x = ( MAPSIZE - 2 ) * SEEX;
    minimap_tiles_range.y = ( MAPSIZE - 2 ) * SEEY;
    minimap_tile_size.x = std::max( width / minimap_tiles_range.x, 1 );
    minimap_tile_size.y = std::max( height / minimap_tiles_range.y, 1 );
    //maintain a square "pixel" shape
    if( get_option<bool>( "PIXEL_MINIMAP_RATIO" ) ) {
        int smallest_size = std::min( minimap_tile_size.x, minimap_tile_size.y );
        minimap_tile_size.x = smallest_size;
        minimap_tile_size.y = smallest_size;
    }
    minimap_tiles_limit.x = std::min( width / minimap_tile_size.x, minimap_tiles_range.x );
    minimap_tiles_limit.y = std::min( height / minimap_tile_size.y, minimap_tiles_range.y );
    // Center the drawn area within the total area.
    minimap_drawn_width = minimap_tiles_limit.x * minimap_tile_size.x;
    minimap_drawn_height = minimap_tiles_limit.y * minimap_tile_size.y;
    minimap_border_width = std::max( ( width - minimap_drawn_width ) / 2, 0 );
    minimap_border_height = std::max( ( height - minimap_drawn_height ) / 2, 0 );
    //prepare the minimap clipped area
    minimap_clip_rect.x = destx + minimap_border_width;
    minimap_clip_rect.y = desty + minimap_border_height;
    minimap_clip_rect.w = width - minimap_border_width * 2;
    minimap_clip_rect.h = height - minimap_border_height * 2;

    main_minimap_tex.reset();
    main_minimap_tex = create_minimap_cache_texture( minimap_clip_rect.w, minimap_clip_rect.h);

    previous_submap_view = tripoint( INT_MIN, INT_MIN, INT_MIN );

    //allocate the textures for the texture pool
    for( int i = 0; i < static_cast<int>( tex_pool.texture_pool.size() ); i++ ) {
        tex_pool.texture_pool[i] = create_minimap_cache_texture( minimap_tile_size.x * SEEX,
                                   minimap_tile_size.y * SEEY );
    }
}

//the main call for drawing the pixel minimap to the screen
void cata_tiles::draw_minimap( int destx, int desty, const tripoint &center, int width, int height )
{
    if( !g ) {
        return;
    }

    //set up class variables on the first run
    if( !minimap_prep || minimap_reinit_flag ) {
        minimap_reinit_flag = false;
        minimap_cache.clear();
        tex_pool.texture_pool.clear();
        tex_pool.reinit();
        init_minimap( destx, desty, width, height );
    }

    //invalidate the cache if the game shifted more than one submap in the last update, or if z-level changed
    tripoint current_submap_view = g->m.get_abs_sub();
    tripoint submap_view_diff = current_submap_view - previous_submap_view;
    if( abs( submap_view_diff.x ) > 1 || abs( submap_view_diff.y ) > 1 ||
        abs( submap_view_diff.z ) > 0 ) {
        minimap_cache.clear();
    }
    previous_submap_view = current_submap_view;

    //clear leftover flags for the current draw cycle
    prepare_minimap_cache_for_updates();

    const int start_x = center.x - minimap_tiles_limit.x / 2;
    const int start_y = center.y - minimap_tiles_limit.y / 2;

    auto &ch = g->m.access_cache( center.z );
    //retrieve night vision goggle status once per draw
    auto vision_cache = g->u.get_vision_modes();
    bool nv_goggle = vision_cache[NV_GOGGLES];

    const int brightness = get_option<int>( "PIXEL_MINIMAP_BRIGHTNESS" );
    //check all of exposed submaps (MAPSIZE*MAPSIZE submaps) and apply new color changes to the cache
    for( int y = 0; y < MAPSIZE * SEEY; y++ ) {
        for( int x = 0; x < MAPSIZE * SEEX; x++ ) {
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
                color = cursesColorToSDL( vp->vehicle().part_color( vp->part_index() ) );
            } else if( g->m.has_furn( p ) ) {
                auto &furniture = g->m.furn( p ).obj();
                color = cursesColorToSDL( furniture.color() );
            } else {
                auto &terrain = g->m.ter( p ).obj();
                color = cursesColorToSDL( terrain.color() );
            }
            pixel pix( color );
            //color terrain according to lighting conditions
            if( nv_goggle ) {
                if( lighting == LL_LOW ) {
                    color_pixel_nightvision( pix );
                } else if( lighting != LL_DARK && lighting != LL_BLANK ) {
                    color_pixel_overexposed( pix );
                }
            } else if( lighting == LL_LOW ) {
                color_pixel_grayscale( pix );
            }

            pix.adjust_brightness( brightness );
            //add an individual color update to the cache
            update_minimap_cache( p, pix );
        }
    }

    //update minimap textures
    process_minimap_cache_updates();
    //prepare to copy to intermediate texture
    printErrorIf( SDL_SetRenderTarget( renderer, main_minimap_tex.get() ) != 0, "SDL_SetRenderTarget failed" );

    //attempt to draw the submap cache if any of its tiles are exposed in the minimap area
    //the drawn flag prevents it from being drawn more than once
    SDL_Rect drawrect;
    drawrect.w = SEEX * minimap_tile_size.x;
    drawrect.h = SEEY * minimap_tile_size.y;
    for( int y = 0; y < minimap_tiles_limit.y; y++ ) {
        if( start_y + y < minimap_min.y || start_y + y >= minimap_max.y ) {
            continue;
        }
        for( int x = 0; x < minimap_tiles_limit.x; x++ ) {
            if( start_x + x < minimap_min.x || start_x + x >= minimap_max.x ) {
                continue;
            }
            tripoint p( start_x + x, start_y + y, center.z );
            tripoint current_submap_loc = convert_tripoint_to_abs_submap( p );
            auto it = minimap_cache.find( current_submap_loc );

            //a missing submap cache should be pretty improbable
            if( it == minimap_cache.end() ) {
                continue;
            }
            if( it->second.drawn ) {
                continue;
            }
            it->second.drawn = true;

            //the position of the submap texture has to account for the actual (current) 12x12 tile size
            //the clipping rectangle handles the portions that need to hide
            tripoint drawpoint( ( p.x / SEEX ) * SEEX - start_x, ( p.y / SEEY ) * SEEY - start_y, p.z );
            drawrect.x = drawpoint.x * minimap_tile_size.x;
            drawrect.y = drawpoint.y * minimap_tile_size.y;
            printErrorIf( SDL_RenderCopy( renderer, it->second.minimap_tex.get(), NULL, &drawrect ) != 0, "SDL_RenderCopy failed" );
        }
    }
    //set display buffer to main screen
    set_displaybuffer_rendertarget();
    //paint intermediate texture to screen
    printErrorIf( SDL_RenderCopy( renderer, main_minimap_tex.get(), NULL, &minimap_clip_rect ) != 0, "SDL_RenderCopy failed" );

    //unused submap caches get deleted
    clear_unused_minimap_cache();

    //handles the enemy faction red highlights
    //this value should be divisible by 200
    const int indicator_length = get_option<int>( "PIXEL_MINIMAP_BLINK" ) * 200; //default is 2000 ms, 2 seconds

    int flicker = 100;
    int mixture = 0;

    if( indicator_length > 0 ) {
        const float t = get_animation_phase( 2 * indicator_length );
        const float s = std::sin( 2 * M_PI * t );

        flicker = lerp_clamped( 25, 100, std::abs( s ) );
        mixture = lerp_clamped( 0, 100, std::max( s, 0.0f ) );
    }

    // Now draw critters over terrain.
    for( int y = 0; y < minimap_tiles_limit.y; y++ ) {
        if( start_y + y < minimap_min.y || start_y + y >= minimap_max.y ) {
            continue;
        }
        for( int x = 0; x < minimap_tiles_limit.x; x++ ) {
            if( start_x + x < minimap_min.x || start_x + x >= minimap_max.x ) {
                continue;
            }
            tripoint p( start_x + x, start_y + y, center.z );

            lit_level lighting = ch.visibility_cache[p.x][p.y];

            if( lighting != LL_DARK && lighting != LL_BLANK ) {
                Creature *critter = g->critter_at( p, true );
                if( critter != nullptr ) {
                    // use player::sees, otherwise shady zombies or worms will be visible early
                    if( critter == &( g->u ) || g->u.sees( *critter ) ) {
                        SDL_Color c = cursesColorToSDL( critter->symbol_color() );
                        c.a = 255;
                        if( indicator_length > 0 ) {
                            const auto m = dynamic_cast<monster *>( critter );
                            if( m != nullptr ) {
                                //faction status (attacking or tracking) determines if red highlights get applied to creature
                                monster_attitude matt = m->attitude( &( g->u ) );
                                if( MATT_ATTACK == matt || MATT_FOLLOW == matt ) {
                                    const pixel red_pixel( 0xFF, 0x0, 0x0 );

                                    pixel p( c );

                                    p.mix_with( red_pixel, mixture );
                                    p.adjust_brightness( flicker );

                                    c = p.getSdlColor();
                                }
                            }
                        }
                        draw_rhombus(
                            destx + minimap_border_width + x * minimap_tile_size.x,
                            desty + minimap_border_height + y * minimap_tile_size.y,
                            minimap_tile_size.x,
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

void cata_tiles::get_window_tile_counts(const int width, const int height, int &columns, int &rows) const
{
    columns = tile_iso ? ceil((double) width / tile_width ) * 2 + 4 : ceil((double) width / tile_width );
    rows = tile_iso ? ceil((double) height / ( tile_width / 2 - 1 ) ) * 2 + 4 : ceil((double) height / tile_height);
}

bool cata_tiles::draw_from_id_string( std::string id, tripoint pos, int subtile, int rota,
                                      lit_level ll, bool apply_night_vision_goggles )
{
    int nullint = 0;
    return cata_tiles::draw_from_id_string( std::move( id ), C_NONE, empty_string, pos, subtile, rota,
                                            ll, apply_night_vision_goggles, nullint );
}

bool cata_tiles::draw_from_id_string( std::string id, TILE_CATEGORY category,
                                      const std::string &subcategory, tripoint pos,
                                      int subtile, int rota, lit_level ll,
                                      bool apply_night_vision_goggles )
{
    int nullint = 0;
    return cata_tiles::draw_from_id_string( id, category, subcategory, pos, subtile, rota,
                                            ll, apply_night_vision_goggles, nullint );
}

bool cata_tiles::draw_from_id_string( std::string id, tripoint pos, int subtile, int rota,
                                      lit_level ll, bool apply_night_vision_goggles, int &height_3d )
{
    return cata_tiles::draw_from_id_string( std::move( id ), C_NONE, empty_string, pos, subtile, rota,
                                            ll, apply_night_vision_goggles, height_3d );
}

const tile_type *cata_tiles::find_furniture_looks_like( std::string id, std::string &effective_id )
{
    std::string looks_like = id;
    int cnt = 0;
    while( !looks_like.empty() && cnt < 10 ) {
        const tile_type *lltt = find_tile_with_season( looks_like );
        if( lltt ) {
            effective_id = looks_like;
            return lltt;
        }
        const furn_str_id fid( looks_like );
        if( !fid.is_valid() ) {
            return nullptr;
        }
        const furn_t &furn = fid.obj();
        looks_like = furn.looks_like;
        cnt += 1;
    }
    return nullptr;
}

const tile_type *cata_tiles::find_terrain_looks_like( std::string id, std::string &effective_id )
{
    std::string looks_like = id;
    int cnt = 0;
    while( !looks_like.empty() && cnt < 10 ) {
        const tile_type *lltt = find_tile_with_season( looks_like );
        if( lltt ) {
            effective_id = looks_like;
            return lltt;
        }
        const ter_str_id tid( looks_like );
        if( !tid.is_valid() ) {
            return nullptr;
        }
        const ter_t &ter = tid.obj();
        looks_like = ter.looks_like;
        cnt += 1;
    }
    return nullptr;
}

const tile_type *cata_tiles::find_monster_looks_like( std::string id, std::string &effective_id )
{
    std::string looks_like = id;
    int cnt = 0;
    while( !looks_like.empty() && cnt < 10 ) {
        const tile_type *lltt = find_tile_with_season( looks_like );
        if( lltt ) {
            effective_id = looks_like;
            return lltt;
        }
        const mtype_id mid( looks_like );
        if( !mid.is_valid() ) {
            return nullptr;
        }
        const mtype &mt = mid.obj();
        looks_like = mt.looks_like;
        cnt += 1;
    }
    return nullptr;
}

const tile_type *cata_tiles::find_vpart_looks_like( std::string id, std::string &effective_id )
{
    std::string looks_like = id;
    int cnt = 0;
    while( !looks_like.empty() && cnt < 10 ) {
        const tile_type *lltt = find_tile_with_season( "vp_" + looks_like );
        if( lltt ) {
            effective_id = "vp_" + looks_like;
            return lltt;
        }
        const vpart_id new_vpid( looks_like );
        if( !new_vpid.is_valid() ) {
            return nullptr;
        }
        const vpart_info &new_vpi = new_vpid.obj();
        looks_like = new_vpi.looks_like;
        cnt += 1;
    }
    return nullptr;
}

const tile_type *cata_tiles::find_item_looks_like( std::string id, std::string &effective_id )
{
    std::string looks_like = id;
    int cnt = 0;
    while( !looks_like.empty() && cnt < 10 ) {
        const tile_type *lltt = find_tile_with_season( looks_like );
        if( lltt ) {
            effective_id = looks_like;
            return lltt;
        }
        if( !item::type_is_defined( looks_like ) ) {
            return nullptr;
        }
        const itype *new_it = item::find_type( looks_like );
        looks_like = new_it->looks_like;
        cnt += 1;
    }
    return nullptr;
}

const tile_type *cata_tiles::find_tile_with_season( std::string id )
{
    constexpr size_t suffix_len = 15;
    constexpr char season_suffix[4][suffix_len] = {
        "_season_spring", "_season_summer", "_season_autumn", "_season_winter"
    };

    std::string seasonal_id = id + season_suffix[season_of_year( calendar::turn )];

    const tile_type *tt = tileset_ptr->find_tile_type( seasonal_id );
    if( tt ) {
        id = std::move( seasonal_id );
    } else {
        tt = tileset_ptr->find_tile_type( id );
    }
    return tt;
}

bool cata_tiles::draw_from_id_string( std::string id, TILE_CATEGORY category,
                                      const std::string &subcategory, tripoint pos,
                                      int subtile, int rota, lit_level ll,
                                      bool apply_night_vision_goggles, int &height_3d )
{
    // If the ID string does not produce a drawable tile
    // it will revert to the "unknown" tile.
    // The "unknown" tile is one that is highly visible so you kinda can't miss it :D

    // check to make sure that we are drawing within a valid area
    // [0->width|height / tile_width|height]

    if( !tile_iso &&
        ( pos.x - o_x < 0 || pos.x - o_x >= screentile_width ||
          pos.y - o_y < 0 || pos.y - o_y >= screentile_height ) ) {
        return false;
    }

    std::string effective_id;
    const tile_type *tt = find_tile_with_season( id );

    if( !tt ) {
        uint32_t sym = UNKNOWN_UNICODE;
        nc_color col = c_white;
        if( category == C_FURNITURE ) {
            tt = find_furniture_looks_like( id, effective_id );
            if( !tt ) {
                const furn_str_id fid( id );
                if( fid.is_valid() ) {
                    const furn_t &f = fid.obj();
                    sym = f.symbol();
                    col = f.color();
                }
            }
        } else if( category == C_TERRAIN ) {
            tt = find_terrain_looks_like( id, effective_id );
            if( !tt ) {
                const ter_str_id tid( id );
                if( tid.is_valid() ) {
                    const ter_t &t = tid.obj();
                    sym = t.symbol();
                    col = t.color();
                }
            }
        } else if( category == C_MONSTER ) {
            tt = find_monster_looks_like( id, effective_id );
            if( !tt ) {
                const mtype_id mid( id );
                if( mid.is_valid() ) {
                    const mtype &mt = mid.obj();
                    int len = mt.sym.length();
                    const char *s = mt.sym.c_str();
                    sym = UTF8_getch( &s, &len );
                    col = mt.color;
                }
            }
        } else if( category == C_VEHICLE_PART ) {
            tt = find_vpart_looks_like( id.substr( 3 ), effective_id );
            if( !tt ) {
                const vpart_id vpid( id.substr( 3 ) );
                if( vpid.is_valid() ) {
                    const vpart_info &v = vpid.obj();
                    sym = v.sym;
                    if( !subcategory.empty() ) {
                        sym = special_symbol( subcategory[0] );
                        rota = 0;
                        subtile = -1;
                    }
                    col = v.color;
                }
            }
        } else if( category == C_FIELD ) {
            const field_id fid = field_from_ident( id );
            sym = fieldlist[fid].sym;
            // TODO: field density?
            col = fieldlist[fid].color[0];
        } else if( category == C_TRAP ) {
            const trap_str_id tmp( id );
            if( tmp.is_valid() ) {
                const trap &t = tmp.obj();
                sym = t.sym;
                col = t.color;
            }
        } else if( category == C_ITEM ) {
            tt = find_item_looks_like( id, effective_id );
            if( !tt ) {
                const auto tmp = item( id, 0 );
                sym = tmp.symbol().empty() ? ' ' : tmp.symbol().front();
                col = tmp.color();
            }
        }
        // Special cases for walls
        switch(sym) {
            case LINE_XOXO: sym = LINE_XOXO_C; break;
            case LINE_OXOX: sym = LINE_OXOX_C; break;
            case LINE_XXOO: sym = LINE_XXOO_C; break;
            case LINE_OXXO: sym = LINE_OXXO_C; break;
            case LINE_OOXX: sym = LINE_OOXX_C; break;
            case LINE_XOOX: sym = LINE_XOOX_C; break;
            case LINE_XXXO: sym = LINE_XXXO_C; break;
            case LINE_XXOX: sym = LINE_XXOX_C; break;
            case LINE_XOXX: sym = LINE_XOXX_C; break;
            case LINE_OXXX: sym = LINE_OXXX_C; break;
            case LINE_XXXX: sym = LINE_XXXX_C; break;
            default: break; // sym goes unchanged
        }
        if( sym != 0 && sym < 256 ) {
            // see cursesport.cpp, function wattron
            const int pairNumber = col.to_color_pair_index();
            const cata_cursesport::pairs &colorpair = cata_cursesport::colorpairs[pairNumber];
            // What about isBlink?
            const bool isBold = col.is_bold();
            const int FG = colorpair.FG + (isBold ? 8 : 0);
            std::string generic_id = get_ascii_tile_id( sym, FG, -1 );
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
        if(!category_id.empty() && !subcategory.empty()) {
            tt = tileset_ptr->find_tile_type( "unknown_" + category_id + "_" + subcategory );
        }
    }

    // if at this point we have no tile, try just the category
    if( !tt ) {
        const std::string &category_id = TILE_CATEGORY_IDS[category];
        if(!category_id.empty()) {
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

    if( effective_id.empty() ) {
        effective_id = id;
    }
    const tile_type &display_tile = *tt;
    // check to see if the display_tile is multitile, and if so if it has the key related to subtile
    if (subtile != -1 && display_tile.multitile) {
        auto const &display_subtiles = display_tile.available_subtiles;
        auto const end = std::end(display_subtiles);
        if (std::find(begin(display_subtiles), end, multitile_keys[subtile]) != end) {
            // append subtile name to tile and re-find display_tile
            return draw_from_id_string(
                std::move( effective_id.append( "_", 1 ).append( multitile_keys[subtile] ) ),
                pos, -1, rota, ll, apply_night_vision_goggles, height_3d );
        }
    }

    // make sure we aren't going to rotate the tile if it shouldn't be rotated
    if (!display_tile.rotates) {
        rota = 0;
    }

    // translate from player-relative to screen relative tile position
    const point screen_pos = player_to_screen( pos.x, pos.y );

    // seed the PRNG to get a reproducible random int
    // TODO faster solution here
    unsigned int seed = 0;
    // TODO determine ways other than category to differentiate more types of sprites
    switch( category ) {
        case C_TERRAIN:
        case C_FIELD:
        case C_LIGHTING:
            // stationary map tiles, seed based on map coordinates
            seed = g->m.getabs( pos ).x + g->m.getabs( pos ).y * 65536;
            break;
        case C_VEHICLE_PART:
            // vehicle parts, seed based on coordinates within the vehicle
            // TODO also use some vehicle id, for less predictability
        {
            // new scope for variable declarations
            const optional_vpart_position vp = g->m.veh_at( pos );
            vehicle_part &part = vp->vehicle().parts[vp->part_index()];
            seed = part.mount.x + part.mount.y * 65536;
        }
        break;
        case C_ITEM:
        case C_FURNITURE:
        case C_TRAP:
        case C_NONE:
        case C_BULLET:
        case C_HIT_ENTITY:
        case C_WEATHER:
            // TODO come up with ways to make random sprites consistent for these types
            break;
        case C_MONSTER:
            // FIXME add persistent id to Creature type, instead of using monster pointer address
            seed = reinterpret_cast<uintptr_t>( g->critter_at<monster>( pos ) );
            break;
        default:
            // player
            if( id.substr(7) == "player_" ) {
                seed = g->u.name[0];
                break;
            }
            // NPC
            if( id.substr(4) == "npc_" ) {
                if( npc * const guy = g->critter_at<npc>( pos ) ) {
                    seed = guy->getID();
                    break;
                }
            }
    }

    unsigned int loc_rand = 0;
    // only bother mixing up a hash/random value if the tile has some sprites to randomly pick between
    if(display_tile.fg.size()>1 || display_tile.bg.size()>1) {
        static const auto rot32 = []( const unsigned int x, const int k ) {
            return ( x << k ) | ( x >> ( 32 - k ) );
        };
        // use a fair mix function to turn the "random" seed into a random int
        // taken from public domain code at http://burtleburtle.net/bob/c/lookup3.c 2015/12/11
        unsigned int a = seed, b = -seed, c = seed*seed;
        c ^= b; c -= rot32(b,14);
        a ^= c; a -= rot32(c,11);
        b ^= a; b -= rot32(a,25);
        c ^= b; c -= rot32(b,16);
        a ^= c; a -= rot32(c, 4);
        b ^= a; b -= rot32(a,14);
        c ^= b; c -= rot32(b,24);
        loc_rand = c;
    }

    //draw it!
    draw_tile_at( display_tile, screen_pos.x, screen_pos.y, loc_rand, rota, ll, apply_night_vision_goggles, height_3d );

    return true;
}

bool cata_tiles::draw_sprite_at( const tile_type &tile, const weighted_int_list<std::vector<int>> &svlist,
                                 int x, int y, unsigned int loc_rand, bool rota_fg, int rota, lit_level ll,
                                 bool apply_night_vision_goggles )
{
    int nullint = 0;
    return cata_tiles::draw_sprite_at( tile, svlist, x, y, loc_rand, rota_fg, rota, ll,
                                       apply_night_vision_goggles, nullint );
}

bool cata_tiles::draw_sprite_at( const tile_type &tile, const weighted_int_list<std::vector<int>> &svlist,
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
        rotate_sprite = !tile_iso;
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
    if( apply_night_vision_goggles ) {
        if(ll != LL_LOW){
            if( const auto ptr = tileset_ptr->get_overexposed_tile( spritelist[sprite_num] ) ) {
                sprite_tex = ptr;
            }
        } else {
            if( const auto ptr = tileset_ptr->get_night_tile( spritelist[sprite_num] ) ) {
                sprite_tex = ptr;
            }
        }
    } else if( ll == LL_LOW  ) {
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

    if ( rotate_sprite ) {
        switch ( rota ) {
            default:
            case 0: // unrotated (and 180, with just two sprites)
                ret = sprite_tex->render_copy_ex( renderer, &destination,
                    0, NULL, SDL_FLIP_NONE );
                break;
            case 1: // 90 degrees (and 270, with just two sprites)
#if (defined _WIN32 || defined WINDOWS)
                destination.y -= 1;
#endif
                ret = sprite_tex->render_copy_ex( renderer, &destination,
                    -90, NULL, SDL_FLIP_NONE );
                break;
            case 2: // 180 degrees, implemented with flips instead of rotation
                ret = sprite_tex->render_copy_ex( renderer, &destination,
                    0, NULL, static_cast<SDL_RendererFlip>( SDL_FLIP_HORIZONTAL | SDL_FLIP_VERTICAL ) );
                break;
            case 3: // 270 degrees
#if (defined _WIN32 || defined WINDOWS)
                destination.x -= 1;
#endif
                ret = sprite_tex->render_copy_ex( renderer, &destination,
                    90, NULL, SDL_FLIP_NONE );
                break;
        }
    } else { // don't rotate, same as case 0 above
        ret = sprite_tex->render_copy_ex( renderer, &destination,
            0, NULL, SDL_FLIP_NONE );
    }

    printErrorIf( ret != 0, "SDL_RenderCopyEx() failed" );
    // this reference passes all the way back up the call chain back to
    // cata_tiles::draw() std::vector<tile_render_info> draw_points[].height_3d
    // where we are accumulating the height of every sprite stacked up in a tile
    height_3d += tile.height_3d;
    return true;
}

bool cata_tiles::draw_tile_at( const tile_type &tile, int x, int y, unsigned int loc_rand, int rota,
                               lit_level ll, bool apply_night_vision_goggles, int &height_3d )
{
    draw_sprite_at( tile, tile.bg, x, y, loc_rand, false, rota, ll, apply_night_vision_goggles );
    draw_sprite_at( tile, tile.fg, x, y, loc_rand, true, rota, ll, apply_night_vision_goggles, height_3d );
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

    tripoint pbelow = tripoint( p.x, p.y, p.z - 1 );
    SDL_Color tercol = cursesColorToSDL( c_dark_gray );

    const ter_t &curr_ter = g->m.ter( pbelow ).obj();
    const furn_t &curr_furn = g->m.furn( pbelow ).obj();
    int part_below;
    int sizefactor = 2;
    const vehicle *veh;
    //        const vehicle *veh;
    if( curr_furn.has_flag( TFLAG_SEEN_FROM_ABOVE ) ) {
        tercol = cursesColorToSDL( curr_furn.color() );
    } else if( curr_furn.movecost < 0 ) {
        tercol = cursesColorToSDL( curr_furn.color() );
    } else if( ( veh = g->m.veh_at_internal( pbelow, part_below ) ) != nullptr ) {
        const int roof = veh->roof_at_part( part_below );
        const auto vpobst = vpart_position( const_cast<vehicle&>( *veh ), part_below ).obstacle_at_part();
        tercol = cursesColorToSDL( ( roof >= 0 ||
                                     vpobst ) ? c_light_gray : c_magenta );
        sizefactor = ( roof >= 0 || vpobst ) ? 4 : 2;
    } else if( curr_ter.has_flag( TFLAG_SEEN_FROM_ABOVE ) || curr_ter.movecost == 0 ) {
        tercol = cursesColorToSDL( curr_ter.color() );
    } else if( !curr_ter.has_flag( TFLAG_NO_FLOOR ) ) {
        sizefactor = 4;
        tercol = cursesColorToSDL( curr_ter.color() );
    } else {
        tercol = cursesColorToSDL( curr_ter.color() );
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
        screen_x = ( ( pbelow.x - o_x ) - ( o_y - pbelow.y ) + screentile_width - 2 ) * tile_width / 2 +
                   op_x;
        // y uses tile_width because width is definitive for iso tiles
        // tile footprints are half as tall as wide, arbitrarily tall
        screen_y = ( ( pbelow.y - o_y ) - ( pbelow.x - o_x ) - 4 ) * tile_width / 4 +
                   screentile_height * tile_height / 2 + // TODO: more obvious centering math
                   op_y;
    } else {
        screen_x = ( pbelow.x - o_x ) * tile_width + op_x;
        screen_y = ( pbelow.y - o_y ) * tile_height + op_y;
    }
    belowRect.x = screen_x + ( tile_width - belowRect.w ) / 2;
    belowRect.y = screen_y + ( tile_height - belowRect.h ) / 2;
    if( tile_iso ) {
        belowRect.y += tile_height / 8;
    }
    printErrorIf( SDL_SetRenderDrawColor( renderer, tercol.r, tercol.g, tercol.b, 255 ) != 0, "SDL_SetRenderDrawColor failed" );
    printErrorIf( SDL_RenderFillRect( renderer, &belowRect ) != 0, "SDL_RenderFillRect failed" );

    return true;
}

bool cata_tiles::draw_terrain( const tripoint &p, lit_level ll, int &height_3d )
{
    const ter_id t = g->m.ter( p ); // get the ter_id value at this point
    // check for null, if null return false
    if (t == t_null) {
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

    const std::string& tname = t.obj().id.str();

    return draw_from_id_string( tname, C_TERRAIN, empty_string, p, subtile, rotation, ll,
                                nv_goggles_activated, height_3d );
}

bool cata_tiles::draw_furniture( const tripoint &p, lit_level ll, int &height_3d )
{
    // get furniture ID at x,y
    bool has_furn = g->m.has_furn( p );
    if (!has_furn) {
        return false;
    }

    const furn_id f_id = g->m.furn( p );

    // for rotation information
    const int neighborhood[4] = {
        static_cast<int> (g->m.furn( tripoint( p.x, p.y + 1, p.z ))), // south
        static_cast<int> (g->m.furn( tripoint( p.x + 1, p.y, p.z ))), // east
        static_cast<int> (g->m.furn( tripoint( p.x - 1, p.y, p.z ))), // west
        static_cast<int> (g->m.furn( tripoint( p.x, p.y - 1, p.z ))) // north
    };

    int subtile = 0;
    int rotation = 0;
    get_tile_values(f_id, neighborhood, subtile, rotation);

    // get the name of this furniture piece
    const std::string& f_name = f_id.obj().id.str();
    bool ret = draw_from_id_string( f_name, C_FURNITURE, empty_string, p, subtile, rotation, ll,
                                    nv_goggles_activated, height_3d );
    if( ret && g->m.sees_some_items( p, g->u ) ) {
        draw_item_highlight( p );
    }
    return ret;
}

bool cata_tiles::draw_trap( const tripoint &p, lit_level ll, int &height_3d )
{
    const trap &tr = g->m.tr_at( p );
    if( !tr.can_see( p, g->u) ) {
        return false;
    }

    const int neighborhood[4] = {
        static_cast<int> (g->m.tr_at( tripoint( p.x, p.y + 1, p.z ) ).loadid), // south
        static_cast<int> (g->m.tr_at( tripoint( p.x + 1, p.y, p.z ) ).loadid), // east
        static_cast<int> (g->m.tr_at( tripoint( p.x - 1, p.y, p.z ) ).loadid), // west
        static_cast<int> (g->m.tr_at( tripoint( p.x, p.y - 1, p.z ) ).loadid) // north
    };

    int subtile = 0;
    int rotation = 0;
    get_tile_values(tr.loadid, neighborhood, subtile, rotation);

    return draw_from_id_string( tr.id.str(), C_TRAP, empty_string, p, subtile, rotation, ll,
                               nv_goggles_activated, height_3d );
}

bool cata_tiles::draw_field_or_item( const tripoint &p, lit_level ll, int &height_3d )
{
    // check for field
    const field &f = g->m.field_at( p );
    field_id f_id = f.fieldSymbol();
    bool is_draw_field;
    bool do_item;
    switch(f_id) {
        case fd_null:
            //only draw items
            is_draw_field = false;
            do_item = true;
            break;
        case fd_blood:
        case fd_blood_veggy:
        case fd_blood_insect:
        case fd_blood_invertebrate:
        case fd_gibs_flesh:
        case fd_gibs_veggy:
        case fd_gibs_insect:
        case fd_gibs_invertebrate:
        case fd_bile:
        case fd_slime:
        case fd_acid:
        case fd_sap:
        case fd_sludge:
        case fd_cigsmoke:
        case fd_weedsmoke:
        case fd_cracksmoke:
        case fd_methsmoke:
        case fd_hot_air1:
        case fd_hot_air2:
        case fd_hot_air3:
        case fd_hot_air4:
        case fd_spotlight:
            //need to draw fields and items both
            is_draw_field = true;
            do_item = true;
            break;
        default:
            //only draw fields
            do_item = false;
            is_draw_field = true;
            break;
    }
    bool ret_draw_field = true;
    bool ret_draw_item = true;
    if (is_draw_field) {
        const std::string fd_name = fieldlist[f.fieldSymbol()].id;

        // for rotation information
        const int neighborhood[4] = {
            static_cast<int> (g->m.field_at( tripoint( p.x, p.y + 1, p.z ) ).fieldSymbol()), // south
            static_cast<int> (g->m.field_at( tripoint( p.x + 1, p.y, p.z ) ).fieldSymbol()), // east
            static_cast<int> (g->m.field_at( tripoint( p.x - 1, p.y, p.z ) ).fieldSymbol()), // west
            static_cast<int> (g->m.field_at( tripoint( p.x, p.y - 1, p.z ) ).fieldSymbol()) // north
        };

        int subtile = 0;
        int rotation = 0;
        get_tile_values(f.fieldSymbol(), neighborhood, subtile, rotation);

        ret_draw_field = draw_from_id_string( fd_name, C_FIELD, empty_string, p, subtile, rotation,
                                              ll, nv_goggles_activated );
    }
    if(do_item) {
        if( !g->m.sees_some_items( p, g->u ) ) {
            return false;
        }
        const maptile &cur_maptile = g->m.maptile_at( p );
        // get the last item in the stack, it will be used for display
        const item &displayed_item = cur_maptile.get_uppermost_item();
        // get the item's name, as that is the key used to find it in the map
        const std::string &it_name = displayed_item.typeId();
        const std::string it_category = displayed_item.type->get_item_type_string();
        ret_draw_item = draw_from_id_string( it_name, C_ITEM, it_category, p, 0, 0, ll,
                                             nv_goggles_activated, height_3d );
        if ( ret_draw_item && cur_maptile.get_item_count() > 1 ) {
            draw_item_highlight( p );
        }
    }
    return ret_draw_field && ret_draw_item;
}

bool cata_tiles::draw_vpart_below( const tripoint &p, lit_level /*ll*/, int &/*height_3d*/ )
{
    if( !g->m.need_draw_lower_floor( p ) ) {
        return false;
    }
    tripoint pbelow( p.x, p.y, p.z - 1 );
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
    const int veh_part = vp->part_index();
    // veh_part is the index of the part
    // get a north-east-south-west value instead of east-south-west-north value to use with rotation
    int veh_dir = (veh->face.dir4() + 1) % 4;
    if (veh_dir == 1 || veh_dir == 3) {
        veh_dir = (veh_dir + 2) % 4;
    }

    // Gets the visible part, should work fine once tileset vp_ids are updated to work with the vehicle part json ids
    // get the vpart_id
    char part_mod = 0;
    const vpart_id &vp_id = veh->part_id_string(veh_part, part_mod);
    const char sym = veh->face.dir_symbol(veh->part_sym(veh_part));
    std::string subcategory(1, sym);

    // prefix with vp_ ident
    const std::string vpid = "vp_" + vp_id.str();
    int subtile = 0;
    if (part_mod > 0) {
        switch (part_mod) {
            case 1:
                subtile = open_;
                break;
            case 2:
                subtile = broken;
                break;
        }
    }
    const cata::optional<vpart_reference> cargopart = vp.part_with_feature( "CARGO" );
    bool draw_highlight = cargopart && !veh->get_items( cargopart->part_index() ).empty();
    bool ret = draw_from_id_string( vpid, C_VEHICLE_PART, subcategory, p, subtile, veh_dir,
                                   ll, nv_goggles_activated, height_3d );
    if ( ret && draw_highlight ) {
        draw_item_highlight( p );
    }
    return ret;
}

bool cata_tiles::draw_critter_at( const tripoint &p, lit_level ll, int &height_3d )
{
    const auto critter = g->critter_at( p, true );
    if( critter != nullptr ) {
        return draw_entity( *critter, p, ll, height_3d );
    }
    return false;
}

bool cata_tiles::draw_entity( const Creature &critter, const tripoint &p, lit_level ll, int &height_3d )
{
    if( !g->u.sees( critter ) ) {
        if( g->u.sees_with_infrared( critter ) ) {
            return draw_from_id_string( "infrared_creature", C_NONE, empty_string, p, 0, 0,
                                        LL_LIT, false, height_3d );
        }
        return false;
    }
    const monster *m = dynamic_cast<const monster*>( &critter );
    if( m != nullptr ) {
        const auto ent_name = m->type->id;
        const auto ent_category = C_MONSTER;
        std::string ent_subcategory = empty_string;
        if( !m->type->species.empty() ) {
            ent_subcategory = m->type->species.begin()->str();
        }
        const int subtile = corner;
        return draw_from_id_string(ent_name.str(), ent_category, ent_subcategory, p, subtile,
                                   0, ll, false, height_3d );
    }
    const player *pl = dynamic_cast<const player*>( &critter );
    if( pl != nullptr ) {
        draw_entity_with_overlays( *pl, p, ll, height_3d );
        return true;
    }
    return false;
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
    draw_from_id_string( ent_name, C_NONE, "", p, corner, 0, ll, false, height_3d );

    // next up, draw all the overlays
    std::vector<std::string> overlays = pl.get_overlay_ids();
    for( const std::string &overlay : overlays ) {
        bool exists = true;
        std::string draw_id = pl.male ? "overlay_male_" + overlay : "overlay_female_" + overlay;
        if( !tileset_ptr->find_tile_type( draw_id ) ) {
            draw_id = "overlay_" + overlay;
            if( !tileset_ptr->find_tile_type( draw_id ) ) {
                exists = false;
            }
        }

        // make sure we don't draw an annoying "unknown" tile when we have nothing to draw
        if( exists ) {
            int overlay_height_3d = prev_height_3d;
            draw_from_id_string( draw_id, C_NONE, "", p, corner, 0, ll, false, overlay_height_3d );
            // the tallest height-having overlay is the one that counts
            height_3d = std::max( height_3d, overlay_height_3d );
        }
    }
}

bool cata_tiles::draw_item_highlight( const tripoint &pos )
{
    return draw_from_id_string( ITEM_HIGHLIGHT, C_NONE, empty_string, pos, 0, 0, LL_LIT, false );
}

SDL_Surface_Ptr create_tile_surface( const int w, const int h )
{
    SDL_Surface_Ptr surface;
    #if SDL_BYTEORDER == SDL_BIG_ENDIAN
        surface.reset( SDL_CreateRGBSurface( 0, w, h, 32, 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF ) );
    #else
        surface.reset( SDL_CreateRGBSurface( 0, w, h, 32, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000 ) );
    #endif
    throwErrorIf( !surface, "Failed to create surface" );
    assert( surface );
    return surface;
}

SDL_Surface_Ptr cata_tiles::create_tile_surface()
{
    return ::create_tile_surface(tile_width, tile_height);
}

void tileset_loader::ensure_default_item_highlight()
{
    if( ts.find_tile_type( ITEM_HIGHLIGHT ) ) {
        return;
    }
    const Uint8 highlight_alpha = 127;

    int index = ts.tile_values.size();

    const SDL_Surface_Ptr surface = create_tile_surface( ts.tile_width, ts.tile_height );
    assert( surface );
    throwErrorIf( SDL_FillRect( surface.get(), NULL, SDL_MapRGBA( surface->format, 0, 0, 127, highlight_alpha ) ) != 0, "SDL_FillRect failed" );
    SDL_Texture_Ptr texture( SDL_CreateTextureFromSurface( renderer, surface.get() ) );
    throwErrorIf( !texture, "Failed to create texture for default item highlight" );
    ts.tile_values.emplace_back( std::move( texture ), SDL_Rect{ 0, 0, ts.tile_width, ts.tile_height } );
    ts.tile_ids[ITEM_HIGHLIGHT].fg.add(std::vector<int>({index}),1);
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
    bul_id = std::move(name);
}
void cata_tiles::init_draw_hit( const tripoint &p, std::string name )
{
    do_draw_hit = true;
    hit_pos = p;
    hit_entity_id = std::move(name);
}
void cata_tiles::init_draw_line( const tripoint &p, std::vector<tripoint> trajectory,
                                 std::string name, bool target_line )
{
    do_draw_line = true;
    is_target_line = target_line;
    line_pos = p;
    line_endpoint_id = std::move(name);
    line_trajectory = std::move(trajectory);
}
void cata_tiles::init_draw_weather(weather_printable weather, std::string name)
{
    do_draw_weather = true;
    weather_name = std::move(name);
    anim_weather = std::move(weather);
}
void cata_tiles::init_draw_sct()
{
    do_draw_sct = true;
}
void cata_tiles::init_draw_zones(const tripoint &_start, const tripoint &_end, const tripoint &_offset)
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

        draw_from_id_string( exp_name, { exp_pos.x - i, exp_pos.y - i, exp_pos.z },
                             subtile, rotation++, LL_LIT, nv_goggles_activated );
        draw_from_id_string( exp_name, { exp_pos.x - i, exp_pos.y + i, exp_pos.z },
                             subtile, rotation++, LL_LIT, nv_goggles_activated );
        draw_from_id_string( exp_name, { exp_pos.x + i, exp_pos.y + i, exp_pos.z },
                             subtile, rotation++, LL_LIT, nv_goggles_activated );
        draw_from_id_string( exp_name, { exp_pos.x + i, exp_pos.y - i, exp_pos.z },
                             subtile, rotation++, LL_LIT, nv_goggles_activated );

        subtile = edge;
        for (int j = 1 - i; j < 0 + i; j++) {
            rotation = 0;
            draw_from_id_string( exp_name, { exp_pos.x + j, exp_pos.y - i, exp_pos.z },
                                 subtile, rotation, LL_LIT, nv_goggles_activated);
            draw_from_id_string( exp_name, { exp_pos.x + j, exp_pos.y + i, exp_pos.z },
                                 subtile, rotation, LL_LIT, nv_goggles_activated );

            rotation = 1;
            draw_from_id_string( exp_name, { exp_pos.x - i, exp_pos.y + j, exp_pos.z },
                                 subtile, rotation, LL_LIT, nv_goggles_activated );
            draw_from_id_string( exp_name, { exp_pos.x + i, exp_pos.y + j, exp_pos.z },
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

void cata_tiles::draw_weather_frame()
{

    for( auto weather_iterator = anim_weather.vdrops.begin();
         weather_iterator != anim_weather.vdrops.end(); ++weather_iterator ) {
        // TODO: Z-level awareness if weather ever happens on anything but z-level 0.
        int x = 0;
        int y = 0;
        if( tile_iso ) {
            x = weather_iterator->first;
            y = weather_iterator->second;
        } else {
            // currently in ASCII screen coordinates
            x = weather_iterator->first + o_x;
            y = weather_iterator->second + o_y;
        }
        draw_from_id_string( weather_name, C_WEATHER, empty_string, {x, y, 0}, 0, 0,
                             LL_LIT, nv_goggles_activated );
    }
}

void cata_tiles::draw_sct_frame( std::multimap<point, formatted_text> &overlay_strings )
{
    const bool use_font = get_option<bool>("ANIMATION_SCT_USE_FONT");

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
                const int direction_offset = (-direction_XY( direction ).x + 1) * full_text_length / 2;

                overlay_strings.emplace( 
                    player_to_screen( iDX + direction_offset, iDY ),
                    formatted_text( sText, FG, direction ) );
            } else {
                for (std::string::iterator it = sText.begin(); it != sText.end(); ++it) {
                    const std::string generic_id = get_ascii_tile_id(*it, FG, -1);

                    if (tileset_ptr->find_tile_type(generic_id)) {
                        draw_from_id_string(generic_id, C_NONE, empty_string,
                            { iDX + iOffsetX, iDY + iOffsetY, g->u.pos().z }, 0, 0, LL_LIT, false);
                    }

                    if (tile_iso) {
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
    for( int iY = zone_start.y; iY <= zone_end.y; ++ iY) {
        for( int iX = zone_start.x; iX <= zone_end.x; ++iX ) {
            draw_from_id_string( ITEM_HIGHLIGHT, C_NONE, empty_string,
                                 { iX + zone_offset.x, iY + zone_offset.y, g->u.pos().z },
                                 0, 0, LL_LIT, false);
        }
    }

}
void cata_tiles::draw_footsteps_frame()
{
    static const std::string footstep_tilestring = "footstep";
    for( const auto &footstep : sounds::get_footstep_markers() ) {
        draw_from_id_string( footstep_tilestring, footstep, 0, 0, LL_LIT, false);
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
    if (tid == t_null) {
        subtile = 0;
        rota = 0;
        return;
    }

    // get terrain neighborhood
    const ter_id neighborhood[4] = {
        g->m.ter( tripoint( p.x, p.y + 1, p.z ) ), // south
        g->m.ter( tripoint( p.x + 1, p.y, p.z ) ), // east
        g->m.ter( tripoint( p.x - 1, p.y, p.z ) ), // west
        g->m.ter( tripoint( p.x, p.y - 1, p.z ) ) // north
    };

    bool connects[4];
    char val = 0;
    int num_connects = 0;

    // populate connection information
    for (int i = 0; i < 4; ++i) {
        connects[i] = (neighborhood[i] == tid);

        if (connects[i]) {
            ++num_connects;
            val += 1 << i;
        }
    }

    get_rotation_and_subtile(val, num_connects, rota, subtile);
}
void cata_tiles::get_rotation_and_subtile(const char val, const int num_connects, int &rotation, int &subtile)
{
    switch(num_connects) {
        case 0:
            rotation = 0;
            subtile = unconnected;
            break;
        case 4:
            rotation = 0;
            subtile = center;
            break;
        case 1: // all end pieces
            subtile = end_piece;
            switch(val) {
                case 8:
                    rotation = 2;
                    break;
                case 4:
                    rotation = 3;
                    break;
                case 2:
                    rotation = 1;
                    break;
                case 1:
                    rotation = 0;
                    break;
            }
            break;
        case 2:
            switch(val) {
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
            }
            break;
        case 3: // all t_connections
            subtile = t_connection;
            switch(val) {
                case 14:
                    rotation = 2;
                    break;
                case 11:
                    rotation = 1;
                    break;
                case 7:
                    rotation = 0;
                    break;
                case 13:
                    rotation = 3;
                    break;
            }
            break;
    }
}

void cata_tiles::get_connect_values( const tripoint &p, int &subtile, int &rotation, int connect_group )
{
    const bool connects[4] = {
        g->m.ter( tripoint( p.x, p.y + 1, p.z ) ).obj().connects_to( connect_group ),
        g->m.ter( tripoint( p.x + 1, p.y, p.z ) ).obj().connects_to( connect_group ),
        g->m.ter( tripoint( p.x - 1, p.y, p.z ) ).obj().connects_to( connect_group ),
        g->m.ter( tripoint( p.x, p.y - 1, p.z ) ).obj().connects_to( connect_group )
    };
    char val = 0;
    int num_connects = 0;

    // populate connection information
    for (int i = 0; i < 4; ++i) {
        if (connects[i]) {
            ++num_connects;
            val += 1 << i;
        }
    }
    get_rotation_and_subtile(val, num_connects, rotation, subtile);
}

void cata_tiles::get_tile_values(const int t, const int *tn, int &subtile, int &rotation)
{
    bool connects[4];
    int num_connects = 0;
    char val = 0;
    for (int i = 0; i < 4; ++i) {
        connects[i] = (tn[i] == t);
        if (connects[i]) {
            ++num_connects;
            val += 1 << i;
        }
    }
    get_rotation_and_subtile(val, num_connects, rotation, subtile);
}

void cata_tiles::do_tile_loading_report() {
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
    lr_generic( mtypes.begin(), mtypes.end(), []( const std::vector<mtype>::iterator &m ) {
        return ( *m ).id.str();
    }, "Monsters", "" );
    tile_loading_report( vpart_info::all(), "Vehicle Parts", "vp_" );
    tile_loading_report<trap>(trap::count(), "Traps", "");
    tile_loading_report(fieldlist, num_fields, "Fields", "");

    // needed until DebugLog ostream::flush bugfix lands
    DebugLog( D_INFO, DC_ALL );
}

point cata_tiles::player_to_screen( const int x, const int y ) const
{
    int screen_x = 0;
    int screen_y = 0;
    if (tile_iso) {
        screen_x = ((x - o_x) - (o_y - y) + screentile_width - 2) * tile_width / 2 +
            op_x;
        // y uses tile_width because width is definitive for iso tiles
        // tile footprints are half as tall as wide, arbitrarily tall
        screen_y = ((y - o_y) - (x - o_x) - 4) * tile_width / 4 +
            screentile_height * tile_height / 2 + // TODO: more obvious centering math
            op_y;
    }
    else {
        screen_x = (x - o_x) * tile_width + op_x;
        screen_y = (y - o_y) * tile_height + op_y;
    }
    return {screen_x, screen_y};
}

template<typename Iter, typename Func>
void cata_tiles::lr_generic( Iter begin, Iter end, Func id_func, const std::string &label, const std::string &prefix )
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
void cata_tiles::tile_loading_report(maptype const & tiletypemap, std::string const & label, std::string const & prefix) {
    lr_generic( tiletypemap.begin(), tiletypemap.end(),
                []( decltype(tiletypemap.begin()) const & v ) {
                    // c_str works for std::string and for string_id!
                    return v->first.c_str();
                }, label, prefix );
}

template <typename base_type>
void cata_tiles::tile_loading_report(size_t const count, std::string const & label, std::string const & prefix) {
    lr_generic( static_cast<size_t>( 0 ), count,
                []( const size_t i ) {
                    return int_id<base_type>( i ).id().str();
                }, label, prefix );
}

template <typename arraytype>
void cata_tiles::tile_loading_report(arraytype const & array, int array_length, std::string const & label, std::string const & prefix) {
    const auto begin = &(array[0]);
    lr_generic( begin, begin + array_length,
                []( decltype(begin) const v ) {
                    return v->id;
                }, label, prefix );
}

#endif // SDL_TILES
