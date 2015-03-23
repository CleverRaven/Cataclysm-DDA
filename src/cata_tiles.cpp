#if (defined SDLTILES)
#include "cata_tiles.h"
#include "game.h"
#include "options.h"
#include "mapdata.h"
#include "debug.h"
#include "json.h"
#include "path_info.h"
#include "monstergenerator.h"
#include "item.h"
#include "item_factory.h"
#include "veh_type.h"
#include "filesystem.h"
#include "sounds.h"

#include <algorithm>
#include <fstream>
#include <memory>

#include "SDL2/SDL.h"
#include "SDL2/SDL_ttf.h"
#include "SDL2/SDL_image.h"

#define dbg(x) DebugLog((DebugLevel)(x),D_SDL) << __FILE__ << ":" << __LINE__ << ": "

////////////////////////////////////////////////////////////////////////////////////////////////////
// template specializations inside std are explicitly allowed by the standard
////////////////////////////////////////////////////////////////////////////////////////////////////
namespace std {
template<> struct default_delete<SDL_Surface> {
    void operator()(SDL_Surface* const ptr) const noexcept { SDL_FreeSurface(ptr); }
};
template<> struct default_delete<SDL_Texture> {
    void operator()(SDL_Texture* const ptr) const noexcept { SDL_DestroyTexture(ptr); }
};
} //namespace std

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
enum class light_type : int {
    clear,
    light_normal,
    light_dark,
    boomer_normal,
    boomer_dark,
    hidden = -1,
};

// Make sure to change to_string if this changes!
enum class tile_category : int {
    none,
    vehicle_part,
    terrain,
    item,
    furniture,
    trap,
    field,
    lighting,
    monster,
    bullet,
    hit_entity,
    weather,
    num_tile_categories
};

std::string const& to_string(tile_category const cat)
{
    constexpr int size = static_cast<int>(tile_category::num_tile_categories);
    static std::array<std::string, size> const strings {{
        "",
        "vehicle_part",
        "terrain",
        "item",
        "furniture",
        "trap",
        "field",
        "lighting",
        "monster",
        "bullet",
        "hit_entity",
        "weather",
    }};

    auto const i = static_cast<int>(cat);
    if (i < 0 || i >= size) {
        dbg(D_ERROR) << "bad tile_category value";
        strings[0];
    }

    return strings[i];
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Local functions and types
////////////////////////////////////////////////////////////////////////////////////////////////////
namespace {
std::string const empty_string;
std::string const item_highlight {"highlight_item"};

class ascii_id {
public:
    // see cursesport.cpp, function wattron

    enum : int {
        index_cp = 6,
        index_fg = 7,
        index_bg = 8,
    };

    enum : char { default_value = -1 };

    static void set_fg_color(char const n = default_value) { id_[index_fg] = n; }
    static void set_fg_color(nc_color const col) {
        id_[index_fg] = static_cast<char>(
            colorpairs[(col & A_COLOR) >> 17].FG + (col & A_BOLD ? 8 : 0));
    }

    static void set_bg_color() { id_[index_bg] = default_value; }
    static void set_bg_color(nc_color const col) {
        id_[index_bg] = static_cast<char>(
            colorpairs[(col & A_COLOR) >> 17].BG + (col & A_BLINK ? 8 : 0));
    }

    static void set_codepoint(long const sym) {
        id_[index_cp] = static_cast<char>(sym);
    }

    static std::string const& id() { return id_; }
private:
    static std::string id_;
};

// X is replaced by ascii code (converted to char)
// F is replaced by foreground (converted to char)
// B is replaced by background (converted to char)
std::string ascii_id::id_ {"ASCII_XFB"};

std::string const& to_string(multitile_type const mt)
{
    constexpr int size = static_cast<int>(multitile_type::num_multitile_types);
    static std::array<std::string, size> const strings {{
        "center",
        "corner",
        "edge",
        "t_connection",
        "end_piece",
        "unconnected",
        "open",
        "broken",
    }};

    auto const i = static_cast<int>(mt);
    if (i < 0 || i >= size) {
        dbg(D_ERROR) << "bad multitile_type value";
    }

    return strings[i];
}

multitile_type from_string(std::string const& s)
{
    static std::map<std::string, multitile_type> const map {
        {"center",       multitile_type::center},
        {"corner",       multitile_type::corner},
        {"edge",         multitile_type::edge},
        {"t_connection", multitile_type::t_connection},
        {"end_piece",    multitile_type::end_piece},
        {"unconnected",  multitile_type::unconnected},
        {"open",         multitile_type::open},
        {"broken",       multitile_type::broken},
    };

    auto const it = map.find(s);
    if (it == map.end()) {
        dbg(D_ERROR) << "bad multitile_type string";
        return static_cast<multitile_type>(0);
    }

    return it->second;
}

std::unique_ptr<SDL_Surface> create_tile_surface(int const tile_width, int const tile_height)
{
    constexpr bool big_e = SDL_BYTEORDER == SDL_BIG_ENDIAN;
    enum : Uint32 {
        r_mask = big_e ? 0xFF000000 : 0x000000FF,
        g_mask = big_e ? 0x00FF0000 : 0x0000FF00,
        b_mask = big_e ? 0x0000FF00 : 0x00FF0000,
        a_mask = big_e ? 0x000000FF : 0xFF000000,
    };

    SDL_Surface *const surface =
        SDL_CreateRGBSurface(0, tile_width, tile_height, 32, r_mask, g_mask, b_mask, a_mask);

    if (!surface) {
        dbg( D_ERROR ) << "Failed to create surface: " << SDL_GetError();
    }

    return std::unique_ptr<SDL_Surface> {surface};
}

bool ends_with(std::string const &s, std::string const &ending)
{
    if (ending.size() > s.size()) {
        return false;
    }

    return std::equal(ending.rbegin(), ending.rend(), s.rbegin());
}

//--------------------------------------------------------------------------------------------------
template <typename Container>
tile_type const* file_tile_category(
    Container const &ids, std::string const &id, tile_category const cat, std::string const &sub_cat)
{
    long     sym = -1;
    nc_color col = c_white;

    switch (cat) {
    case tile_category::none :
        break;
    case tile_category::vehicle_part : {
        // TODO reset rotation here.
        auto const it = vehicle_part_types.find(id.substr(3));
        if (it != vehicle_part_types.end()) {
            if (sub_cat.empty()) {
                sym = it->second.sym;
            } else {
                sym = special_symbol(sub_cat[0]);
            }
            col = it->second.color;
        }
    } break;
    case tile_category::terrain : {
        auto const it = termap.find(id);
        if (it != termap.end()) {
            sym = it->second.sym;
            col = it->second.color;
        }
    } break;
    case tile_category::item :
        if (auto const it = item_controller->find_template(id)) {
            sym = it->sym;
            col = it->color;
        }
        break;
    case tile_category::furniture : {
        auto const it = furnmap.find(id);       
        if (it != furnmap.end()) {
            sym = it->second.sym;
            col = it->second.color;
        }
    } break;
    case tile_category::trap : {
        auto const it = trapmap.find(id);
        if (it != trapmap.end()) {
            auto const& trap = *traplist[it->second];
            sym = trap.sym;
            col = trap.color;
        }
    } break;
    case tile_category::field : {
        // TODO: field density?    
        auto const &f = fieldlist[field_from_ident(id)];
        sym = f.sym;
        col = f.color[0];
    } break;
    case tile_category::lighting :
        break;
    case tile_category::monster :
        // TODO clean up has_mtype
        if (MonsterGenerator::generator().has_mtype(id)) {
            const mtype *m = MonsterGenerator::generator().get_mtype(id);
            int len = m->sym.length();
            const char *s = m->sym.c_str();
            sym = UTF8_getch(&s, &len);
            col = m->color;
        }
        break;
    case tile_category::bullet :
        break;
    case tile_category::hit_entity :
        break;
    case tile_category::weather :
        break;
    }

    // Special cases for walls
    switch (sym) {
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
    default: break;
    }

    if (sym < 0 || sym > 255) {
        return nullptr;
    }

    ascii_id::set_codepoint(sym);
    ascii_id::set_fg_color(col);
    ascii_id::set_bg_color();
    
    auto it = ids.find(ascii_id::id());
    if (it != ids.end()) {
        return &it->second;
    }
    
    // Try again without color this time (using default color).
    ascii_id::set_fg_color();
    if ((it = ids.find(ascii_id::id())) != ids.end()) {
        return &it->second;
    }

    return nullptr;
}

//--------------------------------------------------------------------------------------------------
template <typename Container>
tile_type const* find_tile_optional(Container const &ids, std::string const &id)
{
    auto const it = ids.find(id);
    return (it != std::end(ids)) ? &it->second : nullptr;
}

//--------------------------------------------------------------------------------------------------
template <typename Container, typename Iterator = typename Container::const_iterator>
tile_type const* find_tile_fallback(Container const &ids, tile_category const cat, std::string const &sub_cat)
{
    static std::string const unknown {"unknown"};

    // with no category there is nothing more we can do ~> fall back on unknown
    if (cat == tile_category::none) {
        return find_tile_optional(ids, unknown);
    }

    std::string cat_string = unknown + '_' + to_string(cat);

    // try to fallback on category + subcategory first
    if (!sub_cat.empty()) {
        if (auto const result = find_tile_optional(ids, cat_string + '_' + sub_cat)) {
            return result;
        }
    }
    
    // failing that, try just the subcategory
    if (auto const result = find_tile_optional(ids, cat_string)) {
        return result;
    }
   
    // finally, fall back on unknown
    return find_tile_optional(ids, unknown);
}

} //namespace
////////////////////////////////////////////////////////////////////////////////////////////////////
extern int WindowHeight, WindowWidth;
extern int fontwidth, fontheight;

cata_tiles::cata_tiles(SDL_Renderer *const render)
  : renderer(render)
{
}

cata_tiles::~cata_tiles()
{
    clear();
}

void cata_tiles::clear()
{
    for (auto &tex : tile_values) {
        SDL_DestroyTexture(tex);
    }

    tile_values.clear();
    tile_ids.clear();
    seasonal_variations_.clear();
}

void cata_tiles::init(std::string const &load_file_path)
{
    std::string json_path, tileset_path;
    // get path information from load_file_path
    get_tile_information(load_file_path, json_path, tileset_path);
    // send this information to old init to avoid redundant code
    load_tilejson(json_path, tileset_path);
}

void cata_tiles::reinit(std::string const &load_file_path)
{
    clear_buffer();
    clear();
    init(load_file_path);
}

void cata_tiles::get_tile_information(std::string dir_path, std::string &json_path, std::string &tileset_path)
{
    dbg( D_INFO ) << "Attempting to Initialize JSON and TILESET path information from [" << dir_path << "]";
    const std::string filename = "tileset.txt";                 // tileset-information-file
    const std::string default_json = FILENAMES["defaulttilejson"];    // defaults
    const std::string default_tileset = FILENAMES["defaulttilepng"];

    // search for the files (tileset.txt)
    auto files = get_files_from_path(filename, dir_path, true);

    for(std::vector<std::string>::iterator it = files.begin(); it != files.end(); ++it) {   // iterate through every file found
        std::ifstream fin;
        fin.open(it->c_str());
        if(!fin.is_open()) {
            fin.close();
            dbg( D_ERROR ) << "Could not read " << *it << " -- Setting to default values!";
            json_path = default_json;
            tileset_path = default_tileset;
            return;
        }
        // should only have 2 values inside it, otherwise is going to only load the last 2 values
        while(!fin.eof()) {
            std::string sOption;
            fin >> sOption;

            if(sOption == "") {
                getline(fin, sOption);    // Empty line, chomp it
            } else if(sOption[0] == '#') { // # indicates a comment
                getline(fin, sOption);
            } else {
                if (sOption.find("NAME") != std::string::npos) {
                    std::string tileset_name;
                    tileset_name = "";
                    fin >> tileset_name;
                    if(tileset_name != OPTIONS["TILES"].getValue()) {   // if the current tileset name isn't the same
                        // as the current one in the options break
                        break;                                          // out of the while loop, close the file and
                    }                                                   // continue with the next file
                } else if (sOption.find("VIEW") != std::string::npos) { // we don't need the view name here
                    getline(fin, sOption);                              // so we just skip it
                } else if (sOption.find("JSON") != std::string::npos) {
                    fin >> json_path;
                    json_path = FILENAMES["gfxdir"] + json_path;
                    dbg( D_INFO ) << "JSON path set to [" << json_path << "].";
                } else if (sOption.find("TILESET") != std::string::npos) {
                    fin >> tileset_path;
                    tileset_path = FILENAMES["gfxdir"] + tileset_path;
                    dbg( D_INFO ) << "TILESET path set to [" << tileset_path << "].";
                }
            }
        }

        fin.close();
    }
    if (json_path == "") {
        json_path = default_json;
        dbg( D_INFO ) << "JSON set to default [" << json_path << "].";
    }
    if (tileset_path == "") {
        tileset_path = default_tileset;
        dbg( D_INFO ) << "TILESET set to default [" << tileset_path << "].";
    }
}

int cata_tiles::load_tileset(std::string const &path, int const R, int const G, int const B)
{
    std::string img_path = path;
#ifdef PREFIX   // use the PREFIX path over the current directory
    img_path = (FILENAMES["datadir"] + "/" + img_path);
#endif
    // reinit tile_atlas
    std::unique_ptr<SDL_Surface> tile_atlas {IMG_Load(img_path.c_str())};
    if(!tile_atlas) {
        throw std::string("Could not load tileset image at ") + img_path + ", error: " + IMG_GetError();
    }

    // sx and sy will take care of any extraneous pixels that do not add up to a full tile
    int const sx = tile_atlas->w - (tile_atlas->w % tile_width_);
    int const sy = tile_atlas->h - (tile_atlas->h % tile_height_);

    // Set up initial source and destination information. Destination is going to be unchanging
    SDL_Rect src_rect = {0, 0, tile_width_, tile_height_};
    SDL_Rect dst_rect = {0, 0, tile_width_, tile_height_};

    auto const key_r = static_cast<Uint8>(R & 0xFF);
    auto const key_g = static_cast<Uint8>(G & 0xFF);
    auto const key_b = static_cast<Uint8>(B & 0xFF);
    bool const use_color_key = (R == key_r) && (G == key_g) && (B == key_b);

    /** split the atlas into tiles using SDL_Rect structs instead of slicing the atlas into individual surfaces */
    size_t const initial_count = tile_values.size();
    for (int y = 0; y < sy; y += tile_height_) {
        for (int x = 0; x < sx; x += tile_width_) {
            src_rect.x = x;
            src_rect.y = y;

            auto tile_surf = create_tile_surface(tile_width_, tile_height_);
            if (!tile_surf) {
                continue;
            }

            if (SDL_BlitSurface(tile_atlas.get(), &src_rect, tile_surf.get(), &dst_rect)) {
                dbg( D_ERROR ) << "SDL_BlitSurface failed: " << SDL_GetError();
            }

            if (use_color_key) {
                SDL_SetColorKey(tile_surf.get(), SDL_TRUE,
                    SDL_MapRGB(tile_surf->format, key_r, key_g, key_b));
                SDL_SetSurfaceRLE(tile_surf.get(), true);
            }

            std::unique_ptr<SDL_Texture> tile_tex {
                SDL_CreateTextureFromSurface(renderer, tile_surf.get())};

            if (!tile_tex) {
                dbg( D_ERROR) << "failed to create texture: " << SDL_GetError();
                continue;
            }

            tile_values.push_back(tile_tex.get());
            tile_tex.release();
        }
    }

    auto const tilecount = static_cast<int>(tile_values.size() - initial_count);
    dbg( D_INFO ) << "Tiles Created: " << tilecount;
    return tilecount;
}

void cata_tiles::set_draw_scale(int const scale)
{
    tile_width_  = (default_tile_width  * scale) / 16;
    tile_height_ = (default_tile_height * scale) / 16;

    tile_ratiox = (float)tile_width_  / (float)fontwidth;
    tile_ratioy = (float)tile_height_ / (float)fontheight;
}

void cata_tiles::load_tilejson(std::string const &path, const std::string &image_path)
{
    dbg( D_INFO ) << "Attempting to Load JSON file " << path;
    std::ifstream config_file(path, std::ifstream::in | std::ifstream::binary);

    if (!config_file.good()) {
        throw std::string("failed to open tile info json: ") + path;
    }

    load_tilejson_from_file( config_file, image_path );
    if (!tile_ids.count("unknown")) {
        debugmsg("The tileset you're using has no 'unknown' tile defined!");
    }
}

void cata_tiles::load_tilejson_from_file(std::ifstream &f, const std::string &image_path)
{
    JsonIn config_json(f);
    // it's all one json object
    JsonObject config = config_json.get_object();

    /** 1) Make sure that the loaded file has the "tile_info" section */
    if (!config.has_member("tile_info")) {
        config.throw_error( "\"tile_info\" missing" );
    }

    JsonArray info = config.get_array("tile_info");
    while (info.has_more()) {
        JsonObject curr_info = info.next_object();
        tile_height_ = curr_info.get_int("height");
        tile_width_  = curr_info.get_int("width");

        default_tile_width  = tile_width_;
        default_tile_height = tile_height_;
    }

    set_draw_scale(16);

    /** 2) Load tile information if available */   
    if (config.has_array("tiles-new")) {
        // new system, several entries
        // When loading multiple tileset images this defines where
        // the tiles from the most recently loaded image start from.
        int offset = 0;
        JsonArray tiles_new = config.get_array("tiles-new");
        while (tiles_new.has_more()) {
            JsonObject tile_part_def = tiles_new.next_object();
            const std::string tileset_image_path = tile_part_def.get_string("file");
            int R = -1;
            int G = -1;
            int B = -1;
            if (tile_part_def.has_object("transparency")) {
                JsonObject tra = tile_part_def.get_object("transparency");
                R = tra.get_int("R");
                G = tra.get_int("G");
                B = tra.get_int("B");
            }
            // First load the tileset image to get the number of available tiles.
            dbg( D_INFO ) << "Attempting to Load Tileset file " << tileset_image_path;
            const int newsize = load_tileset(tileset_image_path, R, G, B);
            // Now load the tile definitions for the loaded tileset image.
            load_tilejson_from_file(tile_part_def, offset, newsize);
            if (tile_part_def.has_member("ascii")) {
                load_ascii_tilejson_from_file(tile_part_def, offset, newsize);
            }
            // Make sure the tile definitions of the next tileset image don't
            // override the current ones.
            offset += newsize;
        }
    } else {
        // old system, no tile file path entry, only one array of tiles
        dbg( D_INFO ) << "Attempting to Load Tileset file " << image_path;
        const int newsize = load_tileset(image_path, -1, -1, -1);
        load_tilejson_from_file(config, 0, newsize);
    }
}

void cata_tiles::add_ascii_subtile(tile_type &curr_tile, const std::string &t_id,
    int fg, const std::string &s_id)
{
    tile_type const& tile = (tile_ids[t_id + "_" + s_id] = tile_type(fg, -1, true));
    multitile_variations_[t_id][from_string(s_id)] = &tile;
}

void cata_tiles::load_ascii_tilejson_from_file(JsonObject &config, int const offset, int const size)
{
    if (!config.has_member("ascii")) {
        config.throw_error( "\"ascii\" section missing" );
    }
    JsonArray ascii = config.get_array("ascii");
    while (ascii.has_more()) {
        JsonObject entry = ascii.next_object();
        load_ascii_set(entry, offset, size);
    }
}

void cata_tiles::load_ascii_set(JsonObject &entry, int const offset, int const size)
{
    // tile for ASCII char 0 is at `in_image_offset`,
    // the other ASCII chars follow from there.
    const int in_image_offset = entry.get_int("offset");
    if (in_image_offset >= size) {
        entry.throw_error("invalid offset (out of range)", "offset");
    }

    // color, of the ASCII char. Can be -1 to indicate all/default colors.
    const std::string scolor = entry.get_string("color", "DEFAULT");
    
    // Add an offset for bold colors (ncrses has this bold attribute,
    // this mimics it). bold does not apply to default color.
    char const bold = entry.get_bool("bold", false) ? 8 : 0;

    ascii_id::set_bg_color();

    if (scolor == "BLACK") {
        ascii_id::set_fg_color(COLOR_BLACK + bold);
    } else if (scolor == "RED") {
        ascii_id::set_fg_color(COLOR_RED + bold);
    } else if (scolor == "GREEN") {
        ascii_id::set_fg_color(COLOR_GREEN + bold);
    } else if (scolor == "YELLOW") {
        ascii_id::set_fg_color(COLOR_YELLOW + bold);
    } else if (scolor == "BLUE") {
        ascii_id::set_fg_color(COLOR_BLUE + bold);
    } else if (scolor == "MAGENTA") {
        ascii_id::set_fg_color(COLOR_MAGENTA + bold);
    } else if (scolor == "CYAN") {
        ascii_id::set_fg_color(COLOR_CYAN + bold);
    } else if (scolor == "WHITE") {
        ascii_id::set_fg_color(COLOR_WHITE + bold);
    } else if (scolor == "DEFAULT") {
        ascii_id::set_fg_color();
    } else {
        entry.throw_error("invalid color for ascii", "color");
    }

    const int base_offset = offset + in_image_offset;

    // Finally load all 256 ascii chars (actually extended ascii)
    for (int ch = 0; ch < 256; ++ch) {
        const int index_in_image = ch + in_image_offset;
        if (index_in_image < 0 || index_in_image >= size) {
            // Out of range is ignored for now.
            continue;
        }

        ascii_id::set_codepoint(ch);
        std::string const& id = ascii_id::id();
        tile_type& curr_tile = (tile_ids[id] = tile_type(index_in_image + offset, 0));

        switch (ch) {
        case LINE_OXOX_C://box bottom/top side (horizontal line)
            curr_tile.fg = 205 + base_offset; break;
        case LINE_XOXO_C://box left/right side (vertical line)
            curr_tile.fg = 186 + base_offset; break;
        case LINE_OXXO_C://box top left
            curr_tile.fg = 201 + base_offset; break;
        case LINE_OOXX_C://box top right
            curr_tile.fg = 187 + base_offset; break;
        case LINE_XOOX_C://box bottom right
            curr_tile.fg = 188 + base_offset; break;
        case LINE_XXOO_C://box bottom left
            curr_tile.fg = 200 + base_offset; break;
        case LINE_XXOX_C://box bottom north T (left, right, up)
            curr_tile.fg = 202 + base_offset; break;
        case LINE_XXXO_C://box bottom east T (up, right, down)
            curr_tile.fg = 208 + base_offset; break;
        case LINE_OXXX_C://box bottom south T (left, right, down)
            curr_tile.fg = 203 + base_offset; break;
        case LINE_XXXX_C://box X (left down up right)
            curr_tile.fg = 206 + base_offset; break;
        case LINE_XOXX_C://box bottom east T (left, down, up)
            curr_tile.fg = 184 + base_offset; break;
        }

        if (ch == LINE_XOXO_C || ch == LINE_OXOX_C) {
            curr_tile.rotates = false;
            curr_tile.multitile = true;
            add_ascii_subtile(curr_tile, id, 206 + base_offset, "center");
            add_ascii_subtile(curr_tile, id, 201 + base_offset, "corner");
            add_ascii_subtile(curr_tile, id, 186 + base_offset, "edge");
            add_ascii_subtile(curr_tile, id, 203 + base_offset, "t_connection");
            add_ascii_subtile(curr_tile, id, 208 + base_offset, "end_piece");
            add_ascii_subtile(curr_tile, id, 219 + base_offset, "unconnected");
        }
    }
}

void cata_tiles::load_tilejson_from_file(JsonObject &config, int const offset, int const size)
{
    if (!config.has_member("tiles")) {
        config.throw_error( "\"tiles\" section missing" );
    }

    JsonArray tiles = config.get_array("tiles");
    while (tiles.has_more()) {
        JsonObject entry = tiles.next_object();

        std::string t_id = entry.get_string("id");
        tile_type &curr_tile = load_tile(entry, t_id, offset, size);

        bool const t_multi = entry.get_bool("multitile", false);
        bool const t_rota =  entry.get_bool("rotates", t_multi);
        if (t_multi) {
            // fetch additional tiles
            JsonArray subentries = entry.get_array("additional_tiles");
            while (subentries.has_more()) {
                JsonObject subentry = subentries.next_object();
                std::string s_id = subentry.get_string("id");
                std::string m_id = t_id + "_" + s_id;
                
                tile_type &curr_subtile = load_tile(subentry, m_id, offset, size);
                curr_subtile.rotates = true;

                auto mit = multitile_variations_.find(t_id);
                if (mit == multitile_variations_.end()) {
                    mit = multitile_variations_.insert(
                        std::make_pair(t_id, multitile_variation_t {})).first;
                }

                mit->second[from_string(s_id)] = &curr_subtile;
            }
        }

        // write the information of the base tile to curr_tile
        curr_tile.multitile = t_multi;
        curr_tile.rotates   = t_rota;
    }
    dbg( D_INFO ) << "Tile Width: " << tile_width_ << " Tile Height: " << tile_height_ << " Tile Definitions: " << tile_ids.size();
}

tile_type& cata_tiles::load_tile(JsonObject &entry, const std::string &id,
    int const offset, int const size)
{
    static std::string const key_fg {"fg"}; 
    static std::string const key_bg {"bg"};

    // -1 indicates an absent fore / background.
    int fg = entry.get_int(key_fg, -1);
    int bg = entry.get_int(key_bg, -1);
    if (fg != -1 && fg < 0 || fg >= size) {
        entry.throw_error("invalid value for fg (out of range)", "fg");
    } else if (fg != -1) {
        fg += offset;
    }

    if (bg != -1 && bg < 0 || bg >= size) {
        entry.throw_error("invalid value for bg (out of range)", "bg");
    } else if (bg != -1) {
        bg += offset;
    }
    
    auto const &result = tile_ids.insert(std::make_pair(id, tile_type(fg, bg)));
    tile_type &tile = result.first->second;

    // if an insertion actually happened, get a pointer to the key (id).
    if (result.second) {
        tile.id = &result.first->first;
    }

    static std::array<std::string, 4> const suffixes {{
        "_season_spring", "_season_summer", "_season_autumn", "_season_winter"
    }};

    int i = 0;
    if (ends_with(id, suffixes[i++]) || ends_with(id, suffixes[i++]) ||
        ends_with(id, suffixes[i++]) || ends_with(id, suffixes[i++]))
    {
        // all season suffixes are the same length
        std::string main_id = id.substr(0, id.size() - 14);
        
        auto sit = seasonal_variations_.find(main_id);
        if (sit == seasonal_variations_.end()) {
            sit = seasonal_variations_.insert(
                std::make_pair(main_id, seasonal_variation_t {})).first;
        }
        
        sit->second[i - 1] = &tile;
        
        auto const it = tile_ids.find(main_id);
        if (it != std::end(tile_ids)) {
            it->second.has_seasonal = true;
        } else {
            dbg(D_ERROR) << "cata_tiles::load_tile no main id for season variation " << id;
        }
    }
   
    return tile;
}

void cata_tiles::draw(int const destx, int const desty, int const centerx, int const centery,
    int const width, int const height)
{
    //set clipping to prevent drawing over stuff we shouldn't
    SDL_Rect const clip_rect = {destx, desty, width, height};
    SDL_RenderSetClipRect(renderer, &clip_rect);

    init_light();

    // Rounding up to include incomplete tiles at the bottom/right edges
    int const sx = (width  + tile_width_  - 1) / tile_width_;
    int const sy = (height + tile_height_ - 1) / tile_height_;

    o_x  = centerx - POSX;
    o_y  = centery - POSY;
    op_x = destx;
    op_y = desty;
    screentile_width  = sx;
    screentile_height = sy;

    for (int my = 0; my < sy; ++my) {
        for (int mx = 0; mx < sx; ++mx) {
            int const x = mx + o_x;
            int const y = my + o_y;

            const auto critter = g->critter_at(x, y);
            
            if (draw_lighting(x, y, light_at(x, y))) {
                if (critter && g->u.sees_with_infrared(*critter)) {
                    draw_from_id_string("infrared_creature", x, y, 0, 0);
                }
                continue;
            }

            // light is no longer being considered, for now.
            // Draw Terrain if possible. If not possible then we need to continue.
            if (!draw_terrain(x, y)) {
                continue;
            }

            draw_furniture(x, y);
            draw_trap(x, y);
            draw_field_or_item(x, y);
            draw_vpart(x, y);
            
            if (critter) {
                draw_entity(*critter, x, y);
            }
        }
    }

    draw_footsteps_frame();

    bool did_animation = false;
    if (do_draw_explosion) {
        did_animation = true;
        draw_explosion_frame();
    }
    if (do_draw_bullet) {
        did_animation = true;
        draw_bullet_frame();
    }
    if (do_draw_hit) {
        did_animation = true;
        draw_hit_frame();
        void_hit();
    }
    if (do_draw_line) {
        did_animation = true;
        draw_line();
        void_line();
    }
    if (do_draw_weather) {
        did_animation = true;
        draw_weather_frame();
        void_weather();
    }
    if (do_draw_sct) {
        did_animation = true;
        draw_sct_frame();
        void_sct();
    }
    if (do_draw_zones) {
        did_animation = true;
        draw_zones_frame();
        void_zones();
    }

    if (!did_animation) {
        // check to see if player is located at ter
        int const tx = g->ter_view_x;
        int const ty = g->ter_view_y;
        if (g->u.posx() + g->u.view_offset_x != tx || g->u.posy() + g->u.view_offset_y != ty) {
            draw_from_id_string("cursor", tx, ty, 0, 0);
        }
    }

    SDL_RenderSetClipRect(renderer, nullptr);
}

void cata_tiles::clear_buffer()
{
    //TODO convert this to use sdltiles ClearScreen() function
    SDL_RenderClear(renderer);
}

bool cata_tiles::draw_from_id_string(std::string const &id, int x, int y, int subtile, int rota)
{
    return cata_tiles::draw_from_id_string(id, tile_category::none, empty_string, x, y, subtile, rota);
}

void cata_tiles::draw_tile_at(tile_type const &tile, int const x, int const y, int const rota)
{
#if (defined _WIN32 || defined WINDOWS)
    constexpr int adjustment = -1;
#else
    constexpr int adjustment = 0;
#endif

    SDL_Rect dest {x, y, tile_width_, tile_height_};
    int const size = static_cast<int>(tile_values.size());

    SDL_Rect  const* const from = nullptr;
    SDL_Rect  const* const to   = &dest;
    SDL_Point const* const c    = nullptr;

    // blit background first : always non-rotated
    if (tile.bg >= 0 && tile.bg < size) {
        if (SDL_RenderCopy(renderer, tile_values[tile.bg], from, to)) {
            dbg( D_ERROR ) << "SDL_RenderCopyEx(bg) failed: " << SDL_GetError();
        }
    }

    int ret = 0;
    if (tile.fg >= 0 && tile.fg < size) {
        auto const& t = tile_values[tile.fg];

        switch (rota) {
        case 0 : ret = SDL_RenderCopy(renderer, t, from, to); break;
        case 1 :
            dest.y += adjustment;
            ret = SDL_RenderCopyEx(renderer, t, from, to, -90.0, c, SDL_FLIP_NONE);
            break;
        case 2 :
            ret = SDL_RenderCopyEx(renderer, t, from, to, 0.0, c,
                static_cast<SDL_RendererFlip>(SDL_FLIP_HORIZONTAL | SDL_FLIP_VERTICAL));
            break;
        case 3 :
            dest.x += adjustment;
            ret = SDL_RenderCopyEx(renderer, t, from, to,  90.0, c, SDL_FLIP_NONE);
            break;
        default :
            ret = -1;
            break;
        }
    }

    if (ret) {
        dbg( D_ERROR ) << "SDL_RenderCopyEx(fg) failed: " << SDL_GetError();
    }
}

bool cata_tiles::draw_from_id_string(std::string const &id, tile_category category,
    const std::string &subcategory, int const x, int const y, int const subtile, int rota)
{
    // If the ID string does not produce a drawable tile it will revert to the "unknown" tile.
    // The "unknown" tile is one that is highly visible so you kinda can't miss it :D
   
    int const x0 = x - o_x;
    int const y0 = y - o_y;
    
    // check to make sure that we are drawing within a valid area
    // [0->width|height / tile_width|height]
    if (x0 < 0 || x0 >= screentile_width || y0 < 0 || y0 >= screentile_height) {
        return false;
    }

    // translate from player-relative to screen relative tile position
    const int screen_x = x0 * tile_width_  + op_x;
    const int screen_y = y0 * tile_height_ + op_y;

    // first, look for the base tile
    auto const it = tile_ids.find(id);
    tile_type const* display_tile = (it != tile_ids.end()) ? &it->second : nullptr;

    // if we fail to find a tile, first search based on category and subcategory
    // finally, try to find a fallback and, while this really shouldn't happen,
    // the tileset creator might have forgotten to define the unknown tile, so fail.
    while (!display_tile) {
        if (display_tile = file_tile_category(tile_ids, id, category, subcategory)) {
            break;
        } else if (display_tile = find_tile_fallback(tile_ids, category, subcategory)) {
            break;
        }

        return false;
    }   

    // then get a seasonal variation if one exists
    if (display_tile->has_seasonal) {
        auto const sit = seasonal_variations_.find(id);
        if (sit != seasonal_variations_.end()) {
            if (auto const ptr = sit->second[calendar::turn.get_season()]) {
                display_tile = ptr;
            }
        }
    }

    // then get the subtile if it's a multitile
    if (display_tile->multitile && subtile >= 0) {
        auto const mit = multitile_variations_.find(*display_tile->id);
        if (mit != multitile_variations_.end()) {
            if (auto const ptr = mit->second[subtile]) {
                display_tile = ptr;
            }
        }
    }

    // if both bg and fg are -1 then return unknown tile
    if (display_tile->bg == -1 && display_tile->fg == -1) {
        if (!(display_tile = find_tile_fallback(tile_ids, tile_category::none, subcategory))) {
            return false;
        }
    }

    // make sure we aren't going to rotate the tile if it shouldn't be rotated and draw it!
    draw_tile_at(*display_tile, screen_x, screen_y, display_tile->rotates ? rota : 0);

    return true;
}

bool cata_tiles::draw_lighting(int const x, int const y, light_type const l)
{
    if (l == light_type::clear) {
        return false;
    }

    std::string light_name;
    switch (l) {
    case light_type::hidden:
        light_name = "lighting_hidden";
        break;
    case light_type::light_normal:
        light_name = "lighting_lowlight_light";
        break;
    case light_type::light_dark:
        light_name = "lighting_lowlight_dark";
        break;
    case light_type::boomer_normal:
        light_name = "lighting_boomered_light";
        break;
    case light_type::boomer_dark:
        light_name = "lighting_boomered_dark";
        break;
    case light_type::clear:
        return false;
    }

    // lighting is never rotated, though, could possibly add in random rotation?
    draw_from_id_string(light_name, tile_category::lighting, empty_string, x, y, 0, 0);

    return true;
}

bool cata_tiles::draw_terrain(int const x, int const y)
{
    int const t = g->m.ter(x, y);
    if (t == t_null) {
        return false;
    }

    int subtile = 0, rotation = 0;

    // need to check for walls, and then deal with wallfication details!
    // check walls, windows and doors for wall connections
    // may or may not have a subtile available, but should be able to rotate to some extent
    switch (terlist[t].sym) {
    case LINE_XOXO : // vertical
    case LINE_OXOX : // horizontal
    case '"'       :
    case '+'       :
    case '\''      :
        get_wall_values(x, y, LINE_XOXO, LINE_OXOX, subtile, rotation);
        break;
    default :
        get_terrain_orientation(x, y, rotation, subtile);
        break;
    }

    return draw_from_id_string(terlist[t].id, tile_category::terrain, empty_string, x, y, subtile, rotation);
}

bool cata_tiles::draw_furniture(int const x, int const y)
{
    if (!g->m.has_furn(x, y)) {
        return false;
    }

    int const f_id = g->m.furn(x, y);

    // for rotation information
    const int neighborhood[4] = {
        g->m.furn(x,     y + 1), // south
        g->m.furn(x + 1, y    ), // east
        g->m.furn(x - 1, y    ), // west
        g->m.furn(x,     y - 1)  // north
    };

    int subtile = 0, rotation = 0;
    get_tile_values(f_id, neighborhood, subtile, rotation);

    bool const ret = draw_from_id_string(furnlist[f_id].id, tile_category::furniture, empty_string, x, y, subtile, rotation);
    if (ret && g->m.sees_some_items(x, y, g->u)) {
        draw_item_highlight(x, y);
    }

    return ret;
}

bool cata_tiles::draw_trap(int const x, int const y)
{
    int const tr_id = g->m.tr_at(x, y);
    if (tr_id == tr_null) {
        return false;
    }

    if (!traplist[tr_id]->can_see(tripoint(x, y, g->get_levz()), g->u)) {
        return false;
    }

    const int neighborhood[4] = {
        g->m.tr_at(x,     y + 1), // south
        g->m.tr_at(x + 1, y    ), // east
        g->m.tr_at(x - 1, y    ), // west
        g->m.tr_at(x,     y - 1)  // north
    };

    int subtile = 0, rotation = 0;
    get_tile_values(tr_id, neighborhood, subtile, rotation);

    return draw_from_id_string(traplist[tr_id]->id, tile_category::trap, empty_string, x, y, subtile, rotation);
}

bool cata_tiles::draw_field_or_item(int const x, int const y)
{
    // check for field
    const field &f = g->m.field_at(x, y);
    field_id f_id = f.fieldSymbol();
    bool is_draw_field;
    bool do_item;

    switch(f_id) {
        default:
            //only draw fields
            do_item = false;
            is_draw_field = true;
            break;
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
    }

    bool ret = true;   
    if (is_draw_field) {
        // for rotation inforomation
        const int neighborhood[4] = {
            static_cast<int> (g->m.field_at(x, y + 1).fieldSymbol()), // south
            static_cast<int> (g->m.field_at(x + 1, y).fieldSymbol()), // east
            static_cast<int> (g->m.field_at(x - 1, y).fieldSymbol()), // west
            static_cast<int> (g->m.field_at(x, y - 1).fieldSymbol()) // north
        };

        int subtile = 0, rotation = 0;
        get_tile_values(f.fieldSymbol(), neighborhood, subtile, rotation);
        ret &= draw_from_id_string(fieldlist[f.fieldSymbol()].id, tile_category::field, empty_string, x, y, subtile, rotation);
    }

    if (do_item && (ret &= g->m.sees_some_items(x, y, g->u))) {
        auto items = g->m.i_at(x, y);
        auto const size = items.size();
        // get the last item in the stack, it will be used for display
        const item &display_item = items[size - 1];

        std::string it_category = display_item.type->get_item_type_string();
        ret &= draw_from_id_string(display_item.type->id, tile_category::item, it_category, x, y, 0, 0);
        
        if (ret && size > 1) {
            draw_item_highlight(x, y);
        }
    }

    return ret;
}

bool cata_tiles::draw_vpart(int const x, int const y)
{
    int veh_part = 0;
    vehicle *veh = g->m.veh_at(x, y, veh_part);

    if (!veh) {
        return false;
    }
    // veh_part is the index of the part
    // get a north-east-south-west value instead of east-south-west-north value to use with rotation
    int veh_dir = (veh->face.dir4() + 1) % 4;
    if (veh_dir == 1 || veh_dir == 3) {
        veh_dir = (veh_dir + 2) % 4;
    }

    // Gets the visible part, should work fine once tileset vp_ids are updated to work with the vehicle part json ids
    // get the vpart_id
    char part_mod = 0;
    std::string vpid = veh->part_id_string(veh_part, part_mod);
    const char sym = veh->face.dir_symbol(veh->part_sym(veh_part));
    std::string subcategory(1, sym);

    // prefix with vp_ ident
    vpid = "vp_" + vpid;
    int subtile = 0;
    if (part_mod == 1) {
        subtile = multitile_type::open;
    } else if (part_mod == 2) {
        subtile = multitile_type::broken;
    }

    int cargopart = veh->part_with_feature(veh_part, "CARGO");
    bool draw_highlight = (cargopart > 0) && (!veh->get_items(cargopart).empty());
    bool ret = draw_from_id_string(vpid, tile_category::vehicle_part, subcategory, x, y, subtile, veh_dir);
    if (ret && draw_highlight) {
        draw_item_highlight(x, y);
    }
    return ret;
}

bool cata_tiles::draw_entity( const Creature &critter, const int x, const int y )
{
    if( !g->u.sees( critter ) ) {
        if( g->u.sees_with_infrared( critter ) ) {
            return draw_from_id_string( "infrared_creature", tile_category::none, empty_string, x, y, 0, 0 );
        }
    } else if (auto const m = dynamic_cast<const monster*>(&critter)) {
        auto const &type = *m->type;

        // TODO: why just the first species type? what if draw fails?
        std::string const &subcategory = type.species.empty() ? empty_string : *type.species.begin();
        return draw_from_id_string(type.id, tile_category::monster, subcategory, x, y, multitile_type::corner, 0);
    } else if (auto const p = dynamic_cast<const player*>(&critter)) {
        draw_entity_with_overlays( *p, x, y );
        return true;
    }

    return false;
}

void cata_tiles::draw_entity_with_overlays( const player &p, const int x, const int y )
{
    static std::string const ent_nmale   {"npc_male"};
    static std::string const ent_nfemale {"npc_female"};
    static std::string const ent_pmale   {"player_male"};
    static std::string const ent_pfemale {"player_female"};
    static std::string const ovr_male    {"overlay_male_"};
    static std::string const ovr_female  {"overlay_female_"};
    static std::string const ovr_general {"overlay_"};

    std::string const &ent_name = p.is_npc() ? (p.male ? ent_nmale : ent_nfemale)
                                             : (p.male ? ent_pmale : ent_pfemale);

    std::string const &ovr_prefix = p.male ? ovr_male : ovr_female;

    // first draw the character itself (I guess this means a tileset that
    // takes this seriously needs a naked sprite)
    draw_from_id_string(ent_name, tile_category::none, empty_string, x, y, multitile_type::corner, 0);

    std::string id;
    for (const std::string& ovr : p.get_overlay_ids()) {       
        // note the assignment to id
        if (tile_ids.count(id = ovr_prefix + ovr) || tile_ids.count(id = ovr_general + ovr)) {
            draw_from_id_string(id, tile_category::none, empty_string, x, y, multitile_type::corner, 0);
        }
    }
}

bool cata_tiles::draw_item_highlight(int const x, int const y)
{
    create_default_item_highlight();
    return draw_from_id_string(item_highlight, tile_category::none, empty_string, x, y, 0, 0);
}

void cata_tiles::create_default_item_highlight()
{
    constexpr Uint8 highlight_alpha = 127;

    if (tile_ids.count(item_highlight)) {
        return;
    }

    auto surface = create_tile_surface(tile_width_, tile_height_);
    if (!surface) {
        return;
    }

    SDL_FillRect(surface.get(), nullptr, SDL_MapRGBA(surface->format, 0, 0, 127, highlight_alpha));
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface.get());
    if (!texture) {
        dbg( D_ERROR ) << "Failed to create texture: " << SDL_GetError();
        return;
    }

    tile_values.push_back(texture);
    tile_ids[item_highlight] = tile_type(tile_values.size(), -1);
}

/* Animation Functions */
/* -- Inits */
void cata_tiles::init_explosion(int x, int y, int radius)
{
    do_draw_explosion = true;
    exp_pos_x = x;
    exp_pos_y = y;
    exp_rad = radius;
}

void cata_tiles::init_draw_bullet(int x, int y, std::string name)
{
    do_draw_bullet = true;
    bul_pos_x = x;
    bul_pos_y = y;
    bul_id = name;
}

void cata_tiles::init_draw_hit(int x, int y, std::string name)
{
    do_draw_hit = true;
    hit_pos_x = x;
    hit_pos_y = y;
    hit_entity_id = name;
}

void cata_tiles::init_draw_line(int x, int y, std::vector<point> trajectory, std::string name, bool target_line)
{
    do_draw_line = true;
    is_target_line = target_line;
    line_pos_x = x;
    line_pos_y = y;
    line_endpoint_id = name;
    line_trajectory = trajectory;
}

void cata_tiles::init_draw_weather(weather_printable weather, std::string name)
{
    do_draw_weather = true;
    weather_name = name;
    anim_weather = weather;
}

void cata_tiles::init_draw_sct()
{
    do_draw_sct = true;
}

void cata_tiles::init_draw_zones(const point &p_pointStart, const point &p_pointEnd, const point &p_pointOffset)
{
    do_draw_zones = true;
    pStartZone = p_pointStart;
    pEndZone = p_pointEnd;
    pZoneOffset = p_pointOffset;
}

/* -- Void Animators */
void cata_tiles::void_explosion()
{
    do_draw_explosion = false;
    exp_pos_x = -1;
    exp_pos_y = -1;
    exp_rad = -1;
}

void cata_tiles::void_bullet()
{
    do_draw_bullet = false;
    bul_pos_x = -1;
    bul_pos_y = -1;
    bul_id = "";
}

void cata_tiles::void_hit()
{
    do_draw_hit = false;
    hit_pos_x = -1;
    hit_pos_y = -1;
    hit_entity_id = "";
}

void cata_tiles::void_line()
{
    do_draw_line = false;
    is_target_line = false;
    line_pos_x = -1;
    line_pos_y = -1;
    line_endpoint_id = "";
    line_trajectory.clear();
}

void cata_tiles::void_weather()
{
    do_draw_weather = false;
    weather_name = "";
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
    int subtile, rotation;
    const int mx = exp_pos_x, my = exp_pos_y;

    for (int i = 1; i < exp_rad; ++i) {
        subtile = multitile_type::corner;
        rotation = 0;

        draw_from_id_string(exp_name, mx - i, my - i, subtile, rotation++);
        draw_from_id_string(exp_name, mx - i, my + i, subtile, rotation++);
        draw_from_id_string(exp_name, mx + i, my + i, subtile, rotation++);
        draw_from_id_string(exp_name, mx + i, my - i, subtile, rotation++);

        subtile = multitile_type::edge;
        for (int j = 1 - i; j < 0 + i; j++) {
            rotation = 0;
            draw_from_id_string(exp_name, mx + j, my - i, subtile, rotation);
            draw_from_id_string(exp_name, mx + j, my + i, subtile, rotation);

            rotation = 1;
            draw_from_id_string(exp_name, mx - i, my + j, subtile, rotation);
            draw_from_id_string(exp_name, mx + i, my + j, subtile, rotation);
        }
    }
}
void cata_tiles::draw_bullet_frame()
{
    draw_from_id_string(bul_id, tile_category::bullet, empty_string, bul_pos_x, bul_pos_y, 0, 0);
}

void cata_tiles::draw_hit_frame()
{
    const int mx = hit_pos_x, my = hit_pos_y;
    std::string hit_overlay = "animation_hit";

    draw_from_id_string(hit_entity_id, tile_category::hit_entity, empty_string, mx, my, 0, 0);
    draw_from_id_string(hit_overlay, mx, my, 0, 0);
}

void cata_tiles::draw_line()
{
    int mx = line_pos_x, my = line_pos_y;
    std::string line_overlay = "animation_line";
    if (!is_target_line || g->u.sees(mx, my)) {
        for( auto it = line_trajectory.begin(); it != line_trajectory.end() - 1; ++it ) {
            mx = it->x;
            my = it->y;

            draw_from_id_string(line_overlay, mx, my, 0, 0);
        }
    }
    mx = line_trajectory[line_trajectory.size() - 1].x;
    my = line_trajectory[line_trajectory.size() - 1].y;

    draw_from_id_string(line_endpoint_id, mx, my, 0, 0);
}

void cata_tiles::draw_weather_frame()
{
    for (auto const &drop : anim_weather.vdrops) {
        // currently in ascii screen coordinates
        int const x = drop.first  + o_x;
        int const y = drop.second + o_y;
        draw_from_id_string(weather_name, tile_category::weather, empty_string, x, y, 0, 0);
    }
}

void cata_tiles::draw_sct_frame()
{
    static std::array<std::string, 2> const which {{
        "first", "second"
    }};

    ascii_id::set_bg_color();

    for (auto const &text : SCT.vSCT) {
        const int dx = text.getPosX();
        const int dy = text.getPosY();

        int x_off = 0;
        bool const is_old = text.getStep() >= SCT.iMaxSteps / 2;

        for (int i = 0; i < 2; ++i) {
            ascii_id::set_fg_color(static_cast<char>(
                msgtype_to_tilecolor(text.getMsgType(which[i]), is_old)));
            
            for (auto const &c : text.getText(which[i])) {
                ascii_id::set_codepoint(c);
                auto const it = tile_ids.find(ascii_id::id());
                if (it != tile_ids.end()) {
                    const int x = (dx + x_off - o_x) * tile_width_  + op_x;
                    const int y = (dy         - o_y) * tile_height_ + op_y;

                    draw_tile_at(it->second, x, y, 0);
                }

                ++x_off;
            }
        }
    }
}

void cata_tiles::draw_zones_frame()
{
    if (!tile_ids.count(item_highlight)) {
        create_default_item_highlight();
    }

    for (int y = pStartZone.y; y <= pEndZone.y; ++y) {
        for (int x = pStartZone.x; x <= pEndZone.x; ++x) {
            draw_from_id_string(item_highlight, x + pZoneOffset.x, y + pZoneOffset.y, 0, 0);
        }
    }
}
void cata_tiles::draw_footsteps_frame()
{
    static const std::string footstep_tilestring = "footstep";
    for( const auto &footstep : sounds::get_footstep_markers() ) {
        draw_from_id_string(footstep_tilestring, footstep.x, footstep.y, 0, 0);
    }
}
/* END OF ANIMATION FUNCTIONS */

void cata_tiles::init_light()
{
    g->reset_light_level();

    sightrange_natural = g->u.sight_range(1);
    g_lightlevel = (int)g->light_level();
    sightrange_light = g->u.sight_range(g_lightlevel);
    sightrange_lowlight = std::max(g_lightlevel / 2, sightrange_natural);
    sightrange_max = g->u.unimpaired_range();
    u_clairvoyance = g->u.clairvoyance();

    boomered = g->u.has_effect("boomered");
    sight_impaired = g->u.sight_impaired();
    bionight_bionic_active = g->u.has_active_bionic("bio_night");
}

light_type cata_tiles::light_at(int const x, int const y)
{
    /** Logic */
    const int dist = rl_dist(g->u.posx(), g->u.posy(), x, y);

    int real_max_sight_range = sightrange_light > sightrange_max ? sightrange_light : sightrange_max;
    int distance_to_look = DAYLIGHT_LEVEL;

    bool can_see = g->m.pl_sees( x, y, distance_to_look );
    lit_level lit = g->m.light_at(x, y);

    if (lit != LL_BRIGHT && dist > real_max_sight_range) {
        int intlit = (int)lit - (dist - real_max_sight_range) / 2;
        if (intlit < 0) {
            intlit = LL_DARK;
        }
        lit = (lit_level)intlit;
    }

    if (lit > LL_DARK && real_max_sight_range > 1) {
        real_max_sight_range = distance_to_look;
    }

    /** Conditional Returns */
    if ((bionight_bionic_active && dist < 15 && dist > sightrange_natural) ||
            dist > real_max_sight_range ||
            (dist > sightrange_light &&
             (lit == LL_DARK ||
              (sight_impaired && lit != LL_BRIGHT) ||
              !can_see))) {
        if (boomered) {
            // exit w/ dark boomerfication
            return light_type::boomer_dark;
        } else {
            // exit w/ dark normal
            return light_type::light_dark;
        }
    } else if (dist > sightrange_light && sight_impaired && lit == LL_BRIGHT) {
        if (boomered) {
            // exit w/ light boomerfication
            return light_type::boomer_normal;
        } else {
            // exit w/ light normal
            return light_type::light_normal;
        }
    } else if (dist <= u_clairvoyance || can_see) {
        // check for rain

        // return with okay to draw the square = 0
        return light_type::clear;
    }

    return light_type::hidden;
}

void cata_tiles::get_terrain_orientation(int x, int y, int &rota, int &subtile)
{
    // get terrain at x,y
    ter_id tid = g->m.ter(x, y);
    if (tid == t_null) {
        subtile = 0;
        rota = 0;
        return;
    }

    // get terrain neighborhood
    const ter_id neighborhood[4] = {
        g->m.ter(x, y + 1), // south
        g->m.ter(x + 1, y), // east
        g->m.ter(x - 1, y), // west
        g->m.ter(x, y - 1) // north
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
    switch (num_connects) {
        case 0:
            rotation = 0;
            subtile = multitile_type::unconnected;
            break;
        case 4:
            rotation = 0;
            subtile = multitile_type::center;
            break;
        case 1: // all end pieces
            subtile = multitile_type::end_piece;
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
                    subtile = multitile_type::edge;
                    rotation = 0;
                    break;
                case 6:
                    subtile = multitile_type::edge;
                    rotation = 1;
                    break;
                    // corners
                case 12:
                    subtile = multitile_type::corner;
                    rotation = 2;
                    break;
                case 10:
                    subtile = multitile_type::corner;
                    rotation = 1;
                    break;
                case 3:
                    subtile = multitile_type::corner;
                    rotation = 0;
                    break;
                case 5:
                    subtile = multitile_type::corner;
                    rotation = 3;
                    break;
            }
            break;
        case 3: // all t_connections
            subtile = multitile_type::t_connection;
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

void cata_tiles::get_wall_values(const int x, const int y, const long vertical_wall_symbol,
                                 const long horizontal_wall_symbol, int &subtile, int &rotation)
{
    // makes the assumption that x,y is a wall | window | door of some sort
    const long neighborhood[4] = {
        terlist[g->m.ter(x, y + 1)].sym, // south
        terlist[g->m.ter(x + 1, y)].sym, // east
        terlist[g->m.ter(x - 1, y)].sym, // west
        terlist[g->m.ter(x, y - 1)].sym // north
    };

    bool connects[4];

    char val = 0;
    int num_connects = 0;

    // populate connection information
    for (int i = 0; i < 4; ++i) {
        connects[i] = (neighborhood[i] == vertical_wall_symbol || neighborhood[i] == horizontal_wall_symbol) || (neighborhood[i] == '"' || neighborhood[i] == '+' || neighborhood[i] == '\'');

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

#endif // SDL_TILES
