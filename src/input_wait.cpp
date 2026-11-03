#include "input_wait.h"

#include <algorithm>
#include <limits>

uint32_t ms_until_elapsed_exceeds( const uint32_t start, const uint32_t now, const uint32_t period )
{
    const uint32_t elapsed = now - start;
    return elapsed > period ? 0 : period - elapsed + 1;
}

uint32_t ms_until_elapsed_reaches( const uint32_t start, const uint32_t now, const uint32_t period )
{
    const uint32_t elapsed = now - start;
    return elapsed >= period ? 0 : period - elapsed;
}

namespace
{

void take_sooner( std::optional<uint32_t> &acc, const uint32_t ms )
{
    acc = acc ? std::min( *acc, ms ) : ms;
}

} // namespace

int input_wait_timeout_ms( const input_wait_state &state )
{
    std::optional<uint32_t> wait = state.input_ms;
    if( state.present_ms ) {
        take_sooner( wait, *state.present_ms );
    } else if( state.platform_ms ) {
        take_sooner( wait, *state.platform_ms );
    }
    if( !wait ) {
        return -1;
    }
    return static_cast<int>( std::min<uint32_t>( *wait, std::numeric_limits<int>::max() ) );
}

std::optional<uint32_t> touch_wait_ms( const touch_timers &t )
{
    std::optional<uint32_t> wait;
    // a back button held past the delay toggles the shortcut strip once
    if( t.back_down_time > 0 && !t.back_toggle_handled ) {
        take_sooner( wait, ms_until_elapsed_exceeds( t.back_down_time, t.now, t.initial_delay ) );
    }
    const bool single_finger = !t.quick_shortcut_touch && !t.multi_finger_touch;
    // a held finger repeats past the delay, then every repeat delay; both
    // checks must pass, as in CheckMessages
    if( single_finger && t.finger_down_time > 0 ) {
        take_sooner( wait, std::max(
                         ms_until_elapsed_exceeds( t.finger_down_time, t.now, t.initial_delay ),
                         ms_until_elapsed_exceeds( t.finger_repeat_time, t.now, t.finger_repeat_delay ) ) );
    }
    // first tap with no second tap in the delay becomes a single tap
    if( single_finger && t.last_tap_time > 0 ) {
        take_sooner( wait, ms_until_elapsed_reaches( t.last_tap_time, t.now, t.initial_delay ) );
    }
    // held shortcut shows hint past delay and repaints at present rate
    if( t.quick_shortcut_touch && t.finger_down_time > 0 ) {
        take_sooner( wait, std::max(
                         ms_until_elapsed_exceeds( t.finger_down_time, t.now, t.initial_delay ),
                         ms_until_elapsed_reaches( t.last_present, t.now, t.present_interval ) ) );
    }
    return wait;
}
