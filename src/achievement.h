#ifndef CATA_SRC_ACHIEVEMENT_H
#define CATA_SRC_ACHIEVEMENT_H

#include <list>
#include <string>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "event_bus.h"
#include "string_id.h"
#include "translations.h"

class JsonObject;
struct achievement_requirement;
class achievement_tracker;
class stats_tracker;

class achievement
{
    public:
        achievement() = default;

        void load( const JsonObject &, const std::string & );
        void check() const;
        static void load_achievement( const JsonObject &, const std::string & );
        static void check_consistency();
        static const std::vector<achievement> &get_all();
        static void reset();

        string_id<achievement> id;
        bool was_loaded = false;

        const translation &description() const {
            return description_;
        }

        const std::vector<achievement_requirement> &requirements() const {
            return requirements_;
        }
    private:
        translation description_;
        std::vector<achievement_requirement> requirements_;
};

enum class achievement_completion {
    pending,
    completed,
    last
};

template<>
struct enum_traits<achievement_completion> {
    static constexpr achievement_completion last = achievement_completion::last;
};

struct achievement_state {
    achievement_completion completion;
    time_point last_state_change;

    void serialize( JsonOut & ) const;
    void deserialize( JsonIn & );
};

class achievements_tracker : public event_subscriber
{
    public:
        // Non-movable because achievement_tracker stores a pointer to us
        achievements_tracker( const achievements_tracker & ) = delete;
        achievements_tracker &operator=( const achievements_tracker & ) = delete;

        achievements_tracker( stats_tracker &,
                              const std::function<void( const achievement * )> &achievement_attained_callback );
        ~achievements_tracker() override;

        // Return all scores which are valid now and existed at game start
        std::vector<const achievement *> valid_achievements() const;

        void report_achievement( const achievement *, achievement_completion );

        void clear();
        void notify( const cata::event & ) override;

        void serialize( JsonOut & ) const;
        void deserialize( JsonIn & );
    private:
        void init_watchers();

        stats_tracker *stats_ = nullptr;
        std::function<void( const achievement * )> achievement_attained_callback_;
        std::list<achievement_tracker> watchers_;
        std::unordered_set<string_id<achievement>> initial_achievements_;
        std::unordered_map<string_id<achievement>, achievement_state> achievements_status_;
};

#endif // CATA_SRC_ACHIEVEMENT_H
