#pragma once
#ifndef CATA_SRC_MONSTER_ORACLE_H
#define CATA_SRC_MONSTER_ORACLE_H

#include <iosfwd>

#include "behavior_oracle.h"

class monster;

namespace behavior
{
enum class status_t : char;

class monster_oracle_t : public oracle_t
{
    public:
        explicit monster_oracle_t( const monster *subject ) {
            this->subject = subject;
        }
        /**
         * Predicates used by AI to determine goals.
         */
        status_t not_hallucination( const std::string & ) const;
        status_t items_available( const std::string & ) const;
        status_t adjacent_plants( const std::string & ) const;
        status_t special_available( const std::string &special_name ) const;
        status_t split_possible( const std::string & ) const;
    private:
        const monster *subject;
};

} // namespace behavior
#endif // CATA_SRC_MONSTER_ORACLE_H
