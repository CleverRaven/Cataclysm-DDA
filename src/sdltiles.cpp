#if (defined TILES)
#include "catacurse.h"
#include "options.h"
#include "output.h"
#include "input.h"
#include "color.h"
#include "catacharset.h"
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
#include "file_wrapper.h"

#ifdef _MSC_VER
#include "wdirent.h"
#include <direct.h>
#else
#include <dirent.h>
#endif

#if (defined _WIN32 || defined WINDOWS)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#ifndef strcasecmp
#define strcasecmp strcmpi
#endif
#else
#include <wordexp.h>
#endif

#include "SDL2/SDL.h"
#include "SDL2/SDL_ttf.h"
#ifdef SDLTILES
#include "SDL2/SDL_image.h"
#endif

#ifdef SDL_SOUND
#include "SDL_mixer.h"
#endif

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
    virtual void OutputChar(Uint16 t, int x, int y, unsigned char color) = 0;
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
    virtual void OutputChar(Uint16 t, int x, int y, unsigned char color);
protected:
    void cache_glyphs();
    SDL_Texture *create_glyph(int t, int color);

    TTF_Font* font;
    SDL_Texture *glyph_cache[128][16];
    // Maps (character code, color) to SDL_Texture*
    typedef std::map<std::pair<Uint16, unsigned char>, SDL_Texture *> t_glyph_map;
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
    virtual void OutputChar(Uint16 t, int x, int y, unsigned char color);
    virtual void draw_ascii_lines(unsigned char line_id, int drawx, int drawy, int FG) const;
protected:
    SDL_Texture *ascii[16];
    int tilewidth;
};

static Font *font = NULL;
static Font *map_font = NULL;

static SDL_Color windowsPalette[256];
static SDL_Window *window = NULL;
static SDL_Renderer* renderer = NULL;
static SDL_PixelFormat *format;
static SDL_Texture *display_buffer;
int WindowWidth;        //Width of the actual window, not the curses window
int WindowHeight;       //Height of the actual window, not the curses window
int lastchar;          //the last character that was pressed, resets in getch
bool lastchar_isbutton; // Whether lastchar was a gamepad button press rather than a keypress.
bool lastchar_is_mouse; // Mouse button pressed
int inputdelay;         //How long getch will wait for a character to be typed
int delaydpad = -1;     // Used for entering diagonal directions with d-pad.
int dpad_delay = 100;   // Delay in milli-seconds between registering a d-pad event and processing it.
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

    if (SDL_Init(init_flags) < 0) {
        return false;
    }
    if (TTF_Init() < 0) {
        return false;
    }
    #ifdef SDLTILES
    if (IMG_Init(IMG_INIT_PNG) < 0) {
        return false;
    }
    #endif // SDLTILES

    SDL_InitSubSystem(SDL_INIT_JOYSTICK);

    //SDL2 has no functionality for INPUT_DELAY, we would have to query it manually, which is expensive
    //SDL2 instead uses the OS's Input Delay.

    atexit(SDL_Quit);

    return true;
};

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
        return false;
    }
    if (window_flags & SDL_WINDOW_FULLSCREEN) {
        SDL_GetWindowSize(window, &WindowWidth, &WindowHeight);
        // Ignore previous values, use the whole window, but nothing more.
        TERMINAL_WIDTH = WindowWidth / fontwidth;
        TERMINAL_HEIGHT = WindowHeight / fontheight;
    }

    format = SDL_AllocFormat(SDL_GetWindowPixelFormat(window));

    bool software_renderer = OPTIONS["SOFTWARE_RENDERING"];
    if( !software_renderer ) {
        DebugLog() << "Attempting to initialize accelerated SDL renderer.\n";

        renderer = SDL_CreateRenderer( window, -1, SDL_RENDERER_ACCELERATED |
                                       SDL_RENDERER_PRESENTVSYNC );
        if( renderer == NULL ) {
            DebugLog() << "Failed to initialize accelerated renderer, falling back to software rendering: " << SDL_GetError() << "\n";
            software_renderer = true;
        }
    }
    if( software_renderer ) {
        renderer = SDL_CreateRenderer( window, -1, SDL_RENDERER_SOFTWARE );
        if( renderer == NULL ) {
            DebugLog() << "Failed to initialize software renderer: " << SDL_GetError() << "\n";
            return false;
        }
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    display_buffer = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_TARGET, WindowWidth, WindowHeight);
    SDL_SetRenderTarget(renderer, display_buffer);
    ClearScreen();

    if(OPTIONS["HIDE_CURSOR"] != "show" && SDL_ShowCursor(-1)) {
        SDL_ShowCursor(SDL_DISABLE);
    } else {
        SDL_ShowCursor(SDL_ENABLE);
    }

    // Initialize joysticks.
    int numjoy = SDL_NumJoysticks();

    if(numjoy > 1) {
        DebugLog() << "You have more than one gamepads/joysticks plugged in, only the first will be used.\n";
    }

    if(numjoy >= 1) {
        joystick = SDL_JoystickOpen(0);
    } else {
        joystick = NULL;
    }

    SDL_JoystickEventState(SDL_ENABLE);

    // Set up audio mixer.
#ifdef SDL_SOUND
    int audio_rate = 22050;
    Uint16 audio_format = AUDIO_S16;
    int audio_channels = 2;
    int audio_buffers = 4096;

    SDL_Init(SDL_INIT_AUDIO);

    if(Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers)) {
      DebugLog() << "Failed to open audio mixer.\n";
    }
#endif

    return true;
};

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
    if(renderer)
        SDL_DestroyRenderer(renderer);
    renderer = NULL;
    if (display_buffer != NULL) {
        SDL_DestroyTexture(display_buffer);
        display_buffer = NULL;
    }
    if(window)
        SDL_DestroyWindow(window);
    window = NULL;
};

inline void FillRectDIB(SDL_Rect &rect, unsigned char color) {
    SDL_SetRenderDrawColor(renderer, windowsPalette[color].r, windowsPalette[color].g, windowsPalette[color].b, 255);
    SDL_RenderFillRect(renderer, &rect);
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
};
inline void HorzLineDIB(int x, int y, int x2, int thickness, unsigned char color)
{
    SDL_Rect rect;
    rect.x = x;
    rect.y = y;
    rect.w = x2-x;
    rect.h = thickness;
    FillRectDIB(rect, color);
};
inline void FillRectDIB(int x, int y, int width, int height, unsigned char color)
{
    SDL_Rect rect;
    rect.x = x;
    rect.y = y;
    rect.w = width;
    rect.h = height;
    FillRectDIB(rect, color);
};


SDL_Texture *CachedTTFFont::create_glyph(int ch, int color)
{
    SDL_Surface * sglyph = (fontblending ? TTF_RenderGlyph_Blended : TTF_RenderGlyph_Solid)(font, ch, windowsPalette[color]);
    if (sglyph == NULL) {
        DebugLog() << "Failed to create glyph for " << std::string(1, ch) << ": " << TTF_GetError() << "\n";
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
    // Note: bits per pixel must be 8 to be synchron with the surface
    // that TTF_RenderGlyph above returns. This is important for SDL_BlitScaled
    SDL_Surface *surface = SDL_CreateRGBSurface(0, fontwidth, fontheight, 32, rmask, gmask, bmask, amask);
    if (surface == NULL) {
        DebugLog() << "CreateRGBSurface failed: " << SDL_GetError() << "\n";
        SDL_Texture *glyph = SDL_CreateTextureFromSurface(renderer, sglyph);
        SDL_FreeSurface(sglyph);
        return glyph;
    }
    SDL_Rect src_rect = { 0, 0, sglyph->w, sglyph->h };
    SDL_Rect dst_rect = { 0, 0, fontwidth, fontheight };
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
        DebugLog() << SDL_GetError() << "\n";
        SDL_FreeSurface(surface);
    } else {
        SDL_FreeSurface(sglyph);
        sglyph = surface;
    }

    SDL_Texture *glyph = SDL_CreateTextureFromSurface(renderer, sglyph);
    SDL_FreeSurface(sglyph);
    return glyph;
}

void CachedTTFFont::cache_glyphs()
{
    for(int ch = 0; ch < 128; ch++) {
        for(int color = 0; color < 16; color++) {
            glyph_cache[ch][color] = create_glyph(ch, color);
        }
    }
}

void CachedTTFFont::OutputChar(Uint16 t, int x, int y, unsigned char color)
{
    color &= 0xf;
    SDL_Texture * glyph;
    if (t >= 0x80) {
        SDL_Texture *&glyphr = glyph_cache_map[t_glyph_map::key_type(t, color)];
        if (glyphr == NULL) {
            glyphr = create_glyph(t, color);
        }
        glyph = glyphr;
    } else {
        glyph = glyph_cache[t][color];
    }
    if (glyph == NULL) {
        // Nothing we can do here )-:
        return;
    }
    SDL_Rect rect;
    rect.x = x;
    rect.y = y;
    rect.w = fontwidth;
    rect.h = fontheight;
    SDL_RenderCopy(renderer, glyph, NULL, &rect);
}

void BitmapFont::OutputChar(Uint16 t, int x, int y, unsigned char color)
{
    SDL_Rect src;
    src.x = (t % tilewidth) * fontwidth;
    src.y = (t / tilewidth) * fontheight;
    src.w = fontwidth;
    src.h = fontheight;
    SDL_Rect rect;
    rect.x = x; rect.y = y; rect.w = fontwidth; rect.h = fontheight;
    SDL_RenderCopy(renderer,ascii[color],&src,&rect);
}

#ifdef SDLTILES
// only update if the set interval has elapsed
void try_update()
{
    unsigned long now = SDL_GetTicks();
    if (now - lastupdate >= interval) {
        // Select default target (the window), copy rendered buffer
        // there, present it, select the buffer as target again.
        SDL_SetRenderTarget(renderer, NULL);
        SDL_RenderCopy(renderer, display_buffer, NULL, NULL);
        SDL_RenderPresent(renderer);
        SDL_SetRenderTarget(renderer, display_buffer);
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
        update = true;
    } else if (g && win == g->w_terrain && map_font != NULL) {
        // Special font for the terrain window
        update = map_font->draw_window(win);
    } else if (win == w_hit_animation && map_font != NULL) {
        // The animation window overlays the terrain window,
        // it uses the same font, but it's only 1 square in size.
        // The offset must not use the global font, but the map font
        int offsetx = win->x * map_font->fontwidth;
        int offsety = win->y * map_font->fontheight;
        update = map_font->draw_window(win, offsetx, offsety);
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

bool Font::draw_window(WINDOW *win, int offsetx, int offsety)
{
    int i,j,w,tmp;
    int drawx,drawy;
    bool update = false;
    for (j=0; j<win->height; j++){
        if (win->line[j].touched) {
            update = true;

            win->line[j].touched=false;

            for (i=0,w=0; w<win->width; i++,w++) {
                drawx = offsetx + w * fontwidth;
                drawy = offsety + j * fontheight;
                if (((drawx+fontwidth)<=WindowWidth) && ((drawy+fontheight)<=WindowHeight)){
                const char* utf8str = win->line[j].chars+i;
                int len = ANY_LENGTH;
                tmp = UTF8_getch(&utf8str, &len);
                int FG = win->line[j].FG[w];
                int BG = win->line[j].BG[w];
                FillRectDIB(drawx,drawy,fontwidth,fontheight,BG);

                if ( tmp != UNKNOWN_UNICODE){
                    int cw = mk_wcwidth((wchar_t)tmp);
                    len = ANY_LENGTH-len;
                    if(cw>1)
                    {
                        FillRectDIB(drawx+fontwidth*(cw-1),drawy,fontwidth,fontheight,BG);
                        w+=cw-1;
                    }
                    if(len>1)
                    {
                        i+=len-1;
                    }
                    if(tmp) OutputChar(tmp, drawx,drawy,FG);
                } else {
                    draw_ascii_lines((unsigned char)win->line[j].chars[i], drawx, drawy, FG);
                }//(tmp < 0)
                }
            };//for (i=0;i<_windows[w].width;i++)
        }
    };// for (j=0;j<_windows[w].height;j++)
    win->draw = false; //We drew the window, mark it as so
    return update;
}

#define ALT_BUFFER_SIZE 8
static char alt_buffer[ALT_BUFFER_SIZE];
static int alt_buffer_len = 0;
static bool alt_down = false;

static void begin_alt_code()
{
    alt_buffer[0] = '\0';
    alt_down = true;
    alt_buffer_len = 0;
}

static int add_alt_code(char c)
{
    // not exactly how it works, but acceptable
    if(c>='0' && c<='9')
    {
        if(alt_buffer_len<ALT_BUFFER_SIZE-1)
        {
            alt_buffer[alt_buffer_len] = c;
            alt_buffer[++alt_buffer_len] = '\0';
        }
    }
    return 0;
}

static int end_alt_code()
{
    alt_down = false;
    return atoi(alt_buffer);
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

        if(delaydpad == -1) {
            delaydpad = SDL_GetTicks() + dpad_delay;
            queued_dpad = lc;
        }

        // Okay it seems we're ready to process.
        if(SDL_GetTicks() > delaydpad) {

            if(lc != ERR) {
                if(dpad_continuous && (lc & lastdpad) == 0) {
                    // Continuous movement should only work in the same or similar directions.
                    dpad_continuous = false;
                    lastdpad = lc;
                    return 0;
                }

                lastchar_isbutton = true;
                lastchar = lc;
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
        delaydpad = -1;

        // If we didn't hold it down for a while, just
        // fire the last registered press.
        if(queued_dpad != ERR) {
            lastchar = queued_dpad;
            lastchar_isbutton = true;
            queued_dpad = ERR;
            return 1;
        }
    }

    return 0;
}

//Check for any window messages (keypress, paint, mousemove, etc)
void CheckMessages()
{
    SDL_Event ev;
    bool quit = false;
    if(HandleDPad()) {
        return;
    }

    lastchar_is_mouse = false;
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
                int lc = 0;
                //hide mouse cursor on keyboard input
                if(OPTIONS["HIDE_CURSOR"] != "show" && SDL_ShowCursor(-1)) { SDL_ShowCursor(SDL_DISABLE); }
                const Uint8 *keystate = SDL_GetKeyboardState(NULL);
                // manually handle Alt+F4 for older SDL lib, no big deal
                if( ev.key.keysym.sym == SDLK_F4
                && (keystate[SDL_SCANCODE_RALT] || keystate[SDL_SCANCODE_LALT]) ) {
                    quit = true;
                    break;
                }
                switch (ev.key.keysym.sym) {
                    case SDLK_KP_ENTER:
                    case SDLK_RETURN:
                    case SDLK_RETURN2:
                        lc = 10;
                        break;
                    case SDLK_BACKSPACE:
                    case SDLK_KP_BACKSPACE:
                        lc = 127;
                        break;
                    case SDLK_ESCAPE:
                        lc = 27;
                        break;
                    case SDLK_TAB:
                        lc = 9;
                        break;
                    case SDLK_RALT:
                    case SDLK_LALT:
                        begin_alt_code();
                        break;
                    case SDLK_LEFT:
                        lc = KEY_LEFT;
                        break;
                    case SDLK_RIGHT:
                        lc = KEY_RIGHT;
                        break;
                    case SDLK_UP:
                        lc = KEY_UP;
                        break;
                    case SDLK_DOWN:
                        lc = KEY_DOWN;
                        break;
                    case SDLK_PAGEUP:
                        lc = KEY_PPAGE;
                        break;
                    case SDLK_PAGEDOWN:
                        lc = KEY_NPAGE;
                        break;
                }
                if( !lc ) { break; }
                if( alt_down ) {
                    add_alt_code( lc );
                } else {
                    lastchar = lc;
                }
                lastchar_isbutton = false;
            }
            break;
            case SDL_KEYUP:
            {
                if( ev.key.keysym.sym == SDLK_LALT || ev.key.keysym.sym == SDLK_RALT ) {
                    int code = end_alt_code();
                    if( code ) { lastchar = code; }
                }
            }
            break;
            case SDL_TEXTINPUT:
                lastchar = *ev.text.text;
            break;
            case SDL_JOYBUTTONDOWN:
                lastchar = ev.jbutton.button;
                lastchar_isbutton = true;
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
                    lastchar_is_mouse = true;
                    lastchar = MOUSE_MOVE;
                }
                break;

            case SDL_MOUSEBUTTONUP:
                lastchar_is_mouse = true;
                switch (ev.button.button) {
                    case SDL_BUTTON_LEFT:
                        lastchar = MOUSE_BUTTON_LEFT;
                        break;
                    case SDL_BUTTON_RIGHT:
                        lastchar = MOUSE_BUTTON_RIGHT;
                        break;
                    }
                break;

            case SDL_MOUSEWHEEL:
                lastchar_is_mouse = true;
                if(ev.wheel.y > 0) {
                    lastchar = SCROLLWHEEL_UP;
                } else if(ev.wheel.y < 0) {
                    lastchar = SCROLLWHEEL_DOWN;
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
                            DebugLog() << "sdltiles.cpp::font_folder_list():" <<
                            "Skipping wrong font file: \"" << f.c_str() << "\"\n";
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
    std::ifstream fin(FILENAMES["fontlist"].c_str());
    if ( !fin.is_open() ) {
        // Try opening the fontlist at the old location.
        fin.open(FILENAMES["legacy_fontlist"].c_str());
        if( !fin.is_open() ) {
            DebugLog() << "Generating fontlist\n";
            assure_dir_exist(FILENAMES["config_dir"]);
            save_font_list();
            fin.open(FILENAMES["fontlist"].c_str());
            if( !fin ) {
                DebugLog() << "Can't open or create fontlist file.\n";
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
    lastchar = -1;
    inputdelay = -1;

    std::string typeface, map_typeface;
    int fontsize = 8;
    int map_fontwidth = 8;
    int map_fontheight = 16;
    int map_fontsize = 8;
 
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
            InStream.close();
            // Save legacy as user fontdata.
            assure_dir_exist(FILENAMES["config_dir"]);
            std::ofstream OutStream(FILENAMES["fontdata"].c_str(), std::ofstream::binary);
            if(!OutStream.good()) {
                DebugLog() << "Can't save user fontdata file.\n"
                << "Check permissions for: " << FILENAMES["fontdata"].c_str();
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
            jOut.end_object();
            OutStream << "\n";
            OutStream.close();
        } else {
            DebugLog() << "Can't load fontdata files.\n"
            << "Check permissions for:\n" << FILENAMES["legacy_fontdata"].c_str() << "\n"
            << FILENAMES["fontdata"].c_str() << "\n";
            return NULL;
        }
    }

    if(!InitSDL()) {
        DebugLog() << "Failed to initialize SDL: " << SDL_GetError() << "\n";
        return NULL;
    }

    TERMINAL_WIDTH = OPTIONS["TERMINAL_X"];
    TERMINAL_HEIGHT = OPTIONS["TERMINAL_Y"];

    if(!WinCreate()) {
        DebugLog() << "Failed to create game window: " << SDL_GetError() << "\n";
        return NULL;
    }

    #ifdef SDLTILES
    DebugLog() << "Initializing SDL Tiles context\n";
    tilecontext = new cata_tiles(renderer);
    try {
        tilecontext->init(FILENAMES["gfxdir"]);
        DebugLog() << "Tiles initialized successfully.\n";
    } catch(std::string err) {
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
            DebugLog() << "Failed to load " << typeface << ": " << err.what() << "\n";
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
        DebugLog() << "Failed to load " << typeface << ": " << err.what() << "\n";
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

void load_colors(JsonObject &jsobj)
{
    std::string colors[16]={"BLACK","RED","GREEN","BROWN","BLUE","MAGENTA","CYAN","GRAY",
    "DGRAY","LRED","LGREEN","YELLOW","LBLUE","LMAGENTA","LCYAN","WHITE"};
    JsonArray jsarr;
    for(int c=0;c<16;c++)
    {
        jsarr = jsobj.get_array(colors[c]);
        if(jsarr.size()<3)continue;
        consolecolors[colors[c]].clear();
        consolecolors[colors[c]].push_back(jsarr.get_int(2));
        consolecolors[colors[c]].push_back(jsarr.get_int(1));
        consolecolors[colors[c]].push_back(jsarr.get_int(0));
    }
}

#define ccolor(s) consolecolors[s][0],consolecolors[s][1],consolecolors[s][2]
int curses_start_color(void)
{
    colorpairs = new pairs[100];
    //Load the console colors from colors.json
    std::ifstream colorfile(FILENAMES["colors"].c_str(), std::ifstream::in | std::ifstream::binary);
    try{
        JsonIn jsin(colorfile);
        char ch;
        // Manually load the colordef object because the json handler isn't loaded yet.
        jsin.eat_whitespace();
        ch = jsin.peek();
        if( ch == '[' ) {
            jsin.start_array();
            // find type and dispatch each object until array close
            while (!jsin.end_array()) {
                jsin.eat_whitespace();
                char ch = jsin.peek();
                if (ch != '{') {
                    std::stringstream err;
                    err << jsin.line_number() << ": ";
                    err << "expected array of objects but found '";
                    err << ch << "', not '{'";
                    throw err.str();
                }
                JsonObject jo = jsin.get_object();
                load_colors(jo);
                jo.finish();
            }
        } else {
            // not an array?
            std::stringstream err;
            err << jsin.line_number() << ": ";
            err << "expected object or array, but found '" << ch << "'";
            throw err.str();
        }
    }
    catch(std::string e){
        throw FILENAMES["colors"] + ": " + e;
    }
    if(consolecolors.empty())return 0;
    windowsPalette[0]  = BGR(ccolor("BLACK"));
    windowsPalette[1]  = BGR(ccolor("RED"));
    windowsPalette[2]  = BGR(ccolor("GREEN"));
    windowsPalette[3]  = BGR(ccolor("BROWN"));
    windowsPalette[4]  = BGR(ccolor("BLUE"));
    windowsPalette[5]  = BGR(ccolor("MAGENTA"));
    windowsPalette[6]  = BGR(ccolor("CYAN"));
    windowsPalette[7]  = BGR(ccolor("GRAY"));
    windowsPalette[8]  = BGR(ccolor("DGRAY"));
    windowsPalette[9]  = BGR(ccolor("LRED"));
    windowsPalette[10] = BGR(ccolor("LGREEN"));
    windowsPalette[11] = BGR(ccolor("YELLOW"));
    windowsPalette[12] = BGR(ccolor("LBLUE"));
    windowsPalette[13] = BGR(ccolor("LMAGENTA"));
    windowsPalette[14] = BGR(ccolor("LCYAN"));
    windowsPalette[15] = BGR(ccolor("WHITE"));
    return 0;
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
    lastchar=ERR;//ERR=-1
    input_event rval;

    if (inputdelay < 0)
    {
        do
        {
            rval.type = CATA_INPUT_ERROR;
            CheckMessages();
            if (lastchar!=ERR) break;
            SDL_Delay(1);
        }
        while (lastchar==ERR);
    }
    else if (inputdelay > 0)
    {
        unsigned long starttime=SDL_GetTicks();
        unsigned long endtime;
        bool timedout = false;
        do
        {
            rval.type = CATA_INPUT_ERROR;
            CheckMessages();
            endtime=SDL_GetTicks();
            if (lastchar!=ERR) break;
            SDL_Delay(1);
            timedout = endtime >= starttime + inputdelay;
            if (timedout) {
                rval.type = CATA_INPUT_TIMEOUT;
            }
        }
        while (!timedout);
    }
    else
    {
        CheckMessages();
    }

    if (rval.type != CATA_INPUT_TIMEOUT) {
        if (lastchar == ERR) {
            rval.type = CATA_INPUT_ERROR;
        } else if (lastchar_isbutton) {
            rval.type = CATA_INPUT_GAMEPAD;
            rval.add_input(lastchar);
        } else if (lastchar_is_mouse) {
            rval.type = CATA_INPUT_MOUSE;
            rval.add_input(lastchar);
            SDL_GetMouseState(&rval.mouse_x, &rval.mouse_y);
        } else {
            rval.type = CATA_INPUT_KEYBOARD;
            rval.add_input(lastchar);
            previously_pressed_key = lastchar;
        }
    }

    return rval;
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
    if (coordinate_x < win_left || coordinate_x > win_right || coordinate_y < win_top || coordinate_y > win_bottom) {
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
    DebugLog() << "Loading bitmap font [" + typeface + "].\n" ;
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
    memset(glyph_cache, 0x00, sizeof(glyph_cache));
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
    for (size_t i = 0; i < 128; i++) {
        for (size_t j = 0; j < 16; j++) {
            if (glyph_cache[i][j] != NULL) {
                SDL_DestroyTexture(glyph_cache[i][j]);
                glyph_cache[i][j] = NULL;
            }
        }
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
        DebugLog() << "Using font [" + typeface + "].\n" ;
    }
    //make fontdata compatible with wincurse
    if(!fexists(typeface.c_str())) {
        faceIndex = 0;
        typeface = FILENAMES["fontdir"] + typeface + ".ttf";
        DebugLog() << "Using compatible font [" + typeface + "].\n" ;
    }
    //different default font with wincurse
    if(!fexists(typeface.c_str())) {
        faceIndex = 0;
        typeface = FILENAMES["fontdir"] + "fixedsys.ttf";
        DebugLog() << "Using fallback font [" + typeface + "].\n" ;
    }
    DebugLog() << "Loading truetype font [" + typeface + "].\n" ;
    if(fontsize <= 0) {
        fontsize = fontheight - 1;
    }
    // SDL_ttf handles bitmap fonts size incorrectly
    if (typeface.length() > 4 && strcasecmp(typeface.substr(typeface.length() - 4).c_str(), ".fon") == 0) {
        faceIndex = test_face_size(typeface, fontsize, faceIndex);
    }
    font = TTF_OpenFontIndex(typeface.c_str(), fontsize, faceIndex);
    if (font == NULL) {
        throw std::runtime_error(TTF_GetError());
    }
    TTF_SetFontStyle(font, TTF_STYLE_NORMAL);
    // glyph height hack by utunnels
    // SDL_ttf doesn't use FT_HAS_VERTICAL for function TTF_GlyphMetrics
    // this causes baseline problems for certain fonts
    // I can only guess by check a certain tall character...
    cache_glyphs();
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

void to_map_font_dimension(int &w, int &h) {
    w = (w * fontwidth) / map_font_width();
    h = (h * fontheight) / map_font_height();
}

void from_map_font_dimension(int &w, int &h) {
    w = (w * map_font_width() + fontwidth - 1) / fontwidth;
    h = (h * map_font_height() + fontheight - 1) / fontheight;
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
    current_music = Mix_LoadMUS((FILENAMES["datadir"] + "/sound/" + filename).c_str());
    Mix_VolumeMusic(volume * OPTIONS["MUSIC_VOLUME"] / 100);
    Mix_PlayMusic(current_music, 0);
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
    if(current_playlist_at >= playlists[current_playlist].files.size()) {
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

        for(int i=0; i < playlists_.size(); i++) {
            JsonObject playlist = playlists_.get_object(i);

            std::string playlist_id = playlist.get_string("id");
            music_playlist playlist_to_load;
            playlist_to_load.shuffle = playlist.get_bool("shuffle", false);

            JsonArray playlist_files = playlist.get_array("files");
            for(int j=0; j < playlist_files.size(); j++) {
                JsonObject entry = playlist_files.get_object(j);
                playlist_to_load.files.push_back(entry.get_string("file"));
                playlist_to_load.volumes.push_back(entry.get_int("volume"));
            }

            playlists[playlist_id] = playlist_to_load;
        }
    }
}

#endif // TILES
