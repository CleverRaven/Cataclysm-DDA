#ifndef CATA_TESTS_ACTIVITY_SCHEDULING_HELPER_H
#define CATA_TESTS_ACTIVITY_SCHEDULING_HELPER_H

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "activity_actor.h"
#include "calendar.h"
#include "clone_ptr.h"
#include "type_id.h"

class avatar;

struct schedule {
    virtual void setup( avatar & ) const = 0;
    virtual void do_turn( avatar & ) const = 0;
    virtual bool instantaneous() const {
        return false;
    }
    time_duration interval = 5_minutes;

    virtual ~schedule() = default;
};

class activity_schedule : public schedule
{
        activity_id act;
        cata::clone_ptr<activity_actor> actor;

    public:
        void setup( avatar &guy ) const override;
        void do_turn( avatar &guy ) const override;

        activity_schedule( const activity_id &id, const time_duration &ticks ) : act( id ) {
            interval = ticks;
        }
        activity_schedule( const activity_actor &assigned,
                           const time_duration &ticks ) : actor( assigned.clone() ) {
            interval = ticks;
        }
};

class meal_schedule : public schedule
{
        itype_id food;

    public:
        void setup( avatar &guy ) const override;
        void do_turn( avatar & ) const override;
        bool instantaneous() const override;

        explicit meal_schedule( const itype_id &eaten ) : food( eaten ) {}
};

class clear_guts : public schedule
{
    public:
        void setup( avatar &guy ) const override;
        void do_turn( avatar & ) const override;
        bool instantaneous() const override;
};

const clear_guts sched_clear_guts{};

class sleep_schedule : public schedule
{
    public:
        void setup( avatar &guy ) const override;
        void do_turn( avatar &guy ) const override;
};

class tasklist
{
        // The tasks we have yet to do, and how long
        // The first member is a pointer because we need to preserve the virtual
        // class that it's pointing to
        // It will _never_ be nullptr
        std::vector<std::pair<const schedule *, time_duration>> tasks;
        // How long we've been on the current task
        time_duration advanced = 0_seconds;
        // The current task's index
        size_t cursor = 0;

    public:
        const schedule &next_task();

        void advance( const time_duration &how_long );

        void enschedule( const schedule &added, const time_duration &how_long );

        void clear();

        time_duration duration();
};

struct weariness_events {

        struct weary_transition {
            int minutes = 0;
            int from = 0;
            int to = 0;
            int new_weariness = 0;
            int new_threshold = 0;
            int intake = 0;
            int tracker = 0;
        };

        std::vector<weary_transition> transitions;

    public:
        void log( int old_level, int new_level, const time_duration &when,
                  int new_weariness, int new_threshold, int intake, int tracker );

        // Return the first time a transition between `from` and `to` occurs, in minutes
        // if around = 0_seconds or equivalent, otherwise return the time closest to around
        int transition_minutes( int from, int to, const time_duration &around ) const;

        bool have_weary_decrease() const;

        std::string summarize() const;

        bool empty() const;
};

weariness_events do_activity( tasklist tasks, bool do_clear_avatar = true );

#endif // CATA_TESTS_ACTIVITY_SCHEDULING_HELPER_H
