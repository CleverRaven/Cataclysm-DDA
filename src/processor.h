#pragma once
#ifndef PROCESSOR_H
#define PROCESSOR_H

#include "player.h"
#include "enums.h"
#include "json.h"

namespace processors
{

void load( JsonObject &jo, const std::string &src );
void check();
void reset();

/** interacting with the processor interface via player's activity */
void interact_with_processor( const tripoint &pos, player &p );
void interact_with_working_processor( const tripoint &pos, player &p );

};

namespace processes
{

void load( JsonObject &jo, const std::string &src );
void check();
void reset();

}

#endif
