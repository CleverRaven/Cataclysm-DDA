#pragma once
#ifndef MONEXAMINE_H
#define MONEXAMINE_H

class monster;

namespace monexamine
{
/*
*Manages the milking and milking cool down of monsters.
*Milked item uses starting_ammo, where ammo type is the milked item
*and amount the times per day you can milk the monster.
*/
void milk_source( monster &source_mon );
}
#endif
