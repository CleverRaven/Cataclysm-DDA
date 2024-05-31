#include "morale_types.h"

#include <cstddef>
#include <set>
#include <vector>

#include "generic_factory.h"
#include "itype.h"
#include "json.h"
#include "string_formatter.h"
#include "debug.h"

const morale_type MORALE_NULL( "morale_null" );

static const morale_type morale_accomplishment( "morale_accomplishment" );
static const morale_type morale_antifruit( "morale_antifruit" );
static const morale_type morale_antijunk( "morale_antijunk" );
static const morale_type morale_antiwheat( "morale_antiwheat" );
static const morale_type morale_ate_with_table( "morale_ate_with_table" );
static const morale_type morale_ate_without_table( "morale_ate_without_table" );
static const morale_type morale_book( "morale_book" );
static const morale_type morale_butcher( "morale_butcher" );
static const morale_type morale_cannibal( "morale_cannibal" );
static const morale_type morale_chat( "morale_chat" );
static const morale_type morale_cold( "morale_cold" );
static const morale_type morale_comfy( "morale_comfy" );
static const morale_type morale_craving_alcohol( "morale_craving_alcohol" );
static const morale_type morale_craving_caffeine( "morale_craving_caffeine" );
static const morale_type morale_craving_cocaine( "morale_craving_cocaine" );
static const morale_type morale_craving_crack( "morale_craving_crack" );
static const morale_type morale_craving_diazepam( "morale_craving_diazepam" );
static const morale_type morale_craving_marloss( "morale_craving_marloss" );
static const morale_type morale_craving_mutagen( "morale_craving_mutagen" );
static const morale_type morale_craving_nicotine( "morale_craving_nicotine" );
static const morale_type morale_craving_opiate( "morale_craving_opiate" );
static const morale_type morale_craving_speed( "morale_craving_speed" );
static const morale_type morale_dried_off( "morale_dried_off" );
static const morale_type morale_failure( "morale_failure" );
static const morale_type morale_feeling_bad( "morale_feeling_bad" );
static const morale_type morale_feeling_good( "morale_feeling_good" );
static const morale_type morale_food_bad( "morale_food_bad" );
static const morale_type morale_food_good( "morale_food_good" );
static const morale_type morale_food_hot( "morale_food_hot" );
static const morale_type morale_funeral( "morale_funeral" );
static const morale_type morale_game( "morale_game" );
static const morale_type morale_game_found_kitten( "morale_game_found_kitten" );
static const morale_type morale_gravedigger( "morale_gravedigger" );
static const morale_type morale_haircut( "morale_haircut" );
static const morale_type morale_honey( "morale_honey" );
static const morale_type morale_hot( "morale_hot" );
static const morale_type morale_killed_friend( "morale_killed_friend" );
static const morale_type morale_killed_innocent( "morale_killed_innocent" );
static const morale_type morale_killed_monster( "morale_killed_monster" );
static const morale_type morale_killer_has_killed( "morale_killer_has_killed" );
static const morale_type morale_killer_need_to_kill( "morale_killer_need_to_kill" );
static const morale_type morale_lactose( "morale_lactose" );
static const morale_type morale_marloss( "morale_marloss" );
static const morale_type morale_meatarian( "morale_meatarian" );
static const morale_type morale_moodswing( "morale_moodswing" );
static const morale_type morale_music( "morale_music" );
static const morale_type morale_mutagen( "morale_mutagen" );
static const morale_type morale_mutagen_chimera( "morale_mutagen_chimera" );
static const morale_type morale_mutagen_elf( "morale_mutagen_elf" );
static const morale_type morale_mutagen_mutation( "morale_mutagen_mutation" );
static const morale_type morale_mutilate_corpse( "morale_mutilate_corpse" );
static const morale_type morale_no_digest( "morale_no_digest" );
static const morale_type morale_null( "morale_null" );
static const morale_type morale_perm_badtemper( "morale_perm_badtemper" );
static const morale_type morale_perm_constrained( "morale_perm_constrained" );
static const morale_type morale_perm_fancy( "morale_perm_fancy" );
static const morale_type morale_perm_filthy( "morale_perm_filthy" );
static const morale_type morale_perm_hoarder( "morale_perm_hoarder" );
static const morale_type morale_perm_masochist( "morale_perm_masochist" );
static const morale_type morale_perm_nomad( "morale_perm_nomad" );
static const morale_type morale_perm_numb( "morale_perm_numb" );
static const morale_type morale_perm_optimist( "morale_perm_optimist" );
static const morale_type morale_photos( "morale_photos" );
static const morale_type morale_play_with_pet( "morale_play_with_pet" );
static const morale_type morale_pyromania_nearfire( "morale_pyromania_nearfire" );
static const morale_type morale_pyromania_nofire( "morale_pyromania_nofire" );
static const morale_type morale_pyromania_startfire( "morale_pyromania_startfire" );
static const morale_type morale_scream( "morale_scream" );
static const morale_type morale_shave( "morale_shave" );
static const morale_type morale_support( "morale_support" );
static const morale_type morale_sweettooth( "morale_sweettooth" );
static const morale_type morale_vegetarian( "morale_vegetarian" );
static const morale_type morale_vomited( "morale_vomited" );
static const morale_type morale_wet( "morale_wet" );

const morale_type &morale_type_data::convert_legacy( int lmt )
{
    static const std::vector<morale_type> legacy_morale_types = {{
            morale_null,
            morale_food_good,
            morale_food_hot,
            morale_ate_with_table,
            morale_ate_without_table,
            morale_music,
            morale_honey,
            morale_game,
            morale_marloss,
            morale_mutagen,
            morale_feeling_good,
            morale_support,
            morale_photos,

            morale_craving_nicotine,
            morale_craving_caffeine,
            morale_craving_alcohol,
            morale_craving_opiate,
            morale_craving_speed,
            morale_craving_cocaine,
            morale_craving_crack,
            morale_craving_mutagen,
            morale_craving_diazepam,
            morale_craving_marloss,

            morale_food_bad,
            morale_cannibal,
            morale_vegetarian,
            morale_meatarian,
            morale_antifruit,
            morale_lactose,
            morale_antijunk,
            morale_antiwheat,
            morale_sweettooth,
            morale_no_digest,
            morale_wet,
            morale_dried_off,
            morale_cold,
            morale_hot,
            morale_feeling_bad,
            morale_killed_innocent,
            morale_killed_friend,
            morale_killed_monster,
            morale_mutilate_corpse,
            morale_mutagen_elf,
            morale_mutagen_chimera,
            morale_mutagen_mutation,

            morale_moodswing,
            morale_book,
            morale_comfy,

            morale_scream,

            morale_perm_masochist,
            morale_perm_hoarder,
            morale_perm_fancy,
            morale_perm_optimist,
            morale_perm_badtemper,
            morale_perm_constrained,
            morale_perm_nomad,
            morale_game_found_kitten,

            morale_haircut,
            morale_shave,
            morale_chat,

            morale_vomited,

            morale_play_with_pet,

            morale_pyromania_startfire,
            morale_pyromania_nearfire,
            morale_pyromania_nofire,

            morale_killer_has_killed,
            morale_killer_need_to_kill,

            morale_perm_filthy,

            morale_butcher,
            morale_gravedigger,
            morale_funeral,

            morale_accomplishment,
            morale_failure,
            morale_perm_numb,

            morale_null
        }
    };

    if( lmt >= 0 && static_cast<size_t>( lmt ) <= legacy_morale_types.size() ) {
        return legacy_morale_types[ lmt ];
    }

    debugmsg( "Requested invalid legacy morale type %d", lmt );
    return legacy_morale_types.front();
}

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

void morale_type_data::load_type( const JsonObject &jo, const std::string &src )
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

void morale_type_data::load( const JsonObject &jo, const std::string_view )
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
