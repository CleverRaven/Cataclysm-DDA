#if (defined TILES)
#include "game.h"
#include "cata_utility.h"
#include "color_loader.h"
#include "cursesport.h"
#include "font_loader.h"
#include "game_ui.h"
#include "loading_ui.h"
#include "options.h"
#include "output.h"
#include "input.h"
#include "color.h"
#include "catacharset.h"
#include "cursesdef.h"
#include "debug.h"
#include "player.h"
#include "translations.h"
#include "cata_tiles.h"
#include "get_version.h"
#include "init.h"
#include "path_info.h"
#include "string_formatter.h"
#include "filesystem.h"
#include "lightmap.h"
#include "rng.h"
#include <algorithm>
#include <cstring>
#include <vector>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <memory>
#include <stdexcept>
#include <limits>

#if (defined _WIN32 || defined WINDOWS)
#   include "platform_win.h"
#   include <shlwapi.h>
#   ifndef strcasecmp
#       define strcasecmp StrCmpI
#   endif
#endif

#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>

#ifdef SDL_SOUND
#   include <SDL_mixer.h>
#   include "sounds.h"
#endif

#define dbg(x) DebugLog((DebugLevel)(x),D_SDL) << __FILE__ << ":" << __LINE__ << ": "

//***********************************
//Globals                           *
//***********************************

std::unique_ptr<cata_tiles> tilecontext;
static unsigned long lastupdate = 0;
static unsigned long interval = 25;
static bool needupdate = false;
extern bool tile_iso;

#ifdef SDL_SOUND
/** The music we're currently playing. */
Mix_Music *current_music = NULL;
std::string current_playlist = "";
size_t current_playlist_at = 0;
size_t absolute_playlist_at = 0;
std::vector<std::size_t> playlist_indexes;

struct sound_effect {
    int volume;

    struct deleter {
        // Operator overloaded to leverage deletion API.
        void operator()( Mix_Chunk* const c ) const {
            Mix_FreeChunk( c );
        };
    };
    std::unique_ptr<Mix_Chunk, deleter> chunk;
};

using id_and_variant = std::pair<std::string, std::string>;
std::map<id_and_variant, std::vector<sound_effect>> sound_effects_p;

struct music_playlist {
    // list of filenames relative to the soundpack location
    struct entry {
        std::string file;
        int volume;
    };
    std::vector<entry> entries;
    bool shuffle;

    music_playlist() : shuffle(false) {
    }
};

std::map<std::string, music_playlist> playlists;

std::string current_soundpack_path = "";
#endif

struct SDL_Renderer_deleter {
    void operator()( SDL_Renderer * const renderer ) {
        SDL_DestroyRenderer( renderer );
    }
};
using SDL_Renderer_Ptr = std::unique_ptr<SDL_Renderer, SDL_Renderer_deleter>;
struct SDL_Window_deleter {
    void operator()( SDL_Window * const window ) {
        SDL_DestroyWindow( window );
    }
};
using SDL_Window_Ptr = std::unique_ptr<SDL_Window, SDL_Window_deleter>;
struct SDL_PixelFormat_deleter {
    void operator()( SDL_PixelFormat * const format ) {
        SDL_FreeFormat( format );
    }
};
using SDL_PixelFormat_Ptr = std::unique_ptr<SDL_PixelFormat, SDL_PixelFormat_deleter>;
struct TTF_Font_deleter {
    void operator()( TTF_Font * const font ) {
        TTF_CloseFont( font );
    }
};
using TTF_Font_Ptr = std::unique_ptr<TTF_Font, TTF_Font_deleter>;

/**
 * A class that draws a single character on screen.
 */
class Font {
public:
    Font(int w, int h) : fontwidth(w), fontheight(h) { }
    virtual ~Font() = default;
    /**
     * Draw character t at (x,y) on the screen,
     * using (curses) color.
     */
    virtual void OutputChar(std::string ch, int x, int y, unsigned char color, cata_cursesport::font_style FS) = 0;
    virtual void draw_ascii_lines(unsigned char line_id, int drawx, int drawy, int FG, cata_cursesport::font_style FS) const;
    bool draw_window( const catacurses::window &win );
    bool draw_window( const catacurses::window &win, int offsetx, int offsety );

    static std::unique_ptr<Font> load_font(const std::string &typeface, int fontsize, int fontwidth, int fontheight, bool fontblending);
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
    CachedTTFFont( int w, int h, std::string typeface, int fontsize, bool fontblending );
    ~CachedTTFFont() override = default;

    virtual void OutputChar(std::string ch, int x, int y, unsigned char color, cata_cursesport::font_style FS) override;
protected:
    TTF_Font *get_font(cata_cursesport::font_style FS);
    SDL_Texture_Ptr create_glyph( const std::string &ch, int color, cata_cursesport::font_style FS );

    std::map<cata_cursesport::font_style, TTF_Font_Ptr> font_map;
    // Maps (character code, color) to SDL_Texture*

    struct key_t {
        std::string   codepoints;
        unsigned char color;
        cata_cursesport::font_style FS;

        // Operator overload required to use in std::map.
        bool operator<(key_t const &rhs) const noexcept {
            return (FS.to_ulong() == rhs.FS.to_ulong()) ? ((color == rhs.color) ? codepoints < rhs.codepoints : color < rhs.color) : FS.to_ulong() < rhs.FS.to_ulong();
        }
    };

    struct cached_t {
        SDL_Texture_Ptr texture;
        int          width;
    };

    std::map<key_t, cached_t> glyph_cache_map;

    const bool fontblending;
    std::string typeface;
    int fontsize;
    int faceIndex;
};

/**
 * A font created from a bitmap. Each character is taken from a
 * specific area of the source bitmap.
 */
class BitmapFont : public Font {
public:
    BitmapFont( int w, int h, const std::string &path );
    ~BitmapFont() override = default;

    virtual void OutputChar(std::string ch, int x, int y, unsigned char color, cata_cursesport::font_style FS) override;
    void OutputChar(long t, int x, int y, unsigned char color);
  virtual void draw_ascii_lines(unsigned char line_id, int drawx, int drawy, int FG, cata_cursesport::font_style FS) const override;
protected:
    std::array<SDL_Texture_Ptr, color_loader<SDL_Color>::COLOR_NAMES_COUNT> ascii;
    int tilewidth;
};

static std::unique_ptr<Font> font;
static std::unique_ptr<Font> map_font;
static std::unique_ptr<Font> overmap_font;

static std::array<SDL_Color, color_loader<SDL_Color>::COLOR_NAMES_COUNT> windowsPalette;
static SDL_Window_Ptr window;
static SDL_Renderer_Ptr renderer;
static SDL_PixelFormat_Ptr format;
static SDL_Texture_Ptr display_buffer;
static int WindowWidth;        //Width of the actual window, not the curses window
static int WindowHeight;       //Height of the actual window, not the curses window
// input from various input sources. Each input source sets the type and
// the actual input value (key pressed, mouse button clicked, ...)
// This value is finally returned by input_manager::get_input_event.
static input_event last_input;

static constexpr int ERR = -1;
static int inputdelay;         //How long getch will wait for a character to be typed
static Uint32 delaydpad = std::numeric_limits<Uint32>::max();     // Used for entering diagonal directions with d-pad.
static Uint32 dpad_delay = 100;   // Delay in milliseconds between registering a d-pad event and processing it.
static bool dpad_continuous = false;  // Whether we're currently moving continuously with the dpad.
static int lastdpad = ERR;      // Keeps track of the last dpad press.
static int queued_dpad = ERR;   // Queued dpad press, for individual button presses.
int fontwidth;          //the width of the font, background is always this size
int fontheight;         //the height of the font, background is always this size
static int TERMINAL_WIDTH;
static int TERMINAL_HEIGHT;
bool fullscreen;

static SDL_Joystick *joystick; // Only one joystick for now.

using cata_cursesport::curseline;
using cata_cursesport::cursecell;
static std::vector<curseline> oversized_framebuffer;
static std::vector<curseline> terminal_framebuffer;
static std::weak_ptr<void> winBuffer; //tracking last drawn window to fix the framebuffer
static int fontScaleBuffer; //tracking zoom levels to fix framebuffer w/tiles
extern catacurses::window w_hit_animation; //this window overlays w_terrain which can be oversized

//***********************************
//Non-curses, Window functions      *
//***********************************

static bool operator==( const cata_cursesport::WINDOW *const lhs, const catacurses::window &rhs )
{
    return lhs == rhs.get();
}

void ClearScreen()
{
    SDL_RenderClear( renderer.get() );
}

bool InitSDL()
{
    int init_flags = SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER;
    int ret;

#ifdef SDL_HINT_WINDOWS_DISABLE_THREAD_NAMING
    SDL_SetHint(SDL_HINT_WINDOWS_DISABLE_THREAD_NAMING, "1");
#endif

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
    ret = IMG_Init( IMG_INIT_PNG );
    if( (ret & IMG_INIT_PNG) != IMG_INIT_PNG ) {
        dbg( D_ERROR ) << "IMG_Init failed to initialize PNG support, tiles won't work, error: " << IMG_GetError();
        // cata_tiles won't be able to load the tiles, but the normal SDL
        // code will display fine.
    }

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

bool SetupRenderTarget()
{
    if( SDL_SetRenderDrawBlendMode( renderer.get(), SDL_BLENDMODE_NONE ) != 0 ) {
        dbg( D_ERROR ) << "SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE) failed: " << SDL_GetError();
        // Ignored for now, rendering could still work
    }
    display_buffer.reset( SDL_CreateTexture( renderer.get(), SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, WindowWidth, WindowHeight ) );
    if( !display_buffer ) {
        dbg( D_ERROR ) << "Failed to create window buffer: " << SDL_GetError();
        return false;
    }
    if( SDL_SetRenderTarget( renderer.get(), display_buffer.get() ) != 0 ) {
        dbg( D_ERROR ) << "Failed to select render target: " << SDL_GetError();
        return false;
    }

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
    window_flags |= SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;

    if( get_option<std::string>( "SCALING_MODE" ) != "none" ) {
        SDL_SetHint( SDL_HINT_RENDER_SCALE_QUALITY, get_option<std::string>( "SCALING_MODE" ).c_str() );
    }

    if (get_option<std::string>( "FULLSCREEN" ) == "fullscreen") {
        window_flags |= SDL_WINDOW_FULLSCREEN;
    } else if (get_option<std::string>( "FULLSCREEN" ) == "windowedbl") {
        window_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
        fullscreen = true;
        SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");
    }

    int display = get_option<int>( "DISPLAY" );
    if ( display < 0 || display >= SDL_GetNumVideoDisplays() ) {
        display = 0;
    }

    ::window.reset( SDL_CreateWindow( version.c_str(),
            SDL_WINDOWPOS_CENTERED_DISPLAY( display ),
            SDL_WINDOWPOS_CENTERED_DISPLAY( display ),
            WindowWidth,
            WindowHeight,
            window_flags
        ) );

    if( !::window ) {
        dbg(D_ERROR) << "SDL_CreateWindow failed: " << SDL_GetError();
        return false;
    }
    if (window_flags & SDL_WINDOW_FULLSCREEN || window_flags & SDL_WINDOW_FULLSCREEN_DESKTOP) {
        SDL_GetWindowSize( ::window.get(), &WindowWidth, &WindowHeight );
        // Ignore previous values, use the whole window, but nothing more.
        TERMINAL_WIDTH = WindowWidth / fontwidth;
        TERMINAL_HEIGHT = WindowHeight / fontheight;
    }

    // Initialize framebuffer caches
    terminal_framebuffer.resize(TERMINAL_HEIGHT);
    for (int i = 0; i < TERMINAL_HEIGHT; i++) {
        terminal_framebuffer[i].chars.assign(TERMINAL_WIDTH, cursecell(""));
    }

    oversized_framebuffer.resize(TERMINAL_HEIGHT);
    for (int i = 0; i < TERMINAL_HEIGHT; i++) {
        oversized_framebuffer[i].chars.assign(TERMINAL_WIDTH, cursecell(""));
    }

    const Uint32 wformat = SDL_GetWindowPixelFormat( ::window.get() );
    format.reset( SDL_AllocFormat( wformat ) );
    if( !format ) {
        dbg(D_ERROR) << "SDL_AllocFormat(" << wformat << ") failed: " << SDL_GetError();
        return false;
    }

    bool software_renderer = get_option<bool>( "SOFTWARE_RENDERING" );
    if( !software_renderer ) {
        dbg( D_INFO ) << "Attempting to initialize accelerated SDL renderer.";

        renderer.reset( SDL_CreateRenderer( ::window.get(), -1, SDL_RENDERER_ACCELERATED |
                                            SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE ) );
        if( !renderer ) {
            dbg( D_ERROR ) << "Failed to initialize accelerated renderer, falling back to software rendering: " << SDL_GetError();
            software_renderer = true;
        } else if( !SetupRenderTarget() ) {
            dbg( D_ERROR ) << "Failed to initialize display buffer under accelerated rendering, falling back to software rendering.";
            software_renderer = true;
            display_buffer.reset();
            renderer.reset();
        }
    }
    if( software_renderer ) {
        if( get_option<bool>( "FRAMEBUFFER_ACCEL" ) ) {
            SDL_SetHint( SDL_HINT_FRAMEBUFFER_ACCELERATION, "1" );
        }
        renderer.reset( SDL_CreateRenderer( ::window.get(), -1, SDL_RENDERER_SOFTWARE | SDL_RENDERER_TARGETTEXTURE ) );
        if( !renderer ) {
            dbg( D_ERROR ) << "Failed to initialize software renderer: " << SDL_GetError();
            return false;
        } else if( !SetupRenderTarget() ) {
            dbg( D_ERROR ) << "Failed to initialize display buffer under software rendering, unable to continue.";
            return false;
        }
    }

    SDL_SetWindowMinimumSize( ::window.get(), fontwidth * 80, fontheight * 24 );

    ClearScreen();

    // Errors here are ignored, worst case: the option does not work as expected,
    // but that won't crash
    if(get_option<std::string>( "HIDE_CURSOR" ) != "show" && SDL_ShowCursor(-1)) {
        SDL_ShowCursor(SDL_DISABLE);
    } else {
        SDL_ShowCursor(SDL_ENABLE);
    }

    // Initialize joysticks.
    int numjoy = SDL_NumJoysticks();

    if( get_option<bool>( "ENABLE_JOYSTICK" ) && numjoy >= 1 ) {
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
    int audio_rate = 44100;
    Uint16 audio_format = AUDIO_S16;
    int audio_channels = 2;
    int audio_buffers = 2048;

    if(Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers)) {
        dbg( D_ERROR ) << "Failed to open audio mixer, sound won't work: " << Mix_GetError();
    }
    Mix_AllocateChannels(128);
    Mix_ReserveChannels(20);

    // For the sound effects system.
    Mix_GroupChannels( 2, 9, 1 );
    Mix_GroupChannels( 0, 1, 2 );
    Mix_GroupChannels( 11, 14, 3 );
    Mix_GroupChannels( 15, 17, 4 );
#endif

    return true;
}

// forward declaration
void load_soundset();
void cleanup_sound();

void WinDestroy()
{
#ifdef SDL_SOUND
    // De-allocate all loaded sound.
    cleanup_sound();
    Mix_CloseAudio();
#endif
    tilecontext.reset();

    if(joystick) {
        SDL_JoystickClose(joystick);
        joystick = 0;
    }
    format.reset();
    display_buffer.reset();
    renderer.reset();
    ::window.reset();
}

inline void FillRectDIB(SDL_Rect &rect, unsigned char color) {
    if( SDL_SetRenderDrawColor( renderer.get(), windowsPalette[color].r, windowsPalette[color].g,
                                windowsPalette[color].b, 255 ) != 0 ) {
        dbg(D_ERROR) << "SDL_SetRenderDrawColor failed: " << SDL_GetError();
    }
    if( SDL_RenderFillRect( renderer.get(), &rect ) != 0 ) {
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


SDL_Texture_Ptr CachedTTFFont::create_glyph( const std::string &ch, const int color, cata_cursesport::font_style FS )
{
    const auto function = fontblending ? TTF_RenderUTF8_Blended : TTF_RenderUTF8_Solid;
    TTF_Font *_font = get_font(FS);
    SDL_Surface_Ptr sglyph( function( _font, ch.c_str(), windowsPalette[color] ) );
    if( !sglyph ) {
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
    // Note: bits per pixel must be 8 to be synchronized with the surface
    // that TTF_RenderGlyph above returns. This is important for SDL_BlitScaled
    SDL_Surface_Ptr surface( SDL_CreateRGBSurface( 0, fontwidth * wf, fontheight, 32,
                                                   rmask, gmask, bmask, amask ) );
    if( !surface ) {
        dbg( D_ERROR ) << "CreateRGBSurface failed: " << SDL_GetError();
        return SDL_Texture_Ptr( SDL_CreateTextureFromSurface( renderer.get(), sglyph.get() ) );
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

    if ( SDL_BlitSurface( sglyph.get(), &src_rect, surface.get(), &dst_rect ) != 0 ) {
        dbg( D_ERROR ) << "SDL_BlitSurface failed: " << SDL_GetError();
    } else {
        sglyph = std::move( surface );
    }

    return SDL_Texture_Ptr( SDL_CreateTextureFromSurface( renderer.get(), sglyph.get() ) );
}

void CachedTTFFont::OutputChar(std::string ch, int const x, int const y, unsigned char const color, cata_cursesport::font_style FS)
{
    key_t key {std::move(ch), static_cast<unsigned char>(color & 0xf), FS};

    auto it = glyph_cache_map.find( key );
    if( it == std::end( glyph_cache_map ) ) {
        cached_t new_entry {
            create_glyph( key.codepoints, key.color, FS ),
            static_cast<int>( fontwidth * utf8_wrapper( key.codepoints ).display_width() )
        };
        it = glyph_cache_map.insert( std::make_pair( std::move( key ), std::move( new_entry ) ) ).first;
    }
    const cached_t &value = it->second;

    if (!value.texture) {
        // Nothing we can do here )-:
        return;
    }
    SDL_Rect rect {x, y, value.width, fontheight};
    if( SDL_RenderCopy( renderer.get(), value.texture.get(), nullptr, &rect ) ) {
        dbg(D_ERROR) << "SDL_RenderCopy failed: " << SDL_GetError();
    }
}

void BitmapFont::OutputChar(std::string ch, int x, int y, unsigned char color, cata_cursesport::font_style FS)
{
    (void) FS; // unused
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
    if( SDL_RenderCopy( renderer.get(), ascii[color].get(), &src, &rect ) != 0 ) {
        dbg(D_ERROR) << "SDL_RenderCopy failed: " << SDL_GetError();
    }
}

void refresh_display()
{
    needupdate = false;
    lastupdate = SDL_GetTicks();

    // Select default target (the window), copy rendered buffer
    // there, present it, select the buffer as target again.
    if( SDL_SetRenderTarget( renderer.get(), NULL ) != 0 ) {
        dbg(D_ERROR) << "SDL_SetRenderTarget failed: " << SDL_GetError();
    }
    if( SDL_RenderCopy( renderer.get(), display_buffer.get(), NULL, NULL ) != 0 ) {
        dbg(D_ERROR) << "SDL_RenderCopy failed: " << SDL_GetError();
    }
    SDL_RenderPresent( renderer.get() );
    if( SDL_SetRenderTarget( renderer.get(), display_buffer.get() ) != 0 ) {
        dbg(D_ERROR) << "SDL_SetRenderTarget failed: " << SDL_GetError();
    }
}

// only update if the set interval has elapsed
static void try_sdl_update()
{
    unsigned long now = SDL_GetTicks();
    if (now - lastupdate >= interval) {
        refresh_display();
    } else {
        needupdate = true;
    }
}



//for resetting the render target after updating texture caches in cata_tiles.cpp
void set_displaybuffer_rendertarget()
{
    if( SDL_SetRenderTarget( renderer.get(), display_buffer.get() ) != 0 ) {
        dbg(D_ERROR) << "SDL_SetRenderTarget failed: " << SDL_GetError();
    }
}

// Populate a map with the available video displays and their name
void find_videodisplays() {
    std::map<int, std::string> displays;

    int numdisplays = SDL_GetNumVideoDisplays();
    for( int i = 0 ; i < numdisplays ; i++ ) {
        displays.insert( { i, SDL_GetDisplayName( i ) } );
    }

    int current_display = get_option<int>( "DISPLAY" );
    get_options().add("DISPLAY", "graphics", _("Display"),
                      _("Sets which video display will be used to show the game. Requires restart."),
                      displays, current_display, 0, options_manager::COPT_CURSES_HIDE
                      );
}

// line_id is one of the LINE_*_C constants
// FG is a curses color
void Font::draw_ascii_lines(unsigned char line_id, int drawx, int drawy, int FG, cata_cursesport::font_style FS) const
{
    (void) FS; // unused
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

void invalidate_framebuffer( std::vector<curseline> &framebuffer, int x, int y, int width, int height )
{
    for( int j = 0, fby = y; j < height; j++, fby++ ) {
        std::fill_n( framebuffer[fby].chars.begin() + x, width, cursecell( "" ) );
    }
}

void invalidate_framebuffer( std::vector<curseline> &framebuffer )
{
    for( unsigned int i = 0; i < framebuffer.size(); i++ ) {
        std::fill_n( framebuffer[i].chars.begin(), framebuffer[i].chars.size(), cursecell( "" ) );
    }
}

void invalidate_all_framebuffers()
{
    invalidate_framebuffer( terminal_framebuffer );
    invalidate_framebuffer( oversized_framebuffer );
}

void reinitialize_framebuffer()
{
    //Re-initialize the framebuffer with new values.
    const int new_height = std::max( TERMY, std::max( OVERMAP_WINDOW_HEIGHT, TERRAIN_WINDOW_HEIGHT ) );
    const int new_width = std::max( TERMX, std::max( OVERMAP_WINDOW_WIDTH, TERRAIN_WINDOW_WIDTH ) );
    oversized_framebuffer.resize( new_height );
    for( int i = 0; i < new_height; i++ ) {
        oversized_framebuffer[i].chars.assign( new_width, cursecell( "" ) );
    }
    terminal_framebuffer.resize( new_height );
    for( int i = 0; i < new_height; i++ ) {
        terminal_framebuffer[i].chars.assign( new_width, cursecell( "" ) );
    }
}

void invalidate_framebuffer_proportion( cata_cursesport::WINDOW* win )
{
    const int oversized_width = std::max( TERMX, std::max( OVERMAP_WINDOW_WIDTH, TERRAIN_WINDOW_WIDTH ) );
    const int oversized_height = std::max( TERMY, std::max( OVERMAP_WINDOW_HEIGHT, TERRAIN_WINDOW_HEIGHT ) );

    // check if the framebuffers/windows have been prepared yet
    if ( oversized_height == 0 || oversized_width == 0 ) {
        return;
    }
    if ( !g || win == nullptr ) {
        return;
    }
    if ( win == g->w_overmap || win == g->w_terrain || win == w_hit_animation ) {
        return;
    }

    // track the dimensions for conversion
    const int termpixel_x = win->x * font->fontwidth;
    const int termpixel_y = win->y * font->fontheight;
    const int termpixel_x2 = termpixel_x + win->width * font->fontwidth - 1;
    const int termpixel_y2 = termpixel_y + win->height * font->fontheight - 1;

    if ( map_font != nullptr && map_font->fontwidth != 0 && map_font->fontheight != 0 ) {
        const int mapfont_x = termpixel_x / map_font->fontwidth;
        const int mapfont_y = termpixel_y / map_font->fontheight;
        const int mapfont_x2 = std::min( termpixel_x2 / map_font->fontwidth, oversized_width - 1 );
        const int mapfont_y2 = std::min( termpixel_y2 / map_font->fontheight, oversized_height - 1 );
        const int mapfont_width = mapfont_x2 - mapfont_x + 1;
        const int mapfont_height = mapfont_y2 - mapfont_y + 1;
        invalidate_framebuffer( oversized_framebuffer, mapfont_x, mapfont_y, mapfont_width, mapfont_height );
    }

    if ( overmap_font != nullptr && overmap_font->fontwidth != 0 && overmap_font->fontheight != 0 ) {
        const int overmapfont_x = termpixel_x / overmap_font->fontwidth;
        const int overmapfont_y = termpixel_y / overmap_font->fontheight;
        const int overmapfont_x2 = std::min( termpixel_x2 / overmap_font->fontwidth, oversized_width - 1 );
        const int overmapfont_y2 = std::min( termpixel_y2 / overmap_font->fontheight, oversized_height - 1 );
        const int overmapfont_width = overmapfont_x2 - overmapfont_x + 1;
        const int overmapfont_height = overmapfont_y2 - overmapfont_y + 1;
        invalidate_framebuffer( oversized_framebuffer, overmapfont_x, overmapfont_y, overmapfont_width, overmapfont_height );
    }
}

// clear the framebuffer when werase is called on certain windows that don't use the main terminal font
void cata_cursesport::handle_additional_window_clear( WINDOW* win )
{
    if ( !g ) {
        return;
    }
    if( win == g->w_terrain || win == g->w_overmap ){
        invalidate_framebuffer( oversized_framebuffer );
    }
}

void clear_window_area( const catacurses::window &win_ )
{
    cata_cursesport::WINDOW *const win = win_.get<cata_cursesport::WINDOW>();
    FillRectDIB(win->x * fontwidth, win->y * fontheight,
                win->width * fontwidth, win->height * fontheight, catacurses::black);
}

void cata_cursesport::curses_drawwindow( const catacurses::window &w )
{
    WINDOW *const win = w.get<WINDOW>();
    bool update = false;
    if (g && w == g->w_terrain && use_tiles) {
        // Strings with colors do be drawn with map_font on top of tiles.
        std::multimap<point, formatted_text> overlay_strings;

        // game::w_terrain can be drawn by the tilecontext.
        // skip the normal drawing code for it.
        tilecontext->draw(
            win->x * fontwidth,
            win->y * fontheight,
            tripoint( g->ter_view_x, g->ter_view_y, g->ter_view_z ),
            TERRAIN_WINDOW_TERM_WIDTH * font->fontwidth,
            TERRAIN_WINDOW_TERM_HEIGHT * font->fontheight,
            overlay_strings );

        point prev_coord;
        int x_offset = 0;
        int alignment_offset = 0;
        for( const auto &iter : overlay_strings ) {
            const point coord = iter.first;
            const formatted_text ft = iter.second;
            const utf8_wrapper text( ft.text );

            // Strings at equal coords are displayed sequentially.
            if( coord != prev_coord ) {
                x_offset = 0;
            }

            // Calculate length of all strings in sequence to align them.
            if ( x_offset == 0 ) {
                int full_text_length = 0;
                const auto range = overlay_strings.equal_range( coord );
                for( auto ri = range.first; ri != range.second; ++ri ) {
                    utf8_wrapper rt(ri->second.text);
                    full_text_length += rt.display_width();
                }

                alignment_offset = 0;
                if( ft.alignment == TEXT_ALIGNMENT_CENTER ) {
                    alignment_offset = full_text_length / 2;
                }
                else if( ft.alignment == TEXT_ALIGNMENT_RIGHT ) {
                    alignment_offset = full_text_length - 1;
                }
            }

            for( size_t i = 0; i < text.display_width(); ++i ) {
                const int x0 = win->x * fontwidth;
                const int y0 = win->y * fontheight;
                const int x = x0 + ( x_offset - alignment_offset + i ) * map_font->fontwidth + coord.x;
                const int y = y0 + coord.y;                

                // Clip to window bounds.
                if( x < x0 || x > x0 + ( TERRAIN_WINDOW_TERM_WIDTH - 1 ) * font->fontwidth
                    || y < y0 || y > y0 + ( TERRAIN_WINDOW_TERM_HEIGHT - 1 ) * font->fontheight ) {
                    continue;
                }

                // TODO: draw with outline / BG color for better readability
                map_font->OutputChar( text.substr_display( i, 1 ).str(), x, y, ft.color, win->FS );
            }

            prev_coord = coord;
            x_offset = text.display_width();
        }

        invalidate_framebuffer(terminal_framebuffer, win->x, win->y, TERRAIN_WINDOW_TERM_WIDTH, TERRAIN_WINDOW_TERM_HEIGHT);

        update = true;
    } else if (g && w == g->w_terrain && map_font ) {
        // When the terrain updates, predraw a black space around its edge
        // to keep various former interface elements from showing through the gaps
        // TODO: Maybe track down screen changes and use g->w_blackspace to draw this instead

        //calculate width differences between map_font and font
        int partial_width = std::max(TERRAIN_WINDOW_TERM_WIDTH * fontwidth - TERRAIN_WINDOW_WIDTH * map_font->fontwidth, 0);
        int partial_height = std::max(TERRAIN_WINDOW_TERM_HEIGHT * fontheight - TERRAIN_WINDOW_HEIGHT * map_font->fontheight, 0);
        //Gap between terrain and lower window edge
        if( partial_height > 0 ) {
            FillRectDIB( win->x * map_font->fontwidth,
                         ( win->y + TERRAIN_WINDOW_HEIGHT ) * map_font->fontheight,
                         TERRAIN_WINDOW_WIDTH * map_font->fontwidth + partial_width, partial_height, catacurses::black );
        }
        //Gap between terrain and sidebar
        if( partial_width > 0 ) {
            FillRectDIB( ( win->x + TERRAIN_WINDOW_WIDTH ) * map_font->fontwidth, win->y * map_font->fontheight,
                         partial_width, TERRAIN_WINDOW_HEIGHT * map_font->fontheight + partial_height, catacurses::black );
        }
        // Special font for the terrain window
        update = map_font->draw_window( w );
    } else if (g && w == g->w_overmap && overmap_font ) {
        // Special font for the terrain window
        update = overmap_font->draw_window( w );
    } else if (w == w_hit_animation && map_font ) {
        // The animation window overlays the terrain window,
        // it uses the same font, but it's only 1 square in size.
        // The offset must not use the global font, but the map font
        int offsetx = win->x * map_font->fontwidth;
        int offsety = win->y * map_font->fontheight;
        update = map_font->draw_window( w, offsetx, offsety );
    } else if (g && w == g->w_blackspace) {
        // fill-in black space window skips draw code
        // so as not to confuse framebuffer any more than necessary
        int offsetx = win->x * font->fontwidth;
        int offsety = win->y * font->fontheight;
        int wwidth = win->width * font->fontwidth;
        int wheight = win->height * font->fontheight;
        FillRectDIB( offsetx, offsety, wwidth, wheight, catacurses::black );
        update = true;
    } else if (g && w == g->w_pixel_minimap && g->pixel_minimap_option) {
        // Make sure the entire minimap window is black before drawing.
        clear_window_area( w );
        tilecontext->draw_minimap(
            win->x * fontwidth, win->y * fontheight,
            tripoint( g->u.pos().x, g->u.pos().y, g->ter_view_z ),
            win->width * font->fontwidth, win->height * font->fontheight);
        update = true;
    } else {
        // Either not using tiles (tilecontext) or not the w_terrain window.
        update = font->draw_window( w );
    }
    if(update) {
        needupdate = true;
    }
}

bool Font::draw_window( const catacurses::window &win )
{
    cata_cursesport::WINDOW *const w = win.get<cata_cursesport::WINDOW>();
    // Use global font sizes here to make this independent of the
    // font used for this window.
    return draw_window( win, w->x * ::fontwidth, w->y * ::fontheight );
}

bool Font::draw_window( const catacurses::window &w, const int offsetx, const int offsety )
{
    cata_cursesport::WINDOW *const win = w.get<cata_cursesport::WINDOW>();
    //Keeping track of the last drawn window
    const cata_cursesport::WINDOW *winBuffer = static_cast<cata_cursesport::WINDOW*>( ::winBuffer.lock().get() );
    if( !fontScaleBuffer ) {
            fontScaleBuffer = tilecontext->get_tile_width();
    }
    const int fontScale = tilecontext->get_tile_width();
    //This creates a problem when map_font is different from the regular font
    //Specifically when showing the overmap
    //And in some instances of screen change, i.e. inventory.
    bool oldWinCompatible = false;

    // clear the oversized buffer proportionally
    invalidate_framebuffer_proportion( win );

    // use the oversize buffer when dealing with windows that can have a different font than the main text font
    bool use_oversized_framebuffer = g && ( w == g->w_terrain || w == g->w_overmap ||
                                            w == w_hit_animation );

    /*
    Let's try to keep track of different windows.
    A number of windows are coexisting on the screen, so don't have to interfere.

    g->w_terrain, g->w_minimap, g->w_HP, g->w_status, g->w_status2, g->w_messages,
     g->w_location, and g->w_minimap, can be buffered if either of them was
     the previous window.

    g->w_overmap and g->w_omlegend are likewise.

    Everything else works on strict equality because there aren't yet IDs for some of them.
    */
    if( g && ( w == g->w_terrain || w == g->w_minimap || w == g->w_HP || w == g->w_status ||
         w == g->w_status2 || w == g->w_messages || w == g->w_location ) ) {
        if ( winBuffer == g->w_terrain || winBuffer == g->w_minimap ||
             winBuffer == g->w_HP || winBuffer == g->w_status || winBuffer == g->w_status2 ||
             winBuffer == g->w_messages || winBuffer == g->w_location ) {
            oldWinCompatible = true;
        }
    }else if( g && ( w == g->w_overmap || w == g->w_omlegend ) ) {
        if ( winBuffer == g->w_overmap || winBuffer == g->w_omlegend ) {
            oldWinCompatible = true;
        }
    }else {
        if( win == winBuffer ) {
            oldWinCompatible = true;
        }
    }

    // @todo: Get this from UTF system to make sure it is exactly the kind of space we need
    static const std::string space_string = " ";

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

            std::vector<curseline> &framebuffer = use_oversized_framebuffer ? oversized_framebuffer :
                                             terminal_framebuffer;

            cursecell &oldcell = framebuffer[fby].chars[fbx];

            if (oldWinCompatible && cell == oldcell && fontScale == fontScaleBuffer) {
                continue;
            }
            oldcell = cell;

            if( cell.ch.empty() ) {
                continue; // second cell of a multi-cell character
            }

            // Spaces are used a lot, so this does help noticeably
            if( cell.ch == space_string ) {
                FillRectDIB( drawx, drawy, fontwidth, fontheight, cell.BG );
                continue;
            }
            const char *utf8str = cell.ch.c_str();
            int len = cell.ch.length();
            const int codepoint = UTF8_getch( &utf8str, &len );
            const catacurses::base_color FG = cell.FG;
            const catacurses::base_color BG = cell.BG;
            const cata_cursesport::font_style FS = cell.FS;
            if( codepoint != UNKNOWN_UNICODE ) {
                const int cw = utf8_width( cell.ch );
                if( cw < 1 ) {
                    // utf8_width() may return a negative width
                    continue;
                }
                FillRectDIB( drawx, drawy, fontwidth * cw, fontheight, BG );
                OutputChar( cell.ch, drawx, drawy, FG, FS );
            } else {
                FillRectDIB( drawx, drawy, fontwidth, fontheight, BG );
                draw_ascii_lines( static_cast<unsigned char>( cell.ch[0] ), drawx, drawy, FG, FS );
            }

        }
    }
    win->draw = false; //We drew the window, mark it as so
    //Keeping track of last drawn window and tilemode zoom level
    ::winBuffer = w.weak_ptr();
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

                if( !dpad_continuous ) {
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
 * or something that a call to ncurses getch would return.
 */
long sdl_keysym_to_curses( SDL_Keysym keysym )
{

    if( get_option<bool>( "DIAG_MOVE_WITH_MODIFIERS" ) ) {
        //Shift + Cursor Arrow (diagonal clockwise)
        if( keysym.mod & KMOD_SHIFT ) {
            switch( keysym.sym ) {
                case SDLK_LEFT:
                    return inp_mngr.get_first_char_for_action( "LEFTUP" );
                case SDLK_RIGHT:
                    return inp_mngr.get_first_char_for_action( "RIGHTDOWN" );
                case SDLK_UP:
                    return inp_mngr.get_first_char_for_action( "RIGHTUP" );
                case SDLK_DOWN:
                    return inp_mngr.get_first_char_for_action( "LEFTDOWN" );
            }
        }
        //Ctrl + Cursor Arrow (diagonal counter-clockwise)
        if( keysym.mod & KMOD_CTRL ) {
            switch( keysym.sym ) {
                case SDLK_LEFT:
                    return inp_mngr.get_first_char_for_action( "LEFTDOWN" );
                case SDLK_RIGHT:
                    return inp_mngr.get_first_char_for_action( "RIGHTUP" );
                case SDLK_UP:
                    return inp_mngr.get_first_char_for_action( "LEFTUP" );
                case SDLK_DOWN:
                    return inp_mngr.get_first_char_for_action( "RIGHTDOWN" );
            }
        }
    }

    switch (keysym.sym) {
        // This is special: allow entering a Unicode character with ALT+number
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
            if( keysym.mod & KMOD_SHIFT ) {
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
        case SDLK_F1:
            return KEY_F( 1 );
        case SDLK_F2:
            return KEY_F( 2 );
        case SDLK_F3:
            return KEY_F( 3 );
        case SDLK_F4:
            return KEY_F( 4 );
        case SDLK_F5:
            return KEY_F( 5 );
        case SDLK_F6:
            return KEY_F( 6 );
        case SDLK_F7:
            return KEY_F( 7 );
        case SDLK_F8:
            return KEY_F( 8 );
        case SDLK_F9:
            return KEY_F( 9 );
        case SDLK_F10:
            return KEY_F( 10 );
        case SDLK_F11:
            return KEY_F( 11 );
        case SDLK_F12:
            return KEY_F( 12 );
        case SDLK_F13:
            return KEY_F( 13 );
        case SDLK_F14:
            return KEY_F( 14 );
        case SDLK_F15:
            return KEY_F( 15 );
        // Every other key is ignored as there is no curses constant for it.
        // TODO: add more if you find more.
        default:
            return 0;
    }
}

bool handle_resize(int w, int h)
{
    if( ( w != WindowWidth ) || ( h != WindowHeight ) ) {
        WindowWidth = w;
        WindowHeight = h;
        TERMINAL_WIDTH = WindowWidth / fontwidth;
        TERMINAL_HEIGHT = WindowHeight / fontheight;
        SetupRenderTarget();
        game_ui::init_ui();
        tilecontext->reinit_minimap();

        return true;
    }
    return false;
}

void toggle_fullscreen_window()
{
    static int restore_win_w = get_option<int>( "TERMINAL_X" ) * fontwidth;
    static int restore_win_h = get_option<int>( "TERMINAL_Y" ) * fontheight;

    if ( fullscreen ) {
        if( SDL_SetWindowFullscreen( window.get(), 0 ) != 0 ) {
            dbg(D_ERROR) << "SDL_SetWinodwFullscreen failed: " << SDL_GetError();
            return;
        }
        SDL_RestoreWindow( window.get() );
        SDL_SetWindowSize( window.get(), restore_win_w, restore_win_h );
        SDL_SetWindowMinimumSize( window.get(), fontwidth * 80, fontheight * 24 );
    } else {
        restore_win_w = WindowWidth;
        restore_win_h = WindowHeight;
        if( SDL_SetWindowFullscreen( window.get(), SDL_WINDOW_FULLSCREEN_DESKTOP ) != 0 ) {
            dbg(D_ERROR) << "SDL_SetWinodwFullscreen failed: " << SDL_GetError();
            return;
        }
    }
    int nw = 0;
    int nh = 0;
    SDL_GetWindowSize( window.get(), &nw, &nh );
    handle_resize( nw, nh );
    fullscreen = !fullscreen;
}

//Check for any window messages (keypress, paint, mousemove, etc)
void CheckMessages()
{
    SDL_Event ev;
    bool quit = false;
    bool text_refresh = false;
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
                case SDL_WINDOWEVENT_RESIZED:
                    needupdate = handle_resize( ev.window.data1, ev.window.data2 );
                    break;
                default:
                    break;
                }
            break;
            case SDL_KEYDOWN:
            {
                //hide mouse cursor on keyboard input
                if(get_option<std::string>( "HIDE_CURSOR" ) != "show" && SDL_ShowCursor(-1)) {
                    SDL_ShowCursor(SDL_DISABLE);
                }
                const long lc = sdl_keysym_to_curses(ev.key.keysym);
                if( lc <= 0 ) {
                    // a key we don't know in curses and won't handle.
                    break;
#ifdef __linux__
                } else if( SDL_COMPILEDVERSION == SDL_VERSIONNUM( 2, 0, 5 ) && ev.key.repeat ) {
                    // https://bugzilla.libsdl.org/show_bug.cgi?id=3637
                    break;
#endif
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
                    if( len > 0 ) {
                        const unsigned lc = UTF8_getch( &c, &len );
                        last_input = input_event( lc, CATA_INPUT_KEYBOARD );   
                    } else {
                        // no key pressed in this event
                        last_input = input_event();
                        last_input.type = CATA_INPUT_KEYBOARD;
                    }
                    last_input.text = ev.text.text;
                    text_refresh = true;
                }
            break;
            case SDL_TEXTEDITING:
            {
                const char *c = ev.edit.text;
                int len = strlen( ev.edit.text );
                if( len > 0 ) {
                    const unsigned lc = UTF8_getch( &c, &len );
                    last_input = input_event( lc, CATA_INPUT_KEYBOARD );
                } else {
                    // no key pressed in this event
                    last_input = input_event();
                    last_input.type = CATA_INPUT_KEYBOARD;
                }
                last_input.edit = ev.edit.text;
                last_input.edit_refresh = true;
                text_refresh = true;
            }
            break;
            case SDL_JOYBUTTONDOWN:
                last_input = input_event(ev.jbutton.button, CATA_INPUT_KEYBOARD);
            break;
            case SDL_JOYAXISMOTION: // on gamepads, the axes are the analog sticks
                // TODO: somehow get the "digipad" values from the axes
            break;
            case SDL_MOUSEMOTION:
                if (get_option<std::string>( "HIDE_CURSOR" ) == "show" || get_option<std::string>( "HIDE_CURSOR" ) == "hidekb") {
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
        if( text_refresh ) {
            break;
        }
    }
    if (needupdate) {
        try_sdl_update();
    }
    if(quit) {
        catacurses::endwin();
        exit(0);
    }
}

// Check if text ends with suffix
static bool ends_with(const std::string &text, const std::string &suffix) {
    return text.length() >= suffix.length() &&
        strcasecmp(text.c_str() + text.length() - suffix.length(), suffix.c_str()) == 0;
}

//***********************************
//Pseudo-Curses Functions           *
//***********************************

static void font_folder_list(std::ofstream& fout, const std::string &path, std::set<std::string> &bitmap_fonts)
{
    for( const auto &f : get_files_from_path( "", path, true, false ) ) {
            TTF_Font_Ptr fnt( TTF_OpenFont( f.c_str(), 12 ) );
            if( !fnt ) {
                continue;
            }
            long nfaces = 0;
            nfaces = TTF_FontFaces( fnt.get() );
            fnt.reset();

            for(long i = 0; i < nfaces; i++) {
                const TTF_Font_Ptr fnt( TTF_OpenFontIndex( f.c_str(), 12, i ) );
                if( !fnt ) {
                    continue;
                }

                // Add font family
                char *fami = TTF_FontFaceFamilyName( fnt.get() );
                if (fami != NULL) {
                    fout << fami;
                } else {
                    continue;
                }

                // Add font style
                char *style = TTF_FontFaceStyleName( fnt.get() );
                bool isbitmap = ends_with(f, ".fon");
                if (style != NULL && !isbitmap && strcasecmp(style, "Regular") != 0) {
                    fout << " " << style;
                }
                if (isbitmap) {
                    std::set<std::string>::iterator it;
                    it = bitmap_fonts.find(std::string(fami));
                    if (it == bitmap_fonts.end()) {
                        // First appearance of this font family
                        bitmap_fonts.insert(fami);
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

                // We use only 1 style in bitmap fonts.
                if (isbitmap) {
                    break;
                }
            }
    }
}

static void save_font_list()
{
    std::set<std::string> bitmap_fonts;
    std::ofstream fout(FILENAMES["fontlist"].c_str(), std::ios_base::trunc);

    font_folder_list(fout, FILENAMES["fontdir"], bitmap_fonts);

#if (defined _WIN32 || defined WINDOWS)
    char buf[256];
    GetSystemWindowsDirectory(buf, 256);
    strcat(buf, "\\fonts");
    font_folder_list(fout, buf, bitmap_fonts);
#elif (defined _APPLE_ && defined _MACH_)
    /*
    // Well I don't know how osx actually works ....
    font_folder_list(fout, "/System/Library/Fonts", bitmap_fonts);
    font_folder_list(fout, "/Library/Fonts", bitmap_fonts);

    wordexp_t exp;
    wordexp("~/Library/Fonts", &exp, 0);
    font_folder_list(fout, exp.we_wordv[0], bitmap_fonts);
    wordfree(&exp);*/
#else // Other POSIX-ish systems
    font_folder_list(fout, "/usr/share/fonts", bitmap_fonts);
    font_folder_list(fout, "/usr/local/share/fonts", bitmap_fonts);
    char *home;
    if( ( home = getenv( "HOME" ) ) ) {
        std::string userfontdir = home;
        userfontdir += "/.fonts";
        font_folder_list( fout, userfontdir, bitmap_fonts );
    }
#endif
}

static std::string find_system_font( const std::string &name, int& faceIndex )
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
        while( getline( fin, fname ) && getline( fin, fpath ) && getline( fin, iline ) ) {
            if (0 == strcasecmp(fname.c_str(), name.c_str())) {
                faceIndex = atoi( iline.c_str() );
                return fpath;
            }
        }
    }

    return "";
}

// bitmap font size test
// return face index that has this size or below
static int test_face_size( const std::string &f, int size, int faceIndex )
{
    const TTF_Font_Ptr fnt( TTF_OpenFontIndex( f.c_str(), size, faceIndex ) );
    if( fnt ) {
        char* style = TTF_FontFaceStyleName( fnt.get() );
        if(style != NULL) {
            int faces = TTF_FontFaces( fnt.get() );
            for(int i = faces - 1; i >= 0; i--) {
                const TTF_Font_Ptr tf( TTF_OpenFontIndex( f.c_str(), size, i ) );
                char* ts = NULL;
                if( tf ) {
                   if( NULL != ( ts = TTF_FontFaceStyleName( tf.get() ) ) ) {
                       if( 0 == strcasecmp( ts, style ) && TTF_FontHeight( tf.get() ) <= size ) {
                           return i;
                       }
                   }
                }
            }
        }
    }

    return faceIndex;
}

// Calculates the new width of the window
int projected_window_width()
{
    return get_option<int>( "TERMINAL_X" ) * fontwidth;
}

// Calculates the new height of the window
int projected_window_height()
{
    return get_option<int>( "TERMINAL_Y" ) * fontheight;
}

//Basic Init, create the font, backbuffer, etc
void catacurses::init_interface()
{
    last_input = input_event();
    inputdelay = -1;

    font_loader fl;
    fl.load();
    ::fontwidth = fl.fontwidth;
    ::fontheight = fl.fontheight;

    if(!InitSDL()) {
        throw std::runtime_error( "InitSDL failed" );
    }

    find_videodisplays();

    TERMINAL_WIDTH = get_option<int>( "TERMINAL_X" );
    TERMINAL_HEIGHT = get_option<int>( "TERMINAL_Y" );

    if(!WinCreate()) {
        throw std::runtime_error( "WinCreate failed" ); //@todo: throw from WinCreate
    }

    dbg( D_INFO ) << "Initializing SDL Tiles context";
    tilecontext.reset( new cata_tiles( renderer.get() ) );
    try {
        tilecontext->load_tileset( get_option<std::string>( "TILES" ), true );
    } catch( const std::exception &err ) {
        dbg( D_ERROR ) << "failed to check for tileset: " << err.what();
        // use_tiles is the cached value of the USE_TILES option.
        // most (all?) code refers to this to see if cata_tiles should be used.
        // Setting it to false disables this from getting used.
        use_tiles = false;
    }

    color_loader<SDL_Color>().load( windowsPalette );
    init_colors();

    // initialize sound set
    load_soundset();

    // Reset the font pointer
    font = Font::load_font( fl.typeface, fl.fontsize, fl.fontwidth, fl.fontheight, fl.fontblending );
    if( !font ) {
        throw std::runtime_error( "loading font data failed" );
    }
    map_font = Font::load_font( fl.map_typeface, fl.map_fontsize, fl.map_fontwidth, fl.map_fontheight, fl.fontblending );
    overmap_font = Font::load_font( fl.overmap_typeface, fl.overmap_fontsize,
                                    fl.overmap_fontwidth, fl.overmap_fontheight, fl.fontblending );
    stdscr = newwin(get_terminal_height(), get_terminal_width(),0,0);
    //newwin calls `new WINDOW`, and that will throw, but not return nullptr.
}

// This is supposed to be called from init.cpp, and only from there.
void load_tileset() {
    if( !tilecontext || !use_tiles ) {
        return;
    }
    tilecontext->load_tileset( get_option<std::string>( "TILES" ) );
    tilecontext->do_tile_loading_report();
}

std::unique_ptr<Font> Font::load_font(const std::string &typeface, int fontsize, int fontwidth, int fontheight, const bool fontblending )
{
    if (ends_with(typeface, ".bmp") || ends_with(typeface, ".png")) {
        // Seems to be an image file, not a font.
        // Try to load as bitmap font.
        try {
            return std::unique_ptr<Font>( new BitmapFont( fontwidth, fontheight, FILENAMES["fontdir"] + typeface ) );
        } catch(std::exception &err) {
            dbg( D_ERROR ) << "Failed to load " << typeface << ": " << err.what();
            // Continue to load as truetype font
        }
    }
    // Not loaded as bitmap font (or it failed), try to load as truetype
    try {
        return std::unique_ptr<Font>( new CachedTTFFont( fontwidth, fontheight, typeface, fontsize, fontblending ) );
    } catch(std::exception &err) {
        dbg( D_ERROR ) << "Failed to load " << typeface << ": " << err.what();
    }
    return nullptr;
}

//Ends the terminal, destroy everything
void catacurses::endwin()
{
    tilecontext.reset();
    font.reset();
    map_font.reset();
    overmap_font.reset();
    WinDestroy();
}

template<>
SDL_Color color_loader<SDL_Color>::from_rgb( const int r, const int g, const int b )
{
    SDL_Color result;
    result.b=b;    //Blue
    result.g=g;    //Green
    result.r=r;    //Red
    //result.a=0;//The Alpha, is not used, so just set it to 0
    return result;
}

void input_manager::set_timeout( const int t )
{
    input_timeout = t;
    inputdelay = t;
}

// This is how we're actually going to handle input events, SDL getch
// is simply a wrapper around this.
input_event input_manager::get_input_event() {
    previously_pressed_key = 0;
    // standards note: getch is sometimes required to call refresh
    // see, e.g., http://linux.die.net/man/3/getch
    // so although it's non-obvious, that refresh() call (and maybe InvalidateRect?) IS supposed to be there

    wrefresh( catacurses::stdscr );

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
    tilecontext->set_draw_scale(size);
    game_ui::init_ui();
}

bool input_context::get_coordinates( const catacurses::window &capture_win_, int& x, int& y) {
    if(!coordinate_input_received) {
        return false;
    }

    cata_cursesport::WINDOW *const capture_win = ( capture_win_.get() ? capture_win_ : g->w_terrain ).get<cata_cursesport::WINDOW>();

    // this contains the font dimensions of the capture_win,
    // not necessarily the global standard font dimensions.
    int fw = fontwidth;
    int fh = fontheight;
    // tiles might have different dimensions than standard font
    if (use_tiles && capture_win == g->w_terrain) {
        fw = tilecontext->get_tile_width();
        fh = tilecontext->get_tile_height();
        // add_msg( m_info, "tile map fw %d fh %d", fw, fh);
    } else if (map_font && capture_win == g->w_terrain) {
        // map font (if any) might differ from standard font
        fw = map_font->fontwidth;
        fh = map_font->fontheight;
    }

    // Translate mouse coordinates to map coordinates based on tile size,
    // the window position is *always* in standard font dimensions!
    const int win_left = capture_win->x * fontwidth;
    const int win_top = capture_win->y * fontheight;
    // But the size of the window is in the font dimensions of the window.
    const int win_right = win_left + (capture_win->width * fw);
    const int win_bottom = win_top + (capture_win->height * fh);
    // add_msg( m_info, "win_ left %d top %d right %d bottom %d", win_left,win_top,win_right,win_bottom);
    // add_msg( m_info, "coordinate_ x %d y %d", coordinate_x, coordinate_y);
    // Check if click is within bounds of the window we care about
    if( coordinate_x < win_left || coordinate_x > win_right ||
        coordinate_y < win_top || coordinate_y > win_bottom ) {
        // add_msg( m_info, "out of bounds");
        return false;
    }

    if ( tile_iso && use_tiles ) {
        const int screen_column = round( (float) ( coordinate_x - win_left - (( win_right - win_left ) / 2 + win_left ) ) / ( fw / 2 ) );
        const int screen_row = round( (float) ( coordinate_y - win_top - ( win_bottom - win_top ) / 2 + win_top ) / ( fw / 4 ) );
        const int selected_x = ( screen_column - screen_row ) / 2;
        const int selected_y = ( screen_row + screen_column ) / 2;
        x = g->ter_view_x + selected_x;
        y = g->ter_view_y + selected_y;
    } else {
        const int selected_column = (coordinate_x - win_left) / fw;
        const int selected_row = (coordinate_y - win_top) / fh;

        x = g->ter_view_x - ((capture_win->width / 2) - selected_column);
        y = g->ter_view_y - ((capture_win->height / 2) - selected_row);
    }

    return true;
}

int get_terminal_width() {
    return TERMINAL_WIDTH;
}

int get_terminal_height() {
    return TERMINAL_HEIGHT;
}

BitmapFont::BitmapFont( const int w, const int h, const std::string &typeface )
: Font( w, h )
{
    dbg( D_INFO ) << "Loading bitmap font [" + typeface + "]." ;
    SDL_Surface_Ptr asciiload( IMG_Load( typeface.c_str() ) );
    if( !asciiload ) {
        throw std::runtime_error(IMG_GetError());
    }
    if (asciiload->w * asciiload->h < (fontwidth * fontheight * 256)) {
        throw std::runtime_error("bitmap for font is to small");
    }
    Uint32 key = SDL_MapRGB(asciiload->format, 0xFF, 0, 0xFF);
    SDL_SetColorKey( asciiload.get(),SDL_TRUE,key );
    SDL_Surface_Ptr ascii_surf[std::tuple_size<decltype( ascii )>::value];
    ascii_surf[0].reset( SDL_ConvertSurface( asciiload.get(), format.get(), 0 ) );
    SDL_SetSurfaceRLE( ascii_surf[0].get(), true );
    asciiload.reset();

    for (size_t a = 1; a < std::tuple_size<decltype( ascii )>::value; ++a) {
        ascii_surf[a].reset( SDL_ConvertSurface( ascii_surf[0].get(), format.get(), 0 ) );
        SDL_SetSurfaceRLE( ascii_surf[a].get(), true );
    }

    for (size_t a = 0; a < std::tuple_size<decltype( ascii )>::value - 1; ++a) {
        SDL_LockSurface( ascii_surf[a].get() );
        int size = ascii_surf[a]->h * ascii_surf[a]->w;
        Uint32 *pixels = (Uint32 *)ascii_surf[a]->pixels;
        Uint32 color = (windowsPalette[a].r << 16) | (windowsPalette[a].g << 8) | windowsPalette[a].b;
        for(int i = 0; i < size; i++) {
            if(pixels[i] == 0xFFFFFF) {
                pixels[i] = color;
            }
        }
        SDL_UnlockSurface( ascii_surf[a].get() );
    }
    tilewidth = ascii_surf[0]->w / fontwidth;

    //convert ascii_surf to SDL_Texture
    for( size_t a = 0; a < std::tuple_size<decltype( ascii )>::value; ++a) {
        ascii[a].reset( SDL_CreateTextureFromSurface( renderer.get(), ascii_surf[a].get() ) );
    }
}

void BitmapFont::draw_ascii_lines(unsigned char line_id, int drawx, int drawy, int FG, cata_cursesport::font_style FS) const
{
    (void) FS; // unused
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



CachedTTFFont::CachedTTFFont( const int w, const int h, std::string _typeface, int _fontsize, const bool _fontblending )
: Font( w, h )
, fontblending( _fontblending )
, typeface( _typeface )
, fontsize( _fontsize )
, faceIndex( 0 )
{
    const std::string sysfnt = find_system_font(typeface, faceIndex);
    if (!sysfnt.empty()) {
        typeface = sysfnt;
        dbg( D_INFO ) << "Using font [" + typeface + "]." ;
    }
    //make fontdata compatible with wincurse
    if(!file_exist(typeface)) {
        faceIndex = 0;
        typeface = FILENAMES["fontdir"] + typeface + ".ttf";
        dbg( D_INFO ) << "Using compatible font [" + typeface + "]." ;
    }
    //different default font with wincurse
    if(!file_exist(typeface)) {
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

    // Preload normal styled font to force exception here
    TTF_Font *font = get_font(cata_cursesport::font_style());
    (void) font;
}

TTF_Font *CachedTTFFont::get_font(cata_cursesport::font_style FS)
{
    auto it = font_map.find(FS);
    TTF_Font *_font = NULL;
    if (it != font_map.end()) {
        _font = it->second.get();
        return _font;
    }

    int style = TTF_STYLE_NORMAL;
    int shrink_to_fit_style = TTF_STYLE_NORMAL;
    if ( FS[cata_cursesport::FS_BOLD] ) {
        style |= TTF_STYLE_BOLD;
        shrink_to_fit_style |= TTF_STYLE_BOLD;
    }
    if ( FS[cata_cursesport::FS_ITALIC] ) {
        style |= TTF_STYLE_ITALIC;
    }
    if ( FS[cata_cursesport::FS_STRIKETHROUGH] ) {
        style |= TTF_STYLE_STRIKETHROUGH;
        shrink_to_fit_style |= TTF_STYLE_STRIKETHROUGH;
    }
    if ( FS[cata_cursesport::FS_UNDERLINE] ) {
        style |= TTF_STYLE_UNDERLINE;
        shrink_to_fit_style |= TTF_STYLE_UNDERLINE;
    }

    int _fontsize = fontsize;
    while ( _fontsize > 0 ) {
        _font = TTF_OpenFontIndex( typeface.c_str(), _fontsize, faceIndex );
        if( !_font ) {
            throw std::runtime_error(TTF_GetError());
        }
        TTF_SetFontStyle( _font, shrink_to_fit_style );
        int width = 0;
        int height = 0;
        // @todo: Check all to get maximum?
        TTF_SizeText( _font, "#", &width, &height );
        if ( width <= fontwidth && height <= fontheight ) {
            break;
        }
        TTF_CloseFont( _font );
        _font = NULL;
        _fontsize--;
    }
    if (!_font) {
        throw std::runtime_error("No font size that satisfies the requirements found");
    }
    TTF_SetFontStyle( _font, style );
    font_map.emplace( FS, TTF_Font_Ptr( _font ) );

    return _font;
}

int map_font_width() {
    if (use_tiles && tilecontext ) {
        return tilecontext->get_tile_width();
    }
    return (map_font ? map_font : font)->fontwidth;
}

int map_font_height() {
    if (use_tiles && tilecontext ) {
        return tilecontext->get_tile_height();
    }
    return (map_font ? map_font : font)->fontheight;
}

int overmap_font_width() {
    return (overmap_font ? overmap_font : font)->fontwidth;
}

int overmap_font_height() {
    return (overmap_font ? overmap_font : font)->fontheight;
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

SDL_Color cursesColorToSDL( const nc_color &color ) {
    const int pair_id = color.to_color_pair_index();
    const auto pair = cata_cursesport::colorpairs[pair_id];

    int palette_index = pair.FG != 0 ? pair.FG : pair.BG;

    if( color.is_bold() ) {
        palette_index += color_loader<SDL_Color>::COLOR_NAMES_COUNT / 2;
    }

    return windowsPalette[palette_index];
}

#ifdef SDL_SOUND

void musicFinished();

void play_music_file( const std::string &filename, int volume ) {
    const std::string path = ( current_soundpack_path + "/" + filename );
    current_music = Mix_LoadMUS(path.c_str());
    if( current_music == nullptr ) {
        dbg( D_ERROR ) << "Failed to load audio file " << path << ": " << Mix_GetError();
        return;
    }
    Mix_VolumeMusic(get_option<bool>( "SOUND_ENABLED" ) ? volume * get_option<int>( "MUSIC_VOLUME" ) / 100 : 0);
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

    const auto iter = playlists.find( current_playlist );
    if( iter == playlists.end() ) {
        return;
    }
    const music_playlist &list = iter->second;
    if( list.entries.empty() ) {
        return;
    }

    // Load the next file to play.
    absolute_playlist_at++;

    // Wrap around if we reached the end of the playlist.
    if( absolute_playlist_at >= list.entries.size() ) {
        absolute_playlist_at = 0;
    }

    current_playlist_at = playlist_indexes.at( absolute_playlist_at );

    const auto &next = list.entries[current_playlist_at];
    play_music_file( next.file, next.volume );
}
#endif

void play_music(std::string playlist) {
#ifdef SDL_SOUND
    const auto iter = playlists.find( playlist );
    if( iter == playlists.end() ) {
        return;
    }
    const music_playlist &list = iter->second;
    if( list.entries.empty() ) {
        return;
    }

    // Don't interrupt playlist that's already playing.
    if(playlist == current_playlist) {
        return;
    }

    for( size_t i = 0; i < list.entries.size(); i++ ) {
        playlist_indexes.push_back( i );
    }
    if( list.shuffle ) {
        std::random_shuffle( playlist_indexes.begin(), playlist_indexes.end() );
    }

    current_playlist = playlist;
    current_playlist_at = playlist_indexes.at( absolute_playlist_at );

    const auto &next = list.entries[current_playlist_at];
    play_music_file( next.file, next.volume );
#else
    (void)playlist;
#endif
}

void update_music_volume() {
#ifdef SDL_SOUND
    Mix_VolumeMusic( get_option<bool>( "SOUND_ENABLED" ) ? get_option<int>( "MUSIC_VOLUME" ) : 0 );
#endif
}

#ifdef SDL_SOUND
void sfx::load_sound_effects( JsonObject &jsobj ) {
    const id_and_variant key( jsobj.get_string( "id" ), jsobj.get_string( "variant", "default" ) );
    const int volume = jsobj.get_int( "volume", 100 );
    auto &effects = sound_effects_p[key];

    JsonArray jsarr = jsobj.get_array( "files" );
    while( jsarr.has_more() ) {
        sound_effect new_sound_effect;
        const std::string file = jsarr.next_string();
        std::string path = ( current_soundpack_path + "/" + file );
        new_sound_effect.chunk.reset( Mix_LoadWAV( path.c_str() ) );
        if( !new_sound_effect.chunk ) {
            dbg( D_ERROR ) << "Failed to load audio file " << path << ": " << Mix_GetError();
            continue; // don't want empty chunks in the map
        }
        new_sound_effect.volume = volume;

        effects.push_back( std::move( new_sound_effect ) );
    }
}

void sfx::load_playlist( JsonObject &jsobj )
{
    JsonArray jarr = jsobj.get_array( "playlists" );
    while( jarr.has_more() ) {
        JsonObject playlist = jarr.next_object();

        const std::string playlist_id = playlist.get_string( "id" );
        music_playlist playlist_to_load;
        playlist_to_load.shuffle = playlist.get_bool( "shuffle", false );

        JsonArray files = playlist.get_array( "files" );
        while( files.has_more() ) {
            JsonObject entry = files.next_object();
            const music_playlist::entry e{ entry.get_string( "file" ),  entry.get_int( "volume" ) };
            playlist_to_load.entries.push_back( e );
        }

        playlists[playlist_id] = std::move( playlist_to_load );
    }
}

// Returns a random sound effect matching given id and variant or `nullptr` if there is no
// matching sound effect.
const sound_effect* find_random_effect( const id_and_variant &id_variants_pair )
{
    const auto iter = sound_effects_p.find( id_variants_pair );
    if( iter == sound_effects_p.end() ) {
        return nullptr;
    }
    return &random_entry_ref( iter->second );
}
// Same as above, but with fallback to "default" variant. May still return `nullptr`
const sound_effect* find_random_effect( const std::string &id, const std::string& variant )
{
    const auto eff = find_random_effect( id_and_variant( id, variant ) );
    if( eff != nullptr ) {
        return eff;
    }
    return find_random_effect( id_and_variant( id, "default" ) );
}

// Contains the chunks that have been dynamically created via do_pitch_shift. It is used to
// distinguish between dynamically created chunks and static chunks (the later must not be freed).
std::set<Mix_Chunk*> dynamic_chunks;
// Deletes the dynamically created chunk (if such a chunk had been played).
void cleanup_when_channel_finished( int channel )
{
    Mix_Chunk *chunk = Mix_GetChunk( channel );
    const auto iter = dynamic_chunks.find( chunk );
    if( iter != dynamic_chunks.end() ) {
        dynamic_chunks.erase( iter );
        free( chunk->abuf );
        free( chunk );
    }
}

Mix_Chunk *do_pitch_shift( Mix_Chunk *s, float pitch ) {
    Mix_Chunk *result;
    Uint32 s_in = s->alen / 4;
    Uint32 s_out = ( Uint32 )( ( float )s_in * pitch );
    float pitch_real = ( float )s_out / ( float )s_in;
    Uint32 i, j;
    result = ( Mix_Chunk * )malloc( sizeof( Mix_Chunk ) );
    dynamic_chunks.insert( result );
    result->allocated = 1;
    result->alen = s_out * 4;
    result->abuf = ( Uint8* )malloc( result->alen * sizeof( Uint8 ) );
    result->volume = s->volume;
    for( i = 0; i < s_out; i++ ) {
        Sint16 lt;
        Sint16 rt;
        Sint16 lt_out;
        Sint16 rt_out;
        Sint64 lt_avg = 0;
        Sint64 rt_avg = 0;
        Uint32 begin = ( Uint32 )( ( float )i / pitch_real );
        Uint32 end = ( Uint32 )( ( float )( i + 1 ) / pitch_real );

        // check for boundary case
        if( end > 0 && ( end >= ( s->alen / 4 ) ) )
            end = begin;

        for( j = begin; j <= end; j++ ) {
            lt = ( s->abuf[( 4 * j ) + 1] << 8 ) | ( s->abuf[( 4 * j ) + 0] );
            rt = ( s->abuf[( 4 * j ) + 3] << 8 ) | ( s->abuf[( 4 * j ) + 2] );
            lt_avg += lt;
            rt_avg += rt;
        }
        lt_out = ( Sint16 )( ( float )lt_avg / ( float )( end - begin + 1 ) );
        rt_out = ( Sint16 )( ( float )rt_avg / ( float )( end - begin + 1 ) );
        result->abuf[( 4 * i ) + 1] = (Uint8)(( lt_out >> 8 ) & 0xFF);
        result->abuf[( 4 * i ) + 0] = (Uint8)(lt_out & 0xFF);
        result->abuf[( 4 * i ) + 3] = (Uint8)(( rt_out >> 8 ) & 0xFF);
        result->abuf[( 4 * i ) + 2] = (Uint8)(rt_out & 0xFF);
    }
    return result;
}

void sfx::play_variant_sound( const std::string &id, const std::string &variant, int volume ) {
    if( volume == 0 ) {
        return;
    }

    const sound_effect* eff = find_random_effect( id, variant );
    if( eff == nullptr ) {
        eff = find_random_effect( id, "default" );
        if( eff == nullptr ) {
            return;
        }
    }
    const sound_effect& selected_sound_effect = *eff;

    Mix_Chunk *effect_to_play = selected_sound_effect.chunk.get();
    Mix_VolumeChunk( effect_to_play, !get_option<bool>( "SOUND_ENABLED" ) ? 0 :
                     selected_sound_effect.volume * get_option<int>( "SOUND_EFFECT_VOLUME" ) * volume / ( 100 * 100 ) );
    Mix_PlayChannel( -1, effect_to_play, 0 );
}

void sfx::play_variant_sound( const std::string &id, const std::string &variant, int volume, int angle,
                              float pitch_min, float pitch_max ) {
    if( volume == 0 ) {
        return;
    }

    const sound_effect* eff = find_random_effect( id, variant );
    if( eff == nullptr ) {
        return;
    }
    const sound_effect& selected_sound_effect = *eff;

    Mix_ChannelFinished( cleanup_when_channel_finished );
    Mix_Chunk *effect_to_play = selected_sound_effect.chunk.get();
    float pitch_random = rng_float( pitch_min, pitch_max );
    Mix_Chunk *shifted_effect = do_pitch_shift( effect_to_play, pitch_random );
    Mix_VolumeChunk( shifted_effect, !get_option<bool>( "SOUND_ENABLED" ) ? 0 :
                     selected_sound_effect.volume * get_option<int>( "SOUND_EFFECT_VOLUME" ) * volume / ( 100 * 100 ) );
    int channel = Mix_PlayChannel( -1, shifted_effect, 0 );
    Mix_SetPosition( channel, angle, 1 );
}

void sfx::play_ambient_variant_sound( const std::string &id, const std::string &variant, int volume, int channel,
                                      int duration ) {
    if( volume == 0 ) {
        return;
    }

    const sound_effect* eff = find_random_effect( id, variant );
    if( eff == nullptr ) {
        return;
    }
    const sound_effect& selected_sound_effect = *eff;

    Mix_Chunk *effect_to_play = selected_sound_effect.chunk.get();
    Mix_VolumeChunk( effect_to_play, !get_option<bool>( "SOUND_ENABLED" ) ? 0 :
                     selected_sound_effect.volume * get_option<int>( "SOUND_EFFECT_VOLUME" ) * volume / ( 100 * 100 ) );
    if( Mix_FadeInChannel( channel, effect_to_play, -1, duration ) == -1 ) {
        dbg( D_ERROR ) << "Failed to play sound effect: " << Mix_GetError();
    }
}
#endif

void load_soundset() {
#ifdef SDL_SOUND
    const std::string default_path = FILENAMES["defaultsounddir"];
    const std::string default_soundpack = "basic";
    std::string current_soundpack = get_option<std::string>( "SOUNDPACKS" );
    std::string soundpack_path;

    // Get current soundpack and it's directory path.
    if (current_soundpack.empty()) {
        dbg( D_ERROR ) << "Soundpack not set in options or empty.";
        soundpack_path = default_path;
        current_soundpack = default_soundpack;
    } else {
        dbg( D_INFO ) << "Current soundpack is: " << current_soundpack;
        soundpack_path = SOUNDPACKS[current_soundpack];
    }

    if (soundpack_path.empty()) {
        dbg( D_ERROR ) << "Soundpack with name " << current_soundpack << " can't be found or empty string";
        soundpack_path = default_path;
        current_soundpack = default_soundpack;
    } else {
        dbg( D_INFO ) << '"' << current_soundpack << '"' << " soundpack: found path: " << soundpack_path;
    }

    current_soundpack_path = soundpack_path;
    try {
        loading_ui ui( false );
        DynamicDataLoader::get_instance().load_data_from_path( soundpack_path, "core", ui );
    } catch( const std::exception &err ) {
        dbg( D_ERROR ) << "failed to load sounds: " << err.what();
    }
#endif
}

void cleanup_sound() {
#ifdef SDL_SOUND
    sound_effects_p.clear();
    playlists.clear();
#endif
}

#endif // TILES
