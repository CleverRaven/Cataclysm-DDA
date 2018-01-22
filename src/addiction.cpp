#include "addiction.h"
#include "debug.h"
#include "effect.h"
#include "pldata.h"
#include "player.h"
#include "morale_types.h"
#include "rng.h"
#include "translations.h"

const efftype_id effect_add_cig( "add_cig" );
const efftype_id effect_add_caffeine( "add_caffeine" );
const efftype_id effect_add_alcohol( "add_alcohol" );
const efftype_id effect_add_sleep( "add_sleep" );
const efftype_id effect_add_diazepam( "add_diazepam" );
const efftype_id effect_add_pkiller( "add_pkiller" );
const efftype_id effect_add_speed( "add_speed" );
const efftype_id effect_add_coke( "add_coke" );
const efftype_id effect_add_crack( "add_crack" );
const efftype_id effect_add_mutagen( "add_mutagen" );
const efftype_id effect_add_marloss_r( "add_marloss_r" );
const efftype_id effect_add_marloss_b( "add_marloss_b" );
const efftype_id effect_add_marloss_y( "add_marloss_y" );
const efftype_id effect_hallu( "hallu" );
const efftype_id effect_shakes( "shakes" );

/**
 * Returns the name of an addiction. It should be able to finish the sentence
 * "Became addicted to ______".
 */
const std::string &addiction_type_name( add_type const cur )
{
    static const std::map<add_type, std::string> type_map = {{
            { ADD_CIG, _( "nicotine" ) },
            { ADD_CAFFEINE, _( "caffeine" ) },
            { ADD_ALCOHOL, _( "alcohol" ) },
            { ADD_SLEEP, _( "sleeping pills" ) },
            { ADD_PKILLER, _( "opiates" ) },
            { ADD_SPEED, _( "amphetamine" ) },
            { ADD_COKE, _( "cocaine" ) },
            { ADD_CRACK, _( "crack cocaine" ) },
            { ADD_MUTAGEN, _( "mutation" ) },
            { ADD_DIAZEPAM, _( "diazepam" ) },
            { ADD_MARLOSS_R, _( "Marloss berries" ) },
            { ADD_MARLOSS_B, _( "Marloss seeds" ) },
            { ADD_MARLOSS_Y, _( "Marloss gel" ) },
        }
    };

    const auto iter = type_map.find( cur );
    if( iter != type_map.end() ) {
        return iter->second;
    }

    static const std::string error_string( "bugs in addiction.cpp" );
    return error_string;
}

morale_type addiction_craving( add_type const cur )
{
    static const std::map<add_type, morale_type> type_map = {{
            { ADD_CIG, MORALE_CRAVING_NICOTINE },
            { ADD_CAFFEINE, MORALE_CRAVING_CAFFEINE },
            { ADD_ALCOHOL, MORALE_CRAVING_ALCOHOL },
            { ADD_PKILLER, MORALE_CRAVING_OPIATE },
            { ADD_SPEED, MORALE_CRAVING_SPEED },
            { ADD_COKE, MORALE_CRAVING_COCAINE },
            { ADD_CRACK, MORALE_CRAVING_CRACK },
            { ADD_MUTAGEN, MORALE_CRAVING_MUTAGEN },
            { ADD_DIAZEPAM, MORALE_CRAVING_DIAZEPAM },
            { ADD_MARLOSS_R, MORALE_CRAVING_MARLOSS },
            { ADD_MARLOSS_B, MORALE_CRAVING_MARLOSS },
            { ADD_MARLOSS_Y, MORALE_CRAVING_MARLOSS },
        }
    };

    const auto iter = type_map.find( cur );
    if( iter != type_map.end() ) {
        return iter->second;
    }

    return MORALE_NULL;
}

add_type addiction_type( std::string const &name )
{
    static const std::map<std::string, add_type> type_map = {{
            { "nicotine", ADD_CIG },
            { "caffeine", ADD_CAFFEINE },
            { "alcohol", ADD_ALCOHOL },
            { "sleeping pill", ADD_SLEEP },
            { "opiate", ADD_PKILLER },
            { "amphetamine", ADD_SPEED },
            { "cocaine", ADD_COKE },
            { "crack", ADD_CRACK },
            { "mutagen", ADD_MUTAGEN },
            { "diazepam", ADD_DIAZEPAM },
            { "marloss_r", ADD_MARLOSS_R },
            { "marloss_b", ADD_MARLOSS_B },
            { "marloss_y", ADD_MARLOSS_Y },
            { "none", ADD_NULL }
        }
    };

    const auto iter = type_map.find( name );
    if( iter != type_map.end() ) {
        return iter->second;
    }

    return ADD_NULL;
}

const efftype_id &addiction_effect( add_type add )
{
    static const std::map<add_type, efftype_id> type_map = {{
            { ADD_CIG, effect_add_cig },
            { ADD_CAFFEINE, effect_add_caffeine },
            { ADD_ALCOHOL, effect_add_alcohol },
            { ADD_SLEEP, effect_add_sleep },
            { ADD_PKILLER, effect_add_diazepam },
            { ADD_SPEED, effect_add_speed },
            { ADD_COKE, effect_add_coke },
            { ADD_CRACK, effect_add_crack },
            { ADD_MUTAGEN, effect_add_mutagen },
            { ADD_DIAZEPAM, effect_add_diazepam },
            { ADD_MARLOSS_R, effect_add_marloss_r },
            { ADD_MARLOSS_B, effect_add_marloss_b },
            { ADD_MARLOSS_Y, effect_add_marloss_y },
        }
    };

    const auto iter = type_map.find( add );
    if( iter != type_map.end() ) {
        return iter->second;
    }

    debugmsg( "Invalid addiction type enum %d", add );
    return efftype_id::NULL_ID();
}
