#if (defined TILES)
#include "catacurse.h"
#include "options.h"
#include "output.h"
#include "color.h"
#include "debug.h"
#include "catacharset.h"
#include <fstream>
#include <sys/stat.h>

#ifdef _MSC_VER
#include "wdirent.h"
#include <direct.h>
#else
#include <dirent.h>
#endif

// SDL headers end up in different places depending on the OS, sadly
#if (defined _WIN32 || defined WINDOWS)
#include <windows.h>
#include "SDL.h"
#include "SDL_ttf.h"
#else
#include <wordexp.h>
#if (defined OSX_SDL_FW)
#include "SDL.h"
#include "SDL_ttf/SDL_ttf.h"
#else
#include "SDL/SDL.h"
#include "SDL/SDL_ttf.h"
#endif
#endif

//***********************************
//Globals                           *
//***********************************

static SDL_Color windowsPalette[256];
static SDL_Surface *screen = NULL;
static SDL_Surface *glyph_cache[128][16]; //cache ascii characters
TTF_Font* font;
static int ttf_height_hack = 0;
int WindowWidth;        //Width of the actual window, not the curses window
int WindowHeight;       //Height of the actual window, not the curses window
int lastchar;          //the last character that was pressed, resets in getch
int inputdelay;         //How long getch will wait for a character to be typed
//WINDOW *_windows;  //Probably need to change this to dynamic at some point
//int WindowCount;        //The number of curses windows currently in use
int fontwidth;          //the width of the font, background is always this size
int fontheight;         //the height of the font, background is always this size
int halfwidth;          //half of the font width, used for centering lines
int halfheight;          //half of the font height, used for centering lines

static bool fontblending = false;

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
  return ifile;
}

//Registers, creates, and shows the Window!!
bool WinCreate()
{
	int init_flags = SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER;

	if(SDL_Init(init_flags) < 0)
	{
		return false;
	}

	if(TTF_Init()<0)
	{
		return false;
	}

	SDL_EnableUNICODE(1);
	SDL_EnableKeyRepeat(500, 60);

	atexit(SDL_Quit);

	SDL_WM_SetCaption("Cataclysm: Dark Days Ahead - 0.6git", NULL);

    char center_string[] = "SDL_VIDEO_CENTERED=center"; // indirection needed to avoid a warning
    SDL_putenv(center_string);
	screen = SDL_SetVideoMode(WindowWidth, WindowHeight, 32, (SDL_SWSURFACE|SDL_DOUBLEBUF));
	//SDL_SetColors(screen,windowsPalette,0,256);

	if(screen==NULL) return false;

	ClearScreen();

    if(OPTIONS["HIDE_CURSOR"] != "Always" && SDL_ShowCursor(-1))
        SDL_ShowCursor(SDL_DISABLE);
    else
        SDL_ShowCursor(SDL_ENABLE);

    return true;
};

void WinDestroy()
{
	if(screen) SDL_FreeSurface(screen);
	screen = NULL;
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
    int top=999, bottom=-999;

    start_color();

    for(int ch=0; ch<128; ch++)
    {
        for(int color=0; color<16; color++)
        {
            SDL_Surface * glyph = glyph_cache[ch][color] = (fontblending?TTF_RenderGlyph_Blended:TTF_RenderGlyph_Solid)(font, ch, windowsPalette[color]);
            int minx, maxx, miny, maxy, advance;
            if(glyph!=NULL && color==0 && 0==TTF_GlyphMetrics(font, ch, &minx, &maxx, &miny, &maxy, &advance) )
            {
               int t = TTF_FontAscent(font)-maxy;
               int b = t + glyph->h;
               if(t<top) top = t;
               if(b>bottom) bottom = b;
            }
        }
    }

    int height = bottom - top;
    int delta = (fontheight-height)/2;

    ttf_height_hack =  delta - top;
}

static void OutputChar(Uint16 t, int x, int y, unsigned char color)
{
    color &= 0xf;

    SDL_Surface * glyph = t<0x80?glyph_cache[t][color]:(fontblending?TTF_RenderGlyph_Blended:TTF_RenderGlyph_Solid)(font, t, windowsPalette[color]);

    if(glyph)
    {
		int minx=0, maxy=0, dx=0, dy = 0;
		if( 0==TTF_GlyphMetrics(font, t, &minx, NULL, NULL, &maxy, NULL))
		{
			dx = minx;
			dy = TTF_FontAscent(font)-maxy+ttf_height_hack;
			SDL_Rect rect;
			rect.x = x+dx; rect.y = y+dy; rect.w = fontwidth; rect.h = fontheight;
			SDL_BlitSurface(glyph, NULL, screen, &rect);
		}
		if(t>=0x80) SDL_FreeSurface(glyph);
    }
}

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
        SDL_UpdateRect(screen, tx*fontwidth, ty*fontheight, tw*fontwidth, th*fontheight);
    }
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
}

static int end_alt_code()
{
    alt_down = false;
    return atoi(alt_buffer);
}

//Check for any window messages (keypress, paint, mousemove, etc)
void CheckMessages()
{
	SDL_Event ev;
    bool quit = false;
	while(SDL_PollEvent(&ev))
	{
		switch(ev.type)
		{
			case SDL_KEYDOWN:
			{
       int lc = 0;
       if(OPTIONS["HIDE_CURSOR"] != "Always" && SDL_ShowCursor(-1)) {
           SDL_ShowCursor(SDL_DISABLE); //hide mouse cursor on keyboard input
       }
				Uint8 *keystate = SDL_GetKeyState(NULL);
				// manually handle Alt+F4 for older SDL lib, no big deal
				if(ev.key.keysym.sym==SDLK_F4 && (keystate[SDLK_RALT] || keystate[SDLK_LALT]) )
				{
					quit = true;
					break;
				}
				else if(ev.key.keysym.sym==SDLK_RSHIFT || ev.key.keysym.sym==SDLK_LSHIFT ||
					ev.key.keysym.sym==SDLK_RCTRL || ev.key.keysym.sym==SDLK_LCTRL || ev.key.keysym.sym==SDLK_RALT )
				{
					break; // temporary fix for unwanted keys
				}
                else if(ev.key.keysym.sym==SDLK_LALT)
                {
                    begin_alt_code();
                    break;
                }
				else if (ev.key.keysym.unicode != 0) {
					lc = ev.key.keysym.unicode;
					switch (lc){
						case 13:            //Reroute ENTER key for compatilbity purposes
							lc=10;
							break;
						case 8:             //Reroute BACKSPACE key for compatilbity purposes
							lc=127;
							break;
					}
				}
				if(ev.key.keysym.sym==SDLK_LEFT) {
					lc = KEY_LEFT;
				}
				else if(ev.key.keysym.sym==SDLK_RIGHT) {
					lc = KEY_RIGHT;
				}
				else if(ev.key.keysym.sym==SDLK_UP) {
					lc = KEY_UP;
				}
				else if(ev.key.keysym.sym==SDLK_DOWN) {
					lc = KEY_DOWN;
				}
				else if(ev.key.keysym.sym==SDLK_PAGEUP) {
					lc = KEY_PPAGE;
				}
				else if(ev.key.keysym.sym==SDLK_PAGEDOWN) {
					lc = KEY_NPAGE;

				}
                if(!lc) break;
                if(alt_down) {
                    add_alt_code(lc);
                }else {
                    lastchar = lc;
                }
			}
			break;
            case SDL_KEYUP:
            {
                if(ev.key.keysym.sym==SDLK_LALT) {
                    int code = end_alt_code();
                    if(code) lastchar = code;
                }
            }
            break;
			case SDL_MOUSEMOTION:
                if((OPTIONS["HIDE_CURSOR"] == "Always" || OPTIONS["HIDE_CURSOR"] == "HiddenKB") &&
                    !SDL_ShowCursor(-1)) SDL_ShowCursor(SDL_ENABLE);
                break;
			case SDL_QUIT:
                quit = true;
				break;

		}
	}
    if(quit)
    {
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
            std::string f = path + "/" + ent->d_name;
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
            if(fnt)
            {
                nfaces = TTF_FontFaces(fnt);
                TTF_CloseFont(fnt);
            }
            for(long i = 0; i < nfaces; i++)
            {
                fnt = TTF_OpenFontIndex(f.c_str(), 12, i);
                if(fnt)
                {
                    char *fami = TTF_FontFaceFamilyName(fnt);
                    char *style = TTF_FontFaceStyleName(fnt);
                    bool isbitmap = (0 == strcasecmp(".fon", f.substr(f.length() - 4).c_str()) );
                    if(fami && (!isbitmap || i==0) )
                    {
                        fout << fami << std::endl;
                        fout << f << std::endl;
                        fout << i << std::endl;
                    }
                    if(fami && style && 0 != strcasecmp(style, "Regular"))
                    {
                        if(!isbitmap)
                        {
                            fout << fami << " " << style << std::endl;
                            fout << f << std::endl;
                            fout << i << std::endl;
                        }
                    }
                    TTF_CloseFont(fnt);
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
	if(!fexists("data/fontlist.txt")) save_font_list();

	std::ifstream fin("data/fontlist.txt");
	if(fin)
	{
		std::string fname;
		std::string fpath;
		std::string iline;
		int index = 0;
		do
		{
			getline(fin, fname);
			if(fname=="end of list") break;
			getline(fin, fpath);
			getline(fin, iline);
			index = atoi(iline.c_str());
            if(0==strcasecmp(fname.c_str(), name.c_str()))
			{
				faceIndex = index;
				return fpath;
			}
		}while(true);
	}

	return "";
}

// bitmap font font size test
// return face index that has this size or below
static int test_face_size(std::string f, int size, int faceIndex)
{
    TTF_Font* fnt = TTF_OpenFontIndex(f.c_str(), size, faceIndex);
    if(fnt)
    {
        char* style = TTF_FontFaceStyleName(fnt);
        if(style != NULL)
        {
            int faces = TTF_FontFaces(fnt);
            bool found = false;
            for(int i = faces - 1; i >= 0 && !found; i--)
            {
                TTF_Font* tf = TTF_OpenFontIndex(f.c_str(), size, i);
                char* ts = NULL;
                if(NULL != tf && NULL != (ts = TTF_FontFaceStyleName(tf)))
                {
                    if(0 == strcasecmp(ts, style) && TTF_FontHeight(tf) <= size)
                    {
                        faceIndex = i;
                        found = true;
                    }
                }
                TTF_CloseFont(tf);
            }
        }
        TTF_CloseFont(fnt);
    }

    return faceIndex;
}

//Basic Init, create the font, backbuffer, etc
WINDOW *curses_init(void)
{
    lastchar=-1;
    inputdelay=-1;

    std::string typeface = "";
    std::string blending = "solid";
    std::ifstream fin;
    int faceIndex = 0;
    int fontsize = 0; //actuall size
    fin.open("data/FONTDATA");
    if (!fin.is_open()){
        fontheight=16;
        fontwidth=8;
    } else {
        getline(fin, typeface);
        fin >> fontwidth;
        fin >> fontheight;
        fin >> fontsize;
        fin >> blending;
        if ((fontwidth <= 4) || (fontheight <= 4)) {
            fontheight=16;
            fontwidth=8;
        }
        fin.close();
    }

    fontblending = (blending=="blended");

    halfwidth=fontwidth / 2;
    halfheight=fontheight / 2;

    const int SidebarWidth = (OPTIONS["SIDEBAR_STYLE"] == "Narrow") ? 45 : 55;
    WindowWidth= (SidebarWidth + (OPTIONS["VIEWPORT_X"] * 2 + 1));
    if (WindowWidth < FULL_SCREEN_WIDTH) WindowWidth = FULL_SCREEN_WIDTH;
    WindowWidth *= fontwidth;
    WindowHeight= (OPTIONS["VIEWPORT_Y"] * 2 + 1) *fontheight;
    if(!WinCreate()) {}// do something here

    std::string sysfnt = find_system_font(typeface, faceIndex);
    if(sysfnt!="") typeface = sysfnt;

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

    // STL_ttf handles bitmap fonts size incorrectly
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

    mainwin = newwin((OPTIONS["VIEWPORT_Y"] * 2 + 1),(55 + (OPTIONS["VIEWPORT_Y"] * 2 + 1)),0,0);
    return mainwin;   //create the 'stdscr' window and return its ref
}

//Ported from windows and copied comments as well
//Not terribly sure how this function is suppose to work,
//but jday helped to figure most of it out
int curses_getch(WINDOW* win)
{
	// standards note: getch is sometimes required to call refresh
	// see, e.g., http://linux.die.net/man/3/getch
	// so although it's non-obvious, that refresh() call (and maybe InvalidateRect?) IS supposed to be there
	wrefresh(win);
	lastchar=ERR;//ERR=-1
    if (inputdelay < 0)
	{
        do
        {
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
        do
        {
            CheckMessages();
            endtime=SDL_GetTicks();
            if (lastchar!=ERR) break;
            SDL_Delay(1);
        }
        while (endtime<(starttime+inputdelay));
	}
	else
	{
		CheckMessages();
	}
    return lastchar;
}


//Ends the terminal, destroy everything
int curses_destroy(void)
{
	TTF_CloseFont(font);
	for(int i=0; i<128; i++)
	{
		for(int j=0; j<16; j++)
		{
			if(glyph_cache[i][j]) SDL_FreeSurface(glyph_cache[i][j]);
			glyph_cache[i][j] = NULL;
		}
	}
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

int curses_start_color(void)
{
	colorpairs=new pairs[100];
	windowsPalette[0]= BGR(0,0,0); // Black
	windowsPalette[1]= BGR(0, 0, 255); // Red
	windowsPalette[2]= BGR(0,110,0); // Green
	windowsPalette[3]= BGR(23,51,92); // Brown???
	windowsPalette[4]= BGR(200, 0, 0); // Blue
	windowsPalette[5]= BGR(98, 58, 139); // Purple
	windowsPalette[6]= BGR(180, 150, 0); // Cyan
	windowsPalette[7]= BGR(150, 150, 150);// Gray
	windowsPalette[8]= BGR(99, 99, 99);// Dark Gray
	windowsPalette[9]= BGR(150, 150, 255); // Light Red/Salmon?
	windowsPalette[10]= BGR(0, 255, 0); // Bright Green
	windowsPalette[11]= BGR(0, 255, 255); // Yellow
	windowsPalette[12]= BGR(255, 100, 100); // Light Blue
	windowsPalette[13]= BGR(240, 0, 255); // Pink
	windowsPalette[14]= BGR(255, 240, 0); // Light Cyan?
	windowsPalette[15]= BGR(255, 255, 255); //White
	//SDL_SetColors(screen,windowsPalette,0,256);
	return 0;
}

void curses_timeout(int t)
{
    inputdelay = t;
}

#endif // TILES
