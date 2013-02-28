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
	DirectionDown,
	DirectionUp,

	Pickup,
	Nothing,
	Undefined
};

InputEvent get_input();
void get_direction(int &x, int &y, InputEvent &input);

#endif
