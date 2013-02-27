#ifndef _INPUT_H_
#define _INPUT_H_

enum InputEvent {
	Confirm,
	Cancel,
	Close,
	Up,
	Down,
	Left,
	Right,
	Undefined
};

InputEvent get_input();

#endif
