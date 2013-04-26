#ifndef _INPUT_H_
#define _INPUT_H_

enum InputEvent {
	Confirm,
	Cancel,
	Close,
	Help,

	DirectionN,
	DirectionS,
	DirectionE,
	DirectionW,
	DirectionNW,
	DirectionNE,
	DirectionSE,
	DirectionSW,
	DirectionNone,

	DirectionDown, /* Think stairs */
	DirectionUp,
    Filter,
    Reset,
	Pickup,
	Nothing,
	
	DirectionN_shift,
	DirectionS_shift,
	DirectionE_shift,
	DirectionW_shift,
	DirectionNW_shift,
	DirectionNE_shift,
	DirectionSW_shift,
	DirectionSE_shift,

	Undefined
};

InputEvent get_input(int ch = '\0');
void get_direction(int &x, int &y, InputEvent &input);

#endif
