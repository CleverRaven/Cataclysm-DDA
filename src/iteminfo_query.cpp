#include "iteminfo_query.h"

#include <string>
#include <vector>

iteminfo_query::iteminfo_query() = default;

iteminfo_query::iteminfo_query( const std::string &bits ) : iteminfo_query_base( bits )
{
}

iteminfo_query::iteminfo_query( const std::vector<iteminfo_parts> &setBits )
{
    for( auto &bit : setBits ) {
        set( static_cast<size_t>( bit ) );
    }
}

iteminfo_query::iteminfo_query( const iteminfo_query_base &values )
    : iteminfo_query_base( values )
{
}

bool iteminfo_query::test( const iteminfo_parts &value ) const
{
    return iteminfo_query_base::test( static_cast<size_t>( value ) );
}

const iteminfo_query iteminfo_query::all = iteminfo_query(
            std::string( static_cast<size_t>( iteminfo_parts::NUM_VALUES ), '1' ) );

const iteminfo_query iteminfo_query::notext = iteminfo_query(
            iteminfo_query::all & ~iteminfo_query(
std::vector<iteminfo_parts> {
    iteminfo_parts::DESCRIPTION,
    iteminfo_parts::DESCRIPTION_TECHNIQUES,
    iteminfo_parts::DESCRIPTION_GUNMOD_ADDREACHATTACK,
    iteminfo_parts::DESCRIPTION_MELEEDMG,
    iteminfo_parts::DESCRIPTION_MELEEDMG_CRIT,
    iteminfo_parts::DESCRIPTION_MELEEDMG_BASH,
    iteminfo_parts::DESCRIPTION_MELEEDMG_CUT,
    iteminfo_parts::DESCRIPTION_MELEEDMG_PIERCE,
    iteminfo_parts::DESCRIPTION_MELEEDMG_MOVES,
    iteminfo_parts::DESCRIPTION_APPLICABLEMARTIALARTS,
    iteminfo_parts::DESCRIPTION_REPAIREDWITH,
    iteminfo_parts::DESCRIPTION_CONDUCTIVITY,
    iteminfo_parts::DESCRIPTION_FLAGS,
    iteminfo_parts::DESCRIPTION_FLAGS_HELMETCOMPAT,
    iteminfo_parts::DESCRIPTION_FLAGS_FITS,
    iteminfo_parts::DESCRIPTION_FLAGS_VARSIZE,
    iteminfo_parts::DESCRIPTION_FLAGS_SIDED,
    iteminfo_parts::DESCRIPTION_FLAGS_POWERARMOR,
    iteminfo_parts::DESCRIPTION_FLAGS_POWERARMOR_RADIATIONHINT,
    iteminfo_parts::DESCRIPTION_IRRADIATION,
    iteminfo_parts::DESCRIPTION_RECHARGE_UPSMODDED,
    iteminfo_parts::DESCRIPTION_RECHARGE_NORELOAD,
    iteminfo_parts::DESCRIPTION_RECHARGE_UPSCAPABLE,
    iteminfo_parts::DESCRIPTION_RADIO_ACTIVATION,
    iteminfo_parts::DESCRIPTION_RADIO_ACTIVATION_CHANNEL,
    iteminfo_parts::DESCRIPTION_RADIO_ACTIVATION_PROC,
    iteminfo_parts::DESCRIPTION_CBM_SLOTS,
    iteminfo_parts::DESCRIPTION_TWOHANDED,
    iteminfo_parts::DESCRIPTION_GUNMOD_DISABLESSIGHTS,
    iteminfo_parts::DESCRIPTION_GUNMOD_CONSUMABLE,
    iteminfo_parts::DESCRIPTION_RADIOACTIVITY_DAMAGED,
    iteminfo_parts::DESCRIPTION_RADIOACTIVITY_ALWAYS,
    iteminfo_parts::DESCRIPTION_BREWABLE_DURATION,
    iteminfo_parts::DESCRIPTION_BREWABLE_PRODUCTS,
    iteminfo_parts::DESCRIPTION_FAULTS,
    iteminfo_parts::DESCRIPTION_HOLSTERS,
    iteminfo_parts::DESCRIPTION_ACTIVATABLE_TRANSFORMATION,
    iteminfo_parts::DESCRIPTION_NOTES,
    iteminfo_parts::DESCRIPTION_CONTENTS,
    iteminfo_parts::DESCRIPTION_APPLICABLE_RECIPES,
    iteminfo_parts::DESCRIPTION_MED_ADDICTING
} ) );

const iteminfo_query iteminfo_query::anyflags = iteminfo_query(
std::vector<iteminfo_parts> {
    iteminfo_parts::DESCRIPTION_FLAGS,
    iteminfo_parts::DESCRIPTION_FLAGS_HELMETCOMPAT,
    iteminfo_parts::DESCRIPTION_FLAGS_FITS,
    iteminfo_parts::DESCRIPTION_FLAGS_VARSIZE,
    iteminfo_parts::DESCRIPTION_FLAGS_SIDED,
    iteminfo_parts::DESCRIPTION_FLAGS_POWERARMOR,
    iteminfo_parts::DESCRIPTION_FLAGS_POWERARMOR_RADIATIONHINT,
    iteminfo_parts::DESCRIPTION_IRRADIATION
} );
