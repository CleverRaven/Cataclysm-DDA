#pragma once
#ifndef CATA_SRC_NPC_SLEEP_H
#define CATA_SRC_NPC_SLEEP_H

#include "calendar.h"

class JsonObject;
class JsonOut;

enum class npc_sleep_phase : int {
    none,
    // lying down, rolling to fall asleep on a cadence
    settling,
    asleep,
    // woken early; the sleep executor lies back down here, searches, or gives the goal up
    woken
};

// how one attempt to sleep is going, what has happened to it, and the holds that outlive
// it. effects say whether the NPC lies or sleeps; permission and interruption are decided
// elsewhere
struct npc_sleep_session {
    // the attempt
    npc_sleep_phase phase = npc_sleep_phase::none;
    time_point settle_started = calendar::turn_zero;
    time_point next_attempt = calendar::turn_zero;
    time_point woken_at = calendar::turn_zero;
    // the history
    time_point wake_window_start = calendar::turn_zero;
    int wakes = 0;
    // a stalled sleep steps aside for other work until this passes
    time_point stalled_until = calendar::turn_zero;
    // no new sleep before this passes, unless sleepiness reaches the floor
    time_point stay_awake_until = calendar::turn_zero;
    int stay_awake_floor = 0;

    void begin_settling();
    // settling or asleep -> woken; too many wakes inside restless_window stall the sleep
    void note_wake();
    void stall();
    // forgets the attempt and its history, keeps the holds
    void end();
    bool in_grace( int sleepiness ) const;
    bool stalled() const;
    // woken but still worth lying back down: half way to tired
    bool wants_resume( int sleepiness ) const;
    // lying still a while makes sleep come easier: a point per attempt interval, capped
    int patience() const;

    void serialize( JsonOut &json ) const;
    void deserialize( const JsonObject &data );
};

#endif // CATA_SRC_NPC_SLEEP_H
