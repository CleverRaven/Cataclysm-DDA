#pragma once
#ifndef CATA_SRC_PERF_H
#define CATA_SRC_PERF_H

#include <array>
#include <chrono>
#include <iostream>
#include <list>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "debug.h"

struct cata_timer {
        struct timer_stats {
            std::string name;
            std::chrono::high_resolution_clock::time_point current_start;
            uint64_t duration = 0;
            uint64_t count = 0;
            std::map<std::string, timer_stats, std::less<>> child_timers;

            timer_stats( std::string_view name ) : name{ name } {}

            void print_stats_recursively( std::string_view prefix = "" ) const {
                DebugLog( DebugLevel::D_WARNING,
                          D_MAIN ) << prefix << name << ": " << std::to_string( duration ) << "ms (avg: " << std::to_string(
                                       duration / count ) << "ms) (count: " << count << ")";
                std::string child_prefix;
                child_prefix.reserve( prefix.length() + 2 );
                child_prefix += prefix;
                child_prefix += "  ";
                for( const auto& [name, timer] : child_timers ) {
                    timer.print_stats_recursively( child_prefix );
                }
            }
        };

        using timers_map = std::map<std::string, timer_stats, std::less<>>;

        cata_timer( std::string_view name ) {
            timers_map &timer_map = ( [&]() -> timers_map& {
                if( timer_stack().empty() ) {
                    return top_level_timer_map();
                } else {
                    return timer_stack().back()->second.child_timers;
                }
            } )();
            timer = timer_map.find( name );
            if( timer == timer_map.end() ) {
                timer = timer_map.emplace( name, timer_stats{ name } ).first;
            }
            timer_stack().push_back( timer );
            timer->second.current_start = std::chrono::high_resolution_clock::now();
        }

        ~cata_timer() {
            std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
            timer->second.duration += std::chrono::duration_cast<std::chrono::microseconds>( (
                                          end - current_start ) ).count();
            ++timer->second.count;
            timer_stack().pop_back();
        }

        static void print_stats() {
            for( const auto& [name, timer] : top_level_timer_map() ) {
                timer.print_stats_recursively();
            }
        }
    private:
        timers_map::iterator timer;
        std::chrono::high_resolution_clock::time_point current_start =
            std::chrono::high_resolution_clock::now();

        static timers_map &top_level_timer_map();
        static std::vector<timers_map::iterator> &timer_stack();
};

#endif // CATA_SRC_PERF_H
