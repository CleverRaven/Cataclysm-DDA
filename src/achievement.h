#ifndef CATA_SRC_ACHIEVEMENT_H
#define CATA_SRC_ACHIEVEMENT_H

#include <array>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "calendar.h"
#include "cata_variant.h"
#include "event_bus.h"
#include "optional.h"
#include "string_id.h"
#include "translations.h"

class JsonIn;
class JsonObject;
class JsonOut;
class achievements_tracker;
class requirement_watcher;
class stats_tracker;
namespace cata
{
class event;
}  // namespace cata
struct achievement_requirement;
template <typename E> struct enum_traits;

enum class achievement_comparison {
    less_equal,
    greater_equal,
    anything,
    last,
};

template<>
struct enum_traits<achievement_comparison> {
    static constexpr achievement_comparison last = achievement_comparison::last;
};

enum class achievement_completion {
    pending,
    completed,
    failed,
    last
};

template<>
struct enum_traits<achievement_completion> {
    static constexpr achievement_completion last = achievement_completion::last;
};

class achievement
{
    public:
        achievement() = default;

        void load( const JsonObject &, const std::string & );
        void check() const;
        static void load_achievement( const JsonObject &, const std::string & );
        static void finalize();
        static void check_consistency();
        static const std::vector<achievement> &get_all();
        static void reset();

        string_id<achievement> id;
        bool was_loaded = false;

        const translation &name() const {
            return name_;
        }

        const translation &description() const {
            return description_;
        }

        const std::vector<string_id<achievement>> &hidden_by() const {
            return hidden_by_;
        }

        class time_bound
        {
            public:
                enum class epoch {
                    cataclysm,
                    game_start,
                    last
                };

                void deserialize( JsonIn & );
                void check( const string_id<achievement> & ) const;

                time_point target() const;
                achievement_completion completed() const;
                std::string ui_text() const;
            private:
                achievement_comparison comparison_;
                epoch epoch_;
                time_duration period_;
        };

        const cata::optional<time_bound> &time_constraint() const {
            return time_constraint_;
        }

        const std::vector<achievement_requirement> &requirements() const {
            return requirements_;
        }
    private:
        translation name_;
        translation description_;
        std::vector<string_id<achievement>> hidden_by_;
        cata::optional<time_bound> time_constraint_;
        std::vector<achievement_requirement> requirements_;
};

template<>
struct enum_traits<achievement::time_bound::epoch> {
    static constexpr achievement::time_bound::epoch last = achievement::time_bound::epoch::last;
};

// Once an achievement is either completed or failed it is stored as an
// achievement_state
struct achievement_state {
    // The final state
    achievement_completion completion;

    // When it became that state
    time_point last_state_change;

    // The values for each requirement at the time of completion or failure
    std::vector<cata_variant> final_values;

    std::string ui_text( const achievement * ) const;

    void serialize( JsonOut & ) const;
    void deserialize( JsonIn & );
};

class achievement_tracker
{
    public:
        // Non-movable because requirement_watcher stores a pointer to us
        achievement_tracker( const achievement_tracker & ) = delete;
        achievement_tracker &operator=( const achievement_tracker & ) = delete;

        achievement_tracker( const achievement &a, achievements_tracker &tracker,
                             stats_tracker &stats );

        void set_requirement( requirement_watcher *watcher, bool is_satisfied );

        bool time_is_expired() const;
        std::vector<cata_variant> current_values() const;
        std::string ui_text() const;
    private:
        const achievement *achievement_;
        achievements_tracker *tracker_;
        std::vector<std::unique_ptr<requirement_watcher>> watchers_;

        // sorted_watchers_ maintains two sets of watchers, categorised by
        // whether they watch a satisfied or unsatisfied requirement.  This
        // allows us to check whether the achievment is met on each new stat
        // value in O(1) time.
        std::array<std::unordered_set<requirement_watcher *>, 2> sorted_watchers_;
};

class achievements_tracker : public event_subscriber
{
    public:
        // Non-movable because achievement_tracker stores a pointer to us
        achievements_tracker( const achievements_tracker & ) = delete;
        achievements_tracker &operator=( const achievements_tracker & ) = delete;

        achievements_tracker(
            stats_tracker &,
            const std::function<void( const achievement * )> &achievement_attained_callback );
        ~achievements_tracker() override;

        // Return all scores which are valid now and existed at game start
        std::vector<const achievement *> valid_achievements() const;

        void report_achievement( const achievement *, achievement_completion );

        achievement_completion is_completed( const string_id<achievement> & ) const;
        bool is_hidden( const achievement * ) const;
        std::string ui_text_for( const achievement * ) const;

        void clear();
        void notify( const cata::event & ) override;

        void serialize( JsonOut & ) const;
        void deserialize( JsonIn & );
    private:
        void init_watchers();

        stats_tracker *stats_ = nullptr;
        std::function<void( const achievement * )> achievement_attained_callback_;
        std::unordered_set<string_id<achievement>> initial_achievements_;

        // Class invariant: each valid achievement has exactly one of a watcher
        // (if it's pending) or a status (if it's completed or failed).
        std::unordered_map<string_id<achievement>, achievement_tracker> trackers_;
        std::unordered_map<string_id<achievement>, achievement_state> achievements_status_;
};

#endif // CATA_SRC_ACHIEVEMENT_H
