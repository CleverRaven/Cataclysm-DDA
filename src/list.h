#pragma once
#ifndef CATA_SRC_LIST_H
#define CATA_SRC_LIST_H

#include <memory>

#include <plf/list.h> // IWYU pragma: export

namespace cata
{
template<class T, class Allocator = std::allocator<T>>
using list = plf::list<T, Allocator>;
} // namespace cata

#endif // CATA_SRC_LIST_H
