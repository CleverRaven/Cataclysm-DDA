#if (defined TILES)
#include "catacurse.h"
#include "options.h"
#include "output.h"
#include "input.h"
#include "color.h"
#include "catacharset.h"
#include "cursesdef.h"
#include "debug.h"
#include <vector>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <stdexcept>
#include "cata_tiles.h"
#include "get_version.h"
#include "init.h"
#include "path_info.h"
#include "filesystem.h"

//TODO replace these includes with filesystem.h
#ifdef _MSC_VER
#   include "wdirent.h"
#   include <direct.h>
#else
#   include <dirent.h>
#endif

#if (defined _WIN32 || defined WINDOWS)
#   include "platform_win.h"
#   include <shlwapi.h>
#   ifndef strcasecmp
#       define strcasecmp StrCmpI
#   endif
#else
#   include <wordexp.h>
#endif

#include "SDL2/SDL.h"
#include "SDL2/SDL_ttf.h"
#ifdef SDLTILES
#include "SDL2/SDL_image.h"
#endif

#ifdef SDL_SOUND
#include "SDL_mixer.h"
#endif

#define dbg(x) DebugLog((DebugLevel)(x),D_SDL) << __FILE__ << ":" << __LINE__ << ": "

//***********************************
//Globals                           *
//***********************************

#ifdef SDLTILES
cata_tiles *tilecontext;
static unsigned long lastupdate = 0;
static unsigned long interval = 25;
static bool needupdate = false;
#endif

#ifdef SDL_SOUND
/** The music we're currently playing. */
Mix_Music *current_music = NULL;
std::string current_playlist = "";
int current_playlist_at = 0;
#endif

/**
 * A class that draws a single character on screen.
 */
class Font {
public:
    Font(int w, int h) : fontwidth(w), fontheight(h) { }
    virtual ~Font() { }
    /**
     * Draw character t at (x,y) on the screen,
     * using (curses) color.
     */
    virtual void OutputChar(const std::string &ch, int x, int y, unsigned char color) = 0;
    virtual void draw_ascii_lines(unsigned char line_id, int drawx, int drawy, int FG) const;
    bool draw_window(WINDOW *win);
    bool draw_window(WINDOW *win, int offsetx, int offsety);

    static Font *load_font(const std::string &typeface, int fontsize, int fontwidth, int fontheight);
public:
    // the width of the font, background is always this size
    int fontwidth;
    // the height of the font, background is always this size
    int fontheight;
};

/**
 * Uses a ttf font. Its glyphs are cached.
 */
class CachedTTFFont : public Font {
public:
    CachedTTFFont(int w, int h);
    virtual ~CachedTTFFont();

    void clear();
    void load_font(std::string typeface, int fontsize);
    virtual void OutputChar(const std::string &ch, int x, int y, unsigned char color);
protected:
    SDL_Texture *create_glyph(const std::string &ch, int color);

    TTF_Font* font;
    // Maps (character code, color) to SDL_Texture*
    typedef std::map<std::pair<std::string, unsigned char>, SDL_Texture *> t_glyph_map;
    t_glyph_map glyph_cache_map;
};

/**
 * A font created from a bitmap. Each character is taken from a
 * specific area of the source bitmap.
 */
class BitmapFont : public Font {
public:
    BitmapFont(int w, int h);
    virtual ~BitmapFont();

    void clear();
    void load_font(const std::string &path);
    virtual void OutputChar(const std::string &ch, int x, int y, unsigned char color);
    void OutputChar(long t, int x, int y, unsigned char color);
    virtual void draw_ascii_lines(unsigned char line_id, int drawx, int drawy, int FG) const;
protected:
    SDL_Texture *ascii[16];
    int tilewidth;
};

static Font *font = NULL;
static Font *map_font = NULL;
static Font *overmap_font = NULL;

std::array<std::string, 16> main_color_names{ { "BLACK","RED","GREEN","BROWN","BLUE","MAGENTA",
"CYAN","GRAY","DGRAY","LRED","LGREEN","YELLOW","LBLUE","LMAGENTA","LCYAN","WHITE" } };
static SDL_Color windowsPalette[256];
static SDL_Window *window = NULL;
static SDL_Renderer* renderer = NULL;
static SDL_PixelFormat *format;
static SDL_Texture *display_buffer;
int WindowWidth;        //Width of the actual window, not the curses window
int WindowHeight;       //Height of the actual window, not the curses window
// input from various input sources. Each input source sets the type and
// the actual input value (key pressed, mouse button clicked, ...)
// This value is finally returned by input_manager::get_input_event.
input_event last_input;

int inputdelay;         //How long getch will wait for a character to be typed
Uint32 delaydpad = std::numeric_limits<Uint32>::max();     // Used for entering diagonal directions with d-pad.
Uint32 dpad_delay = 100;   // Delay in milli-seconds between registering a d-pad event and processing it.
bool dpad_continuous = false;  // Whether we're currently moving continously with the dpad.
int lastdpad = ERR;      // Keeps track of the last dpad press.
int queued_dpad = ERR;   // Queued dpad press, for individual button presses.
//WINDOW *_windows;  //Probably need to change this to dynamic at some point
//int WindowCount;        //The number of curses windows currently in use
int fontwidth;          //the width of the font, background is always this size
int fontheight;         //the height of the font, background is always this size
static int TERMINAL_WIDTH;
static int TERMINAL_HEIGHT;
std::map< std::string,std::vector<int> > consolecolors;

static SDL_Joystick *joystick; // Only one joystick for now.

static bool fontblending = false;

// Cache of bitmap fonts family.
// Used only while fontlist.txt is created.
static std::set<std::string> *bitmap_fonts;

static std::vector<curseline> framebuffer;
static WINDOW *winBuffer; //tracking last drawn window to fix the framebuffer
static int fontScaleBuffer; //tracking zoom levels to fix framebuffer w/tiles

#ifdef SDLTILES
//***********************************
//Tile-version specific functions   *
//***********************************

void init_interface()
{
    return; // dummy function, we have nothing to do here
}
#endif
//***********************************
//Non-curses, Window functions      *
//***********************************

void ClearScreen()
{
    SDL_RenderClear(renderer);
}


bool fexists(const char *filename)
{
  std::ifstream ifile(filename);
  return (bool)ifile;
}

bool InitSDL()
{
    int init_flags = SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER;
    int ret;

    ret = SDL_Init( init_flags );
    if( ret != 0 ) {
        dbg( D_ERROR ) << "SDL_Init failed with " << ret << ", error: " << SDL_GetError();
        return false;
    }
    ret = TTF_Init();
    if( ret != 0 ) {
        dbg( D_ERROR ) << "TTF_Init failed with " << ret << ", error: " << TTF_GetError();
        return false;
    }
    #ifdef SDLTILES
    ret = IMG_Init( IMG_INIT_PNG );
    if( (ret & IMG_INIT_PNG) != IMG_INIT_PNG ) {
        dbg( D_ERROR ) << "IMG_Init failed to initialize PNG support, tiles won't work, error: " << IMG_GetError();
        // cata_tiles won't be able to load the tiles, but the normal SDL
        // code will display fine.
    }
    #endif // SDLTILES

    ret = SDL_InitSubSystem( SDL_INIT_JOYSTICK );
    if( ret != 0 ) {
        dbg( D_WARNING ) << "Initializing joystick subsystem failed with " << ret << ", error: " <<
        SDL_GetError() << "\nIf you don't have a joystick plugged in, this is probably fine.";
    }

    //SDL2 has no functionality for INPUT_DELAY, we would have to query it manually, which is expensive
    //SDL2 instead uses the OS's Input Delay.

    atexit(SDL_Quit);

    return true;
}

//Registers, creates, and shows the Window!!
bool WinCreate()
{
    std::string version = string_format("Cataclysm: Dark Days Ahead - %s", getVersionString());

    // Common flags used for fulscreen and for windowed
    int window_flags = 0;
    WindowWidth = TERMINAL_WIDTH * fontwidth;
    WindowHeight = TERMINAL_HEIGHT * fontheight;

    if (OPTIONS["FULLSCREEN"]) {
        window_flags |= SDL_WINDOW_FULLSCREEN;
    }

    window = SDL_CreateWindow(version.c_str(),
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            WindowWidth,
            WindowHeight,
            window_flags
        );

    if (window == NULL) {
        dbg(D_ERROR) << "SDL_CreateWindow failed: " << SDL_GetError();
        return false;
    }
    if (window_flags & SDL_WINDOW_FULLSCREEN) {
        SDL_GetWindowSize(window, &WindowWidth, &WindowHeight);
        // Ignore previous values, use the whole window, but nothing more.
        TERMINAL_WIDTH = WindowWidth / fontwidth;
        TERMINAL_HEIGHT = WindowHeight / fontheight;
    }

    // Initialize framebuffer cache
    framebuffer.resize(TERMINAL_HEIGHT);
    for (int i = 0; i < TERMINAL_HEIGHT; i++) {
        framebuffer[i].chars.assign(TERMINAL_WIDTH, cursecell(""));
    }

    const Uint32 wformat = SDL_GetWindowPixelFormat(window);
    format = SDL_AllocFormat(wformat);
    if(format == 0) {
        dbg(D_ERROR) << "SDL_AllocFormat(" << wformat << ") failed: " << SDL_GetError();
        return false;
    }

    bool software_renderer = OPTIONS["SOFTWARE_RENDERING"];
    if( !software_renderer ) {
        dbg( D_INFO ) << "Attempting to initialize accelerated SDL renderer.";

        renderer = SDL_CreateRenderer( window, -1, SDL_RENDERER_ACCELERATED |
                                       SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE );
        if( renderer == NULL ) {
            dbg( D_ERROR ) << "Failed to initialize accelerated renderer, falling back to software rendering: " << SDL_GetError();
            software_renderer = true;
        }
    }
    if( software_renderer ) {
        renderer = SDL_CreateRenderer( window, -1, SDL_RENDERER_SOFTWARE | SDL_RENDERER_TARGETTEXTURE );
        if( renderer == NULL ) {
            dbg( D_ERROR ) << "Failed to initialize software renderer: " << SDL_GetError();
            return false;
        }
    }

    if( SDL_SetRenderDrawBlendMode( renderer, SDL_BLENDMODE_NONE ) != 0 ) {
        dbg( D_ERROR ) << "SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE) failed: " << SDL_GetError();
        // Ignored for now, rendering could still work
    }
    display_buffer = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, WindowWidth, WindowHeight);
    if( display_buffer == nullptr ) {
        dbg( D_ERROR ) << "Failed to create window buffer: " << SDL_GetError();
        return false;
    }
    if( SDL_SetRenderTarget( renderer, display_buffer ) != 0 ) {
        dbg( D_ERROR ) << "Failed to select render target: " << SDL_GetError();
        return false;
    }
    ClearScreen();

    // Errors here are ignored, worst case: the option does not work as expected,
    // but that won't crash
    if(OPTIONS["HIDE_CURSOR"] != "show" && SDL_ShowCursor(-1)) {
        SDL_ShowCursor(SDL_DISABLE);
    } else {
        SDL_ShowCursor(SDL_ENABLE);
    }

    // Initialize joysticks.
    int numjoy = SDL_NumJoysticks();

    if( OPTIONS["ENABLE_JOYSTICK"] && numjoy >= 1 ) {
        if( numjoy > 1 ) {
            dbg( D_WARNING ) << "You have more than one gamepads/joysticks plugged in, only the first will be used.";
        }
        joystick = SDL_JoystickOpen(0);
        if( joystick == nullptr ) {
            dbg( D_ERROR ) << "Opening the first joystick failed: " << SDL_GetError();
        } else {
            const int ret = SDL_JoystickEventState(SDL_ENABLE);
            if( ret < 0 ) {
                dbg( D_ERROR ) << "SDL_JoystickEventState(SDL_ENABLE) failed: " << SDL_GetError();
            }
        }
    } else {
        joystick = NULL;
    }

    // Set up audio mixer.
#ifdef SDL_SOUND
    int audio_rate = 22050;
    Uint16 audio_format = AUDIO_S16;
    int audio_channels = 2;
    int audio_buffers = 4096;

    if(Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers)) {
        dbg( D_ERROR ) << "Failed to open audio mixer, sound won't work: " << Mix_GetError();
    }
#endif

    return true;
}

void WinDestroy()
{
#ifdef SDL_SOUND
    Mix_CloseAudio();
#endif
    if(joystick) {
        SDL_JoystickClose(joystick);
        joystick = 0;
    }
    if(format)
        SDL_FreeFormat(format);
    format = NULL;
    if (display_buffer != NULL) {
        SDL_DestroyTexture(display_buffer);
        display_buffer = NULL;
    }
    if( renderer != NULL ) {
        SDL_DestroyRenderer( renderer );
        renderer = NULL;
    }
    if(window)
        SDL_DestroyWindow(window);
    window = NULL;
}

inline void FillRectDIB(SDL_Rect &rect, unsigned char color) {
    if( SDL_SetRenderDrawColor( renderer, windowsPalette[color].r, windowsPalette[color].g,
                                windowsPalette[color].b, 255 ) != 0 ) {
        dbg(D_ERROR) << "SDL_SetRenderDrawColor failed: " << SDL_GetError();
    }
    if( SDL_RenderFillRect( renderer, &rect ) != 0 ) {
        dbg(D_ERROR) << "SDL_RenderFillRect failed: " << SDL_GetError();
    }
}

//The following 3 methods use mem functions for fast drawing
inline void VertLineDIB(int x, int y, int y2, int thickness, unsigned char color)
{
    SDL_Rect rect;
    rect.x = x;
    rect.y = y;
    rect.w = thickness;
    rect.h = y2-y;
    FillRectDIB(rect, color);
}
inline void HorzLineDIB(int x, int y, int x2, int thickness, unsigned char color)
{
    SDL_Rect rect;
    rect.x = x;
    rect.y = y;
    rect.w = x2-x;
    rect.h = thickness;
    FillRectDIB(rect, color);
}
inline void FillRectDIB(int x, int y, int width, int height, unsigned char color)
{
    SDL_Rect rect;
    rect.x = x;
    rect.y = y;
    rect.w = width;
    rect.h = height;
    FillRectDIB(rect, color);
}


SDL_Texture *CachedTTFFont::create_glyph(const std::string &ch, int color)
{
    SDL_Surface * sglyph = (fontblending ? TTF_RenderUTF8_Blended : TTF_RenderUTF8_Solid)(font, ch.c_str(), windowsPalette[color]);
    if (sglyph == NULL) {
        dbg( D_ERROR ) << "Failed to create glyph for " << ch << ": " << TTF_GetError();
        return NULL;
    }
    /* SDL interprets each pixel as a 32-bit number, so our masks must depend
       on the endianness (byte order) of the machine */
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    static const Uint32 rmask = 0xff000000;
    static const Uint32 gmask = 0x00ff0000;
    static const Uint32 bmask = 0x0000ff00;
    static const Uint32 amask = 0x000000ff;
#else
    static const Uint32 rmask = 0x000000ff;
    static const Uint32 gmask = 0x0000ff00;
    static const Uint32 bmask = 0x00ff0000;
    static const Uint32 amask = 0xff000000;
#endif
    const int wf = utf8_wrapper( ch ).display_width();
    // Note: bits per pixel must be 8 to be synchron with the surface
    // that TTF_RenderGlyph above returns. This is important for SDL_BlitScaled
    SDL_Surface *surface = SDL_CreateRGBSurface(0, fontwidth * wf, fontheight, 32,
                                                rmask, gmask, bmask, amask);
    if (surface == NULL) {
        dbg( D_ERROR ) << "CreateRGBSurface failed: " << SDL_GetError();
        SDL_Texture *glyph = SDL_CreateTextureFromSurface(renderer, sglyph);
        SDL_FreeSurface(sglyph);
        return glyph;
    }
    SDL_Rect src_rect = { 0, 0, sglyph->w, sglyph->h };
    SDL_Rect dst_rect = { 0, 0, fontwidth * wf, fontheight };
    if (src_rect.w < dst_rect.w) {
        dst_rect.x = (dst_rect.w - src_rect.w) / 2;
        dst_rect.w = src_rect.w;
    } else if (src_rect.w > dst_rect.w) {
        src_rect.x = (src_rect.w - dst_rect.w) / 2;
        src_rect.w = dst_rect.w;
    }
    if (src_rect.h < dst_rect.h) {
        dst_rect.y = (dst_rect.h - src_rect.h) / 2;
        dst_rect.h = src_rect.h;
    } else if (src_rect.h > dst_rect.h) {
        src_rect.y = (src_rect.h - dst_rect.h) / 2;
        src_rect.h = dst_rect.h;
    }

    if (SDL_BlitSurface(sglyph, &src_rect, surface, &dst_rect) != 0) {
        dbg( D_ERROR ) << "SDL_BlitSurface failed: " << SDL_GetError();
        SDL_FreeSurface(surface);
    } else {
        SDL_FreeSurface(sglyph);
        sglyph = surface;
    }

    SDL_Texture *glyph = SDL_CreateTextureFromSurface(renderer, sglyph);
    SDL_FreeSurface(sglyph);
    return glyph;
}

void CachedTTFFont::OutputChar(const std::string &ch, int x, int y, unsigned char color)
{
    color &= 0xf;
    const t_glyph_map::key_type key( ch, color );
    auto it = glyph_cache_map.find( key );
    SDL_Texture *glyph;
    if( it == glyph_cache_map.end() ) {
        glyph = glyph_cache_map[key] = create_glyph( ch, color );
    } else {
        glyph = it->second;
    }
    if (glyph == NULL) {
        // Nothing we can do here )-:
        return;
    }
    SDL_Rect rect;
    rect.x = x;
    rect.y = y;
    rect.w = fontwidth * utf8_wrapper( ch ).display_width();
    rect.h = fontheight;
    if( SDL_RenderCopy( renderer, glyph, NULL, &rect ) != 0 ) {
        dbg(D_ERROR) << "SDL_RenderCopy failed: " << SDL_GetError();
    }
}

void BitmapFont::OutputChar(const std::string &ch, int x, int y, unsigned char color)
{
    int len = ch.length();
    const char *s = ch.c_str();
    const long t = UTF8_getch(&s, &len);
    BitmapFont::OutputChar(t, x, y, color);
}

void BitmapFont::OutputChar(long t, int x, int y, unsigned char color)
{
    if( t > 256 ) {
        return;
    }
    SDL_Rect src;
    src.x = (t % tilewidth) * fontwidth;
    src.y = (t / tilewidth) * fontheight;
    src.w = fontwidth;
    src.h = fontheight;
    SDL_Rect rect;
    rect.x = x; rect.y = y; rect.w = fontwidth; rect.h = fontheight;
    if( SDL_RenderCopy( renderer, ascii[color], &src, &rect ) != 0 ) {
        dbg(D_ERROR) << "SDL_RenderCopy failed: " << SDL_GetError();
    }
}

#ifdef SDLTILES
// only update if the set interval has elapsed
void try_update()
{
    unsigned long now = SDL_GetTicks();
    if (now - lastupdate >= interval) {
        // Select default target (the window), copy rendered buffer
        // there, present it, select the buffer as target again.
        if( SDL_SetRenderTarget( renderer, NULL ) != 0 ) {
            dbg(D_ERROR) << "SDL_SetRenderTarget failed: " << SDL_GetError();
        }
        if( SDL_RenderCopy( renderer, display_buffer, NULL, NULL ) != 0 ) {
            dbg(D_ERROR) << "SDL_RenderCopy failed: " << SDL_GetError();
        }
        SDL_RenderPresent(renderer);
        if( SDL_SetRenderTarget( renderer, display_buffer ) != 0 ) {
            dbg(D_ERROR) << "SDL_SetRenderTarget failed: " << SDL_GetError();
        }
        needupdate = false;
        lastupdate = now;
    } else {
        needupdate = true;
    }
}
#endif

// line_id is one of the LINE_*_C constants
// FG is a curses color
void Font::draw_ascii_lines(unsigned char line_id, int drawx, int drawy, int FG) const
{
    switch (line_id) {
        case LINE_OXOX_C://box bottom/top side (horizontal line)
            HorzLineDIB(drawx, drawy + (fontheight / 2), drawx + fontwidth, 1, FG);
            break;
        case LINE_XOXO_C://box left/right side (vertical line)
            VertLineDIB(drawx + (fontwidth / 2), drawy, drawy + fontheight, 2, FG);
            break;
        case LINE_OXXO_C://box top left
            HorzLineDIB(drawx + (fontwidth / 2), drawy + (fontheight / 2), drawx + fontwidth, 1, FG);
            VertLineDIB(drawx + (fontwidth / 2), drawy + (fontheight / 2), drawy + fontheight, 2, FG);
            break;
        case LINE_OOXX_C://box top right
            HorzLineDIB(drawx, drawy + (fontheight / 2), drawx + (fontwidth / 2), 1, FG);
            VertLineDIB(drawx + (fontwidth / 2), drawy + (fontheight / 2), drawy + fontheight, 2, FG);
            break;
        case LINE_XOOX_C://box bottom right
            HorzLineDIB(drawx, drawy + (fontheight / 2), drawx + (fontwidth / 2), 1, FG);
            VertLineDIB(drawx + (fontwidth / 2), drawy, drawy + (fontheight / 2) + 1, 2, FG);
            break;
        case LINE_XXOO_C://box bottom left
            HorzLineDIB(drawx + (fontwidth / 2), drawy + (fontheight / 2), drawx + fontwidth, 1, FG);
            VertLineDIB(drawx + (fontwidth / 2), drawy, drawy + (fontheight / 2) + 1, 2, FG);
            break;
        case LINE_XXOX_C://box bottom north T (left, right, up)
            HorzLineDIB(drawx, drawy + (fontheight / 2), drawx + fontwidth, 1, FG);
            VertLineDIB(drawx + (fontwidth / 2), drawy, drawy + (fontheight / 2), 2, FG);
            break;
        case LINE_XXXO_C://box bottom east T (up, right, down)
            VertLineDIB(drawx + (fontwidth / 2), drawy, drawy + fontheight, 2, FG);
            HorzLineDIB(drawx + (fontwidth / 2), drawy + (fontheight / 2), drawx + fontwidth, 1, FG);
            break;
        case LINE_OXXX_C://box bottom south T (left, right, down)
            HorzLineDIB(drawx, drawy + (fontheight / 2), drawx + fontwidth, 1, FG);
            VertLineDIB(drawx + (fontwidth / 2), drawy + (fontheight / 2), drawy + fontheight, 2, FG);
            break;
        case LINE_XXXX_C://box X (left down up right)
            HorzLineDIB(drawx, drawy + (fontheight / 2), drawx + fontwidth, 1, FG);
            VertLineDIB(drawx + (fontwidth / 2), drawy, drawy + fontheight, 2, FG);
            break;
        case LINE_XOXX_C://box bottom east T (left, down, up)
            VertLineDIB(drawx + (fontwidth / 2), drawy, drawy + fontheight, 2, FG);
            HorzLineDIB(drawx, drawy + (fontheight / 2), drawx + (fontwidth / 2), 1, FG);
            break;
        default:
            break;
    }
}

void invalidate_framebuffer(int x, int y, int width, int height)
{
    for (int j = 0, fby = y; j < height; j++, fby++) {
        std::fill_n(framebuffer[fby].chars.begin() + x, width, cursecell(""));
    }
}

void reinitialize_framebuffer()
{
    //Re-initialize the framebuffer with new values.
    const int new_height = std::max(TERMY, std::max(OVERMAP_WINDOW_HEIGHT, TERRAIN_WINDOW_HEIGHT));
    const int new_width = std::max(TERMX, std::max(OVERMAP_WINDOW_WIDTH, TERRAIN_WINDOW_WIDTH));
    framebuffer.resize( new_height );
    for( int i = 0; i < new_height; i++ ) {
        framebuffer[i].chars.assign( new_width, cursecell("") );
    }
}

extern WINDOW *w_hit_animation;
void curses_drawwindow(WINDOW *win)
{
    bool update = false;
#ifdef SDLTILES
    if (g && win == g->w_terrain && use_tiles) {
        // game::w_terrain can be drawn by the tilecontext.
        // skip the normal drawing code for it.
        tilecontext->draw(
            win->x * fontwidth,
            win->y * fontheight,
            g->ter_view_x,
            g->ter_view_y,
            TERRAIN_WINDOW_TERM_WIDTH * font->fontwidth,
            TERRAIN_WINDOW_TERM_HEIGHT * font->fontheight);

        invalidate_framebuffer(win->x, win->y, TERRAIN_WINDOW_TERM_WIDTH, TERRAIN_WINDOW_TERM_HEIGHT);

        update = true;
    } else if (g && win == g->w_terrain && map_font != NULL) {
        // When the terrain updates, predraw a black space around its edge
        // to keep various former interface elements from showing through the gaps
        // TODO: Maybe track down screen changes and use g->w_blackspace to draw this instead
        //Gap between terrain and lower window edge
        FillRectDIB(win->x * fontwidth, (win->y + TERRAIN_WINDOW_TERM_HEIGHT - 1) * fontheight,
                    TERRAIN_WINDOW_TERM_WIDTH * fontwidth, fontheight, COLOR_BLACK);
        //Gap between terrain and sidebar
        FillRectDIB((win->x + TERRAIN_WINDOW_TERM_WIDTH - 1) * fontwidth, win->y * fontheight,
                    fontwidth, TERRAIN_WINDOW_TERM_HEIGHT * fontheight, COLOR_BLACK);
        // Special font for the terrain window
        update = map_font->draw_window(win);
    } else if (g && win == g->w_overmap && overmap_font != NULL) {
        // Special font for the terrain window
        update = overmap_font->draw_window(win);
    } else if (win == w_hit_animation && map_font != NULL) {
        // The animation window overlays the terrain window,
        // it uses the same font, but it's only 1 square in size.
        // The offset must not use the global font, but the map font
        int offsetx = win->x * map_font->fontwidth;
        int offsety = win->y * map_font->fontheight;
        update = map_font->draw_window(win, offsetx, offsety);
    } else if (g && win == g->w_blackspace) {
        // fill-in black space window skips draw code
        // so as not to confuse framebuffer any more than necessary
        int offsetx = win->x * font->fontwidth;
        int offsety = win->y * font->fontheight;
        int wwidth = win->width * font->fontwidth;
        int wheight = win->height * font->fontheight;
        FillRectDIB(offsetx, offsety, wwidth, wheight, COLOR_BLACK);
        update = true;
    } else {
        // Either not using tiles (tilecontext) or not the w_terrain window.
        update = font->draw_window(win);
    }
#else
    // Not using sdl tiles anyway
    update = font->draw_window(win);
#endif
    if(update) {
        needupdate = true;
    }
}

bool Font::draw_window(WINDOW *win)
{
    // Use global font sizes here to make this independent of the
    // font used for this window.
    return draw_window(win, win->x * ::fontwidth, win->y * ::fontheight);
}

bool Font::draw_window( WINDOW *win, int offsetx, int offsety )
{
    //Keeping track of the last drawn window
    if( winBuffer == NULL ) {
            winBuffer = win;
    }
    if( !fontScaleBuffer ) {
            fontScaleBuffer = tilecontext->get_tile_width();
    }
    const int fontScale = tilecontext->get_tile_width();
    bool update = false;
    for( int j = 0; j < win->height; j++ ) {
        if( !win->line[j].touched ) {
            continue;
        }
        update = true;
        win->line[j].touched = false;
        for( int i = 0; i < win->width; i++ ) {
            const cursecell &cell = win->line[j].chars[i];

            const int drawx = offsetx + i * fontwidth;
            const int drawy = offsety + j * fontheight;
            if( drawx + fontwidth > WindowWidth || drawy + fontheight > WindowHeight ) {
                // Outside of the display area, would not render anyway
                continue;
            }

            // Avoid redrawing an unchanged tile by checking the framebuffer cache
            // TODO: handle caching when drawing normal windows over graphical tiles
            const int fbx = win->x + i;
            const int fby = win->y + j;
            cursecell &oldcell = framebuffer[fby].chars[fbx];
            //This creates a problem when map_font is different from the regular font
            //Specifically when showing the overmap
            //And in some instances of screen change, i.e. inventory.
            bool oldWinCompatible = false;
            /*
            Let's try to keep track of different windows.
            A number of windows are coexisting on the screen, so don't have to interfere.

            g->w_terrain, g->w_minimap, g->w_HP, g->w_status, g->w_status2, g->w_messages,
             g->w_location, and g->w_minimap, can be buffered if either of them was
             the previous window.

            g->w_overmap and g->w_omlegend are likewise.

            Everything else works on strict equality because there aren't yet IDs for some of them.
            */
            if ( win == g->w_terrain || win == g->w_minimap || win == g->w_HP || win == g->w_status ||
                 win == g->w_status2 || win == g->w_messages || win == g->w_location ) {
                if ( winBuffer == g->w_terrain || winBuffer == g->w_minimap ||
                     winBuffer == g->w_HP || winBuffer == g->w_status || winBuffer == g->w_status2 ||
                     winBuffer == g->w_messages || winBuffer == g->w_location ) {
                    oldWinCompatible = true;
                }
            }else if ( win == g->w_overmap || win == g->w_omlegend ){
                if ( winBuffer == g->w_overmap || winBuffer == g->w_omlegend ) {
                    oldWinCompatible = true;
                }
            }else {
                if( win == winBuffer ) {
                    oldWinCompatible = true;
                }
            }

            if (cell == oldcell && oldWinCompatible && fontScale == fontScaleBuffer) {
                continue;
            }
            oldcell = cell;

            if( cell.ch.empty() ) {
                continue; // second cell of a multi-cell character
            }
            const char *utf8str = cell.ch.c_str();
            int len = cell.ch.length();
            const int codepoint = UTF8_getch( &utf8str, &len );
            const int FG = cell.FG;
            const int BG = cell.BG;
            if( codepoint != UNKNOWN_UNICODE ) {
                const int cw = utf8_width( cell.ch.c_str() );
                if( cw < 1 ) {
                    // utf8_width() may return a negative width
                    continue;
                }
                FillRectDIB( drawx, drawy, fontwidth * cw, fontheight, BG );
                OutputChar( cell.ch, drawx, drawy, FG );
            } else {
                FillRectDIB( drawx, drawy, fontwidth, fontheight, BG );
                draw_ascii_lines( static_cast<unsigned char>( cell.ch[0] ), drawx, drawy, FG );
            }

        }
    }
    win->draw = false; //We drew the window, mark it as so
    //Keeping track of last drawn window and tilemode zoom level
    winBuffer = win;
    fontScaleBuffer = tilecontext->get_tile_width();

    return update;
}

static long alt_buffer = 0;
static bool alt_down = false;

static void begin_alt_code()
{
    alt_buffer = 0;
    alt_down = true;
}

static bool add_alt_code( char c )
{
    if( alt_down && c >= '0' && c <= '9' ) {
        alt_buffer = alt_buffer * 10 + ( c - '0' );
        return true;
    }
    return false;
}

static long end_alt_code()
{
    alt_down = false;
    return alt_buffer;
}

int HandleDPad()
{
    // Check if we have a gamepad d-pad event.
    if(SDL_JoystickGetHat(joystick, 0) != SDL_HAT_CENTERED) {
        // When someone tries to press a diagonal, they likely will
        // press a single direction first. Wait a few milliseconds to
        // give them time to press both of the buttons for the diagonal.
        int button = SDL_JoystickGetHat(joystick, 0);
        int lc = ERR;
        if(button == SDL_HAT_LEFT) {
            lc = JOY_LEFT;
        } else if(button == SDL_HAT_DOWN) {
            lc = JOY_DOWN;
        } else if(button == SDL_HAT_RIGHT) {
            lc = JOY_RIGHT;
        } else if(button == SDL_HAT_UP) {
            lc = JOY_UP;
        } else if(button == SDL_HAT_LEFTUP) {
            lc = JOY_LEFTUP;
        } else if(button == SDL_HAT_LEFTDOWN) {
            lc = JOY_LEFTDOWN;
        } else if(button == SDL_HAT_RIGHTUP) {
            lc = JOY_RIGHTUP;
        } else if(button == SDL_HAT_RIGHTDOWN) {
            lc = JOY_RIGHTDOWN;
        }

        if( delaydpad == std::numeric_limits<Uint32>::max() ) {
            delaydpad = SDL_GetTicks() + dpad_delay;
            queued_dpad = lc;
        }

        // Okay it seems we're ready to process.
        if( SDL_GetTicks() > delaydpad ) {

            if(lc != ERR) {
                if(dpad_continuous && (lc & lastdpad) == 0) {
                    // Continuous movement should only work in the same or similar directions.
                    dpad_continuous = false;
                    lastdpad = lc;
                    return 0;
                }

                last_input = input_event(lc, CATA_INPUT_GAMEPAD);
                lastdpad = lc;
                queued_dpad = ERR;

                if(dpad_continuous == false) {
                    delaydpad = SDL_GetTicks() + 200;
                    dpad_continuous = true;
                } else {
                    delaydpad = SDL_GetTicks() + 60;
                }
                return 1;
            }
        }
    } else {
        dpad_continuous = false;
        delaydpad = std::numeric_limits<Uint32>::max();

        // If we didn't hold it down for a while, just
        // fire the last registered press.
        if(queued_dpad != ERR) {
            last_input = input_event(queued_dpad, CATA_INPUT_GAMEPAD);
            queued_dpad = ERR;
            return 1;
        }
    }

    return 0;
}

/**
 * Translate SDL key codes to key identifiers used by ncurses, this
 * allows the input_manager to only consider those.
 * @return 0 if the input can not be translated (unknown key?),
 * -1 when a ALT+number sequence has been started,
 * or somthing that a call to ncurses getch would return.
 */
long sdl_keysym_to_curses(SDL_Keysym keysym)
{
    switch (keysym.sym) {
        // This is special: allow entering a unicode character with ALT+number
        case SDLK_RALT:
        case SDLK_LALT:
            begin_alt_code();
            return -1;
        // The following are simple translations:
        case SDLK_KP_ENTER:
        case SDLK_RETURN:
        case SDLK_RETURN2:
            return '\n';
        case SDLK_BACKSPACE:
        case SDLK_KP_BACKSPACE:
            return KEY_BACKSPACE;
        case SDLK_DELETE:
            return KEY_DC;
        case SDLK_ESCAPE:
            return KEY_ESCAPE;
        case SDLK_TAB:
            if (keysym.mod & KMOD_SHIFT) {
                return KEY_BTAB;
            }
            return '\t';
        case SDLK_LEFT:
            return KEY_LEFT;
        case SDLK_RIGHT:
            return KEY_RIGHT;
        case SDLK_UP:
            return KEY_UP;
        case SDLK_DOWN:
            return KEY_DOWN;
        case SDLK_PAGEUP:
            return KEY_PPAGE;
        case SDLK_PAGEDOWN:
            return KEY_NPAGE;
        case SDLK_HOME:
            return KEY_HOME;
        case SDLK_END:
            return KEY_END;
        case SDLK_F1: return KEY_F(1);
        case SDLK_F2: return KEY_F(2);
        case SDLK_F3: return KEY_F(3);
        case SDLK_F4: return KEY_F(4);
        case SDLK_F5: return KEY_F(5);
        case SDLK_F6: return KEY_F(6);
        case SDLK_F7: return KEY_F(7);
        case SDLK_F8: return KEY_F(8);
        case SDLK_F9: return KEY_F(9);
        case SDLK_F10: return KEY_F(10);
        case SDLK_F11: return KEY_F(11);
        case SDLK_F12: return KEY_F(12);
        case SDLK_F13: return KEY_F(13);
        case SDLK_F14: return KEY_F(14);
        case SDLK_F15: return KEY_F(15);
        // Every other key is ignored as there is no curses constant for it.
        // TODO: add more if you find more.
        default:
            return 0;
    }
}

//Check for any window messages (keypress, paint, mousemove, etc)
void CheckMessages()
{
    SDL_Event ev;
    bool quit = false;
    if(HandleDPad()) {
        return;
    }

    last_input = input_event();
    while(SDL_PollEvent(&ev)) {
        switch(ev.type) {
            case SDL_WINDOWEVENT:
                switch(ev.window.event) {
                case SDL_WINDOWEVENT_SHOWN:
                case SDL_WINDOWEVENT_EXPOSED:
                case SDL_WINDOWEVENT_RESTORED:
                    needupdate = true;
                    break;
                default:
                    break;
                }
            break;
            case SDL_KEYDOWN:
            {
                //hide mouse cursor on keyboard input
                if(OPTIONS["HIDE_CURSOR"] != "show" && SDL_ShowCursor(-1)) {
                    SDL_ShowCursor(SDL_DISABLE);
                }
                const Uint8 *keystate = SDL_GetKeyboardState(NULL);
                // manually handle Alt+F4 for older SDL lib, no big deal
                if( ev.key.keysym.sym == SDLK_F4
                && (keystate[SDL_SCANCODE_RALT] || keystate[SDL_SCANCODE_LALT]) ) {
                    quit = true;
                    break;
                }
                const long lc = sdl_keysym_to_curses(ev.key.keysym);
                if( lc <= 0 ) {
                    // a key we don't know in curses and won't handle.
                    break;
                } else if( add_alt_code( lc ) ) {
                    // key was handled
                } else {
                    last_input = input_event(lc, CATA_INPUT_KEYBOARD);
                }
            }
            break;
            case SDL_KEYUP:
            {
                if( ev.key.keysym.sym == SDLK_LALT || ev.key.keysym.sym == SDLK_RALT ) {
                    int code = end_alt_code();
                    if( code ) {
                        last_input = input_event(code, CATA_INPUT_KEYBOARD);
                        last_input.text = utf32_to_utf8(code);
                    }
                }
            }
            break;
            case SDL_TEXTINPUT:
                if( !add_alt_code( *ev.text.text ) ) {
                    const char *c = ev.text.text;
                    int len = strlen(ev.text.text);
                    const unsigned lc = UTF8_getch( &c, &len );
                    last_input = input_event( lc, CATA_INPUT_KEYBOARD );
                    last_input.text = ev.text.text;
                }
            break;
            case SDL_JOYBUTTONDOWN:
                last_input = input_event(ev.jbutton.button, CATA_INPUT_KEYBOARD);
            break;
            case SDL_JOYAXISMOTION: // on gamepads, the axes are the analog sticks
                // TODO: somehow get the "digipad" values from the axes
            break;
            case SDL_MOUSEMOTION:
                if (OPTIONS["HIDE_CURSOR"] == "show" || OPTIONS["HIDE_CURSOR"] == "hidekb") {
                    if (!SDL_ShowCursor(-1)) {
                        SDL_ShowCursor(SDL_ENABLE);
                    }

                    // Only monitor motion when cursor is visible
                    last_input = input_event(MOUSE_MOVE, CATA_INPUT_MOUSE);
                }
                break;

            case SDL_MOUSEBUTTONUP:
                switch (ev.button.button) {
                    case SDL_BUTTON_LEFT:
                        last_input = input_event(MOUSE_BUTTON_LEFT, CATA_INPUT_MOUSE);
                        break;
                    case SDL_BUTTON_RIGHT:
                        last_input = input_event(MOUSE_BUTTON_RIGHT, CATA_INPUT_MOUSE);
                        break;
                    }
                break;

            case SDL_MOUSEWHEEL:
                if(ev.wheel.y > 0) {
                    last_input = input_event(SCROLLWHEEL_UP, CATA_INPUT_MOUSE);
                } else if(ev.wheel.y < 0) {
                    last_input = input_event(SCROLLWHEEL_DOWN, CATA_INPUT_MOUSE);
                }
                break;

            case SDL_QUIT:
                quit = true;
                break;
        }
    }
#ifdef SDLTILES
    if (needupdate) {
        try_update();
    }
#endif
    if(quit) {
        endwin();
        exit(0);
    }
}

// Check if text ends with suffix
static bool ends_with(const std::string &text, const std::string &suffix) {
    return text.length() >= suffix.length() &&
        strcasecmp(text.c_str() + text.length() - suffix.length(), suffix.c_str()) == 0;
}

//***********************************
//Psuedo-Curses Functions           *
//***********************************

static void font_folder_list(std::ofstream& fout, std::string path)
{
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir (path.c_str())) != NULL) {
        bool found = false;
        while (!found && (ent = readdir (dir)) != NULL) {
            if( 0 == strcmp( ent->d_name, "." ) ||
                0 == strcmp( ent->d_name, ".." ) ) {
                continue;
            }
            char path_last = *path.rbegin();
            std::string f;
            if (is_filesep(path_last)) {
                f = path + ent->d_name;
            } else {
                f = path + FILE_SEP + ent->d_name;
            }

            struct stat stat_buffer;
            if( stat( f.c_str(), &stat_buffer ) == -1 ) {
                continue;
            }
            if( S_ISDIR(stat_buffer.st_mode) ) {
                font_folder_list( fout, f );
                continue;
            }

            TTF_Font* fnt = TTF_OpenFont(f.c_str(), 12);
            if (fnt == NULL) {
                continue;
            }
            long nfaces = 0;
            nfaces = TTF_FontFaces(fnt);
            TTF_CloseFont(fnt);
            fnt = NULL;

            for(long i = 0; i < nfaces; i++) {
                fnt = TTF_OpenFontIndex(f.c_str(), 12, i);
                if (fnt == NULL) {
                    continue;
                }

                // Add font family
                char *fami = TTF_FontFaceFamilyName(fnt);
                if (fami != NULL) {
                    fout << fami;
                } else {
                    TTF_CloseFont(fnt);
                    continue;
                }

                // Add font style
                char *style = TTF_FontFaceStyleName(fnt);
                bool isbitmap = ends_with(f, ".fon");
                if (style != NULL && !isbitmap && strcasecmp(style, "Regular") != 0) {
                    fout << " " << style;
                }
                if (isbitmap) {
                    std::set<std::string>::iterator it;
                    it = bitmap_fonts->find(std::string(fami));
                    if (it == bitmap_fonts->end()) {
                        // First appearance of this font family
                        bitmap_fonts->insert(fami);
                    } else { // Font in set. Add filename to family string
                        size_t start = f.find_last_of("/\\");
                        size_t end = f.find_last_of(".");
                        if (start != std::string::npos && end != std::string::npos) {
                            fout << " [" << f.substr(start + 1, end - start - 1) + "]";
                        } else {
                            dbg( D_INFO ) << "Skipping wrong font file: \"" << f << "\"";
                        }
                    }
                }
                fout << std::endl;

                // Add filename and font index
                fout << f << std::endl;
                fout << i << std::endl;

                TTF_CloseFont(fnt);
                fnt = NULL;

                // We use only 1 style in bitmap fonts.
                if (isbitmap) {
                    break;
                }
            }
        }
        closedir (dir);
    }
}

static void save_font_list()
{
    std::ofstream fout(FILENAMES["fontlist"].c_str(), std::ios_base::trunc);
    bitmap_fonts = new std::set<std::string>;

    font_folder_list(fout, FILENAMES["fontdir"]);

#if (defined _WIN32 || defined WINDOWS)
    char buf[256];
    GetSystemWindowsDirectory(buf, 256);
    strcat(buf, "\\fonts");
    font_folder_list(fout, buf);
#elif (defined _APPLE_ && defined _MACH_)

    /*
    // Well I don't know how osx actually works ....
    font_folder_list(fout, "/System/Library/Fonts");
    font_folder_list(fout, "/Library/Fonts");

    wordexp_t exp;
    wordexp("~/Library/Fonts", &exp, 0);
    font_folder_list(fout, exp.we_wordv[0]);
    wordfree(&exp);*/
#elif (defined linux || defined __linux)
    font_folder_list(fout, "/usr/share/fonts");
    font_folder_list(fout, "/usr/local/share/fonts");
    wordexp_t exp;
    wordexp("~/.fonts", &exp, 0);
    font_folder_list(fout, exp.we_wordv[0]);
    wordfree(&exp);
#endif
    //TODO: other systems

    bitmap_fonts->clear();
    delete bitmap_fonts;

    fout << "end of list" << std::endl;
}

static std::string find_system_font(std::string name, int& faceIndex)
{
    const std::string fontlist_path = FILENAMES["fontlist"];
    std::ifstream fin(fontlist_path.c_str());
    if ( !fin.is_open() ) {
        // Try opening the fontlist at the old location.
        fin.open(FILENAMES["legacy_fontlist"].c_str());
        if( !fin.is_open() ) {
            dbg( D_INFO ) << "Generating fontlist";
            assure_dir_exist(FILENAMES["config_dir"]);
            save_font_list();
            fin.open(fontlist_path.c_str());
            if( !fin ) {
                dbg( D_ERROR ) << "Can't open or create fontlist file " << fontlist_path;
                return "";
            }
        } else {
            // Write out fontlist to the new location.
            save_font_list();
        }
    }
    if ( fin.is_open() ) {
        std::string fname;
        std::string fpath;
        std::string iline;
        int index = 0;
        do {
            getline(fin, fname);
            if (fname == "end of list") break;
            getline(fin, fpath);
            getline(fin, iline);
            index = atoi(iline.c_str());
            if (0 == strcasecmp(fname.c_str(), name.c_str())) {
                faceIndex = index;
                return fpath;
            }
        } while (!fin.eof());
    }

    return "";
}

// bitmap font font size test
// return face index that has this size or below
static int test_face_size(std::string f, int size, int faceIndex)
{
    TTF_Font* fnt = TTF_OpenFontIndex(f.c_str(), size, faceIndex);
    if(fnt != NULL) {
        char* style = TTF_FontFaceStyleName(fnt);
        if(style != NULL) {
            int faces = TTF_FontFaces(fnt);
            bool found = false;
            for(int i = faces - 1; i >= 0 && !found; i--) {
                TTF_Font* tf = TTF_OpenFontIndex(f.c_str(), size, i);
                char* ts = NULL;
                if(NULL != tf) {
                   if( NULL != (ts = TTF_FontFaceStyleName(tf))) {
                       if(0 == strcasecmp(ts, style) && TTF_FontHeight(tf) <= size) {
                           faceIndex = i;
                           found = true;
                       }
                   }
                   TTF_CloseFont(tf);
                   tf = NULL;
                }
            }
        }
        TTF_CloseFont(fnt);
        fnt = NULL;
    }

    return faceIndex;
}

// Calculates the new width of the window, given the number of columns.
int projected_window_width(int)
{
    return OPTIONS["TERMINAL_X"] * fontwidth;
}

// Calculates the new height of the window, given the number of rows.
int projected_window_height(int)
{
    return OPTIONS["TERMINAL_Y"] * fontheight;
}

// forward declaration
void load_soundset();

//Basic Init, create the font, backbuffer, etc
WINDOW *curses_init(void)
{
    last_input = input_event();
    inputdelay = -1;

    std::string typeface, map_typeface, overmap_typeface;
    int fontsize = 8;
    int map_fontwidth = 8;
    int map_fontheight = 16;
    int map_fontsize = 8;
    int overmap_fontwidth = 8;
    int overmap_fontheight = 16;
    int overmap_fontsize = 8;

    std::ifstream jsonstream(FILENAMES["fontdata"].c_str(), std::ifstream::binary);
    if (jsonstream.good()) {
        JsonIn json(jsonstream);
        JsonObject config = json.get_object();
        fontblending = config.get_bool("fontblending", fontblending);
        fontwidth = config.get_int("fontwidth", fontwidth);
        fontheight = config.get_int("fontheight", fontheight);
        fontsize = config.get_int("fontsize", fontsize);
        typeface = config.get_string("typeface", typeface);
        map_fontwidth = config.get_int("map_fontwidth", fontwidth);
        map_fontheight = config.get_int("map_fontheight", fontheight);
        map_fontsize = config.get_int("map_fontsize", fontsize);
        map_typeface = config.get_string("map_typeface", typeface);
        overmap_fontwidth = config.get_int("overmap_fontwidth", fontwidth);
        overmap_fontheight = config.get_int("overmap_fontheight", fontheight);
        overmap_fontsize = config.get_int("overmap_fontsize", fontsize);
        overmap_typeface = config.get_string("overmap_typeface", typeface);
        jsonstream.close();
    } else { // User fontdata is missed. Try to load legacy fontdata.
        std::ifstream InStream(FILENAMES["legacy_fontdata"].c_str(), std::ifstream::binary);
        if(InStream.good()) {
            JsonIn jIn(InStream);
            JsonObject config = jIn.get_object();
            fontblending = config.get_bool("fontblending", fontblending);
            fontwidth = config.get_int("fontwidth", fontwidth);
            fontheight = config.get_int("fontheight", fontheight);
            fontsize = config.get_int("fontsize", fontsize);
            typeface = config.get_string("typeface", typeface);
            map_fontwidth = config.get_int("map_fontwidth", fontwidth);
            map_fontheight = config.get_int("map_fontheight", fontheight);
            map_fontsize = config.get_int("map_fontsize", fontsize);
            map_typeface = config.get_string("map_typeface", typeface);
            overmap_fontwidth = config.get_int("overmap_fontwidth", fontwidth);
            overmap_fontheight = config.get_int("overmap_fontheight", fontheight);
            overmap_fontsize = config.get_int("overmap_fontsize", fontsize);
            overmap_typeface = config.get_string("overmap_typeface", typeface);
            InStream.close();
            // Save legacy as user fontdata.
            assure_dir_exist(FILENAMES["config_dir"]);
            std::ofstream OutStream(FILENAMES["fontdata"].c_str(), std::ofstream::binary);
            if(!OutStream.good()) {
                dbg(D_ERROR) << "Can't save user fontdata file.\n" <<
                    "Check permissions for: " << FILENAMES["fontdata"];
                return NULL;
            }
            JsonOut jOut(OutStream, true); // pretty-print
            jOut.start_object();
            jOut.member("fontblending", fontblending);
            jOut.member("fontwidth", fontwidth);
            jOut.member("fontheight", fontheight);
            jOut.member("fontsize", fontsize);
            jOut.member("typeface", typeface);
            jOut.member("map_fontwidth", map_fontwidth);
            jOut.member("map_fontheight", map_fontheight);
            jOut.member("map_fontsize", map_fontsize);
            jOut.member("map_typeface", map_typeface);
            jOut.member("overmap_fontwidth", overmap_fontwidth);
            jOut.member("overmap_fontheight", overmap_fontheight);
            jOut.member("overmap_fontsize", overmap_fontsize);
            jOut.member("overmap_typeface", overmap_typeface);
            jOut.end_object();
            OutStream << "\n";
            OutStream.close();
        } else {
            dbg(D_ERROR) << "Can't load fontdata files.\n" << "Check permissions for:\n" <<
                FILENAMES["legacy_fontdata"] << "\n" << FILENAMES["fontdata"];
            return NULL;
        }
    }

    if(!InitSDL()) {
        return NULL;
    }

    TERMINAL_WIDTH = OPTIONS["TERMINAL_X"];
    TERMINAL_HEIGHT = OPTIONS["TERMINAL_Y"];

    if(!WinCreate()) {
        return NULL;
    }

    #ifdef SDLTILES
    dbg( D_INFO ) << "Initializing SDL Tiles context";
    tilecontext = new cata_tiles(renderer);
    try {
        tilecontext->init(FILENAMES["gfxdir"]);
        dbg( D_INFO ) << "Tiles initialized successfully.";
    } catch(std::string err) {
        dbg( D_ERROR ) << "failed to initialize tile: " << err;
        // use_tiles is the cached value of the USE_TILES option.
        // most (all?) code refers to this to see if cata_tiles should be used.
        // Setting it to false disables this from getting used.
        use_tiles = false;
    }
    #endif // SDLTILES

    init_colors();

    // initialize sound set
    load_soundset();

    // Reset the font pointer
    font = Font::load_font(typeface, fontsize, fontwidth, fontheight);
    if (font == NULL) {
        return NULL;
    }
    map_font = Font::load_font(map_typeface, map_fontsize, map_fontwidth, map_fontheight);
    overmap_font = Font::load_font( overmap_typeface, overmap_fontsize,
                                    overmap_fontwidth, overmap_fontheight );
    mainwin = newwin(get_terminal_height(), get_terminal_width(),0,0);
    return mainwin;   //create the 'stdscr' window and return its ref
}

Font *Font::load_font(const std::string &typeface, int fontsize, int fontwidth, int fontheight)
{
    #ifdef SDLTILES
    if (ends_with(typeface, ".bmp") || ends_with(typeface, ".png")) {
        // Seems to be an image file, not a font.
        // Try to load as bitmap font.
        BitmapFont *bm_font = new BitmapFont(fontwidth, fontheight);
        try {
            bm_font->load_font(FILENAMES["fontdir"] + typeface);
            // It worked, tell the world to use bitmap_font.
            return bm_font;
        } catch(std::exception &err) {
            delete bm_font;
            dbg( D_ERROR ) << "Failed to load " << typeface << ": " << err.what();
            // Continue to load as truetype font
        }
    }
    #endif // SDLTILES
    // Not loaded as bitmap font (or it failed), try to load as truetype
    CachedTTFFont *ttf_font = new CachedTTFFont(fontwidth, fontheight);
    try {
        ttf_font->load_font(typeface, fontsize);
        // It worked, tell the world to use cached_ttf_font
        return ttf_font;
    } catch(std::exception &err) {
        delete ttf_font;
        dbg( D_ERROR ) << "Failed to load " << typeface << ": " << err.what();
    }
    return NULL;
}

//Ported from windows and copied comments as well
//Not terribly sure how this function is suppose to work,
//but jday helped to figure most of it out
int curses_getch(WINDOW* win)
{
    input_event evt = inp_mngr.get_input_event(win);
    while(evt.type != CATA_INPUT_KEYBOARD) {
        evt = inp_mngr.get_input_event(win);
        if (evt.type == CATA_INPUT_TIMEOUT) {
            return ERR; // Calling functions expect an ERR on timeout
        }
    }
    return evt.sequence[0];
}

//Ends the terminal, destroy everything
int curses_destroy(void)
{
    delete font;
    font = NULL;
    delete map_font;
    map_font = NULL;
    WinDestroy();
    return 1;
}

//copied from gdi version and don't bother to rename it
inline SDL_Color BGR(int b, int g, int r)
{
    SDL_Color result;
    result.b=b;    //Blue
    result.g=g;    //Green
    result.r=r;    //Red
    //result.a=0;//The Alpha, isnt used, so just set it to 0
    return result;
}

void load_colors( JsonObject &jsobj )
{
    JsonArray jsarr;
    for( size_t c = 0; c < main_color_names.size(); c++ ) {
        const std::string &color = main_color_names[c];
        auto &bgr = consolecolors[color];
        jsarr = jsobj.get_array( color );
        bgr.resize( 3 );
        // Strange ordering, isn't it? Entries in consolecolors are BGR,
        // the json contains them as RGB.
        bgr[0] = jsarr.get_int( 2 );
        bgr[1] = jsarr.get_int( 1 );
        bgr[2] = jsarr.get_int( 0 );
    }
}

// translate color entry in consolecolors to SDL_Color
inline SDL_Color ccolor( const std::string &color )
{
    const auto it = consolecolors.find( color );
    if( it == consolecolors.end() ) {
        dbg( D_ERROR ) << "requested non-existing color " << color << "\n";
        return SDL_Color { 0, 0, 0, 0 };
    }
    return BGR( it->second[0], it->second[1], it->second[2] );
}

// This function mimics the ncurses interface. It must not throw.
// Instead it should return ERR or OK, see man curs_color
int curses_start_color( void )
{
    const std::string path = FILENAMES["colors"];
    colorpairs = new pairs[100];
    std::ifstream colorfile( path.c_str(), std::ifstream::in | std::ifstream::binary );
    try {
        JsonIn jsin( colorfile );
        // Manually load the colordef object because the json handler isn't loaded yet.
        jsin.start_array();
        while( !jsin.end_array() ) {
            JsonObject jo = jsin.get_object();
            load_colors( jo );
            jo.finish();
        }
    } catch( std::string e ) {
        dbg( D_ERROR ) << "Failed to load color definitions from " << path << ": " << e;
        return ERR;
    }
    for( size_t c = 0; c < main_color_names.size(); c++ ) {
        windowsPalette[c]  = ccolor( main_color_names[c] );
    }
    return OK;
}

void curses_timeout(int t)
{
    inputdelay = t;
}

extern WINDOW *mainwin;

// This is how we're actually going to handle input events, SDL getch
// is simply a wrapper around this.
input_event input_manager::get_input_event(WINDOW *win) {
    previously_pressed_key = 0;
    // standards note: getch is sometimes required to call refresh
    // see, e.g., http://linux.die.net/man/3/getch
    // so although it's non-obvious, that refresh() call (and maybe InvalidateRect?) IS supposed to be there

    if(win == NULL) win = mainwin;

    wrefresh(win);

    if (inputdelay < 0)
    {
        do
        {
            CheckMessages();
            if (last_input.type != CATA_INPUT_ERROR) break;
            SDL_Delay(1);
        }
        while (last_input.type == CATA_INPUT_ERROR);
    }
    else if (inputdelay > 0)
    {
        unsigned long starttime=SDL_GetTicks();
        unsigned long endtime;
        bool timedout = false;
        do
        {
            CheckMessages();
            endtime=SDL_GetTicks();
            if (last_input.type != CATA_INPUT_ERROR) break;
            SDL_Delay(1);
            timedout = endtime >= starttime + inputdelay;
            if (timedout) {
                last_input.type = CATA_INPUT_TIMEOUT;
            }
        }
        while (!timedout);
    }
    else
    {
        CheckMessages();
    }

    if (last_input.type == CATA_INPUT_MOUSE) {
        SDL_GetMouseState(&last_input.mouse_x, &last_input.mouse_y);
    } else if (last_input.type == CATA_INPUT_KEYBOARD) {
        previously_pressed_key = last_input.get_first_input();
    }

    return last_input;
}

bool gamepad_available() {
    return joystick != NULL;
}

void rescale_tileset(int size) {
    #ifdef SDLTILES
        tilecontext->set_draw_scale(size);
        g->init_ui();
        ClearScreen();
    #endif
}

bool input_context::get_coordinates(WINDOW* capture_win, int& x, int& y) {
    if(!coordinate_input_received) {
        return false;
    }

    if (!capture_win) {
        capture_win = g->w_terrain;
    }

    // this contains the font dimensions of the capture_win,
    // not necessarily the global standard font dimensions.
    int fw = fontwidth;
    int fh = fontheight;
#ifdef SDLTILES
    // tiles might have different dimensions than standard font
    if (use_tiles && capture_win == g->w_terrain) {
        fw = tilecontext->get_tile_width();
        fh = tilecontext->get_tile_height();
    } else
#endif
    if (map_font != NULL && capture_win == g->w_terrain) {
        // map font (if any) might differ from standard font
        fw = map_font->fontwidth;
        fh = map_font->fontheight;
    }

    // Translate mouse coords to map coords based on tile size,
    // the window position is *always* in standard font dimensions!
    const int win_left = capture_win->x * fontwidth;
    const int win_top = capture_win->y * fontheight;
    // But the size of the window is in the font dimensions of the window.
    const int win_right = win_left + (capture_win->width * fw);
    const int win_bottom = win_top + (capture_win->height * fh);
    // Check if click is within bounds of the window we care about
    if( coordinate_x < win_left || coordinate_x > win_right ||
        coordinate_y < win_top || coordinate_y > win_bottom ) {
        return false;
    }
    const int selected_column = (coordinate_x - win_left) / fw;
    const int selected_row = (coordinate_y - win_top) / fh;

    x = g->ter_view_x - ((capture_win->width / 2) - selected_column);
    y = g->ter_view_y - ((capture_win->height / 2) - selected_row);

    return true;
}

int get_terminal_width() {
    return TERMINAL_WIDTH;
}

int get_terminal_height() {
    return TERMINAL_HEIGHT;
}

BitmapFont::BitmapFont(int w, int h)
: Font(w, h)
{
    memset(ascii, 0x00, sizeof(ascii));
}

BitmapFont::~BitmapFont()
{
    clear();
}

void BitmapFont::clear()
{
    for (size_t a = 0; a < 16; a++) {
        if (ascii[a] != NULL) {
            SDL_DestroyTexture(ascii[a]);
            ascii[a] = NULL;
        }
    }
}

void BitmapFont::load_font(const std::string &typeface)
{
    clear();
    dbg( D_INFO ) << "Loading bitmap font [" + typeface + "]." ;
    SDL_Surface *asciiload = IMG_Load(typeface.c_str());
    if (asciiload == NULL) {
        throw std::runtime_error(IMG_GetError());
    }
    if (asciiload->w * asciiload->h < (fontwidth * fontheight * 256)) {
        SDL_FreeSurface(asciiload);
        throw std::runtime_error("bitmap for font is to small");
    }
    Uint32 key = SDL_MapRGB(asciiload->format, 0xFF, 0, 0xFF);
    SDL_SetColorKey(asciiload,SDL_TRUE,key);
    SDL_Surface *ascii_surf[16];
    ascii_surf[0] = SDL_ConvertSurface(asciiload,format,0);
    SDL_SetSurfaceRLE(ascii_surf[0], true);
    SDL_FreeSurface(asciiload);

    for (size_t a = 1; a < 16; ++a) {
        ascii_surf[a] = SDL_ConvertSurface(ascii_surf[0],format,0);
        SDL_SetSurfaceRLE(ascii_surf[a], true);
    }

    for (size_t a = 0; a < 16 - 1; ++a) {
        SDL_LockSurface(ascii_surf[a]);
        int size = ascii_surf[a]->h * ascii_surf[a]->w;
        Uint32 *pixels = (Uint32 *)ascii_surf[a]->pixels;
        Uint32 color = (windowsPalette[a].r << 16) | (windowsPalette[a].g << 8) | windowsPalette[a].b;
        for(int i = 0; i < size; i++) {
            if(pixels[i] == 0xFFFFFF) {
                pixels[i] = color;
            }
        }
        SDL_UnlockSurface(ascii_surf[a]);
    }
    tilewidth = ascii_surf[0]->w / fontwidth;

    //convert ascii_surf to SDL_Texture
    for(int a = 0; a < 16; ++a) {
        ascii[a] = SDL_CreateTextureFromSurface(renderer,ascii_surf[a]);
        SDL_FreeSurface(ascii_surf[a]);
    }
}

void BitmapFont::draw_ascii_lines(unsigned char line_id, int drawx, int drawy, int FG) const
{
    BitmapFont *t = const_cast<BitmapFont*>(this);
    switch (line_id) {
        case LINE_OXOX_C://box bottom/top side (horizontal line)
            t->OutputChar(0xcd, drawx, drawy, FG);
            break;
        case LINE_XOXO_C://box left/right side (vertical line)
            t->OutputChar(0xba, drawx, drawy, FG);
            break;
        case LINE_OXXO_C://box top left
            t->OutputChar(0xc9, drawx, drawy, FG);
            break;
        case LINE_OOXX_C://box top right
            t->OutputChar(0xbb, drawx, drawy, FG);
            break;
        case LINE_XOOX_C://box bottom right
            t->OutputChar(0xbc, drawx, drawy, FG);
            break;
        case LINE_XXOO_C://box bottom left
            t->OutputChar(0xc8, drawx, drawy, FG);
            break;
        case LINE_XXOX_C://box bottom north T (left, right, up)
            t->OutputChar(0xca, drawx, drawy, FG);
            break;
        case LINE_XXXO_C://box bottom east T (up, right, down)
            t->OutputChar(0xcc, drawx, drawy, FG);
            break;
        case LINE_OXXX_C://box bottom south T (left, right, down)
            t->OutputChar(0xcb, drawx, drawy, FG);
            break;
        case LINE_XXXX_C://box X (left down up right)
            t->OutputChar(0xce, drawx, drawy, FG);
            break;
        case LINE_XOXX_C://box bottom east T (left, down, up)
            t->OutputChar(0xb9, drawx, drawy, FG);
            break;
        default:
            break;
    }
}




CachedTTFFont::CachedTTFFont(int w, int h)
: Font(w, h)
, font(NULL)
{
}

CachedTTFFont::~CachedTTFFont()
{
    clear();
}

void CachedTTFFont::clear()
{
    if (font != NULL) {
        TTF_CloseFont(font);
        font = NULL;
    }
    for (t_glyph_map::iterator a = glyph_cache_map.begin(); a != glyph_cache_map.end(); ++a) {
        if (a->second != NULL) {
            SDL_DestroyTexture(a->second);
        }
    }
    glyph_cache_map.clear();
}

void CachedTTFFont::load_font(std::string typeface, int fontsize)
{
    clear();
    int faceIndex = 0;
    const std::string sysfnt = find_system_font(typeface, faceIndex);
    if (!sysfnt.empty()) {
        typeface = sysfnt;
        dbg( D_INFO ) << "Using font [" + typeface + "]." ;
    }
    //make fontdata compatible with wincurse
    if(!fexists(typeface.c_str())) {
        faceIndex = 0;
        typeface = FILENAMES["fontdir"] + typeface + ".ttf";
        dbg( D_INFO ) << "Using compatible font [" + typeface + "]." ;
    }
    //different default font with wincurse
    if(!fexists(typeface.c_str())) {
        faceIndex = 0;
        typeface = FILENAMES["fontdir"] + "fixedsys.ttf";
        dbg( D_INFO ) << "Using fallback font [" + typeface + "]." ;
    }
    dbg( D_INFO ) << "Loading truetype font [" + typeface + "]." ;
    if(fontsize <= 0) {
        fontsize = fontheight - 1;
    }
    // SDL_ttf handles bitmap fonts size incorrectly
    if( typeface.length() > 4 &&
        strcasecmp(typeface.substr(typeface.length() - 4).c_str(), ".fon") == 0 ) {
        faceIndex = test_face_size(typeface, fontsize, faceIndex);
    }
    font = TTF_OpenFontIndex(typeface.c_str(), fontsize, faceIndex);
    if (font == NULL) {
        throw std::runtime_error(TTF_GetError());
    }
    TTF_SetFontStyle(font, TTF_STYLE_NORMAL);
}

int map_font_width() {
#ifdef SDLTILES
    if (use_tiles && tilecontext != NULL) {
        return tilecontext->get_tile_width();
    }
#endif
    return (map_font != NULL ? map_font : font)->fontwidth;
}

int map_font_height() {
#ifdef SDLTILES
    if (use_tiles && tilecontext != NULL) {
        return tilecontext->get_tile_height();
    }
#endif
    return (map_font != NULL ? map_font : font)->fontheight;
}

int overmap_font_width() {
    return (overmap_font != NULL ? overmap_font : font)->fontwidth;
}

int overmap_font_height() {
    return (overmap_font != NULL ? overmap_font : font)->fontheight;
}

void to_map_font_dimension(int &w, int &h) {
    w = (w * fontwidth) / map_font_width();
    h = (h * fontheight) / map_font_height();
}

void from_map_font_dimension(int &w, int &h) {
    w = (w * map_font_width() + fontwidth - 1) / fontwidth;
    h = (h * map_font_height() + fontheight - 1) / fontheight;
}

void to_overmap_font_dimension(int &w, int &h) {
    w = (w * fontwidth) / overmap_font_width();
    h = (h * fontheight) / overmap_font_height();
}

bool is_draw_tiles_mode() {
    return use_tiles;
}

struct music_playlist {
    // list of filenames relative to the soundpack location
    std::vector<std::string> files;
    std::vector<int> volumes;
    bool shuffle;

    music_playlist() : shuffle(false) {
    }
};

std::map<std::string, music_playlist> playlists;

#ifdef SDL_SOUND

void musicFinished();

void play_music_file(std::string filename, int volume) {
    const std::string path = (FILENAMES["datadir"] + "/sound/" + filename);
    current_music = Mix_LoadMUS(path.c_str());
    if( current_music == nullptr ) {
        dbg( D_ERROR ) << "Failed to load audio file " << path << ": " << Mix_GetError();
        return;
    }
    Mix_VolumeMusic(volume * OPTIONS["MUSIC_VOLUME"] / 100);
    if( Mix_PlayMusic( current_music, 0 ) != 0 ) {
        dbg( D_ERROR ) << "Starting playlist " << path << " failed: " << Mix_GetError();
        return;
    }
    Mix_HookMusicFinished(musicFinished);
}

/** Callback called when we finish playing music. */
void musicFinished() {
    Mix_HaltMusic();
    Mix_FreeMusic(current_music);
    current_music = NULL;

    // Load the next file to play.
    current_playlist_at++;

    // Wrap around if we reached the end of the playlist.
    if(current_playlist_at >= (int)playlists[current_playlist].files.size()) {
        current_playlist_at = 0;
    }

    std::string filename = playlists[current_playlist].files[current_playlist_at];
    play_music_file(filename, playlists[current_playlist].volumes[current_playlist_at]);
}
#endif

void play_music(std::string playlist) {
#ifdef SDL_SOUND

    if(playlists[playlist].files.size() == 0) {
        return;
    }

    // Don't interrupt playlist that's already playing.
    if(playlist == current_playlist) {
        return;
    }

    std::string filename = playlists[playlist].files[0];
    int volume = playlists[playlist].volumes[0];

    current_playlist = playlist;
    current_playlist_at = 0;

    play_music_file(filename, volume);
#else
    (void)playlist;
#endif
}

void load_soundset() {
    std::string location = FILENAMES["datadir"] + "/sound/soundset.json";
    std::ifstream jsonstream(location.c_str(), std::ifstream::binary);
    if (jsonstream.good()) {
        JsonIn json(jsonstream);
        JsonObject config = json.get_object();
        JsonArray playlists_ = config.get_array("playlists");

        for(int i=0; i < (int)playlists_.size(); i++) {
            JsonObject playlist = playlists_.get_object(i);

            std::string playlist_id = playlist.get_string("id");
            music_playlist playlist_to_load;
            playlist_to_load.shuffle = playlist.get_bool("shuffle", false);

            JsonArray playlist_files = playlist.get_array("files");
            for(int j=0; j < (int)playlist_files.size(); j++) {
                JsonObject entry = playlist_files.get_object(j);
                playlist_to_load.files.push_back(entry.get_string("file"));
                playlist_to_load.volumes.push_back(entry.get_int("volume"));
            }

            playlists[playlist_id] = playlist_to_load;
        }
    }
}

#endif // TILES
