#include "keypress.h"

#include "cursesdef.h"

#include <errno.h>

long input(long ch)
{
    if (ch == -1) {
        ch = get_keypress();
    }
    return convert_to_dialog_key(ch);
}

long get_keypress()
{
    long ch = getch();

    // Our current tiles and Windows code doesn't have ungetch()
#if !(defined TILES || defined SDLTILES || defined _WIN32 || defined WINDOWS)
    if (ch != ERR) {
        int newch;

        // Clear the buffer of characters that match the one we're going to act on.
        timeout(0);
        do {
            newch = getch();
        } while( newch != ERR && newch == ch );
        timeout(-1);

        // If we read a different character than the one we're going to act on, re-queue it.
        if (newch != ERR && newch != ch) {
            ungetch(newch);
        }
    }
#endif

    return ch;
}

long convert_to_dialog_key(const long ch)
{
    switch (ch)
    {
        case KEY_UP:    return 'k';
        case KEY_LEFT:  return 'h';
        case KEY_RIGHT: return 'l';
        case KEY_DOWN:  return 'j';
        case KEY_NPAGE: return '>';
        case KEY_PPAGE: return '<';
        case 459:       return '\n';
        default:        return ch;
    }
}


bool input_wait(char & ret_ch, int delay_ms)
{
 while(true)
 {
  ret_ch = '\0';
  timeout(delay_ms);
  long ch = getch();
  switch (ch) {
   case KEY_UP:
    ret_ch = 'k';
    break;
   case KEY_LEFT:
    ret_ch = 'h';
    break;
   case KEY_RIGHT:
    ret_ch = 'l';
    break;
   case KEY_DOWN:
    ret_ch = 'j';
    break;
   case KEY_NPAGE:
    ret_ch = '>';
    break;
   case KEY_PPAGE:
    ret_ch = '<';
    break;
   case 459:
    ret_ch = '\n';
    break;
   case ERR:
    if(errno == EINTR)
     return input_wait(ret_ch, delay_ms);
    break;
   default:
    ret_ch = ch;
    break;
  }
  timeout(-1);
  if( ret_ch != '\0' )
   return true;
  return false;
 }
}

