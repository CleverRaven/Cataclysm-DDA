#ifndef CATA_SRC_COMPATIBILITY_H
#define CATA_SRC_COMPATIBILITY_H

#include <fstream>
#include <list>
#include <map>
#include <set>
#include <string>

// Some older standard libraries don't have all their classes
// nothrow-move-assignable when you might expect them to be.
// Some of our classes can't be in that case, so we need to know when we're in
// that situation.  Can probably get rid of this once we're on C++17.
// std::list is a problem on clang-3.8 on Travis CI Ubuntu Xenial.
constexpr bool list_is_noexcept = std::is_nothrow_move_assignable<std::list<int>>::value;
// std::string is a problem on gcc-5.3 on Travis CI Ubuntu Xenial.
constexpr bool string_is_noexcept = std::is_nothrow_move_assignable<std::string>::value;
// std::set is a problem in Visual Studio
constexpr bool set_is_noexcept = std::is_nothrow_move_constructible<std::set<std::string>>::value;
// as is std::map
constexpr bool map_is_noexcept = std::is_nothrow_move_constructible<std::map<int, int>>::value;
// std::basic_ifstream<char> and std::basic_ofstream<char> is not noexcept on clang-6
constexpr bool basic_ifstream_is_noexcept =
    std::is_nothrow_move_constructible<std::basic_ifstream<char>>::value;
constexpr bool basic_ofstream_is_noexcept =
    std::is_nothrow_move_constructible<std::basic_ofstream<char>>::value;

// Due to a bug in MinGW we have to manually reset FPU or SSE floating point mode on Windows
void reset_floating_point_mode();

#endif // CATA_SRC_COMPATIBILITY_H
