#pragma once
#ifndef CATA_SRC_PERF_H
#define CATA_SRC_PERF_H

#include <stdint.h>

#include <chrono>
#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "debug.h"

struct cata_timer {
        struct timer_stats {
            std::string name;
            std::chrono::steady_clock::time_point current_start;
            uint64_t duration = 0;
            uint64_t wait_accumulator = 0;
            uint64_t wait_duration = 0;
            uint64_t count = 0;
            std::map<std::string, timer_stats, std::less<>> child_timers;
            bool wait = false;

            explicit timer_stats( std::string_view name ) : name{ name } {}

            // NOLINTNEXTLINE(cata-large-inline-function)
            void print_stats_recursively( std::string_view prefix = "" ) const {
                uint64_t child_duration_sum = 0;
                for( const auto& [name, timer] : child_timers ) {
                    // Use wait duration for wait nodes specifically, otherwise regular duration.
                    child_duration_sum += timer.wait ? timer.wait_duration : timer.duration;
                }
                std::string terminal_string = ")";
                uint64_t adj_duration = 0;
                if( wait ) {
                    adj_duration = wait_duration;
                    terminal_string = ") (wait time)";
                } else {
                    adj_duration = duration - wait_duration;
                }
                if( child_duration_sum != 0 ) {
                    uint64_t self_duration = duration - child_duration_sum;
                    uint64_t self_percentage = ( self_duration * 100 ) / adj_duration;
                    terminal_string = ") (self duration: " +
                                      std::to_string( self_duration ) + "us, " + std::to_string( self_percentage ) + "%)";
                }
                DebugLog( DebugLevel::D_WARNING, D_MAIN )
                        << prefix << name << ": " << std::to_string( adj_duration )
                        << "us (avg: " << std::to_string( adj_duration / count )
                        << "us) (count: " << count
                        << terminal_string;
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

        // Timers with the same name at the same level will be merged together.
        // wait = true is intended to represent non-compute time,
        // mostly wrapping functions that wait on player input.
        // wait = true timers should be leaf nodes, we never expect them to have children.
        // NOLINTNEXTLINE(cata-large-inline-function)
        explicit cata_timer( std::string_view name, bool wait = false ) {
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
            timer->second.wait = wait;
            timer_stack().push_back( timer );
            timer->second.current_start = std::chrono::steady_clock::now();
        }

        // NOLINTNEXTLINE(cata-large-inline-function)
        ~cata_timer() {
            std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
            uint64_t elapsed_time = std::chrono::duration_cast<std::chrono::microseconds>( (
                                        end - current_start ) ).count();
            if( timer->second.wait ) {
                timer->second.wait_accumulator += elapsed_time;
            } else {
                timer->second.duration += elapsed_time;
            }
            ++timer->second.count;
            timer_stack().pop_back();
            if( !timer_stack().empty() ) {
                // Wait time is bubbled back up the tree to parents so we can exclude it.
                timer_stack().back()->second.wait_accumulator += timer->second.wait_accumulator;
            }
            timer->second.wait_duration += timer->second.wait_accumulator;
            timer->second.wait_accumulator = 0;
        }

        static void print_stats() {
            for( const auto& [name, timer] : top_level_timer_map() ) {
                timer.print_stats_recursively();
            }
        }
    private:
        timers_map::iterator timer;
        std::chrono::steady_clock::time_point current_start = std::chrono::steady_clock::now();

        static timers_map &top_level_timer_map();
        static std::vector<timers_map::iterator> &timer_stack();
};

#endif // CATA_SRC_PERF_H
