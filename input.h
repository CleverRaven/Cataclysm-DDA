#ifndef _INPUT_H_
#define _INPUT_H_

#include <string>

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
	Undefined
};

InputEvent get_input(int ch = '\0');
void get_direction(int &x, int &y, InputEvent &input);
std::string get_input_string_from_file(std::string fname="input.txt");
#endif
