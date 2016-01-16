#ifndef MONFACTION_H
#define MONFACTION_H

#include "string_id.h"
#include "int_id.h"
#include <unordered_map>

class monfaction;
class JsonObject;

using mfaction_id = int_id<monfaction>;
using mfaction_str_id = string_id<monfaction>;

enum mf_attitude {
    MFA_BY_MOOD = 0,    // Hostile if angry
    MFA_NEUTRAL,        // Neutral even when angry
    MFA_FRIENDLY        // Friendly
};

typedef std::unordered_map< mfaction_id, mf_attitude > mfaction_att_map;

namespace monfactions
{
void finalize();
void load_monster_faction( JsonObject &jo );
mfaction_id get_or_add_faction( const mfaction_str_id &id );
}

class monfaction
{
    public:
        mfaction_id loadid;
        mfaction_id base_faction;
        mfaction_str_id id;

        mfaction_att_map attitude_map;

        mf_attitude attitude( const mfaction_id &other ) const;
};

#endif

