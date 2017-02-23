#pragma once
#ifndef ROTATABLE_SYMBOLS_H
#define ROTATABLE_SYMBOLS_H

#include <string>

class JsonObject;

namespace rotatable_symbols
{

void load( JsonObject &jo, const std::string &src );
void reset();

// Rotate a symbol n times (clockwise).
// @param sym Symbol to rotate.
// @param n Number of rotations.

long get( long sym, int n );

}

#endif // ROTATABLE_SYMBOLS_H
