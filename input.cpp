#include "cursesdef.h"
#include "input.h"

InputEvent get_input() /* Redundancy for the sake of future changes. */
{
	char ch = getch();

	switch(ch)
	{
		case 'k':
			return Up;
		case 'j':
			return Down;
		case 'h':
			return Left;
		case 'l':
			return Right;

		case '\n':
			return Confirm;
	}

	return Undefined;
}
