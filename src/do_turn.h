#pragma once
#ifndef CATA_SRC_DO_TURN_H
#define CATA_SRC_DO_TURN_H

/** MAIN GAME LOOP. Returns true if game is over (death, saved, quit, etc.). */
bool do_turn();
void handle_key_blocking_activity();

#endif // CATA_SRC_DO_TURN_H
