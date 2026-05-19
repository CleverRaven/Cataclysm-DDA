#pragma once
#ifndef CATA_SRC_COLONY_H
#define CATA_SRC_COLONY_H

#include <memory>

#include <plf/colony.h> // IWYU pragma: export

namespace cata
{
template<class T, class Allocator = std::allocator<T>>
using colony = plf::colony<T, Allocator>;
} // namespace cata

#endif // CATA_SRC_COLONY_H
