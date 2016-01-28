#ifndef OVERMAP_TYPES_H
#define OVERMAP_TYPES_H

class scent_trace
{
    public:
        // Default constructor makes an invalid trace.
        scent_trace() : creation_turn( -1 ), initial_strength( 0 ) {};
        scent_trace( int turn, int strength ) : creation_turn( turn ), initial_strength( strength ) {}
        // TODO: give these accessors so they can become private and immutable.
        int creation_turn; // Timestamp when scent trace was created.
        int initial_strength; // Original strength, doesn't weaken, it's just adjusted by age.
};

#endif
