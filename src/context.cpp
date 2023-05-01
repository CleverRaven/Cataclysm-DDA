#include "context.h"

context::context()
{
    clear();
}
context::~context() = default;

void context::clear( )
{
    caster_level_adjustment = 0;
    caster_level_adjustment_by_spell.clear();
    caster_level_adjustment_by_school.clear();
}


