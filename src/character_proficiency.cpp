#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "cached_options.h"
#include "calendar.h"
#include "character.h"
#include "enums.h"
#include "event.h"
#include "event_bus.h"
#include "pimpl.h"
#include "proficiency.h"
#include "translations.h"
#include "type_id.h"

bool Character::has_proficiency( const proficiency_id &prof ) const
{
    return _proficiencies->has_learned( prof );
}

float Character::get_proficiency_practice( const proficiency_id &prof ) const
{
    return _proficiencies->pct_practiced( prof );
}

time_duration Character::get_proficiency_practiced_time( const proficiency_id &prof ) const
{
    return _proficiencies->pct_practiced_time( prof );
}

void Character::set_proficiency_practiced_time( const proficiency_id &prof, int turns )
{
    if( turns < 0 ) {
        _proficiencies->remove( prof );
        return;
    }
    _proficiencies->set_time_practiced( prof, time_duration::from_turns( turns ) );
}

bool Character::has_prof_prereqs( const proficiency_id &prof ) const
{
    return _proficiencies->has_prereqs( prof );
}

void Character::add_proficiency( const proficiency_id &prof, bool ignore_requirements )
{
    if( ignore_requirements ) {
        _proficiencies->direct_learn( prof );
        return;
    }
    _proficiencies->learn( prof );
}

void Character::lose_proficiency( const proficiency_id &prof, bool ignore_requirements )
{
    if( ignore_requirements ) {
        _proficiencies->direct_remove( prof );
        return;
    }
    _proficiencies->remove( prof );
}

std::vector<display_proficiency> Character::display_proficiencies() const
{
    return _proficiencies->display();
}

bool Character::practice_proficiency( const proficiency_id &prof, const time_duration &amount,
                                      const std::optional<time_duration> &max )
{
    // Proficiencies can ignore focus using the `ignore_focus` JSON property
    const bool ignore_focus = prof->ignore_focus();
    float focus_adjusted = adjust_for_focus( to_seconds<float>( amount ) );
    const time_duration &focused_amount = ignore_focus ? amount : time_duration::from_seconds(
            focus_adjusted );

    const float pct_before = _proficiencies->pct_practiced( prof );
    const bool learned = _proficiencies->practice( prof, focused_amount,
                         ignore_focus ? 0.f : std::fmod( focus_adjusted, 1.f ), max );
    const float pct_after = _proficiencies->pct_practiced( prof );

    // Drain focus if necessary
    if( !ignore_focus && pct_after > pct_before ) {
        focus_pool -= focus_pool / 100;
    }

    if( learned ) {
        get_event_bus().send<event_type::gains_proficiency>( getID(), prof );
        add_msg_if_player( m_good, _( "You are now proficient in %s!" ), prof->name() );
    }
    return learned;
}

time_duration Character::proficiency_training_needed( const proficiency_id &prof ) const
{
    return _proficiencies->training_time_needed( prof );
}

std::vector<proficiency_id> Character::known_proficiencies() const
{
    return _proficiencies->known_profs();
}

std::vector<proficiency_id> Character::learning_proficiencies() const
{
    return _proficiencies->learning_profs();
}

int Character::get_proficiency_bonus( const std::string &category,
                                      proficiency_bonus_type prof_bonus ) const
{
    return _proficiencies->get_proficiency_bonus( category, prof_bonus );
}

void Character::set_proficiency_practice( const proficiency_id &id, const time_duration &amount )
{
    if( !test_mode ) {
        return;
    }

    _proficiencies->set_time_practiced( id, amount );
}

std::vector<proficiency_id> Character::proficiencies_offered_to( const Character *guy ) const
{
    std::vector<proficiency_id> ret;
    for( const proficiency_id &known : known_proficiencies() ) {
        if( known->is_teachable() && ( !guy || !guy->has_proficiency( known ) ) ) {
            ret.push_back( known );
        }
    }
    return ret;
}
