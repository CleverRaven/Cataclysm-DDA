#pragma once
#ifndef CATA_SRC_MONFACTION_H
#define CATA_SRC_MONFACTION_H

#include <unordered_map>

#include "type_id.h"

class JsonObject;

enum mf_attitude {
    MFA_BY_MOOD = 0,    // Hostile if angry
    MFA_NEUTRAL,        // Neutral even when angry
    MFA_FRIENDLY,       // Friendly
    MFA_HATE            // Attacks on sight
};

using mfaction_att_map = std::unordered_map< mfaction_id, mf_attitude >;

namespace monfactions
{
void finalize();
void load_monster_faction( const JsonObject &jo );
mfaction_id get_or_add_faction( const mfaction_str_id &id );
} // namespace monfactions

class monfaction
{
    public:
        mfaction_id loadid;
        mfaction_id base_faction;
        mfaction_str_id id;

        mfaction_att_map attitude_map;

        mf_attitude attitude( const mfaction_id &other ) const;
};

#endif // CATA_SRC_MONFACTION_H
