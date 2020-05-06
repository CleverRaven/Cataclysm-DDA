#pragma once
#ifndef CATA_SRC_MEMORY_FAST_H
#define CATA_SRC_MEMORY_FAST_H

#include <memory>

#if __GLIBCXX__
template<typename T> using shared_ptr_fast = std::__shared_ptr<T, __gnu_cxx::_S_single>;
template<typename T> using weak_ptr_fast = std::__weak_ptr<T, __gnu_cxx::_S_single>;
template<typename T, typename... Args> shared_ptr_fast<T> make_shared_fast(
    Args &&... args )
{
    return std::__make_shared<T, __gnu_cxx::_S_single>( args... );
}
#else
template<typename T> using shared_ptr_fast = std::shared_ptr<T>;
template<typename T> using weak_ptr_fast = std::weak_ptr<T>;
template<typename T, typename... Args> shared_ptr_fast<T> make_shared_fast(
    Args &&... args )
{
    return std::make_shared<T>( args... );
}
#endif

#endif // CATA_SRC_MEMORY_FAST_H
