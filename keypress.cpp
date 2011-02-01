#include "keypress.h"

long input()
{
 long ch = getch();
 switch (ch) {
  case '7': return 'y';
  case KEY_UP:
  case '8': return 'k';
  case '9': return 'u';
  case KEY_LEFT:
  case '4': return 'h';
  case '5': return '.';
  case KEY_RIGHT:
  case '6': return 'l';
  case '1': return 'b';
  case KEY_DOWN:
  case '2': return 'j';
  case '3': return 'n';
  default:  return ch;
 }
}

void get_direction(int &x, int &y, char ch)
{
 x = 0;
 y = 0;
 switch (ch) {
 case 'y':
  x = -1;
  y = -1;
  return;
 case 'u':
  x = 1;
  y = -1;
  return;
 case 'h':
  x = -1;
  return;
 case 'j':
  y = 1;
  return;
 case 'k':
  y = -1;
  return;
 case 'l':
  x = 1;
  return;
 case 'b':
  x = -1;
  y = 1;
  return;
 case 'n':
  x = 1;
  y = 1;
  return;
 case '.':
 case ',':
 case 'g':
  x = 0;
  y = 0;
  return;
 default:
  x = -2;
  y = -2;
 }
}
