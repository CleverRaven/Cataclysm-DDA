#include "effect_source.h"

#include <memory>

#include "character.h"
#include "creature.h"
#include "debug.h"
#include "faction.h"
#include "game.h"
#include "monster.h"
#include "npc.h"

effect_source::effect_source( const monster *mon )
{
    if( mon == nullptr ) {
        debugmsg( "effect_source constructor called with nullptr monster" );
        return;
    }
    this->mfac = std::optional<mfaction_id>( mon->faction );
}

effect_source::effect_source( const faction *fac )
{
    if( fac == nullptr ) {
        debugmsg( "effect_source constructor called with nullptr faction" );
        return;
    }
    this->fac = std::optional<faction_id>( fac->id );
}

effect_source::effect_source( const Character *character )
{
    if( character == nullptr ) {
        debugmsg( "effect_source constructor called with nullptr character" );
        return;
    }
    this->character = character->getID();
    this->fac = character->get_faction() != nullptr
                ? character->get_faction()->id : faction_id();
}

effect_source::effect_source( const Creature *creature )
{
    if( creature == nullptr ) {
        // TODO: most unassigned effects will fall here with nullptrs
        // once everything is assigned this warning can be enabled
        // debugmsg( "effect_source constructor called with nullptr creature" );
        return;
    }

    const Character *const character = creature->as_character();
    if( character != nullptr ) {
        this->character = character->getID();
        this->fac = character->get_faction() != nullptr
                    ? character->get_faction()->id : faction_id();
        return;
    }

    const monster *const monster = creature->as_monster();
    if( monster != nullptr ) {
        this->mfac = monster->faction;
        return;
    } else {
        debugmsg( "effect_source constructor couldn't extract anything from provided creature" );
        return;
    }
}

effect_source effect_source::empty()
{
    return effect_source();
}

std::optional<character_id> effect_source::get_character_id() const
{
    return this->character;
}

std::optional<faction_id> effect_source::get_faction_id() const
{
    return this->fac;
}

std::optional<mfaction_id> effect_source::get_mfaction_id() const
{
    return this->mfac;
}

Creature *effect_source::resolve_creature() const
{
    // if effect has source try figuring out who to credit
    const std::optional<character_id> eff_source = get_character_id();
    Creature *source_creature;
    if( !eff_source ) {
        source_creature = nullptr;
    } else if( eff_source.value() == get_player_character().getID() ) {
        source_creature = &get_player_character();
    } else {
        source_creature = g->find_npc( eff_source.value() );
    }

    return source_creature;
}
