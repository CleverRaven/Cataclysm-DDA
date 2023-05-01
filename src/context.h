#pragma once
#ifndef CATA_SRC_CONTEXT_H
#define CATA_SRC_CONTEXT_H

#include <map>
#include <string>

class context
{
    public:
        context();
        ~context();

        void clear();
        float caster_level_adjustment;
        std::map<std::string, float> caster_level_adjustment_by_spell;
        std::map<std::string, float> caster_level_adjustment_by_school;
};

context &get_context();

#endif // CATA_SRC_CONTEXT_H
