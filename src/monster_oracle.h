#pragma once
#ifndef CATA_SRC_MONSTER_ORACLE_H
#define CATA_SRC_MONSTER_ORACLE_H

#include <string>

#include "behavior_oracle.h"

class monster;

namespace behavior
{

class monster_oracle_t : public oracle_t
{
    public:
        monster_oracle_t( const monster *subject ) {
            this->subject = subject;
        }
        /**
         * Predicates used by AI to determine goals.
         */
        status_t not_hallucination( const std::string & ) const;
        status_t items_available( const std::string & ) const;
        status_t adjacent_plants( const std::string & ) const;
        status_t special_available( const std::string &special_name ) const;
    private:
        const monster *subject;
};

} // namespace behavior
#endif // CATA_SRC_MONSTER_ORACLE_H
