#pragma once
#ifndef BEHAVIOR_ORACLE_H
#define BEHAVIOR_ORACLE_H

#include <functional>
#include <string>
#include <unordered_map>

class Character;

namespace behavior
{
enum status_t : char;

/**
 * An oracle is a class in charge of introspecting into a game entity in a particular way.
 * The base class has trivial functionality, but provides a mechanism for aggregating oracle
 * instances regardless of type.
 * Specialized oracles, such as a character_oracle_t, provide some number of predicates to retrieve
 * information about the referenced entity.
 */
class oracle_t
{
    public:
        status_t return_running() const;
};

class character_oracle_t : public oracle_t
{
    public:
        character_oracle_t( const Character *subject ) {
            this->subject = subject;
        }
        /**
         * Predicates used by AI to determine goals.
         */
        status_t needs_warmth_badly() const;
        status_t needs_water_badly() const;
        status_t needs_food_badly() const;
        status_t can_wear_warmer_clothes() const;
        status_t can_make_fire() const;
        status_t can_take_shelter() const;
        status_t has_water() const;
        status_t has_food() const;
    private:
        const Character *subject;
};

extern std::unordered_map<std::string, std::function<status_t( const oracle_t * )>> predicate_map;

} // namespace behavior
#endif
