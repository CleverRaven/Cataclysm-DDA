#if (defined SDLTILES)
#include <algorithm>
#include "cata_tiles.h"
#include "debug.h"

// SDL headers end up in different places depending on the OS, sadly
#if (defined SDLTILES)
    #if (defined _WIN32 || defined WINDOWS)
        #include "SDL_image.h" // Make sure to add this to the other OS inclusions
    #elif (defined OSX_SDL_FW)
        #include "SDL_image/SDL_image.h" // Make sure to add this to the other OS inclusions
    #else
        #include "SDL/SDL_image.h" // Make sure to add this to the other OS inclusions
    #endif
#endif

extern game *g;
//extern SDL_Surface *screen;
extern int WindowHeight, WindowWidth;
extern int fontwidth, fontheight;

cata_tiles::cata_tiles()
{
    //ctor
    buffer = NULL;
    tile_atlas = NULL;
    display_screen = NULL;
    tile_values = NULL;
    tile_ids = NULL;
    screen_tiles = NULL;

    tile_height = 0;
    tile_width = 0;
    screentile_height = 0;
    screentile_width = 0;

    in_animation = false;
    do_draw_explosion = false;
    do_draw_bullet = false;
    do_draw_hit = false;
    do_draw_line = false;
    do_draw_weather = false;
    boomered = false;
    sight_impaired = false;
    bionight_bionic_active = false;
}

cata_tiles::~cata_tiles()
{
    //dtor
    // free surfaces
    if (buffer)
    {
        SDL_FreeSurface(buffer);
    }
    if (tile_atlas)
    {
        SDL_FreeSurface(tile_atlas);
    }
    // release maps
    if (tile_values)
    {
        for (tile_iterator it = tile_values->begin(); it != tile_values->end(); ++it)
        {
            it->second = NULL;
        }
        tile_values->clear();
        tile_values = NULL;
    }
    if (tile_ids)
    {
        for (tile_id_iterator it = tile_ids->begin(); it != tile_ids->end(); ++it)
        {
            it->second = NULL;
        }
        tile_ids->clear();
        tile_ids = NULL;
    }
    if (screen_tiles)
    {
        for (int i = 0; i < num_tiles; ++i)
        {
            delete &(screen_tiles[i]);
        }
        screen_tiles = NULL;
    }
}

void cata_tiles::init(SDL_Surface *screen, std::string json_path, std::string tileset_path)
{
    display_screen = screen;
    // load files
    DebugLog() << "Attempting to Load JSON file\n";
    load_tilejson(json_path);
    DebugLog() << "Attempting to Load Tileset file\n";
    load_tileset(tileset_path);
    DebugLog() << "Attempting to Create Rotation Cache\n";
    create_rotation_cache();
// may be useless, figure out and remove if necessary
// removed for now since it's not used currently
/*
    // getting to here assumes that everything went okay, so figure out how many tiles we need to fill the screen
    num_tiles = WindowWidth * WindowHeight;
    screen_tiles = new tile[num_tiles];

    for (int y = 0; y < WindowHeight; ++y)
    {
        for (int x = 0; x < WindowWidth; ++x)
        {
            tile t(x, y, 0, 0);
            screen_tiles[x + y * WindowWidth] = t;
        }
    }
*/
}
void cata_tiles::init(SDL_Surface *screen, std::string load_file_path)
{
    std::string json_path, tileset_path;
    // get path information from load_file_path
    get_tile_information(load_file_path, json_path, tileset_path);
    // send this information to old init to avoid redundant code
    init(screen, json_path, tileset_path);
}
void cata_tiles::get_tile_information(std::string file_path, std::string &json_path, std::string &tileset_path)
{
    DebugLog() << "Attempting to Initialize JSON and TILESET path information from [" << file_path << "]\n";
    const std::string default_json = "gfx/tile_config.json";
    const std::string default_tileset = "gfx/tinytile.png";

    std::ifstream fin;
    fin.open(file_path.c_str());
    if(!fin.is_open()) {
        fin.close();
        DebugLog() << "\tCould not read ."<<file_path<<" -- Setting to default values!\n";
        json_path = default_json;
        tileset_path = default_tileset;
        return;
    }
// should only have 2 values inside it, otherwise is going to only load the last 2 values
    while(!fin.eof()) {
        std::string sOption;
        fin >> sOption;

        //DebugLog() << "\tCurrent: " << sOption << " -- ";

        if(sOption == "") {
            getline(fin, sOption);    // Empty line, chomp it
            //DebugLog() << "Empty line, skipping\n";
        } else if(sOption[0] == '#') { // # indicates a comment
            getline(fin, sOption);
            //DebugLog() << "Comment line, skipping\n";
        } else {
            //std::string sValue;


            //DebugLog() << "Value [" << sValue << "]\n";
/*
std::size_t found = str.find(str2);
  if (found!=std::string::npos)
    std::cout << "first 'needle' found at: " << found << '\n';

*/          if (sOption.find("JSON") != std::string::npos)
            {
                fin >> json_path;
                //json_path = sValue;
                DebugLog() << "\tJSON path set to [" << json_path <<"].\n";
            }
            else if (sOption.find("TILESET") != std::string::npos)
            {
                fin >> tileset_path;
                //tileset_path = sValue;
                DebugLog() << "\tTILESET path set to [" << tileset_path <<"].\n";
            }
        }
    }

    fin.close();
    if (json_path == "")
    {
        json_path = default_json;
        DebugLog() << "\tJSON set to default [" << json_path << "].\n";
    }
    if (tileset_path == "")
    {
        tileset_path = default_tileset;
        DebugLog() << "\tTILESET set to default [" << tileset_path << "].\n";
    }
}

void cata_tiles::load_tileset(std::string path)
{
    /** release buffer from memory if it has already been initialized */
    if (buffer)
    {
        SDL_FreeSurface(buffer);
    }
    /** release tile_atlas from memory if it has already been initialized */
    if (tile_atlas)
    {
        SDL_FreeSurface(tile_atlas);
    }
    /** create the buffer screen */
    buffer = SDL_AllocSurface(SDL_SWSURFACE, WindowWidth, WindowHeight, 32, 0xff0000, 0xff00, 0xff, 0);

    screentile_height = WindowHeight / tile_height;
    screentile_width = WindowWidth / tile_width;

DebugLog() << "Buffer Surface-- Width: " << buffer->w << " Height: " << buffer->h << "\n";
    /** reinit tile_atlas */
    tile_atlas = IMG_Load(path.c_str());
    if(!tile_atlas) {
        std::cerr << "Could not locate tileset file at " << path << std::endl;
        // TODO: run without tileset
    }

    /** Check to make sure the tile_atlas loaded correctly, will be NULL if didn't load */
    if (tile_atlas)
    {
        /** get dimensions of the atlas image */
        int w = tile_atlas->w;
        int h = tile_atlas->h;
        /** sx and sy will take care of any extraneous pixels that do not add up to a full tile */
        int sx = w / tile_width;
        int sy = h / tile_height;

        sx *= tile_width;
        sy *= tile_height;

        /** split the atlas into tiles using SDL_Rect structs instead of slicing the atlas into individual surfaces */
        int tilecount = 0;
        for (int y = 0; y < sy; y += tile_height)
        {
            for (int x = 0; x < sx; x += tile_width)
            {
                SDL_Rect *this_tile = new SDL_Rect();

                this_tile->w = tile_width;
                this_tile->h = tile_height;
                this_tile->x = x;
                this_tile->y = y;

                if (!tile_values)
                {
                    tile_values = new tile_map;
                }

                (*tile_values)[tilecount++] = this_tile;
            }
        }
        DebugLog() << "Tiles Created: " << tilecount << "\n";
    }
}

void cata_tiles::load_tilejson(std::string path)
{
    catajson config(path);

    if (!json_good())
    {
        //throw (std::string)"ERROR: " + path + (std::string)" could not be read.";
        DebugLog() << (std::string)"ERROR: " + path + (std::string)" could not be read.\n";
        return;
    }

    /** 1) Mae sure that the loaded file has the "tile_info" section */
    if (!config.has("tile_info"))
    {
        DebugLog() << (std::string)"ERROR: "+path+ (std::string)" --\"tile_info\" missing\n";
        //throw (std::string)"ERROR: "+path+ (std::string)" --\"tile_info\" missing";
        return;
    }
    catajson info = config.get("tile_info");
    //int tilecount = 0;

    for (info.set_begin(); info.has_curr(); info.next())
    {
        catajson curr_info = info.curr();

        if (!curr_info.has("height") || !curr_info.has("width") || !curr_info.get("height").is_number() || !curr_info.get("width").is_number())
        {
            continue;
        }
        tile_height = curr_info.get("height").as_int();
        tile_width = curr_info.get("width").as_int();
    }
    /*
    if (!info.has("height") || !info.has("width") || !info.get("height").is_number() || !info.get("width").is_number())
    {
        throw (std::string)"ERROR: "+path+ (std::string)" --malformed \"height\" or \"width\" gat in \"tile_info\"";
    }
    tile_height = info.get("height").as_int();
    tile_width = info.get("width").as_int();
    */
    /** 2) Load tile information if available */
    if (!config.has("tiles"))
    {
        //throw (std::string)"ERROR: "+path+ (std::string)" --No tiles to load";
        return;
    }
    catajson tiles = config.get("tiles");

    for (tiles.set_begin(); tiles.has_curr(); tiles.next())
    {
        catajson entry = tiles.curr();
        if (!entry.has("id") || !entry.get("id").is_string())
        {
            continue;
        }
//popup_nowait(_("Please wait as the tileset loads [%3d]"), tilecount++);

        tile_type *curr_tile = new tile_type();
        std::string t_id = entry.get("id").as_string();
        if (t_id == "explosion")
        {
            DebugLog() << "Explosion tile id found\n";
        }
        int t_fg, t_bg;
        t_fg = t_bg = -1;
        bool t_multi, t_rota;
        t_multi = t_rota = false;

        if (entry.has("fg")) t_fg = entry.get("fg").as_int();
        if (entry.has("bg")) t_bg = entry.get("bg").as_int();
        if (entry.has("multitile") && entry.get("multitile").is_bool())
        {
            t_multi = entry.get("multitile").as_bool();
            //DebugLog() << "Multitile Definition Found: \""<<t_id << "\"\n";
        }
        if (t_multi)
        {
            t_rota = true;
            if (t_id == "explosion")
            {
                DebugLog() << "--Explosion is Multitile\n";
            }
/*
catajson compList = curr.get("components");
for (compList.set_begin(); compList.has_curr(); compList.next())
{
    ++cl;
    std::vector<component> component_choices;
    catajson comp = compList.curr();
    // interchangable components
    for (comp.set_begin(); comp.has_curr(); comp.next())
    {
        std::string name = comp.curr().get(0).as_string();
        int quant = comp.curr().get(1).as_int();
        component_choices.push_back(component(name, quant));
    }
    last_rec->components.push_back(component_choices);
}
*/
            // fetch additional tiles
            if (entry.has("additional_tiles"))
            {
                //DebugLog() << "Tile: \"" << t_id << "\" has Subtiles!\n";
                catajson subentries = entry.get("additional_tiles");

                for (subentries.set_begin(); subentries.has_curr(); subentries.next())
                {
                    catajson subentry = subentries.curr();

                    if (!subentry.has("id") || !subentry.get("id").is_string())
                    {
                        continue;
                    }

                    tile_type *curr_subtile = new tile_type();
                    std::string s_id = subentry.get("id").as_string();
                    int s_fg, s_bg;
                    s_fg = t_fg;
                    s_bg = t_bg;
                    if (subentry.has("fg")) s_fg = subentry.get("fg").as_int();
                    if (subentry.has("bg")) s_bg = subentry.get("bg").as_int();

                    curr_subtile->fg = s_fg;
                    curr_subtile->bg = s_bg;
                    curr_subtile->rotates = true;
                    std::string m_id = t_id + "_" + s_id;

                    if (!tile_ids)
                    {
                        tile_ids = new tile_id_map;
                    }
                    (*tile_ids)[m_id] = curr_subtile;
                    curr_tile->available_subtiles.push_back(s_id);
                    if (t_id == "explosion")
                    {
                        DebugLog() << "--Explosion subtile ID Added: ["<< s_id << "] with value: [" << m_id << "]\n";
                    }
                    //DebugLog() << "\tSubtile: \""<<s_id<<"\" written\n";
                }
            }
        }
        else if (entry.has("rotates") && entry.get("rotates").is_bool())
        {
            t_rota = entry.get("rotates").as_bool();
        }

        // write the information of the base tile to curr_tile
        curr_tile->bg = t_bg;
        curr_tile->fg = t_fg;
        curr_tile->multitile = t_multi;
        curr_tile->rotates = t_rota;

        if (!tile_ids)
        {
            tile_ids = new tile_id_map;
        }
        (*tile_ids)[t_id] = curr_tile;
    }
    DebugLog() << "Tile Width: " << tile_width << " Tile Height: " << tile_height << " Tile Definitions: " << tile_ids->size() << "\n";
}

void cata_tiles::create_rotation_cache()
{
    /*
    Each tile has the potential of having 3 additional rotational values applied to it.
    0 is the default tile in the tileset
    1 is an East rotation
    2 is a South rotation
    3 is a West rotation
    These rotations are stored in a map<tile number, vector<SDL_Surface*> > with 3 values relating to east, south, and west in that order
    */
    /*
    if (fg >= 0 && fg < tile_values->size())
    {
        // get rect
        SDL_Rect *fgrect = (*tile_values)[fg];
        // get new surface of just the rotated fgrect and blit it to the screen
        SDL_Surface *rotatile = rotate_tile(tile_atlas, fgrect, rota);
        SDL_BlitSurface(rotatile, NULL, buffer, &destination);
        SDL_FreeSurface(rotatile);
    }
    */
    for (tile_iterator it = tile_values->begin(); it != tile_values->end(); ++it)
    {
        const int tile_num = it->first;
        SDL_Rect *tile_rect = it->second;

        std::vector<SDL_Surface*> rotations;
        for (int i = 1; i < 4; ++i)
        {
            rotations.push_back(rotate_tile(tile_atlas, tile_rect, i));
        }
        rotation_cache[tile_num] = rotations;
        //DebugLog() << "Tile ["<<tile_num<<"] rotations added\n";
    }
}

void cata_tiles::draw()
{
    /** steps to drawing
    1) make sure g exists
    2) Clear the buffer
    3) get player position
    4) Init lighting
    5) Iterate through tile array and draw them to screen
        a) Draw lighting if necessary
        b) Draw terrain
        c) Draw Furniture
        d) Draw Traps
        e) Draw Fields | Draw Items
        f) Draw Entities
    */
    // Check existance of g
    if (!g)
    {
        return;
    }
    // Clear buffer
    SDL_FillRect(buffer, NULL, 0);

    // get player position

    // init lighting
    init_light();
}
void cata_tiles::draw(int destx, int desty, int centerx, int centery, int width, int height)
{
    if (!g) return;
    SDL_FillRect(buffer, NULL, 0);

    int posx = centerx;
    int posy = centery;

    int sx = ceil((double)width / tile_width);
    int sy = ceil((double)height / tile_height);

    init_light();

    int x, y;
    LIGHTING l;

    o_x = posx - sx/2;
    o_y = posy - sy/2;

    for (int my = 0; my < sy; ++my)
    {
        for (int mx = 0; mx < sx; ++mx)
        {
            x = mx + o_x;
            y = my + o_y;
            l = light_at(x, y);

            if (l != CLEAR)
            {
                // Draw lighting
                draw_lighting(x,y,l);
                // continue on to next part of loop
                continue;
            }
            // light is no longer being considered, for now.
            // Draw Terrain if possible. If not possible then we need to continue on to the next part of loop
            if (!draw_terrain(x,y))
            {
                //DebugLog() << "Terrain Failed to load at <"<<x<<", "<<y<<">\n";
                continue;
            }
            draw_furniture(x,y);
            draw_trap(x,y);
            draw_field_or_item(x,y);
            draw_vpart(x,y);
            draw_entity(x,y);
        }
    }
    in_animation = do_draw_explosion || do_draw_bullet || do_draw_hit || do_draw_line || do_draw_weather;
    if (in_animation)
    {
        if (do_draw_explosion)
        {
            draw_explosion_frame(destx, desty, centerx, centery, width, height);
        }
        if (do_draw_bullet)
        {
            draw_bullet_frame(destx, desty, centerx, centery, width, height);
        }
        if (do_draw_hit)
        {
            draw_hit_frame(destx, desty, centerx, centery, width, height);
            void_hit();
        }
        if (do_draw_line)
        {
            draw_line(destx, desty, centerx, centery, width, height);
            void_line();
        }
        if (do_draw_weather)
        {
            draw_weather_frame(destx, desty, centerx, centery, width, height);
            void_weather();
        }
    }
    // check to see if player is located at ter
    else if (g->u.posx + g->u.view_offset_x != g->ter_view_x || g->u.posy + g->u.view_offset_y != g->ter_view_y)
    {
        draw_from_id_string("cursor", g->ter_view_x, g->ter_view_y, 0, 0);
        //draw_explosion_frame(1, 1, 1, 0, 0);
        //draw_from_id_string("explosion", posx - 1, posy - 1, corner, 0);
    }

    //extern SDL_Surface *screen;
    SDL_Rect srcrect = {0, 0, (Uint16)width, (Uint16)height};
    SDL_Rect desrect = {(Sint16)destx, (Sint16)desty, (Uint16)width, (Uint16)height};

    SDL_BlitSurface(buffer, &srcrect, display_screen, &desrect);
}

bool cata_tiles::draw_from_id_string(std::string id, int x, int y, int subtile, int rota, bool is_at_screen_position)
{
    // For the moment, if the ID string does not produce a drawable tile it will revert to the "unknown" tile.
    // The "unknown" tile is one that is highly visible so you kinda can't miss it :D

    // check to make sure that we are drawing within a valid area [0->width|height / tile_width|height]
    if (is_at_screen_position && (x < 0 || x > screentile_width || y < 0 || y > screentile_height)) return false;
    else if (!is_at_screen_position && ((x-o_x) < 0 || (x-o_x) > screentile_width || (y-o_y) < 0 || (y-o_y) > screentile_height)) return false;

    tile_id_iterator it = tile_ids->find(id);
    // if id is not found, return unknown tile
    if (it == tile_ids->end()) return draw_from_id_string("unknown", x, y, subtile, rota, is_at_screen_position);

    tile_type *display_tile = it->second;
    // if found id does not have a valid tile_type then return unknown tile
    if (!display_tile) return draw_from_id_string("unknown", x, y, subtile, rota, is_at_screen_position);

    // if both bg and fg are -1 then return unknown tile
    if (display_tile->bg == -1 && display_tile->fg == -1) return draw_from_id_string("unknown", x, y, subtile, rota, is_at_screen_position);

    // check to see if the display_tile is multitile, and if so if it has the key related to subtile
    if (subtile != -1 && display_tile->multitile)
    {
        std::vector<std::string> display_subtiles = display_tile->available_subtiles;
        if (std::find(display_subtiles.begin(), display_subtiles.end(), multitile_keys[subtile]) != display_subtiles.end())
        {
            //DebugLog() << "Changing tile ID from <" << id << "> to ";
            // append subtile name to tile and re-find display_tile
            id += "_" + multitile_keys[subtile];
            //DebugLog() << "<"<< id << ">\n";
            return draw_from_id_string(id, x, y, -1, rota);
        }
        /*
        if (display_tile->available_subtiles.find(multitile_keys[subtile]) != display_tile->available_subtiles.end())
        {
            // append subtile name to tile and re-find display_tile
            id += "_" + multitile_keys[subtile];
            draw_from_id_string(id, x, y, -1, rota);
        }
        */
    }

    // make sure we aren't going to rotate the tile if it shouldn't be rotated
    if (!display_tile->rotates)
    {
        rota = 0;
    }

    // translate from player-relative to screen relative tile position
    int screen_x;// = (x - o_x) * tile_width;
    int screen_y;// = (y - o_y) * tile_height;
    if (!is_at_screen_position)
    {
        screen_x = (x - o_x) * tile_width;
        screen_y = (y - o_y) * tile_height;
    }
    else
    {
        screen_x = x * tile_width;
        screen_y = y * tile_width;
    }
//DebugLog() << "Drawing Tile: [" << id << "] at <" << x << ", " << y << "> FG: "<< display_tile->fg << " BG: " << display_tile->bg << "\n";
    // call to draw_tile
    return draw_tile_at(display_tile, screen_x, screen_y, rota);
}

bool cata_tiles::draw_tile_at(tile_type* tile, int x, int y, int rota)
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
    if (bg >= 0 && bg < tile_values->size())
    {
        SDL_Rect *bgrect = (*tile_values)[bg];
        SDL_BlitSurface(tile_atlas, bgrect, buffer, &destination);
    }
    // blit foreground based on rotation
    if (rota == 0)
    {
        if (fg >= 0 && fg < tile_values->size())
        {
            SDL_Rect *fgrect = (*tile_values)[fg];
            SDL_BlitSurface(tile_atlas, fgrect, buffer, &destination);
        }
    }
    else
    {
        if (fg >= 0 && fg < tile_values->size())
        {
            // get rect
            //SDL_Rect *fgrect = (*tile_values)[fg];
            //DebugLog() << "Drawing rotated tile [" << fg << "] with rotation value ["<< rota << "]\n";
            // get new surface of just the rotated fgrect and blit it to the screen
            std::vector<SDL_Surface*> fgtiles = rotation_cache[fg];
            //DebugLog() << "\tAvailable Rotation tiles [" << fgtiles.size() << "]\n";
            SDL_Surface *rotatile = fgtiles[rota - 1];
            //SDL_Surface *rotatile = rotate_tile(tile_atlas, fgrect, rota);
            SDL_BlitSurface(rotatile, NULL, buffer, &destination);
            //SDL_FreeSurface(rotatile);
        }
    }

    return true;
}
/**
    may be able to cache results for each rotation using a double dx,dy pair. Would certainly increase speed at the expense of:
    [4 * tile_width * tile_height * (2*sizeof(double))] memory use.
    0 rota would have all zeros
    1 rota would have a 90 degree right rotation
    2 rota would have a 180 degree rotation
    3 rota would have a 90 degree left (270 right) rotation
*/
SDL_Surface *cata_tiles::rotate_tile(SDL_Surface *src, SDL_Rect* rect, int rota)
{
    if (!src || !rect || rect->w == 0 || rect->h == 0)
    {
        return NULL;
    }
    double cx, cy;
    cx = (double)(rect->w-1) / 2;
    cy = (double)(rect->h-1) / 2;

    double X, Y;
    double X2, Y2;

    double cosAngle, sinAngle;

    SDL_Surface *ret = SDL_CreateRGBSurface(src->flags, rect->w, rect->h, src->format->BitsPerPixel, src->format->Rmask, src->format->Gmask, src->format->Bmask, src->format->Amask);

    // simple rotational values wooo!
    switch(rota)
    {
        case 0: cosAngle = 1; sinAngle = 0; break;
        case 1: cosAngle = 0; sinAngle = 1; break;
        case 2: cosAngle = -1; sinAngle = 0; break;
        case 3: cosAngle = 0; sinAngle = -1; break;
        default: cosAngle = 0; sinAngle = 0; break;
    }

    for (int x = 0; x < rect->w; ++x)
    {
        for (int y = 0; y < rect->h; ++y)
        {
            X = (double)x - cx;
            Y = (double)y - cy;
            X2 = (X * cosAngle - Y * sinAngle);
            Y2 = (X * sinAngle + Y * cosAngle);
            X2 += cx + rect->x;
            Y2 += cy + rect->y;

            if( (int)X2 >= (int)src->w || (int)X2 < 0 || (int)Y2 >= src->h || (int)Y2 < 0) put_pixel(ret, x, y, SDL_MapRGB(src->format, 255, 0, 255));
            else put_pixel(ret, x, y, get_pixel(src, X2, Y2));
        }
    }

    return ret;
}
Uint32 cata_tiles::get_pixel(SDL_Surface *surface, int x, int y)
{
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to retrieve */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

    switch(bpp) {
    case 1:
        return *p;
        break;

    case 2:
        return *(Uint16 *)p;
        break;

    case 3:
        if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
            return p[0] << 16 | p[1] << 8 | p[2];
        else
            return p[0] | p[1] << 8 | p[2] << 16;
        break;

    case 4:
        return *(Uint32 *)p;
        break;

    default:
        return 0;       /* shouldn't happen, but avoids warnings */
    }
}
void cata_tiles::put_pixel(SDL_Surface *surface, int x, int y, Uint32 pixel)
{
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to set */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

    switch(bpp) {
    case 1:
        *p = pixel;
        break;

    case 2:
        *(Uint16 *)p = pixel;
        break;

    case 3:
        if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
            p[0] = (pixel >> 16) & 0xff;
            p[1] = (pixel >> 8) & 0xff;
            p[2] = pixel & 0xff;
        } else {
            p[0] = pixel & 0xff;
            p[1] = (pixel >> 8) & 0xff;
            p[2] = (pixel >> 16) & 0xff;
        }
        break;

    case 4:
        *(Uint32 *)p = pixel;
        break;
    }
}

bool cata_tiles::draw_lighting(int x, int y, LIGHTING l)
{
    std::string light_name;
    switch(l)
    {
        case HIDDEN:        light_name = "lighting_hidden"; break;
        case LIGHT_NORMAL:  light_name = "lighting_lowlight_light"; break;
        case LIGHT_DARK:    light_name = "lighting_lowlight_dark"; break;
        case BOOMER_NORMAL: light_name = "lighting_boomered_light"; break;
        case BOOMER_DARK:   light_name = "lighting_boomered_dark"; break;
    }

    // lighting is never rotated, though, could possibly add in random rotation?
    draw_from_id_string(light_name, x, y, 0, 0);

    return false;
}

bool cata_tiles::draw_terrain(int x, int y)
{
    int t = g->m.ter(x, y); // get the ter_id value at this point
	// check for null, if null return false
	if (t == t_null) return false;

	// need to check for walls, and then deal with wallfication details!
	int s = terlist[t].sym;
	//bool wallchange = false;

	//char alteration = 0;
	int subtile = 0, rotation = 0;

    // check walls
	if (s == LINE_XOXO /*vertical*/ || s == LINE_OXOX /*horizontal*/)
	{
		get_wall_values(x, y, LINE_XOXO, LINE_OXOX, subtile, rotation);
		//wallchange = true;
	}
	// check windows and doors for wall connections, may or may not have a subtile available, but should be able to rotate to some extent
	else if (s == '"' || s == '+' || s == '\'')
    {
        get_wall_values(x, y, LINE_XOXO, LINE_OXOX, subtile, rotation);
    }
    else
    {
        get_terrain_orientation(x, y, rotation, subtile);
        // do something to get other terrain orientation values
    }

	std::string tname;

    tname = terrain_names[t];

    return draw_from_id_string(tname, x, y, subtile, rotation);
}

bool cata_tiles::draw_furniture(int x, int y)
{
    // get furniture ID at x,y
    bool has_furn = g->m.has_furn(x,y);
    if (!has_furn) return false;

    int f_id = g->m.furn(x,y);

    // for rotation inforomation
    const int neighborhood[4] =
    {
        g->m.furn(x, y+1), // south
        g->m.furn(x+1, y), // east
        g->m.furn(x-1, y), // west
        g->m.furn(x, y-1)  // north
    };

    int subtile = 0, rotation = 0;
    get_tile_values(f_id, neighborhood, subtile, rotation);

    // get the name of this furniture piece
    std::string f_name = furn_names[f_id]; // replace with furniture names array access

    return draw_from_id_string(f_name, x, y, subtile, rotation); // for now just draw it normally, add in rotations later
}

bool cata_tiles::draw_trap(int x, int y)
{
    int tr_id = g->m.tr_at(x,y);
    if (tr_id == tr_null) return false;

    std::string tr_name = trap_names[tr_id];

    const int neighborhood[4] =
    {
        g->m.tr_at(x, y+1), // south
        g->m.tr_at(x+1, y), // east
        g->m.tr_at(x-1, y), // west
        g->m.tr_at(x, y-1)  // north
    };

    int subtile = 0, rotation = 0;
    get_tile_values(tr_id, neighborhood, subtile, rotation);

    return draw_from_id_string(tr_name, x, y, subtile, rotation);
}

bool cata_tiles::draw_field_or_item(int x, int y)
{
    // check for field
    field f = g->m.field_at(x,y);
    // check for items
    std::vector<item> items = g->m.i_at(x, y);
    bool do_item = f.fieldSymbol() == fd_null;

    if (!do_item)
    {
        std::string fd_name = field_names[f.fieldSymbol()];

        // for rotation inforomation
        const int neighborhood[4] =
        {
            g->m.field_at(x, y+1).fieldSymbol(), // south
            g->m.field_at(x+1, y).fieldSymbol(), // east
            g->m.field_at(x-1, y).fieldSymbol(), // west
            g->m.field_at(x, y-1).fieldSymbol()  // north
        };

        int subtile = 0, rotation = 0;
        get_tile_values(f.fieldSymbol(), neighborhood, subtile, rotation);

        return draw_from_id_string(fd_name, x, y, subtile, rotation);
    }
    else
    {
        if (g->m.has_flag(container, x, y) || items.empty())
        {
            return false;
        }
        // get the last item in the stack, it will be used for display
        item display_item = items[items.size() - 1];
        // get the item's name, as that is the key used to find it in the map
        std::string it_name = display_item.type->id;

        return draw_from_id_string(it_name, x, y, 0, 0);
    }
}
/** Deprecated: combined with field drawing as they are mutex */
bool cata_tiles::draw_item(int x, int y)
{
    return false;
}
bool cata_tiles::draw_vpart(int x, int y)
{
    int veh_part = 0;
    vehicle *veh = g->m.veh_at(x, y, veh_part);

    if (!veh) return false;
    // veh_part is the index of the part
    // get a north-east-south-west value instead of east-south-west-north value to use with rotation
    int veh_dir = (veh->face.dir4() + 1) % 4;
    if (veh_dir == 1 || veh_dir == 3) veh_dir = (veh_dir + 2) % 4;
    // get the veh part itself
    vehicle_part vpart = veh->parts[veh_part];
    // get the vpart_id
    int vpid = vpart.id;

    std::string vp_name = veh_part_names[vpid];

    return draw_from_id_string(vp_name, x, y, 0, veh_dir);
}

bool cata_tiles::draw_entity(int x, int y)
{
    // figure out what entity exists at x,y
    std::string ent_name;
    bool entity_here = false;
    // check for monster (most common)
    if (!entity_here && g->mon_at(x,y) >= 0)
    {
        monster m = g->zombie(g->mon_at(x,y));
        if (!m.dead)
        {
            ent_name = monster_names[m.type->id];
            entity_here = true;
        }
    }
    // check for NPC (next most common)
    if (!entity_here && g->npc_at(x,y) >= 0)
    {
        npc &m = *g->active_npc[g->npc_at(x,y)];
        if (!m.dead)
        {
            ent_name = m.male? "npc_male":"npc_female";
            entity_here = true;
        }
    }
    // check for PC (least common, only ever 1)
    if (!entity_here && g->u.posx == x && g->u.posy == y)
    {
        ent_name = g->u.male? "player_male":"player_female";
        entity_here = true;
    }
    if (entity_here)
    {
        int subtile = corner;
        return draw_from_id_string(ent_name, x, y, subtile, 0);
    }
    return false;
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
/* -- Animation Renders */
void cata_tiles::draw_explosion_frame(int destx, int desty, int centerx, int centery, int width, int height)
{
    std::string exp_name = "explosion";
    int subtile, rotation;
    const int mx = exp_pos_x, my = exp_pos_y;

    //mx = exp_pos_x;// + offset_x;
    //my = exp_pos_y;// + offset_y;

    for (int i = 1; i < exp_rad; ++i)
    {
        subtile = corner;
        rotation = 0;

        draw_from_id_string(exp_name, mx - i, my - i, subtile, rotation++);
        draw_from_id_string(exp_name, mx - i, my + i, subtile, rotation++);
        draw_from_id_string(exp_name, mx + i, my + i, subtile, rotation++);
        draw_from_id_string(exp_name, mx + i, my - i, subtile, rotation++);

        subtile = edge;
        for (int j = 1 - i; j < 0 + i; j++)
        {
            rotation = 0;
            draw_from_id_string(exp_name, mx + j, my - i, subtile, rotation);
            draw_from_id_string(exp_name, mx + j, my + i, subtile, rotation);

            rotation = 1;
            draw_from_id_string(exp_name, mx - i, my + j, subtile, rotation);
            draw_from_id_string(exp_name, mx + i, my + j, subtile, rotation);
        }
    }
}
void cata_tiles::draw_bullet_frame(int destx, int desty, int centerx, int centery, int width, int height)
{
    const int mx = bul_pos_x, my = bul_pos_y;

    draw_from_id_string(bul_id, mx, my, 0, 0);
}
void cata_tiles::draw_hit_frame(int destx, int desty, int centerx, int centery, int width, int height)
{
    const int mx = hit_pos_x, my = hit_pos_y;
    std::string hit_overlay = "animation_hit";

    draw_from_id_string(hit_entity_id, mx, my, 0, 0);
    draw_from_id_string(hit_overlay, mx, my, 0, 0);
}
void cata_tiles::draw_line(int destx, int desty, int centerx, int centery, int width, int height)
{
    int mx = line_pos_x, my = line_pos_y;
    std::string line_overlay = "animation_line";
    if (!is_target_line || g->u_see(mx,my))
    {
        for (int i = 0; i < line_trajectory.size()-1; i++)
        {
            mx = line_trajectory[i].x;
            my = line_trajectory[i].y;

            draw_from_id_string(line_overlay, mx, my, 0, 0);
        }
    }
    mx = line_trajectory[line_trajectory.size()-1].x;
    my = line_trajectory[line_trajectory.size()-1].y;

    draw_from_id_string(line_endpoint_id, mx, my, 0, 0);
}
void cata_tiles::draw_weather_frame(int destx, int desty, int centerx, int centery, int width, int height)
{
    for (std::vector<std::pair<int, int> >::iterator weather_iterator = anim_weather.vdrops.begin();
         weather_iterator != anim_weather.vdrops.end();
         ++weather_iterator)
    {
        // currently in ascii screen coordinates
        int x = weather_iterator->first;
        int y = weather_iterator->second;

        x = x + g->ter_view_x - getmaxx(g->w_terrain)/2;
        y = y + g->ter_view_y - getmaxy(g->w_terrain)/2;

        draw_from_id_string(weather_name, x, y,0, 0, false);
    }
}
/* END OF ANIMATION FUNCTIONS */

void cata_tiles::init_light()
{
    g->m.build_map_cache(g);
    g->reset_light_level();

    sightrange_natural = g->u.sight_range(1);
    g_lightlevel = (int)g->light_level();
    sightrange_light = g->u.sight_range(g_lightlevel);
    sightrange_lowlight = std::max(g_lightlevel/2, sightrange_natural);
    sightrange_max = g->u.unimpaired_range();
    u_clairvoyance = g->u.clairvoyance();

    boomered = g->u.has_disease("boomered");
    sight_impaired = g->u.sight_impaired();
    bionight_bionic_active = g->u.has_active_bionic("bio_night");
}

LIGHTING cata_tiles::light_at(int x, int y)
{
    /** Logic */
    const int dist = rl_dist(g->u.posx, g->u.posy, x, y);
    /*
    int sight_range = sightrange_light;
    int low_sight_range = sightrange_lowlight;

    if (!g->m.is_outside(x, y))
    {
        sight_range = sightrange_natural;
    }
    else if (dist <= g->u.sight_range(g_lightlevel))
    {
        low_sight_range = std::max(g_lightlevel, sightrange_natural);
    }
    */

    int real_max_sight_range = sightrange_light > sightrange_max ? sightrange_light : sightrange_max;
    int distance_to_look = real_max_sight_range;

    if (true)
    //if (OPTIONS[OPT_GRADUAL_NIGHT_LIGHT] > 0)
    {
        distance_to_look = DAYLIGHT_LEVEL;
    }

    bool can_see = g->m.pl_sees(g->u.posx, g->u.posy, x, y, distance_to_look);
    lit_level lit = g->m.light_at(x, y);

    if (true)
    //if (OPTIONS[OPT_GRADUAL_NIGHT_LIGHT] > 0)
    {
        if (lit != LL_BRIGHT && dist > real_max_sight_range)
        {
            int intlit = (int)lit - (dist - real_max_sight_range)/2;
            if (intlit < 0) intlit = LL_DARK;
            lit = (lit_level)intlit;
        }

        if (lit > LL_DARK && real_max_sight_range > 1)
        {
            real_max_sight_range = distance_to_look;
        }
    }

    /** Conditional Returns */
    if ((bionight_bionic_active && dist < 15 && dist > sightrange_natural) ||
        dist > real_max_sight_range ||
        (dist > sightrange_light &&
          (lit == LL_DARK ||
           (sight_impaired && lit != LL_BRIGHT)||
           !can_see)))
    {
        if (boomered)
        {
            // exit w/ dark boomerfication
            return BOOMER_DARK;
        }
        else
        {
            // exit w/ dark normal
            return LIGHT_DARK;
        }
    }
    else if (dist > sightrange_light && sight_impaired && lit == LL_BRIGHT)
    {
        if (boomered)
        {
            // exit w/ light boomerfication
            return BOOMER_NORMAL;
        }
        else
        {
            // exit w/ light normal
            return LIGHT_NORMAL;
        }
    }
    else if (dist <= u_clairvoyance || can_see)
    {
        // check for rain

        // return with okay to draw the square = 0
        return CLEAR;
    }

    return HIDDEN;
}

void cata_tiles::get_terrain_orientation(int x, int y, int& rota, int& subtile)
{
    // get terrain at x,y
    ter_id tid = g->m.ter(x, y);
    if (tid == t_null)
    {
        subtile = 0;
        rota = 0;
        return;
    }

    // get terrain neighborhood
    const ter_id neighborhood[4] =
    {
        g->m.ter(x, y+1), // south
        g->m.ter(x+1, y), // east
        g->m.ter(x-1, y), // west
        g->m.ter(x, y-1)  // north
    };

    bool connects[4];
    char val = 0;
    int num_connects = 0;

    // populate connection information
    for (int i = 0; i < 4; ++i)
    {
        connects[i] = (neighborhood[i] == tid);

        if (connects[i])
        {
            ++num_connects;
            val += 1 << i;
        }
    }

    get_rotation_and_subtile(val, num_connects, rota, subtile);
}
void cata_tiles::get_rotation_and_subtile(const char val, const int num_connects, int &rotation, int &subtile)
{
	switch(num_connects)
    {
        case 0: rotation = 0; subtile = unconnected; break;
        case 4: rotation = 0; subtile = center; break;
        case 1: // all end pieces
            subtile = end_piece;
            switch(val)
            {
                case 8: rotation = 2; break;
                case 4: rotation = 3; break;
                case 2: rotation = 1; break;
                case 1: rotation = 0; break;
            }
            break;
        case 2:
            switch(val)
            {// edges
                case 9: subtile = edge; rotation = 0; break;
                case 6: subtile = edge; rotation = 1; break;
             // corners
                case 12: subtile = corner; rotation = 2; break;
                case 10: subtile = corner; rotation = 1; break;
                case 3:  subtile = corner; rotation = 0; break;
                case 5:  subtile = corner; rotation = 3; break;
            }
            break;
        case 3: // all t_connections
            subtile = t_connection;
            switch(val)
            {
                case 14: rotation = 2; break;
                case 11: rotation = 1; break;
                case 7:  rotation = 0; break;
                case 13: rotation = 3; break;
            }
            break;
    }
}
void cata_tiles::get_wall_values(const int x, const int y, const long vertical_wall_symbol, const long horizontal_wall_symbol, int& subtile, int& rotation)
{
    // makes the assumption that x,y is a wall | window | door of some sort
    const long neighborhood[4] = {
		terlist[g->m.ter(x, y+1)].sym, // south
		terlist[g->m.ter(x+1, y)].sym, // east
		terlist[g->m.ter(x-1, y)].sym, // west
		terlist[g->m.ter(x, y-1)].sym  // north
	};

	bool connects[4];

	char val = 0;
	int num_connects = 0;

	// populate connection information
	for (int i = 0; i < 4; ++i)
	{
		connects[i] = (neighborhood[i] == vertical_wall_symbol || neighborhood[i] == horizontal_wall_symbol) || (neighborhood[i] == '"' || neighborhood[i] == '+' || neighborhood[i] == '\'');

		if (connects[i])
		{
		    ++num_connects;
			val += 1 << i;
		}
	}
	get_rotation_and_subtile(val, num_connects, rotation, subtile);
    //DebugLog() << "Expected Wall Tile: <" << multitile_keys[subtile] << "-" << rotation << "> at <" << x << ", " << y << ">\n";
}

void cata_tiles::get_tile_values(const int t, const int *tn, int &subtile, int &rotation)
{
    bool connects[4];
    int num_connects = 0;
    char val = 0;
    for (int i = 0; i < 4; ++i)
    {
        connects[i] = (tn[i] == t);
        if (connects[i])
        {
            ++num_connects;
            val += 1 << i;
        }
    }
    get_rotation_and_subtile(val, num_connects, rotation, subtile);
}
#endif // SDL_TILES
