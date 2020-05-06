#pragma once

#include <algorithm>
#include <array>
#include <bitset>
#include <cassert>
#include <ccomplex>
#include <cctype>
#include <cerrno>
#include <cfenv>
#include <cfloat>
#include <chrono>
#include <cinttypes>
#include <climits>
#include <clocale>
#include <cmath>
#include <codecvt>
#include <complex>
#include <csetjmp>
#include <csignal>
#include <cstdarg>
#include <cstdbool>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctgmath>
#include <ctime>
#include <cuchar>
#include <cwchar>
#include <cwctype>
#include <deque>
#include <exception>
#include <forward_list>
#include <fstream>
#include <functional>
#include <initializer_list>
#include <iomanip>
#include <ios>
#include <iosfwd>
#include <iostream>
#include <istream>
#include <iterator>
#include <limits>
#include <list>
#include <locale>
#include <map>
#include <memory>
#include <mutex>
#include <new>
#include <numeric>
#include <ostream>
#include <queue>
#include <random>
#include <ratio>
#include <regex>
#include <scoped_allocator>
#include <set>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <streambuf>
#include <string>
#include <strstream>
#include <system_error>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "../src/platform_win.h"

#if defined(TILES)
#   if defined(_MSC_VER) && defined(USE_VCPKG)
#      include <SDL2/SDL.h>
#      include <SDL2/SDL_image.h>
#      include <SDL2/SDL_mixer.h>
#      include <SDL2/SDL_ttf.h>
#      include <SDL2/SDL_version.h>
#      ifdef SDL_SOUND
#          include <SDL2/SDL_mixer.h>
#      endif
#   else
#      include <SDL.h>
#      include <SDL_image.h>
#      include <SDL_mixer.h>
#      include <SDL_ttf.h>
#      include <SDL_version.h>
#      ifdef SDL_SOUND
#          include <SDL_mixer.h>
#      endif
#   endif
#endif
