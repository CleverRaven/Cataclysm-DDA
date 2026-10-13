#pragma once
#ifndef CATA_SRC_DO_TURN_H
#define CATA_SRC_DO_TURN_H

class map;
class npc;

void handle_key_blocking_activity();

struct npc_turn_result {
    // spent no moves for the whole retry budget and was rebooted ("faints")
    bool spun_out = false;
};

// one NPC's share of a game turn, exactly as the game loop runs it
npc_turn_result process_npc_turn( map &m, npc &guy );

#endif // CATA_SRC_DO_TURN_H
