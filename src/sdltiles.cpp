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
#include <sys/stat.h>
#include "cata_tiles.h"
#include "get_version.h"
#include "init.h"
//

#ifdef _MSC_VER
#include "wdirent.h"
#include <direct.h>
#else
#include <dirent.h>
#endif

// SDL headers end up in different places depending on the OS, sadly
#if (defined _WIN32 || defined WINDOWS)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include "SDL.h"
#include "SDL_ttf.h"
#ifdef SDLTILES
#include "SDL_image.h" // Make sure to add this to the other OS inclusions
#endif
#ifndef strcasecmp
#define strcasecmp strcmpi
#endif
#else
#include <wordexp.h>
#if (defined OSX_SDL_FW)
#include "SDL.h"
#include "SDL_ttf/SDL_ttf.h"
#ifdef SDLTILES
#include "SDL_image/SDL_image.h" // Make sure to add this to the other OS inclusions
#endif
#else
#include "SDL/SDL.h"
#include "SDL/SDL_ttf.h"
#ifdef SDLTILES
#include "SDL/SDL_image.h" // Make sure to add this to the other OS inclusions
#endif
#endif
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

static SDL_Color windowsPalette[256];
static SDL_Window *window = NULL;
static SDL_Renderer* renderer = NULL;
static SDL_Surface *screen = NULL;
static SDL_Surface *glyph_cache[128][16]; //cache ascii characters
static SDL_Surface *ascii[16];
int tilewidth = 0;
TTF_Font* font;
static int ttf_height_hack = 0;
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
int halfwidth;          //half of the font width, used for centering lines
int halfheight;          //half of the font height, used for centering lines
std::map< std::string,std::vector<int> > consolecolors;
void (*OutputChar)(Uint16 t, int x, int y, unsigned char color);

static SDL_Joystick *joystick; // Only one joystick for now.

static bool fontblending = false;

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
    SDL_FillRect(screen, NULL, 0);
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

    //SDL_EnableUNICODE(1);
    //SDL_EnableKeyRepeat(500, OPTIONS["INPUT_DELAY"]);

    atexit(SDL_Quit);

    return true;
};

//Registers, creates, and shows the Window!!
bool WinCreate()
{

    std::string version = string_format("Cataclysm: Dark Days Ahead - %s", getVersionString());

    //Flags used for setting up SDL VideoMode
    int window_flags = 0;

    //If FULLSCREEN was selected in options add SDL_WINDOW_FULLSCREEN flag to screen_flags, causing screen to go fullscreen.
    if(OPTIONS["FULLSCREEN"]) {
        window_flags = window_flags | SDL_WINDOW_FULLSCREEN;
    }

	window = SDL_CreateWindow(version.c_str(),
			SDL_WINDOWPOS_CENTERED,
			SDL_WINDOWPOS_CENTERED,
			WindowWidth,
			WindowHeight,
			window_flags
		);
		
	//create renderer and convert that to a SDL_Surface?

    if (window == NULL) return false;
	
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	
	if(renderer == NULL) {
		renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
		if(renderer == NULL) {
			return false;
		}
	}
	
	//we have to do it this way, since cata is based on old SDL 1.2 technology
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    screen = SDL_CreateRGBSurface(0, WindowWidth, WindowHeight, 32, 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
#else
    screen = SDL_CreateRGBSurface(0, WindowWidth, WindowHeight, 32, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
#endif

    ClearScreen();

    if(OPTIONS["HIDE_CURSOR"] != "show" && SDL_ShowCursor(-1))
        SDL_ShowCursor(SDL_DISABLE);
    else
        SDL_ShowCursor(SDL_ENABLE);

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

    return true;
};

void WinDestroy()
{
    if(joystick) {
        SDL_JoystickClose(joystick);
        joystick = 0;
    }

	if(screen) {
		SDL_FreeSurface(screen);
	}
	
	if(renderer) {
		SDL_DestroyRenderer(renderer);
	}
	renderer = NULL; 
	if(window) {
		SDL_DestroyWindow(window);
	}
    window = NULL;
};

//The following 3 methods use mem functions for fast drawing
inline void VertLineDIB(int x, int y, int y2, int thickness, unsigned char color)
{
    SDL_Rect rect;
    rect.x = x;
    rect.y = y;
    rect.w = thickness;
    rect.h = y2-y;
    SDL_FillRect(screen, &rect, SDL_MapRGB(screen->format, windowsPalette[color].r,windowsPalette[color].g,windowsPalette[color].b));
};
inline void HorzLineDIB(int x, int y, int x2, int thickness, unsigned char color)
{
    SDL_Rect rect;
    rect.x = x;
    rect.y = y;
    rect.w = x2-x;
    rect.h = thickness;
    SDL_FillRect(screen, &rect, SDL_MapRGB(screen->format, windowsPalette[color].r,windowsPalette[color].g,windowsPalette[color].b));
};
inline void FillRectDIB(int x, int y, int width, int height, unsigned char color)
{
    SDL_Rect rect;
    rect.x = x;
    rect.y = y;
    rect.w = width;
    rect.h = height;
    SDL_FillRect(screen, &rect, SDL_MapRGB(screen->format, windowsPalette[color].r,windowsPalette[color].g,windowsPalette[color].b));
};


static void cache_glyphs()
{

    int top = 999;
    int bottom = -999;
    start_color();

    for(int ch = 0; ch < 128; ch++) {
        for(int color = 0; color < 16; color++) {
            SDL_Surface * glyph = glyph_cache[ch][color] =
              (fontblending ? TTF_RenderGlyph_Blended : TTF_RenderGlyph_Solid)(font, ch, windowsPalette[color]);
            int minx, maxx, miny, maxy, advance;
            if(glyph != NULL && color==0 &&
               0 == TTF_GlyphMetrics(font, ch, &minx, &maxx, &miny, &maxy, &advance) ) {
                int t = TTF_FontAscent(font)-maxy;
                int b = t + glyph->h;
                if(t < top) top = t;
                if(b > bottom) bottom = b;
            }
        }
    }

    int height = bottom - top;
    int delta = (fontheight - height) / 2;

    ttf_height_hack =  delta - top;
}

static void OutputFontChar(Uint16 t, int x, int y, unsigned char color)
{
    color &= 0xf;

    SDL_Surface * glyph = t < 0x80 ? glyph_cache[t][color] :
      (fontblending ? TTF_RenderGlyph_Blended : TTF_RenderGlyph_Solid) (font, t, windowsPalette[color]);

    if(glyph) {
        int minx = 0, maxy = 0, dx = 0, dy = 0;
        if( 0 == TTF_GlyphMetrics(font, t, &minx, NULL, NULL, &maxy, NULL))
        {
            dx = minx;
            dy = TTF_FontAscent(font) - maxy + ttf_height_hack;
            SDL_Rect rect;
            rect.x = x + dx;
            rect.y = y + dy;
            rect.w = fontwidth;
            rect.h = fontheight;
            SDL_BlitSurface(glyph, NULL, screen, &rect);
        }
        if(t >= 0x80) SDL_FreeSurface(glyph);
    }
}

#ifdef SDLTILES
static void OutputImageChar(Uint16 t, int x, int y, unsigned char color)
{
    SDL_Rect src;
    src.x = (t % tilewidth) * fontwidth;
    src.y = (t / tilewidth) * fontheight;
    src.w = fontwidth;
    src.h = fontheight;
    SDL_Rect rect;
    rect.x = x; rect.y = y; rect.w = fontwidth; rect.h = fontheight;
    SDL_BlitSurface(ascii[color], &src, screen, &rect);
}

// only update if the set interval has elapsed
void try_update()
{
    unsigned long now = SDL_GetTicks();
    if (now - lastupdate >= interval) {
		//copy screen to renderer
		//we have to move the surface to texture memory and then update the renderer
		//SDL 1.2 pretty much did this behind the scenes
		SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, screen);
		SDL_RenderCopy(renderer,texture,NULL,NULL);
		SDL_RenderPresent(renderer);
		SDL_DestroyTexture(texture);
		
        needupdate = false;
        lastupdate = now;
    } else {
        needupdate = true;
    }
}

void curses_drawwindow(WINDOW *win)
{
    int i,j,w,drawx,drawy;
    unsigned tmp;

	bool inner_update = false;

    for (j=0; j<win->height; j++){
        if (win->line[j].touched) {
            if (!inner_update) {
				inner_update = true;
            }

            needupdate = true;

            win->line[j].touched=false;

            for (i=0,w=0; w<win->width; i++,w++) {
                drawx=((win->x+w)*fontwidth);
                drawy=((win->y+j)*fontheight);//-j;
                if (((drawx+fontwidth)<=WindowWidth) && ((drawy+fontheight)<=WindowHeight)){
                const char* utf8str = win->line[j].chars+i;
                int len = ANY_LENGTH;
                tmp = UTF8_getch(&utf8str, &len);
                int FG = win->line[j].FG[w];
                int BG = win->line[j].BG[w];
                FillRectDIB(drawx,drawy,fontwidth,fontheight,BG);

                if ( tmp != UNKNOWN_UNICODE){
                    int cw = mk_wcwidth(tmp);
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
                    switch ((unsigned char)win->line[j].chars[i]) {
                    case LINE_OXOX_C://box bottom/top side (horizontal line)
                        HorzLineDIB(drawx,drawy+halfheight,drawx+fontwidth,1,FG);
                        break;
                    case LINE_XOXO_C://box left/right side (vertical line)
                        VertLineDIB(drawx+halfwidth,drawy,drawy+fontheight,2,FG);
                        break;
                    case LINE_OXXO_C://box top left
                        HorzLineDIB(drawx+halfwidth,drawy+halfheight,drawx+fontwidth,1,FG);
                        VertLineDIB(drawx+halfwidth,drawy+halfheight,drawy+fontheight,2,FG);
                        break;
                    case LINE_OOXX_C://box top right
                        HorzLineDIB(drawx,drawy+halfheight,drawx+halfwidth,1,FG);
                        VertLineDIB(drawx+halfwidth,drawy+halfheight,drawy+fontheight,2,FG);
                        break;
                    case LINE_XOOX_C://box bottom right
                        HorzLineDIB(drawx,drawy+halfheight,drawx+halfwidth,1,FG);
                        VertLineDIB(drawx+halfwidth,drawy,drawy+halfheight+1,2,FG);
                        break;
                    case LINE_XXOO_C://box bottom left
                        HorzLineDIB(drawx+halfwidth,drawy+halfheight,drawx+fontwidth,1,FG);
                        VertLineDIB(drawx+halfwidth,drawy,drawy+halfheight+1,2,FG);
                        break;
                    case LINE_XXOX_C://box bottom north T (left, right, up)
                        HorzLineDIB(drawx,drawy+halfheight,drawx+fontwidth,1,FG);
                        VertLineDIB(drawx+halfwidth,drawy,drawy+halfheight,2,FG);
                        break;
                    case LINE_XXXO_C://box bottom east T (up, right, down)
                        VertLineDIB(drawx+halfwidth,drawy,drawy+fontheight,2,FG);
                        HorzLineDIB(drawx+halfwidth,drawy+halfheight,drawx+fontwidth,1,FG);
                        break;
                    case LINE_OXXX_C://box bottom south T (left, right, down)
                        HorzLineDIB(drawx,drawy+halfheight,drawx+fontwidth,1,FG);
                        VertLineDIB(drawx+halfwidth,drawy+halfheight,drawy+fontheight,2,FG);
                        break;
                    case LINE_XXXX_C://box X (left down up right)
                        HorzLineDIB(drawx,drawy+halfheight,drawx+fontwidth,1,FG);
                        VertLineDIB(drawx+halfwidth,drawy,drawy+fontheight,2,FG);
                        break;
                    case LINE_XOXX_C://box bottom east T (left, down, up)
                        VertLineDIB(drawx+halfwidth,drawy,drawy+fontheight,2,FG);
                        HorzLineDIB(drawx,drawy+halfheight,drawx+halfwidth,1,FG);
                        break;
                    default:
                        break;
                    }
                    };//switch (tmp)
                }//(tmp < 0)
            };//for (i=0;i<_windows[w].width;i++)
        }
    };// for (j=0;j<_windows[w].height;j++)
    win->draw=false;                //We drew the window, mark it as so

    if (g && win == g->w_terrain && use_tiles) {
        //GfxDraw(thegame, win->x*fontwidth, win->y*fontheight, thegame->terrain_view_x, thegame->terrain_view_y, win->width*fontwidth, win->height*fontheight);
        tilecontext->draw(win->x * fontwidth, win->y * fontheight, g->ter_view_x, g->ter_view_y, tilecontext->terrain_term_x * fontwidth, tilecontext->terrain_term_y * fontheight);
    }
//*/
	//just use the regular update system
    if (inner_update || needupdate) try_update();
}
#else
void curses_drawwindow(WINDOW *win)
{
    int i,j,w,drawx,drawy;
    unsigned tmp;

    int miny = 99999;
    int maxy = -99999;

    for (j=0; j<win->height; j++)
    {
        if (win->line[j].touched)
        {
            if(j<miny) {
                miny=j;
            }
            if(j>maxy) {
                maxy=j;
            }


            win->line[j].touched=false;

            for (i=0,w=0; w<win->width; i++,w++)
            {
                drawx=((win->x+w)*fontwidth);
                drawy=((win->y+j)*fontheight);//-j;
                if (((drawx+fontwidth)<=WindowWidth) && ((drawy+fontheight)<=WindowHeight))
                {
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
                        if(0!=tmp) {
                            OutputChar(tmp, drawx,drawy,FG);
                        }
                    } else {
                        switch ((unsigned char)win->line[j].chars[i]) {
                        case LINE_OXOX_C://box bottom/top side (horizontal line)
                            HorzLineDIB(drawx,drawy+halfheight,drawx+fontwidth,1,FG);
                            break;
                        case LINE_XOXO_C://box left/right side (vertical line)
                            VertLineDIB(drawx+halfwidth,drawy,drawy+fontheight,2,FG);
                            break;
                        case LINE_OXXO_C://box top left
                            HorzLineDIB(drawx+halfwidth,drawy+halfheight,drawx+fontwidth,1,FG);
                            VertLineDIB(drawx+halfwidth,drawy+halfheight,drawy+fontheight,2,FG);
                            break;
                        case LINE_OOXX_C://box top right
                            HorzLineDIB(drawx,drawy+halfheight,drawx+halfwidth,1,FG);
                            VertLineDIB(drawx+halfwidth,drawy+halfheight,drawy+fontheight,2,FG);
                            break;
                        case LINE_XOOX_C://box bottom right
                            HorzLineDIB(drawx,drawy+halfheight,drawx+halfwidth,1,FG);
                            VertLineDIB(drawx+halfwidth,drawy,drawy+halfheight+1,2,FG);
                            break;
                        case LINE_XXOO_C://box bottom left
                            HorzLineDIB(drawx+halfwidth,drawy+halfheight,drawx+fontwidth,1,FG);
                            VertLineDIB(drawx+halfwidth,drawy,drawy+halfheight+1,2,FG);
                            break;
                        case LINE_XXOX_C://box bottom north T (left, right, up)
                            HorzLineDIB(drawx,drawy+halfheight,drawx+fontwidth,1,FG);
                            VertLineDIB(drawx+halfwidth,drawy,drawy+halfheight,2,FG);
                            break;
                        case LINE_XXXO_C://box bottom east T (up, right, down)
                            VertLineDIB(drawx+halfwidth,drawy,drawy+fontheight,2,FG);
                            HorzLineDIB(drawx+halfwidth,drawy+halfheight,drawx+fontwidth,1,FG);
                            break;
                        case LINE_OXXX_C://box bottom south T (left, right, down)
                            HorzLineDIB(drawx,drawy+halfheight,drawx+fontwidth,1,FG);
                            VertLineDIB(drawx+halfwidth,drawy+halfheight,drawy+fontheight,2,FG);
                            break;
                        case LINE_XXXX_C://box X (left down up right)
                            HorzLineDIB(drawx,drawy+halfheight,drawx+fontwidth,1,FG);
                            VertLineDIB(drawx+halfwidth,drawy,drawy+fontheight,2,FG);
                            break;
                        case LINE_XOXX_C://box bottom east T (left, down, up)
                            VertLineDIB(drawx+halfwidth,drawy,drawy+fontheight,2,FG);
                            HorzLineDIB(drawx,drawy+halfheight,drawx+halfwidth,1,FG);
                            break;
                        default:
                            break;
                        }//switch (tmp)
                    }
                }//(tmp < 0)
            }//for (i=0;i<_windows[w].width;i++)
        }
    }// for (j=0;j<_windows[w].height;j++)
    win->draw=false;                //We drew the window, mark it as so

    if(maxy>=0)
    {
        int tx=win->x, ty=win->y+miny, tw=win->width, th=maxy-miny+1;
        int maxw=WindowWidth/fontwidth, maxh=WindowHeight/fontheight;
        if(tw+tx>maxw) {
            tw= maxw-tx;
        }
        if(th+ty>maxh) {
            th= maxh-ty;
        }
		try_update();
        //SDL_UpdateRect(screen, tx*fontwidth, ty*fontheight, tw*fontwidth, th*fontheight);
    }
}
#endif

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
            case SDL_KEYDOWN:
            {
                int lc = 0;
                //hide mouse cursor on keyboard input
                if(OPTIONS["HIDE_CURSOR"] != "show" && SDL_ShowCursor(-1)) { SDL_ShowCursor(SDL_DISABLE); }
                const Uint8 *keystate = SDL_GetKeyboardState(NULL);
                // manually handle Alt+F4 for older SDL lib, no big deal
                if( ev.key.keysym.sym == SDLK_F4 && (keystate[SDL_SCANCODE_RALT] || keystate[SDL_SCANCODE_LALT]) ) {
                    quit = true;
                    break;
                }
                switch (ev.key.keysym.sym) {
					case SDLK_BACKSPACE:
					case SDLK_KP_BACKSPACE:
						lc=127;
						break;
					case SDLK_KP_ENTER:
					case SDLK_RETURN:
					case SDLK_RETURN2:
						lc=10;
						break;
                    case SDLK_RSHIFT:
					case SDLK_LSHIFT:
					case SDLK_RCTRL:
					case SDLK_LCTRL:
					case SDLK_RALT:
                        lc= 0;
                        break; // temporary fix for unwanted keys
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
                if( ev.key.keysym.sym == SDLK_LALT ) {
                    int code = end_alt_code();
                    if( code ) { lastchar = code; }
                }
            }
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
    if (needupdate) { try_update(); }
#endif
    if(quit) {
        endwin();
        exit(0);
    }
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
            #if (defined _WIN32 || defined WINDOWS)
                std::string f = path + "\\" + ent->d_name;
            #else
                std::string f = path + "/" + ent->d_name;
            #endif

            struct stat stat_buffer;
            if( stat( f.c_str(), &stat_buffer ) == -1 ) {
                continue;
            }
            if( S_ISDIR(stat_buffer.st_mode) ) {
                font_folder_list( fout, f );
                continue;
            }
            TTF_Font* fnt = TTF_OpenFont(f.c_str(), 12);
            long nfaces = 0;
            if(fnt != NULL) {
                nfaces = TTF_FontFaces(fnt);
                TTF_CloseFont(fnt);
                fnt = NULL;
            }
            for(long i = 0; i < nfaces; i++) {
                fnt = TTF_OpenFontIndex(f.c_str(), 12, i);
                if(fnt != NULL) {
                    char *fami = TTF_FontFaceFamilyName(fnt);
                    char *style = TTF_FontFaceStyleName(fnt);
                    bool isbitmap = (0 == strcasecmp(".fon", f.substr(f.length() - 4).c_str()) );
                    if(fami != NULL && (!isbitmap || i == 0) ) {
                        fout << fami << std::endl;
                        fout << f << std::endl;
                        fout << i << std::endl;
                    }
                    if(fami != NULL && style != NULL && 0 != strcasecmp(style, "Regular")) {
                        if(!isbitmap) {
                            fout << fami << " " << style << std::endl;
                            fout << f << std::endl;
                            fout << i << std::endl;
                        }
                    }
                    TTF_CloseFont(fnt);
                    fnt = NULL;
                }
            }
        }
        closedir (dir);
    }

}

static void save_font_list()
{
    std::ofstream fout("data/fontlist.txt", std::ios_base::trunc);

    font_folder_list(fout, "data");
    font_folder_list(fout, "data/font");

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

    fout << "end of list" << std::endl;

}

static std::string find_system_font(std::string name, int& faceIndex)
{
    struct stat stat_buf;
    int rc = stat("data/fontlist.txt", &stat_buf);
    if( rc == 0 ? stat_buf.st_size == 0 : true) {
      DebugLog() << "Generating fontlist\n";
      save_font_list();
    }


    std::ifstream fin("data/fontlist.txt");
    if (fin) {
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
        } while (true);
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
    int newWindowWidth = OPTIONS["TERMINAL_X"];
    newWindowWidth = newWindowWidth < FULL_SCREEN_WIDTH ? FULL_SCREEN_WIDTH : newWindowWidth;

    return newWindowWidth * fontwidth;
}

// Calculates the new height of the window, given the number of rows.
int projected_window_height(int)
{
    return OPTIONS["TERMINAL_Y"] * fontheight;
}

//Basic Init, create the font, backbuffer, etc
WINDOW *curses_init(void)
{
    lastchar = -1;
    inputdelay = -1;

    std::string typeface = "Terminus";
    std::string blending = "solid";
    std::ifstream fin;
    int faceIndex = 0;
    int fontsize = 0; //actuall size
    fin.open("data/FONTDATA");
    if (!fin.is_open()){
        fontwidth = 8;
        fontheight = 16;
        std::ofstream fout;//create data/FONDATA file
        fout.open("data/FONTDATA");
        if(fout.is_open()) {
            fout << typeface << "\n";
            fout << fontwidth << "\n";
            fout << fontheight;
            fout.close();
        }
    } else {
        getline(fin, typeface);
        fin >> fontwidth;
        fin >> fontheight;
        fin >> fontsize;
        fin >> blending;
        if ((fontwidth <= 4) || (fontheight <= 4)) {
            fontheight = 16;
            fontwidth = 8;
        }
        fin.close();
    }

    fontblending = (blending=="blended");

    halfwidth=fontwidth / 2;
    halfheight=fontheight / 2;

    if(!InitSDL()) {
        DebugLog() << (std::string)"Failed to initialize SDL!\n";
    }

    #ifdef SDLTILES
    DebugLog() << "Initializing SDL Tiles context\n";
    tilecontext = new cata_tiles;
    tilecontext->init("gfx");
    #endif // SDLTILES
    DebugLog() << (std::string)"Tiles initialized successfully.\n";
    WindowWidth= OPTIONS["TERMINAL_X"];
    if (WindowWidth < FULL_SCREEN_WIDTH) WindowWidth = FULL_SCREEN_WIDTH;
    WindowWidth *= fontwidth;
    WindowHeight = OPTIONS["TERMINAL_Y"] * fontheight;
    if(!WinCreate()) {
        DebugLog() << (std::string)"Failed to create game window!\n";
        return NULL;
    }
    #ifdef SDLTILES
    tilecontext->set_screen(screen);

    while(!strcasecmp(typeface.substr(typeface.length()-4).c_str(),".bmp") ||
          !strcasecmp(typeface.substr(typeface.length()-4).c_str(),".png")) {
        SDL_Surface *asciiload;
        typeface = "data/font/" + typeface;
        asciiload = IMG_Load(typeface.c_str());
        if(!asciiload || asciiload->w*asciiload->h < (fontwidth * fontheight * 256)) {
            SDL_FreeSurface(asciiload);
            break;
        }
        Uint32 key = SDL_MapRGB(asciiload->format, 0xFF, 0, 0xFF);
        SDL_SetColorKey(asciiload,SDL_TRUE,key);
		//SDL_RendererInfo info;
		//SDL_GetRendererInfo(renderer, &info);
		//for now, until we can figure out the above.
		//ascii[0] = SDL_ConvertSurfaceFormat(asciiload, SDL_PIXELFORMAT_RGBA8888, 0);
		ascii[0] = asciiload;
		//SDL_FreeSurface(asciiload);
        for(int a = 1; a < 16; a++) {
            ascii[a]=SDL_ConvertSurface(ascii[0],ascii[0]->format,ascii[0]->flags);
        }

        init_colors();
        for(int a = 0; a < 15; a++) {
            SDL_LockSurface(ascii[a]);
            int size = ascii[a]->h * ascii[a]->w;
            Uint32 *pixels = (Uint32 *)ascii[a]->pixels;
            Uint32 color = (windowsPalette[a].r << 16) | (windowsPalette[a].g << 8) | windowsPalette[a].b;
            for(int i=0;i<size;i++) {
                if(pixels[i] == 0xFFFFFF)
                    pixels[i] = color;
            }
            SDL_UnlockSurface(ascii[a]);
        }
        if(fontwidth)tilewidth=ascii[0]->w/fontwidth;
        OutputChar = &OutputImageChar;

        mainwin = newwin(OPTIONS["TERMINAL_Y"], OPTIONS["TERMINAL_X"],0,0);
        return mainwin;
    }
    #endif // SDLTILES

    std::string sysfnt = find_system_font(typeface, faceIndex);
    if(sysfnt != "") typeface = sysfnt;

    //make fontdata compatible with wincurse
    if(!fexists(typeface.c_str())) {
        faceIndex = 0;
        typeface = "data/font/" + typeface + ".ttf";
    }

    //different default font with wincurse
    if(!fexists(typeface.c_str())) {
        faceIndex = 0;
        typeface = "data/font/fixedsys.ttf";
    }

    if(fontsize <= 0) fontsize = fontheight - 1;

    // SDL_ttf handles bitmap fonts size incorrectly
    if(0 == strcasecmp(typeface.substr(typeface.length() - 4).c_str(), ".fon"))
        faceIndex = test_face_size(typeface, fontsize, faceIndex);

    font = TTF_OpenFontIndex(typeface.c_str(), fontsize, faceIndex);

    //if(!font) something went wrong

    TTF_SetFontStyle(font, TTF_STYLE_NORMAL);

    // glyph height hack by utunnels
    // SDL_ttf doesn't use FT_HAS_VERTICAL for function TTF_GlyphMetrics
    // this causes baseline problems for certain fonts
    // I can only guess by check a certain tall character...
    cache_glyphs();
    init_colors();

    OutputChar = &OutputFontChar;

    mainwin = newwin(OPTIONS["TERMINAL_Y"], OPTIONS["TERMINAL_X"],0,0);
    return mainwin;   //create the 'stdscr' window and return its ref
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
    TTF_CloseFont(font);
    font = NULL;
    for (int i=0; i<128; i++) {
        for (int j=0; j<16; j++) {
            if (glyph_cache[i][j]) {
                SDL_FreeSurface(glyph_cache[i][j]);
            }
            glyph_cache[i][j] = NULL;
        }
    }
    for(int a=0;a<16;a++)SDL_FreeSurface(ascii[a]);
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
    std::ifstream colorfile("data/raw/colors.json", std::ifstream::in | std::ifstream::binary);
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
        throw "data/raw/colors.json: " + e;
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
        }
    }

    return rval;
}

bool gamepad_available() {
    return joystick != NULL;
}

void rescale_tileset(int size) {
    #ifdef SDLTILES
        tilecontext->load_rescaled_tileset(size);
    #endif
}

bool input_context::get_coordinates(WINDOW* capture_win, int& x, int& y) {
    if(!coordinate_input_received) {
        return false;
    }

    if (!capture_win) {
        capture_win = g->w_terrain;
    }

    int view_columns = capture_win->width;
    int view_rows = capture_win->height;
    int selected_column, selected_row;

    // Translate mouse coords to map coords based on tile size
#ifdef SDLTILES
    if (use_tiles)
    {
        // Check if click is within bounds of the window we care about
        int win_left = capture_win->x * fontwidth;
        int win_right = win_left + (capture_win->width * tilecontext->tile_width);
        int win_top = capture_win->y * fontheight;
        int win_bottom = win_top + (capture_win->height * tilecontext->tile_height);
        if (coordinate_x < win_left || coordinate_x > win_right || coordinate_y < win_top || coordinate_y > win_bottom) {
            return false;
        }

        selected_column = (coordinate_x - win_left) / tilecontext->tile_width;
        selected_row = (coordinate_y - win_top) / tilecontext->tile_height;
    }
    else
#endif
    {
        // Check if click is within bounds of the window we care about
        int win_left = capture_win->x * fontwidth;
        int win_right = (capture_win->x + capture_win->width) * fontwidth;
        int win_top = capture_win->y * fontheight;
        int win_bottom = (capture_win->y + capture_win->height) * fontheight;
        if (coordinate_x < win_left || coordinate_x > win_right || coordinate_y < win_top || coordinate_y > win_bottom) {
            return false;
        }

        selected_column = (coordinate_x - win_left) / fontwidth;
        selected_row = (coordinate_y - win_top) / fontheight;
    }

    x = g->ter_view_x - ((view_columns/2) - selected_column);
    y = g->ter_view_y - ((view_rows/2) - selected_row);

    return true;
}

#endif // TILES
