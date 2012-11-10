#if (defined _WIN32 || defined WINDOWS)
#include "catacurse.h"
#include <cstdlib>
#include <fstream>

#include <iostream>

#include "debug.h"

//***********************************
//Globals                           *
//***********************************

WINDOW *mainwin;
int lastchar;          //the last character that was pressed, resets in getch
int inputdelay;         //How long getch will wait for a character to be typed
pairs *colorpairs;   //storage for pair'ed colored, should be dynamic, meh
int colorMapping[16];
unsigned char *dcbits;  //the bits of the screen image, for direct access

HANDLE consoleWin;
HANDLE keyboardInput;
HANDLE backBuffer;
int frameCounter;

char szDirectory[MAX_PATH] = "";

//***********************************
//Non-curses, Window functions      *
//***********************************

bool WinCreate()
{
	const WCHAR *szTitle=  (L"Cataclysm");

	consoleWin = GetStdHandle(STD_OUTPUT_HANDLE);
	keyboardInput = GetStdHandle(STD_INPUT_HANDLE);

	SetConsoleTitleW(szTitle);

	CONSOLE_SCREEN_BUFFER_INFO bInfo;
	GetConsoleScreenBufferInfo(consoleWin, &bInfo);
	if (bInfo.srWindow.Bottom < 24 || bInfo.srWindow.Right < 79) {
		COORD bufferSize = {80, 40};
		SetConsoleScreenBufferSize(consoleWin, bufferSize);

		SMALL_RECT windowSize = {0, 0, bufferSize.X-1, bufferSize.Y-1};
		SetConsoleWindowInfo(consoleWin, TRUE, &windowSize);
	}
	GetConsoleScreenBufferInfo(consoleWin, &bInfo);
	COORD winSize = { bInfo.srWindow.Right - bInfo.srWindow.Left + 1, bInfo.srWindow.Bottom - bInfo.srWindow.Top + 1 };

	// if buffer is bigger then the actual console,
	// scrollbars appear, and apparently they cause ugly
	// flickering, so let's crop buffer size
	SetConsoleScreenBufferSize(consoleWin, winSize);

	mainwin = newwin(winSize.Y,winSize.X,0,0);

	DWORD consoleMode;
	GetConsoleMode(keyboardInput, &consoleMode);
	consoleMode &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT);
	consoleMode |= (ENABLE_MOUSE_INPUT | ENABLE_EXTENDED_FLAGS);
	SetConsoleMode(keyboardInput, consoleMode);
	GetConsoleMode(consoleWin, &consoleMode);
	consoleMode &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT);
	consoleMode |= (ENABLE_MOUSE_INPUT | ENABLE_EXTENDED_FLAGS);
	SetConsoleMode(consoleWin, consoleMode);

	CONSOLE_CURSOR_INFO consoleInfo = { 0 };
	GetConsoleCursorInfo(consoleWin, &consoleInfo);
	consoleInfo.bVisible = 0;
	SetConsoleCursorInfo(consoleWin, &consoleInfo);

	backBuffer = CreateConsoleScreenBuffer(GENERIC_READ|GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, 0);
	GetConsoleCursorInfo(backBuffer, &consoleInfo);
	consoleInfo.bVisible = 0;
	SetConsoleCursorInfo(backBuffer, &consoleInfo);


	DebugLog() << "===== ===== ===== ===== ===== =====\n";
	DebugLog() << "  1) buffer size: " << bInfo.dwSize.X << "," << bInfo.dwSize.Y << "\n";
	DebugLog() << "  1) window size: " << bInfo.srWindow.Right << "," << bInfo.srWindow.Bottom << "\n";

	return true;
};

void WinDestroy()
{
};
/*
//This function processes any Windows messages we get. Keyboard, OnClose, etc
LRESULT CALLBACK ProcessMessages(HWND__ *hWnd,unsigned int Msg,
                                 WPARAM wParam, LPARAM lParam)
{
    switch (Msg){
        case WM_CHAR:               //This handles most key presses
            lastchar=(int)wParam;
            switch (lastchar){
                case 13:            //Reroute ENTER key for compatilbity purposes
                    lastchar=10;
                    break;
                case 8:             //Reroute BACKSPACE key for compatilbity purposes
                    lastchar=127;
                    break;
            };
            break;
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
                default:
                    break;
            };
        case WM_ERASEBKGND:
            return 1;               //We don't want to erase our background
        case WM_PAINT:              //Pull from our backbuffer, onto the screen
            BitBlt(WindowDC, 0, 0, WindowWidth, WindowHeight, backbuffer, 0, 0,SRCCOPY);
            ValidateRect(WindowHandle,NULL);
            break;
        case WM_DESTROY:
            exit(0);//A messy exit, but easy way to escape game loop
        default://If we didnt process a message, return the default value for it
            return DefWindowProcW(hWnd, Msg, wParam, lParam);
    };
    return 0;
};
*/

void copyConsole(HANDLE conOut, HANDLE conIn) {
	SMALL_RECT rwRect;
	CHAR_INFO buf[80*25];
	COORD coordBufSize;
	COORD coordBufCoord = { 0, 0 };

	rwRect.Top = 0;
	rwRect.Left = 0;
	rwRect.Bottom = 24;
	rwRect.Right = 79;

	coordBufSize.Y = 25;
	coordBufSize.X = 80;

	SetConsoleActiveScreenBuffer(conIn);
	ReadConsoleOutput(conIn, buf, coordBufSize, coordBufCoord, &rwRect);
	WriteConsoleOutput(conOut, buf, coordBufSize, coordBufCoord, &rwRect);
	SetConsoleActiveScreenBuffer(conOut);
}

void debugPrint(size_t yPos, const char* buf)
{
	COORD statusLine = { 0, yPos };
	SetConsoleCursorPosition(consoleWin, statusLine);
	SetConsoleTextAttribute(consoleWin, 3);
	DebugLog() << "after draw window, delay: " << inputdelay << " cnt: " << frameCounter << " " << buf << "\n";
}

// variable used to actually check if we need to redraw
bool didRedraw;

// all the drawing will (except 'terrain' which is kinda special*)
// will be done to backBuffer, that backBuffer, will be than 'synched'
// to main console window just at the beginning of "getch" function
//
// we need to draw terain (also) directly onto the console to have that cool
// bullet/explosions "animations" :)

unsigned long DrawWindow(WINDOW *win)
{
	int i,j,drawx,drawy;
	char tmp;
	unsigned long startTime=GetTickCount();	

	CHAR_INFO* screenBuffer = reinterpret_cast<CHAR_INFO*>( win->custom );
	bool isTerrainWindow = (25==win->width && 25==win->height);
	bool haveAnimation = false;
	for (j=0; j<win->height; j++){
		if (win->line[j].touched) {
			for (i=0; i<win->width; i++){
				int pos = j*win->width + i;

				win->line[j].touched=false;

				if (1) { //((drawx+fontwidth)<=WindowWidth) && ((drawy+fontheight)<=WindowHeight))...
					tmp = win->line[j].chars[i];
					int FG = colorMapping[ win->line[j].FG[i] ];
					int BG = colorMapping[ win->line[j].BG[i] ];

					screenBuffer[pos].Attributes = BG*16 + FG;
					screenBuffer[pos].Char.UnicodeChar = ' ';

					WCHAR wchr = 0;
					if ( tmp > 0){
						//if (tmp==95){//If your font doesnt draw underscores..uncomment
						//        HorzLineDIB(drawx,drawy+fontheight-2,drawx+fontwidth,1,FG);
						//    } else { // all the wa to here
						wchr = tmp;

						if (isTerrainWindow && 4==FG && !haveAnimation &&
							('*' == wchr || '#' == wchr || '|' == wchr ||
							 '-' == wchr ||	'/' == wchr || '\\' == wchr)) {
							haveAnimation = true;
						}
						//    }     //and this line too.
					} else if (  tmp < 0 ) {
						switch (tmp) {
							case -60://box bottom/top side (horizontal line)
								wchr = 0x2500;
								break;
							case -77://box left/right side (vertical line)
								wchr = 0x2502;
								break;
							case -38://box top left
								wchr = 0x250c;
								break;
							case -65://box top right
								wchr = 0x2510;
								break;
							case -39://box bottom right
								wchr = 0x2518;
								break;
							case -64://box bottom left
								wchr = 0x2514;
								break;
							case -63://box bottom north T (left, right, up)
								wchr = 0x2534;
								break;
							case -61://box bottom east T (up, right, down)
								wchr = 0x251c;
								break;
							case -62://box bottom south T (left, right, down)
								wchr = 0x252c;
								break;
							case -59://box X (left down up right)
								wchr = 0x2534;
								break;
							case -76://box bottom east T (left, down, up)
								wchr = 0x2524;
								break;
							default:
								break;
						}
					};//switch (tmp)
					if (wchr != 0) {
						screenBuffer[pos].Char.UnicodeChar = tmp;
					}
				}//(tmp < 0)
			};//for (i=0;i<_windows[w].width;i++)
		}
	};// for (j=0;j<_windows[w].height;j++)
	win->draw=false;                //We drew the window, mark it as so


	SMALL_RECT rwRect;
	COORD coordBufSize;
	COORD coordBufCoord = { 0, 0 };

	rwRect.Top = win->y;
	rwRect.Left = win->x;
	rwRect.Bottom = win->y+win->height - 1;
	rwRect.Right = win->x+win->width - 1;

	coordBufSize.Y = win->height;
	coordBufSize.X = win->width;

	// draw everything to backBuffer
	WriteConsoleOutput(backBuffer, screenBuffer, coordBufSize, coordBufCoord, &rwRect);

	// draw terrain both to backBuffer AND directly to console...
	// now besides that, because everything else, goes to backBuffer,
	// we need to mark 'didRedraw', to actually redraw console later...
	if (haveAnimation) {
		WriteConsoleOutput(consoleWin, screenBuffer, coordBufSize, coordBufCoord, &rwRect);
	} else {
		didRedraw = false;
	}
	unsigned long t1 = GetTickCount() - startTime;

	frameCounter++;
	return t1;
};

int translateConsoleInput(KEY_EVENT_RECORD key)
{
	DebugLog() << " : got event ";
	if (! key.bKeyDown) {
		return 1;
	}
	
	int code = key.wVirtualKeyCode;
	DebugLog() << " key up " << code ;
	int processed = 0;
	switch (code) {
		case VK_BACK:   lastchar = KEY_BACKSPACE; processed = 1; break;
		case VK_RETURN: lastchar = 10;            processed = 1; break;
		case VK_LEFT:   lastchar = KEY_LEFT;      processed = 1; break;
		case VK_RIGHT:  lastchar = KEY_RIGHT;     processed = 1; break;
		case VK_UP:     lastchar = KEY_UP;        processed = 1; break;
		case VK_DOWN:   lastchar = KEY_DOWN;      processed = 1; break;
		case VK_ESCAPE: lastchar = 27;            processed = 1; break;
		case VK_SPACE:  lastchar = ' ';           processed = 1; break;
		// ignore ... ;p
		case VK_SHIFT:                            processed = 1; break;
		default:
				break;
	}
	if (processed) {
		DebugLog() << " proc " << lastchar;
		return 0;
	}

	int mask = (ENHANCED_KEY|LEFT_ALT_PRESSED|LEFT_CTRL_PRESSED|RIGHT_ALT_PRESSED|RIGHT_CTRL_PRESSED);
	if (0 == (key.dwControlKeyState & mask)) {
		lastchar = key.uChar.AsciiChar;
	}
	DebugLog() << "translate" << lastchar;

	return 0;
}

//Check for any window messages (keypress, paint, mousemove, etc)
void CheckMessages()
{
	INPUT_RECORD inputData[1];
	DWORD elementsRead = 0;
	DWORD count1 = 0;
	DWORD count2 = 0;

	if (! didRedraw) {
		LockWindowUpdate(GetConsoleWindow());
			copyConsole(consoleWin, backBuffer);
		LockWindowUpdate(0);
		didRedraw = true;
	}

	GetNumberOfConsoleInputEvents(keyboardInput, &count1);
	ReadConsoleInput(keyboardInput, inputData, 1, &elementsRead);

	GetNumberOfConsoleInputEvents(keyboardInput, &count2);
	DebugLog() << " checkmsg()[" << elementsRead << "/" << count1 << "," << count2 <<"]";
	if (! elementsRead) {
		DebugLog() << "\n";
		return;
	}
	switch (inputData[0].EventType) {
		case MOUSE_EVENT:
			DebugLog() << " : hated mouse";
			break;
		case KEY_EVENT:
			translateConsoleInput(inputData[0].Event.KeyEvent);
			break;
		case MENU_EVENT:
			DebugLog() << " : menu";
			break;
		case WINDOW_BUFFER_SIZE_EVENT:
			DebugLog() << " : buffer size changed? what the heck?";
			break;
		default:
			break;
	}
	DebugLog() << "\n";

};

//***********************************
//Psuedo-Curses Functions           *
//***********************************

//Basic Init, create the font, backbuffer, etc
WINDOW *initscr(void)
{
	// _windows = new WINDOW[20];         //initialize all of our variables
	lastchar=-1;
	inputdelay=-1;
	std::string typeface;
	char * typeface_c;
	std::ifstream fin;
	fin.open("data\\FONTDATA");
	if (!fin.is_open()){
		std::cout << "Failed to open FONTDATA, loading defaults." << std::endl;
	} else {
		getline(fin, typeface);
		typeface_c= new char [typeface.size()+1];
		strcpy (typeface_c, typeface.c_str());
	}

	WinCreate();    //Create the actual window, register it, etc

	delete typeface_c;
	return mainwin;   //create the 'stdscr' window and return its ref
};

WINDOW *newwin(int nlines, int ncols, int begin_y, int begin_x)
{
	int i,j;
	
	DebugLog() << "newwin()" << begin_y << "," << begin_x << "  " << nlines << "," << ncols << "\n";

	WINDOW *newwindow = new WINDOW;
	newwindow->x=begin_x;
	newwindow->y=begin_y;
	newwindow->width=ncols;
	newwindow->height=nlines;
	newwindow->inuse=true;
	newwindow->draw=false;
	newwindow->BG=0;
	newwindow->FG=8;
	newwindow->cursorx=0;
	newwindow->cursory=0;
	newwindow->line = new curseline[nlines];
	newwindow->custom = new CHAR_INFO[ncols * nlines];

	for (j=0; j<nlines; j++)
	{
		newwindow->line[j].chars= new char[ncols];
		newwindow->line[j].FG= new char[ncols];
		newwindow->line[j].BG= new char[ncols];
		newwindow->line[j].touched=true;//Touch them all !?
		for (i=0; i<ncols; i++)
		{
			newwindow->line[j].chars[i]=0;
			newwindow->line[j].FG[i]=0;
			newwindow->line[j].BG[i]=0;
		}
	}

	return newwindow;
};


//Deletes the window and marks it as free. Clears it just in case.
int delwin(WINDOW *win)
{
	int j;
	win->inuse=false;
	win->draw=false;
	for (j=0; j<win->height; j++){
		delete win->line[j].chars;
		delete win->line[j].FG;
		delete win->line[j].BG;
	}

	CHAR_INFO* screenBuffer = reinterpret_cast<CHAR_INFO*>( win->custom );
	delete[] screenBuffer;
	win->custom = 0;

	delete[] win->line;
	delete win;
	return 1;
};

inline int newline(WINDOW *win){
	if (win->cursory < win->height - 1){
		win->cursory++;
		win->cursorx=0;
		return 1;
	}
	return 0;
};

inline void addedchar(WINDOW *win){
	win->cursorx++;
	win->line[win->cursory].touched=true;
	if (win->cursorx > win->width)
		newline(win);
};


//Borders the window with fancy lines!
int wborder(WINDOW *win, chtype ls, chtype rs, chtype ts, chtype bs, chtype tl, chtype tr, chtype bl, chtype br)
{

	int i, j;
	int oldx=win->cursorx;//methods below move the cursor, save the value!
	int oldy=win->cursory;//methods below move the cursor, save the value!
	if (ls>0)
		for (j=1; j<win->height-1; j++)
			mvwaddch(win, j, 0, 179);
	if (rs>0)
		for (j=1; j<win->height-1; j++)
			mvwaddch(win, j, win->width-1, 179);
	if (ts>0)
		for (i=1; i<win->width-1; i++)
			mvwaddch(win, 0, i, 196);
	if (bs>0)
		for (i=1; i<win->width-1; i++)
			mvwaddch(win, win->height-1, i, 196);
	if (tl>0)
		mvwaddch(win,0, 0, 218);
	if (tr>0)
		mvwaddch(win,0, win->width-1, 191);
	if (bl>0)
		mvwaddch(win,win->height-1, 0, 192);
	if (br>0)
		mvwaddch(win,win->height-1, win->width-1, 217);
	//_windows[w].cursorx=oldx;//methods above move the cursor, put it back
	//_windows[w].cursory=oldy;//methods above move the cursor, put it back
	wmove(win,oldy,oldx);
	return 1;
};


//Refreshes a window, causing it to redraw on top.
int wrefresh(WINDOW *win)
{
	if (win==0) win=mainwin;
	if (win->draw) {
		unsigned long timeTaken = DrawWindow(win);
		const char* temp = (win == mainwin? "\n ----- mainwin -----\n" : "\n");
		DebugLog() << "wrefresh()" << win->y << "," << win->x << "  " << win->height << "," << win->width << "  time: " << timeTaken << temp;
	}
	return 1;
};

//Refreshes window 0 (stdscr), causing it to redraw on top.
int refresh(void)
{
	int t = wrefresh(mainwin);
	return t;
};

//Not terribly sure how this function is suppose to work,
//but jday helped to figure most of it out
int getch(void)
{
	refresh();
	//InvalidateRect(WindowHandle,NULL,true);
	lastchar=ERR;//ERR=-1

	DebugLog() << __FUNCTION__ << " " << inputdelay;
	if (inputdelay < 0)
	{
		do
		{
			CheckMessages();
			if (lastchar!=ERR) break;
			MsgWaitForMultipleObjects(0, NULL, FALSE, 50, QS_ALLEVENTS);//low cpu wait!
		}
		while (lastchar==ERR);
	}
	else if (inputdelay > 0)
	{
		unsigned long starttime=GetTickCount();
		unsigned long endtime;
		do
		{
			CheckMessages();        //MsgWaitForMultipleObjects won't work very good here
			endtime=GetTickCount(); //it responds to mouse movement, and WM_PAINT, not good
			if (lastchar!=ERR) break;
			Sleep(2);
		}
		while (endtime<(starttime+inputdelay));
	}
	else
	{
		CheckMessages();
	};
	Sleep(25);
	return lastchar;
};

//The core printing function, prints characters to the array, and sets colors
inline int printstring(WINDOW *win, char *fmt)
{
	int size = strlen(fmt);
	int j;
	for (j=0; j<size; j++){
		if (!(fmt[j]==10)){//check that this isnt a newline char
			if (win->cursorx <= win->width - 1 && win->cursory <= win->height - 1) {
				win->line[win->cursory].chars[win->cursorx]=fmt[j];
				win->line[win->cursory].FG[win->cursorx]=win->FG;
				win->line[win->cursory].BG[win->cursorx]=win->BG;
				win->line[win->cursory].touched=true;
				addedchar(win);
			} else
				return 0; //if we try and write anything outside the window, abort completely
		} else // if the character is a newline, make sure to move down a line
			if (newline(win)==0){
				return 0;
			}
	}
	win->draw=true;
	return 1;
}

//Prints a formatted string to a window at the current cursor, base function
int wprintw(WINDOW *win, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	char printbuf[2048];
	vsnprintf(printbuf, 2047, fmt, args);
	va_end(args);
	return printstring(win,printbuf);
};

//Prints a formatted string to a window, moves the cursor
int mvwprintw(WINDOW *win, int y, int x, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	char printbuf[2048];
	vsnprintf(printbuf, 2047, fmt, args);
	va_end(args);
	if (wmove(win,y,x)==0) return 0;
	return printstring(win,printbuf);
};

//Prints a formatted string to window 0 (stdscr), moves the cursor
int mvprintw(int y, int x, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	char printbuf[2048];
	vsnprintf(printbuf, 2047, fmt, args);
	va_end(args);
	if (move(y,x)==0) return 0;
	return printstring(mainwin,printbuf);
};

//Prints a formatted string to window 0 (stdscr) at the current cursor
int printw(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	char printbuf[2078];
	vsnprintf(printbuf, 2047, fmt, args);
	va_end(args);
	return printstring(mainwin,printbuf);
};

//erases a window of all text and attributes
int werase(WINDOW *win)
{
	int j,i;
	for (j=0; j<win->height; j++)
	{
		for (i=0; i<win->width; i++)   {
			win->line[j].chars[i]=0;
			win->line[j].FG[i]=0;
			win->line[j].BG[i]=0;
		}
		win->line[j].touched=true;
	}
	win->draw=true;
	wmove(win,0,0);
	wrefresh(win);
	return 1;
};

//erases window 0 (stdscr) of all text and attributes
int erase(void)
{
	return werase(mainwin);
};

//pairs up a foreground and background color and puts it into the array of pairs
int init_pair(short pair, short f, short b)
{
	colorpairs[pair].FG=f;
	colorpairs[pair].BG=b;
	return 1;
};

//moves the cursor in a window
int wmove(WINDOW *win, int y, int x)
{
	if (x>=win->width)
	{return 0;}//FIXES MAP CRASH -> >= vs > only
	if (y>=win->height)
	{return 0;}// > crashes?
	if (y<0)
	{return 0;}
	if (x<0)
	{return 0;}
	win->cursorx=x;
	win->cursory=y;
	return 1;
};

//Clears windows 0 (stdscr)     I'm not sure if its suppose to do this?
int clear(void)
{
	return wclear(mainwin);
};

//Ends the terminal, destroy everything
int endwin(void)
{
	//DeleteObject(font);
	WinDestroy();
	//RemoveFontResourceExA("data\\termfont",FR_PRIVATE,NULL);//Unload it
	return 1;
};

//adds a character to the window
int mvwaddch(WINDOW *win, int y, int x, const chtype ch)
{
	if (wmove(win,y,x)==0) return 0;
	return waddch(win, ch);
};

//clears a window
int wclear(WINDOW *win)
{
	werase(win);
	wrefresh(win);
	return 1;
};

//gets the max x of a window (the width)
int getmaxx(WINDOW *win)
{
	if (win==0) return mainwin->width;     //StdScr
	return win->width;
};

//gets the max y of a window (the height)
int getmaxy(WINDOW *win)
{
	if (win==0) return mainwin->height;     //StdScr
	return win->height;
};

inline RGBQUAD BGR(int b, int g, int r)
{
	RGBQUAD result;
	result.rgbBlue=b;    //Blue
	result.rgbGreen=g;    //Green
	result.rgbRed=r;    //Red
	result.rgbReserved=0;//The Alpha, isnt used, so just set it to 0
	return result;
};

int start_color(void)
{
	colorpairs=new pairs[50];
	colorMapping[0] = 0;
	colorMapping[1] = 4;
	colorMapping[2] = 2;
	colorMapping[3] = 6;
	colorMapping[4] = 1;
	colorMapping[5] = 5;
	colorMapping[6] = 3;
	colorMapping[7] = 7;

	colorMapping[8]  = 8+0;
	colorMapping[9]  = 8+4;
	colorMapping[10] = 8+2;
	colorMapping[11] = 8+6;
	colorMapping[12] = 8+1;
	colorMapping[13] = 8+5;
	colorMapping[14] = 8+3;
	colorMapping[15] = 8+7;
	return 1;
};

int keypad(WINDOW *faux, bool bf)
{
	return 1;
};

int noecho(void)
{
	return 1;
};
int cbreak(void)
{
	return 1;
};
int keypad(int faux, bool bf)
{
	return 1;
};
int curs_set(int visibility)
{
	return 1;
};

int mvaddch(int y, int x, const chtype ch)
{
	return mvwaddch(mainwin,y,x,ch);
};

int wattron(WINDOW *win, int attrs)
{
	bool isBold = !!(attrs & A_BOLD);
	bool isBlink = !!(attrs & A_BLINK);
	int pairNumber = (attrs & A_COLOR) >> 17;
	win->FG=colorpairs[pairNumber].FG;
	win->BG=colorpairs[pairNumber].BG;
	if (isBold) win->FG += 8;
	if (isBlink) win->BG += 8;
	return 1;
};
int wattroff(WINDOW *win, int attrs)
{
	win->FG=8;                                  //reset to white
	win->BG=0;                                  //reset to black
	return 1;
};
int attron(int attrs)
{
	return wattron(mainwin, attrs);
};
int attroff(int attrs)
{
	return wattroff(mainwin,attrs);
};
int waddch(WINDOW *win, const chtype ch)
{
	char charcode;
	charcode=ch;

	switch (ch){        //LINE_NESW  - X for on, O for off
		case 4194424:   //#define LINE_XOXO 4194424
			charcode=179;
			break;
		case 4194417:   //#define LINE_OXOX 4194417
			charcode=196;
			break;
		case 4194413:   //#define LINE_XXOO 4194413
			charcode=192;
			break;
		case 4194412:   //#define LINE_OXXO 4194412
			charcode=218;
			break;
		case 4194411:   //#define LINE_OOXX 4194411
			charcode=191;
			break;
		case 4194410:   //#define LINE_XOOX 4194410
			charcode=217;
			break;
		case 4194422:   //#define LINE_XXOX 4194422
			charcode=193;
			break;
		case 4194420:   //#define LINE_XXXO 4194420
			charcode=195;
			break;
		case 4194421:   //#define LINE_XOXX 4194421
			charcode=180;
			break;
		case 4194423:   //#define LINE_OXXX 4194423
			charcode=194;
			break;
		case 4194414:   //#define LINE_XXXX 4194414
			charcode=197;
			break;
		default:
			charcode = (char)ch;
			break;
	}


	int curx=win->cursorx;
	int cury=win->cursory;

	//if (win2 > -1){
	win->line[cury].chars[curx]=charcode;
	win->line[cury].FG[curx]=win->FG;
	win->line[cury].BG[curx]=win->BG;


	win->draw=true;
	addedchar(win);
	return 1;
	//  else{
	//  win2=win2+1;

};




//Move the cursor of windows 0 (stdscr)
int move(int y, int x)
{
	return wmove(mainwin,y,x);
};

//Set the amount of time getch waits for input
void timeout(int delay)
{
	inputdelay=delay;
};

#endif
