#pragma once
#ifndef OVERMAP_TYPES_H
#define OVERMAP_TYPES_H

#include "calendar.h"

class scent_trace
{
    public:
        // Default constructor makes an invalid trace.
        scent_trace() : creation_time( calendar::before_time_starts ), initial_strength( 0 ) {}
        scent_trace( const time_point &ct, int strength ) : creation_time( ct ),
            initial_strength( strength ) {}
        // TODO: give these accessors so they can become private and immutable.
        time_point creation_time; // Timestamp when scent trace was created.
        int initial_strength; // Original strength, doesn't weaken, it's just adjusted by age.
};

#endif
