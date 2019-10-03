#include "morale_types.h"

#include <cstddef>
#include <set>
#include <vector>

#include "generic_factory.h"
#include "itype.h"
#include "json.h"
#include "string_formatter.h"
#include "debug.h"

const morale_type &morale_type_data::convert_legacy( int lmt )
{
    static const std::vector<morale_type> legacy_morale_types = {{
            morale_type( "morale_null" ),
            morale_type( "morale_food_good" ),
            morale_type( "morale_food_hot" ),
            morale_type( "morale_ate_with_table" ),
            morale_type( "morale_ate_without_table" ),
            morale_type( "morale_music" ),
            morale_type( "morale_honey" ),
            morale_type( "morale_game" ),
            morale_type( "morale_marloss" ),
            morale_type( "morale_mutagen" ),
            morale_type( "morale_feeling_good" ),
            morale_type( "morale_support" ),
            morale_type( "morale_photos" ),

            morale_type( "morale_craving_nicotine" ),
            morale_type( "morale_craving_caffeine" ),
            morale_type( "morale_craving_alcohol" ),
            morale_type( "morale_craving_opiate" ),
            morale_type( "morale_craving_speed" ),
            morale_type( "morale_craving_cocaine" ),
            morale_type( "morale_craving_crack" ),
            morale_type( "morale_craving_mutagen" ),
            morale_type( "morale_craving_diazepam" ),
            morale_type( "morale_craving_marloss" ),

            morale_type( "morale_food_bad" ),
            morale_type( "morale_cannibal" ),
            morale_type( "morale_vegetarian" ),
            morale_type( "morale_meatarian" ),
            morale_type( "morale_antifruit" ),
            morale_type( "morale_lactose" ),
            morale_type( "morale_antijunk" ),
            morale_type( "morale_antiwheat" ),
            morale_type( "morale_sweettooth" ),
            morale_type( "morale_no_digest" ),
            morale_type( "morale_wet" ),
            morale_type( "morale_dried_off" ),
            morale_type( "morale_cold" ),
            morale_type( "morale_hot" ),
            morale_type( "morale_feeling_bad" ),
            morale_type( "morale_killed_innocent" ),
            morale_type( "morale_killed_friend" ),
            morale_type( "morale_killed_monster" ),
            morale_type( "morale_mutilate_corpse" ),
            morale_type( "morale_mutagen_elf" ),
            morale_type( "morale_mutagen_chimera" ),
            morale_type( "morale_mutagen_mutation" ),

            morale_type( "morale_moodswing" ),
            morale_type( "morale_book" ),
            morale_type( "morale_sit" ),
            morale_type( "morale_comfy" ),

            morale_type( "morale_scream" ),

            morale_type( "morale_perm_masochist" ),
            morale_type( "morale_perm_hoarder" ),
            morale_type( "morale_perm_fancy" ),
            morale_type( "morale_perm_optimist" ),
            morale_type( "morale_perm_badtemper" ),
            morale_type( "morale_perm_constrained" ),
            morale_type( "morale_perm_nomad" ),
            morale_type( "morale_game_found_kitten" ),

            morale_type( "morale_haircut" ),
            morale_type( "morale_shave" ),
            morale_type( "morale_chat" ),

            morale_type( "morale_vomited" ),

            morale_type( "morale_play_with_pet" ),

            morale_type( "morale_pyromania_startfire" ),
            morale_type( "morale_pyromania_nearfire" ),
            morale_type( "morale_pyromania_nofire" ),

            morale_type( "morale_killer_has_killed" ),
            morale_type( "morale_killer_need_to_kill" ),

            morale_type( "morale_perm_filthy" ),

            morale_type( "morale_butcher" ),
            morale_type( "morale_gravedigger" ),
            morale_type( "morale_funeral" ),

            morale_type( "morale_accomplishment" ),
            morale_type( "morale_failure" ),

            morale_type( "morale_null" )
        }
    };

    if( lmt >= 0 && static_cast<size_t>( lmt ) <= legacy_morale_types.size() ) {
        return legacy_morale_types[ lmt ];
    }

    debugmsg( "Requested invalid legacy morale type %d", lmt );
    return legacy_morale_types.front();
}

// Legacy crap
const morale_type MORALE_NULL( "morale_null" );
const morale_type MORALE_FOOD_GOOD( "morale_food_good" );
const morale_type MORALE_FOOD_HOT( "morale_food_hot" );
const morale_type MORALE_ATE_WITH_TABLE( "morale_ate_with_table" );
const morale_type MORALE_ATE_WITHOUT_TABLE( "morale_ate_without_table" );
const morale_type MORALE_MUSIC( "morale_music" );
const morale_type MORALE_HONEY( "morale_honey" );
const morale_type MORALE_GAME( "morale_game" );
const morale_type MORALE_MARLOSS( "morale_marloss" );
const morale_type MORALE_MUTAGEN( "morale_mutagen" );
const morale_type MORALE_FEELING_GOOD( "morale_feeling_good" );
const morale_type MORALE_SUPPORT( "morale_support" );
const morale_type MORALE_PHOTOS( "morale_photos" );
const morale_type MORALE_CRAVING_NICOTINE( "morale_craving_nicotine" );
const morale_type MORALE_CRAVING_CAFFEINE( "morale_craving_caffeine" );
const morale_type MORALE_CRAVING_ALCOHOL( "morale_craving_alcohol" );
const morale_type MORALE_CRAVING_OPIATE( "morale_craving_opiate" );
const morale_type MORALE_CRAVING_SPEED( "morale_craving_speed" );
const morale_type MORALE_CRAVING_COCAINE( "morale_craving_cocaine" );
const morale_type MORALE_CRAVING_CRACK( "morale_craving_crack" );
const morale_type MORALE_CRAVING_MUTAGEN( "morale_craving_mutagen" );
const morale_type MORALE_CRAVING_DIAZEPAM( "morale_craving_diazepam" );
const morale_type MORALE_CRAVING_MARLOSS( "morale_craving_marloss" );
const morale_type MORALE_FOOD_BAD( "morale_food_bad" );
const morale_type MORALE_CANNIBAL( "morale_cannibal" );
const morale_type MORALE_VEGETARIAN( "morale_vegetarian" );
const morale_type MORALE_MEATARIAN( "morale_meatarian" );
const morale_type MORALE_ANTIFRUIT( "morale_antifruit" );
const morale_type MORALE_LACTOSE( "morale_lactose" );
const morale_type MORALE_ANTIJUNK( "morale_antijunk" );
const morale_type MORALE_ANTIWHEAT( "morale_antiwheat" );
const morale_type MORALE_SWEETTOOTH( "morale_sweettooth" );
const morale_type MORALE_NO_DIGEST( "morale_no_digest" );
const morale_type MORALE_WET( "morale_wet" );
const morale_type MORALE_DRIED_OFF( "morale_dried_off" );
const morale_type MORALE_COLD( "morale_cold" );
const morale_type MORALE_HOT( "morale_hot" );
const morale_type MORALE_FEELING_BAD( "morale_feeling_bad" );
const morale_type MORALE_KILLED_INNOCENT( "morale_killed_innocent" );
const morale_type MORALE_KILLED_FRIEND( "morale_killed_friend" );
const morale_type MORALE_KILLED_MONSTER( "morale_killed_monster" );
const morale_type MORALE_MUTILATE_CORPSE( "morale_mutilate_corpse" );
const morale_type MORALE_MUTAGEN_ELF( "morale_mutagen_elf" );
const morale_type MORALE_MUTAGEN_CHIMERA( "morale_mutagen_chimera" );
const morale_type MORALE_MUTAGEN_MUTATION( "morale_mutagen_mutation" );
const morale_type MORALE_MOODSWING( "morale_moodswing" );
const morale_type MORALE_BOOK( "morale_book" );
const morale_type MORALE_SIT( "morale_sit" );
const morale_type MORALE_COMFY( "morale_comfy" );
const morale_type MORALE_SCREAM( "morale_scream" );
const morale_type MORALE_PERM_MASOCHIST( "morale_perm_masochist" );
const morale_type MORALE_PERM_NOFACE( "morale_perm_noface" );
const morale_type MORALE_PERM_FPMODE_ON( "morale_perm_fpmode_on" );
const morale_type MORALE_PERM_HOARDER( "morale_perm_hoarder" );
const morale_type MORALE_PERM_FANCY( "morale_perm_fancy" );
const morale_type MORALE_PERM_OPTIMIST( "morale_perm_optimist" );
const morale_type MORALE_PERM_BADTEMPER( "morale_perm_badtemper" );
const morale_type MORALE_PERM_CONSTRAINED( "morale_perm_constrained" );
const morale_type MORALE_PERM_NOMAD( "morale_perm_nomad" );
const morale_type MORALE_GAME_FOUND_KITTEN( "morale_game_found_kitten" );
const morale_type MORALE_HAIRCUT( "morale_haircut" );
const morale_type MORALE_SHAVE( "morale_shave" );
const morale_type MORALE_CHAT( "morale_chat" );
const morale_type MORALE_VOMITED( "morale_vomited" );
const morale_type MORALE_PLAY_WITH_PET( "morale_play_with_pet" );
const morale_type MORALE_PYROMANIA_STARTFIRE( "morale_pyromania_startfire" );
const morale_type MORALE_PYROMANIA_NEARFIRE( "morale_pyromania_nearfire" );
const morale_type MORALE_PYROMANIA_NOFIRE( "morale_pyromania_nofire" );
const morale_type MORALE_KILLER_HAS_KILLED( "morale_killer_has_killed" );
const morale_type MORALE_KILLER_NEED_TO_KILL( "morale_killer_need_to_kill" );
const morale_type MORALE_PERM_FILTHY( "morale_perm_filthy" );
const morale_type MORALE_PERM_DEBUG( "morale_perm_debug" );
const morale_type MORALE_BUTCHER( "morale_butcher" );
const morale_type MORALE_GRAVEDIGGER( "morale_gravedigger" );
const morale_type MORALE_FUNERAL( "morale_funeral" );
const morale_type MORALE_TREE_COMMUNION( "morale_tree_communion" );
const morale_type MORALE_ACCOMPLISHMENT( "morale_accomplishment" );
const morale_type MORALE_FAILURE( "morale_failure" );

namespace
{

generic_factory<morale_type_data> morale_data( "morale type" );

} // namespace

template<>
const morale_type_data &morale_type::obj() const
{
    return morale_data.obj( *this );
}

template<>
bool morale_type::is_valid() const
{
    return morale_data.is_valid( *this );
}

void morale_type_data::load_type( JsonObject &jo, const std::string &src )
{
    morale_data.load( jo, src );
}

void morale_type_data::check_all()
{
    morale_data.check();
}

void morale_type_data::reset()
{
    morale_data.reset();
}

void morale_type_data::load( JsonObject &jo, const std::string & )
{
    mandatory( jo, was_loaded, "id", id );
    mandatory( jo, was_loaded, "text", text );

    optional( jo, was_loaded, "permanent", permanent, false );
}

void morale_type_data::check() const
{
}

std::string morale_type_data::describe( const itype *it ) const
{
    if( it ) {
        return string_format( text, it->nname( 1 ) );
    } else {
        // if `msg` contains conversion specification (e.g. %s) but `it` is nullptr,
        // `string_format` will return an error message
        return string_format( text );
    }
}
