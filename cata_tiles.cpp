#if (defined SDLTILES)
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

cata_tiles::cata_tiles()
{
    //ctor
    buffer = NULL;
    tile_atlas = NULL;
    tile_values = NULL;
    tile_ids = NULL;
    screen_tiles = NULL;
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
// may be useless, figure out and remove if necessary
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

        std::string output = "";

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
    int tilecount = 0;

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
        int t_fg, t_bg;
        t_fg = t_bg = -1;
        bool t_multi, t_rota;
        t_multi = t_rota = false;

        if (entry.has("fg")) t_fg = entry.get("fg").as_int();
        if (entry.has("bg")) t_bg = entry.get("bg").as_int();
        if (entry.has("multitile") && entry.get("multitile").is_bool())
        {
            t_multi = entry.get("multitle").as_bool();
        }
        if (t_multi)
        {
            t_rota = true;
            // fetch additional tiles
            if (entry.has("additional_tiles"))
            {
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

    int x, y, t;
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
                continue;
            }
            draw_furniture(x,y);
            draw_trap(x,y);
            draw_field_or_item(x,y);
            draw_vpart(x,y);
            draw_entity(x,y);
        }
    }

    // finally blit the buffer onto the screen
    /**
    SDL_Rect srcrect = {0,0,width,height};
	SDL_Rect destrect = {destx, desty, width, height};

	SDL_BlitSurface(bscreen, &srcrect, screen, &destrect);
    */
    //extern SDL_Surface *screen;
    SDL_Rect srcrect = {0,0,width, height};
    SDL_Rect desrect = {destx, desty, width, height};

    SDL_BlitSurface(buffer, &srcrect, display_screen, &desrect);
}

bool cata_tiles::draw_from_id_string(std::string id, int x, int y, int rota)
{
    tile_id_iterator it = tile_ids->find(id);
    // if id is not found, return false
    if (it == tile_ids->end()) return false;

    tile_type *display_tile = it->second;
    // if found id does not have a valid tile_type then return false
    if (!display_tile) return false;

    // translate from player-relative to screen relative tile position
    int screen_x = (x - o_x) * tile_width;
    int screen_y = (y - o_y) * tile_height;
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
            SDL_Rect *fgrect = (*tile_values)[fg];
            // get new surface of just the rotated fgrect and blit it to the screen
            SDL_Surface *rotatile = rotate_tile(tile_atlas, fgrect, rota);
            SDL_BlitSurface(rotatile, NULL, buffer, &destination);
            SDL_FreeSurface(rotatile);
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
    cx = (double)rect->w / 2;
    cy = (double)rect->h / 2;

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
    draw_from_id_string(light_name, x, y, 0);
}

bool cata_tiles::draw_terrain(int x, int y)
{
    int t = g->m.ter(x, y); // get the ter_id value at this point
	// check for null, if null return false
	if (t == t_null) return false;

	// need to check for walls, and then deal with wallfication details!
	int s = terlist[t].sym;
	bool wallchange = false;

	char alteration = 0;
// removed any special case things for making stuff rotate for now
	if (s == LINE_XOXO /*vertical*/ || s == LINE_OXOX /*horizontal*/)
	{
		//alteration = determine_wall_corner(g->m, realx, realy, LINE_XOXO, LINE_OXOX);
	}
	if (alteration > 0)
	{
		wallchange = true;
	}

	std::string tname;

    tname = terrain_names[t];
/*
    if (wallchange)
    {
        // change the suffix!
        unsigned found = tname.find_last_of("_");
        std::string tsub = tname.substr(0, found);
        tsub.append(wall_suffix[alteration-1]);

        tname = tsub;
    }
*/
    return draw_from_id_string(tname, x, y, 0);
}

bool cata_tiles::draw_furniture(int x, int y)
{
    // get furniture ID at x,y
    bool has_furn = g->m.has_furn(x,y);
    if (!has_furn) return false;

    int f_id = g->m.furn(x,y);
    // get the name of this furniture piece
    std::string f_name = furn_names[f_id]; // replace with furniture names array access

    return draw_from_id_string(f_name, x, y, 0); // for now just draw it normally, add in rotations later
    /*
    // get the tile that exists here
    tile_id_iterator it = tile_ids->find(f_name);

    if (it == tile_ids->end()) return false;

    tile_type *furn_tile = it->second;
    // make sure tile exists
    if (!furn_tile) return false;
    */
}

bool cata_tiles::draw_trap(int x, int y)
{
    int tr_id = g->m.tr_at(x,y);
    if (tr_id == tr_null) return false;

    std::string tr_name = trap_names[tr_id];

    return draw_from_id_string(tr_name, x, y, 0);
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
        return draw_from_id_string(fd_name, x, y, 0);
    }
    else
    {
        if (g->m.has_flag(container, x, y) || items.size() == 0)
        {
            return false;
        }
        // get the last item in the stack, it will be used for display
        item display_item = items[items.size() - 1];
        // get the item's name, as that is the key used to find it in the map
        std::string it_name = display_item.type->id;

        return draw_from_id_string(it_name, x, y, 0);
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
    const int veh_dir = (veh->face.dir4() + 1) % 4;
    // get the veh part itself
    vehicle_part vpart = veh->parts[veh_part];
    // get the vpart_id
    int vpid = vpart.id;

    std::string vp_name = veh_part_names[vpid];

    return draw_from_id_string(vp_name, x, y, veh_dir);
}

bool cata_tiles::draw_entity(int x, int y)
{
    // figure out what entity exists at x,y
    std::string ent_name;
    bool entity_here = false;
    // check for monster (most common)
    if (g->mon_at(x,y) >= 0)
    {
        monster m = g->z[g->mon_at(x,y)];
        if (!m.dead)
        {
            ent_name = monster_names[m.type->id];
            entity_here = true;
        }
    }
    // check for NPC (next most common)
    if (g->npc_at(x,y) > 0)
    {
        npc &m = *g->active_npc[g->npc_at(x,y)];
        if (!m.dead)
        {
            ent_name = m.male? "npc_male":"npc_female";
            entity_here = true;
        }
    }
    // check for PC (least common, only ever 1)
    if (g->u.posx == x && g->u.posy == y)
    {
        ent_name = g->u.male? "player_male":"player_female";
        entity_here = true;
    }
    if (entity_here)
    {
        return draw_from_id_string(ent_name, x, y, 0);
    }
    return false;
}


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

    int real_max_sight_range = sightrange_light > sightrange_max ? sightrange_light : sightrange_max;
    int distance_to_look = real_max_sight_range;

    if (OPTIONS[OPT_GRADUAL_NIGHT_LIGHT] > 0)
    {
        distance_to_look = DAYLIGHT_LEVEL;
    }

    bool can_see = g->m.pl_sees(g->u.posx, g->u.posy, x, y, distance_to_look);
    lit_level lit = g->m.light_at(x, y);

    if (OPTIONS[OPT_GRADUAL_NIGHT_LIGHT] > 0)
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
#endif // SDL_TILES
