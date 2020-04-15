#pragma once
#ifndef CATA_SRC_GAMEMODE_H
#define CATA_SRC_GAMEMODE_H

#include <memory>
#include <string>

#include "enums.h"

enum action_id : int;
struct special_game;

std::string special_game_name( special_game_id id );
std::unique_ptr<special_game> get_special_game( special_game_id id );

struct special_game {
    virtual ~special_game() = default;
    virtual special_game_id id() {
        return SGAME_NULL;
    }
    // Run when the game begins
    virtual bool init() {
        return true;
    }
    // Run every turn--before any player actions
    virtual void per_turn() { }
    // Run after a keypress, but before the game handles the action
    // It may modify the action, e.g. to cancel it
    virtual void pre_action( action_id & ) { }
    // Run after the game handles the action
    virtual void post_action( action_id ) { }
    // Run when the player dies (or the game otherwise ends)
    virtual void game_over() { }

};

#endif // CATA_SRC_GAMEMODE_H
