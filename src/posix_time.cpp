#include "posix_time.h"

#if (defined _WIN32 || defined __WIN32__) && ! defined __CYGWIN__

#include "platform_win.h"
#include <cstdint>

namespace {

double get_ticks_per_nanosecond() noexcept {
    // On xp and later this should never fail. If it does, it should only happen if
    // ticks_per_second is not aligned to 8 bytes, which should also never happen.
    // just bail here.

    LARGE_INTEGER ticks_per_second;
    if (!QueryPerformanceFrequency(&ticks_per_second)) {
        std::terminate();
    }

    return ticks_per_second.QuadPart / 100000000.0;
}

std::int64_t query_performance_counter() noexcept {
    LARGE_INTEGER result;
    QueryPerformanceCounter(&result);
    return result.QuadPart;
}

} // namespace

int nanosleep(const struct timespec *requested_delay, struct timespec *remaining_delay)
{   
    if (requested_delay->tv_sec < 0 || requested_delay->tv_nsec < 0 ||
        requested_delay->tv_nsec > 999999999l
    ) {
        errno = EINVAL;
        return -1;
    }

    auto const ns = requested_delay->tv_nsec;
    auto const ms = static_cast<DWORD>(ns / 1000000);

    if (requested_delay->tv_sec) {
        // For requested delays of one second or more, the default 15ms resolution is sufficient.
        Sleep(requested_delay->tv_sec * 1000 + ms);
    } else if (requested_delay->tv_nsec) {
        static double const tpn = get_ticks_per_nanosecond();
        auto const deadline = query_performance_counter() + static_cast<std::int64_t>(tpn * ns);

        // SDL sets the timer resolution to 1ms
        if (ms >= 1) {
            Sleep(ms);
        }

        // busy wait for any remnaining time.
        while (query_performance_counter() < deadline) {
        }
    }

    if (remaining_delay) {
        remaining_delay->tv_nsec = 0;
        remaining_delay->tv_sec  = 0;
    }

    return 0;
}
#endif
