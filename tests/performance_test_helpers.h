#pragma once

#include <chrono>
#include <utility>

template<typename Func>
std::chrono::microseconds::rep measure_us( Func &&operation )
{
    const auto before = std::chrono::steady_clock::now();
    std::forward<Func>( operation )();
    const auto after = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>( after - before ).count();
}