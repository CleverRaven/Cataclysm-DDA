#include "behavior_strategy.h"

#include <unordered_map>

#include "behavior.h"

namespace behavior
{
sequential_t default_sequential;
fallback_t default_fallback;
sequential_until_done_t default_until_done;

std::unordered_map<std::string, const strategy_t *> strategy_map = {{
        { "sequential", &default_sequential },
        { "fallback", &default_fallback },
        { "sequential_until_done", &default_until_done }
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
        if( outcome.result == running || outcome.result == failure ) {
            return outcome;
        }
    }
    return { success, nullptr };
}

// A standard behavior strategy, execute runnable children in order until one succeeds.
behavior_return fallback_t::evaluate( const oracle_t *subject,
                                      const std::vector<const node_t *> children ) const
{
    for( const node_t *child : children ) {
        behavior_return outcome = child->tick( subject );
        if( outcome.result == running || outcome.result == success ) {
            return outcome;
        }
    }
    return { failure, nullptr };
}

// A non-standard behavior strategy, execute runnable children in order unconditionally.
behavior_return sequential_until_done_t::evaluate( const oracle_t *subject,
        const std::vector<const node_t *> children ) const
{
    for( const node_t *child : children ) {
        behavior_return outcome = child->tick( subject );
        if( outcome.result == running ) {
            return outcome;
        }
    }
    return { success, nullptr };
}
