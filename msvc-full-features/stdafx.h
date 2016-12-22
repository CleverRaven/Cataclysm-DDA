#pragma once

#include <cassert>
#include <cinttypes>
#include <csignal>
#include <cstdio>
#include <cwchar>
#include <ccomplex>
//#include <ciso646>
//#include <cstdalign>
#include <cstdlib>
#include <cwctype>
#include <cctype>
#include <climits>
#include <cstdarg>
#include <cstring>
#include <cerrno>
#include <clocale>
#include <cstdbool>
#include <ctgmath>
#include <cfenv>
#include <cmath>
#include <cstddef>
#include <ctime>
#include <cfloat>
#include <csetjmp>
#include <cstdint>
#include <cuchar>
#include <algorithm>
#include <fstream>
#include <list>
#include <regex>
#include <tuple>
#include <array>
#include <functional>
#include <locale>
#include <scoped_allocator>
#include <type_traits>
//#include <atomic>
//#include <future>
#include <map>
#include <set>
#include <typeindex>
#include <bitset>
#include <initializer_list>
#include <memory>
#include <sstream>
#include <typeinfo>
#include <chrono>
#include <iomanip>
#include <mutex>
#include <stack>
#include <unordered_map>
#include <codecvt>
#include <ios>
#include <new>
#include <stdexcept>
#include <unordered_set>
#include <complex>
#include <iosfwd>
#include <numeric>
#include <streambuf>
#include <utility>
//#include <condition_variable>
#include <iostream>
#include <ostream>
#include <string>
//#include <valarray>
#include <deque>
#include <istream>
#include <queue>
#include <strstream>
#include <vector>
#include <exception>
#include <iterator>
#include <random>
#include <system_error>
#include <forward_list>
#include <limits>
#include <ratio>
//#include <thread>

#include "../src/platform_win.h"

#if defined(TILES)
#   include <SDL.h>
#   include <SDL_ttf.h>
#   include <SDL_image.h>
#   if defined(SDL_SOUND)
#       include <SDL_mixer.h>
#   endif
#endif
