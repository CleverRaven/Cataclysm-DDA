#if ((!defined TILES) && (!defined SDLTILES) && (defined _WIN32 || defined WINDOWS))
#include "catacurse.h"
#include "options.h"
#include "output.h"
#include "color.h"
#include "catacharset.h"
#include "get_version.h"
#include <cstdlib>
#include <fstream>
#include "init.h"

//***********************************
//Globals                           *
//***********************************

const char *szWindowClass = ("CataCurseWindow");    //Class name :D
HINSTANCE WindowINST;   //the instance of the window
HWND WindowHandle;      //the handle of the window
HDC WindowDC;           //Device Context of the window, used for backbuffer
int WindowWidth;        //Width of the actual window, not the curses window
int WindowHeight;       //Height of the actual window, not the curses window
int lastchar;          //the last character that was pressed, resets in getch
int inputdelay;         //How long getch will wait for a character to be typed
//WINDOW *_windows;  //Probably need to change this to dynamic at some point
//int WindowCount;        //The number of curses windows currently in use
HDC backbuffer;         //an off-screen DC to prevent flickering, lower cpu
HBITMAP backbit;        //the bitmap that is used in conjunction wth the above
int fontwidth;          //the width of the font, background is always this size
int fontheight;         //the height of the font, background is always this size
int halfwidth;          //half of the font width, used for centering lines
int halfheight;          //half of the font height, used for centering lines
HFONT font;             //Handle to the font created by CreateFont
RGBQUAD *windowsPalette;  //The coor palette, 16 colors emulates a terminal
unsigned char *dcbits;  //the bits of the screen image, for direct access
bool CursorVisible = true; // Showcursor is a somewhat weird function
std::map< std::string, std::vector<int> > consolecolors;

//***********************************
//Non-curses, Window functions      *
//***********************************

// declare this locally, because it's not generally cross-compatible in catacurse.h
LRESULT CALLBACK ProcessMessages(HWND__ *hWnd,u_int32_t Msg,WPARAM wParam, LPARAM lParam);

//Registers, creates, and shows the Window!!
bool WinCreate()
{
    WindowINST = GetModuleHandle(0); // Get current process handle
    std::string title = string_format("Cataclysm: Dark Days Ahead - %s", getVersionString());

    // Register window class
    WNDCLASSEXA WindowClassType   = WNDCLASSEXA();
    WindowClassType.cbSize        = sizeof(WNDCLASSEXA);
    WindowClassType.lpfnWndProc   = ProcessMessages;//the procedure that gets msgs
    WindowClassType.hInstance     = WindowINST;// hInstance
    WindowClassType.hIcon         = LoadIcon(WindowINST, MAKEINTRESOURCE(0)); // Get first resource
    WindowClassType.hIconSm       = LoadIcon(WindowINST, MAKEINTRESOURCE(0));
    WindowClassType.hCursor       = LoadCursor(NULL, IDC_ARROW);
    WindowClassType.lpszClassName = szWindowClass;
    if (!RegisterClassExA(&WindowClassType))
        return false;

    // Adjust window size
    uint32_t WndStyle = WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU | WS_VISIBLE; // Basic window, show on creation
    RECT WndRect;
    WndRect.left   = WndRect.top = 0;
    WndRect.right  = WindowWidth;
    WndRect.bottom = WindowHeight;
    AdjustWindowRect(&WndRect, WndStyle, false);

    // Center window
    RECT WorkArea;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &WorkArea, 0);
    int WindowX = WorkArea.right/2 - (WndRect.right - WndRect.left)/2;
    int WindowY = WorkArea.bottom/2 - (WndRect.bottom - WndRect.top)/2;

    // Magic
    WindowHandle = CreateWindowExA(0, szWindowClass , title.c_str(), WndStyle,
                                   WindowX, WindowY,
                                   WndRect.right - WndRect.left,
                                   WndRect.bottom - WndRect.top,
                                   0, 0, WindowINST, NULL);
    if (WindowHandle == 0)
        return false;

    return true;
};

//Unregisters, releases the DC if needed, and destroys the window.
void WinDestroy()
{
    if ((WindowDC != NULL) && (ReleaseDC(WindowHandle, WindowDC) == 0)){
        WindowDC = 0;
    }
    if ((!WindowHandle == 0) && (!(DestroyWindow(WindowHandle)))){
        WindowHandle = 0;
    }
    if (!(UnregisterClassA(szWindowClass, WindowINST))){
        WindowINST = 0;
    }
};

// Copied from sdlcurses.cpp
#define ALT_BUFFER_SIZE 8
static char alt_buffer[ALT_BUFFER_SIZE] = {};
static int alt_buffer_len = 0;
static bool alt_down = false;

static void begin_alt_code()
{
    alt_buffer[0] = '\0';
    alt_down = true;
    alt_buffer_len = 0;
}

void add_alt_code(char c)
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

//This function processes any Windows messages we get. Keyboard, OnClose, etc
LRESULT CALLBACK ProcessMessages(HWND__ *hWnd,unsigned int Msg,
                                 WPARAM wParam, LPARAM lParam)
{
    uint16_t MouseOver;
    switch (Msg)
    {
    case WM_DEADCHAR:
    case WM_CHAR:
        lastchar = (int)wParam;
        switch (lastchar){
            case VK_RETURN: //Reroute ENTER key for compatilbity purposes
                lastchar=10;
                break;
            case VK_BACK: //Reroute BACKSPACE key for compatilbity purposes
                lastchar=127;
                break;
        }
        return 0;

    case WM_KEYDOWN:                //Here we handle non-character input
        switch (wParam){
            case VK_LEFT:
                lastchar = KEY_LEFT;
                break;
            case VK_RIGHT:
                lastchar = KEY_RIGHT;
                break;
            case VK_UP:
                lastchar = KEY_UP;
                break;
            case VK_DOWN:
                lastchar = KEY_DOWN;
                break;
            case VK_NEXT:
                lastchar = KEY_NPAGE;
                break;
            case VK_PRIOR:
                lastchar = KEY_PPAGE;
                break;
            case VK_F1:
                lastchar = KEY_F(1);
                break;
            case VK_F2:
                lastchar = KEY_F(2);
                break;
            case VK_F3:
                lastchar = KEY_F(3);
                break;
            case VK_F4:
                lastchar = KEY_F(4);
                break;
            case VK_F5:
                lastchar = KEY_F(5);
                break;
            case VK_F6:
                lastchar = KEY_F(6);
                break;
            case VK_F7:
                lastchar = KEY_F(7);
                break;
            case VK_F8:
                lastchar = KEY_F(8);
                break;
            case VK_F9:
                lastchar = KEY_F(9);
                break;
            case VK_F10:
                lastchar = KEY_F(10);
                break;
            case VK_F11:
                lastchar = KEY_F(11);
                break;
            case VK_F12:
                lastchar = KEY_F(12);
                break;
            default:
                break;
        };
        return 0;

    case WM_KEYUP:
        if (!GetAsyncKeyState(VK_LMENU) && alt_down){ // LeftAlt hack
            if (int code = end_alt_code())
                lastchar = code;
        }
        return 0;

    case WM_SYSCHAR:
        add_alt_code((char)wParam);
        return 0;

    case WM_SYSKEYDOWN:
        if (GetAsyncKeyState(VK_LMENU) && !alt_down){ // LeftAlt hack
            begin_alt_code();
        }
        break;

    case WM_SETCURSOR:
        MouseOver = LOWORD(lParam);
        if (OPTIONS["HIDE_CURSOR"] == "hide")
        {
            if (MouseOver==HTCLIENT && CursorVisible)
            {
                CursorVisible = false;
                ShowCursor(false);
            }
            else if (MouseOver!=HTCLIENT && !CursorVisible)
            {
                CursorVisible = true;
                ShowCursor(true);
            }
        }
        else if (!CursorVisible)
        {
            CursorVisible = true;
            ShowCursor(true);
        }
        break;

    case WM_ERASEBKGND:
        return 1; // Don't erase background

    case WM_PAINT:
        BitBlt(WindowDC, 0, 0, WindowWidth, WindowHeight, backbuffer, 0, 0,SRCCOPY);
        ValidateRect(WindowHandle,NULL);
        return 0;

    case WM_DESTROY:
        exit(0); // A messy exit, but easy way to escape game loop
    };

    return DefWindowProcA(hWnd, Msg, wParam, lParam);
}

//The following 3 methods use mem functions for fast drawing
inline void VertLineDIB(int x, int y, int y2,int thickness, unsigned char color)
{
    int j;
    for (j=y; j<y2; j++)
        memset(&dcbits[x+j*WindowWidth],color,thickness);
}
inline void HorzLineDIB(int x, int y, int x2,int thickness, unsigned char color)
{
    int j;
    for (j=y; j<y+thickness; j++)
        memset(&dcbits[x+j*WindowWidth],color,x2-x);
}
inline void FillRectDIB(int x, int y, int width, int height, unsigned char color)
{
    int j;
    for (j=y; j<y+height; j++)
        //NOTE TO FUTURE: this breaks if j is negative. Apparently it doesn't break if j is too large, though?
        memset(&dcbits[x+j*WindowWidth],color,width);
}

void curses_drawwindow(WINDOW *win)
{
    int i,j,w,drawx,drawy;
    unsigned tmp;
    RECT update = {win->x * fontwidth, -1,
                   (win->x + win->width) * fontwidth, -1};

    for (j=0; j<win->height; j++){
        if (win->line[j].touched)
        {
            update.bottom = (win->y+j+1)*fontheight;
            if (update.top == -1)
            {
                update.top = update.bottom - fontheight;
            }

            win->line[j].touched=false;

            for (i=0,w=0; w<win->width; i++,w++){
                drawx=((win->x+w)*fontwidth);
                drawy=((win->y+j)*fontheight);//-j;
                if (((drawx+fontwidth)<=WindowWidth) && ((drawy+fontheight)<=WindowHeight)){
                const char* utf8str = win->line[j].chars+i;
                int len = ANY_LENGTH;
                tmp = UTF8_getch(&utf8str, &len);
                int FG = win->line[j].FG[w];
                int BG = win->line[j].BG[w];
                FillRectDIB(drawx,drawy,fontwidth,fontheight,BG);

                if (tmp != UNKNOWN_UNICODE) {

                    int color = RGB(windowsPalette[FG].rgbRed,windowsPalette[FG].rgbGreen,windowsPalette[FG].rgbBlue);
                    SetTextColor(backbuffer,color);

                    int cw = mk_wcwidth(tmp);
                    len = ANY_LENGTH-len;
                    if (cw > 1) {
                        FillRectDIB(drawx+fontwidth*(cw-1), drawy, fontwidth, fontheight, BG);
                        w += cw - 1;
                    }
                    if (len > 1) {
                        i += len - 1;
                    }
                    if (tmp) ExtTextOutW (backbuffer, drawx, drawy, 0, NULL, (WCHAR*)&tmp, 1, NULL);
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
    if (update.top != -1)
    {
        RedrawWindow(WindowHandle, &update, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
    }
}

//Check for any window messages (keypress, paint, mousemove, etc)
void CheckMessages()
{
    MSG msg;
    while (PeekMessage(&msg, 0 , 0, 0, PM_REMOVE)){
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

// Calculates the new width of the window, given the number of columns.
int projected_window_width(int column_count)
{
	return (55 + (OPTIONS["VIEWPORT_X"] * 2 + 1)) * fontwidth;
}

// Calculates the new height of the window, given the number of rows.
int projected_window_height(int row_count)
{
	return (OPTIONS["VIEWPORT_Y"] * 2 + 1) * fontheight;
}

//***********************************
//Psuedo-Curses Functions           *
//***********************************

//Basic Init, create the font, backbuffer, etc
WINDOW *curses_init(void)
{
   // _windows = new WINDOW[20];         //initialize all of our variables
    lastchar=-1;
    inputdelay=-1;

    std::string typeface = "Terminus";
    char * typeface_c = 0;
    std::ifstream fin;
    fin.open("data/FONTDATA");
    if (!fin.is_open()){
        typeface_c = (char*) "Terminus";
        fontwidth = 8;
        fontheight = 16;
        std::ofstream fout;//create data/FONDATA file
        fout.open("data\\FONTDATA");
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
        if ((fontwidth <= 4) || (fontheight <=4)){
            MessageBox(WindowHandle, "Invalid font size specified!", NULL, 0);
            fontheight = 16;
            fontwidth  = 8;
        }
    }
    typeface_c = new char [typeface.size()+1];
    strcpy (typeface_c, typeface.c_str());

    halfwidth=fontwidth / 2;
    halfheight=fontheight / 2;
    WindowWidth= (55 + (OPTIONS["VIEWPORT_X"] * 2 + 1)) * fontwidth;
    WindowHeight = (OPTIONS["VIEWPORT_Y"] * 2 + 1) *fontheight;

    WinCreate();    //Create the actual window, register it, etc
    timeBeginPeriod(1); // Set Sleep resolution to 1ms
    CheckMessages();    //Let the message queue handle setting up the window

    WindowDC   = GetDC(WindowHandle);
    backbuffer = CreateCompatibleDC(WindowDC);

    BITMAPINFO bmi = BITMAPINFO();
    bmi.bmiHeader.biSize         = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth        = WindowWidth;
    bmi.bmiHeader.biHeight       = -WindowHeight;
    bmi.bmiHeader.biPlanes       = 1;
    bmi.bmiHeader.biBitCount     = 8;
    bmi.bmiHeader.biCompression  = BI_RGB; // Raw RGB
    bmi.bmiHeader.biSizeImage    = WindowWidth * WindowHeight * 1;
    bmi.bmiHeader.biClrUsed      = 16; // Colors in the palette
    bmi.bmiHeader.biClrImportant = 16; // Colors in the palette
    backbit = CreateDIBSection(0, &bmi, DIB_RGB_COLORS, (void**)&dcbits, NULL, 0);
    DeleteObject(SelectObject(backbuffer, backbit));//load the buffer into DC

    // Load private fonts
    if (SetCurrentDirectory("data\\font")){
        WIN32_FIND_DATA findData;
        for (HANDLE findFont = FindFirstFile(".\\*", &findData); findFont != INVALID_HANDLE_VALUE; )
        {
            if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)){ // Skip folders
                AddFontResourceExA(findData.cFileName, FR_PRIVATE,NULL);
            }
            if (!FindNextFile(findFont, &findData)){
                FindClose(findFont);
                break;
            }
        }
        SetCurrentDirectory("..\\..");
    }

    // Use desired font, if possible
    font = CreateFont(fontheight, fontwidth, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                      ANSI_CHARSET, OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
                      PROOF_QUALITY, FF_MODERN, typeface_c);

    SetBkMode(backbuffer, TRANSPARENT);//Transparent font backgrounds
    SelectObject(backbuffer, font);//Load our font into the DC
//    WindowCount=0;

    delete typeface_c;
    mainwin = newwin((OPTIONS["VIEWPORT_Y"] * 2 + 1),(55 + (OPTIONS["VIEWPORT_Y"] * 2 + 1)),0,0);
    return mainwin;   //create the 'stdscr' window and return its ref
}

// A very accurate and responsive timer (NEVER use GetTickCount)
uint64_t GetPerfCount(){
    uint64_t Count;
    QueryPerformanceCounter((PLARGE_INTEGER)&Count);
    return Count;
}

//Not terribly sure how this function is suppose to work,
//but jday helped to figure most of it out
int curses_getch(WINDOW* win)
{
    // standards note: getch is sometimes required to call refresh
    // see, e.g., http://linux.die.net/man/3/getch
    // so although it's non-obvious, that refresh() call (and maybe InvalidateRect?) IS supposed to be there
    uint64_t Frequency;
    QueryPerformanceFrequency((PLARGE_INTEGER)&Frequency);
    wrefresh(win);
    InvalidateRect(WindowHandle,NULL,true);
    lastchar = ERR;
    if (inputdelay < 0)
    {
        for (; lastchar==ERR; Sleep(1))
            CheckMessages();
    }
    else if (inputdelay > 0)
    {
        for (uint64_t t0=GetPerfCount(), t1=0; t1 < (t0 + inputdelay*Frequency/1000); t1=GetPerfCount())
        {
            CheckMessages();
            if (lastchar!=ERR) break;
            Sleep(1);
        }
    }
    else
    {
        CheckMessages();
    };

    if (lastchar!=ERR && OPTIONS["HIDE_CURSOR"] == "hidekb" && CursorVisible) {
        CursorVisible = false;
        ShowCursor(false);
    }

    return lastchar;
}


//Ends the terminal, destroy everything
int curses_destroy(void)
{
    DeleteObject(font);
    WinDestroy();
    RemoveFontResourceExA("data\\termfont",FR_PRIVATE,NULL);//Unload it
    return 1;
}

inline RGBQUAD BGR(int b, int g, int r)
{
    RGBQUAD result;
    result.rgbBlue=b;    //Blue
    result.rgbGreen=g;    //Green
    result.rgbRed=r;    //Red
    result.rgbReserved=0;//The Alpha, isnt used, so just set it to 0
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
    windowsPalette = new RGBQUAD[16];

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

    if(consolecolors.empty())return SetDIBColorTable(backbuffer, 0, 16, windowsPalette);
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
    return SetDIBColorTable(backbuffer, 0, 16, windowsPalette);
}

void curses_timeout(int t)
{
    inputdelay = t;
}

#endif
