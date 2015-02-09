#if (defined SDLTILES)
#include "cata_tiles.h"
#include "debug.h"
#include "json.h"
#include "path_info.h"
#include "monstergenerator.h"
#include "item.h"
#include "veh_type.h"
#include "filesystem.h"
#include "sounds.h"

#include <algorithm>
#include <fstream>

#include "SDL2/SDL_image.h"

#define dbg(x) DebugLog((DebugLevel)(x),D_SDL) << __FILE__ << ":" << __LINE__ << ": "

#define ITEM_HIGHLIGHT "highlight_item"

extern game *g;
//extern SDL_Surface *screen;
extern int WindowHeight, WindowWidth;
extern int fontwidth, fontheight;

static const std::string empty_string;
static const std::string TILE_CATEGORY_IDS[] = {
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
};

cata_tiles::cata_tiles(SDL_Renderer *render)
{
    //ctor
    renderer = render;

    tile_height = 0;
    tile_width = 0;
    tile_ratiox = 0;
    tile_ratioy = 0;

    in_animation = false;
    do_draw_explosion = false;
    do_draw_bullet = false;
    do_draw_hit = false;
    do_draw_line = false;
    do_draw_weather = false;
    do_draw_sct = false;
    do_draw_zones = false;

    boomered = false;
    sight_impaired = false;
    bionight_bionic_active = false;

    last_pos_x = 0;
    last_pos_y = 0;
}

cata_tiles::~cata_tiles()
{
    clear();
}

void cata_tiles::clear()
{
    // release maps
    for (tile_iterator it = tile_values.begin(); it != tile_values.end(); ++it) {
        SDL_DestroyTexture(*it);
    }
    tile_values.clear();
    for (tile_id_iterator it = tile_ids.begin(); it != tile_ids.end(); ++it) {
        it->second = NULL;
    }
    tile_ids.clear();
}

void cata_tiles::init(std::string load_file_path)
{
    std::string json_path, tileset_path;
    // get path information from load_file_path
    get_tile_information(load_file_path, json_path, tileset_path);
    // send this information to old init to avoid redundant code
    load_tilejson(json_path, tileset_path);
}

void cata_tiles::reinit(std::string load_file_path)
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

int cata_tiles::load_tileset(std::string path, int R, int G, int B)
{
    std::string img_path = path;
#ifdef PREFIX   // use the PREFIX path over the current directory
    img_path = (FILENAMES["datadir"] + "/" + img_path);
#endif
    /** reinit tile_atlas */
    SDL_Surface *tile_atlas = IMG_Load(img_path.c_str());

    if(!tile_atlas) {
        throw std::string("Could not load tileset image at ") + img_path + ", error: " + IMG_GetError();
    }

        /** get dimensions of the atlas image */
        int w = tile_atlas->w;
        int h = tile_atlas->h;
        /** sx and sy will take care of any extraneous pixels that do not add up to a full tile */
        int sx = w / tile_width;
        int sy = h / tile_height;

        sx *= tile_width;
        sy *= tile_height;

        // Set up initial source and destination information. Destination is going to be unchanging
        SDL_Rect source_rect = {0,0,tile_width,tile_height};
        SDL_Rect dest_rect = {0,0,tile_width,tile_height};

        /** split the atlas into tiles using SDL_Rect structs instead of slicing the atlas into individual surfaces */
        int tilecount = 0;
        for (int y = 0; y < sy; y += tile_height) {
            for (int x = 0; x < sx; x += tile_width) {
                source_rect.x = x;
                source_rect.y = y;

                SDL_Surface *tile_surf = create_tile_surface();
                if( tile_surf == nullptr ) {
                    continue;
                }
                if( SDL_BlitSurface( tile_atlas, &source_rect, tile_surf, &dest_rect ) != 0 ) {
                    dbg( D_ERROR ) << "SDL_BlitSurface failed: " << SDL_GetError();
                }
                if (R >= 0 && R <= 255 && G >= 0 && G <= 255 && B >= 0 && B <= 255) {
                    Uint32 key = SDL_MapRGB(tile_surf->format, 0,0,0);
                    SDL_SetColorKey(tile_surf, SDL_TRUE, key);
                    SDL_SetSurfaceRLE(tile_surf, true);
                }

                SDL_Texture *tile_tex = SDL_CreateTextureFromSurface(renderer,tile_surf);
                if( tile_tex == nullptr ) {
                    dbg( D_ERROR) << "failed to create texture: " << SDL_GetError();
                }

                SDL_FreeSurface(tile_surf);

                if( tile_tex != nullptr ) {
                tile_values.push_back(tile_tex);
                tilecount++;
                }
            }
        }

        dbg( D_INFO ) << "Tiles Created: " << tilecount;
        SDL_FreeSurface(tile_atlas);
        return tilecount;
}

void cata_tiles::set_draw_scale(int scale) {
    tile_width = default_tile_width * scale / 16;
    tile_height = default_tile_height * scale / 16;

    tile_ratiox = ((float)tile_width/(float)fontwidth);
    tile_ratioy = ((float)tile_height/(float)fontheight);
}

void cata_tiles::load_tilejson(std::string path, const std::string &image_path)
{
    dbg( D_INFO ) << "Attempting to Load JSON file " << path;
    std::ifstream config_file(path.c_str(), std::ifstream::in | std::ifstream::binary);

    if (!config_file.good()) {
        throw std::string("failed to open tile info json: ") + path;
    }

        load_tilejson_from_file( config_file, image_path );
        if (tile_ids.count("unknown") == 0) {
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
        tile_height = curr_info.get_int("height");
        tile_width = curr_info.get_int("width");

        default_tile_width = tile_width;
        default_tile_height = tile_height;
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

void cata_tiles::add_ascii_subtile(tile_type *curr_tile, const std::string &t_id, int fg, const std::string &s_id)
{
    const std::string m_id = t_id + "_" + s_id;
    tile_type *curr_subtile = new tile_type();
    curr_subtile->fg = fg;
    curr_subtile->bg = -1;
    curr_subtile->rotates = true;
    tile_ids[m_id] = curr_subtile;
    curr_tile->available_subtiles.push_back(s_id);
}

void cata_tiles::load_ascii_tilejson_from_file(JsonObject &config, int offset, int size)
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

void cata_tiles::load_ascii_set(JsonObject &entry, int offset, int size)
{
    // tile for ASCII char 0 is at `in_image_offset`,
    // the other ASCII chars follow from there.
    const int in_image_offset = entry.get_int("offset");
    if (in_image_offset >= size) {
        entry.throw_error("invalid offset (out of range)", "offset");
    }
    // color, of the ASCII char. Can be -1 to indicate all/default colors.
    int FG = -1;
    const std::string scolor = entry.get_string("color", "DEFAULT");
    if (scolor == "BLACK") {
        FG = COLOR_BLACK;
    } else if (scolor == "RED") {
        FG = COLOR_RED;
    } else if (scolor == "GREEN") {
        FG = COLOR_GREEN;
    } else if (scolor == "YELLOW") {
        FG = COLOR_YELLOW;
    } else if (scolor == "BLUE") {
        FG = COLOR_BLUE;
    } else if (scolor == "MAGENTA") {
        FG = COLOR_MAGENTA;
    } else if (scolor == "CYAN") {
        FG = COLOR_CYAN;
    } else if (scolor == "WHITE") {
        FG = COLOR_WHITE;
    } else if (scolor == "DEFAULT") {
        FG = -1;
    } else {
        entry.throw_error("invalid color for ascii", "color");
    }
    // Add an offset for bold colors (ncrses has this bold attribute,
    // this mimics it). bold does not apply to default color.
    if (FG != -1 && entry.get_bool("bold", false)) {
        FG += 8;
    }
    const int base_offset = offset + in_image_offset;
    // template for the id of the ascii chars:
    // X is replaced by ascii code (converted to char)
    // F is replaced by foreground (converted to char)
    // B is replaced by background (converted to char)
    std::string id("ASCII_XFB");
    // Finally load all 256 ascii chars (actually extended ascii)
    for (int ascii_char = 0; ascii_char < 256; ascii_char++) {
        const int index_in_image = ascii_char + in_image_offset;
        if (index_in_image < 0 || index_in_image >= size) {
            // Out of range is ignored for now.
            continue;
        }
        id[6] = static_cast<char>(ascii_char);
        id[7] = static_cast<char>(FG);
        id[8] = static_cast<char>(-1);
        tile_type *curr_tile = new tile_type();
        curr_tile->fg = index_in_image + offset;
        curr_tile->bg = 0;
        switch(ascii_char) {
        case LINE_OXOX_C://box bottom/top side (horizontal line)
            curr_tile->fg = 205 + base_offset;
            break;
        case LINE_XOXO_C://box left/right side (vertical line)
            curr_tile->fg = 186 + base_offset;
            break;
        case LINE_OXXO_C://box top left
            curr_tile->fg = 201 + base_offset;
            break;
        case LINE_OOXX_C://box top right
            curr_tile->fg = 187 + base_offset;
            break;
        case LINE_XOOX_C://box bottom right
            curr_tile->fg = 188 + base_offset;
            break;
        case LINE_XXOO_C://box bottom left
            curr_tile->fg = 200 + base_offset;
            break;
        case LINE_XXOX_C://box bottom north T (left, right, up)
            curr_tile->fg = 202 + base_offset;
            break;
        case LINE_XXXO_C://box bottom east T (up, right, down)
            curr_tile->fg = 208 + base_offset;
            break;
        case LINE_OXXX_C://box bottom south T (left, right, down)
            curr_tile->fg = 203 + base_offset;
            break;
        case LINE_XXXX_C://box X (left down up right)
            curr_tile->fg = 206 + base_offset;
            break;
        case LINE_XOXX_C://box bottom east T (left, down, up)
            curr_tile->fg = 184 + base_offset;
            break;
        }
        tile_ids[id] = curr_tile;
        if (ascii_char == LINE_XOXO_C || ascii_char == LINE_OXOX_C) {
            curr_tile->rotates = false;
            curr_tile->multitile = true;
            add_ascii_subtile(curr_tile, id, 206 + base_offset, "center");
            add_ascii_subtile(curr_tile, id, 201 + base_offset, "corner");
            add_ascii_subtile(curr_tile, id, 186 + base_offset, "edge");
            add_ascii_subtile(curr_tile, id, 203 + base_offset, "t_connection");
            add_ascii_subtile(curr_tile, id, 208 + base_offset, "end_piece");
            add_ascii_subtile(curr_tile, id, 219 + base_offset, "unconnected");
        }
    }
}

void cata_tiles::load_tilejson_from_file(JsonObject &config, int offset, int size)
{
    if (!config.has_member("tiles")) {
        config.throw_error( "\"tiles\" section missing" );
    }

    JsonArray tiles = config.get_array("tiles");
    while (tiles.has_more()) {
        JsonObject entry = tiles.next_object();

        std::string t_id = entry.get_string("id");
        tile_type *curr_tile = load_tile(entry, t_id, offset, size);
        bool t_multi = entry.get_bool("multitile", false);
        bool t_rota = entry.get_bool("rotates", t_multi);
        if (t_multi) {
            // fetch additional tiles
            JsonArray subentries = entry.get_array("additional_tiles");
            while (subentries.has_more()) {
                JsonObject subentry = subentries.next_object();
                const std::string s_id = subentry.get_string("id");
                const std::string m_id = t_id + "_" + s_id;
                tile_type *curr_subtile = load_tile(subentry, m_id, offset, size);
                curr_subtile->rotates = true;
                curr_tile->available_subtiles.push_back(s_id);
            }
        }

        // write the information of the base tile to curr_tile
        curr_tile->multitile = t_multi;
        curr_tile->rotates = t_rota;
    }
    dbg( D_INFO ) << "Tile Width: " << tile_width << " Tile Height: " << tile_height << " Tile Definitions: " << tile_ids.size();
}

tile_type *cata_tiles::load_tile(JsonObject &entry, const std::string &id, int offset, int size)
{
    int fg = entry.get_int("fg", -1);
    int bg = entry.get_int("bg", -1);
    if (fg == -1) {
        // OK, keep this value, indicates "doesn't have a foreground"
    } else if (fg < 0 || fg >= size) {
        entry.throw_error("invalid value for fg (out of range)", "fg");
    } else {
        fg += offset;
    }
    if (bg == -1) {
        // OK, keep this value, indicates "doesn't have a background"
    } else if (bg < 0 || bg >= size) {
        entry.throw_error("invalid value for bg (out of range)", "bg");
    } else {
        bg += offset;
    }
    tile_type *curr_subtile = new tile_type();
    curr_subtile->fg = fg;
    curr_subtile->bg = bg;
    tile_ids[id] = curr_subtile;
    return curr_subtile;
}

void cata_tiles::draw(int destx, int desty, int centerx, int centery, int width, int height)
{
    if (!g) {
        return;
    }

    {
        //set clipping to prevent drawing over stuff we shouldn't
        SDL_Rect clipRect = {destx, desty, width, height};
        SDL_RenderSetClipRect(renderer, &clipRect);
    }

    int posx = centerx;
    int posy = centery;

    int sx, sy;
    get_window_tile_counts(width, height, sx, sy);

    init_light();

    int x, y;
    LIGHTING l;

    o_x = posx - POSX;
    o_y = posy - POSY;
    op_x = destx;
    op_y = desty;
    // Rounding up to include incomplete tiles at the bottom/right edges
    screentile_width = (width + tile_width - 1) / tile_width;
    screentile_height = (height + tile_height - 1) / tile_height;

    for (int my = 0; my < sy; ++my) {
        for (int mx = 0; mx < sx; ++mx) {
            x = mx + o_x;
            y = my + o_y;
            l = light_at(x, y);
            const auto critter = g->critter_at( x, y );
            if (l != CLEAR) {
                // Draw lighting
                draw_lighting(x, y, l);
                if( critter != nullptr && g->u.sees_with_infrared( *critter ) ) {
                    draw_from_id_string( "infrared_creature", C_NONE, empty_string, x, y, 0, 0 );
                }
                continue;
            }
            // light is no longer being considered, for now.
            // Draw Terrain if possible. If not possible then we need to continue on to the next part of loop
            if (!draw_terrain(x, y)) {
                continue;
            }
            draw_furniture(x, y);
            draw_trap(x, y);
            draw_field_or_item(x, y);
            draw_vpart(x, y);
            if( critter != nullptr ) {
                draw_entity( *critter, x, y );
            }
        }
    }
    in_animation = do_draw_explosion || do_draw_bullet || do_draw_hit ||
                   do_draw_line || do_draw_weather || do_draw_sct ||
                   do_draw_zones;

    draw_footsteps_frame();
    if (in_animation) {
        if (do_draw_explosion) {
            draw_explosion_frame();
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
            draw_sct_frame();
            void_sct();
        }
        if (do_draw_zones) {
            draw_zones_frame();
            void_zones();
        }
    }
    // check to see if player is located at ter
    else if (g->u.posx() + g->u.view_offset_x != g->ter_view_x ||
             g->u.posy() + g->u.view_offset_y != g->ter_view_y) {
        draw_from_id_string("cursor", C_NONE, empty_string, g->ter_view_x, g->ter_view_y, 0, 0);
    }

    SDL_RenderSetClipRect(renderer, NULL);
}

void cata_tiles::clear_buffer()
{
    //TODO convert this to use sdltiles ClearScreen() function
    SDL_RenderClear(renderer);
}

void cata_tiles::get_window_tile_counts(const int width, const int height, int &columns, int &rows) const
{
    columns = ceil((double) width / tile_width);
    rows = ceil((double) height / tile_height);
}

bool cata_tiles::draw_from_id_string(std::string id, int x, int y, int subtile, int rota)
{
    return cata_tiles::draw_from_id_string(std::move(id), C_NONE, empty_string, x, y, subtile, rota);
}

bool cata_tiles::draw_from_id_string(std::string id, TILE_CATEGORY category,
                                     const std::string &subcategory, int x, int y,
                                     int subtile, int rota)
{
    // If the ID string does not produce a drawable tile
    // it will revert to the "unknown" tile.
    // The "unknown" tile is one that is highly visible so you kinda can't miss it :D

    // check to make sure that we are drawing within a valid area
    // [0->width|height / tile_width|height]
    if( x - o_x < 0 || x - o_x >= screentile_width ||
        y - o_y < 0 || y - o_y >= screentile_height ) {
        return false;
    }

    constexpr size_t suffix_len = 15;
    constexpr char const season_suffix[4][suffix_len] = {
        "_season_spring", "_season_summer", "_season_autumn", "_season_winter"};
   
    std::string seasonal_id;
    seasonal_id.reserve(id.size() + suffix_len);
    seasonal_id.append(id).append(season_suffix[calendar::turn.get_season()], suffix_len - 1);

    tile_id_iterator it = tile_ids.find(seasonal_id);
    if (it == tile_ids.end()) {
        it = tile_ids.find(id);
    } else {
        id = std::move(seasonal_id);
    }

    if (it == tile_ids.end()) {
        long sym = -1;
        nc_color col = c_white;
        if (category == C_FURNITURE) {
            if (furnmap.count(id) > 0) {
                const furn_t &f = furnmap[id];
                sym = f.sym;
                col = f.color;
            }
        } else if (category == C_TERRAIN) {
            if (termap.count(id) > 0) {
                const ter_t &t = termap[id];
                sym = t.sym;
                col = t.color;
            }
        } else if (category == C_MONSTER) {
            if (MonsterGenerator::generator().has_mtype(id)) {
                const mtype *m = MonsterGenerator::generator().get_mtype(id);
                int len = m->sym.length();
                const char *s = m->sym.c_str();
                sym = UTF8_getch(&s, &len);
                col = m->color;
            }
        } else if (category == C_VEHICLE_PART) {
            if (vehicle_part_types.count(id.substr(3)) > 0) {
                const vpart_info &v = vehicle_part_types[id.substr(3)];
                sym = v.sym;
                if (!subcategory.empty()) {
                    sym = special_symbol(subcategory[0]);
                    rota = 0;
                    subtile = -1;
                }
                col = v.color;
            }
        } else if (category == C_FIELD) {
            const field_id fid = field_from_ident( id );
            sym = fieldlist[fid].sym;
            // TODO: field density?
            col = fieldlist[fid].color[0];
        } else if (category == C_TRAP) {
            if (trapmap.count(id) > 0) {
                const trap *t = traplist[trapmap[id]];
                sym = t->sym;
                col = t->color;
            }
        } else if (category == C_ITEM) {
            const auto tmp = item( id, 0 );
            sym = tmp.symbol();
            col = tmp.color();
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
        if (sym != 0 && sym < 256 && sym >= 0) {
            // see cursesport.cpp, function wattron
            const int pairNumber = (col & A_COLOR) >> 17;
            const pairs &colorpair = colorpairs[pairNumber];
            // What about isBlink?
            const bool isBold = col & A_BOLD;
            const int FG = colorpair.FG + (isBold ? 8 : 0);
//            const int BG = colorpair.BG;
            // static so it does not need to be allocated every time,
            // see load_ascii_set for the meaning
            static std::string generic_id("ASCII_XFG");
            generic_id[6] = static_cast<char>(sym);
            generic_id[7] = static_cast<char>(FG);
            generic_id[8] = static_cast<char>(-1);
            if (tile_ids.count(generic_id) > 0) {
                return draw_from_id_string(generic_id, x, y, subtile, rota);
            }
            // Try again without color this time (using default color).
            generic_id[7] = static_cast<char>(-1);
            generic_id[8] = static_cast<char>(-1);
            if (tile_ids.count(generic_id) > 0) {
                return draw_from_id_string(generic_id, x, y, subtile, rota);
            }
        }
    }

    // if id is not found, try to find a tile for the category+subcategory combination
    if (it == tile_ids.end()) {
        const std::string &category_id = TILE_CATEGORY_IDS[category];
        if(!category_id.empty() && !subcategory.empty()) {
            it = tile_ids.find("unknown_" + category_id + "_" + subcategory);
        }
    }

    // if at this point we have no tile, try just the category
    if (it == tile_ids.end()) {
        const std::string &category_id = TILE_CATEGORY_IDS[category];
        if(!category_id.empty()) {
            it = tile_ids.find("unknown_" + category_id);
        }
    }

    // if we still have no tile, we're out of luck, fall back to unknown
    if (it == tile_ids.end()) {
        it = tile_ids.find("unknown");
    }

    //  this really shouldn't happen, but the tileset creator might have forgotten to define an unknown tile
    if (it == tile_ids.end()) {
        return false;
    }

    tile_type *display_tile = it->second;
    // if found id does not have a valid tile_type then return unknown tile
    if (!display_tile) {
        return draw_from_id_string("unknown", x, y, subtile, rota);
    }

    // if both bg and fg are -1 then return unknown tile
    if (display_tile->bg == -1 && display_tile->fg == -1) {
        return draw_from_id_string("unknown", x, y, subtile, rota);
    }

    // check to see if the display_tile is multitile, and if so if it has the key related to subtile
    if (subtile != -1 && display_tile->multitile) {
        std::vector<std::string> display_subtiles = display_tile->available_subtiles;
        if (std::find(display_subtiles.begin(), display_subtiles.end(), multitile_keys[subtile]) != display_subtiles.end()) {
            // append subtile name to tile and re-find display_tile
            return draw_from_id_string(
                std::move(id.append("_", 1).append(multitile_keys[subtile])),x, y, -1, rota);
        }
    }

    // make sure we aren't going to rotate the tile if it shouldn't be rotated
    if (!display_tile->rotates) {
        rota = 0;
    }

    // translate from player-relative to screen relative tile position
    const int screen_x = (x - o_x) * tile_width + op_x;
    const int screen_y = (y - o_y) * tile_height + op_y;

    //draw it!
    draw_tile_at(display_tile, screen_x, screen_y, rota);

    return true;
}

bool cata_tiles::draw_tile_at(tile_type *tile, int x, int y, int rota)
{
    // don't need to check for tile existance, should always exist if it gets this far
    const int fg = tile->fg;
    const int bg = tile->bg;

    SDL_Rect destination;
    destination.x = x;
    destination.y = y;
    destination.w = tile_width;
    destination.h = tile_height;

    // blit background first : always non-rotated
    if( bg >= 0 && static_cast<size_t>( bg ) < tile_values.size() ) {
        SDL_Texture *bg_tex = tile_values[bg];
        if( SDL_RenderCopyEx( renderer, bg_tex, NULL, &destination, 0, NULL, SDL_FLIP_NONE ) != 0 ) {
            dbg( D_ERROR ) << "SDL_RenderCopyEx(bg) failed: " << SDL_GetError();
        }
    }

    int ret = 0;
    // blit foreground based on rotation
    if (rota == 0) {
        if (fg >= 0 && static_cast<size_t>( fg ) < tile_values.size()) {
            SDL_Texture *fg_tex = tile_values[fg];
            ret = SDL_RenderCopyEx( renderer, fg_tex, NULL, &destination, 0, NULL, SDL_FLIP_NONE );
        }
    } else {
        if (fg >= 0 && static_cast<size_t>( fg ) < tile_values.size()) {
            SDL_Texture *fg_tex = tile_values[fg];

            if(rota == 1) {
#if (defined _WIN32 || defined WINDOWS)
                destination.y -= 1;
#endif
                ret = SDL_RenderCopyEx( renderer, fg_tex, NULL, &destination,
                    -90, NULL, SDL_FLIP_NONE );
            } else if(rota == 2) {
                //flip rather then rotate here
                ret = SDL_RenderCopyEx( renderer, fg_tex, NULL, &destination,
                    0, NULL, static_cast<SDL_RendererFlip>( SDL_FLIP_HORIZONTAL | SDL_FLIP_VERTICAL ) );
            } else { //rota == 3
#if (defined _WIN32 || defined WINDOWS)
                destination.x -= 1;
#endif
                ret = SDL_RenderCopyEx( renderer, fg_tex, NULL, &destination,
                    90, NULL, SDL_FLIP_NONE );
            }
        }
    }
    if( ret != 0 ) {
        dbg( D_ERROR ) << "SDL_RenderCopyEx(fg) failed: " << SDL_GetError();
    }

    return true;
}

bool cata_tiles::draw_lighting(int x, int y, LIGHTING l)
{
    std::string light_name;
    switch(l) {
        case HIDDEN:
            light_name = "lighting_hidden";
            break;
        case LIGHT_NORMAL:
            light_name = "lighting_lowlight_light";
            break;
        case LIGHT_DARK:
            light_name = "lighting_lowlight_dark";
            break;
        case BOOMER_NORMAL:
            light_name = "lighting_boomered_light";
            break;
        case BOOMER_DARK:
            light_name = "lighting_boomered_dark";
            break;
        case CLEAR: // Actually handled by the caller.
            return false;
    }

    // lighting is never rotated, though, could possibly add in random rotation?
    draw_from_id_string(light_name, C_LIGHTING, empty_string, x, y, 0, 0);

    return false;
}

bool cata_tiles::draw_terrain(int x, int y)
{
    int t = g->m.ter(x, y); // get the ter_id value at this point
    // check for null, if null return false
    if (t == t_null) {
        return false;
    }

    // need to check for walls, and then deal with wallfication details!
    int s = terlist[t].sym;

    //char alteration = 0;
    int subtile = 0, rotation = 0;

    // check walls
    if (s == LINE_XOXO /*vertical*/ || s == LINE_OXOX /*horizontal*/) {
        get_wall_values(x, y, LINE_XOXO, LINE_OXOX, subtile, rotation);
    }
    // check windows and doors for wall connections, may or may not have a subtile available, but should be able to rotate to some extent
    else if (s == '"' || s == '+' || s == '\'') {
        get_wall_values(x, y, LINE_XOXO, LINE_OXOX, subtile, rotation);
    } else {
        get_terrain_orientation(x, y, rotation, subtile);
        // do something to get other terrain orientation values
    }

    std::string tname;

    tname = terlist[t].id;

    return draw_from_id_string(tname, C_TERRAIN, empty_string, x, y, subtile, rotation);
}

bool cata_tiles::draw_furniture(int x, int y)
{
    // get furniture ID at x,y
    bool has_furn = g->m.has_furn(x, y);
    if (!has_furn) {
        return false;
    }

    int f_id = g->m.furn(x, y);

    // for rotation information
    const int neighborhood[4] = {
        static_cast<int> (g->m.furn(x, y + 1)), // south
        static_cast<int> (g->m.furn(x + 1, y)), // east
        static_cast<int> (g->m.furn(x - 1, y)), // west
        static_cast<int> (g->m.furn(x, y - 1)) // north
    };

    int subtile = 0, rotation = 0;
    get_tile_values(f_id, neighborhood, subtile, rotation);

    // get the name of this furniture piece
    std::string f_name = furnlist[f_id].id; // replace with furniture names array access
    bool ret = draw_from_id_string(f_name, C_FURNITURE, empty_string, x, y, subtile, rotation);
    if (ret && g->m.sees_some_items(x, y, g->u)) {
        draw_item_highlight(x, y);
    }
    return ret;
}

bool cata_tiles::draw_trap(int x, int y)
{
    int tr_id = g->m.tr_at(x, y);
    if (tr_id == tr_null) {
        return false;
    }
    if (!traplist[tr_id]->can_see(g->u, x, y)) {
        return false;
    }

    const std::string tr_name = traplist[tr_id]->id;

    const int neighborhood[4] = {
        static_cast<int> (g->m.tr_at(x, y + 1)), // south
        static_cast<int> (g->m.tr_at(x + 1, y)), // east
        static_cast<int> (g->m.tr_at(x - 1, y)), // west
        static_cast<int> (g->m.tr_at(x, y - 1)) // north
    };

    int subtile = 0, rotation = 0;
    get_tile_values(tr_id, neighborhood, subtile, rotation);

    return draw_from_id_string(tr_name, C_TRAP, empty_string, x, y, subtile, rotation);
}

bool cata_tiles::draw_field_or_item(int x, int y)
{
    // check for field
    const field &f = g->m.field_at(x, y);
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

        // for rotation inforomation
        const int neighborhood[4] = {
            static_cast<int> (g->m.field_at(x, y + 1).fieldSymbol()), // south
            static_cast<int> (g->m.field_at(x + 1, y).fieldSymbol()), // east
            static_cast<int> (g->m.field_at(x - 1, y).fieldSymbol()), // west
            static_cast<int> (g->m.field_at(x, y - 1).fieldSymbol()) // north
        };

        int subtile = 0, rotation = 0;
        get_tile_values(f.fieldSymbol(), neighborhood, subtile, rotation);

        ret_draw_field = draw_from_id_string(fd_name, C_FIELD, empty_string, x, y, subtile, rotation);
    }
    if(do_item) {
        if (!g->m.sees_some_items(x, y, g->u)) {
            return false;
        }
        auto items = g->m.i_at(x, y);
        // get the last item in the stack, it will be used for display
        const item &display_item = items[items.size() - 1];
        // get the item's name, as that is the key used to find it in the map
        const std::string &it_name = display_item.type->id;
        const std::string it_category = display_item.type->get_item_type_string();
        ret_draw_item = draw_from_id_string(it_name, C_ITEM, it_category, x, y, 0, 0);
        if (ret_draw_item && items.size() > 1) {
            draw_item_highlight(x, y);
        }
    }
    return ret_draw_field && ret_draw_item;
}

bool cata_tiles::draw_vpart(int x, int y)
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
    int cargopart = veh->part_with_feature(veh_part, "CARGO");
    bool draw_highlight = (cargopart > 0) && (!veh->get_items(cargopart).empty());
    bool ret = draw_from_id_string(vpid, C_VEHICLE_PART, subcategory, x, y, subtile, veh_dir);
    if (ret && draw_highlight) {
        draw_item_highlight(x, y);
    }
    return ret;
}

bool cata_tiles::draw_entity( const Creature &critter, const int x, const int y )
{
    if( !g->u.sees( critter ) ) {
        if( g->u.sees_with_infrared( critter ) ) {
            return draw_from_id_string( "infrared_creature", C_NONE, empty_string, x, y, 0, 0 );
        }
        return false;
    }
    const monster *m = dynamic_cast<const monster*>( &critter );
    if( m != nullptr ) {
        const auto ent_name = m->type->id;
        const auto ent_category = C_MONSTER;
        std::string ent_subcategory = empty_string;
        if( !m->type->species.empty() ) {
            ent_subcategory = *m->type->species.begin();
        }
        const int subtile = corner;
        return draw_from_id_string(ent_name, ent_category, ent_subcategory, x, y, subtile, 0);
    }
    const player *p = dynamic_cast<const player*>( &critter );
    if( p != nullptr ) {
        draw_entity_with_overlays( *p, x, y );
        return true;
    }
    return false;
}

void cata_tiles::draw_entity_with_overlays( const player &p, const int x, const int y )
{
    std::string ent_name;

    if( p.is_npc() ) {
        ent_name = p.male ? "npc_male" : "npc_female";
    } else {
        ent_name = p.male ? "player_male" : "player_female";
    }
        // first draw the character itself(i guess this means a tileset that
        // takes this seriously needs a naked sprite)
        draw_from_id_string(ent_name, C_NONE, "", x, y, corner, 0);

        // next up, draw all the overlays
        std::vector<std::string> overlays = p.get_overlay_ids();
        for(const std::string& overlay : overlays) {
            bool exists = true;
            std::string draw_id = p.male ? "overlay_male_" + overlay : "overlay_female_" + overlay;
            if (tile_ids.find(draw_id) == tile_ids.end()) {
                draw_id = "overlay_" + overlay;
                if(tile_ids.find(draw_id) == tile_ids.end()) {
                    exists = false;
                }
            }

            // make sure we don't draw an annoying "unknown" tile when we have nothing to draw
            if (exists) {
                draw_from_id_string(draw_id, C_NONE, "", x, y, corner, 0);
            }
        }
}

bool cata_tiles::draw_item_highlight(int x, int y)
{
    bool item_highlight_available = tile_ids.find(ITEM_HIGHLIGHT) != tile_ids.end();

    if (!item_highlight_available) {
        create_default_item_highlight();
        item_highlight_available = true;
    }
    return draw_from_id_string(ITEM_HIGHLIGHT, C_NONE, empty_string, x, y, 0, 0);
}

SDL_Surface *cata_tiles::create_tile_surface()
{
    SDL_Surface *surface;
    #if SDL_BYTEORDER == SDL_BIG_ENDIAN
        surface = SDL_CreateRGBSurface(0, tile_width, tile_height, 32, 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
    #else
        surface = SDL_CreateRGBSurface(0, tile_width, tile_height, 32, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
    #endif
    if( surface == nullptr ) {
        dbg( D_ERROR ) << "Failed to create surface: " << SDL_GetError();
    }
    return surface;
}

void cata_tiles::create_default_item_highlight()
{
    const Uint8 highlight_alpha = 127;

    std::string key = ITEM_HIGHLIGHT;
    int index = tile_values.size();

    SDL_Surface *surface = create_tile_surface();
    if( surface == nullptr ) {
        return;
    }
    SDL_FillRect(surface, NULL, SDL_MapRGBA(surface->format, 0, 0, 127, highlight_alpha));
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    if( texture == nullptr ) {
        dbg( D_ERROR ) << "Failed to create texture: " << SDL_GetError();
    }
    SDL_FreeSurface(surface);

    if( texture != nullptr ) {
    tile_values.push_back(texture);
    tile_type *type = new tile_type;
    type->fg = index;
    type->bg = -1;
    tile_ids[key] = type;
    }
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
        subtile = corner;
        rotation = 0;

        draw_from_id_string(exp_name, mx - i, my - i, subtile, rotation++);
        draw_from_id_string(exp_name, mx - i, my + i, subtile, rotation++);
        draw_from_id_string(exp_name, mx + i, my + i, subtile, rotation++);
        draw_from_id_string(exp_name, mx + i, my - i, subtile, rotation++);

        subtile = edge;
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
    const int mx = bul_pos_x, my = bul_pos_y;

    draw_from_id_string(bul_id, C_BULLET, empty_string, mx, my, 0, 0);
}
void cata_tiles::draw_hit_frame()
{
    const int mx = hit_pos_x, my = hit_pos_y;
    std::string hit_overlay = "animation_hit";

    draw_from_id_string(hit_entity_id, C_HIT_ENTITY, empty_string, mx, my, 0, 0);
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
    for( auto weather_iterator = anim_weather.vdrops.begin();
         weather_iterator != anim_weather.vdrops.end(); ++weather_iterator ) {
        // currently in ascii screen coordinates
        int x = weather_iterator->first + o_x;
        int y = weather_iterator->second + o_y;
        draw_from_id_string(weather_name, C_WEATHER, empty_string, x, y, 0, 0);
    }
}
void cata_tiles::draw_sct_frame()
{
    for( auto iter = SCT.vSCT.begin(); iter != SCT.vSCT.end(); ++iter ) {
        const int iDX = iter->getPosX();
        const int iDY = iter->getPosY();

        int iOffsetX = 0;

        for (int j=0; j < 2; ++j) {
            std::string sText = iter->getText((j == 0) ? "first" : "second");
            int FG = msgtype_to_tilecolor( iter->getMsgType((j == 0) ? "first" : "second"),
                                           (iter->getStep() >= SCT.iMaxSteps / 2) );

            for( std::string::iterator it = sText.begin(); it != sText.end(); ++it ) {
                std::string generic_id("ASCII_XFB");
                generic_id[6] = static_cast<char>(*it);
                generic_id[7] = static_cast<char>(FG);
                generic_id[8] = static_cast<char>(-1);

                if (tile_ids.count(generic_id) > 0) {
                    draw_from_id_string(generic_id, C_NONE, empty_string, iDX + iOffsetX, iDY, 0, 0);
                }

                iOffsetX++;
            }
        }
    }
}
void cata_tiles::draw_zones_frame()
{
    bool item_highlight_available = tile_ids.find(ITEM_HIGHLIGHT) != tile_ids.end();

    if (!item_highlight_available) {
        create_default_item_highlight();
        item_highlight_available = true;
    }

    for (int iY=pStartZone.y; iY <= pEndZone.y; ++iY) {
        for (int iX=pStartZone.x; iX <= pEndZone.x; ++iX) {
            draw_from_id_string(ITEM_HIGHLIGHT, C_NONE, empty_string, iX + pZoneOffset.x, iY + pZoneOffset.y, 0, 0);
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

LIGHTING cata_tiles::light_at(int x, int y)
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
            return BOOMER_DARK;
        } else {
            // exit w/ dark normal
            return LIGHT_DARK;
        }
    } else if (dist > sightrange_light && sight_impaired && lit == LL_BRIGHT) {
        if (boomered) {
            // exit w/ light boomerfication
            return BOOMER_NORMAL;
        } else {
            // exit w/ light normal
            return LIGHT_NORMAL;
        }
    } else if (dist <= u_clairvoyance || can_see) {
        // check for rain

        // return with okay to draw the square = 0
        return CLEAR;
    }

    return HIDDEN;
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
