#pragma once
#ifndef CATA_SRC_DEMANGLE_H
#define CATA_SRC_DEMANGLE_H

#include <string>

/**
 * Demangles a C++ symbol
 */
std::string demangle( const char *symbol );

#endif // CATA_SRC_DEMANGLE_H
