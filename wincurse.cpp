#if ((!defined TILES) && (defined _WIN32 || defined WINDOWS))
#include "catacurse.h"
#include "options.h"
#include "output.h"
#include "color.h"
#include "catacharset.h"
#include <cstdlib>
#include <fstream>

//***********************************
//Globals                           *
//***********************************

const WCHAR *szWindowClass = (L"CataCurseWindow");    //Class name :D
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
char szDirectory[MAX_PATH] = "";

bool CursorVisible = true; // Showcursor is a somewhat weird function

//***********************************
//Non-curses, Window functions      *
//***********************************

// declare this locally, because it's not generally cross-compatible in catacurse.h
LRESULT CALLBACK ProcessMessages(HWND__ *hWnd,u_int32_t Msg,WPARAM wParam, LPARAM lParam);

//Registers, creates, and shows the Window!!
bool WinCreate()
{
    WindowINST = GetModuleHandle(0); // Get current process handle
    const WCHAR *szTitle=  (L"Cataclysm: Dark Days Ahead - 0.6git");

    // Register window class
    WNDCLASSEXW WindowClassType = {0};
    WindowClassType.cbSize        = sizeof(WNDCLASSEXW);
    WindowClassType.lpfnWndProc   = ProcessMessages;//the procedure that gets msgs
    WindowClassType.hInstance     = WindowINST;// hInstance
    WindowClassType.hIcon         = LoadIcon(WindowINST, MAKEINTRESOURCE(0)); // Get first resource
    WindowClassType.hIconSm       = LoadIcon(WindowINST, MAKEINTRESOURCE(0));
    WindowClassType.hCursor       = LoadCursor(NULL, IDC_ARROW);
    WindowClassType.lpszMenuName  = NULL;
    WindowClassType.lpszClassName = szWindowClass;
    if (!RegisterClassExW(&WindowClassType))
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
    WindowHandle = CreateWindowExW(0, szWindowClass , szTitle, WndStyle,
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
    if ((WindowDC > 0) && (ReleaseDC(WindowHandle, WindowDC) == 0)){
        WindowDC = 0;
    }
    if ((!WindowHandle == 0) && (!(DestroyWindow(WindowHandle)))){
        WindowHandle = 0;
    }
    if (!(UnregisterClassW(szWindowClass, WindowINST))){
        WindowINST = 0;
    }
};

//This function processes any Windows messages we get. Keyboard, OnClose, etc
LRESULT CALLBACK ProcessMessages(HWND__ *hWnd,unsigned int Msg,
                                 WPARAM wParam, LPARAM lParam)
{
    uint16_t MouseOver;
    switch (Msg)
    {
    case WM_CHAR:
        lastchar=(int)wParam;
        switch (lastchar){
            case VK_RETURN: //Reroute ENTER key for compatilbity purposes
                lastchar=10;
                break;
            case VK_BACK: //Reroute BACKSPACE key for compatilbity purposes
                lastchar=127;
                break;
        };
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

    case WM_SETCURSOR:
        MouseOver = LOWORD(lParam);
        if (OPTIONS["HIDE_CURSOR"] == "Hidden")
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

    return DefWindowProcW(hWnd, Msg, wParam, lParam);
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

                if ( tmp != UNKNOWN_UNICODE){

                    int color = RGB(windowsPalette[FG].rgbRed,windowsPalette[FG].rgbGreen,windowsPalette[FG].rgbBlue);
                    SetTextColor(backbuffer,color);

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
                    if(tmp) ExtTextOutW (backbuffer,drawx,drawy,0,NULL,(WCHAR*)&tmp,1,NULL);
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

//***********************************
//Psuedo-Curses Functions           *
//***********************************

//Basic Init, create the font, backbuffer, etc
WINDOW *curses_init(void)
{
   // _windows = new WINDOW[20];         //initialize all of our variables
    lastchar=-1;
    inputdelay=-1;

    std::string typeface;
    char * typeface_c;
    std::ifstream fin;
    fin.open("data\\FONTDATA");
    if (!fin.is_open()){
        MessageBox(WindowHandle, "Failed to open FONTDATA, loading defaults.", NULL, 0);
        fontheight = 16;
        fontwidth  = 8;
    } else {
        getline(fin, typeface);
        typeface_c = new char [typeface.size()+1];
        strcpy (typeface_c, typeface.c_str());
        fin >> fontwidth;
        fin >> fontheight;
        if ((fontwidth <= 4) || (fontheight <=4)){
            MessageBox(WindowHandle, "Invalid font size specified!", NULL, 0);
            fontheight = 16;
            fontwidth  = 8;
        }
    }

    halfwidth=fontwidth / 2;
    halfheight=fontheight / 2;
    WindowWidth= (55 + (OPTIONS["VIEWPORT_X"] * 2 + 1)) * fontwidth;
    WindowHeight = (OPTIONS["VIEWPORT_Y"] * 2 + 1) *fontheight;

    WinCreate();    //Create the actual window, register it, etc
    timeBeginPeriod(1); // Set Sleep resolution to 1ms
    CheckMessages();    //Let the message queue handle setting up the window

    WindowDC   = GetDC(WindowHandle);
    backbuffer = CreateCompatibleDC(WindowDC);

    BITMAPINFO bmi = {0};
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

    int nResults = AddFontResourceExA("data\\termfont",FR_PRIVATE,NULL);
    if (nResults>0){
        font = CreateFont(fontheight, fontwidth, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                          ANSI_CHARSET, OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
                          PROOF_QUALITY, FF_MODERN, typeface_c);   //Create our font

    } else {
        MessageBox(WindowHandle, "Failed to load default font, using FixedSys.", NULL, 0);
        font = CreateFont(fontheight, fontwidth, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                      ANSI_CHARSET, OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
                      PROOF_QUALITY, FF_MODERN, "FixedSys");   //Create our font
    }
    //FixedSys will be user-changable at some point in time??
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

    if (lastchar!=ERR && OPTIONS["HIDE_CURSOR"] == "HiddenKB" && CursorVisible) {
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

int curses_start_color(void)
{
    colorpairs = new pairs[100];
    windowsPalette = new RGBQUAD[16];
    windowsPalette[0]  = BGR(0,0,0);         // Black
    windowsPalette[1]  = BGR(0, 0, 255);     // Red
    windowsPalette[2]  = BGR(0,110,0);       // Green
    windowsPalette[3]  = BGR(23,51,92);      // Brown???
    windowsPalette[4]  = BGR(200, 0, 0);     // Blue
    windowsPalette[5]  = BGR(98, 58, 139);   // Purple
    windowsPalette[6]  = BGR(180, 150, 0);   // Cyan
    windowsPalette[7]  = BGR(150, 150, 150); // Gray
    windowsPalette[8]  = BGR(99, 99, 99);    // Dark Gray
    windowsPalette[9]  = BGR(150, 150, 255); // Light Red/Salmon?
    windowsPalette[10] = BGR(0, 255, 0);     // Bright Green
    windowsPalette[11] = BGR(0, 255, 255);   // Yellow
    windowsPalette[12] = BGR(255, 100, 100); // Light Blue
    windowsPalette[13] = BGR(240, 0, 255);   // Pink
    windowsPalette[14] = BGR(255, 240, 0);   // Light Cyan?
    windowsPalette[15] = BGR(255, 255, 255); //White
    return SetDIBColorTable(backbuffer, 0, 16, windowsPalette);
}

void curses_timeout(int t)
{
    inputdelay = t;
}

#endif
