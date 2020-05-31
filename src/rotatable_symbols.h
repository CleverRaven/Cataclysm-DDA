#pragma once
#ifndef CATA_SRC_ROTATABLE_SYMBOLS_H
#define CATA_SRC_ROTATABLE_SYMBOLS_H

#include <cstdint>
#include <string>

class JsonObject;

namespace rotatable_symbols
{

void load( const JsonObject &jo, const std::string &src );
void reset();

// Rotate a symbol n times (clockwise).
// @param symbol Symbol to rotate.
// @param n Number of rotations.

uint32_t get( const uint32_t &symbol, int n );

} // namespace rotatable_symbols

#endif // CATA_SRC_ROTATABLE_SYMBOLS_H
