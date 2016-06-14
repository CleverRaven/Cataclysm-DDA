#include "npc_class.h"
#include "debug.h"
#include "rng.h"
#include "generic_factory.h"

#include <list>

static const std::array<npc_class_id, 17> legacy_ids = {{
        npc_class_id( "NC_NONE" ),
        npc_class_id( "NC_EVAC_SHOPKEEP" ),  // Found in the Evacuation Center, unique, has more goods than he should be able to carry
        npc_class_id( "NC_SHOPKEEP" ),       // Found in towns.  Stays in his shop mostly.
        npc_class_id( "NC_HACKER" ),         // Weak in combat but has hacking skills and equipment
        npc_class_id( "NC_DOCTOR" ),         // Found in towns, or roaming.  Stays in the clinic.
        npc_class_id( "NC_TRADER" ),         // Roaming trader, journeying between towns.
        npc_class_id( "NC_NINJA" ),          // Specializes in unarmed combat, carries few items
        npc_class_id( "NC_COWBOY" ),         // Gunslinger and survivalist
        npc_class_id( "NC_SCIENTIST" ),      // Uses intelligence-based skills and high-tech items
        npc_class_id( "NC_BOUNTY_HUNTER" ),  // Resourceful and well-armored
        npc_class_id( "NC_THUG" ),           // Moderate melee skills and poor equipment
        npc_class_id( "NC_SCAVENGER" ),      // Good with pistols light weapons
        npc_class_id( "NC_ARSONIST" ),       // Evacuation Center, restocks moltovs and anarcist type stuff
        npc_class_id( "NC_HUNTER" ),         // Survivor type good with bow or rifle
        npc_class_id( "NC_SOLDIER" ),        // Well equiped and trained combatant, good with rifles and melee
        npc_class_id( "NC_BARTENDER" ),      // Stocks alcohol
        npc_class_id( "NC_JUNK_SHOPKEEP" )   // Stocks wide range of items...
    }
};

npc_class_id NC_NONE( "NC_NONE" );
npc_class_id NC_EVAC_SHOPKEEP( "NC_EVAC_SHOPKEEP" );
npc_class_id NC_SHOPKEEP( "NC_SHOPKEEP" );
npc_class_id NC_HACKER( "NC_HACKER" );
npc_class_id NC_DOCTOR( "NC_DOCTOR" );
npc_class_id NC_TRADER( "NC_TRADER" );
npc_class_id NC_NINJA( "NC_NINJA" );
npc_class_id NC_COWBOY( "NC_COWBOY" );
npc_class_id NC_SCIENTIST( "NC_SCIENTIST" );
npc_class_id NC_BOUNTY_HUNTER( "NC_BOUNTY_HUNTER" );
npc_class_id NC_THUG( "NC_THUG" );
npc_class_id NC_SCAVENGER( "NC_SCAVENGER" );
npc_class_id NC_ARSONIST( "NC_ARSONIST" );
npc_class_id NC_HUNTER( "NC_HUNTER" );
npc_class_id NC_SOLDIER( "NC_SOLDIER" );
npc_class_id NC_BARTENDER( "NC_BARTENDER" );
npc_class_id NC_JUNK_SHOPKEEP( "NC_JUNK_SHOPKEEP" );

generic_factory<npc_class> npc_class_factory( "npc_class" );

template<>
const npc_class_id string_id<npc_class>::NULL_ID( "NC_NONE" );

template<>
const npc_class &string_id<npc_class>::obj() const
{
    return npc_class_factory.obj( *this );
}

template<>
bool string_id<npc_class>::is_valid() const
{
    return npc_class_factory.is_valid( *this );
}

npc_class::npc_class() : id( NC_NONE )
{
}

void npc_class::load_npc_class( JsonObject &jo )
{
    npc_class_factory.load( jo );
}

void npc_class::reset_npc_classes()
{
    npc_class_factory.reset();
}

void npc_class::check_consistency()
{
    for( const auto &legacy : legacy_ids ) {
        if( !npc_class_factory.is_valid( legacy ) ) {
            debugmsg( "Missing legacy npc class %s", legacy.c_str() );
        }
    }
}

void npc_class::load( JsonObject &jo )
{
    mandatory( jo, was_loaded, "name", name, translated_string_reader );
}

const npc_class_id &npc_class::from_legacy_int( int i )
{
    if( i < 0 || ( size_t )i >= legacy_ids.size() ) {
        debugmsg( "Invalid legacy class id: %d", i );
        return NULL_ID;
    }

    return legacy_ids[ i ];
}

const npc_class_id &npc_class::random_common()
{
    // @todo Rewrite, make `common` a class member
    std::list<const npc_class_id *> common_classes;
    for( const auto &pr : npc_class_factory.get_all() ) {
        if( pr.id != NC_SHOPKEEP ) {
            common_classes.push_back( &pr.id );
        }
    }

    if( common_classes.empty() ) {
        return NC_NONE;
    }

    return *random_entry( common_classes );
}

const std::string &npc_class::get_name() const
{
    return name;
}
