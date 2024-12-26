#include "gpkey.h"

#include <map>
#include <string>
#include <utility>
#include <vector>
#include <initializer_list>
#include <iostream>

#include "string_formatter.h"

std::string convert_to_gamepad( const std::string keybind_in_pre )
{
    std::string keybind_out;
    //define unicode
	// PS Prompts
    //std::string gp_cross = "\u00D7";
    //std::string gp_square = "\u25A1";
    //std::string gp_triangle = "\u25B3";
    //std::string gp_circle = "\u25CB";
	// Eckbok Prompts
    std::string gp_cross = "A";
    std::string gp_circle = "B";
    std::string gp_square = "X";
    std::string gp_triangle = "Y";
    std::string gp_l1 = "L1";
    std::string gp_l2 = "L2";
    std::string gp_r1 = "R1";
    std::string gp_r2 = "R2";
    std::string gp_lstick = "L3";
    std::string gp_rstick = "R3";

	std::string gp_select = "\u23F8";
    std::string gp_start = "\u23F5";
	std::string gp_hotkey = "\u25CD";
	std::string gp_hotkey_mod = "\u23F8+";
	
    std::string gp_up = "\u2191";
    std::string gp_down = "\u2193";
    std::string gp_left = "\u2190";
    std::string gp_right = "\u2192";
    std::string gp_up_left = "\u2196";
    std::string gp_up_right = "\u2197";
    std::string gp_down_left = "\u2199";
    std::string gp_down_right = "\u2198";
	
    std::string gp_lstick_up = "L\u2B89";
	std::string gp_lstick_down = "L\u2B8B";
	std::string gp_lstick_left = "L\u2B88";
	std::string gp_lstick_right = "L\u2B8A";
    std::string gp_rstick_up = "R\u2B89";
	std::string gp_rstick_down = "R\u2B8B";
	std::string gp_rstick_left = "R\u2B88";
	std::string gp_rstick_right = "R\u2B8A";

    std::string l_mod = "L1+";
	std::string keybind_in = keybind_in_pre;
	if( ( keybind_in_pre.rfind("CTRL+", 0 ) == 0 ) || ( keybind_in_pre.rfind("Ctrl+", 0 ) == 0 ) ) {
		keybind_in.erase(0, 5);
		l_mod = "L2+";
	}

    if( keybind_in == "d" ) {
        keybind_out = gp_triangle;
    } else if( keybind_in == "e" ) {
        keybind_out = gp_square;
    } else if( keybind_in == "c" ) { //replace references of examine terrain/furnish to examine all
        keybind_out = gp_square;
    } else if( keybind_in == "f" ) {
        keybind_out = gp_r1;
    } else if( keybind_in == "g" ) {
        keybind_out = gp_r2;
    } else if( keybind_in == "l" ) {
        keybind_out = gp_lstick;
    } else if( keybind_in == "m" ) {
        keybind_out = gp_start;
    } else if( keybind_in == "p" ) {
        keybind_out = gp_select;
    } else if( keybind_in == "r" ) {
        keybind_out = gp_rstick;
    } else if( keybind_in == "u" ) {
        keybind_out = string_format( "%s", gp_lstick_left );
    } else if( keybind_in == "v" ) {
        keybind_out = string_format( "%s", gp_lstick_right );
    } else if( keybind_in == "w" ) {
        keybind_out = string_format( "%s", gp_rstick_up );
    } else if( keybind_in == "x" ) {
        keybind_out = string_format( "%s", gp_rstick_down );
    } else if( keybind_in == "y" ) {
        keybind_out = string_format( "%s", gp_rstick_left );
    } else if( keybind_in == "z" ) {
        keybind_out = string_format( "%s", gp_rstick_right );
    } else if( keybind_in == "D" ) {
        keybind_out = string_format( "%s%s", l_mod, gp_triangle );
    } else if( keybind_in == "B" ) { //replace references of drop to multidrop
        keybind_out = string_format( "%s%s", l_mod, gp_triangle );
    } else if( keybind_in == "E" ) {
        keybind_out = string_format( "%s%s", l_mod, gp_square );
    } else if( keybind_in == "N" ) { //replace references of pickup adjacent to pickup nearby
        keybind_out = string_format( "%s%s", l_mod, gp_square );
    } else if( keybind_in == "F" ) {
        keybind_out = string_format( "%s%s", l_mod, gp_r1 );
    } else if( keybind_in == "G" ) {
        keybind_out = string_format( "%s%s", l_mod, gp_r2 );
    } else if( keybind_in == "L" ) {
        keybind_out = string_format( "%s%s", l_mod, gp_lstick );
    } else if( keybind_in == "M" ) {
        keybind_out = string_format( "%s%s", l_mod, gp_start );
    } else if( keybind_in == "P" ) {
        keybind_out = string_format( "%s%s", l_mod, gp_select );
    } else if( keybind_in == "R" ) {
        keybind_out = string_format( "%s%s", l_mod, gp_rstick );
    } else if( keybind_in == "U" ) {
        keybind_out = string_format( "%s%s", l_mod, gp_lstick_left );
    } else if( keybind_in == "V" ) {
        keybind_out = string_format( "%s%s", l_mod, gp_lstick_right );
    } else if( keybind_in == "W" ) {
        keybind_out = string_format( "%s%s", l_mod, gp_rstick_up );
    } else if( keybind_in == "X" ) {
        keybind_out = string_format( "%s%s", l_mod, gp_rstick_down );
    } else if( keybind_in == "Y" ) {
        keybind_out = string_format( "%s%s", l_mod, gp_rstick_left );
    } else if( keybind_in == "Z" ) {
        keybind_out = string_format( "%s%s", l_mod, gp_rstick_right );
    } else if( keybind_in == "F1" ) {
        keybind_out = gp_hotkey;
    } else if( keybind_in == "&" ) {
        keybind_out = string_format( "%s%s", gp_hotkey_mod, gp_cross );
    } else if( keybind_in == "*" ) {
        keybind_out = string_format( "%s%s", gp_hotkey_mod, gp_triangle );
    } else if( keybind_in == "\\" ) {
        keybind_out = string_format( "%s%s", gp_hotkey_mod, gp_circle );
    } else if( keybind_in == "/" ) {
        keybind_out = string_format( "%s%s", gp_hotkey_mod, gp_square );
    } else if( keybind_in == "?" ) {
        keybind_out = string_format( "%s%s", gp_hotkey_mod, gp_l1 );
    } else if( keybind_in == "[" ) {
        keybind_out = string_format( "%s", gp_lstick_up );
    } else if( keybind_in == "]" ) {
        keybind_out = string_format( "%s", gp_lstick_down );
    } else if( keybind_in == "{" ) {
        keybind_out = string_format( "%s%s", l_mod, gp_lstick_up );
    } else if( keybind_in == "}" ) {
        keybind_out = string_format( "%s%s", l_mod, gp_lstick_down );
    } else if( keybind_in == "-" ) {
        keybind_out = string_format( "%s%s", gp_hotkey_mod, gp_r1 );
    } else if( keybind_in == "+" ) {
        keybind_out = string_format( "%s%s", gp_hotkey_mod, gp_r2 );
    } else if( keybind_in == "=" ) {
        keybind_out = string_format( "%s%s", gp_hotkey_mod, gp_l1 );
    } else if( keybind_in == "UP" ) {
        keybind_out = gp_up;
    } else if( keybind_in == "DOWN" ) {
        keybind_out = gp_down;
    } else if( keybind_in == "LEFT" ) {
        keybind_out = gp_left;
    } else if( keybind_in == "RIGHT" ) {
        keybind_out = gp_right;
	//handle diagonals
    } else if( keybind_in == "HOME" ) { //NW
        keybind_out = string_format( "%s%s", l_mod, gp_up_left );
	} else if( keybind_in == "PPAGE" ) { //NE
        keybind_out = string_format( "%s%s", l_mod, gp_up_right );
	} else if( keybind_in == "END" ) { //SW
        keybind_out = string_format( "%s%s", l_mod, gp_down_left );
	} else if( keybind_in == "NPAGE" ) { //SE
        keybind_out = string_format( "%s%s", l_mod, gp_down_right );
    } else if( keybind_in == "ESC" ) {
        keybind_out = gp_circle;
    } else if( keybind_in == "S" ) { //replace references of save to main menu (where save exists)
        keybind_out = gp_circle;
    } else if( keybind_in == "RETURN" ) {
        keybind_out = gp_cross;
    } else {
        keybind_out = keybind_in;
    }
    return keybind_out;
}