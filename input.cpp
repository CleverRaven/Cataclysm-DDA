#include "cursesdef.h"
#include "input.h"

/* TODO Replace the hardcoded values with an abstraction layer.
 * Lower redundancy across the methods. */

InputEvent get_input(int ch)
{
	if (ch == '\0')
		ch = getch();

	switch(ch)
	{
		case 'k':
		case '8':
        case KEY_UP:
			return DirectionN;
		case 'j':
		case '2':
        case KEY_DOWN:
			return DirectionS;
		case 'l':
		case '6':
        case KEY_RIGHT:
			return DirectionE;
		case 'h':
		case '4':
        case KEY_LEFT:
			return DirectionW;
		case 'y':
		case '7':
			return DirectionNW;
		case 'u':
		case '9':
			return DirectionNE;
		case 'b':
		case '1':
			return DirectionSW;
		case 'n':
		case '3':
			return DirectionSE;
		case '.':
		case '5':
			return DirectionNone;
		case '>':
			return DirectionDown;
		case '<':
			return DirectionUp;

		case '\n':
			return Confirm;
		case ' ':
			return Close;
		case 27: /* TODO Fix delay */
		case 'q':
			return Cancel;
		case '?':
			return Help;

		case ',':
		case 'g':
			return Pickup;

		default:
			return Undefined;
	}

}

void get_direction(int &x, int &y, InputEvent &input)
{
	x = 0;
	y = 0;

	switch(input) {
		case DirectionN:
			--y;
			break;
		case DirectionS:
			++y;
			break;
		case DirectionE:
			++x;
			break;
		case DirectionW:
			--x;
			break;
		case DirectionNW:
			--x;
			--y;
			break;
		case DirectionNE:
			++x;
			--y;
			break;
		case DirectionSW:
			--x;
			++y;
			break;
		case DirectionSE:
			++x;
			++y;
			break;
		case DirectionNone:
		case Pickup:
			break;
		default:
			x = -2;
			y = -2;
	}
}
