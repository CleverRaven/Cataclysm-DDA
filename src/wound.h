#ifndef _WOUND_H_
#define _WOUND_H_

#include "pldata.h"
#include "creature.h"
#include "json.h"

class wound;
class Creature;

class wound : public JsonSerializer, public JsonDeserializer
{
    public:
        wound();
    protected:
        int duration;
        int heal_mod;
        int severity;
        body_part bp;
        int side;
}