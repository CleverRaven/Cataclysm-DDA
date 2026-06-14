#pragma once
#ifndef CATA_SRC_NPC_OPINION_H
#define CATA_SRC_NPC_OPINION_H

class JsonOut;
class JsonObject;

struct npc_opinion {
    int trust;
    int fear;
    int value;
    int anger;
    int owed; // Positive when the npc owes the player. Negative if player owes them.
    int sold; // Total value of goods sold/donated by player to the npc. Cannot be negative.

    npc_opinion() {
        trust = 0;
        fear = 0;
        value = 0;
        anger = 0;
        owed = 0;
        sold = 0;
    }

    npc_opinion &operator+=( const npc_opinion &rhs ) {
        trust += rhs.trust;
        fear += rhs.fear;
        value += rhs.value;
        anger += rhs.anger;
        owed += rhs.owed;
        sold += rhs.sold;
        return *this;
    }

    npc_opinion operator+( const npc_opinion &rhs ) {
        return npc_opinion( *this ) += rhs;
    }

    void serialize( JsonOut &json ) const;
    void deserialize( const JsonObject &data );
};

#endif // CATA_SRC_NPC_OPINION_H
