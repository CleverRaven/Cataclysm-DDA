#pragma once
#ifndef CATA_SRC_KILL_TRACKER_H
#define CATA_SRC_KILL_TRACKER_H

#include <iosfwd>
#include <map>
#include <vector>

#include "event_subscriber.h"
#include "type_id.h"

class JsonObject;
class JsonOut;

namespace cata
{
class event;
}  // namespace cata

class kill_tracker : public event_subscriber
{
        /** to keep track of new kills, need to access private member (kills, npc_kills), may include getter for thows and remove frend class diary. */
        friend class diary;
    public:
        kill_tracker() = default;
        void reset( const std::map<mtype_id, int> &kills,
                    const std::vector<std::string> &npc_kills );
        /** Returns the number of kills of the given mon_id by the player. */
        int kill_count( const mtype_id & ) const;
        /** Returns the number of kills of the given monster species by the player. */
        int kill_count( const species_id & ) const;
        int monster_kill_count() const;
        int npc_kill_count() const;
        // TEMPORARY until 0.G
        int legacy_kill_xp() const;

        std::string get_kills_text() const;

        void clear();

        void notify( const cata::event & ) override;

        void serialize( JsonOut & ) const;
        void deserialize( const JsonObject &data );
    private:
        std::map<mtype_id, int> kills;         // Player's kill count
        std::vector<std::string> npc_kills;    // names of NPCs the player killed
};

#endif // CATA_SRC_KILL_TRACKER_H
