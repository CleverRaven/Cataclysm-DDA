#pragma once
#ifndef CATA_KILL_TRACKER_H
#define CATA_KILL_TRACKER_H

#include <map>
#include <string>
#include <vector>

#include "event_bus.h"
#include "type_id.h"

class kill_tracker : public event_subscriber
{
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
        // returns player's "kill xp" for monsters via STK
        int kill_xp() const;

        void disp_kills() const;

        void clear();

        void notify( const event & ) override;

        void serialize( JsonOut & ) const;
        void deserialize( JsonIn & );
    private:
        std::map<mtype_id, int> kills;         // Player's kill count
        std::vector<std::string> npc_kills;    // names of NPCs the player killed
};

#endif // CATA_KILL_TRACKER_H
