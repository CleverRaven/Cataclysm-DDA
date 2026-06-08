#include "behavior_strategy.h"

#include <unordered_map>
#include <utility>

#include "behavior.h"

namespace behavior
{
static sequential_t default_sequential;
static fallback_t default_fallback;
static sequential_until_done_t default_until_done;
static utility_t default_utility;

std::unordered_map<std::string, const strategy_t *> strategy_map = {{
        { "sequential", &default_sequential },
        { "fallback", &default_fallback },
        { "sequential_until_done", &default_until_done },
        { "utility", &default_utility }
    }
};
} // namespace behavior

using namespace behavior;

// A standard behavior strategy, execute runnable children in order unless one fails.
behavior_return sequential_t::evaluate( const oracle_t *subject,
                                        const std::vector<const node_t *> children ) const
{
    for( const node_t *child : children ) {
        behavior_return outcome = child->tick( subject );
        if( outcome.result == status_t::running || outcome.result == status_t::failure ) {
            return outcome;
        }
    }
    return { status_t::success, nullptr };
}

// A standard behavior strategy, execute runnable children in order until one succeeds.
behavior_return fallback_t::evaluate( const oracle_t *subject,
                                      const std::vector<const node_t *> children ) const
{
    for( const node_t *child : children ) {
        behavior_return outcome = child->tick( subject );
        if( outcome.result == status_t::running || outcome.result == status_t::success ) {
            return outcome;
        }
    }
    return { status_t::failure, nullptr };
}

// A non-standard behavior strategy, execute runnable children in order unconditionally.
behavior_return sequential_until_done_t::evaluate( const oracle_t *subject,
        const std::vector<const node_t *> children ) const
{
    for( const node_t *child : children ) {
        behavior_return outcome = child->tick( subject );
        if( outcome.result == status_t::running ) {
            return outcome;
        }
    }
    return { status_t::success, nullptr };
}

// Evaluate all children and select the running one with the highest score.
behavior_return utility_t::evaluate( const oracle_t *subject,
                                     const std::vector<const node_t *> children ) const
{
    behavior_return best = { status_t::failure, nullptr, 0.0f };
    bool any_success = false;
    for( const node_t *child : children ) {
        behavior_return outcome = child->tick( subject );
        if( outcome.result == status_t::running ) {
            if( best.result != status_t::running || outcome.score > best.score ) {
                best = outcome;
            }
        } else if( outcome.result == status_t::success ) {
            any_success = true;
        }
    }
    if( best.result == status_t::running ) {
        return best;
    }
    return { any_success ? status_t::success : status_t::failure, nullptr, 0.0f };
}
