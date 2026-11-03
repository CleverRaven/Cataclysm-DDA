#pragma once
#ifndef CATA_SRC_INPUT_WAIT_H
#define CATA_SRC_INPUT_WAIT_H

#include <cstdint>
#include <optional>

// milliseconds until `now - start > period` holds, in wrapping uint32_t
// ticks, exactly as the input loop's timers test it
uint32_t ms_until_elapsed_exceeds( uint32_t start, uint32_t now, uint32_t period );
// as above for `now - start >= period`
uint32_t ms_until_elapsed_reaches( uint32_t start, uint32_t now, uint32_t period );

// what one idle wait of the input loop may block on, each as ms from now
struct input_wait_state {
    // until the caller's input timeout; nullopt waits for input indefinitely
    std::optional<uint32_t> input_ms;
    // until a present deferred by the refresh throttle is due
    std::optional<uint32_t> present_ms;
    // until the next platform timer CheckMessages polls for. ignored while a
    // present is pending: CheckMessages runs those timers only once
    // needupdate is clear
    std::optional<uint32_t> platform_ms;
};

// milliseconds to block for the next event: 0 polls, -1 blocks without a timeout
int input_wait_timeout_ms( const input_wait_state &state );

// android touch state CheckMessages checks by time rather than by event
struct touch_timers {
    uint32_t now = 0;
    uint32_t initial_delay = 0;
    uint32_t finger_down_time = 0;
    uint32_t finger_repeat_time = 0;
    uint32_t finger_repeat_delay = 0;
    uint32_t last_tap_time = 0;
    uint32_t back_down_time = 0;
    bool back_toggle_handled = false;
    bool quick_shortcut_touch = false;
    bool multi_finger_touch = false;
    uint32_t last_present = 0;
    uint32_t present_interval = 0;
};

// milliseconds until next CheckMessages touch timer fires, or nullopt when none
// is armed
std::optional<uint32_t> touch_wait_ms( const touch_timers &t );

#endif // CATA_SRC_INPUT_WAIT_H
