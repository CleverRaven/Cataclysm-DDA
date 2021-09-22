#pragma once
#ifndef CATA_SRC_MONDEATH_H
#define CATA_SRC_MONDEATH_H

class monster;

namespace mdeath
{
// Drop a body
void normal( monster &z );
// Overkill splatter (also part of normal under conditions)
void splatter( monster &z );
// Hallucination disappears
void disappear( monster &z );
// Broken robot drop
void broken( monster &z );
} //namespace mdeath

void make_mon_corpse( monster &z, int damageLvl );

#endif // CATA_SRC_MONDEATH_H
